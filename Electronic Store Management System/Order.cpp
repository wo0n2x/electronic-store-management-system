#include "Order.h"
#include "Customer.h"
#include "Utility.h"
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <cctype>
#include <ctime>
#include <limits>

#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>

using namespace std;

namespace
{
    void printOrderTableLine(int width)
    {
        printTableLine(width);
    }

    void printOrderStatusField(string status, int width)
    {
        int padding =
            width - static_cast<int>(status.length());

        if (padding < 0)
        {
            padding = 0;
        }

        printStatus(status);
        cout << string(padding, ' ');
    }

    bool parseInt(string input, int& value)
    {
        istringstream stream(input);
        char extra;

        return (stream >> value) &&
            !(stream >> extra);
    }

    void readOrderChoice(
        string prompt,
        int& choice,
        int minValue,
        int maxValue
    )
    {
        string input;

        margin();
        cout << prompt;
        cin >> input;

        while (!parseInt(input, choice) ||
            choice < minValue ||
            choice > maxValue)
        {
            displayError(
                "Invalid choice. Please enter a number between " +
                to_string(minValue) +
                " and " +
                to_string(maxValue) +
                "."
            );

            margin();
            cout << prompt;
            cin >> input;
        }
    }

    void readPositiveQuantity(int& quantity)
    {
        string input;

        cout << "\nEnter Quantity : ";
        cin >> input;

        while (!parseInt(input, quantity) ||
            quantity <= 0)
        {
            displayError("Invalid quantity. Please enter a whole number greater than 0.");
            cout << "\nEnter Quantity : ";
            cin >> input;
        }
    }

    char readOrderConfirmation(string prompt)
    {
        string input;

        while (true)
        {
            margin();
            cout << prompt << " (Y/N): ";
            cin >> input;

            if (input.length() == 1)
            {
                char confirm =
                    static_cast<char>(
                        toupper(static_cast<unsigned char>(input[0]))
                    );

                if (confirm == 'Y' || confirm == 'N')
                {
                    return confirm;
                }
            }

            displayError("Please enter Y or N.");
        }
    }

    string currentDateTimeText()
    {
        time_t now =
            time(nullptr);

        tm localTime;
        localtime_s(&localTime, &now);

        stringstream stream;

        stream << put_time(
            &localTime,
            "%Y-%m-%d %H:%M:%S"
        );

        return stream.str();
    }

    string generateReceiptNo(string orderID)
    {
        if (!orderID.empty() &&
            isalpha(static_cast<unsigned char>(orderID[0])))
        {
            return "R" + orderID.substr(1);
        }

        return "R" + orderID;
    }

    string fitReceiptText(string text, int width)
    {
        if (static_cast<int>(text.length()) <= width)
        {
            return text;
        }

        if (width <= 3)
        {
            return text.substr(0, width);
        }

        return text.substr(0, width - 3) + "...";
    }

    string receiptMoneyText(double value)
    {
        stringstream stream;

        stream << "RM "
            << fixed
            << setprecision(2)
            << value;

        return stream.str();
    }

    void printReceiptField(string label, string value)
    {
        margin();
        cout << left
            << setw(16) << label
            << ": "
            << value
            << endl;
    }

    void printReceiptField(string label, double value)
    {
        printReceiptField(label, receiptMoneyText(value));
    }

    void printReceiptStatus(string status)
    {
        margin();
        cout << left
            << setw(16) << "Status"
            << ": ";
        printStatus(status);
        cout << endl;
    }

    void displayReceiptProducts(
        sql::Connection* con,
        string orderID
    )
    {
        sql::PreparedStatement* pstmt = nullptr;
        sql::ResultSet* res = nullptr;

        try
        {
            pstmt = con->prepareStatement(
                "SELECT od.ProductID, "
                "p.ProductName, "
                "od.Quantity, "
                "od.UnitPrice, "
                "od.Subtotal "
                "FROM ORDERDETAILS od "
                "INNER JOIN PRODUCT p "
                "ON od.ProductID = p.ProductID "
                "WHERE od.OrderID = ? "
                "ORDER BY od.ProductID"
            );

            pstmt->setString(1, orderID);

            res = pstmt->executeQuery();

            displaySection("PRODUCTS");

            margin();
            setTableHeaderColor();
            cout << left
                << setw(12) << "Product ID"
                << setw(28) << "Product Name"
                << setw(6) << "Qty"
                << setw(14) << "Unit Price"
                << setw(14) << "Subtotal"
                << endl;
            resetTextColor();

            printOrderTableLine(74);

            while (res->next())
            {
                margin();
                cout << left
                    << setw(12) << res->getString("ProductID")
                    << setw(28)
                    << fitReceiptText(
                        res->getString("ProductName"),
                        27
                    )
                    << setw(6) << res->getInt("Quantity")
                    << setw(14)
                    << receiptMoneyText(res->getDouble("UnitPrice"))
                    << setw(14)
                    << receiptMoneyText(res->getDouble("Subtotal"))
                    << endl;
            }

            printOrderTableLine(74);

            delete res;
            delete pstmt;
        }
        catch (sql::SQLException& e)
        {
            delete res;
            delete pstmt;

            displayError(e.what());
        }
    }

