#pragma once

// ============================================================================
// PriceList.h
// ALL service prices live in this one file. To change a price, edit the
// constant below and rebuild - no other file contains a price.
// Prices are in Sri Lankan Rupees (Rs.).
// ============================================================================

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

namespace FreshFold
{
    namespace PriceList
    {
        // Wash & Fold is charged per kilogram of clothes.
        const double WASH_FOLD_PER_KG = 350.00;

        // Steam Press is charged per clothing item.
        const double STEAM_PRESS_PER_ITEM = 120.00;

        // Dry Cleaning is charged per clothing item...
        const double DRY_CLEAN_PER_ITEM = 600.00;

        // ...plus one fixed handling fee per dry-cleaning order
        // (garment tagging, protective bags and special handling).
        const double DRY_CLEAN_HANDLING_FEE = 150.00;
    }

    // Rounds a money value to 2 decimal places (cents).
    inline double roundToCents(double amount)
    {
        return std::round(amount * 100.0) / 100.0;
    }

    // Formats a money value, e.g. 1950 -> "Rs. 1950.00".
    inline std::string formatRupees(double amount)
    {
        std::ostringstream out;
        out << "Rs. " << std::fixed << std::setprecision(2) << roundToCents(amount);
        return out.str();
    }

    // Formats a quantity: whole numbers without decimals, weights with 2 decimals.
    inline std::string formatQuantity(double quantity)
    {
        std::ostringstream out;
        if (std::floor(quantity) == quantity)
            out << static_cast<long long>(quantity);
        else
            out << std::fixed << std::setprecision(2) << quantity;
        return out.str();
    }
}
