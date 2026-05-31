#include "Order.h"
#include "Utility.h"

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>

using namespace std;


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
    vector<CartItem>& cart,
    string paymentMethod
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
        cout << "Cart is empty.\n";

        return;
    }

    cout << left
        << setw(12) << "ProductID"
        << setw(25) << "Product"
        << setw(12) << "Price"
        << setw(8) << "Qty"
        << setw(12) << "Total"
        << endl;

    cout << "----------------------------------------------------------------\n";

    for (auto& item : cart)
    {
        cout << left
            << setw(12) << item.productID
            << setw(25) << item.productName
            << setw(12) << fixed << setprecision(2) << item.price
            << setw(8) << item.quantity
            << setw(12) << fixed << setprecision(2) << item.subtotal
            << endl;

        grandTotal += item.subtotal;
    }

    cout << "----------------------------------------------------------------\n";

    cout << right
        << setw(45)
        << "Grand Total : RM "
        << fixed << setprecision(2)
        << grandTotal
        << endl;

    cout << "============================================================\n";
}

//generate OrderID
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
            count =
                res->getInt("TotalOrders");
        }

        delete res;
        delete stmt;

        count++;

        string newID =
            "O00" + to_string(count);

        return newID;
    }
    catch (...)
    {
        return "O001";
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
        cout << "\nCart is empty!\n";

        return;
    }

    int paymentChoice;

    string paymentMethod;

    clearScreen();

    displayHeader("CHECKOUT SYSTEM");

        
    displayCart(cart);

    displaySection("PAYMENT METHOD");

    cout << "1. Cash\n";
    cout << "2. Credit Card\n";
    cout << "3. Online Banking\n";
    cout << "4. E-Wallet\n";

    cout << "\nEnter Choice : ";

    while (!(cin >> paymentChoice)
        || paymentChoice < 1
        || paymentChoice > 4)
    {
        cin.clear();

        cin.ignore(1000, '\n');

        cout << "\nInvalid choice!";
        cout << "\nEnter again : ";
    }

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

    clearScreen();

    displayHeader("ORDER SUMMARY");
    cout << "Customer ID : "
        << customerID << endl;

    cout << "Payment     : "
        << paymentMethod << endl;

    displayCart(cart);

    int confirmChoice;

    cout << "\n1. Confirm Order\n";
    cout << "2. Cancel\n";

    cout << "\nEnter Choice : ";

    while (!(cin >> confirmChoice)
        || confirmChoice < 1
        || confirmChoice > 2)
    {
        cin.clear();

        cin.ignore(1000, '\n');

        cout << "\nInvalid choice!";
        cout << "\nEnter again : ";
    }

    if (confirmChoice == 1)
    {
        saveOrder(
            con,
            customerID,
            cart,
            paymentMethod
        );
    }
    else
    {
        cout << "\nCheckout cancelled.\n";
    }
}

void saveOrder(
    sql::Connection* con,
    string customerID,
    vector<CartItem>& cart,
    string paymentMethod
)
{
    if (cart.empty())
    {
        cout << "\nCart is empty!\n";

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
            "(?, NOW(), ?, ?, 'COMPLETED', ?)"
        );

        pstmt->setString(1, orderID);
        pstmt->setString(2, paymentMethod);
        pstmt->setDouble(3, grandTotal);
        pstmt->setString(4, customerID);

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

        cout << "\nDatabase Error : "
            << e.what()
            << endl;
    }
}


//add Product To Cart
void addProductToCart(
    sql::Connection* con,
    vector<CartItem>& cart
)
{
    clearScreen();

    string productID;

    int quantity;


    displayHeader("ADD PRODUCT TO CART");

    cout << "\nEnter Product ID : ";
    cin >> productID;

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

        cout << "\nEnter Quantity : ";

        while (!(cin >> quantity) || quantity <= 0)
        {
            cin.clear();

            cin.ignore(1000, '\n');

            cout << "\nInvalid quantity!";
            cout << "\nEnter again : ";
        }

        if (quantity > stockQty)
        {
            cout << "\nInsufficient stock!\n";

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
    }
    catch (sql::SQLException& e)
    {
        cout << "\nDatabase Error : "
            << e.what()
            << endl;
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
        cout << "1. Create Order\n";
        cout << "2. View Orders\n";
        cout << "3. Search Order\n";
        cout << "4. Cancel Order\n";
        cout << "5. Restore Order\n";
        cout << "6. Back\n";

        cout << "\nEnter Choice : ";

        while (!(cin >> choice) || choice < 1 || choice > 6)
        {
            cin.clear();

            cin.ignore(1000, '\n');

            cout << "\nInvalid input!";

            cout << "\nPlease enter a number between 1 - 6.";

            cout << "\n\nEnter Choice : ";
        }

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

            cancelOrder(con);

            pauseScreen();

            break;

        case 5:

            clearScreen();

            restoreOrder(con);

            pauseScreen();

            break;

        case 6:

            return;

        default:

            cout << "\nInvalid option!\n";

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

    cout << "==================================================\n";
    cout << "                 CREATE ORDER\n";
    cout << "==================================================\n";

    cout << "\nEnter Customer ID : ";
    cin >> customerID;

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM CUSTOMER "
            "WHERE CustomerID=?"
        );

        pstmt->setString(1, customerID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            cout << "\nCustomer not found!\n";

            delete res;
            delete pstmt;

            return;
        }

        string customerName =
            res->getString("CustomerName");

        delete res;
        delete pstmt;

        clearScreen();

        displayHeader("ORDER SESSION STARTED");

        cout << " Customer ID   : "
            << customerID << endl;

        cout << " Customer Name : "
            << customerName << endl;

        cout << "\n==================================================\n";

        int choice;

        do
        {
            cout << "\n1. Add Product To Cart\n";
            cout << "2. View Cart\n";
            cout << "3. Checkout\n";
            cout << "4. Cancel Order\n";

            cout << "\nEnter Choice : ";

            while (!(cin >> choice) || choice < 1 || choice > 4)
            {
                cin.clear();

                cin.ignore(1000, '\n');

                cout << "\nInvalid input!";
                cout << "\nPlease enter a number between 1 - 4.";

                cout << "\n\nEnter Choice : ";
            }

            switch (choice)
            {
            case 1:

                addProductToCart(con, cart);
                break;

            case 2:

                displayCart(cart);

                break;

            case 3:

                checkoutOrder(con, customerID, cart);
                return;

            case 4:

                cout << "\nOrder session cancelled.\n";

                return;
            }

        } while (true);
    }
    catch (sql::SQLException& e)
    {
        cout << "\nDatabase Error : "
            << e.what()
            << endl;
    }
}

