#include "BillingForm.h"
#include "../Models/PriceList.h"
#include "../Utils/StringUtils.h"

#include <stdexcept>

using namespace System;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Drawing::Printing;
using namespace System::Globalization;
using namespace System::Windows::Forms;

namespace FreshFold
{
    BillingForm::BillingForm(DatabaseManager* database, BillingService* billing, int initialOrderId)
        : database(database),
          billing(billing),
          initialOrderId(initialOrderId),
          currentOrderId(0)
    {
        buildLayout();
    }

    void BillingForm::buildLayout()
    {
        Ui::StyleForm(this, "Billing", 1000, 640);
        Controls->Add(Ui::MakeHeader("Billing & Payment", "View invoices and record Cash or Card payments"));

        // ---------- Find order ----------
        Label^ findTitle = Ui::MakeLabel("Find Order", 20, 78);
        findTitle->Font = Ui::MakeFont(11.0f, true);
        findTitle->ForeColor = Ui::Navy();
        Controls->Add(findTitle);

        Controls->Add(Ui::MakeLabel("Order ID", 20, 112));
        txtOrderId = Ui::MakeTextBox(110, 112, 120);
        txtOrderId->KeyDown += gcnew KeyEventHandler(this, &BillingForm::onOrderIdKeyDown);
        Controls->Add(txtOrderId);

        Button^ btnLoad = Ui::MakeButton("Load Order", 240, 109, 160, true);
        btnLoad->Click += gcnew EventHandler(this, &BillingForm::onLoadClick);
        Controls->Add(btnLoad);

        // ---------- Order summary ----------
        lblCustomerValue = addSummaryRow("Customer", 156);
        lblServiceValue = addSummaryRow("Service", 184);
        lblQuantityValue = addSummaryRow("Quantity", 212);
        lblTotalValue = addSummaryRow("Total", 240);
        lblPaidValue = addSummaryRow("Paid so far", 268);
        lblBalanceValue = addSummaryRow("Balance due", 296);
        lblPaymentStatusValue = addSummaryRow("Payment status", 324);

        // ---------- Record payment ----------
        Label^ paymentTitle = Ui::MakeLabel("Record Payment", 20, 362);
        paymentTitle->Font = Ui::MakeFont(11.0f, true);
        paymentTitle->ForeColor = Ui::Navy();
        Controls->Add(paymentTitle);

        Controls->Add(Ui::MakeLabel("Payment method", 20, 396));
        cmbMethod = Ui::MakeDropDown(150, 396, 140);
        cmbMethod->Items->Add("Cash");
        cmbMethod->Items->Add("Card");
        cmbMethod->SelectedIndex = 0;
        Controls->Add(cmbMethod);

        Controls->Add(Ui::MakeLabel("Amount (Rs.)", 20, 432));
        txtAmount = Ui::MakeTextBox(150, 432, 140);
        Controls->Add(txtAmount);

        btnRecordPayment = Ui::MakeButton("Record Payment", 20, 472, 185, true);
        btnRecordPayment->Click += gcnew EventHandler(this, &BillingForm::onRecordPaymentClick);
        Controls->Add(btnRecordPayment);

        btnPrint = Ui::MakeButton("Print Invoice", 215, 472, 185, false);
        btnPrint->Click += gcnew EventHandler(this, &BillingForm::onPrintClick);
        Controls->Add(btnPrint);

        lblStatus = gcnew Label();
        lblStatus->AutoSize = false;
        lblStatus->Size = System::Drawing::Size(380, 80);
        lblStatus->Location = Point(20, 516);
        Controls->Add(lblStatus);

        // ---------- Invoice + payment history ----------
        Label^ invoiceTitle = Ui::MakeLabel("Invoice", 430, 78);
        invoiceTitle->Font = Ui::MakeFont(11.0f, true);
        invoiceTitle->ForeColor = Ui::Navy();
        Controls->Add(invoiceTitle);

        txtInvoice = gcnew TextBox();
        txtInvoice->Multiline = true;
        txtInvoice->ReadOnly = true;
        txtInvoice->ScrollBars = ScrollBars::Vertical;
        txtInvoice->Font = gcnew System::Drawing::Font("Consolas", 9.5f);
        txtInvoice->BackColor = Color::White;
        txtInvoice->Location = Point(430, 104);
        txtInvoice->Size = System::Drawing::Size(550, 330);
        Controls->Add(txtInvoice);

        Label^ historyTitle = Ui::MakeLabel("Payment History", 430, 444);
        historyTitle->Font = Ui::MakeFont(11.0f, true);
        historyTitle->ForeColor = Ui::Navy();
        Controls->Add(historyTitle);

        gridPayments = Ui::MakeGrid(430, 470, 550, 150);
        Controls->Add(gridPayments);

        Shown += gcnew EventHandler(this, &BillingForm::onFormShown);
        clearDisplay();
    }

