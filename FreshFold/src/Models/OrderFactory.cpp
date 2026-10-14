#include "OrderFactory.h"
#include "DryCleaningOrder.h"
#include "PriceList.h"

#include <stdexcept>

namespace FreshFold
{
    double currentUnitPrice(const std::string& serviceType)
    {
        if (serviceType == ServiceType::WASH_FOLD) return PriceList::WASH_FOLD_PER_KG;
        if (serviceType == ServiceType::STEAM_PRESS) return PriceList::STEAM_PRESS_PER_ITEM;
        if (serviceType == ServiceType::DRY_CLEANING) return PriceList::DRY_CLEAN_PER_ITEM;
        throw std::invalid_argument("Please select a valid service.");
    }

    std::unique_ptr<LaundryOrder> createOrder(const std::string& serviceType, double quantity)
    {
        if (serviceType == ServiceType::DRY_CLEANING)
        {
            // Derived-class object stored in a base-class pointer.
            return std::unique_ptr<LaundryOrder>(new DryCleaningOrder(quantity));
        }
        return std::unique_ptr<LaundryOrder>(new LaundryOrder(serviceType, quantity, currentUnitPrice(serviceType)));
    }

    std::unique_ptr<LaundryOrder> createOrderWithPrices(const std::string& serviceType, double quantity,
                                                        double unitPrice, double serviceFee)
    {
        if (serviceType == ServiceType::DRY_CLEANING)
            return std::unique_ptr<LaundryOrder>(new DryCleaningOrder(quantity, unitPrice, serviceFee));

        return std::unique_ptr<LaundryOrder>(new LaundryOrder(serviceType, quantity, unitPrice));
    }
}
