# FreshFold — Laundry & Dry Cleaning Management System

FreshFold is a Windows desktop application for managing a laundry and dry-cleaning shop. Business logic is implemented in modern C++ (C++14) and the GUI and database access use C++/CLI with MySQL Connector/NET. Features include secure PBKDF2 password hashing, customer and order management, invoicing and payment recording. See below for setup and development notes.

**Rajarata University of Sri Lanka · Department of Computing**
**ICT 1306 — Object Oriented Programming · Group Assignment**

---

## Main features

- **Login** with salted PBKDF2 password hashes (no plain-text passwords), and a first-run screen that creates the first administrator account.
- **Dashboard** with live figures from MySQL: total customers, total orders, pending orders and orders ready for pickup.
- **Customers:** add, view, search (by ID, name or phone), update and delete (with confirmation).
- **Orders:** Wash & Fold (charged per kg), Dry Cleaning (per item + handling fee) and Steam Press (per item); automatic bill calculation; update details; status workflow *Pending → In Wash → Ready for Pickup → Completed*; search; cancel.
- **Billing:** formatted invoice, print preview, Cash/Card payments, partial payments, protection against over-payment.

## Technology stack

| Part | Choice |
|---|---|
| Language | C++ (standard C++14 for the business classes, C++/CLI for the GUI and database access) |
| GUI | Windows Forms through C++/CLI, .NET Framework 4.8 |
| IDE / compiler | Visual Studio 2022 (MSVC v143), x64 |
| Database | MySQL 8.0.16 or newer (SQL scripts tested on MySQL 8.0.46) |
| Database connector | MySQL Connector/NET (`MySql.Data.dll`) |
| Password hashing | PBKDF2-HMAC-SHA256, 100,000 iterations (`System.Security.Cryptography.Rfc2898DeriveBytes`) |

### Why Connector/NET instead of Connector/C++?

A Windows Forms program in C++ must be compiled with `/clr` (C++/CLI). MySQL Connector/C++ is a *native* library, and using it inside a `/clr` project causes avoidable problems: some standard headers are blocked under `/clr` (for example `<mutex>` and `<thread>`), the connector's prebuilt Debug/Release runtime libraries must match the project exactly, and every value would have to be converted between `std::string` and `System::String` in both directions. MySQL Connector/NET is MySQL's official driver for .NET, works directly from C++/CLI, and supports parameterized queries and transactions. The program is still written entirely in C++ — the business classes (`Customer`, `LaundryOrder`, `DryCleaningOrder`, …) are standard C++ and compile with any C++ compiler.

---

## Project structure

```
FreshFold/
├── FreshFold.sln                  Visual Studio solution
├── FreshFold.vcxproj              C++/CLI project (.NET Framework 4.8, x64)
├── FreshFold.vcxproj.filters      Folder layout shown in Solution Explorer
├── README.md
├── .gitignore
├── config/
│   └── database.example.ini       Template for config/database.ini (not committed)
├── database/
│   ├── freshfold_db.sql           Creates the database and tables
│   └── sample_data.sql            Optional demonstration data (invented names)
├── docs/
│   ├── class-diagram.md           Mermaid class diagram
│   ├── oop-examples.md            The six OOP concepts in this code
│   └── testing-checklist.md       Manual test cases
├── lib/mysql/                     Put the Connector/NET DLLs here (not committed)
├── screenshots/                   Add your own screenshots here
├── tests/
│   └── ModelTests.cpp             Console tests for the business classes
└── src/
    ├── main.cpp                   Entry point: config → login → dashboard
    ├── Models/                    Standard C++ business classes
    │   ├── PriceList.h            ALL prices in one place
    │   ├── Customer.h/.cpp
    │   ├── LaundryOrder.h/.cpp    Base class (virtual calculateBill)
    │   ├── DryCleaningOrder.h/.cpp  Derived class (overrides calculateBill)
    │   └── OrderFactory.h/.cpp    Creates the right order class
    ├── Database/
    │   ├── DatabaseConfig.h/.cpp  Reads config/database.ini
    │   └── DatabaseManager.h/.cpp All SQL (parameterized) + transactions
    ├── Services/
    │   ├── AuthenticationService.h/.cpp  Login + password hashing
    │   └── BillingService.h/.cpp         Quotes, invoices, payments
    ├── Utils/
    │   └── StringUtils.h          std::string ⇄ System::String (UTF-8)
    └── Forms/                     Windows Forms screens (written in code)
        ├── UiHelpers.h            Shared colours and control styles
        ├── LoginForm.h/.cpp       Screen 1: login + first-run setup
        ├── DashboardForm.h/.cpp   Screen 1: dashboard + navigation
        ├── CustomerForm.h/.cpp    Screen 2: customers
        ├── OrderForm.h/.cpp       Screen 3: orders
        └── BillingForm.h/.cpp     Screen 4: billing and payments
```

