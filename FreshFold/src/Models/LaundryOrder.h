#pragma once

// ============================================================================
// LaundryOrder.h
// OOP: CLASS, ENCAPSULATION, ABSTRACTION, INHERITANCE (base class),
//      POLYMORPHISM (virtual methods)
//
// LaundryOrder is the BASE CLASS for every order. It stores the data that
// all orders share and calculates the standard bill:
//     total = quantity x unit price
// That standard formula is used for Wash & Fold (per kg) and Steam Press
// (per item).
//
// DryCleaningOrder (see DryCleaningOrder.h) INHERITS from this class and
// OVERRIDES calculateBill() because dry cleaning adds a handling fee.
//
// Because calculateBill() is virtual, code that holds a LaundryOrder* or
// LaundryOrder& automatically runs the correct version at runtime.
// ============================================================================

#include <string>

namespace FreshFold
{
    // Names of the three services (also stored in the database exactly like this).
    namespace ServiceType
    {
        const char* const WASH_FOLD = "Wash & Fold";
        const char* const DRY_CLEANING = "Dry Cleaning";
        const char* const STEAM_PRESS = "Steam Press";
    }

    // Processing statuses, in workflow order, plus "Cancelled".
    namespace OrderStatus
    {
        const char* const PENDING = "Pending";
        const char* const IN_WASH = "In Wash";
        const char* const READY = "Ready for Pickup";
        const char* const COMPLETED = "Completed";
        const char* const CANCELLED = "Cancelled";
    }

    // Payment statuses.
    namespace PaymentStatus
    {
        const char* const UNPAID = "Unpaid";
        const char* const PARTIALLY_PAID = "Partially Paid";
        const char* const PAID = "Paid";
    }

    class LaundryOrder
    {
    private:
        // ENCAPSULATION: private data, accessed only through methods.
        int orderId;             // 0 = not saved yet
        int customerId;          // 0 = no customer selected yet
        std::string serviceType;
        double quantity;         // kg for Wash & Fold, items for the others
        double unitPrice;        // price per kg or per item at the time of the order
        std::string status;
        std::string paymentStatus;
        std::string orderDate;   // "YYYY-MM-DD"

    public:
        LaundryOrder(const std::string& serviceType, double quantity, double unitPrice);

        // A virtual destructor is required whenever a class is used through
        // a base-class pointer, so the derived destructor also runs.
        virtual ~LaundryOrder();

        // ---------- ABSTRACTION + POLYMORPHISM ----------
        // The GUI only calls calculateBill(); it never needs to know the formula.
        virtual double calculateBill() const;

        // Extra fixed fee added on top of quantity x unit price (0 for standard orders).
        virtual double getServiceFee() const;

        // Human-readable explanation of the calculation, shown on the invoice.
        virtual std::string getBillBreakdown() const;

        // ---------- Getters ----------
        int getOrderId() const;
        int getCustomerId() const;
        std::string getServiceType() const;
        double getQuantity() const;
        double getUnitPrice() const;
        std::string getStatus() const;
        std::string getPaymentStatus() const;
        std::string getOrderDate() const;

        // ---------- Setters (all validate their input) ----------
        void setOrderId(int id);
        void setCustomerId(int id);
        void setQuantity(double newQuantity);
        void setOrderDate(const std::string& date);
        void setStatus(const std::string& newStatus);           // used when loading from the database
        void setPaymentStatus(const std::string& newStatus);

        // ---------- Business rules ----------
        // Moves the order forward one step: Pending -> In Wash -> Ready for Pickup -> Completed.
        void moveToStatus(const std::string& newStatus);

        // Cancels the order if the rules allow it (not completed, nothing paid yet).
        void cancel();

        // The service, quantity, customer and date can only be changed while
        // the order is still Pending and nothing has been paid.
        bool canEditDetails() const;

        // Returns the next status in the workflow, or "" if there is none.
        std::string getNextStatus() const;

        // ---------- Helpers shared with the GUI ----------
        static bool isValidService(const std::string& service);
        static bool usesWeight(const std::string& service);           // true only for Wash & Fold
        static std::string quantityLabel(const std::string& service); // "Weight (kg)" / "Number of Items"
        static bool isValidStatus(const std::string& value);          // includes "Cancelled"
        static bool isValidPaymentStatus(const std::string& value);
        static std::string nextStatusAfter(const std::string& current);

    protected:
        // Shared by this class and DryCleaningOrder to describe "quantity x price".
        std::string describeQuantityTimesPrice() const;
    };
}
