#pragma once

// ============================================================================
// LoginForm.h  -  SCREEN 1 (part 1): Login and first-run administrator setup
//
// Normal mode : username + password -> AuthenticationService::login()
// Setup mode  : shown automatically when the users table is empty, so the
//               application is never unusable because no account exists.
// ============================================================================

#include "UiHelpers.h"
#include "../Services/AuthenticationService.h"

namespace FreshFold
{
    public ref class LoginForm : public System::Windows::Forms::Form
    {
    public:
        LoginForm(AuthenticationService* authentication);

        // Username of the person who logged in (valid after DialogResult::OK).
        property System::String^ LoggedInUser
        {
            System::String^ get() { return loggedInUser; }
        }

    private:
        AuthenticationService* authentication;   // owned by main(), not by this form
        System::String^ loggedInUser;
        bool setupMode;
        bool databaseReady;
        int failedAttempts;

        System::Windows::Forms::Label^ lblTitle;
        System::Windows::Forms::Label^ lblHint;
        System::Windows::Forms::TextBox^ txtUsername;
        System::Windows::Forms::TextBox^ txtPassword;
        System::Windows::Forms::Label^ lblConfirm;
        System::Windows::Forms::TextBox^ txtConfirm;
        System::Windows::Forms::Button^ btnLogin;
        System::Windows::Forms::Label^ lblStatus;
        System::Windows::Forms::Timer^ lockTimer;

        void buildLayout();
        void checkDatabase();
        void applyMode();
        void tryLogin();
        void tryCreateAdministrator();

        void onFormShown(System::Object^ sender, System::EventArgs^ e);
        void onLoginClick(System::Object^ sender, System::EventArgs^ e);
        void onLockTimerTick(System::Object^ sender, System::EventArgs^ e);
    };
}
