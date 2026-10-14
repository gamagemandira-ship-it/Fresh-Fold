FreshFold — Laundry & Dry Cleaning management 

•	Requirements: Visual Studio 2022 (C++/C++-CLI), .NET 4.8, MySQL 8.x, MySQL Connector/NET.
•	Install MySQL, create schema: mysql -u root -p < database/freshfold_db.sql (optional sample data: database/sample_data.sql).
•	Copy config/database.example.ini → config/database.ini and set credentials (do NOT commit this file).
•	Open FreshFold.sln (x64, Debug), Rebuild and run (F5) or execute x64\Debug\FreshFold.exe.
