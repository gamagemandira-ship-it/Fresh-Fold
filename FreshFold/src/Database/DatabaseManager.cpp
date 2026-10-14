#include "DatabaseManager.h"
#include "../Models/OrderFactory.h"
#include "../Models/PriceList.h"
#include "../Utils/StringUtils.h"

#include <msclr/auto_handle.h>
#include <stdexcept>

using namespace System;
using namespace System::Data;
using namespace System::Globalization;
using namespace System::IO;
using namespace MySql::Data::MySqlClient;

namespace FreshFold
{
    // ========================================================================
    // Private helper functions (only used in this file)
    // ========================================================================

    // Runs a SELECT command and returns all rows in a DataTable.
    static DataTable^ fillTable(MySqlCommand^ command)
    {
        DataTable^ table = gcnew DataTable();
        MySqlDataAdapter adapter(command);
        adapter.Fill(table);
        return table;
    }

    // Builds a LIKE pattern "%text%" and escapes the LIKE wildcards % and _
    // so they are searched for literally.
    static String^ likePattern(String^ text)
    {
        String^ escaped = text->Replace("\\", "\\\\")->Replace("%", "\\%")->Replace("_", "\\_");
        return String::Concat("%", escaped, "%");
    }

    static Decimal toMoney(double amount)
    {
        return Math::Round(Decimal(amount), 2);
    }

    static DateTime toDate(const std::string& isoDate)
    {
        return DateTime::ParseExact(toManagedString(isoDate), "yyyy-MM-dd", CultureInfo::InvariantCulture);
    }

    static bool isMySqlError(Exception^ error, int errorNumber)
    {
        MySqlException^ mysqlError = dynamic_cast<MySqlException^>(error);
        return mysqlError != nullptr && mysqlError->Number == errorNumber;
    }

    // Converts any exception into a DatabaseException with a message that is
    // safe to show to the user. The full technical error goes to the Visual
    // Studio Output window only.
    static DatabaseException^ toDatabaseException(Exception^ error, String^ action)
    {
        DatabaseException^ alreadyFriendly = dynamic_cast<DatabaseException^>(error);
        if (alreadyFriendly != nullptr)
            return alreadyFriendly;

        System::Diagnostics::Debug::WriteLine(String::Concat("[FreshFold database error] ", error->ToString()));

        MySqlException^ mysqlError = dynamic_cast<MySqlException^>(error);
        if (mysqlError != nullptr)
        {
            switch (mysqlError->Number)
            {
            case 0:
            case 1042:
                return gcnew DatabaseException(
                    "Cannot connect to the MySQL server (or the connection was lost).\n"
                    "Check that MySQL is running and that config\\database.ini has the correct server and port.");
            case 1045:
                return gcnew DatabaseException("MySQL rejected the user name or password in config\\database.ini.");
            case 1049:
                return gcnew DatabaseException(
                    "The database named in config\\database.ini was not found.\n"
                    "Run database\\freshfold_db.sql in MySQL first.");
            case 1146:
                return gcnew DatabaseException("A required table is missing. Run database\\freshfold_db.sql to create the tables.");
            case 1044:
            case 1142:
                return gcnew DatabaseException("The MySQL account in config\\database.ini does not have permission for this operation.");
            case 1062:
                return gcnew DatabaseException(String::Concat(action, "\nA record with the same value already exists."));
            case 1217:
            case 1451:
                return gcnew DatabaseException(String::Concat(action, "\nOther records depend on this record."));
            case 1216:
            case 1452:
                return gcnew DatabaseException(String::Concat(action, "\nA related record does not exist (it may have been deleted)."));
            case 3819:
                return gcnew DatabaseException(String::Concat(action, "\nThe database rejected one of the values as invalid."));
            case 1205:
                return gcnew DatabaseException(String::Concat(action, "\nThe database was busy. Please try again."));
            default:
                return gcnew DatabaseException(String::Format("{0}\n(MySQL error {1})", action, mysqlError->Number));
            }
        }

        if (dynamic_cast<TimeoutException^>(error) != nullptr)
            return gcnew DatabaseException(String::Concat(action, "\nThe database did not respond in time."));

        return gcnew DatabaseException(String::Concat(action, "\nAn unexpected error occurred."));
    }

