#include "CustomerForm.h"
#include "../Models/Customer.h"
#include "../Utils/StringUtils.h"

#include <stdexcept>

using namespace System;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace FreshFold
{
    CustomerForm::CustomerForm(DatabaseManager* database)
        : database(database)
    {
        buildLayout();
    }

    void CustomerForm::buildLayout()
    {
        Ui::StyleForm(this, "Customers", 1000, 620);
        Controls->Add(Ui::MakeHeader("Customer Management", "Register, find, update and remove customers"));

        // ---------- Customer details (left side) ----------
        Label^ detailsTitle = Ui::MakeLabel("Customer Details", 20, 78);
        detailsTitle->Font = Ui::MakeFont(11.0f, true);
        detailsTitle->ForeColor = Ui::Navy();
        Controls->Add(detailsTitle);

        Controls->Add(Ui::MakeLabel("Customer ID", 20, 112));
        txtCustomerId = Ui::MakeTextBox(150, 112, 190);
        txtCustomerId->ReadOnly = true;   // assigned automatically by MySQL
        Controls->Add(txtCustomerId);

        Controls->Add(Ui::MakeLabel("Name *", 20, 148));
        txtName = Ui::MakeTextBox(150, 148, 190);
        txtName->MaxLength = 100;
        Controls->Add(txtName);

        Controls->Add(Ui::MakeLabel("Contact Number *", 20, 184));
        txtPhone = Ui::MakeTextBox(150, 184, 190);
        txtPhone->MaxLength = 20;
        Controls->Add(txtPhone);

        Controls->Add(Ui::MakeLabel("Address *", 20, 220));
        txtAddress = Ui::MakeTextBox(150, 220, 190);
        txtAddress->Multiline = true;
        txtAddress->Height = 64;
        txtAddress->MaxLength = 255;
        Controls->Add(txtAddress);

        Button^ btnAdd = Ui::MakeButton("Add Customer", 20, 304, 155, true);
        btnAdd->Click += gcnew EventHandler(this, &CustomerForm::onAddClick);
        Controls->Add(btnAdd);

        Button^ btnUpdate = Ui::MakeButton("Update Customer", 185, 304, 155, false);
        btnUpdate->Click += gcnew EventHandler(this, &CustomerForm::onUpdateClick);
        Controls->Add(btnUpdate);

        Button^ btnDelete = Ui::MakeButton("Delete Customer", 20, 344, 155, false);
        btnDelete->Click += gcnew EventHandler(this, &CustomerForm::onDeleteClick);
        Controls->Add(btnDelete);

        Button^ btnClear = Ui::MakeButton("Clear Form", 185, 344, 155, false);
        btnClear->Click += gcnew EventHandler(this, &CustomerForm::onClearClick);
        Controls->Add(btnClear);

        lblStatus = gcnew Label();
        lblStatus->AutoSize = false;
        lblStatus->Size = System::Drawing::Size(320, 90);
        lblStatus->Location = Point(20, 392);
        Controls->Add(lblStatus);

        Label^ tip = Ui::MakeLabel("Tip: click a row in the table to load it for editing.\n* = required field", 20, 540);
        tip->ForeColor = Color::DimGray;
        Controls->Add(tip);

        // ---------- Search + table (right side) ----------
        Controls->Add(Ui::MakeLabel("Search", 370, 80));
        txtSearch = Ui::MakeTextBox(430, 80, 300);
        txtSearch->KeyDown += gcnew KeyEventHandler(this, &CustomerForm::onSearchKeyDown);
        Controls->Add(txtSearch);

        Button^ btnSearch = Ui::MakeButton("Search", 740, 77, 105, true);
        btnSearch->Click += gcnew EventHandler(this, &CustomerForm::onSearchClick);
        Controls->Add(btnSearch);

        Button^ btnViewAll = Ui::MakeButton("View Customers", 855, 77, 125, false);
        btnViewAll->Click += gcnew EventHandler(this, &CustomerForm::onViewAllClick);
        Controls->Add(btnViewAll);

        Label^ searchHint = Ui::MakeLabel("Search by customer ID, name or contact number.", 430, 104);
        searchHint->ForeColor = Color::DimGray;
        Controls->Add(searchHint);

        gridCustomers = Ui::MakeGrid(370, 134, 610, 466);
        gridCustomers->CellClick += gcnew DataGridViewCellEventHandler(this, &CustomerForm::onGridCellClick);
        Controls->Add(gridCustomers);

        Shown += gcnew EventHandler(this, &CustomerForm::onFormShown);
    }

    // ------------------------------------------------------------------------
    // Table helpers
    // ------------------------------------------------------------------------
    void CustomerForm::showCustomers(DataTable^ table)
    {
        gridCustomers->DataSource = table;
        if (gridCustomers->Columns->Contains("Customer ID"))
            gridCustomers->Columns["Customer ID"]->FillWeight = 45;
        if (gridCustomers->Columns->Contains("Address"))
            gridCustomers->Columns["Address"]->FillWeight = 160;
        gridCustomers->ClearSelection();
    }

    void CustomerForm::loadAllCustomers()
    {
        try
        {
            DataTable^ table = database->getAllCustomers();
            showCustomers(table);
            Ui::SetStatus(lblStatus, String::Format("{0} customer(s) loaded from the database.", table->Rows->Count), true);
        }
        catch (DatabaseException^ ex)
        {
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    void CustomerForm::clearForm()
    {
        txtCustomerId->Clear();
        txtName->Clear();
        txtPhone->Clear();
        txtAddress->Clear();
        gridCustomers->ClearSelection();
        txtName->Focus();
    }

    bool CustomerForm::readSelectedId(int% customerId)
    {
        return Int32::TryParse(txtCustomerId->Text, customerId) && customerId > 0;
    }

    // ------------------------------------------------------------------------
    // Event handlers
    // ------------------------------------------------------------------------
    void CustomerForm::onFormShown(Object^, EventArgs^)
    {
        loadAllCustomers();
        txtName->Focus();
    }

    // CREATE
    void CustomerForm::onAddClick(Object^, EventArgs^)
    {
        if (txtCustomerId->Text->Length > 0)
        {
            Ui::ShowError("This customer is already saved.\nClick 'Clear Form' to register a new customer, "
                          "or 'Update Customer' to save changes.");
            return;
        }

        try
        {
            // OBJECT: a new Customer object is created from the form values.
            // ENCAPSULATION: the constructor validates every value through the
            // setters and throws std::invalid_argument if anything is wrong.
            Customer customer(toStdString(txtName->Text), toStdString(txtPhone->Text), toStdString(txtAddress->Text));

            int newId = database->addCustomer(customer);
            txtCustomerId->Text = Convert::ToString(newId);
            loadAllCustomers();
            Ui::SetStatus(lblStatus, String::Format("Customer #{0} ({1}) was added successfully.",
                                                    newId, toManagedString(customer.getFullName())), true);
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

    // UPDATE
    void CustomerForm::onUpdateClick(Object^, EventArgs^)
    {
        int customerId = 0;
        if (!readSelectedId(customerId))
        {
            Ui::ShowError("Please select a customer from the table first.");
            return;
        }

        try
        {
            Customer customer(customerId, toStdString(txtName->Text), toStdString(txtPhone->Text), toStdString(txtAddress->Text));
            database->updateCustomer(customer);
            loadAllCustomers();
            Ui::SetStatus(lblStatus, String::Format("Customer #{0} was updated successfully.", customerId), true);
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

    // DELETE (with confirmation)
    void CustomerForm::onDeleteClick(Object^, EventArgs^)
    {
        int customerId = 0;
        if (!readSelectedId(customerId))
        {
            Ui::ShowError("Please select a customer from the table first.");
            return;
        }

        if (!Ui::Confirm(String::Format("Delete customer #{0} ({1})?\nThis cannot be undone.", customerId, txtName->Text)))
            return;

        try
        {
            database->deleteCustomer(customerId);
            clearForm();
            loadAllCustomers();
            Ui::SetStatus(lblStatus, String::Format("Customer #{0} was deleted.", customerId), true);
        }
        catch (DatabaseException^ ex)
        {
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    void CustomerForm::onClearClick(Object^, EventArgs^)
    {
        clearForm();
        lblStatus->Text = String::Empty;
    }

    // SEARCH
    void CustomerForm::onSearchClick(Object^, EventArgs^)
    {
        try
        {
            DataTable^ table = database->searchCustomers(txtSearch->Text);
            showCustomers(table);
            Ui::SetStatus(lblStatus, String::Format("{0} customer(s) found.", table->Rows->Count), table->Rows->Count > 0);
        }
        catch (DatabaseException^ ex)
        {
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
    }

    // READ (all)
    void CustomerForm::onViewAllClick(Object^, EventArgs^)
    {
        txtSearch->Clear();
        loadAllCustomers();
    }

    void CustomerForm::onSearchKeyDown(Object^, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter)
        {
            e->SuppressKeyPress = true;   // stops the "ding" sound
            onSearchClick(nullptr, nullptr);
        }
    }

    // READ (one): clicking a row loads that customer from the database
    // into a Customer object and shows its values in the form.
    void CustomerForm::onGridCellClick(Object^, DataGridViewCellEventArgs^ e)
    {
        if (e->RowIndex < 0)
            return;   // header row clicked

        int customerId = Convert::ToInt32(gridCustomers->Rows[e->RowIndex]->Cells["Customer ID"]->Value);
        try
        {
            Customer customer;   // OBJECT: filled from the database
            if (!database->findCustomerById(customerId, customer))
            {
                Ui::SetStatus(lblStatus, "That customer no longer exists. The table has been refreshed.", false);
                loadAllCustomers();
                return;
            }
            txtCustomerId->Text = Convert::ToString(customer.getCustomerId());
            txtName->Text = toManagedString(customer.getFullName());
            txtPhone->Text = toManagedString(customer.getPhone());
            txtAddress->Text = toManagedString(customer.getAddress());
            Ui::SetStatus(lblStatus, String::Format("Customer #{0} loaded. Edit the details and click 'Update Customer'.", customerId), true);
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
}
