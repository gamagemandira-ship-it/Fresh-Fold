#include "OrderForm.h"
#include "BillingForm.h"
#include "../Models/OrderFactory.h"
#include "../Models/PriceList.h"
#include "../Utils/StringUtils.h"

#include <stdexcept>

using namespace System;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Globalization;
using namespace System::Windows::Forms;

namespace FreshFold
{
    OrderForm::OrderForm(DatabaseManager* database, BillingService* billing)
        : database(database),
          billing(billing)
    {
        buildLayout();
    }

    void OrderForm::buildLayout()
    {
        Ui::StyleForm(this, "Orders", 1100, 680);
        Controls->Add(Ui::MakeHeader("Order Management", "Create laundry and dry-cleaning orders and track their progress"));

        // ---------- Order details (left side) ----------
        Label^ detailsTitle = Ui::MakeLabel("Order Details", 20, 78);
        detailsTitle->Font = Ui::MakeFont(11.0f, true);
        detailsTitle->ForeColor = Ui::Navy();
        Controls->Add(detailsTitle);

        Controls->Add(Ui::MakeLabel("Order ID", 20, 110));
        txtOrderId = Ui::MakeTextBox(160, 110, 220);
        txtOrderId->ReadOnly = true;   // assigned automatically by MySQL
        Controls->Add(txtOrderId);

        Controls->Add(Ui::MakeLabel("Customer *", 20, 146));
        cmbCustomer = Ui::MakeDropDown(160, 146, 220);
        cmbCustomer->DropDownWidth = 340;
        cmbCustomer->DisplayMember = "display";
        cmbCustomer->ValueMember = "customer_id";
        Controls->Add(cmbCustomer);

        Controls->Add(Ui::MakeLabel("Service *", 20, 182));
        cmbService = Ui::MakeDropDown(160, 182, 220);
        cmbService->Items->Add(toManagedString(ServiceType::WASH_FOLD));
        cmbService->Items->Add(toManagedString(ServiceType::DRY_CLEANING));
        cmbService->Items->Add(toManagedString(ServiceType::STEAM_PRESS));
        cmbService->SelectedIndexChanged += gcnew EventHandler(this, &OrderForm::onServiceChanged);
        Controls->Add(cmbService);

        lblQuantity = Ui::MakeLabel("Quantity *", 20, 218);
        Controls->Add(lblQuantity);
        txtQuantity = Ui::MakeTextBox(160, 218, 220);
        Controls->Add(txtQuantity);

        Controls->Add(Ui::MakeLabel("Order Date", 20, 254));
        dtpOrderDate = gcnew DateTimePicker();
        dtpOrderDate->Location = Point(160, 254);
        dtpOrderDate->Width = 220;
        dtpOrderDate->Format = DateTimePickerFormat::Custom;
        dtpOrderDate->CustomFormat = "yyyy-MM-dd";
        Controls->Add(dtpOrderDate);

        Controls->Add(Ui::MakeLabel("Processing Status", 20, 290));
        cmbStatus = Ui::MakeDropDown(160, 290, 220);
        cmbStatus->Items->Add(toManagedString(OrderStatus::PENDING));
        cmbStatus->Items->Add(toManagedString(OrderStatus::IN_WASH));
        cmbStatus->Items->Add(toManagedString(OrderStatus::READY));
        cmbStatus->Items->Add(toManagedString(OrderStatus::COMPLETED));
        Controls->Add(cmbStatus);

        lblCurrentStatus = gcnew Label();
        lblCurrentStatus->AutoSize = false;
        lblCurrentStatus->Size = System::Drawing::Size(360, 20);
        lblCurrentStatus->Location = Point(20, 320);
        lblCurrentStatus->ForeColor = Color::DimGray;
        Controls->Add(lblCurrentStatus);

        Button^ btnCalculate = Ui::MakeButton("Calculate Bill", 20, 346, 175, false);
        btnCalculate->Click += gcnew EventHandler(this, &OrderForm::onCalculateClick);
        Controls->Add(btnCalculate);

        Button^ btnCreate = Ui::MakeButton("Create Order", 205, 346, 175, true);
        btnCreate->Click += gcnew EventHandler(this, &OrderForm::onCreateClick);
        Controls->Add(btnCreate);

        Button^ btnUpdateOrder = Ui::MakeButton("Update Order", 20, 386, 175, false);
        btnUpdateOrder->Click += gcnew EventHandler(this, &OrderForm::onUpdateOrderClick);
        Controls->Add(btnUpdateOrder);

        Button^ btnUpdateStatus = Ui::MakeButton("Update Status", 205, 386, 175, false);
        btnUpdateStatus->Click += gcnew EventHandler(this, &OrderForm::onUpdateStatusClick);
        Controls->Add(btnUpdateStatus);

        Button^ btnCancelOrder = Ui::MakeButton("Cancel Order", 20, 426, 175, false);
        btnCancelOrder->Click += gcnew EventHandler(this, &OrderForm::onCancelOrderClick);
        Controls->Add(btnCancelOrder);

        Button^ btnBilling = Ui::MakeButton("Bill && Payment", 205, 426, 175, false);   // && shows a single &
        btnBilling->Click += gcnew EventHandler(this, &OrderForm::onBillingClick);
        Controls->Add(btnBilling);

        Button^ btnClear = Ui::MakeButton("Clear Form", 20, 466, 175, false);
        btnClear->Click += gcnew EventHandler(this, &OrderForm::onClearClick);
        Controls->Add(btnClear);

        txtBill = gcnew TextBox();
        txtBill->Multiline = true;
        txtBill->ReadOnly = true;
        txtBill->Font = gcnew System::Drawing::Font("Consolas", 9.0f);
        txtBill->Location = Point(20, 508);
        txtBill->Size = System::Drawing::Size(360, 96);
        txtBill->BackColor = Color::White;
        Controls->Add(txtBill);

        lblStatus = gcnew Label();
        lblStatus->AutoSize = false;
        lblStatus->Size = System::Drawing::Size(380, 64);
        lblStatus->Location = Point(20, 610);
        Controls->Add(lblStatus);

        // ---------- Search + table (right side) ----------
        Controls->Add(Ui::MakeLabel("Search", 420, 80));
        txtSearch = Ui::MakeTextBox(480, 80, 330);
        txtSearch->KeyDown += gcnew KeyEventHandler(this, &OrderForm::onSearchKeyDown);
        Controls->Add(txtSearch);

        Button^ btnSearch = Ui::MakeButton("Search", 820, 77, 110, true);
        btnSearch->Click += gcnew EventHandler(this, &OrderForm::onSearchClick);
        Controls->Add(btnSearch);

        Button^ btnShowAll = Ui::MakeButton("Show All Orders", 940, 77, 140, false);
        btnShowAll->Click += gcnew EventHandler(this, &OrderForm::onShowAllClick);
        Controls->Add(btnShowAll);

        Label^ searchHint = Ui::MakeLabel("Search by order ID, customer name, phone, service, status or payment status.", 480, 104);
        searchHint->ForeColor = Color::DimGray;
        Controls->Add(searchHint);

        gridOrders = Ui::MakeGrid(420, 134, 660, 526);
        gridOrders->CellClick += gcnew DataGridViewCellEventHandler(this, &OrderForm::onGridCellClick);
        Controls->Add(gridOrders);

        Shown += gcnew EventHandler(this, &OrderForm::onFormShown);
    }

