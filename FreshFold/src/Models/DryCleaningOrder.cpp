#include "DryCleaningOrder.h"

#include <sstream>
#include <stdexcept>

namespace FreshFold
{
    // The base-class constructor runs first and stores the common data.
    DryCleaningOrder::DryCleaningOrder(double numberOfItems, double pricePerItem, double handlingFee)
        : LaundryOrder(ServiceType::DRY_CLEANING, numberOfItems, pricePerItem),
          handlingFee(handlingFee)
    {
        if (handlingFee < 0.0)
            throw std::invalid_argument("Handling fee cannot be negative.");
    }

    double DryCleaningOrder::getHandlingFee() const
    {
        return handlingFee;
    }

    // Dry-cleaning bill = (items x price per item) + handling fee.
    // It reuses the base-class formula and then adds its own part.
    double DryCleaningOrder::calculateBill() const
    {
        return roundToCents(LaundryOrder::calculateBill() + handlingFee);
    }

    double DryCleaningOrder::getServiceFee() const
    {
        return handlingFee;
    }

    std::string DryCleaningOrder::getBillBreakdown() const
    {
        std::ostringstream out;
        out << "Service     : " << getServiceType() << "\n"
            << "Calculation : " << describeQuantityTimesPrice() << "\n"
            << "Handling fee: " << formatRupees(handlingFee) << "\n"
            << "Total       : " << formatRupees(calculateBill());
        return out.str();
    }
}
