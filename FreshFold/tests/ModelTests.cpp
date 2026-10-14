// ============================================================================
// ModelTests.cpp
// Small console program that tests the business classes WITHOUT the GUI or
// the database. It also contains the runtime-polymorphism demonstration.
//
// It is NOT part of the Visual Studio GUI project. Build it separately:
//   Visual Studio "x64 Native Tools Command Prompt", from the project root:
//     cl /EHsc /std:c++14 /I src tests\ModelTests.cpp src\Models\Customer.cpp src\Models\LaundryOrder.cpp src\Models\DryCleaningOrder.cpp src\Models\OrderFactory.cpp src\Database\DatabaseConfig.cpp /Fe:ModelTests.exe
//   or with g++:
//     g++ -std=c++14 -Wall -Wextra -I src tests/ModelTests.cpp src/Models/*.cpp src/Database/DatabaseConfig.cpp -o ModelTests
// ============================================================================

#include "Models/Customer.h"
#include "Models/DryCleaningOrder.h"
#include "Models/LaundryOrder.h"
#include "Models/OrderFactory.h"
#include "Models/PriceList.h"
#include "Database/DatabaseConfig.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace FreshFold;

static int passed = 0;
static int failed = 0;

static void check(bool condition, const std::string& name)
{
    if (condition)
    {
        passed++;
        std::cout << "  [PASS] " << name << "\n";
    }
    else
    {
        failed++;
        std::cout << "  [FAIL] " << name << "\n";
    }
}

static bool sameMoney(double a, double b)
{
    return std::fabs(a - b) < 0.001;
}

