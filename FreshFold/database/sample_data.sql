-- ============================================================================
-- FreshFold - OPTIONAL demonstration data.
-- Run AFTER freshfold_db.sql. All names, phone numbers and addresses are
-- invented placeholders, not real people.
--
-- Safe to re-run: customers are skipped if their phone number already
-- exists, and the sample orders are only added when the orders table is empty.
-- Totals follow PriceList.h: Wash & Fold Rs. 350/kg, Steam Press Rs. 120/item,
-- Dry Cleaning Rs. 600/item + Rs. 150 handling fee.
-- ============================================================================

USE freshfold_db;

INSERT IGNORE INTO customers (full_name, phone, address) VALUES
    ('Sample Customer One',   '070 000 0001', 'No. 1, Example Road, Mihintale'),
    ('Sample Customer Two',   '070 000 0002', 'No. 2, Demo Lane, Anuradhapura'),
    ('Sample Customer Three', '070 000 0003', 'No. 3, Test Avenue, Kurunegala');

INSERT INTO orders (customer_id, service_type, quantity, unit_price, service_fee, total_amount, status, payment_status, order_date)
SELECT c.customer_id, s.service_type, s.quantity, s.unit_price, s.service_fee, s.total_amount, s.status, 'Unpaid', CURRENT_DATE
FROM (
          SELECT '070 000 0001' AS phone, 'Wash & Fold'  AS service_type, 3.50 AS quantity, 350.00 AS unit_price, 0.00   AS service_fee, 1225.00 AS total_amount, 'Pending'          AS status
    UNION ALL SELECT '070 000 0002',      'Dry Cleaning',                 2.00,             600.00,               150.00,                1350.00,                  'In Wash'
    UNION ALL SELECT '070 000 0003',      'Steam Press',                  5.00,             120.00,               0.00,                  600.00,                   'Ready for Pickup'
) AS s
JOIN customers c ON c.phone = s.phone
WHERE NOT EXISTS (SELECT 1 FROM orders);
