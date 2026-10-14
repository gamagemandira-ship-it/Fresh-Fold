#include "LoginForm.h"
#include "../Utils/StringUtils.h"

#include <stdexcept>

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace FreshFold
{
    static const int MAX_FAILED_ATTEMPTS = 5;
    static const int LOCK_SECONDS = 30;

    LoginForm::LoginForm(AuthenticationService* authentication)
        : authentication(authentication),
          loggedInUser(nullptr),
          setupMode(false),
          databaseReady(false),
          failedAttempts(0)
    {
        buildLayout();
    }

    void LoginForm::buildLayout()
    {
        Ui::StyleForm(this, "Login", 400, 420);
        Controls->Add(Ui::MakeHeader("FreshFold", "Laundry & Dry Cleaning Management System"));

        lblTitle = gcnew Label();
        lblTitle->Font = Ui::MakeFont(13.0f, true);
        lblTitle->ForeColor = Ui::Navy();
        lblTitle->AutoSize = true;
        lblTitle->Location = Point(40, 78);
        Controls->Add(lblTitle);

        lblHint = gcnew Label();
        lblHint->AutoSize = false;
        lblHint->Size = System::Drawing::Size(320, 20);
        lblHint->Location = Point(40, 104);
        lblHint->ForeColor = Color::DimGray;
        Controls->Add(lblHint);

        Controls->Add(Ui::MakeLabel("Username", 40, 128));
        txtUsername = Ui::MakeTextBox(40, 150, 320);
        txtUsername->MaxLength = 50;
        Controls->Add(txtUsername);

        Controls->Add(Ui::MakeLabel("Password", 40, 182));
        txtPassword = Ui::MakeTextBox(40, 204, 320);
        txtPassword->UseSystemPasswordChar = true;   // shows dots instead of the password
        txtPassword->MaxLength = 128;
        Controls->Add(txtPassword);

        lblConfirm = Ui::MakeLabel("Confirm password", 40, 236);
        Controls->Add(lblConfirm);
        txtConfirm = Ui::MakeTextBox(40, 258, 320);
        txtConfirm->UseSystemPasswordChar = true;
        txtConfirm->MaxLength = 128;
        Controls->Add(txtConfirm);

        btnLogin = Ui::MakeButton("Login", 40, 244, 320, true);
        btnLogin->Click += gcnew EventHandler(this, &LoginForm::onLoginClick);
        Controls->Add(btnLogin);
        AcceptButton = btnLogin;   // pressing Enter clicks the button

        lblStatus = gcnew Label();
        lblStatus->AutoSize = false;
        lblStatus->Size = System::Drawing::Size(320, 64);
        lblStatus->Location = Point(40, 284);
        Controls->Add(lblStatus);

        lockTimer = gcnew System::Windows::Forms::Timer();
        lockTimer->Interval = LOCK_SECONDS * 1000;
        lockTimer->Tick += gcnew EventHandler(this, &LoginForm::onLockTimerTick);

        // Check the database once the window is visible, so error messages
        // appear on top of the login window.
        Shown += gcnew EventHandler(this, &LoginForm::onFormShown);

        applyMode();
    }

    void LoginForm::onFormShown(Object^, EventArgs^)
    {
        checkDatabase();
    }

    // Connects to MySQL and decides between login mode and first-run setup mode.
    void LoginForm::checkDatabase()
    {
        Cursor = Cursors::WaitCursor;
        try
        {
            setupMode = authentication->isFirstRun();
            databaseReady = true;
            applyMode();
            lblStatus->Text = String::Empty;
            txtUsername->Focus();
        }
        catch (DatabaseException^ ex)
        {
            databaseReady = false;
            applyMode();
            Ui::SetStatus(lblStatus, ex->Message, false);
        }
        Cursor = Cursors::Default;
    }

    void LoginForm::applyMode()
    {
        lblConfirm->Visible = setupMode;
        txtConfirm->Visible = setupMode;
        btnLogin->Top = setupMode ? 298 : 244;
        lblStatus->Top = btnLogin->Bottom + 8;

        if (!databaseReady)
        {
            lblTitle->Text = "Staff Login";
            lblHint->Text = "The database is not available.";
            btnLogin->Text = "Retry Connection";
        }
        else if (setupMode)
        {
            lblTitle->Text = "First-Time Setup";
            lblHint->Text = "Create the administrator account (one time only).";
            btnLogin->Text = "Create Administrator";
        }
        else
        {
            lblTitle->Text = "Staff Login";
            lblHint->Text = "Please log in to continue.";
            btnLogin->Text = "Login";
        }
    }

    void LoginForm::onLoginClick(Object^, EventArgs^)
    {
        if (!databaseReady)
            checkDatabase();
        else if (setupMode)
            tryCreateAdministrator();
        else
            tryLogin();
    }

    void LoginForm::tryLogin()
    {
        String^ username = txtUsername->Text->Trim();
        if (username->Length == 0 || txtPassword->Text->Length == 0)
        {
            Ui::SetStatus(lblStatus, "Please enter both your username and password.", false);
            return;
        }

        bool success = false;
        Cursor = Cursors::WaitCursor;
        try
        {
            success = authentication->login(username, txtPassword->Text);
        }
        catch (DatabaseException^ ex)
        {
            Cursor = Cursors::Default;
            Ui::SetStatus(lblStatus, ex->Message, false);
            return;
        }
        Cursor = Cursors::Default;

        if (success)
        {
            failedAttempts = 0;
            loggedInUser = username;
            Ui::SetStatus(lblStatus, "Login successful.", true);
            Ui::ShowInfo(String::Format("Login successful. Welcome, {0}!", username));
            DialogResult = System::Windows::Forms::DialogResult::OK;
            Close();
            return;
        }

        // Same message for an unknown user and a wrong password, so the
        // screen does not reveal which usernames exist.
        failedAttempts++;
        txtPassword->Clear();
        txtPassword->Focus();
        if (failedAttempts >= MAX_FAILED_ATTEMPTS)
        {
            btnLogin->Enabled = false;
            lockTimer->Start();
            Ui::SetStatus(lblStatus, String::Format(
                "Too many incorrect attempts. Please wait {0} seconds and try again.", LOCK_SECONDS), false);
        }
        else
        {
            Ui::SetStatus(lblStatus, String::Format(
                "Incorrect username or password. (Attempt {0} of {1})", failedAttempts, MAX_FAILED_ATTEMPTS), false);
        }
    }

    void LoginForm::onLockTimerTick(Object^, EventArgs^)
    {
        lockTimer->Stop();
        failedAttempts = 0;
        btnLogin->Enabled = true;
        Ui::SetStatus(lblStatus, "You can try to log in again.", true);
    }

    void LoginForm::tryCreateAdministrator()
    {
        Cursor = Cursors::WaitCursor;
        try
        {
            authentication->createInitialAdmin(txtUsername->Text, txtPassword->Text, txtConfirm->Text);
        }
        catch (const std::exception& ex)
        {
            Cursor = Cursors::Default;
            Ui::SetStatus(lblStatus, toManagedString(ex.what()), false);
            return;
        }
        catch (DatabaseException^ ex)
        {
            Cursor = Cursors::Default;
            Ui::SetStatus(lblStatus, ex->Message, false);
            return;
        }
        Cursor = Cursors::Default;

        Ui::ShowInfo("Administrator account created.\nPlease log in with your new username and password.");
        setupMode = false;
        txtPassword->Clear();
        txtConfirm->Clear();
        applyMode();
        Ui::SetStatus(lblStatus, "Account created. You can log in now.", true);
        txtPassword->Focus();
    }
}
