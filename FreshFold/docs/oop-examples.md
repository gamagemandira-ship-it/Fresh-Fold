# FreshFold – How the Six OOP Concepts Are Used

Every example below is copied from the project source code. The file name is given so you can open it during the demonstration.

Quick summary for the viva:

| Concept | Where to point | One-sentence answer |
|---|---|---|
| Class | `Customer`, `LaundryOrder` | A class is a blueprint that groups data and the methods that work on that data. |
| Object | `CustomerForm::onAddClick`, `DatabaseManager::findOrderById` | An object is a real instance of a class, created at runtime with its own values. |
| Encapsulation | private members + validating setters in `Customer` / `LaundryOrder` | Data is private; it can only change through methods that check the new value. |
| Abstraction | `calculateBill()`, `BillingService::quote()` | The screens ask for a bill without knowing the pricing formula. |
| Inheritance | `class DryCleaningOrder : public LaundryOrder` | Dry cleaning reuses everything from a normal order and adds a handling fee. |
| Polymorphism | `virtual calculateBill()` called through `LaundryOrder&` / `unique_ptr<LaundryOrder>` | The same call runs a different version depending on the real object type. |

---

## 1. Class

**Where:** `src/Models/Customer.h`, `src/Models/LaundryOrder.h`

```cpp
class Customer
{
private:
    int customerId;          // 0 means "not saved in the database yet"
    std::string fullName;
    std::string phone;
    std::string address;

public:
    Customer(const std::string& fullName, const std::string& phone, const std::string& address);
    int getCustomerId() const;
    std::string getFullName() const;
    void setFullName(const std::string& name);
    // ...
};
```

**Explanation:** `Customer` describes what every customer has (ID, name, phone, address) and what can be done with it (read and change the values safely). `LaundryOrder` is the class for an order: customer ID, service, quantity, unit price, status, payment status and date, plus methods such as `calculateBill()` and `moveToStatus()`.

**In the running application:** the Customers screen works with `Customer`; the Orders and Billing screens work with `LaundryOrder` / `DryCleaningOrder`.

---

## 2. Object

**Where:** `src/Forms/CustomerForm.cpp` (`onAddClick`), `src/Database/DatabaseManager.cpp` (`findCustomerById`, `findOrderById`)

Creating an object when a customer is registered:

```cpp
// OBJECT: a new Customer object is created from the form values.
// ENCAPSULATION: the constructor validates every value through the
// setters and throws std::invalid_argument if anything is wrong.
Customer customer(toStdString(txtName->Text), toStdString(txtPhone->Text), toStdString(txtAddress->Text));

int newId = database->addCustomer(customer);
```

Creating an object when a customer is read from MySQL:

```cpp
// OBJECT: a Customer object is created from the database row.
DataRow^ row = table->Rows[0];
result = Customer(Convert::ToInt32(row["customer_id"]),
                  toStdString(Convert::ToString(row["full_name"])),
                  toStdString(Convert::ToString(row["phone"])),
                  toStdString(Convert::ToString(row["address"])));
```

**Explanation:** the class is only the plan; `customer` is a real object in memory with real values. Each customer in the table becomes its own object when it is loaded.

**In the running application:** click **Add Customer** (object created from the form), or click a row in the customer table (object created from the database and shown in the form).

---

## 3. Encapsulation

**Where:** private members in `Customer.h` and `LaundryOrder.h`; validating setters in `Customer.cpp` and `LaundryOrder.cpp`

```cpp
void Customer::setPhone(const std::string& phoneNumber)
{
    std::string cleaned = trim(phoneNumber);
    if (cleaned.empty())
        throw std::invalid_argument("Contact number is required.");
    if (cleaned.length() > 20)
        throw std::invalid_argument("Contact number must be 20 characters or fewer.");
    // ... only digits, spaces, + - ( ) allowed, at least 7 digits ...
    phone = cleaned;
}
```

```cpp
void LaundryOrder::setQuantity(double newQuantity)
{
    if (!(newQuantity > 0.0))   // also rejects NaN
        throw std::invalid_argument("Quantity must be greater than zero.");

    if (usesWeight(serviceType))
    {
        if (newQuantity > 100.0)
            throw std::invalid_argument("Weight must be 100 kg or less for a single order.");
        // ...
    }
    else
    {
        if (std::floor(newQuantity) != newQuantity)
            throw std::invalid_argument("Number of items must be a whole number.");
        // ...
    }
    quantity = newQuantity;
}
```

**Explanation:** `customerId`, `fullName`, `phone`, `address`, `orderId`, `status`, `quantity` and `unitPrice` are `private`. Other code cannot write `customer.phone = "abc"`; it must call `setPhone()`, which rejects bad values. If a setter throws, the old value is kept, so an object is never left in an invalid state.

The order total is also protected: there is **no** `setTotalAmount()`. The total is always calculated from the private quantity and unit price, so nobody can type in a wrong total.

The status workflow is encapsulated too – `moveToStatus()` only allows one step forward (Pending → In Wash → Ready for Pickup → Completed) and refuses "Completed" until the order is paid.

**In the running application:** try to add a customer with an empty name or the phone number `abc`, or create an order with quantity `0` – the error message comes from these setters.

---

## 4. Abstraction

**Where:** `LaundryOrder::calculateBill()`, `BillingService::quote()`, `OrderForm::onCalculateClick()`