    void printReceiptGrandTotal(double amount)
    {
        cout << "\n";
        margin();
        cout << left
            << setw(16) << "Grand Total"
            << ": "
            << receiptMoneyText(amount)
            << endl;
    }

    void printReceiptSuccessBanner()
    {
        const int width = 58;
        string message =
            "PAYMENT PROCESSED SUCCESSFULLY";

        int leftPadding =
            (width - static_cast<int>(message.length())) / 2;

        if (leftPadding < 0)
        {
            leftPadding = 0;
        }

        printOrderTableLine(width);

        margin();
        setSuccessColor();
        cout << string(leftPadding, ' ')
            << message
            << endl;
        resetTextColor();

        printOrderTableLine(width);
    }

    void displayPaymentReceipt(
        sql::Connection* con,
        string orderID,
        string customerID,
        string paymentMethod,
        double orderAmount,
        string paymentDate
    )
    {
        clearScreen();

        displayHeader("PAYMENT RECEIPT");

        cout << "\n";

        printReceiptField(
            "Receipt No",
            generateReceiptNo(orderID)
        );
        printReceiptField("Order ID", orderID);
        printReceiptField("Customer ID", customerID);
        printReceiptField("Payment Method", paymentMethod);
        printReceiptField("Payment Date", paymentDate);

        cout << "\n";

        printReceiptStatus("COMPLETED");

        cout << "\n";

        displayReceiptProducts(con, orderID);

        printReceiptGrandTotal(orderAmount);

        cout << "\n";

        printReceiptSuccessBanner();
    }
}

//=====================================================
// Cart Structure
//=====================================================

struct CartItem
{
    string productID = "";

    string productName = "";

    double price = 0;

    int quantity = 0;

    double subtotal = 0;
};

//=====================================================
// Function Prototypes
//=====================================================

void createOrder(sql::Connection* con);

void viewOrders(sql::Connection* con);

void searchOrder(sql::Connection* con);

void cancelOrder(sql::Connection* con);

void restoreOrder(sql::Connection* con);

void displayCart(vector<CartItem>& cart);

void addProductToCart(
    sql::Connection* con,
    vector<CartItem>& cart
);

void saveOrder(
    sql::Connection* con,
    string customerID,
    vector<CartItem>& cart
);

void checkoutOrder(
    sql::Connection* con,
    string customerID,
    vector<CartItem>& cart
);

void searchOrder(sql::Connection* con);

// Display Cart

void displayCart(vector<CartItem>& cart)
{
    double grandTotal = 0;

    displayHeader("CURRENT CART");

    if (cart.empty())
    {
        displayInfo("Cart is empty.");

        return;
    }

    setTableHeaderColor();
    margin();
    cout << left
        << setw(12) << "ProductID"
        << setw(25) << "Product"
        << setw(12) << "Price"
        << setw(8) << "Qty"
        << setw(12) << "Total"
        << endl;
    resetTextColor();

    printOrderTableLine(64);

    for (auto& item : cart)
    {
        margin();

        cout << left
            << setw(12) << item.productID
            << setw(25) << item.productName
            << setw(12) << fixed << setprecision(2) << item.price
            << setw(8) << item.quantity
            << setw(12) << fixed << setprecision(2) << item.subtotal
            << endl;

        grandTotal += item.subtotal;
    }

    printOrderTableLine(64);

    margin();

    cout << right
        << setw(45)
        << "Grand Total : RM "
        << fixed << setprecision(2)
        << grandTotal
        << endl;

    printOrderTableLine(60);
}

