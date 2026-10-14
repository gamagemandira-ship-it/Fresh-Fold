#include "Customer.h"

#include <stdexcept>

namespace FreshFold
{
    Customer::Customer()
        : customerId(0)
    {
    }

    Customer::Customer(const std::string& fullName, const std::string& phone, const std::string& address)
        : customerId(0)
    {
        // The setters validate the values, so invalid data is rejected
        // at the moment the object is created.
        setFullName(fullName);
        setPhone(phone);
        setAddress(address);
    }

    Customer::Customer(int customerId, const std::string& fullName, const std::string& phone, const std::string& address)
        : Customer(fullName, phone, address)
    {
        setCustomerId(customerId);
    }

    int Customer::getCustomerId() const { return customerId; }
    std::string Customer::getFullName() const { return fullName; }
    std::string Customer::getPhone() const { return phone; }
    std::string Customer::getAddress() const { return address; }

    void Customer::setCustomerId(int id)
    {
        if (id <= 0)
            throw std::invalid_argument("Customer ID must be a positive number.");
        customerId = id;
    }

    void Customer::setFullName(const std::string& name)
    {
        std::string cleaned = trim(name);
        if (cleaned.empty())
            throw std::invalid_argument("Customer name is required.");
        if (cleaned.length() < 2)
            throw std::invalid_argument("Customer name is too short.");
        if (cleaned.length() > 100)
            throw std::invalid_argument("Customer name must be 100 characters or fewer.");
        fullName = cleaned;
    }

    void Customer::setPhone(const std::string& phoneNumber)
    {
        std::string cleaned = trim(phoneNumber);
        if (cleaned.empty())
            throw std::invalid_argument("Contact number is required.");
        if (cleaned.length() > 20)
            throw std::invalid_argument("Contact number must be 20 characters or fewer.");

        // Accept common formats such as "071 234 5678", "+94 71 234 5678"
        // or "(011) 222-3333". Only digits and a few separator characters
        // are allowed, and there must be at least 7 digits.
        int digitCount = 0;
        for (char ch : cleaned)
        {
            if (ch >= '0' && ch <= '9')
                digitCount++;
            else if (ch != ' ' && ch != '+' && ch != '-' && ch != '(' && ch != ')')
                throw std::invalid_argument("Contact number may only contain digits, spaces, +, -, ( and ).");
        }
        if (digitCount < 7)
            throw std::invalid_argument("Contact number must contain at least 7 digits.");

        phone = cleaned;
    }

    void Customer::setAddress(const std::string& newAddress)
    {
        std::string cleaned = trim(newAddress);
        if (cleaned.empty())
            throw std::invalid_argument("Address is required.");
        if (cleaned.length() < 3)
            throw std::invalid_argument("Address is too short.");
        if (cleaned.length() > 255)
            throw std::invalid_argument("Address must be 255 characters or fewer.");
        address = cleaned;
    }

    std::string Customer::trim(const std::string& text)
    {
        const char* whitespace = " \t\r\n";
        std::string::size_type start = text.find_first_not_of(whitespace);
        if (start == std::string::npos)
            return "";
        std::string::size_type end = text.find_last_not_of(whitespace);
        return text.substr(start, end - start + 1);
    }
}
