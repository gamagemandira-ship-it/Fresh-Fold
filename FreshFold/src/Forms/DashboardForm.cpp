#include "DashboardForm.h"
#include "CustomerForm.h"
#include "OrderForm.h"
#include "BillingForm.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace FreshFold
{
    DashboardForm::DashboardForm(DatabaseManager* database, BillingService* billing, String^ username)
        : database(database),
          billing(billing),
          username(username),
          logoutRequested(false)
    {
        buildLayout();
    }

    void DashboardForm::buildLayout()
    {
        Ui::StyleForm(this, "Dashboard", 900, 540);

        // ---------- Left navigation menu ----------
        Panel^ menu = gcnew Panel();
        menu->Location = Point(0, 0);
        menu->Size = System::Drawing::Size(200, 540);
        menu->BackColor = Ui::Navy();
        Controls->Add(menu);

        Label^ appName = gcnew Label();
        appName->Text = "FreshFold";
        appName->Font = Ui::MakeFont(18.0f, true);
        appName->ForeColor = Color::White;
        appName->AutoSize = true;
        appName->Location = Point(18, 18);
        menu->Controls->Add(appName);

        Label^ signedIn = gcnew Label();
        signedIn->Text = String::Concat("Signed in as ", username);
        signedIn->ForeColor = Color::FromArgb(200, 215, 235);
        signedIn->AutoSize = false;
        signedIn->Size = System::Drawing::Size(170, 20);
        signedIn->Location = Point(20, 56);
        menu->Controls->Add(signedIn);

        addNavButton(menu, "Dashboard", 110)->Click += gcnew EventHandler(this, &DashboardForm::onRefreshClick);
        addNavButton(menu, "Customers", 154)->Click += gcnew EventHandler(this, &DashboardForm::onCustomersClick);
        addNavButton(menu, "Orders", 198)->Click += gcnew EventHandler(this, &DashboardForm::onOrdersClick);
        addNavButton(menu, "Billing", 242)->Click += gcnew EventHandler(this, &DashboardForm::onBillingClick);
        addNavButton(menu, "Logout", 480)->Click += gcnew EventHandler(this, &DashboardForm::onLogoutClick);

        // ---------- Header ----------
        Panel^ header = Ui::MakeHeader("Dashboard", "Business overview (live figures from the database)");
        header->Dock = DockStyle::None;
        header->Location = Point(200, 0);
        header->Width = 700;
        Controls->Add(header);

        // ---------- Statistic cards ----------
        lblTotalCustomers = addStatCard("Total Customers", 230, 100);
        lblTotalOrders = addStatCard("Total Orders (excluding cancelled)", 570, 100);
        lblPendingOrders = addStatCard("Pending Orders", 230, 230);
        lblReadyOrders = addStatCard("Ready for Pickup", 570, 230);

        lblLastUpdated = gcnew Label();
        lblLastUpdated->AutoSize = true;
        lblLastUpdated->ForeColor = Color::DimGray;
        lblLastUpdated->Location = Point(230, 362);
        Controls->Add(lblLastUpdated);

        lblError = gcnew Label();
        lblError->AutoSize = false;
        lblError->Size = System::Drawing::Size(660, 90);
        lblError->Location = Point(230, 392);
        lblError->ForeColor = Ui::ErrorRed();
        lblError->Visible = false;
        Controls->Add(lblError);

        Shown += gcnew EventHandler(this, &DashboardForm::onFormShown);
    }

    Button^ DashboardForm::addNavButton(Panel^ menu, String^ text, int y)
    {
        Button^ button = gcnew Button();
        button->Text = text;
        button->TextAlign = ContentAlignment::MiddleLeft;
        button->Padding = System::Windows::Forms::Padding(12, 0, 0, 0);
        button->Location = Point(15, y);
        button->Size = System::Drawing::Size(170, 36);
        button->FlatStyle = FlatStyle::Flat;
        button->FlatAppearance->BorderSize = 0;
        button->BackColor = Ui::NavyLight();
        button->ForeColor = Color::White;
        button->Font = Ui::MakeFont(10.5f, false);
        button->Cursor = Cursors::Hand;
        menu->Controls->Add(button);
        return button;
    }

    // Creates a white card and returns the label that shows the number.
    Label^ DashboardForm::addStatCard(String^ title, int x, int y)
    {
        Panel^ card = gcnew Panel();
        card->Location = Point(x, y);
        card->Size = System::Drawing::Size(320, 110);
        card->BackColor = Color::White;
        card->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
        Controls->Add(card);

        Label^ titleLabel = gcnew Label();
        titleLabel->Text = title;
        titleLabel->ForeColor = Color::DimGray;
        titleLabel->AutoSize = true;
        titleLabel->Location = Point(16, 14);
        card->Controls->Add(titleLabel);

        Label^ valueLabel = gcnew Label();
        valueLabel->Text = "--";
        valueLabel->Font = Ui::MakeFont(26.0f, true);
        valueLabel->ForeColor = Ui::Navy();
        valueLabel->AutoSize = true;
        valueLabel->Location = Point(12, 40);
        card->Controls->Add(valueLabel);

        return valueLabel;
    }

    // Loads the four numbers from MySQL. If the database is unavailable the
    // cards show "--" and an error message - never invented numbers.
    void DashboardForm::loadStatistics()
    {
        Cursor = Cursors::WaitCursor;
        try
        {
            DashboardStats stats = database->getDashboardStats();
            lblTotalCustomers->Text = Convert::ToString(stats.totalCustomers);
            lblTotalOrders->Text = Convert::ToString(stats.totalOrders);
            lblPendingOrders->Text = Convert::ToString(stats.pendingOrders);
            lblReadyOrders->Text = Convert::ToString(stats.readyOrders);
            lblLastUpdated->Text = String::Concat("Last updated: ", DateTime::Now.ToString("yyyy-MM-dd HH:mm:ss"));
            lblError->Visible = false;
        }
        catch (DatabaseException^ ex)
        {
            lblTotalCustomers->Text = "--";
            lblTotalOrders->Text = "--";
            lblPendingOrders->Text = "--";
            lblReadyOrders->Text = "--";
            lblLastUpdated->Text = String::Empty;
            lblError->Text = String::Concat("The statistics could not be loaded.\n", ex->Message);
            lblError->Visible = true;
        }
        Cursor = Cursors::Default;
    }

    void DashboardForm::onFormShown(Object^, EventArgs^)
    {
        loadStatistics();
    }

    void DashboardForm::onRefreshClick(Object^, EventArgs^)
    {
        loadStatistics();
    }

    // Each screen opens as a dialog; when it closes, the statistics are refreshed.
    void DashboardForm::onCustomersClick(Object^, EventArgs^)
    {
        CustomerForm^ screen = gcnew CustomerForm(database);
        screen->ShowDialog(this);
        delete screen;   // releases the window's resources (calls Dispose)
        loadStatistics();
    }

    void DashboardForm::onOrdersClick(Object^, EventArgs^)
    {
        OrderForm^ screen = gcnew OrderForm(database, billing);
        screen->ShowDialog(this);
        delete screen;   // releases the window's resources (calls Dispose)
        loadStatistics();
    }

    void DashboardForm::onBillingClick(Object^, EventArgs^)
    {
        BillingForm^ screen = gcnew BillingForm(database, billing, 0);
        screen->ShowDialog(this);
        delete screen;   // releases the window's resources (calls Dispose)
        loadStatistics();
    }

    void DashboardForm::onLogoutClick(Object^, EventArgs^)
    {
        if (Ui::Confirm("Do you want to log out?"))
        {
            logoutRequested = true;
            Close();
        }
    }
}
