# FreshFold – Manual Testing Checklist

Fill in **Actual result** and **Pass/Fail** while testing the built application on Windows. Nothing below has been marked as passed in advance.

**Before testing:** run `database/freshfold_db.sql`, optionally `database/sample_data.sql`, create `config/database.ini`, build and start the application. Prices used in the expected results come from `src/Models/PriceList.h`: Wash & Fold Rs. 350/kg, Steam Press Rs. 120/item, Dry Cleaning Rs. 600/item + Rs. 150 handling fee.

Tester: ____________________   Date: ____________   Build (Debug/Release): ________

| # | Test scenario | Steps | Expected result | Actual result | Pass/Fail |
|---|---|---|---|---|---|
| 1 | First-run administrator setup | Start the app with an empty `users` table. Enter username `admin`, a password of 8+ characters twice, click **Create Administrator**. | Message "Administrator account created". Screen switches to normal login. `users` table has one row whose `password_hash` starts with `PBKDF2-SHA256$` (not the password). | | |
| 2 | Successful login | Enter the correct username and password, click **Login**. | "Login successful. Welcome, admin!" message, then the Dashboard opens showing "Signed in as admin". | | |
| 3 | Incorrect login | Enter a wrong password (and, separately, an unknown username). | "Incorrect username or password. (Attempt n of 5)". Dashboard does not open. After 5 failures the button is disabled for 30 seconds. | | |
| 4 | Dashboard statistics | Log in. Compare the four cards with `SELECT COUNT(*)` queries in MySQL Workbench. | Total customers, total orders (not cancelled), pending orders and ready-for-pickup orders match the database. | | |
| 5 | Adding a customer | Customers → enter name, contact number, address → **Add Customer**. | Green message "Customer #N … was added successfully". New row appears in the table and in MySQL `customers`. | | |
| 6 | Duplicate customer | Add another customer with the same contact number as test 5. | Message "A customer with this contact number is already registered." No new row. | | |
| 7 | Viewing customers | Click **View Customers**. | All customers from MySQL are listed, newest first. | | |
| 8 | Searching for a customer | Search by an ID, then part of a name, then part of a phone number. | Only matching customers are listed; count message shown. | | |
| 9 | Updating a customer | Click a row, change the address, click **Update Customer**. | "Customer #N was updated successfully." The table and MySQL show the new address. | | |
| 10 | Deleting a customer | Select a customer with **no** orders, click **Delete Customer**, confirm **Yes**. | Confirmation dialog appears; after Yes the row disappears from the table and MySQL. | | |
| 11 | Deleting a customer who has orders | Select a customer who has an order, click **Delete Customer**, confirm. | Message "This customer has orders, so the record cannot be deleted." Record remains. | | |
| 12 | Creating a Wash & Fold order | Orders → select customer → **Wash & Fold** → quantity label shows **Weight (kg)** → enter `2.5` → **Create Order**. | "Order #N was created. Total: Rs. 875.00". Row in table and in MySQL `orders` (quantity 2.50, unit_price 350.00, service_fee 0.00, total 875.00, Pending, Unpaid). | | |
| 13 | Creating a Dry Cleaning order | Select **Dry Cleaning** (label **Number of Items**), quantity `3`, **Create Order**. | Total Rs. 1950.00; MySQL row has unit_price 600.00, service_fee 150.00, total 1950.00. | | |
| 14 | Creating a Steam Press order | Select **Steam Press**, quantity `4`, **Create Order**. | Total Rs. 480.00; service_fee 0.00. | | |
| 15 | Correct bill calculation | Without saving, use **Calculate Bill** for 1.75 kg Wash & Fold and for 2 items Dry Cleaning. | Breakdown shows "1.75 kg x Rs. 350.00 per kg = Rs. 612.50" and "2 items x Rs. 600.00 … + Handling fee Rs. 150.00 = Rs. 1350.00". | | |
| 16 | Runtime polymorphism | Calculate a bill for 3 items Steam Press, then 3 items Dry Cleaning. Also build and run `tests/ModelTests.cpp`. | Rs. 360.00 vs Rs. 1950.00 from the same `calculateBill()` call; the test program prints the "Runtime polymorphism" checks as PASS. | | |
| 17 | Updating order details | Select a Pending, unpaid order, change the quantity, click **Update Order**. Then try the same on an order that is In Wash. | First: "Order #N was updated. New total …". Second: message that only Pending and Unpaid orders can be edited. | | |
| 18 | Updating order status | Select a Pending order; the status box suggests **In Wash**; click **Update Status**. Repeat until **Ready for Pickup**. | Each step succeeds and the table updates. Trying to skip a step (e.g. Pending → Ready for Pickup) shows "Status can only move one step forward…". | | |
| 19 | Completed requires payment | Try **Completed** on an unpaid order that is Ready for Pickup. | Message "The order must be fully paid before it can be marked as Completed." | | |
| 20 | Searching orders | Search by order ID, customer name, `Pending`, `Dry`. | Only matching orders are shown. | | |
| 21 | Cancelling an order | Select an unpaid, not completed order → **Cancel Order** → Yes. Then try to cancel a paid order. | First: status becomes "Cancelled" (row kept). Second: message that an order with a payment cannot be cancelled. | | |
| 22 | Viewing the bill | Billing → enter an order ID → **Load Order**. | Customer, service, quantity, total, paid, balance and payment status shown; invoice text matches the order. | | |
| 23 | Recording a payment (partial, then full) | For a Rs. 1950.00 order, record Rs. 500.00 Cash, then the suggested balance Rs. 1450.00 by Card. | After the first payment: "Partially Paid", balance Rs. 1450.00. After the second: "Paid", balance Rs. 0.00. Two rows in `payments`; Record Payment button disabled. | | |
| 24 | Preventing excessive payments | Enter more than the balance (e.g. balance + 1). | Message "The payment amount cannot be more than the outstanding balance (Rs. …)". No row added to `payments`. | | |
| 25 | Printing the invoice | Click **Print Invoice**. | Print preview shows the formatted invoice; it can be printed or saved with "Microsoft Print to PDF". | | |
| 26 | Disconnected MySQL server | Stop the MySQL service (services.msc → MySQL80 → Stop), then (a) start the app, (b) click Dashboard/Refresh while logged in, (c) try to add a customer. | (a) Login screen shows "Cannot connect to the MySQL server…" and a **Retry Connection** button. (b) Cards show "--" with an error message – no invented numbers. (c) Error message; nothing crashes. No password is shown in any message. | | |
| 27 | Payment while database fails | Stop MySQL after loading an order in Billing, then click **Record Payment**. | Error "The payment could not be saved. Nothing was recorded." After restarting MySQL the order is still unpaid. | | |
| 28 | Invalid input – customers | Empty name / empty phone / phone `abc` / phone `12345` / empty address. | Each is rejected with a clear message; nothing saved. | | |
| 29 | Invalid input – orders | No customer selected; no service; quantity `0`, `-2`, `abc`; `2.5` items for Dry Cleaning. | Each is rejected with a clear message ("Please select a customer.", "Quantity must be greater than zero.", "Number of items must be a whole number…"). | | |
| 30 | Invalid input – payments | Amount `0`, `-10`, `abc`, `10.555`. | Rejected with a clear message; nothing saved. | | |
| 31 | SQL injection attempt | Search customers for `' OR '1'='1` and add a customer named `Robert'); DROP TABLE customers;--`. | Search returns no unexpected rows; the name is stored literally; the `customers` table still exists. | | |
| 32 | Logout | Click **Logout** → Yes. | Dashboard closes and the login screen appears again. | | |
| 33 | Missing configuration file | Rename `config/database.ini` and start the app. | Startup message explaining that the configuration file was not found and how to create it. | | |

## Automated model tests (no database needed)

`tests/ModelTests.cpp` checks validation, billing, the status workflow, runtime polymorphism and the configuration parser (52 checks). Build and run it as described at the top of that file and record the result:

Result: ______ passed, ______ failed    Date: ____________
