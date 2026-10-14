#include "BillingService.h"
#include "../Models/OrderFactory.h"
#include "../Models/PriceList.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace FreshFold
{
    BillingService::BillingService(const DatabaseManager& database)
        : database(database)
    {
    }

    double BillingService::calculateBill(const LaundryOrder& order)
    {
        // Runtime polymorphism: which calculateBill() runs is decided by the
        // real type of 'order', not by the type of this parameter.
        return order.calculateBill();
    }

    BillQuote BillingService::quote(const std::string& serviceType, double quantity) const
    {
        // OBJECT: a temporary order object is created through a base-class pointer.
        std::unique_ptr<LaundryOrder> order = createOrder(serviceType, quantity);

        BillQuote result;
        result.total = calculateBill(*order);
        result.breakdown = order->getBillBreakdown();   // also virtual
        return result;
    }

    bool BillingService::loadInvoice(int orderId, Invoice& invoice) const
    {
        std::unique_ptr<LaundryOrder> order = database.findOrderById(orderId);
        if (!order)
            return false;

        Customer customer;
        if (!database.findCustomerById(order->getCustomerId(), customer))
            throw std::invalid_argument("The customer for this order was not found.");

        invoice.customer = customer;
        invoice.total = calculateBill(*order);
        invoice.totalPaid = database.getTotalPaid(orderId);
        invoice.balance = roundToCents(invoice.total - invoice.totalPaid);
        if (invoice.balance < 0.0)
            invoice.balance = 0.0;
        invoice.order = std::move(order);
        return true;
    }

    std::string BillingService::formatInvoice(const Invoice& invoice)
    {
        const LaundryOrder& order = *invoice.order;
        const std::string line(46, '=');
        const std::string thinLine(46, '-');

        std::ostringstream out;
        out << line << "\n"
            << "                  FRESHFOLD\n"
            << "   Laundry & Dry Cleaning Management System\n"
            << "                   INVOICE\n"
            << line << "\n"
            << "Invoice No. : INV-" << std::setw(6) << std::setfill('0') << order.getOrderId() << std::setfill(' ') << "\n"
            << "Order Date  : " << order.getOrderDate() << "\n"
            << thinLine << "\n"
            << "Customer    : #" << invoice.customer.getCustomerId() << " " << invoice.customer.getFullName() << "\n"
            << "Contact     : " << invoice.customer.getPhone() << "\n"
            << "Address     : " << invoice.customer.getAddress() << "\n"
            << thinLine << "\n"
            << order.getBillBreakdown() << "\n"      // virtual - correct text for each service
            << thinLine << "\n"
            << "Order status   : " << order.getStatus() << "\n"
            << "Payment status : " << order.getPaymentStatus() << "\n"
            << "Amount paid    : " << formatRupees(invoice.totalPaid) << "\n"
            << "BALANCE DUE    : " << formatRupees(invoice.balance) << "\n"
            << line << "\n"
            << "       Thank you for choosing FreshFold!\n";
        return out.str();
    }

    bool BillingService::isValidPaymentMethod(const std::string& method)
    {
        return method == "Cash" || method == "Card";
    }

    std::string BillingService::recordPayment(int orderId, double amount, const std::string& paymentMethod) const
    {
        if (orderId <= 0)
            throw std::invalid_argument("Please load an order first.");
        if (!isValidPaymentMethod(paymentMethod))
            throw std::invalid_argument("Please choose a payment method (Cash or Card).");
        if (!(amount > 0.0) || std::isinf(amount))
            throw std::invalid_argument("The payment amount must be greater than zero.");

        Invoice invoice;
        if (!loadInvoice(orderId, invoice))
            throw std::invalid_argument("The order was not found.");
        if (invoice.order->getStatus() == OrderStatus::CANCELLED)
            throw std::invalid_argument("Payments cannot be recorded for a cancelled order.");
        if (invoice.balance <= 0.0)
            throw std::invalid_argument("This order is already fully paid.");

        // Compare in whole cents to avoid floating-point rounding surprises.
        long long amountCents = std::llround(amount * 100.0);
        long long balanceCents = std::llround(invoice.balance * 100.0);
        if (amountCents > balanceCents)
            throw std::invalid_argument("The payment amount cannot be more than the outstanding balance ("
                                        + formatRupees(invoice.balance) + ").");

        // The database saves the payment and the new payment status in one
        // transaction. If anything fails, nothing is saved and an exception is thrown.
        return database.recordPayment(orderId, amountCents / 100.0, paymentMethod);
    }
}