// Generate OrderID
string generateOrderID(sql::Connection* con)
{
    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT COUNT(*) AS TotalOrders "
            "FROM ORDERS"
        );

        int count = 0;

        if (res->next())
        {
            count = res->getInt("TotalOrders");
        }

        delete res;
        delete stmt;

        count++;

        stringstream ss;

        ss << "O"
            << setw(4)
            << setfill('0')
            << count;

        return ss.str();
    }
    catch (...)
    {
        return "O0001";
    }
}
void checkoutOrder(
    sql::Connection* con,
    string customerID,
    vector<CartItem>& cart
)
{
    if (cart.empty())
    {
        displayInfo("Cart is empty.");

        return;
    }

    string paymentMethod;

    clearScreen();

    displayHeader("CHECKOUT SYSTEM");

        
    displayCart(cart);

    clearScreen();

    displayHeader("ORDER SUMMARY");
    displayPath("HOME > ORDER MANAGEMENT > CREATE ORDER > CHECKOUT");

    margin();
    cout << "Customer ID : "
        << customerID << endl;

    margin();
    cout << "Status      : ";
    printStatus("PENDING");
    cout << endl;

    displayCart(cart);

    int confirmChoice;

    cout << "\n1. Confirm Order\n";
    cout << "2. Cancel\n";

    readOrderChoice("Enter Choice : ", confirmChoice, 1, 2);

    if (confirmChoice == 1)
    {
        saveOrder(
            con,
            customerID,
            cart
        );
    }
    else
    {
        displayInfo("Checkout cancelled.");
    }
}

void saveOrder(
    sql::Connection* con,
    string customerID,
    vector<CartItem>& cart
)

{
    if (cart.empty())
    {
        displayInfo("Cart is empty.");

        return;
    }


    try
    {
        con->setAutoCommit(false);

        string orderID =
            generateOrderID(con);

        double grandTotal = 0;

        for (auto& item : cart)
        {
            grandTotal += item.subtotal;
        }

        sql::PreparedStatement* pstmt;

        pstmt = con->prepareStatement(
            "INSERT INTO ORDERS "
            "(OrderID, OrderDate, PaymentMethod, "
            "TotalAmount, OrderStatus, CustomerID) "
            "VALUES "
            "(?, NOW(), NULL, ?, 'PENDING', ?)"
        );

        pstmt->setString(1, orderID);
        pstmt->setDouble(2, grandTotal);
        pstmt->setString(3, customerID);
        pstmt->executeUpdate();

        delete pstmt;

        for (auto& item : cart)
        {
            pstmt = con->prepareStatement(
                "INSERT INTO ORDERDETAILS "
                "(OrderID, ProductID, UnitPrice, Quantity, Subtotal) "
                "VALUES (?, ?, ?, ?, ?)"
            );

            pstmt->setString(1, orderID);

            pstmt->setString(2, item.productID);

            pstmt->setDouble(3, item.price);

            pstmt->setInt(4, item.quantity);

            pstmt->setDouble(5, item.subtotal);

            pstmt->executeUpdate();

            delete pstmt;

            pstmt = con->prepareStatement(
                "UPDATE PRODUCT "
                "SET StockQuantity = "
                "StockQuantity - ? "
                "WHERE ProductID=?"
            );

            pstmt->setInt(1, item.quantity);
            pstmt->setString(2, item.productID);

            pstmt->executeUpdate();

            delete pstmt;
        }

        con->commit();

        con->setAutoCommit(true);

     
        displaySuccess("ORDER SAVED SUCCESSFULLY");


        cout << "\nOrder ID : "
            << orderID << endl;
    }
    catch (sql::SQLException& e)
    {
        con->rollback();

        con->setAutoCommit(true);

        displayError(e.what());
    }
}


