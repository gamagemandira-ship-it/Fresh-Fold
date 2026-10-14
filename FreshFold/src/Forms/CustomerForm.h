#pragma once

// ============================================================================
// CustomerForm.h  -  SCREEN 2: Customer Management
// Add, view, search, update and delete customers (real MySQL CRUD).
// ============================================================================

#include "UiHelpers.h"
#include "../Database/DatabaseManager.h"

namespace FreshFold
{
    public ref class CustomerForm : public System::Windows::Forms::Form
    {
    public:
        CustomerForm(DatabaseManager* database);

    private:
        DatabaseManager* database;

        System::Windows::Forms::TextBox^ txtCustomerId;
        System::Windows::Forms::TextBox^ txtName;
        System::Windows::Forms::TextBox^ txtPhone;
        System::Windows::Forms::TextBox^ txtAddress;
        System::Windows::Forms::TextBox^ txtSearch;
        System::Windows::Forms::DataGridView^ gridCustomers;
        System::Windows::Forms::Label^ lblStatus;

        void buildLayout();
        void showCustomers(System::Data::DataTable^ table);
        void loadAllCustomers();
        void clearForm();
        bool readSelectedId(int% customerId);

        void onFormShown(System::Object^ sender, System::EventArgs^ e);
        void onAddClick(System::Object^ sender, System::EventArgs^ e);
        void onUpdateClick(System::Object^ sender, System::EventArgs^ e);
        void onDeleteClick(System::Object^ sender, System::EventArgs^ e);
        void onClearClick(System::Object^ sender, System::EventArgs^ e);
        void onSearchClick(System::Object^ sender, System::EventArgs^ e);
        void onViewAllClick(System::Object^ sender, System::EventArgs^ e);
        void onSearchKeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e);
        void onGridCellClick(System::Object^ sender, System::Windows::Forms::DataGridViewCellEventArgs^ e);
    };
}