```cpp
void OrderForm::onCalculateClick(Object^, EventArgs^)
{
    try
    {
        std::string service = readService();
        double quantity = readQuantity(service);

        // ABSTRACTION: the form only asks for a bill. The pricing formula is
        // hidden inside the order classes.
        BillQuote quote = billing->quote(service, quantity);
        txtBill->Text = toDisplayText(quote.breakdown);
        // ...
```

**Explanation:** the Orders screen does not contain any price or formula. It calls one public method and receives the result. All prices are in one file, `src/Models/PriceList.h`:

```cpp
const double WASH_FOLD_PER_KG = 350.00;
const double STEAM_PRESS_PER_ITEM = 120.00;
const double DRY_CLEAN_PER_ITEM = 600.00;
const double DRY_CLEAN_HANDLING_FEE = 150.00;
```

Changing a price means editing one line there – no screen needs to change.

**In the running application:** choose a service, enter a quantity and click **Calculate Bill**. The invoice on the Billing screen uses the same methods.

---

## 5. Inheritance

**Where:** `src/Models/DryCleaningOrder.h` and `.cpp`

```cpp
class DryCleaningOrder : public LaundryOrder
{
private:
    double handlingFee;   // dry-cleaning-specific data

public:
    explicit DryCleaningOrder(double numberOfItems,
                              double pricePerItem = PriceList::DRY_CLEAN_PER_ITEM,
                              double handlingFee = PriceList::DRY_CLEAN_HANDLING_FEE);
    // ...
};
```

```cpp
// The base-class constructor runs first and stores the common data.
DryCleaningOrder::DryCleaningOrder(double numberOfItems, double pricePerItem, double handlingFee)
    : LaundryOrder(ServiceType::DRY_CLEANING, numberOfItems, pricePerItem),
      handlingFee(handlingFee)
```

**Explanation:** a dry-cleaning order **is a** laundry order. It inherits the customer, quantity, status, payment status, validation and workflow rules from `LaundryOrder`, so none of that code is repeated. It adds one new piece of data (`handlingFee`) and changes how the bill is calculated. The base class has a `protected` helper, `describeQuantityTimesPrice()`, that only the base class and derived classes can use.

**In the running application:** create a **Dry Cleaning** order – it has a status, payment status and date exactly like other orders, but its bill includes the handling fee.

---

## 6. Polymorphism (runtime)

**Where:** `LaundryOrder.h` (virtual methods), `DryCleaningOrder.cpp` (overrides), `OrderFactory.cpp`, `BillingService::calculateBill()`, `DatabaseManager::addOrder()`

Virtual method in the base class and override in the derived class:

```cpp
// LaundryOrder.h
virtual double calculateBill() const;

// DryCleaningOrder.h
double calculateBill() const override;
```

```cpp
// Standard bill: quantity x unit price.
double LaundryOrder::calculateBill() const
{
    return roundToCents(quantity * unitPrice);
}

// Dry-cleaning bill = (items x price per item) + handling fee.
double DryCleaningOrder::calculateBill() const
{
    return roundToCents(LaundryOrder::calculateBill() + handlingFee);
}
```

The factory returns a **base-class pointer**, even when the object is a `DryCleaningOrder`:

```cpp
std::unique_ptr<LaundryOrder> createOrder(const std::string& serviceType, double quantity)
{
    if (serviceType == ServiceType::DRY_CLEANING)
    {
        // Derived-class object stored in a base-class pointer.
        return std::unique_ptr<LaundryOrder>(new DryCleaningOrder(quantity));
    }
    return std::unique_ptr<LaundryOrder>(new LaundryOrder(serviceType, quantity, currentUnitPrice(serviceType)));
}
```

The bill is calculated through a **base-class reference**:

```cpp
double BillingService::calculateBill(const LaundryOrder& order)
{
    // Runtime polymorphism: which calculateBill() runs is decided by the
    // real type of 'order', not by the type of this parameter.
    return order.calculateBill();
}
```

and when an order is saved:

```cpp
// POLYMORPHISM: 'order' is a LaundryOrder reference, but if the real
// object is a DryCleaningOrder, its overridden calculateBill() and
// getServiceFee() run here.
double total = order.calculateBill();
double serviceFee = order.getServiceFee();
```

**Explanation:** `BillingService` and `DatabaseManager` only know the type `LaundryOrder`. At runtime C++ looks at the real object (through the virtual table) and runs `DryCleaningOrder::calculateBill()` for dry-cleaning orders and `LaundryOrder::calculateBill()` for the others. `getServiceFee()` and `getBillBreakdown()` work the same way. `LaundryOrder` also has a `virtual` destructor, which is required when objects are deleted through a base-class pointer.

**Proof that it really runs:** `tests/ModelTests.cpp` puts three different orders in a `std::vector<std::unique_ptr<LaundryOrder>>`, calls `calculateBill()` on each through the base pointer, and checks that the dry-cleaning result includes the handling fee:

```
Runtime polymorphism
    Wash & Fold -> Rs. 700.00
    Dry Cleaning -> Rs. 1950.00
    Steam Press -> Rs. 600.00
  [PASS] factory created a DryCleaningOrder
  [PASS] base pointer calls DryCleaningOrder::calculateBill()
  [PASS] base reference calls the derived override
```

**In the running application:** on the Orders screen, calculate a bill for **3 items** of **Steam Press** (Rs. 360.00) and then for **3 items** of **Dry Cleaning** (Rs. 1950.00 = 3 × 600 + 150). The screen code calls the same method in both cases; only the object type is different.