//add Product To Cart
void addProductToCart(
    sql::Connection* con,
    vector<CartItem>& cart
)
{

    string productID;

    int quantity;


    clearScreen();

    displayHeader("ADD PRODUCT TO CART");

    displayPath(
        "HOME > ORDER MANAGEMENT > ADD PRODUCT"
    );

    displaySection(
        "AVAILABLE PRODUCTS"
    );

    sql::Statement* stmt;
    sql::ResultSet* productList;

    stmt = con->createStatement();

    productList = stmt->executeQuery(
        "SELECT ProductID, ProductName, "
        "Price, StockQuantity "
        "FROM PRODUCT "
        "WHERE Status='ACTIVE' "
        "AND StockQuantity > 0 "
        "ORDER BY ProductName"
    );

    margin();
    setTableHeaderColor();
    cout << left
        << setw(12) << "ProductID"
        << setw(30) << "Product Name"
        << setw(12) << "Price"
        << setw(10) << "Stock"
        << endl;
    resetTextColor();

    printOrderTableLine(64);

    while (productList->next())
    {
        margin();

        cout << left
            << setw(12)
            << productList->getString("ProductID")

            << setw(30)
            << productList->getString("ProductName")

            << setw(12)
            << fixed << setprecision(2)
            << productList->getDouble("Price")

            << setw(10)
            << productList->getInt("StockQuantity")

            << endl;
    }

    printOrderTableLine(64);

    delete productList;
    delete stmt;


    cout << "\n";

    margin();
    cout << "Enter Product ID (0 to cancel): ";
    cin >> productID;

    if (productID == "0")
    {
        displayInfo("Add product cancelled.");
        return;
    }
    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM PRODUCT "
            "WHERE ProductID=? "
            "AND Status='ACTIVE'"
        );
        pstmt->setString(1, productID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            displayError("Product not found!");

            delete res;
            delete pstmt;

            pauseScreen();

            addProductToCart(con, cart);

            return;
        }

        string productName =
            res->getString("ProductName");

        double price =
            res->getDouble("Price");

        int stockQty =
            res->getInt("StockQuantity");

        delete res;
        delete pstmt;

        cout << "\nProduct Name : "
            << productName << endl;

        cout << "Price : RM "
            << fixed << setprecision(2)
            << price << endl;

        cout << "Available Stock : "
            << stockQty << endl;

        readPositiveQuantity(quantity);

        if (quantity > stockQty)
        {
            displayError("Insufficient stock!");

            pauseScreen();

            addProductToCart(con, cart);

            return;
        }

        double subtotal =
            quantity * price;

        CartItem item;

        item.productID = productID;

        item.productName = productName;

        item.price = price;

        item.quantity = quantity;

        item.subtotal = subtotal;

        cart.push_back(item);

        displaySuccess("PRODUCT ADDED TO CART");
        cout << "\n";

        margin();
        cout << "Items In Cart : "
            << cart.size()
            << endl;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}
//=====================================================
// Order Management Menu
//=====================================================

void orderManagementMenu(sql::Connection* con)
{
    int choice;

    do
    {
        clearScreen();

        displayHeader("ORDER MANAGEMENT");

        margin();
        cout << "1. Create Order\n";

        margin();
        cout << "2. View Orders\n";

        margin();
        cout << "3. Search Order\n";

        margin();
        cout << "4. Process Payment\n";

        margin();
        cout << "5. Cancel Order\n";

        margin();
        cout << "6. Restore Order\n";

        margin();
        cout << "7. Back\n";

        cout << "\n";

        readOrderChoice("Enter Choice : ", choice, 1, 7);

        switch (choice)
        {
        case 1:

            clearScreen();
            createOrder(con);
            pauseScreen();

            break;

        case 2:

            clearScreen();
            viewOrders(con);
            pauseScreen();

            break;

        case 3:

            clearScreen();
            searchOrder(con);
            pauseScreen();

            break;

        case 4:

            clearScreen();
            processPayment(con);
            pauseScreen();

            break;

        case 5:

            clearScreen();
            cancelOrder(con);
            pauseScreen();

            break;

        case 6:

            clearScreen();
            restoreOrder(con);
            pauseScreen();

            break;

        case 7:

            return;

        default:

            displayError("Invalid choice.");

            pauseScreen();
        }

    } while (true);
}


//=====================================================
// Create Order
//=====================================================
void createOrder(sql::Connection* con)
{
    string customerID;
    vector<CartItem> cart;

    
        clearScreen();

    displayHeader("CREATE ORDER");

    displayPath(
        "HOME > ORDER MANAGEMENT > CREATE ORDER"
    );
    displayCustomerList(con);

    cout << "\n";

    margin();
    cout << "Enter Customer ID (0 to cancel): ";
    cin >> customerID;

    if (customerID == "0")
    {
        displayInfo("Create order cancelled.");
        return;
    }


    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM CUSTOMER "
            "WHERE CustomerID=? "
            "AND Status='ACTIVE'"
        );

        pstmt->setString(1, customerID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            displayError(
                "Customer not found."
            );

            delete res;
            delete pstmt;

            return;
        }

        string customerName =
            res->getString("CustomerName");

        delete res;
        delete pstmt;

        int choice;

        do
        {
            clearScreen();

            displayHeader("ORDER SESSION");

            displayPath(
                "HOME > ORDER MANAGEMENT > CREATE ORDER"
            );

            displaySection(
                "CUSTOMER INFORMATION"
            );

            margin();
            cout << left
                << setw(15) << "Customer ID"
                << ": "
                << customerID
                << endl;

            margin();
            cout << left
                << setw(15) << "Customer Name"
                << ": "
                << customerName
                << endl;

            displaySection(
                "ORDER MENU"
            );

            margin();
            cout << "1. Add Product To Cart\n";

            margin();
            cout << "2. View Cart\n";

            margin();
            cout << "3. Checkout\n";

            margin();
            cout << "4. Cancel Order\n";

            cout << "\n";

            readOrderChoice("Enter Choice : ", choice, 1, 4);

            switch (choice)
            {
            case 1:

                addProductToCart(
                    con,
                    cart
                );

                break;

            case 2:

                clearScreen();

                displayHeader(
                    "VIEW CART"
                );

                displayPath(
                    "HOME > ORDER MANAGEMENT > VIEW CART"
                );

                displayCart(cart);

                pauseScreen();

                break;

            case 3:

                checkoutOrder(
                    con,
                    customerID,
                    cart
                );

                return;

            case 4:

                displayWarning(
                    "Order session cancelled."
                );

                return;
            }

        } while (true);
    }
    catch (sql::SQLException& e)
    {
        displayError(
            e.what()
        );
    }
    

}