// Runs an action and reports whether it threw std::invalid_argument.
template <typename Action>
static bool throwsInvalid(Action action)
{
    try
    {
        action();
    }
    catch (const std::invalid_argument&)
    {
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// POLYMORPHISM DEMO: this function only knows about the BASE class.
// It receives a LaundryOrder reference, but calculateBill() is virtual, so
// a DryCleaningOrder passed in here uses DryCleaningOrder::calculateBill().
// ----------------------------------------------------------------------------
static double billThroughBaseReference(const LaundryOrder& order)
{
    return order.calculateBill();
}

static void testCustomer()
{
    std::cout << "Customer\n";

    Customer c("  Sample Customer  ", "071 234 5678", "No. 1, Sample Road, Anuradhapura");
    check(c.getFullName() == "Sample Customer", "name is trimmed");
    check(c.getCustomerId() == 0, "new customer has ID 0 (not saved)");
    c.setCustomerId(12);
    check(c.getCustomerId() == 12, "ID can be set after saving");

    check(throwsInvalid([] { Customer("", "0712345678", "Somewhere"); }), "empty name rejected");
    check(throwsInvalid([] { Customer("Sample", "   ", "Somewhere"); }), "empty phone rejected");
    check(throwsInvalid([] { Customer("Sample", "0712345678", ""); }), "empty address rejected");
    check(throwsInvalid([] { Customer("Sample", "abc1234567", "Somewhere"); }), "letters in phone rejected");
    check(throwsInvalid([] { Customer("Sample", "12345", "Somewhere"); }), "too few digits rejected");
    check(!throwsInvalid([] { Customer("Sample", "+94 (71) 234-5678", "Somewhere"); }), "international format accepted");
    check(throwsInvalid([&c] { c.setCustomerId(0); }), "ID 0 rejected by setter");

    // Failed setter must not change the object (encapsulation keeps it valid).
    try { c.setFullName(""); } catch (const std::invalid_argument&) {}
    check(c.getFullName() == "Sample Customer", "invalid setter call leaves old value");
}

static void testBilling()
{
    std::cout << "Billing\n";

    LaundryOrder wash(ServiceType::WASH_FOLD, 2.5, PriceList::WASH_FOLD_PER_KG);
    check(sameMoney(wash.calculateBill(), 2.5 * PriceList::WASH_FOLD_PER_KG), "Wash & Fold = kg x price per kg");
    check(sameMoney(wash.getServiceFee(), 0.0), "Wash & Fold has no service fee");

    LaundryOrder press(ServiceType::STEAM_PRESS, 4, PriceList::STEAM_PRESS_PER_ITEM);
    check(sameMoney(press.calculateBill(), 4 * PriceList::STEAM_PRESS_PER_ITEM), "Steam Press = items x price per item");

    DryCleaningOrder dry(3);
    double expectedDry = 3 * PriceList::DRY_CLEAN_PER_ITEM + PriceList::DRY_CLEAN_HANDLING_FEE;
    check(sameMoney(dry.calculateBill(), expectedDry), "Dry Cleaning = items x price + handling fee");
    check(sameMoney(dry.getServiceFee(), PriceList::DRY_CLEAN_HANDLING_FEE), "Dry Cleaning service fee = handling fee");

    // Saved prices are respected (old orders keep their original price).
    std::unique_ptr<LaundryOrder> old = createOrderWithPrices(ServiceType::DRY_CLEANING, 2, 500.0, 100.0);
    check(sameMoney(old->calculateBill(), 1100.0), "order rebuilt from saved prices");

    check(wash.getBillBreakdown().find("2.50 kg x Rs. 350.00 per kg = Rs. 875.00") != std::string::npos,
          "Wash & Fold breakdown text");
    check(dry.getBillBreakdown().find("Handling fee") != std::string::npos, "Dry Cleaning breakdown mentions fee");
    std::cout << "\n--- Sample invoice breakdown (Dry Cleaning) ---\n" << dry.getBillBreakdown() << "\n\n";
}

static void testPolymorphism()
{
    std::cout << "Runtime polymorphism\n";

    // A vector of BASE-class pointers holding different kinds of objects.
    std::vector<std::unique_ptr<LaundryOrder>> orders;
    orders.push_back(createOrder(ServiceType::WASH_FOLD, 2.0));
    orders.push_back(createOrder(ServiceType::DRY_CLEANING, 3.0));
    orders.push_back(createOrder(ServiceType::STEAM_PRESS, 5.0));

    for (const std::unique_ptr<LaundryOrder>& order : orders)
    {
        // Same call for every object; the version that runs depends on the real object type.
        std::cout << "    " << order->getServiceType() << " -> " << formatRupees(order->calculateBill()) << "\n";
    }

    check(dynamic_cast<DryCleaningOrder*>(orders[1].get()) != nullptr, "factory created a DryCleaningOrder");
    check(sameMoney(orders[1]->calculateBill(),
                    3 * PriceList::DRY_CLEAN_PER_ITEM + PriceList::DRY_CLEAN_HANDLING_FEE),
          "base pointer calls DryCleaningOrder::calculateBill()");
    check(sameMoney(billThroughBaseReference(*orders[1]), orders[1]->calculateBill()),
          "base reference calls the derived override");
    check(sameMoney(billThroughBaseReference(*orders[0]), 2.0 * PriceList::WASH_FOLD_PER_KG),
          "base reference calls the base version for Wash & Fold");
}

static void testQuantityValidation()
{
    std::cout << "Quantity validation\n";
    check(throwsInvalid([] { createOrder(ServiceType::WASH_FOLD, 0); }), "zero weight rejected");
    check(throwsInvalid([] { createOrder(ServiceType::WASH_FOLD, -1.5); }), "negative weight rejected");
    check(throwsInvalid([] { createOrder(ServiceType::WASH_FOLD, std::nan("")); }), "NaN weight rejected");
    check(!throwsInvalid([] { createOrder(ServiceType::WASH_FOLD, 1.75); }), "decimal weight accepted");
    check(throwsInvalid([] { createOrder(ServiceType::DRY_CLEANING, 2.5); }), "fractional item count rejected");
    check(throwsInvalid([] { createOrder(ServiceType::STEAM_PRESS, 0); }), "zero items rejected");
    check(throwsInvalid([] { createOrder("Ironing", 2); }), "unknown service rejected");
    check(throwsInvalid([] { createOrder(ServiceType::STEAM_PRESS, 501); }), "too many items rejected");
}

static void testStatusWorkflow()
{
    std::cout << "Status workflow\n";
    std::unique_ptr<LaundryOrder> order = createOrder(ServiceType::STEAM_PRESS, 2);
    check(order->getStatus() == OrderStatus::PENDING, "new order is Pending");
    check(order->canEditDetails(), "pending unpaid order can be edited");

    check(throwsInvalid([&order] { order->moveToStatus(OrderStatus::READY); }), "cannot skip a step");
    order->moveToStatus(OrderStatus::IN_WASH);
    check(order->getStatus() == OrderStatus::IN_WASH, "Pending -> In Wash");
    check(!order->canEditDetails(), "order in wash cannot be edited");
    check(throwsInvalid([&order] { order->moveToStatus(OrderStatus::PENDING); }), "cannot move backwards");
    order->moveToStatus(OrderStatus::READY);
    check(throwsInvalid([&order] { order->moveToStatus(OrderStatus::COMPLETED); }), "unpaid order cannot be completed");
    order->setPaymentStatus(PaymentStatus::PAID);
    order->moveToStatus(OrderStatus::COMPLETED);
    check(order->getStatus() == OrderStatus::COMPLETED, "paid order -> Completed");
    check(throwsInvalid([&order] { order->cancel(); }), "completed order cannot be cancelled");

    std::unique_ptr<LaundryOrder> second = createOrder(ServiceType::WASH_FOLD, 1);
    second->cancel();
    check(second->getStatus() == OrderStatus::CANCELLED, "unpaid order can be cancelled");
    check(throwsInvalid([&second] { second->moveToStatus(OrderStatus::IN_WASH); }), "cancelled order status is frozen");

    std::unique_ptr<LaundryOrder> third = createOrder(ServiceType::WASH_FOLD, 1);
    third->setPaymentStatus(PaymentStatus::PARTIALLY_PAID);
    check(throwsInvalid([&third] { third->cancel(); }), "order with a payment cannot be cancelled");

    check(throwsInvalid([&third] { third->setStatus("Lost"); }), "unknown status rejected");
    check(throwsInvalid([&third] { third->setOrderDate("12/10/2026"); }), "wrong date format rejected");
    check(!throwsInvalid([&third] { third->setOrderDate("2026-10-12"); }), "ISO date accepted");
    check(LaundryOrder::quantityLabel(ServiceType::WASH_FOLD) == "Weight (kg)", "label for Wash & Fold");
    check(LaundryOrder::quantityLabel(ServiceType::DRY_CLEANING) == "Number of Items", "label for Dry Cleaning");
}

static void testConfig()
{
    std::cout << "Database configuration file\n";
    const char* path = "test_database.ini";
    {
        std::ofstream out(path);
        out << "\xEF\xBB\xBF; comment\n[mysql]\nserver = 127.0.0.1\nport=3307\ndatabase=freshfold_db\nuser=freshfold_app\npassword=p@ss=word\n";
    }
    DatabaseConfig config = DatabaseConfig::loadFromFile(path);
    check(config.server == "127.0.0.1" && config.port == 3307, "server and port read");
    check(config.user == "freshfold_app", "user read");
    check(config.password == "p@ss=word", "password containing '=' read correctly");
    std::remove(path);

    bool threw = false;
    try { DatabaseConfig::loadFromFile("missing_file.ini"); } catch (const std::runtime_error&) { threw = true; }
    check(threw, "missing file reported");
}

int main()
{
    testCustomer();
    testBilling();
    testPolymorphism();
    testQuantityValidation();
    testStatusWorkflow();
    testConfig();

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