//=====================================================
// View Orders
//=====================================================

void viewOrders(sql::Connection* con)
{
    clearScreen();

    displayHeader("ORDER LIST");

    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT * FROM ORDERS "
            "ORDER BY OrderDate DESC"
        );

        cout << left
            << setw(12) << "OrderID"
            << setw(15) << "Customer"
            << setw(20) << "Payment"
            << setw(15) << "Status"
            << setw(12) << "Total"
            << endl;

        cout << "--------------------------------------------------------------------------\n";

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

            cout << left
                << setw(12) << res->getString("OrderID")
                << setw(15) << res->getString("CustomerID")
                << setw(20) << res->getString("PaymentMethod")
                << setw(15) << status
                << fixed << setprecision(2)
                << res->getDouble("TotalAmount")
                << endl;
            totalOrders++;
        }

        cout << "--------------------------------------------------------------------------\n";

        cout << "\nTotal Orders : "
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


//=====================================================
// Search Order
//=====================================================

void searchOrder(sql::Connection* con)
{
    clearScreen();

    displayHeader("SEARCH ORDER");

    string orderID;

    cout << "\nEnter Order ID : ";
    cin >> orderID;

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

        cout << "\nOrder ID    : "
            << res->getString("OrderID") << endl;

        cout << "Customer ID : "
            << res->getString("CustomerID") << endl;

        cout << "Payment     : "
            << res->getString("PaymentMethod") << endl;

        cout << "Status      : "
            << res->getString("OrderStatus") << endl;

        string dateTime =
            res->getString("OrderDate");

        string orderDate =
            dateTime.substr(0, 10);

        string orderTime =
            dateTime.substr(11, 8);

        cout << "Order Date  : "
            << orderDate
            << endl;

        cout << "Order Time  : "
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

        cout << left
            << setw(12) << "ProductID"
            << setw(25) << "Product"
            << setw(10) << "Qty"
            << setw(12) << "Price"
            << setw(12) << "Subtotal"
            << endl;

        cout << "----------------------------------------------------------------\n";

        while (res->next())
        {
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

        cout << "----------------------------------------------------------------\n";

        cout << "\nGrand Total : RM "
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

    string orderID;

    cout << "\nEnter Order ID : ";
    cin >> orderID;

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        // Check Order
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

        string status =
            res->getString("OrderStatus");

        if (status == "CANCELLED")
        {
            displayError("Order already cancelled!");

            delete res;
            delete pstmt;

            return;
        }

        cout << "\nOrder ID    : "
             << res->getString("OrderID")
             << endl;

        cout << "Customer ID : "
             << res->getString("CustomerID")
             << endl;

        cout << "Status      : "
             << res->getString("OrderStatus")
             << endl;

        cout << "Total       : RM "
             << fixed << setprecision(2)
             << res->getDouble("TotalAmount")
             << endl;

        delete res;
        delete pstmt;

        int choice;

        cout << "\n1. Confirm Cancel\n";
        cout << "2. Back\n";

        cout << "\nEnter Choice : ";

        while (!(cin >> choice)
            || choice < 1
            || choice > 2)
        {
            cin.clear();

            cin.ignore(1000, '\n');

            cout << "\nInvalid choice!";
            cout << "\nEnter again : ";
        }

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

        cout << "\nProduct stock restored.\n";
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

    displayHeader("RESTORE ORDER");

    string orderID;

    cout << "\nEnter Order ID : ";
    cin >> orderID;

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        // Check Order
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

        string status =
            res->getString("OrderStatus");

        if (status != "CANCELLED")
        {
            displayError("Order is not cancelled!");

            delete res;
            delete pstmt;

            return;
        }

        cout << "\nOrder ID    : "
            << res->getString("OrderID")
            << endl;

        cout << "Customer ID : "
            << res->getString("CustomerID")
            << endl;

        cout << "Status      : "
            << res->getString("OrderStatus")
            << endl;

        cout << "Total       : RM "
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

        cout << "\n1. Confirm Restore\n";
        cout << "2. Back\n";

        cout << "\nEnter Choice : ";

        while (!(cin >> choice)
            || choice < 1
            || choice > 2)
        {
            cin.clear();

            cin.ignore(1000, '\n');

            cout << "\nInvalid choice!";
            cout << "\nEnter again : ";
        }

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