//=====================================================
// View Orders
//=====================================================

void viewOrders(sql::Connection* con)
{
    clearScreen();

    displayHeader("COMPLETED ORDERS");
    displayPath(
        "HOME > ORDER MANAGEMENT > View Orders"
    );

    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT * FROM ORDERS "
            "WHERE OrderStatus='COMPLETED' "
            "ORDER BY OrderDate DESC"
         );

        margin();
        setTableHeaderColor();
        cout << left
            << setw(12) << "OrderID"
            << setw(15) << "Customer"
            << setw(20) << "Payment"
            << setw(15) << "Status"
            << setw(12) << "Total"
            << endl;
        resetTextColor();

        printOrderTableLine(74);

        int totalOrders = 0;

        while (res->next())
        {
            string status =
                res->getString("OrderStatus");

            if (status == "COMPLETED")
            {
                status = "COMPLETED";
            }
            else if (status == "CANCELLED")
            {
                status = "CANCELLED";
            }

            margin();
            cout << left
                << setw(12) << res->getString("OrderID")
                << setw(15) << res->getString("CustomerID")
                << setw(20) << res->getString("PaymentMethod");
            printOrderStatusField(status, 15);
            cout << fixed << setprecision(2)
                << res->getDouble("TotalAmount")
                << endl;
            totalOrders++;
        }

        printOrderTableLine(74);

        margin();

        cout << "\nTotal Completed Orders : "
            << totalOrders
            << endl;
        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}


void processPayment(sql::Connection* con)
{
    clearScreen();

    displayHeader("PROCESS PAYMENT");

    displayPath(
        "HOME > ORDER MANAGEMENT > PROCESS PAYMENT"
    );

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM ORDERS "
            "WHERE OrderStatus='PENDING' "
            "ORDER BY OrderDate DESC"
        );

        res = pstmt->executeQuery();

        cout << "\n";

        margin();
        setTableHeaderColor();
        cout << left
            << setw(12) << "OrderID"
            << setw(15) << "Customer"
            << setw(15) << "Status"
            << setw(15) << "Total"
            << endl;
        resetTextColor();

        printOrderTableLine(57);

        int totalPending = 0;

        while (res->next())
        {
            margin();

            cout << left
                << setw(12) << res->getString("OrderID")
                << setw(15) << res->getString("CustomerID");
            printOrderStatusField(res->getString("OrderStatus"), 15);
            cout << fixed << setprecision(2)
                << res->getDouble("TotalAmount")
                << endl;

            totalPending++;
        }

        printOrderTableLine(57);

        if (totalPending == 0)
        {
            displayInfo("No pending orders available for payment.");
            delete res;
            delete pstmt;

            return;
        }

        string orderID;

        cout << "\n";

        margin();
        cout << "Enter Order ID (0 to cancel): ";
        cin >> orderID;

        if (orderID == "0")
        {
            displayInfo("Payment cancelled.");

            delete res;
            delete pstmt;

            return;
        }

        delete res;
        delete pstmt;

        pstmt = con->prepareStatement(
            "SELECT * FROM ORDERS "
            "WHERE OrderID=? "
            "AND OrderStatus='PENDING'"
        );

        pstmt->setString(1, orderID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            displayError("Pending order not found.");

            delete res;
            delete pstmt;

            return;
        }

        string paymentCustomerID =
            res->getString("CustomerID");

        double paymentAmount =
            res->getDouble("TotalAmount");

        delete res;
        delete pstmt;

        int paymentChoice;
        string paymentMethod;

        cout << "\n";

        margin();
        cout << "1. Cash\n";

        margin();
        cout << "2. Credit Card\n";

        margin();
        cout << "3. Online Banking\n";

        margin();
        cout << "4. E-Wallet\n";

        cout << "\n";

        readOrderChoice("Enter Payment Method : ", paymentChoice, 1, 4);

        switch (paymentChoice)
        {
        case 1:
            paymentMethod = "Cash";
            break;

        case 2:
            paymentMethod = "Credit Card";
            break;

        case 3:
            paymentMethod = "Online Banking";
            break;

        case 4:
            paymentMethod = "E-Wallet";
            break;
        }

        char confirm;

        cout << "\n";

        confirm =
            readOrderConfirmation("Confirm Payment?");

        if (confirm != 'Y')
        {
            displayInfo("Operation cancelled.");
            return;
        }

        pstmt = con->prepareStatement(
            "UPDATE ORDERS "
            "SET PaymentMethod=?, "
            "OrderStatus='COMPLETED' "
            "WHERE OrderID=?"
        );

        pstmt->setString(1, paymentMethod);
        pstmt->setString(2, orderID);

        pstmt->executeUpdate();

        delete pstmt;

        displayPaymentReceipt(
            con,
            orderID,
            paymentCustomerID,
            paymentMethod,
            paymentAmount,
            currentDateTimeText()
        );
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}