The forms are built in code instead of with the Visual Studio Forms Designer. This keeps every screen in one readable `.cpp` file and avoids the designer's limited support for C++/CLI. If you double-click a form header, Visual Studio may try to open a designer — use **right-click → View Code** instead.

---

## Prerequisites (Windows 10 or 11)

1. **Visual Studio 2022** (Community edition is fine). In the Visual Studio Installer select:
   - Workload **Desktop development with C++**
   - In *Individual components*: **C++/CLI support for v143 build tools (Latest)** and **.NET Framework 4.8 targeting pack**
2. **MySQL Server 8.0 or newer** and **MySQL Workbench** (from the MySQL Installer: https://dev.mysql.com/downloads/installer/).
3. **MySQL Connector/NET** (Windows installer): https://dev.mysql.com/downloads/connector/net/

---

## Setup

### 1. Create the database

Open MySQL Workbench, connect as `root`, then **File → Open SQL Script → `database/freshfold_db.sql` → Execute (⚡)**.

Command-line alternative:

```
mysql -u root -p < database/freshfold_db.sql
mysql -u root -p < database/sample_data.sql      (optional demo data)
```

- The script is **safe to re-run**: it uses `CREATE … IF NOT EXISTS` and never deletes data.
- To reset everything, run `DROP DATABASE freshfold_db;` and then run the script again.
- `sample_data.sql` adds three invented customers and three orders. Re-running it does not create duplicates.

**Recommended:** create a separate MySQL account for the application instead of using `root`. At the end of `freshfold_db.sql` there are three commented lines for this — remove the `-- `, choose your own password, and run them once.

### 2. Configure the database connection

Copy `config/database.example.ini` to `config/database.ini` and fill in your values:

```ini
[mysql]
server=localhost
port=3306
database=freshfold_db
user=freshfold_app
password=your-password-here
```

`config/database.ini` is listed in `.gitignore`, so the password is never uploaded to GitHub. The program looks for the file next to `FreshFold.exe` and in a `config` folder in the exe folder or up to four parent folders, so it is found automatically when you run from Visual Studio.

### 3. Add MySQL Connector/NET to the project

1. Install Connector/NET with its Windows installer.
2. Open its installation folder (normally `C:\Program Files (x86)\MySQL\MySQL Connector NET <version>\`), then its **Assemblies** folder, then the folder for **.NET Framework 4.8** (or the closest .NET Framework 4.x folder).
3. Copy **all** `.dll` files from that folder into the project's `lib/mysql/` folder. `MySql.Data.dll` needs the other DLLs (for example Google.Protobuf, BouncyCastle, K4os compression and ZstdSharp) at runtime.

How the project uses them (already configured in `FreshFold.vcxproj`, nothing to click):

| Setting | Value |
|---|---|
| Managed reference | `MySql.Data` with hint path `$(MySqlConnectorDir)MySql.Data.dll`, *Copy Local = true* |
| `MySqlConnectorDir` | `lib\mysql\` inside the project folder (edit the property in the `.vcxproj` if you keep the DLLs elsewhere) |
| Include / library directories, linker inputs | **None needed** — Connector/NET is a .NET assembly, not a native `.lib` |
| Runtime DLLs | A post-build step copies every DLL from `lib\mysql\` next to `FreshFold.exe` |

To check the reference: in Solution Explorer expand **FreshFold → References**; `MySql.Data` must not show a yellow warning icon.

### 4. Build and run

1. Open `FreshFold.sln` in Visual Studio 2022.
2. Select configuration **Debug** and platform **x64** in the toolbar.
3. **Build → Build Solution** (Ctrl+Shift+B).
4. **Debug → Start Debugging** (F5), or run `x64\Debug\FreshFold.exe`.

### 5. Create the first administrator (first run only)

When the `users` table is empty, the login window opens in **First-Time Setup** mode. Enter a username (3–50 letters/digits, `.`, `_` or `-`) and a password (at least 8 characters) twice, then click **Create Administrator**. After that, log in normally.

There is **no built-in default password**, so nothing secret is stored in the source code.

**To change or reset the login** (for example before the demonstration), delete the accounts in MySQL and restart the program; the first-time setup appears again:

```sql
USE freshfold_db;
DELETE FROM users;
```

**How to verify the connection:** start the program. If the login window shows *Staff Login* or *First-Time Setup* without a red error, the program connected to MySQL successfully. The dashboard numbers come straight from the database.

---

## Using the application (five-minute demo)

1. **Login** with the administrator account → dashboard appears with live numbers.
2. **Customers** → add a customer → search for them → edit the address → update.
3. **Orders** → pick the customer → choose *Dry Cleaning* (label changes to *Number of Items*) → enter 3 → **Calculate Bill** (Rs. 1950.00 = 3 × 600 + 150 handling fee) → **Create Order**.
4. **Update Status** → *In Wash* → *Ready for Pickup*.
5. **Bill & Payment** → record Rs. 500 Cash (*Partially Paid*) → record the balance by Card (*Paid*) → **Print Invoice**.
6. Back in Orders → **Update Status** → *Completed*. Return to the dashboard → numbers have changed.

Prices are set in `src/Models/PriceList.h`: Wash & Fold Rs. 350/kg, Steam Press Rs. 120/item, Dry Cleaning Rs. 600/item plus Rs. 150 per order.

### Business rules

- A customer who has orders cannot be deleted (orders are business history).
- Order status moves one step at a time; an order must be fully **Paid** before it can be marked **Completed**.
- Order details (customer, service, quantity, date) can only be edited while the order is **Pending** and **Unpaid**.
- An order can be cancelled only if it is not completed and has no payments. Cancelled orders are kept with status *Cancelled*.
- A payment must be greater than zero and not more than the outstanding balance. The payment row and the order's new payment status are saved in **one transaction**.

---

## OOP concepts demonstrated

| Concept | Where |
|---|---|
| Class | `Customer`, `LaundryOrder` (`src/Models/`) |
| Object | Objects created when registering a customer (`CustomerForm::onAddClick`), when loading from MySQL (`DatabaseManager::findCustomerById`, `findOrderById`) and when creating orders (`createOrder`) |
| Encapsulation | All data members private; setters validate input; no setter for the total |
| Abstraction | Screens call `calculateBill()` / `BillingService::quote()` without knowing the formula; all prices in `PriceList.h` |
| Inheritance | `class DryCleaningOrder : public LaundryOrder` adds a handling fee |
| Polymorphism | `virtual double calculateBill() const` overridden in `DryCleaningOrder`, called through `LaundryOrder&` and `std::unique_ptr<LaundryOrder>` |

Detailed explanations with code: [`docs/oop-examples.md`](docs/oop-examples.md). Class diagram: [`docs/class-diagram.md`](docs/class-diagram.md).

---

## Testing

- **Model tests (no database needed):** `tests/ModelTests.cpp` contains 52 checks for validation, billing, the status workflow and runtime polymorphism. In the *x64 Native Tools Command Prompt for VS 2022*, from the project folder:

  ```
  cl /EHsc /std:c++14 /I src tests\ModelTests.cpp src\Models\Customer.cpp src\Models\LaundryOrder.cpp src\Models\DryCleaningOrder.cpp src\Models\OrderFactory.cpp src\Database\DatabaseConfig.cpp /Fe:ModelTests.exe
  ModelTests.exe
  ```

- **Manual tests:** [`docs/testing-checklist.md`](docs/testing-checklist.md) (33 test cases with an *Actual result* column).

### What has and has not been verified

| Item | Status |
|---|---|
| Business classes + `ModelTests.cpp` | Compiled with g++ 13 and clang (C++14, `-Wall -Wextra`, no warnings) — all 52 checks pass |
| `BillingService` logic | Compiled and tested with g++ using a stand-in for the database class |
| `freshfold_db.sql`, `sample_data.sql` | Executed on MySQL 8.0.46 (twice, to confirm they are safe to re-run) |
| Every SQL statement in `DatabaseManager.cpp` | The exact SQL text was run against MySQL 8.0.46 with parameters (40 checks: CRUD, search, constraints, foreign keys, payment transaction) |
| C++/CLI code (forms, `DatabaseManager`, `AuthenticationService`, `main.cpp`) and the `.vcxproj` | **Not compiled yet** — no Windows/Visual Studio was available. Build it in Visual Studio 2022 and report any compiler errors so they can be fixed. |
| The running GUI, Connector/NET connection, printing | **Not run yet** — use the testing checklist. |

---

## Screenshots

Add screenshots of your own running application to the `screenshots/` folder and link them here, for example:

```markdown
![Login](screenshots/login.png)
![Dashboard](screenshots/dashboard.png)
![Customers](screenshots/customers.png)
![Orders](screenshots/orders.png)
![Billing](screenshots/billing.png)
```

---

## Troubleshooting

| Problem | Solution |
|---|---|
| Build error about `MySql.Data` / `MySql::Data` not found | The DLLs are missing from `lib/mysql/`. See *Setup → step 3*. |
| Build error that C++/CLI or `/clr` is not supported | Install **C++/CLI support for v143 build tools** in the Visual Studio Installer. |
| Build error about .NET Framework 4.8 | Install the **.NET Framework 4.8 targeting pack**. |
| `FileNotFoundException` for `MySql.Data` or another DLL when starting | Copy **all** DLLs from the Connector/NET *.NET Framework 4.8* folder into `lib/mysql/` and rebuild, so they are copied next to the exe. |
| "The database configuration file was not found" | Create `config/database.ini` from `config/database.example.ini`. |
| "Cannot connect to the MySQL server" | Start the MySQL service (`services.msc` → *MySQL80* → Start) and check `server`/`port` in `database.ini`. |
| "MySQL rejected the user name or password" | Check `user`/`password` in `database.ini`, or create the user with the commented lines at the end of `freshfold_db.sql`. |
| "The database named in config\database.ini was not found" | Run `database/freshfold_db.sql`. |
| "Retrieval of the RSA public key is not enabled" | Happens when SSL is turned off on the server. Enable SSL (default in MySQL 8) or change the MySQL user to `mysql_native_password`. |
| Double-clicking a form opens an empty designer | Close it and use right-click → **View Code**. The forms are written in code on purpose. |
| Technical details of an error | Run with F5 and look at the **Output** window — full database errors are written there, not shown to the user. |

---

## Uploading to GitHub

Repository: https://github.com/gamagemandira-ship-it/ICT1306-OOP-Project

If the repository is **empty**:

```
cd FreshFold
git init
git add .
git commit -m "FreshFold laundry management system - initial version"
git branch -M main
git remote add origin https://github.com/gamagemandira-ship-it/ICT1306-OOP-Project.git
git push -u origin main
```

If the repository **already has files** (e.g. a README), do not overwrite them. Clone it first and copy the project into it:

```
git clone https://github.com/gamagemandira-ship-it/ICT1306-OOP-Project.git
(copy the contents of the FreshFold folder into ICT1306-OOP-Project, check any file with the same name before replacing it)
cd ICT1306-OOP-Project
git status
git add .
git commit -m "Add FreshFold laundry management system"
git push
```

Before committing, run `git status` and make sure `config/database.ini`, `lib/mysql/*.dll`, `x64/` and `.vs/` are **not** listed.

---

## Team members

| Name | Index number | Contribution |
|---|---|---|
| _(add name)_ | _(add index)_ | |
| _(add name)_ | _(add index)_ | |
| _(add name)_ | _(add index)_ | |
| _(add name)_ | _(add index)_ | |
