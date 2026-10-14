Instructor setup — FreshFold

This document explains how to set up, build and run FreshFold on a Windows machine (Visual Studio 2022, MySQL 8.x). It also explains how to create an uploadable zip of the project for submission or distribution.

Prerequisites
- Visual Studio 2022 with "Desktop development with C++" and the "C++/CLI support for v143" component.
- .NET Framework 4.8 targeting pack (Visual Studio installer).
- MySQL Server 8.0 or newer and MySQL Workbench (or mysql client).
- MySQL Connector/NET (installed) — you will need the DLLs from the Assemblies -> .NET Framework 4.x folder.

1) Prepare the project files
- Ensure the repository contains these files/folders (they are included in the archive):
  - README.md, LICENSE, INSTRUCTOR_SETUP.md
  - src/ (all code files)
  - database/freshfold_db.sql and database/sample_data.sql
  - config/database.example.ini (this is the example; do NOT commit config/database.ini)
  - create_release.bat (creates a zip of tracked files)

2) Create the runtime settings (local only)
- Before running the program a local config file must be created at runtime location:
  Copy `config/database.example.ini` to `config/database.ini` and edit the values to match the MySQL server and account you will use. Example:

[mysql]
server=localhost
port=3306
database=freshfold_db
user=freshfold_app
password=YourStrongPassword

- Do NOT commit this file; it contains secrets.

3) Create the database and schema
- Open MySQL Workbench or the mysql CLI as an administrative user (root) and run the schema script:
  mysql -u root -p < database/freshfold_db.sql
- (Optional) Import the sample data:
  mysql -u root -p freshfold_db < database/sample_data.sql

4) Create a dedicated MySQL user for the app (recommended)
- From the mysql prompt (as root) run (replace YourStrongPassword):
  CREATE USER IF NOT EXISTS 'freshfold_app'@'localhost' IDENTIFIED BY 'YourStrongPassword';
  GRANT SELECT, INSERT, UPDATE, DELETE ON `freshfold_db`.* TO 'freshfold_app'@'localhost';
  FLUSH PRIVILEGES;

5) Copy Connector/NET DLLs
- Install MySQL Connector/NET and copy the required DLLs from the Connector/NET installation -> Assemblies -> .NET Framework 4.x folder into the project folder `lib/mysql/` so they are packaged next to the exe on build. (If omitted, the app will show missing-assembly errors.)

6) Build the project
- Open `FreshFold.sln` in Visual Studio 2022.
- Select configuration `Debug` and platform `x64`.
- Build -> Rebuild Solution.

7) Run the program
- Start Debugging (F5) or run `x64\Debug\FreshFold.exe`.
- On first run, if the `users` table is empty the app shows the first-run screen to create an administrator.

Creating an uploadable zip (for submission)
- Recommended: use the tracked files so local secrets and build outputs are not included.
- If you use git and have committed your intended files, run from the project root:
  git archive --format=zip -o ..\FreshFold-release.zip HEAD
  This creates `FreshFold-release.zip` in the parent folder.

- Alternatively run the included script (PowerShell / double-click):
  create_release.bat
  It runs `git archive` and writes `..\FreshFold-release.zip`.

Notes for instructors who receive the zip
- Unzip into a folder. Create `config/database.ini` from the example and set credentials.
- Import `database/freshfold_db.sql` into MySQL and optionally `sample_data.sql`.
- Install Connector/NET or copy the `lib/mysql` DLLs into the project before building.
- Build in Visual Studio (x64, Debug) and run.

Troubleshooting
- If the program shows "MySQL rejected the user name or password": check `config/database.ini` and the MySQL user password or grants.
- If the program shows "The database named in config\database.ini was not found": ensure the schema `freshfold_db` exists and `database` value matches.
- If a DLL is missing at runtime, copy all assemblies from the Connector/NET folder into `lib/mysql/` and rebuild.

If you want, I can produce a minimal checklist file to include in the zip; tell me and I will add it to the repository.