    // Adds "Caption:  value" and returns the value label.
    Label^ BillingForm::addSummaryRow(String^ caption, int y)
    {
        Controls->Add(Ui::MakeLabel(String::Concat(caption, ":"), 20, y));

        Label^ value = gcnew Label();
        value->AutoSize = false;
        value->Size = System::Drawing::Size(260, 22);
        value->Location = Point(150, y + 3);
        value->Font = Ui::MakeFont(9.75f, true);
        Controls->Add(value);
        return value;
    }

    void BillingForm::clearDisplay()
    {
        currentOrderId = 0;
        lblCustomerValue->Text = "-";
        lblServiceValue->Text = "-";
        lblQuantityValue->Text = "-";
        lblTotalValue->Text = "-";
        lblPaidValue->Text = "-";
        lblBalanceValue->Text = "-";
        lblPaymentStatusValue->Text = "-";
        txtAmount->Clear();
        txtInvoice->Clear();
        gridPayments->DataSource = nullptr;
        btnRecordPayment->Enabled = false;
        btnPrint->Enabled = false;
    }

    // Loads the order, its customer and its payments, then fills the screen.
    void BillingForm::showInvoice(int orderId)
    {
        try
        {
            Invoice invoice;
            if (!billing->loadInvoice(orderId, invoice))
            {
                clearDisplay();
                Ui::SetStatus(lblStatus, String::Format("Order #{0} was not found.", orderId), false);
                return;
            }

            const LaundryOrder& order = *invoice.order;
            currentOrderId = orderId;
            txtOrderId->Text = Convert::ToString(orderId);

            lblCustomerValue->Text = String::Format("#{0}  {1}  ({2})", invoice.customer.getCustomerId(),
                                                    toManagedString(invoice.customer.getFullName()),
                                                    toManagedString(invoice.customer.getPhone()));
            lblServiceValue->Text = toManagedString(order.getServiceType());
            std::string unit = " item(s)";
            if (LaundryOrder::usesWeight(order.getServiceType()))
                unit = " kg";
            lblQuantityValue->Text = toManagedString(formatQuantity(order.getQuantity()) + unit);
            lblTotalValue->Text = toManagedString(formatRupees(invoice.total));
            lblPaidValue->Text = toManagedString(formatRupees(invoice.totalPaid));
            lblBalanceValue->Text = toManagedString(formatRupees(invoice.balance));
            lblPaymentStatusValue->Text = toManagedString(order.getPaymentStatus());

            txtInvoice->Text = toDisplayText(BillingService::formatInvoice(invoice));

            gridPayments->DataSource = database->getPaymentsForOrder(orderId);
            Ui::FormatColumn(gridPayments, "Amount (Rs.)", "N2");
            gridPayments->ClearSelection();

            bool cancelled = order.getStatus() == OrderStatus::CANCELLED;
            bool canPay = !cancelled && invoice.balance > 0.0;
            btnRecordPayment->Enabled = canPay;
            btnPrint->Enabled = true;

            if (canPay)
            {
                // Suggest the full outstanding balance; the user can change it for a partial payment.
                txtAmount->Text = String::Format(CultureInfo::InvariantCulture, "{0:0.00}", invoice.balance);
                Ui::SetStatus(lblStatus, String::Format("Order #{0} loaded. Balance due: {1}", orderId,
                                                        toManagedString(formatRupees(invoice.balance))), true);
            }
            else if (cancelled)
            {
                txtAmount->Clear();
                Ui::SetStatus(lblStatus, "This order is cancelled. Payments cannot be recorded.", false);
            }
            else
            {
                txtAmount->Clear();
                Ui::SetStatus(lblStatus, "This order is fully paid.", true);
            }
        }
        catch (const std::exception& ex)
        {
            clearDisplay();
            Ui::SetStatus(lblStatus, toManagedString(ex.what()), false);
        }
        catch (DatabaseException^ ex)
        {
            clearDisplay();
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    void BillingForm::onFormShown(Object^, EventArgs^)
    {
        if (initialOrderId > 0)
            showInvoice(initialOrderId);
        else
            txtOrderId->Focus();
    }

    void BillingForm::onLoadClick(Object^, EventArgs^)
    {
        int orderId = 0;
        if (!Int32::TryParse(txtOrderId->Text->Trim(), orderId) || orderId <= 0)
        {
            Ui::SetStatus(lblStatus, "Please enter a valid order ID (a positive whole number).", false);
            return;
        }
        showInvoice(orderId);
    }

    void BillingForm::onOrderIdKeyDown(Object^, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter)
        {
            e->SuppressKeyPress = true;
            onLoadClick(nullptr, nullptr);
        }
    }

    void BillingForm::onRecordPaymentClick(Object^, EventArgs^)
    {
        if (currentOrderId <= 0)
        {
            Ui::ShowError("Please load an order first.");
            return;
        }
        if (cmbMethod->SelectedIndex < 0)
        {
            Ui::SetStatus(lblStatus, "Please choose a payment method.", false);
            return;
        }

        Decimal amount = Decimal::Zero;
        if (!Decimal::TryParse(txtAmount->Text->Trim(), NumberStyles::Number, CultureInfo::InvariantCulture, amount))
        {
            Ui::SetStatus(lblStatus, "Please enter a valid amount, for example 1500.00", false);
            return;
        }
        if (Decimal::Compare(Decimal::Round(amount, 2), amount) != 0)
        {
            Ui::SetStatus(lblStatus, "The amount can have at most 2 decimal places.", false);
            return;
        }

        String^ method = cmbMethod->SelectedItem->ToString();
        if (!Ui::Confirm(String::Format("Record a {0} payment of Rs. {1:N2} for order #{2}?", method, amount, currentOrderId)))
            return;

        int orderId = currentOrderId;
        try
        {
            // BillingService validates the amount (must be positive and not more
            // than the balance). The order is only marked as paid if the database
            // transaction succeeds; otherwise an exception is thrown and nothing changes.
            std::string newStatus = billing->recordPayment(orderId, Decimal::ToDouble(amount), toStdString(method));

            showInvoice(orderId);
            Ui::SetStatus(lblStatus, String::Format("Payment saved. The payment status is now '{0}'.",
                                                    toManagedString(newStatus)), true);
        }
        catch (const std::exception& ex)
        {
            Ui::SetStatus(lblStatus, toManagedString(ex.what()), false);
        }
        catch (DatabaseException^ ex)
        {
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    // Shows a print preview; from there the invoice can be printed
    // (or saved as PDF with "Microsoft Print to PDF").
    void BillingForm::onPrintClick(Object^, EventArgs^)
    {
        if (currentOrderId <= 0 || txtInvoice->Text->Length == 0)
        {
            Ui::ShowError("Please load an order first.");
            return;
        }

        try
        {
            PrintDocument^ document = gcnew PrintDocument();
            document->DocumentName = String::Format("FreshFold Invoice INV-{0:000000}", currentOrderId);
            document->PrintPage += gcnew PrintPageEventHandler(this, &BillingForm::onPrintPage);

            PrintPreviewDialog^ preview = gcnew PrintPreviewDialog();
            preview->Document = document;
            preview->Width = 800;
            preview->Height = 900;
            preview->ShowDialog(this);

            delete preview;
            delete document;
        }
        catch (Exception^ ex)
        {
            Ui::ShowError(String::Concat(
                "The invoice could not be printed. Check that a printer (for example "
                "'Microsoft Print to PDF') is installed.\n\n", ex->Message));
        }
    }

    void BillingForm::onPrintPage(Object^, PrintPageEventArgs^ e)
    {
        System::Drawing::Font^ printFont = gcnew System::Drawing::Font("Consolas", 10.0f);
        e->Graphics->DrawString(txtInvoice->Text, printFont, Brushes::Black,
                                static_cast<float>(e->MarginBounds.Left), static_cast<float>(e->MarginBounds.Top));
        delete printFont;
        e->HasMorePages = false;   // the invoice always fits on one page
    }
}
