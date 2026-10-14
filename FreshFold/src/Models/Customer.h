#pragma once

// ============================================================================
// Customer.h
// OOP: CLASS + ENCAPSULATION
// A Customer object represents one customer of the laundry shop.
// All data members are private. Other code can only read them through
// getters and change them through setters, and every setter validates the
// new value first, so a Customer object can never hold invalid data
// (for example an empty name).
// ============================================================================

#include <string>

namespace FreshFold
{
    class Customer
    {
    private:
        int customerId;          // 0 means "not saved in the database yet"
        std::string fullName;
        std::string phone;
        std::string address;

    public:
        // Empty customer, used before loading data from the database.
        Customer();

        // New customer that has not been saved yet (ID is assigned by MySQL).
        Customer(const std::string& fullName, const std::string& phone, const std::string& address);

        // Existing customer loaded from the database.
        Customer(int customerId, const std::string& fullName, const std::string& phone, const std::string& address);

        // Getters
        int getCustomerId() const;
        std::string getFullName() const;
        std::string getPhone() const;
        std::string getAddress() const;

        // Setters - each one throws std::invalid_argument with a
        // user-friendly message if the value is not acceptable.
        void setCustomerId(int id);
        void setFullName(const std::string& name);
        void setPhone(const std::string& phoneNumber);
        void setAddress(const std::string& newAddress);

        // Removes spaces/tabs/newlines from both ends of a string.
        static std::string trim(const std::string& text);
    };
}