    // ------------------------------------------------------------------------
    // Loading data
    // ------------------------------------------------------------------------
    void OrderForm::loadCustomerChoices()
    {
        try
        {
            DataTable^ table = database->getCustomerChoices();
            cmbCustomer->DataSource = table;
            cmbCustomer->SelectedIndex = -1;
            if (table->Rows->Count == 0)
                Ui::SetStatus(lblStatus, "There are no customers yet. Register a customer on the Customers screen first.", false);
        }
        catch (DatabaseException^ ex)
        {
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    void OrderForm::showOrders(DataTable^ table)
    {
        gridOrders->DataSource = table;
        Ui::FormatColumn(gridOrders, "Total (Rs.)", "N2");
        Ui::FormatColumn(gridOrders, "Quantity", "0.##");
        if (gridOrders->Columns->Contains("Order ID"))
            gridOrders->Columns["Order ID"]->FillWeight = 55;
        if (gridOrders->Columns->Contains("Status"))
            gridOrders->Columns["Status"]->FillWeight = 110;
        gridOrders->ClearSelection();
    }

    void OrderForm::loadAllOrders()
    {
        try
        {
            showOrders(database->getAllOrders());
        }
        catch (DatabaseException^ ex)
        {
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    void OrderForm::clearForm()
    {
        txtOrderId->Clear();
        cmbCustomer->SelectedIndex = -1;
        cmbService->SelectedIndex = -1;
        lblQuantity->Text = "Quantity *";
        txtQuantity->Clear();
        dtpOrderDate->Value = DateTime::Today;
        cmbStatus->SelectedItem = toManagedString(OrderStatus::PENDING);
        cmbStatus->Enabled = false;   // a new order always starts as Pending
        lblCurrentStatus->Text = "New order - it will start as 'Pending' and 'Unpaid'.";
        txtBill->Clear();
        gridOrders->ClearSelection();
    }

    bool OrderForm::readSelectedOrderId(int% orderId)
    {
        return Int32::TryParse(txtOrderId->Text, orderId) && orderId > 0;
    }

    // Loads one order from the database (as a LaundryOrder or DryCleaningOrder
    // object) and shows it in the form.
    void OrderForm::loadOrderIntoForm(int orderId)
    {
        try
        {
            std::unique_ptr<LaundryOrder> order = database->findOrderById(orderId);
            if (!order)
            {
                Ui::SetStatus(lblStatus, String::Format("Order #{0} was not found.", orderId), false);
                clearForm();
                return;
            }

            txtOrderId->Text = Convert::ToString(order->getOrderId());

            // Select the customer; reload the list first if the customer is new.
            cmbCustomer->SelectedValue = order->getCustomerId();
            if (cmbCustomer->SelectedValue == nullptr || Convert::ToInt32(cmbCustomer->SelectedValue) != order->getCustomerId())
            {
                loadCustomerChoices();
                cmbCustomer->SelectedValue = order->getCustomerId();
            }

            cmbService->SelectedItem = toManagedString(order->getServiceType());   // also updates the quantity label
            txtQuantity->Text = toManagedString(formatQuantity(order->getQuantity()));
            dtpOrderDate->Value = DateTime::ParseExact(toManagedString(order->getOrderDate()), "yyyy-MM-dd",
                                                       CultureInfo::InvariantCulture);

            // Pre-select the next allowed status to make "Update Status" quick.
            std::string nextStatus = order->getNextStatus();
            if (nextStatus.empty())
            {
                cmbStatus->SelectedItem = toManagedString(order->getStatus());   // Completed, or none for Cancelled
                if (order->getStatus() == OrderStatus::CANCELLED)
                    cmbStatus->SelectedIndex = -1;
                cmbStatus->Enabled = false;
            }
            else
            {
                cmbStatus->SelectedItem = toManagedString(nextStatus);
                cmbStatus->Enabled = true;
            }

            lblCurrentStatus->Text = String::Format("Current status: {0}   |   Payment: {1}",
                                                    toManagedString(order->getStatus()),
                                                    toManagedString(order->getPaymentStatus()));
            txtBill->Text = toDisplayText(order->getBillBreakdown());   // virtual call
            Ui::SetStatus(lblStatus, String::Format("Order #{0} loaded.", orderId), true);
        }
        catch (const std::exception& ex)
        {
            Ui::SetStatus(lblStatus, toManagedString(ex.what()), false);
        }
        catch (DatabaseException^ ex)
        {
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    // ------------------------------------------------------------------------
    // Reading and validating the form
    // ------------------------------------------------------------------------
    std::string OrderForm::readService()
    {
        if (cmbService->SelectedIndex < 0)
            throw std::invalid_argument("Please select a service.");
        return toStdString(cmbService->SelectedItem->ToString());
    }

    double OrderForm::readQuantity(const std::string& service)
    {
        String^ text = txtQuantity->Text->Trim();
        bool byWeight = LaundryOrder::usesWeight(service);

        if (text->Length == 0 && byWeight)
            throw std::invalid_argument("Please enter the weight in kg.");
        if (text->Length == 0)
            throw std::invalid_argument("Please enter the number of items.");

        if (byWeight)
        {
            double weight = 0.0;
            if (!Double::TryParse(text, NumberStyles::Float, CultureInfo::InvariantCulture, weight))
                throw std::invalid_argument("Weight must be a number, for example 2.5");
            return weight;   // range checks happen in LaundryOrder::setQuantity()
        }

        int items = 0;
        if (!Int32::TryParse(text, NumberStyles::Integer, CultureInfo::InvariantCulture, items))
            throw std::invalid_argument("Number of items must be a whole number, for example 3");
        return items;
    }

    std::unique_ptr<LaundryOrder> OrderForm::buildOrderFromForm()
    {
        if (cmbCustomer->SelectedValue == nullptr)
            throw std::invalid_argument("Please select a customer.");

        std::string service = readService();
        double quantity = readQuantity(service);

        // OBJECT: the factory creates a LaundryOrder or a DryCleaningOrder and
        // returns it through a base-class pointer (POLYMORPHISM).
        std::unique_ptr<LaundryOrder> order = createOrder(service, quantity);
        order->setCustomerId(Convert::ToInt32(cmbCustomer->SelectedValue));
        order->setOrderDate(toStdString(dtpOrderDate->Value.ToString("yyyy-MM-dd", CultureInfo::InvariantCulture)));
        return order;
    }

    // ------------------------------------------------------------------------
    // Event handlers
    // ------------------------------------------------------------------------
    void OrderForm::onFormShown(Object^, EventArgs^)
    {
        loadCustomerChoices();
        clearForm();
        loadAllOrders();
    }

    // The quantity label changes with the service: "Weight (kg)" or "Number of Items".
    void OrderForm::onServiceChanged(Object^, EventArgs^)
    {
        txtBill->Clear();
        if (cmbService->SelectedIndex < 0)
        {
            lblQuantity->Text = "Quantity *";
            return;
        }
        std::string service = toStdString(cmbService->SelectedItem->ToString());
        lblQuantity->Text = String::Concat(toManagedString(LaundryOrder::quantityLabel(service)), " *");
    }

    void OrderForm::onCalculateClick(Object^, EventArgs^)
    {
        try
        {
            std::string service = readService();
            double quantity = readQuantity(service);

            // ABSTRACTION: the form only asks for a bill. The pricing formula is
            // hidden inside the order classes.
            BillQuote quote = billing->quote(service, quantity);
            txtBill->Text = toDisplayText(quote.breakdown);
            Ui::SetStatus(lblStatus, String::Concat("Bill calculated: ", toManagedString(formatRupees(quote.total))), true);
        }
        catch (const std::exception& ex)
        {
            Ui::SetStatus(lblStatus, toManagedString(ex.what()), false);
        }
    }

    // CREATE
    void OrderForm::onCreateClick(Object^, EventArgs^)
    {
        if (txtOrderId->Text->Length > 0)
        {
            Ui::ShowError("This order is already saved.\nClick 'Clear Form' to create a new order.");
            return;
        }

        try
        {
            std::unique_ptr<LaundryOrder> order = buildOrderFromForm();
            int newId = database->addOrder(*order);
            double total = BillingService::calculateBill(*order);

            loadAllOrders();
            loadOrderIntoForm(newId);
            Ui::SetStatus(lblStatus, String::Format("Order #{0} was created. Total: {1}",
                                                    newId, toManagedString(formatRupees(total))), true);
        }
        catch (const std::exception& ex)
        {
            Ui::SetStatus(lblStatus, toManagedString(ex.what()), false);
        }
        catch (DatabaseException^ ex)
        {
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    // UPDATE (details)
    void OrderForm::onUpdateOrderClick(Object^, EventArgs^)
    {
        int orderId = 0;
        if (!readSelectedOrderId(orderId))
        {
            Ui::ShowError("Please select an order from the table first.");
            return;
        }

        try
        {
            std::unique_ptr<LaundryOrder> existing = database->findOrderById(orderId);
            if (!existing)
            {
                Ui::SetStatus(lblStatus, String::Format("Order #{0} was not found.", orderId), false);
                loadAllOrders();
                return;
            }
            if (!existing->canEditDetails())
            {
                Ui::SetStatus(lblStatus, "Only orders that are still 'Pending' and 'Unpaid' can be edited. "
                                         "Use 'Update Status' to move this order forward.", false);
                return;
            }

            // A new object is built from the form (the service may have changed,
            // e.g. from Wash & Fold to Dry Cleaning, so the class may change too).
            std::unique_ptr<LaundryOrder> updated = buildOrderFromForm();
            updated->setOrderId(orderId);
            database->updateOrderDetails(*updated);

            loadAllOrders();
            loadOrderIntoForm(orderId);
            Ui::SetStatus(lblStatus, String::Format("Order #{0} was updated. New total: {1}", orderId,
                                                    toManagedString(formatRupees(BillingService::calculateBill(*updated)))), true);
        }
        catch (const std::exception& ex)
        {
            Ui::SetStatus(lblStatus, toManagedString(ex.what()), false);
        }
        catch (DatabaseException^ ex)
        {
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    // UPDATE (status)
    void OrderForm::onUpdateStatusClick(Object^, EventArgs^)
    {
        int orderId = 0;
        if (!readSelectedOrderId(orderId))
        {
            Ui::ShowError("Please select an order from the table first.");
            return;
        }
        if (cmbStatus->SelectedIndex < 0)
        {
            Ui::ShowError("Please choose the new processing status.");
            return;
        }

        try
        {
            std::unique_ptr<LaundryOrder> order = database->findOrderById(orderId);
            if (!order)
            {
                Ui::SetStatus(lblStatus, String::Format("Order #{0} was not found.", orderId), false);
                return;
            }

            std::string oldStatus = order->getStatus();
            std::string newStatus = toStdString(cmbStatus->SelectedItem->ToString());

            // The business rules (one step forward only, must be paid before
            // "Completed", no changes after cancelling) are checked by the object.
            order->moveToStatus(newStatus);
            database->updateOrderStatus(orderId, oldStatus, newStatus);

            loadAllOrders();
            loadOrderIntoForm(orderId);
            Ui::SetStatus(lblStatus, String::Format("Order #{0}: status changed from '{1}' to '{2}'.", orderId,
                                                    toManagedString(oldStatus), toManagedString(newStatus)), true);
        }
        catch (const std::exception& ex)
        {
            Ui::SetStatus(lblStatus, toManagedString(ex.what()), false);
        }
        catch (DatabaseException^ ex)
        {
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    // CANCEL (orders are not deleted; they are marked "Cancelled")
    void OrderForm::onCancelOrderClick(Object^, EventArgs^)
    {
        int orderId = 0;
        if (!readSelectedOrderId(orderId))
        {
            Ui::ShowError("Please select an order from the table first.");
            return;
        }

        try
        {
            std::unique_ptr<LaundryOrder> order = database->findOrderById(orderId);
            if (!order)
            {
                Ui::SetStatus(lblStatus, String::Format("Order #{0} was not found.", orderId), false);
                return;
            }

            order->cancel();   // throws with the reason if cancelling is not allowed

            if (!Ui::Confirm(String::Format("Cancel order #{0}?\nIt will stay in the history with the status 'Cancelled'.", orderId)))
                return;

            database->cancelOrder(orderId);
            loadAllOrders();
            loadOrderIntoForm(orderId);
            Ui::SetStatus(lblStatus, String::Format("Order #{0} was cancelled.", orderId), true);
        }
        catch (const std::exception& ex)
        {
            Ui::SetStatus(lblStatus, toManagedString(ex.what()), false);
        }
        catch (DatabaseException^ ex)
        {
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    void OrderForm::onClearClick(Object^, EventArgs^)
    {
        clearForm();
        lblStatus->Text = String::Empty;
    }

    // Opens the billing screen for the selected order.
    void OrderForm::onBillingClick(Object^, EventArgs^)
    {
        int orderId = 0;
        if (!readSelectedOrderId(orderId))
        {
            Ui::ShowError("Please select an order from the table first.");
            return;
        }

        BillingForm^ screen = gcnew BillingForm(database, billing, orderId);
        screen->ShowDialog(this);
        delete screen;

        // The payment status may have changed.
        loadAllOrders();
        loadOrderIntoForm(orderId);
    }

    void OrderForm::onSearchClick(Object^, EventArgs^)
    {
        try
        {
            DataTable^ table = database->searchOrders(txtSearch->Text);
            showOrders(table);
            Ui::SetStatus(lblStatus, String::Format("{0} order(s) found.", table->Rows->Count), table->Rows->Count > 0);
        }
        catch (DatabaseException^ ex)
        {
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    void OrderForm::onShowAllClick(Object^, EventArgs^)
    {
        txtSearch->Clear();
        loadAllOrders();
        lblStatus->Text = String::Empty;
    }

    void OrderForm::onSearchKeyDown(Object^, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter)
        {
            e->SuppressKeyPress = true;
            onSearchClick(nullptr, nullptr);
        }
    }

    void OrderForm::onGridCellClick(Object^, DataGridViewCellEventArgs^ e)
    {
        if (e->RowIndex < 0)
            return;
        int orderId = Convert::ToInt32(gridOrders->Rows[e->RowIndex]->Cells["Order ID"]->Value);
        loadOrderIntoForm(orderId);
    }
}
