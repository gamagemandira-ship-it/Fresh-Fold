#pragma once

// ============================================================================
// BillingService.h
// Coordinates bill quotes, invoices and payments.
// It never contains a price or a formula itself - it asks the order object
// to calculateBill(), so the pricing rules stay inside the model classes.
// ============================================================================

#include "../Database/DatabaseManager.h"
#include "../Models/Customer.h"
#include "../Models/LaundryOrder.h"

#include <memory>
#include <string>

namespace FreshFold
{
    // Result of "Calculate Bill" on the order screen.
    struct BillQuote
    {
        double total = 0.0;
        std::string breakdown;
    };

    // Everything needed to show an invoice.
    struct Invoice
    {
        Customer customer;
        std::unique_ptr<LaundryOrder> order;
        double total = 0.0;
        double totalPaid = 0.0;
        double balance = 0.0;
    };

    class BillingService
    {
    private:
        const DatabaseManager& database;

    public:
        explicit BillingService(const DatabaseManager& database);

        // POLYMORPHISM: takes a BASE-CLASS reference. The overridden
        // calculateBill() of the real object (e.g. DryCleaningOrder) runs.
        static double calculateBill(const LaundryOrder& order);

        // Builds a temporary order object and returns its bill (nothing is saved).
        BillQuote quote(const std::string& serviceType, double quantity) const;

        // Loads order + customer + payments. Returns false if the order does not exist.
        bool loadInvoice(int orderId, Invoice& invoice) const;

        // Plain-text invoice (fixed-width layout, for screen and printing).
        static std::string formatInvoice(const Invoice& invoice);

        // Validates and records a payment. Returns the new payment status.
        // Throws std::invalid_argument for invalid input, DatabaseException for database errors.
        std::string recordPayment(int orderId, double amount, const std::string& paymentMethod) const;

        static bool isValidPaymentMethod(const std::string& method);
    };
}
