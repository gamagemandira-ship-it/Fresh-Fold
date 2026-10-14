#pragma once

// ============================================================================
// DashboardForm.h  -  SCREEN 1 (part 2): Dashboard
// Shows live statistics from MySQL and the navigation menu
// (Customers, Orders, Billing, Logout).
// ============================================================================

#include "UiHelpers.h"
#include "../Database/DatabaseManager.h"
#include "../Services/BillingService.h"

namespace FreshFold
{
    public ref class DashboardForm : public System::Windows::Forms::Form
    {
    public:
        DashboardForm(DatabaseManager* database, BillingService* billing, System::String^ username);

        // True when the user clicked Logout (main() then shows the login screen again).
        property bool LogoutRequested
        {
            bool get() { return logoutRequested; }
        }

    private:
        DatabaseManager* database;
        BillingService* billing;
        System::String^ username;
        bool logoutRequested;

        System::Windows::Forms::Label^ lblTotalCustomers;
        System::Windows::Forms::Label^ lblTotalOrders;
        System::Windows::Forms::Label^ lblPendingOrders;
        System::Windows::Forms::Label^ lblReadyOrders;
        System::Windows::Forms::Label^ lblLastUpdated;
        System::Windows::Forms::Label^ lblError;

        void buildLayout();
        System::Windows::Forms::Button^ addNavButton(System::Windows::Forms::Panel^ menu, System::String^ text, int y);
        System::Windows::Forms::Label^ addStatCard(System::String^ title, int x, int y);
        void loadStatistics();

        void onFormShown(System::Object^ sender, System::EventArgs^ e);
        void onRefreshClick(System::Object^ sender, System::EventArgs^ e);
        void onCustomersClick(System::Object^ sender, System::EventArgs^ e);
        void onOrdersClick(System::Object^ sender, System::EventArgs^ e);
        void onBillingClick(System::Object^ sender, System::EventArgs^ e);
        void onLogoutClick(System::Object^ sender, System::EventArgs^ e);
    };
}