//=====================================================
// Search Order
//=====================================================

void searchOrder(sql::Connection* con)
{
    clearScreen();

    displayHeader("SEARCH ORDER");
    displayPath(
        "HOME > ORDER MANAGEMENT > SEARCH ORDER"
    );

    string orderID;

    cout << "\n";
    margin();
    cout << "Enter Order ID (0 to cancel): ";
    cin >> orderID;

    if (orderID == "0")
    {
        displayInfo("Search cancelled.");
        return;
    }

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        // Order Information
        pstmt = con->prepareStatement(
            "SELECT * "
            "FROM ORDERS "
            "WHERE OrderID = ?"
        );

        pstmt->setString(1, orderID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            displayError("Order not found!");

            delete res;
            delete pstmt;

            return;
        }

        displaySection("ORDER DETAILS");

        margin();
        cout << left
            << setw(15) << "Order ID"
            << ": "
            << res->getString("OrderID") << endl;

        margin();
        cout << left
            << setw(15) << "Customer ID"
            << ": "
            << res->getString("CustomerID") << endl;

        margin();
        cout << left
            << setw(15) << "Payment"
            << ": "
            << res->getString("PaymentMethod") << endl;

        margin();
        cout << left
            << setw(15) << "Status"
            << ": ";
        printStatus(res->getString("OrderStatus"));
        cout << endl;

        string dateTime =
            res->getString("OrderDate");

        string orderDate =
            dateTime.substr(0, 10);

        string orderTime =
            dateTime.substr(11, 8);

        margin();
        cout << left
            << setw(15) << "Order Date"
            << ": "
            << orderDate
            << endl;

        margin();
        cout << left
            << setw(15) << "Order Time"
            << ": "
            << orderTime
            << endl;


        double grandTotal =
            res->getDouble("TotalAmount");

        delete res;
        delete pstmt;

        displaySection("ORDER ITEMS");

        // Order Items
        pstmt = con->prepareStatement(
            "SELECT "
            "od.ProductID, "
            "p.ProductName, "
            "od.Quantity, "
            "od.UnitPrice, "
            "od.Subtotal "
            "FROM ORDERDETAILS od "
            "INNER JOIN PRODUCT p "
            "ON od.ProductID = p.ProductID "
            "WHERE od.OrderID = ?"
        );

        pstmt->setString(1, orderID);

        res = pstmt->executeQuery();

        margin();
        setTableHeaderColor();
        cout << left
            << setw(12) << "ProductID"
            << setw(25) << "Product"
            << setw(10) << "Qty"
            << setw(12) << "Price"
            << setw(12) << "Subtotal"
            << endl;
        resetTextColor();

        printOrderTableLine(71);

        while (res->next())
        {
            margin();
            cout << left
                << setw(12)
                << res->getString("ProductID")

                << setw(25)
                << res->getString("ProductName")

                << setw(10)
                << res->getInt("Quantity")

                << setw(12)
                << fixed << setprecision(2)
                << res->getDouble("UnitPrice")

                << setw(12)
                << fixed << setprecision(2)
                << res->getDouble("Subtotal")

                << endl;
        }

        printOrderTableLine(71);

        cout << "\n";
        margin();
        cout << "Grand Total : RM "
            << fixed << setprecision(2)
            << grandTotal
            << endl;

        delete res;
        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }

    system("pause");
}

//=====================================================
// Cancel Order
//=====================================================