    // Column names (aliases) here are what the user sees as table headers.
    static String^ customerListSql(String^ whereClause)
    {
        return String::Concat(
            "SELECT customer_id AS `Customer ID`, full_name AS `Name`, phone AS `Contact Number`, "
            "address AS `Address`, DATE_FORMAT(created_at, '%Y-%m-%d') AS `Registered` "
            "FROM customers ",
            whereClause,
            " ORDER BY customer_id DESC");
    }

    static String^ orderListSql(String^ whereClause)
    {
        return String::Concat(
            "SELECT o.order_id AS `Order ID`, c.full_name AS `Customer`, c.phone AS `Contact Number`, "
            "o.service_type AS `Service`, o.quantity AS `Quantity`, o.total_amount AS `Total (Rs.)`, "
            "o.status AS `Status`, o.payment_status AS `Payment`, "
            "DATE_FORMAT(o.order_date, '%Y-%m-%d') AS `Order Date` "
            "FROM orders o JOIN customers c ON c.customer_id = o.customer_id ",
            whereClause,
            " ORDER BY o.order_id DESC");
    }

    // ========================================================================
    // Connection
    // ========================================================================

    DatabaseManager::DatabaseManager(const DatabaseConfig& config)
        : config(config)
    {
    }

    String^ DatabaseManager::buildConnectionString() const
    {
        // The builder escapes every value correctly, so special characters in
        // the password cannot break the connection string.
        MySqlConnectionStringBuilder^ builder = gcnew MySqlConnectionStringBuilder();
        builder->Server = toManagedString(config.server);
        builder->Port = static_cast<unsigned int>(config.port);
        builder->Database = toManagedString(config.database);
        builder->UserID = toManagedString(config.user);
        builder->Password = toManagedString(config.password);
        builder->CharacterSet = "utf8mb4";
        builder->ConnectionTimeout = 5;      // seconds - fail fast if MySQL is not running
        builder->DefaultCommandTimeout = 15;
        // UPDATE returns the number of rows MATCHED (not only rows changed), so
        // "0 rows" reliably means "record not found / rule not met".
        builder->UseAffectedRows = false;
        return builder->ConnectionString;
    }

    void DatabaseManager::testConnection() const
    {
        try
        {
            // "Stack semantics": the connection is disposed (closed) automatically
            // when this block ends, even if an exception is thrown.
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            // If the users table is missing, attempt to create the schema from the
            // repository SQL script so the application can start for first-time use.
            try
            {
                MySqlCommand check("SHOW TABLES LIKE 'users'", %connection);
                Object^ result = check.ExecuteScalar();
                if (result == nullptr)
                {
                    // Search for the SQL script in the exe folder and up to 4 parents.
                    String^ folder = AppDomain::CurrentDomain->BaseDirectory;
                    String^ scriptPath = nullptr;
                    for (int level = 0; level < 5 && folder != nullptr; level++)
                    {
                        String^ candidate = Path::Combine(folder, "database", "freshfold_db.sql");
                        if (File::Exists(candidate))
                        {
                            scriptPath = candidate;
                            break;
                        }

                        DirectoryInfo^ parent = Directory::GetParent(folder->TrimEnd(gcnew array<wchar_t>{ L'\\', L'/' }));
                        if (parent == nullptr)
                            break;
                        folder = parent->FullName;
                    }

                    if (scriptPath != nullptr)
                    {
                        String^ sql = File::ReadAllText(scriptPath);
                        // Split by ';' and execute each statement.
                        array<String^>^ parts = sql->Split(gcnew array<wchar_t>{ ';' });
                        for each (String^ part in parts)
                        {
                            String^ stmt = part->Trim();
                            if (String::IsNullOrWhiteSpace(stmt))
                                continue;
                            // Skip SQL comments that occupy the whole line (start with --)
                            if (stmt->StartsWith("--"))
                                continue;
                            MySqlCommand command(stmt, %connection);
                            command.ExecuteNonQuery();
                        }
                    }
                }
            }
            catch (Exception^)
            {
                // Ignore schema creation errors here; the outer catch will produce
                // a friendly DatabaseException with details.
            }
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not connect to the database.");
        }
    }

