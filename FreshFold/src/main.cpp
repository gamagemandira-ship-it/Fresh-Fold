// ============================================================================
// main.cpp  -  Program entry point
//
// 1. Finds and reads config\database.ini (MySQL connection settings).
// 2. Creates ONE DatabaseManager, AuthenticationService and BillingService.
//    These objects live for the whole program and are passed to the screens.
// 3. Shows the login screen, then the dashboard. "Logout" returns to login.
// ============================================================================

#include "Database/DatabaseConfig.h"
#include "Database/DatabaseManager.h"
#include "Services/AuthenticationService.h"
#include "Services/BillingService.h"
#include "Forms/LoginForm.h"
#include "Forms/DashboardForm.h"
#include "Utils/StringUtils.h"

#include <stdexcept>

using namespace System;
using namespace System::IO;
using namespace System::Windows::Forms;
using namespace FreshFold;

// Looks for the configuration file next to the .exe, then in a "config"
// folder in the .exe folder or any of the 4 parent folders. This finds
// <project>\config\database.ini when the program is started from Visual Studio
// (the .exe is in <project>\x64\Debug\).
static String^ findConfigFile()
{
    String^ folder = AppDomain::CurrentDomain->BaseDirectory;
    for (int level = 0; level < 5 && folder != nullptr; level++)
    {
        String^ besideExe = Path::Combine(folder, "database.ini");
        if (File::Exists(besideExe))
            return besideExe;

        String^ inConfigFolder = Path::Combine(folder, "config", "database.ini");
        if (File::Exists(inConfigFolder))
            return inConfigFolder;

        DirectoryInfo^ parent = Directory::GetParent(folder->TrimEnd(gcnew array<wchar_t>{ L'\\', L'/' }));
        if (parent == nullptr)
            break;
        folder = parent->FullName;
    }
    return nullptr;
}

// Search for the example config file (database.example.ini) in the same
// locations as findConfigFile(). Returns the path if found, otherwise null.
static String^ findExampleFile()
{
    String^ folder = AppDomain::CurrentDomain->BaseDirectory;
    for (int level = 0; level < 5 && folder != nullptr; level++)
    {
        String^ besideExe = Path::Combine(folder, "database.example.ini");
        if (File::Exists(besideExe))
            return besideExe;

        String^ inConfigFolder = Path::Combine(folder, "config", "database.example.ini");
        if (File::Exists(inConfigFolder))
            return inConfigFolder;

        DirectoryInfo^ parent = Directory::GetParent(folder->TrimEnd(gcnew array<wchar_t>{ L'\\', L'/' }));
        if (parent == nullptr)
            break;
        folder = parent->FullName;
    }
    return nullptr;
}

static void showStartupError(String^ message)
{
    MessageBox::Show(message, "FreshFold - startup problem", MessageBoxButtons::OK, MessageBoxIcon::Error);
}

[STAThreadAttribute]
int main(array<String^>^ args)
{
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);

    // ---------- 1. Read the database settings ----------
    String^ configPath = findConfigFile();
    if (configPath == nullptr)
    {
        // If the real config is missing, try to locate an example INI in the
        // repository and copy it to the runtime config location so the app can
        // start and the user can edit credentials. This helps when the file
        // was not copied to the output directory from the project.
        String^ example = findExampleFile();
        if (example != nullptr)
        {
            try
            {
                String^ targetDir = Path::Combine(AppDomain::CurrentDomain->BaseDirectory, "config");
                if (!Directory::Exists(targetDir))
                    Directory::CreateDirectory(targetDir);

                String^ target = Path::Combine(targetDir, "database.ini");
                // Only create if not already present
                if (!File::Exists(target))
                    File::Copy(example, target);

                configPath = target;
            }
            catch (Exception^ ex)
            {
                showStartupError(String::Concat("Could not create config\\database.ini: \n\n", ex->Message));
                return 1;
            }
        }
        else
        {
            showStartupError(
                "The database configuration file was not found.\n\n"
                "Copy config\\database.example.ini to config\\database.ini and enter your MySQL "
                "server, user name and password. See README.md, section 'Configure the database connection'.");
            return 1;
        }
    }

    DatabaseConfig config;
    try
    {
        config = DatabaseConfig::parse(toStdString(File::ReadAllText(configPath)));
    }
    catch (const std::exception& ex)
    {
        showStartupError(String::Concat("There is a problem with ", configPath, ":\n\n", toManagedString(ex.what())));
        return 1;
    }
    catch (Exception^ ex)
    {
        showStartupError(String::Concat("Could not read ", configPath, ":\n\n", ex->Message));
        return 1;
    }

    // Fast validation: reject obvious example-placeholder values so the user
    // edits config before we attempt to connect.
    if (config.user == "freshfold_app" && config.password == "CHANGE_ME")
    {
        showStartupError(
            String::Concat("Please edit ", configPath, " and replace the example database credentials.\n\n",
                           "The file still contains the example values from database.example.ini."));
        return 1;
    }

    // ---------- 2. Create the shared objects ----------
    DatabaseManager database(config);

    // Immediately test the connection and show a friendly startup error if it fails.
    try
    {
        database.testConnection();
    }
    catch (DatabaseException^ dbex)  // friendly message already formatted in toDatabaseException
    {
        showStartupError(String::Concat("Could not connect to the database:\n\n", dbex->Message));
        return 1;
    }
    catch (Exception^ ex) // fallback
    {
        showStartupError(String::Concat("Could not connect to the database:\n\n", ex->Message));
        return 1;
    }

    AuthenticationService authentication(database);
    BillingService billing(database);

    // If this is the first run (no users), create a default administrator
    // account so the program is immediately usable. The account is created
    // only when the users table is empty.
    try
    {
        if (authentication.isFirstRun())
        {
            // Default credentials (change after first login):
            //   username: admin
            //   password: Admin@1234
            authentication.createInitialAdmin("admin", "Admin@1234", "Admin@1234");
        }
    }
    catch (const std::exception&)
    {
        // Ignore: if creation failed because another process added a user,
        // proceed to normal login flow; errors will be shown in the GUI.
    }

    // ---------- 3. Login -> Dashboard -> (Logout) -> Login ... ----------
    while (true)
    {
        LoginForm^ login = gcnew LoginForm(&authentication);
        if (login->ShowDialog() != DialogResult::OK)
            break;   // login window closed: exit the program

        DashboardForm^ dashboard = gcnew DashboardForm(&database, &billing, login->LoggedInUser);
        dashboard->ShowDialog();
        bool logout = dashboard->LogoutRequested;

        delete dashboard;
        delete login;

        if (!logout)
            break;   // dashboard closed with the X button: exit the program
    }

    return 0;
}