void cancelOrder(sql::Connection* con)
{
    clearScreen();

    displayHeader("CANCEL ORDER");
    displayPath(
        "HOME > ORDER MANAGEMENT > CANCEL ORDER"
    );

    sql::PreparedStatement* pstmt;
    sql::ResultSet* res;

    pstmt = con->prepareStatement(
        "SELECT * "
        "FROM ORDERS "
        "WHERE OrderStatus='COMPLETED' "
        "ORDER BY OrderDate DESC"
    );

    res = pstmt->executeQuery();

    cout << "\n";

    margin();
    setTableHeaderColor();
    cout << left
        << setw(12) << "OrderID"
        << setw(15) << "Customer"
        << setw(15) << "Status"
        << setw(15) << "Total"
        << endl;
    resetTextColor();

    printOrderTableLine(57);

    while (res->next())
    {
        margin();

        cout << left
            << setw(12) << res->getString("OrderID")
            << setw(15) << res->getString("CustomerID");
        printOrderStatusField(res->getString("OrderStatus"), 15);
        cout << fixed << setprecision(2)
            << res->getDouble("TotalAmount")
            << endl;
    }

    printOrderTableLine(57);

    delete res;
    delete pstmt;

    string orderID;

    cout << "\n";

    margin();
    cout << "Enter Order ID (0 to cancel): ";

    cin >> orderID;

    if (orderID == "0")
    {
        displayInfo("Cancel order cancelled.");
        return;
    }

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        // Check Order
        pstmt = con->prepareStatement(
            "SELECT * "
            "FROM ORDERS "
            "WHERE OrderStatus='COMPLETED' "
            "AND OrderID = ?"
        );
        pstmt->setString(1, orderID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            displayError("Order not found!");

            delete res;
            delete pstmt;

            return;
        }

        string status =
            res->getString("OrderStatus");

        if (status == "CANCELLED")
        {
            displayError("Order already cancelled!");

            delete res;
            delete pstmt;

            return;
        }

        displaySection("ORDER DETAILS");

        margin();
        cout << left
            << setw(15) << "Order ID"
            << ": "
            << res->getString("OrderID")
            << endl;

        margin();
        cout << left
            << setw(15) << "Customer ID"
            << ": "
            << res->getString("CustomerID")
            << endl;

        margin();
        cout << left
            << setw(15) << "Status"
            << ": ";
        printStatus(res->getString("OrderStatus"));
        cout << endl;

        margin();
        cout << left
            << setw(15) << "Total"
            << ": RM "
            << fixed << setprecision(2)
            << res->getDouble("TotalAmount")
            << endl;

        delete res;
        delete pstmt;

        int choice;

        cout << "\n1. Confirm Cancel\n";
        cout << "2. Back\n";

        readOrderChoice("Enter Choice : ", choice, 1, 2);

        if (choice == 2)
        {
            return;
        }

        // Retrieve Order Items
        pstmt = con->prepareStatement(
            "SELECT ProductID, Quantity "
            "FROM ORDERDETAILS "
            "WHERE OrderID = ?"
        );

        pstmt->setString(1, orderID);

        res = pstmt->executeQuery();

        while (res->next())
        {
            string productID =
                res->getString("ProductID");

            int quantity =
                res->getInt("Quantity");

            sql::PreparedStatement* updateStock;

            updateStock = con->prepareStatement(
                "UPDATE PRODUCT "
                "SET StockQuantity = StockQuantity + ? "
                "WHERE ProductID = ?"
            );

            updateStock->setInt(1, quantity);
            updateStock->setString(2, productID);

            updateStock->executeUpdate();

            delete updateStock;
        }

        delete res;
        delete pstmt;

        // Update Status
        pstmt = con->prepareStatement(
            "UPDATE ORDERS "
            "SET OrderStatus = 'CANCELLED' "
            "WHERE OrderID = ?"
        );

        pstmt->setString(1, orderID);

        pstmt->executeUpdate();

        delete pstmt;

        displaySuccess(
            "Order cancelled successfully."
        );

        displayInfo("Product stock restored.");
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }

    system("pause");
}


//=====================================================
// Restore Order
//=====================================================

