#pragma once

// ============================================================================
// DryCleaningOrder.h
// OOP: INHERITANCE + POLYMORPHISM
//
// DryCleaningOrder IS-A LaundryOrder (public inheritance). It reuses all of
// the base class data and rules (customer, quantity, status, payment...)
// and adds one thing of its own: a fixed handling fee per order.
//
// It OVERRIDES the virtual methods calculateBill(), getServiceFee() and
// getBillBreakdown(), so when the program calls these methods through a
// LaundryOrder pointer/reference, the dry-cleaning versions run instead.
// ============================================================================

#include "LaundryOrder.h"
#include "PriceList.h"

namespace FreshFold
{
    class DryCleaningOrder : public LaundryOrder
    {
    private:
        double handlingFee;   // dry-cleaning-specific data

    public:
        // New orders use the current prices from PriceList.h.
        // Orders loaded from the database pass the prices that were saved.
        explicit DryCleaningOrder(double numberOfItems,
                                  double pricePerItem = PriceList::DRY_CLEAN_PER_ITEM,
                                  double handlingFee = PriceList::DRY_CLEAN_HANDLING_FEE);

        double getHandlingFee() const;

        // 'override' asks the compiler to confirm that these really replace
        // virtual methods from LaundryOrder.
        double calculateBill() const override;
        double getServiceFee() const override;
        std::string getBillBreakdown() const override;
    };
}
