#include "LaundryOrder.h"
#include "PriceList.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

namespace FreshFold
{
    LaundryOrder::LaundryOrder(const std::string& serviceType, double quantity, double unitPrice)
        : orderId(0),
          customerId(0),
          serviceType(serviceType),
          quantity(0.0),
          unitPrice(unitPrice),
          status(OrderStatus::PENDING),
          paymentStatus(PaymentStatus::UNPAID),
          orderDate("")
    {
        if (!isValidService(serviceType))
            throw std::invalid_argument("Please select a valid service.");
        if (unitPrice < 0.0)
            throw std::invalid_argument("Unit price cannot be negative.");

        // setQuantity() needs serviceType, so it is called after it is stored.
        setQuantity(quantity);
    }

    LaundryOrder::~LaundryOrder()
    {
    }

    // ------------------------------------------------------------------
    // Standard bill: quantity x unit price.
    // Used directly by Wash & Fold and Steam Press orders.
    // ------------------------------------------------------------------
    double LaundryOrder::calculateBill() const
    {
        return roundToCents(quantity * unitPrice);
    }

    double LaundryOrder::getServiceFee() const
    {
        return 0.0;
    }

    std::string LaundryOrder::getBillBreakdown() const
    {
        std::ostringstream out;
        out << "Service     : " << serviceType << "\n"
            << "Calculation : " << describeQuantityTimesPrice() << "\n"
            << "Total       : " << formatRupees(calculateBill());
        return out.str();
    }

    std::string LaundryOrder::describeQuantityTimesPrice() const
    {
        std::string unit = usesWeight(serviceType) ? "kg" : (quantity == 1.0 ? "item" : "items");
        std::string per = usesWeight(serviceType) ? "per kg" : "per item";

        std::ostringstream out;
        out << formatQuantity(quantity) << " " << unit << " x "
            << formatRupees(unitPrice) << " " << per << " = "
            << formatRupees(roundToCents(quantity * unitPrice));
        return out.str();
    }

    // ---------------- Getters ----------------
    int LaundryOrder::getOrderId() const { return orderId; }
    int LaundryOrder::getCustomerId() const { return customerId; }
    std::string LaundryOrder::getServiceType() const { return serviceType; }
    double LaundryOrder::getQuantity() const { return quantity; }
    double LaundryOrder::getUnitPrice() const { return unitPrice; }
    std::string LaundryOrder::getStatus() const { return status; }
    std::string LaundryOrder::getPaymentStatus() const { return paymentStatus; }
    std::string LaundryOrder::getOrderDate() const { return orderDate; }

    // ---------------- Setters ----------------
    void LaundryOrder::setOrderId(int id)
    {
        if (id <= 0)
            throw std::invalid_argument("Order ID must be a positive number.");
        orderId = id;
    }

    void LaundryOrder::setCustomerId(int id)
    {
        if (id <= 0)
            throw std::invalid_argument("Please select a valid customer.");
        customerId = id;
    }

    void LaundryOrder::setQuantity(double newQuantity)
    {
        if (!(newQuantity > 0.0))   // also rejects NaN
            throw std::invalid_argument("Quantity must be greater than zero.");

        if (usesWeight(serviceType))
        {
            if (newQuantity > 100.0)
                throw std::invalid_argument("Weight must be 100 kg or less for a single order.");
            // Weight is stored with 2 decimal places, like the database column.
            newQuantity = roundToCents(newQuantity);
            if (newQuantity <= 0.0)
                throw std::invalid_argument("Quantity must be greater than zero.");
        }
        else
        {
            if (std::floor(newQuantity) != newQuantity)
                throw std::invalid_argument("Number of items must be a whole number.");
            if (newQuantity > 500.0)
                throw std::invalid_argument("Number of items must be 500 or less for a single order.");
        }
        quantity = newQuantity;
    }