void restoreOrder(sql::Connection* con)
{
    clearScreen();
    string orderID;

    displayHeader("RESTORE ORDER");

    displayPath(
        "HOME > ORDER MANAGEMENT > RESTORE ORDER"
    );

    displaySection("CANCELLED ORDERS");
    sql::PreparedStatement* pstmt;
    sql::ResultSet* res;

    pstmt = con->prepareStatement(
        "SELECT OrderID, CustomerID, TotalAmount "
        "FROM ORDERS "
        "WHERE OrderStatus='CANCELLED' "
        "ORDER BY OrderDate DESC"
    );

    res = pstmt->executeQuery();

    margin();
    setTableHeaderColor();
    cout << left
        << setw(12) << "OrderID"
        << setw(15) << "Customer"
        << setw(15) << "Status"
        << setw(15) << "Total"
        << endl;
    resetTextColor();

    printOrderTableLine(57);

    int totalOrders = 0;

    while (res->next())
    {
        margin();

        cout << left
            << setw(12) << res->getString("OrderID")
            << setw(15) << res->getString("CustomerID");
        printOrderStatusField("CANCELLED", 15);
        cout << fixed << setprecision(2)
            << res->getDouble("TotalAmount")
            << endl;

        totalOrders++;
    }

    printOrderTableLine(57);

    if (totalOrders == 0)
    {
        displayInfo(
            "No cancelled orders found."
        );

        delete res;
        delete pstmt;

        return;
    }

    delete res;
    delete pstmt;

    cout << "\n";

    margin();
    cout << "Enter Order ID (0 to cancel): ";
    cin >> orderID;

    if (orderID == "0")
    {
        displayInfo("Restore order cancelled.");
        return;
    }


    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        // Check Order
        pstmt = con->prepareStatement(
            "SELECT * "
            "FROM ORDERS "
            "WHERE OrderID=? "
            "AND OrderStatus='CANCELLED'"
        );

        pstmt->setString(1, orderID);

        res = pstmt->executeQuery();

        if (!res->next())
        {

            displayError("Order not found!");

            delete res;
            delete pstmt;

            return;
        }

        clearScreen();

        displayHeader("RESTORE ORDER");

        displayPath(
            "HOME > ORDER MANAGEMENT > RESTORE ORDER"
        );

        string status =
            res->getString("OrderStatus");

        if (status != "CANCELLED")
        {
            displayError("Order is not cancelled!");

            delete res;
            delete pstmt;

            return;
        }

        displaySection("ORDER DETAILS");

        margin();
        cout << left
            << setw(15) << "Order ID"
            << ": "
            << res->getString("OrderID")
            << endl;

        margin();
        cout << left
            << setw(15) << "Customer ID"
            << ": "
            << res->getString("CustomerID")
            << endl;

        margin();
        cout << left
            << setw(15) << "Status"
            << ": ";
        printStatus(res->getString("OrderStatus"));
        cout << endl;

        margin();
        cout << left
            << setw(15) << "Total"
            << ": RM "
            << fixed << setprecision(2)
            << res->getDouble("TotalAmount")
            << endl;
        delete res;
        delete pstmt;

        // Check Stock Availability
        pstmt = con->prepareStatement(
            "SELECT "
            "od.ProductID, "
            "od.Quantity, "
            "p.StockQuantity "
            "FROM ORDERDETAILS od "
            "INNER JOIN PRODUCT p "
            "ON od.ProductID = p.ProductID "
            "WHERE od.OrderID = ?"
        );

        pstmt->setString(1, orderID);

        res = pstmt->executeQuery();

        while (res->next())
        {
            int requiredQty =
                res->getInt("Quantity");

            int stockQty =
                res->getInt("StockQuantity");

            if (stockQty < requiredQty)
            {
                displayError(
                    "Insufficient stock to restore order!"
                );

                delete res;
                delete pstmt;

                return;
            }
        }

        delete res;
        delete pstmt;

        int choice;
        margin();
        cout << "\n1. Confirm Restore\n";
        margin();
        cout << "2. Back\n";

        readOrderChoice("Enter Choice : ", choice, 1, 2);

        if (choice == 2)
        {
            return;
        }

        // Deduct Stock Again
        pstmt = con->prepareStatement(
            "SELECT ProductID, Quantity "
            "FROM ORDERDETAILS "
            "WHERE OrderID = ?"
        );

        pstmt->setString(1, orderID);

        res = pstmt->executeQuery();

        while (res->next())
        {
            string productID =
                res->getString("ProductID");

            int quantity =
                res->getInt("Quantity");

            sql::PreparedStatement* updateStock;

            updateStock = con->prepareStatement(
                "UPDATE PRODUCT "
                "SET StockQuantity = StockQuantity - ? "
                "WHERE ProductID = ?"
            );

            updateStock->setInt(1, quantity);
            updateStock->setString(2, productID);

            updateStock->executeUpdate();

            delete updateStock;
        }

        delete res;
        delete pstmt;

        // Restore Status
        pstmt = con->prepareStatement(
            "UPDATE ORDERS "
            "SET OrderStatus = 'COMPLETED' "
            "WHERE OrderID = ?"
        );

        pstmt->setString(1, orderID);

        pstmt->executeUpdate();

        delete pstmt;

        displaySuccess(
            "Order restored successfully."
        );
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }

    system("pause");
}
