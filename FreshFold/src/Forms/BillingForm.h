#pragma once

// ============================================================================
// BillingForm.h  -  SCREEN 4: Billing and Payment
// Find an order, view a formatted invoice, record Cash/Card payments
// (partial payments allowed) and print the invoice.
// ============================================================================

#include "UiHelpers.h"
#include "../Database/DatabaseManager.h"
#include "../Services/BillingService.h"

namespace FreshFold
{
    public ref class BillingForm : public System::Windows::Forms::Form
    {
    public:
        // initialOrderId = 0 opens the screen empty; otherwise that order is loaded.
        BillingForm(DatabaseManager* database, BillingService* billing, int initialOrderId);

    private:
        DatabaseManager* database;
        BillingService* billing;
        int initialOrderId;
        int currentOrderId;   // 0 = no order loaded

        System::Windows::Forms::TextBox^ txtOrderId;
        System::Windows::Forms::Label^ lblCustomerValue;
        System::Windows::Forms::Label^ lblServiceValue;
        System::Windows::Forms::Label^ lblQuantityValue;
        System::Windows::Forms::Label^ lblTotalValue;
        System::Windows::Forms::Label^ lblPaidValue;
        System::Windows::Forms::Label^ lblBalanceValue;
        System::Windows::Forms::Label^ lblPaymentStatusValue;
        System::Windows::Forms::ComboBox^ cmbMethod;
        System::Windows::Forms::TextBox^ txtAmount;
        System::Windows::Forms::Button^ btnRecordPayment;
        System::Windows::Forms::Button^ btnPrint;
        System::Windows::Forms::TextBox^ txtInvoice;
        System::Windows::Forms::DataGridView^ gridPayments;
        System::Windows::Forms::Label^ lblStatus;

        void buildLayout();
        System::Windows::Forms::Label^ addSummaryRow(System::String^ caption, int y);
        void clearDisplay();
        void showInvoice(int orderId);

        void onFormShown(System::Object^ sender, System::EventArgs^ e);
        void onLoadClick(System::Object^ sender, System::EventArgs^ e);
        void onOrderIdKeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e);
        void onRecordPaymentClick(System::Object^ sender, System::EventArgs^ e);
        void onPrintClick(System::Object^ sender, System::EventArgs^ e);
        void onPrintPage(System::Object^ sender, System::Drawing::Printing::PrintPageEventArgs^ e);
    };
}
