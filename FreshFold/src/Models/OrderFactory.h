#pragma once

// ============================================================================
// OrderFactory.h
// One place that decides WHICH class to create for a service:
//   "Dry Cleaning"               -> DryCleaningOrder (derived class)
//   "Wash & Fold", "Steam Press" -> LaundryOrder     (base class)
//
// Both functions return std::unique_ptr<LaundryOrder>, i.e. a BASE-CLASS
// pointer. The caller does not know (or care) which class is inside; when
// it calls order->calculateBill() the correct override runs at runtime.
// That is runtime polymorphism.
// ============================================================================

#include "LaundryOrder.h"

#include <memory>
#include <string>

namespace FreshFold
{
    // Creates a NEW order using today's prices from PriceList.h.
    std::unique_ptr<LaundryOrder> createOrder(const std::string& serviceType, double quantity);

    // Re-creates an EXISTING order from the prices that were saved in the database.
    std::unique_ptr<LaundryOrder> createOrderWithPrices(const std::string& serviceType, double quantity,
                                                        double unitPrice, double serviceFee);

    // Current price per kg/item for a service (from PriceList.h).
    double currentUnitPrice(const std::string& serviceType);
}
