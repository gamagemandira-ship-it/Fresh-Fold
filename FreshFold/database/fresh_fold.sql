-- ============================================================================
-- FreshFold - Laundry & Dry Cleaning Management System
-- Database setup script for MySQL 8.0.16 or newer (CHECK constraints are
-- enforced from 8.0.16 onwards).
--
-- HOW TO RUN (MySQL Workbench): File > Open SQL Script > this file > Execute (lightning icon)
-- HOW TO RUN (command line):   mysql -u root -p < database/fresh_fold.sql
--
-- SAFE TO RE-RUN: this script only creates the database and tables if they
-- do not exist yet. Re-running it does NOT delete or reset existing data.
-- To start again from an empty database, run this first (deletes everything!):
--     DROP DATABASE freshfold_db;
--
-- No login account is created here. The application asks you to create the
-- first administrator account the first time it starts (see README).
-- ============================================================================

CREATE DATABASE IF NOT EXISTS freshfold_db
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_0900_ai_ci;

USE freshfold_db;

-- ---------------------------------------------------------------------------
-- Staff login accounts. Only a PBKDF2 password hash is stored, never the
-- password itself.
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS users (
    user_id        INT           NOT NULL AUTO_INCREMENT,
    username       VARCHAR(50)   NOT NULL,
    password_hash  VARCHAR(255)  NOT NULL,
    created_at     TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id),
    UNIQUE KEY uq_users_username (username)
) ENGINE = InnoDB;

-- ---------------------------------------------------------------------------
-- Customers. The phone number is unique so the same person is not
-- registered twice.
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS customers (
    customer_id  INT           NOT NULL AUTO_INCREMENT,
    full_name    VARCHAR(100)  NOT NULL,
    phone        VARCHAR(20)   NOT NULL,
    address      VARCHAR(255)  NOT NULL,
    created_at   TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (customer_id),
    UNIQUE KEY uq_customers_phone (phone),
    CONSTRAINT chk_customers_name    CHECK (CHAR_LENGTH(TRIM(full_name)) > 0),
    CONSTRAINT chk_customers_phone   CHECK (CHAR_LENGTH(TRIM(phone)) > 0),
    CONSTRAINT chk_customers_address CHECK (CHAR_LENGTH(TRIM(address)) > 0)
) ENGINE = InnoDB;

-- ---------------------------------------------------------------------------
-- Orders.
--   quantity     : kilograms for Wash & Fold, number of items otherwise
--   unit_price   : price per kg / per item when the order was created
--   service_fee  : extra fixed fee (Dry Cleaning handling fee, 0 otherwise)
--   total_amount : quantity x unit_price + service_fee (calculated by the
--                  C++ classes and saved here)
-- A customer that has orders cannot be deleted (ON DELETE RESTRICT).
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS orders (
    order_id        INT            NOT NULL AUTO_INCREMENT,
    customer_id     INT            NOT NULL,
    service_type    VARCHAR(20)    NOT NULL,
    quantity        DECIMAL(8,2)   NOT NULL,
    unit_price      DECIMAL(10,2)  NOT NULL,
    service_fee     DECIMAL(10,2)  NOT NULL DEFAULT 0.00,
    total_amount    DECIMAL(10,2)  NOT NULL,
    status          VARCHAR(20)    NOT NULL DEFAULT 'Pending',
    payment_status  VARCHAR(20)    NOT NULL DEFAULT 'Unpaid',
    order_date      DATE           NOT NULL,
    created_at      TIMESTAMP      NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (order_id),
    KEY idx_orders_customer (customer_id),
    KEY idx_orders_status (status),
    CONSTRAINT fk_orders_customer FOREIGN KEY (customer_id)
        REFERENCES customers (customer_id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,
    CONSTRAINT chk_orders_service CHECK (service_type IN ('Wash & Fold', 'Dry Cleaning', 'Steam Press')),
    CONSTRAINT chk_orders_quantity CHECK (quantity > 0),
    CONSTRAINT chk_orders_unit_price CHECK (unit_price >= 0),
    CONSTRAINT chk_orders_service_fee CHECK (service_fee >= 0),
    CONSTRAINT chk_orders_total CHECK (total_amount >= 0),
    CONSTRAINT chk_orders_status CHECK (status IN ('Pending', 'In Wash', 'Ready for Pickup', 'Completed', 'Cancelled')),
    CONSTRAINT chk_orders_payment_status CHECK (payment_status IN ('Unpaid', 'Partially Paid', 'Paid'))
) ENGINE = InnoDB;

-- ---------------------------------------------------------------------------
-- Payments. One order can have several payments (partial payments).
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS payments (
    payment_id      INT            NOT NULL AUTO_INCREMENT,
    order_id        INT            NOT NULL,
    amount          DECIMAL(10,2)  NOT NULL,
    payment_method  VARCHAR(10)    NOT NULL,
    payment_date    DATETIME       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (payment_id),
    KEY idx_payments_order (order_id),
    CONSTRAINT fk_payments_order FOREIGN KEY (order_id)
        REFERENCES orders (order_id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,
    CONSTRAINT chk_payments_amount CHECK (amount > 0),
    CONSTRAINT chk_payments_method CHECK (payment_method IN ('Cash', 'Card'))
) ENGINE = InnoDB;

-- ---------------------------------------------------------------------------
-- OPTIONAL (recommended): a dedicated MySQL account for the application,
-- instead of using root. Remove the "-- " at the start of the three lines
-- below, replace CHOOSE_A_STRONG_PASSWORD with your own password, run them
-- once, and put the same user/password in config/database.ini.
-- Never commit the real password to GitHub.
-- ---------------------------------------------------------------------------
-- CREATE USER IF NOT EXISTS 'freshfold_app'@'localhost' IDENTIFIED BY 'CHOOSE_A_STRONG_PASSWORD';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON freshfold_db.* TO 'freshfold_app'@'localhost';
-- FLUSH PRIVILEGES;
