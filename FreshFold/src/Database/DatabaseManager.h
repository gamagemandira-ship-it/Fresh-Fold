#pragma once

// ============================================================================
// DatabaseManager.h
// The ONLY class that talks to MySQL. It uses MySQL Connector/NET
// (MySql.Data.dll) through C++/CLI.
//
// Rules followed in every method:
//   * Every value from the user is sent as a PARAMETER (@name), never glued
//     into the SQL text, which prevents SQL injection.
//   * A new connection is opened for each operation and closed automatically
//     at the end of the method (C++/CLI "stack semantics" call Dispose()).
//   * MySQL errors are converted into a DatabaseException with a message that
//     is safe to show to the user (no passwords or internal details).
//     Full technical details are written to the Visual Studio Output window.
//
// Business rules (pricing, status workflow...) are NOT here; they live in
// the model classes and BillingService.
// ============================================================================

#include "DatabaseConfig.h"
#include "../Models/Customer.h"
#include "../Models/LaundryOrder.h"

#include <memory>
#include <string>

namespace FreshFold
{
    // Error thrown by DatabaseManager. Message is user-friendly.
    public ref class DatabaseException : public System::Exception
    {
    public:
        DatabaseException(System::String^ message) : System::Exception(message) {}
    };

    // Numbers shown on the dashboard.
    struct DashboardStats
    {
        int totalCustomers = 0;
        int totalOrders = 0;      // excludes cancelled orders
        int pendingOrders = 0;
        int readyOrders = 0;
    };

    class DatabaseManager
    {
    private:
        DatabaseConfig config;

        System::String^ buildConnectionString() const;

    public:
        explicit DatabaseManager(const DatabaseConfig& config);

        // Opens and closes a connection; throws DatabaseException if it fails.
        void testConnection() const;

        // ---------------- Customers (CRUD) ----------------
        int addCustomer(Customer& customer) const;                  // Create - stores the new ID in the object
        bool findCustomerById(int customerId, Customer& result) const; // Read one
        System::Data::DataTable^ getAllCustomers() const;           // Read all (for the table)
        System::Data::DataTable^ searchCustomers(System::String^ searchText) const;
        System::Data::DataTable^ getCustomerChoices() const;        // for the customer drop-down
        void updateCustomer(const Customer& customer) const;        // Update
        void deleteCustomer(int customerId) const;                  // Delete

        // ---------------- Orders (CRUD) ----------------
        int addOrder(LaundryOrder& order) const;                    // Create - stores the new ID in the object
        std::unique_ptr<LaundryOrder> findOrderById(int orderId) const; // Read one (nullptr if not found)
        System::Data::DataTable^ getAllOrders() const;              // Read all
        System::Data::DataTable^ searchOrders(System::String^ searchText) const;
        void updateOrderDetails(const LaundryOrder& order) const;   // Update details (Pending + Unpaid only)
        void updateOrderStatus(int orderId, const std::string& expectedCurrentStatus,
                               const std::string& newStatus) const; // Update status
        void cancelOrder(int orderId) const;                        // "Delete" = cancel (record is kept)

        // ---------------- Dashboard ----------------
        DashboardStats getDashboardStats() const;

        // ---------------- Users (login) ----------------
        int countUsers() const;
        System::String^ getPasswordHash(System::String^ username) const;  // nullptr if the user does not exist
        bool createFirstUser(System::String^ username, System::String^ passwordHash) const; // false if a user already exists

        // ---------------- Payments ----------------
        double getTotalPaid(int orderId) const;
        System::Data::DataTable^ getPaymentsForOrder(int orderId) const;

        // Saves a payment and updates the order's payment status in ONE
        // transaction (both succeed or both are rolled back).
        // Returns the new payment status ("Partially Paid" or "Paid").
        std::string recordPayment(int orderId, double amount, const std::string& paymentMethod) const;
    };
}