    // ========================================================================
    // Customers
    // ========================================================================

    int DatabaseManager::addCustomer(Customer& customer) const
    {
        int newId = 0;
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();

            MySqlCommand command(
                "INSERT INTO customers (full_name, phone, address) VALUES (@name, @phone, @address)",
                %connection);
            command.Parameters->AddWithValue("@name", toManagedString(customer.getFullName()));
            command.Parameters->AddWithValue("@phone", toManagedString(customer.getPhone()));
            command.Parameters->AddWithValue("@address", toManagedString(customer.getAddress()));
            command.ExecuteNonQuery();

            newId = static_cast<int>(command.LastInsertedId);
        }
        catch (Exception^ ex)
        {
            if (isMySqlError(ex, 1062))
                throw gcnew DatabaseException("A customer with this contact number is already registered.");
            throw toDatabaseException(ex, "Could not save the customer.");
        }

        customer.setCustomerId(newId);
        return newId;
    }

    bool DatabaseManager::findCustomerById(int customerId, Customer& result) const
    {
        DataTable^ table = nullptr;
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();

            MySqlCommand command(
                "SELECT customer_id, full_name, phone, address FROM customers WHERE customer_id = @id",
                %connection);
            command.Parameters->AddWithValue("@id", customerId);
            table = fillTable(%command);
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not load the customer.");
        }

        if (table->Rows->Count == 0)
            return false;

        // OBJECT: a Customer object is created from the database row.
        DataRow^ row = table->Rows[0];
        result = Customer(Convert::ToInt32(row["customer_id"]),
                          toStdString(Convert::ToString(row["full_name"])),
                          toStdString(Convert::ToString(row["phone"])),
                          toStdString(Convert::ToString(row["address"])));
        return true;
    }

    DataTable^ DatabaseManager::getAllCustomers() const
    {
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            MySqlCommand command(customerListSql(""), %connection);
            return fillTable(%command);
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not load customers.");
        }
    }

    DataTable^ DatabaseManager::searchCustomers(String^ searchText) const
    {
        String^ text = (searchText == nullptr) ? String::Empty : searchText->Trim();
        if (text->Length == 0)
            return getAllCustomers();

        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            MySqlCommand command(customerListSql(
                "WHERE CAST(customer_id AS CHAR) = @exact OR full_name LIKE @pattern OR phone LIKE @pattern"),
                %connection);
            command.Parameters->AddWithValue("@exact", text);
            command.Parameters->AddWithValue("@pattern", likePattern(text));
            return fillTable(%command);
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not search customers.");
        }
    }

    DataTable^ DatabaseManager::getCustomerChoices() const
    {
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            MySqlCommand command(
                "SELECT customer_id, CONCAT('#', customer_id, '  ', full_name, '  (', phone, ')') AS display "
                "FROM customers ORDER BY full_name",
                %connection);
            return fillTable(%command);
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not load the customer list.");
        }
    }

    void DatabaseManager::updateCustomer(const Customer& customer) const
    {
        if (customer.getCustomerId() <= 0)
            throw std::invalid_argument("Please select a customer to update.");

        int rowsMatched = 0;
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();

            MySqlCommand command(
                "UPDATE customers SET full_name = @name, phone = @phone, address = @address "
                "WHERE customer_id = @id",
                %connection);
            command.Parameters->AddWithValue("@name", toManagedString(customer.getFullName()));
            command.Parameters->AddWithValue("@phone", toManagedString(customer.getPhone()));
            command.Parameters->AddWithValue("@address", toManagedString(customer.getAddress()));
            command.Parameters->AddWithValue("@id", customer.getCustomerId());
            rowsMatched = command.ExecuteNonQuery();
        }
        catch (Exception^ ex)
        {
            if (isMySqlError(ex, 1062))
                throw gcnew DatabaseException("Another customer is already registered with this contact number.");
            throw toDatabaseException(ex, "Could not update the customer.");
        }

        if (rowsMatched == 0)
            throw gcnew DatabaseException("The customer was not found. It may have been deleted.");
    }

    void DatabaseManager::deleteCustomer(int customerId) const
    {
        int rowsDeleted = 0;
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            MySqlCommand command("DELETE FROM customers WHERE customer_id = @id", %connection);
            command.Parameters->AddWithValue("@id", customerId);
            rowsDeleted = command.ExecuteNonQuery();
        }
        catch (Exception^ ex)
        {
            // The foreign key (ON DELETE RESTRICT) blocks deleting a customer who has orders.
            // MySQL reports 1451, or 1217 when the account cannot see the key details.
            if (isMySqlError(ex, 1451) || isMySqlError(ex, 1217))
                throw gcnew DatabaseException("This customer has orders, so the record cannot be deleted.\nOrders are kept as business history.");
            throw toDatabaseException(ex, "Could not delete the customer.");
        }

        if (rowsDeleted == 0)
            throw gcnew DatabaseException("The customer was not found. It may already have been deleted.");
    }

    // ========================================================================
    // Orders
    // ========================================================================

    int DatabaseManager::addOrder(LaundryOrder& order) const
    {
        if (order.getCustomerId() <= 0)
            throw std::invalid_argument("Please select a valid customer.");
        if (order.getOrderDate().empty())
            throw std::invalid_argument("Order date is required.");

        // POLYMORPHISM: 'order' is a LaundryOrder reference, but if the real
        // object is a DryCleaningOrder, its overridden calculateBill() and
        // getServiceFee() run here.
        double total = order.calculateBill();
        double serviceFee = order.getServiceFee();
        if (total < 0.0)
            throw std::invalid_argument("The calculated total cannot be negative.");

        int newId = 0;
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();

            MySqlCommand command(
                "INSERT INTO orders (customer_id, service_type, quantity, unit_price, service_fee, "
                "total_amount, status, payment_status, order_date) "
                "VALUES (@customerId, @service, @quantity, @unitPrice, @serviceFee, "
                "@total, @status, @paymentStatus, @orderDate)",
                %connection);
            command.Parameters->AddWithValue("@customerId", order.getCustomerId());
            command.Parameters->AddWithValue("@service", toManagedString(order.getServiceType()));
            command.Parameters->AddWithValue("@quantity", toMoney(order.getQuantity()));
            command.Parameters->AddWithValue("@unitPrice", toMoney(order.getUnitPrice()));
            command.Parameters->AddWithValue("@serviceFee", toMoney(serviceFee));
            command.Parameters->AddWithValue("@total", toMoney(total));
            command.Parameters->AddWithValue("@status", toManagedString(order.getStatus()));
            command.Parameters->AddWithValue("@paymentStatus", toManagedString(order.getPaymentStatus()));
            command.Parameters->AddWithValue("@orderDate", toDate(order.getOrderDate()));
            command.ExecuteNonQuery();

            newId = static_cast<int>(command.LastInsertedId);
        }
        catch (Exception^ ex)
        {
            if (isMySqlError(ex, 1452) || isMySqlError(ex, 1216))
                throw gcnew DatabaseException("The selected customer no longer exists. Refresh the customer list.");
            throw toDatabaseException(ex, "Could not save the order.");
        }

        order.setOrderId(newId);
        return newId;
    }

    std::unique_ptr<LaundryOrder> DatabaseManager::findOrderById(int orderId) const
    {
        DataTable^ table = nullptr;
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            MySqlCommand command(
                "SELECT order_id, customer_id, service_type, quantity, unit_price, service_fee, "
                "status, payment_status, order_date FROM orders WHERE order_id = @id",
                %connection);
            command.Parameters->AddWithValue("@id", orderId);
            table = fillTable(%command);
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not load the order.");
        }

        // Empty pointer = not found. (In C++/CLI the keyword nullptr means a
        // managed null handle, so an empty unique_ptr is returned explicitly.)
        if (table->Rows->Count == 0)
            return std::unique_ptr<LaundryOrder>();

        DataRow^ row = table->Rows[0];
        std::string serviceType = toStdString(Convert::ToString(row["service_type"]));
        double quantity = Convert::ToDouble(row["quantity"]);
        double unitPrice = Convert::ToDouble(row["unit_price"]);
        double serviceFee = Convert::ToDouble(row["service_fee"]);
        String^ orderDate = safe_cast<DateTime>(row["order_date"]).ToString("yyyy-MM-dd", CultureInfo::InvariantCulture);

        // OBJECT + POLYMORPHISM: the factory creates a DryCleaningOrder or a
        // LaundryOrder, using the prices that were saved with the order.
        std::unique_ptr<LaundryOrder> order = createOrderWithPrices(serviceType, quantity, unitPrice, serviceFee);
        order->setOrderId(Convert::ToInt32(row["order_id"]));
        order->setCustomerId(Convert::ToInt32(row["customer_id"]));
        order->setStatus(toStdString(Convert::ToString(row["status"])));
        order->setPaymentStatus(toStdString(Convert::ToString(row["payment_status"])));
        order->setOrderDate(toStdString(orderDate));
        return order;
    }

    DataTable^ DatabaseManager::getAllOrders() const
    {
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            MySqlCommand command(orderListSql(""), %connection);
            return fillTable(%command);
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not load orders.");
        }
    }

    DataTable^ DatabaseManager::searchOrders(String^ searchText) const
    {
        String^ text = (searchText == nullptr) ? String::Empty : searchText->Trim();
        if (text->Length == 0)
            return getAllOrders();

        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            MySqlCommand command(orderListSql(
                "WHERE CAST(o.order_id AS CHAR) = @exact OR c.full_name LIKE @pattern OR c.phone LIKE @pattern "
                "OR o.status LIKE @pattern OR o.service_type LIKE @pattern OR o.payment_status LIKE @pattern"),
                %connection);
            command.Parameters->AddWithValue("@exact", text);
            command.Parameters->AddWithValue("@pattern", likePattern(text));
            return fillTable(%command);
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not search orders.");
        }
    }

    void DatabaseManager::updateOrderDetails(const LaundryOrder& order) const
    {
        if (order.getOrderId() <= 0)
            throw std::invalid_argument("Please select an order to update.");
        if (order.getCustomerId() <= 0)
            throw std::invalid_argument("Please select a valid customer.");
        if (order.getOrderDate().empty())
            throw std::invalid_argument("Order date is required.");

        // POLYMORPHISM again: the correct calculateBill() runs for the real object type.
        double total = order.calculateBill();
        double serviceFee = order.getServiceFee();

        int rowsMatched = 0;
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();

            // The WHERE clause re-checks the business rule in the database, so an
            // order that was paid or started in the meantime is not changed.
            MySqlCommand command(
                "UPDATE orders SET customer_id = @customerId, service_type = @service, quantity = @quantity, "
                "unit_price = @unitPrice, service_fee = @serviceFee, total_amount = @total, order_date = @orderDate "
                "WHERE order_id = @id AND status = 'Pending' AND payment_status = 'Unpaid'",
                %connection);
            command.Parameters->AddWithValue("@customerId", order.getCustomerId());
            command.Parameters->AddWithValue("@service", toManagedString(order.getServiceType()));
            command.Parameters->AddWithValue("@quantity", toMoney(order.getQuantity()));
            command.Parameters->AddWithValue("@unitPrice", toMoney(order.getUnitPrice()));
            command.Parameters->AddWithValue("@serviceFee", toMoney(serviceFee));
            command.Parameters->AddWithValue("@total", toMoney(total));
            command.Parameters->AddWithValue("@orderDate", toDate(order.getOrderDate()));
            command.Parameters->AddWithValue("@id", order.getOrderId());
            rowsMatched = command.ExecuteNonQuery();
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not update the order.");
        }

        if (rowsMatched == 0)
            throw gcnew DatabaseException("This order can no longer be edited.\nOnly Pending orders with no payments can be changed.");
    }

    void DatabaseManager::updateOrderStatus(int orderId, const std::string& expectedCurrentStatus,
                                            const std::string& newStatus) const
    {
        int rowsMatched = 0;
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();

            // Only update if the status in the database is still the one we checked.
            MySqlCommand command(
                "UPDATE orders SET status = @newStatus WHERE order_id = @id AND status = @currentStatus",
                %connection);
            command.Parameters->AddWithValue("@newStatus", toManagedString(newStatus));
            command.Parameters->AddWithValue("@id", orderId);
            command.Parameters->AddWithValue("@currentStatus", toManagedString(expectedCurrentStatus));
            rowsMatched = command.ExecuteNonQuery();
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not update the order status.");
        }

        if (rowsMatched == 0)
            throw gcnew DatabaseException("The order was not found or its status was changed elsewhere. Refresh and try again.");
    }

    void DatabaseManager::cancelOrder(int orderId) const
    {
        int rowsMatched = 0;
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();

            // Orders are never physically deleted; they are marked "Cancelled".
            MySqlCommand command(
                "UPDATE orders SET status = 'Cancelled' WHERE order_id = @id "
                "AND status NOT IN ('Completed', 'Cancelled') AND payment_status = 'Unpaid'",
                %connection);
            command.Parameters->AddWithValue("@id", orderId);
            rowsMatched = command.ExecuteNonQuery();
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not cancel the order.");
        }

        if (rowsMatched == 0)
            throw gcnew DatabaseException("This order cannot be cancelled. It may be completed, already cancelled, or have a payment.");
    }

    // ========================================================================
    // Dashboard
    // ========================================================================

    DashboardStats DatabaseManager::getDashboardStats() const
    {
        DataTable^ table = nullptr;
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            MySqlCommand command(
                "SELECT (SELECT COUNT(*) FROM customers) AS total_customers, "
                "(SELECT COUNT(*) FROM orders WHERE status <> 'Cancelled') AS total_orders, "
                "(SELECT COUNT(*) FROM orders WHERE status = 'Pending') AS pending_orders, "
                "(SELECT COUNT(*) FROM orders WHERE status = 'Ready for Pickup') AS ready_orders",
                %connection);
            table = fillTable(%command);
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not load the dashboard statistics.");
        }

        DataRow^ row = table->Rows[0];
        DashboardStats stats;
        stats.totalCustomers = Convert::ToInt32(row["total_customers"]);
        stats.totalOrders = Convert::ToInt32(row["total_orders"]);
        stats.pendingOrders = Convert::ToInt32(row["pending_orders"]);
        stats.readyOrders = Convert::ToInt32(row["ready_orders"]);
        return stats;
    }

    // ========================================================================
    // Users
    // ========================================================================

    int DatabaseManager::countUsers() const
    {
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            MySqlCommand command("SELECT COUNT(*) FROM users", %connection);
            return Convert::ToInt32(command.ExecuteScalar());
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not check the user accounts.");
        }
    }

    String^ DatabaseManager::getPasswordHash(String^ username) const
    {
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            MySqlCommand command("SELECT password_hash FROM users WHERE username = @username", %connection);
            command.Parameters->AddWithValue("@username", username);

            Object^ value = command.ExecuteScalar();
            if (value == nullptr || value == DBNull::Value)
                return nullptr;
            return Convert::ToString(value);
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not check the login details.");
        }
    }

    bool DatabaseManager::createFirstUser(String^ username, String^ passwordHash) const
    {
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();

            // Inserts ONLY if the users table is still empty, in a single statement,
            // so the first-run setup cannot be used to add more accounts later.
            MySqlCommand command(
                "INSERT INTO users (username, password_hash) "
                "SELECT @username, @hash FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM users)",
                %connection);
            command.Parameters->AddWithValue("@username", username);
            command.Parameters->AddWithValue("@hash", passwordHash);
            return command.ExecuteNonQuery() == 1;
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not create the administrator account.");
        }
    }

    // ========================================================================
    // Payments
    // ========================================================================

    double DatabaseManager::getTotalPaid(int orderId) const
    {
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            MySqlCommand command("SELECT COALESCE(SUM(amount), 0) FROM payments WHERE order_id = @id", %connection);
            command.Parameters->AddWithValue("@id", orderId);
            return Convert::ToDouble(command.ExecuteScalar());
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not load the payments for this order.");
        }
    }

    DataTable^ DatabaseManager::getPaymentsForOrder(int orderId) const
    {
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();
            MySqlCommand command(
                "SELECT payment_id AS `Payment ID`, DATE_FORMAT(payment_date, '%Y-%m-%d %H:%i') AS `Date`, "
                "payment_method AS `Method`, amount AS `Amount (Rs.)` "
                "FROM payments WHERE order_id = @id ORDER BY payment_date, payment_id",
                %connection);
            command.Parameters->AddWithValue("@id", orderId);
            return fillTable(%command);
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "Could not load the payments for this order.");
        }
    }

    std::string DatabaseManager::recordPayment(int orderId, double amount, const std::string& paymentMethod) const
    {
        String^ newPaymentStatus = nullptr;
        try
        {
            MySqlConnection connection(buildConnectionString());
            connection.Open();

            // TRANSACTION: the payment row and the order's payment status must be
            // saved together. auto_handle disposes the transaction at the end of
            // this block; if Commit() was not reached, Dispose() rolls it back.
            msclr::auto_handle<MySqlTransaction> transaction(connection.BeginTransaction());

            // 1. Lock the order row so two payments cannot be recorded at the same time.
            MySqlCommand lockOrder(
                "SELECT total_amount, status FROM orders WHERE order_id = @id FOR UPDATE",
                %connection, transaction.get());
            lockOrder.Parameters->AddWithValue("@id", orderId);
            DataTable^ orderTable = fillTable(%lockOrder);
            if (orderTable->Rows->Count == 0)
                throw gcnew DatabaseException(String::Format("Order #{0} was not found.", orderId));

            DataRow^ orderRow = orderTable->Rows[0];
            Decimal total = Convert::ToDecimal(orderRow["total_amount"]);
            String^ status = Convert::ToString(orderRow["status"]);
            if (String::Equals(status, "Cancelled"))
                throw gcnew DatabaseException("Payments cannot be recorded for a cancelled order.");

            // 2. Re-check the outstanding balance inside the transaction.
            MySqlCommand sumPaid(
                "SELECT COALESCE(SUM(amount), 0) FROM payments WHERE order_id = @id",
                %connection, transaction.get());
            sumPaid.Parameters->AddWithValue("@id", orderId);
            Decimal alreadyPaid = Convert::ToDecimal(sumPaid.ExecuteScalar());

            Decimal payment = toMoney(amount);
            Decimal outstanding = Decimal::Subtract(total, alreadyPaid);
            if (Decimal::Compare(payment, Decimal::Zero) <= 0)
                throw gcnew DatabaseException("The payment amount must be greater than zero.");
            if (Decimal::Compare(payment, outstanding) > 0)
                throw gcnew DatabaseException(String::Format(
                    "The payment is more than the outstanding balance of Rs. {0:N2}.", outstanding));

            // 3. Insert the payment.
            MySqlCommand insertPayment(
                "INSERT INTO payments (order_id, amount, payment_method) VALUES (@id, @amount, @method)",
                %connection, transaction.get());
            insertPayment.Parameters->AddWithValue("@id", orderId);
            insertPayment.Parameters->AddWithValue("@amount", payment);
            insertPayment.Parameters->AddWithValue("@method", toManagedString(paymentMethod));
            insertPayment.ExecuteNonQuery();

            // 4. Update the order's payment status.
            Decimal paidAfter = Decimal::Add(alreadyPaid, payment);
            if (Decimal::Compare(paidAfter, total) >= 0)
                newPaymentStatus = "Paid";
            else
                newPaymentStatus = "Partially Paid";

            MySqlCommand updateOrder(
                "UPDATE orders SET payment_status = @paymentStatus WHERE order_id = @id",
                %connection, transaction.get());
            updateOrder.Parameters->AddWithValue("@paymentStatus", newPaymentStatus);
            updateOrder.Parameters->AddWithValue("@id", orderId);
            updateOrder.ExecuteNonQuery();

            // 5. Both changes succeeded - make them permanent.
            transaction->Commit();
        }
        catch (Exception^ ex)
        {
            throw toDatabaseException(ex, "The payment could not be saved. Nothing was recorded.");
        }

        return toStdString(newPaymentStatus);
    }
}
