#pragma once

// ============================================================================
// OrderForm.h  -  SCREEN 3: Order Management
// Create orders, calculate bills, update details/status, cancel and search.
// ============================================================================

#include "UiHelpers.h"
#include "../Database/DatabaseManager.h"
#include "../Services/BillingService.h"

#include <memory>
#include <string>

namespace FreshFold
{
    public ref class OrderForm : public System::Windows::Forms::Form
    {
    public:
        OrderForm(DatabaseManager* database, BillingService* billing);

    private:
        DatabaseManager* database;
        BillingService* billing;

        System::Windows::Forms::TextBox^ txtOrderId;
        System::Windows::Forms::ComboBox^ cmbCustomer;
        System::Windows::Forms::ComboBox^ cmbService;
        System::Windows::Forms::Label^ lblQuantity;
        System::Windows::Forms::TextBox^ txtQuantity;
        System::Windows::Forms::DateTimePicker^ dtpOrderDate;
        System::Windows::Forms::ComboBox^ cmbStatus;
        System::Windows::Forms::Label^ lblCurrentStatus;
        System::Windows::Forms::TextBox^ txtBill;
        System::Windows::Forms::TextBox^ txtSearch;
        System::Windows::Forms::DataGridView^ gridOrders;
        System::Windows::Forms::Label^ lblStatus;

        void buildLayout();
        void loadCustomerChoices();
        void showOrders(System::Data::DataTable^ table);
        void loadAllOrders();
        void clearForm();
        void loadOrderIntoForm(int orderId);
        bool readSelectedOrderId(int% orderId);

        // Reads and validates the inputs. Throw std::invalid_argument on bad input.
        std::string readService();
        double readQuantity(const std::string& service);
        std::unique_ptr<LaundryOrder> buildOrderFromForm();

        void onFormShown(System::Object^ sender, System::EventArgs^ e);
        void onServiceChanged(System::Object^ sender, System::EventArgs^ e);
        void onCalculateClick(System::Object^ sender, System::EventArgs^ e);
        void onCreateClick(System::Object^ sender, System::EventArgs^ e);
        void onUpdateOrderClick(System::Object^ sender, System::EventArgs^ e);
        void onUpdateStatusClick(System::Object^ sender, System::EventArgs^ e);
        void onCancelOrderClick(System::Object^ sender, System::EventArgs^ e);
        void onClearClick(System::Object^ sender, System::EventArgs^ e);
        void onBillingClick(System::Object^ sender, System::EventArgs^ e);
        void onSearchClick(System::Object^ sender, System::EventArgs^ e);
        void onShowAllClick(System::Object^ sender, System::EventArgs^ e);
        void onSearchKeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e);
        void onGridCellClick(System::Object^ sender, System::Windows::Forms::DataGridViewCellEventArgs^ e);
    };
}