    void LaundryOrder::setOrderDate(const std::string& date)
    {
        // Expected format: YYYY-MM-DD
        bool formatOk = date.length() == 10 && date[4] == '-' && date[7] == '-';
        for (std::string::size_type i = 0; formatOk && i < date.length(); i++)
        {
            if (i != 4 && i != 7 && (date[i] < '0' || date[i] > '9'))
                formatOk = false;
        }
        if (!formatOk)
            throw std::invalid_argument("Order date must be in the format YYYY-MM-DD.");

        int month = std::stoi(date.substr(5, 2));
        int day = std::stoi(date.substr(8, 2));
        if (month < 1 || month > 12 || day < 1 || day > 31)
            throw std::invalid_argument("Order date is not a valid date.");

        orderDate = date;
    }

    void LaundryOrder::setStatus(const std::string& newStatus)
    {
        if (!isValidStatus(newStatus))
            throw std::invalid_argument("Unknown order status: " + newStatus);
        status = newStatus;
    }

    void LaundryOrder::setPaymentStatus(const std::string& newStatus)
    {
        if (!isValidPaymentStatus(newStatus))
            throw std::invalid_argument("Unknown payment status: " + newStatus);
        paymentStatus = newStatus;
    }

    // ---------------- Business rules ----------------
    void LaundryOrder::moveToStatus(const std::string& newStatus)
    {
        if (!isValidStatus(newStatus) || newStatus == OrderStatus::CANCELLED)
            throw std::invalid_argument("Please choose a valid processing status.");
        if (status == OrderStatus::CANCELLED)
            throw std::invalid_argument("This order has been cancelled. Its status cannot be changed.");
        if (status == OrderStatus::COMPLETED)
            throw std::invalid_argument("This order is already completed.");
        if (newStatus == status)
            throw std::invalid_argument("The order is already '" + status + "'.");

        std::string next = getNextStatus();
        if (newStatus != next)
            throw std::invalid_argument("Status can only move one step forward. The next status for this order is '" + next + "'.");

        if (newStatus == OrderStatus::COMPLETED && paymentStatus != PaymentStatus::PAID)
            throw std::invalid_argument("The order must be fully paid before it can be marked as Completed.");

        status = newStatus;
    }

    void LaundryOrder::cancel()
    {
        if (status == OrderStatus::CANCELLED)
            throw std::invalid_argument("This order is already cancelled.");
        if (status == OrderStatus::COMPLETED)
            throw std::invalid_argument("A completed order cannot be cancelled.");
        if (paymentStatus != PaymentStatus::UNPAID)
            throw std::invalid_argument("This order already has a payment recorded, so it cannot be cancelled.");
        status = OrderStatus::CANCELLED;
    }

    bool LaundryOrder::canEditDetails() const
    {
        return status == OrderStatus::PENDING && paymentStatus == PaymentStatus::UNPAID;
    }

    std::string LaundryOrder::getNextStatus() const
    {
        return nextStatusAfter(status);
    }

    // ---------------- Static helpers ----------------
    bool LaundryOrder::isValidService(const std::string& service)
    {
        return service == ServiceType::WASH_FOLD
            || service == ServiceType::DRY_CLEANING
            || service == ServiceType::STEAM_PRESS;
    }

    bool LaundryOrder::usesWeight(const std::string& service)
    {
        return service == ServiceType::WASH_FOLD;
    }

    std::string LaundryOrder::quantityLabel(const std::string& service)
    {
        return usesWeight(service) ? "Weight (kg)" : "Number of Items";
    }

    bool LaundryOrder::isValidStatus(const std::string& value)
    {
        return value == OrderStatus::PENDING
            || value == OrderStatus::IN_WASH
            || value == OrderStatus::READY
            || value == OrderStatus::COMPLETED
            || value == OrderStatus::CANCELLED;
    }

    bool LaundryOrder::isValidPaymentStatus(const std::string& value)
    {
        return value == PaymentStatus::UNPAID
            || value == PaymentStatus::PARTIALLY_PAID
            || value == PaymentStatus::PAID;
    }

    std::string LaundryOrder::nextStatusAfter(const std::string& current)
    {
        if (current == OrderStatus::PENDING) return OrderStatus::IN_WASH;
        if (current == OrderStatus::IN_WASH) return OrderStatus::READY;
        if (current == OrderStatus::READY) return OrderStatus::COMPLETED;
        return "";
    }
}
