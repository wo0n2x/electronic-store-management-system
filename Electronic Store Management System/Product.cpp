#include "Product.h"
#include "Utility.h"

#include <iostream>
#include <string>
#include <iomanip>

#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>

using namespace std;
#define cout std::cout

void viewProducts(sql::Connection* con);
void searchProduct(sql::Connection* con);
void updateProduct(sql::Connection* con);
void deleteProduct(sql::Connection* con);
void restoreProduct(sql::Connection* con);

//generate productID
string generateProductID(sql::Connection* con)
{
    sql::Statement* stmt;
    sql::ResultSet* res;

    stmt = con->createStatement();

    res = stmt->executeQuery(
        "SELECT ProductID "
        "FROM PRODUCT "
        "ORDER BY ProductID DESC "
        "LIMIT 1"
    );

    string newID = "P001";

    if (res->next())
    {
        string lastID = res->getString("ProductID");

        int number = stoi(lastID.substr(1));

        number++;

        char buffer[10];

        sprintf_s(buffer, "P%03d", number);

        newID = buffer;
    }

    delete res;
    delete stmt;

    return newID;
}

// =====================================
// View Inactive Products
// =====================================

void viewInactiveProducts(sql::Connection* con)
{
    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT ProductID, ProductName "
            "FROM PRODUCT "
            "WHERE Status='INACTIVE' "
            "ORDER BY ProductID"
        );

        cout << "\n";

        margin();
        cout << "============================================================\n";

        margin();
        cout << "INACTIVE PRODUCT LIST\n";

        margin();
        cout << "============================================================\n";

        margin();
        cout << left
            << setw(15) << "PRODUCT ID"
            << setw(35) << "PRODUCT NAME"
            << endl;

        margin();
        cout << "============================================================\n";

        int count = 0;

        while (res->next())
        {
            margin();

            cout << left
                << setw(15) << res->getString("ProductID")
                << setw(35) << res->getString("ProductName")
                << endl;

            count++;
        }

        margin();
        cout << "============================================================\n";

        margin();
        cout << "Total Inactive Products : "
            << count
            << endl;

        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        margin();

        cout << "\n[ERROR] "
            << e.what()
            << endl;
    }
}

//display Categories
void displayCategories(sql::Connection* con)
{
    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT * FROM PRODUCTCATEGORY"
        );

        cout  << "\nAvailable Categories\n";
        cout << "--------------------\n";

        while (res->next())
        {
            cout
                << res->getInt("CategoryID")
                << ". "
                << res->getString("CategoryName")
                << endl;
        }

        delete res;
        delete stmt;
    }
    catch (...)
    {
        cout << "\nUnable to load categories.\n";
    }
}

// Add Product
void addProduct(sql::Connection* con)
{
    string productID = generateProductID(con);

    string productName;
    string brand;
    string description;

    double price;
    int stockQty;
    int categoryID;

    cin.ignore();

    cout << "\n========== ADD PRODUCT ==========\n";

    cout << "Product Name: ";
    getline(cin, productName);

    cout << "Brand: ";
    getline(cin, brand);

    cout << "Price: ";

    while (!(cin >> price) || price <= 0)
    {
        cin.clear();
        cin.ignore(1000, '\n');

        cout << "\nInvalid price!";
        cout << "\nPlease enter a valid number.";

        cout << "\n\nPrice: ";
    }

    cout << "Stock Quantity: ";

    while (!(cin >> stockQty) || stockQty < 0)
    {
        cin.clear();
        cin.ignore(1000, '\n');

        cout << "\nInvalid quantity!";
        cout << "\nPlease enter a valid number.";

        cout << "\n\nStock Quantity: ";
    }

    cin.ignore();

    cout << "Description: ";
    getline(cin, description);

    displayCategories(con);

    cout << "\nSelect Category ID: ";
    cin >> categoryID;

    bool validCategory = false;

    while (!validCategory)
    {
        sql::PreparedStatement* checkStmt;
        sql::ResultSet* checkRes;

        checkStmt = con->prepareStatement(
            "SELECT CategoryID "
            "FROM PRODUCTCATEGORY "
            "WHERE CategoryID = ?"
        );

        checkStmt->setInt(1, categoryID);

        checkRes = checkStmt->executeQuery();

        if (checkRes->next())
        {
            validCategory = true;
        }
        else
        {
            cout << "\nInvalid Category ID!";
            cout << "\nPlease select an existing category.";

            cout << "\n\nSelect Category ID: ";
            cin >> categoryID;
        }

        delete checkRes;
        delete checkStmt;
    }
    try
    {
        sql::PreparedStatement* pstmt;

        pstmt = con->prepareStatement(
            "INSERT INTO PRODUCT "
            "(ProductID, ProductName, Brand, Price, "
            "StockQuantity, ProductDescription, CategoryID, Status) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
        );

        pstmt->setString(1, productID);
        pstmt->setString(2, productName);
        pstmt->setString(3, brand);
        pstmt->setDouble(4, price);
        pstmt->setInt(5, stockQty);
        pstmt->setString(6, description);
        pstmt->setInt(7, categoryID);
        pstmt->setString(8, "ACTIVE");

        pstmt->executeUpdate();

        cout << "\nProduct added successfully!\n";
        cout << "Product ID : " << productID << endl;

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nError: " << e.what() << endl;
    }
}

// update Product
void updateProduct(sql::Connection* con)
{
    string productID;

    cout << "\n==================================================\n";
    cout << "                UPDATE PRODUCT\n";
    cout << "==================================================\n";

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
            cout << "\nProduct not found!\n";

            delete res;
            delete pstmt;

            return;
        }

        string productName =
            res->getString("ProductName");

        string brand =
            res->getString("Brand");

        double price =
            res->getDouble("Price");

        int stockQty =
            res->getInt("StockQuantity");

        string description =
            res->getString("ProductDescription");

        int categoryID =
            res->getInt("CategoryID");

        // Backup old values
        string oldName = productName;
        string oldBrand = brand;
        double oldPrice = price;
        int oldStock = stockQty;
        string oldDescription = description;
        int oldCategory = categoryID;

        delete res;
        delete pstmt;

        int choice;

        do
        {
            clearScreen();

            cout << "==================================================\n";
            cout << "              PRODUCT CONTROL PANEL\n";
            cout << "==================================================\n\n";

            cout << " Product ID     : "
                << productID << endl;

            cout << " Product Name   : "
                << productName << endl;

            cout << " Brand          : "
                << brand << endl;

            cout << " Price          : RM "
                << fixed << setprecision(2)
                << price << endl;

            cout << " Stock Quantity : "
                << stockQty << endl;

            cout << " Category ID    : "
                << categoryID << endl;

            cout << " Description    : "
                << description << endl;

            cout << "\n==================================================\n";

            cout << "\nWhat would you like to update?\n\n";

            cout << "1. Product Name\n";
            cout << "2. Brand\n";
            cout << "3. Price\n";
            cout << "4. Stock Quantity\n";
            cout << "5. Category\n";
            cout << "6. Description\n";
            cout << "7. Save Changes\n";
            cout << "8. Cancel\n";

            cout << "\nSelect Option : ";

            while (!(cin >> choice) || choice < 1 || choice > 8)
            {
                cin.clear();
                cin.ignore(1000, '\n');

                cout << "\nInvalid option!";
                cout << "\nPlease enter a number between 1 - 8.";

                cout << "\n\nSelect Option : ";
            }

            cin.ignore(1000, '\n');

            switch (choice)
            {
            case 1:

                cout << "\nNew Product Name : ";
                getline(cin, productName);

                break;

            case 2:

                cout << "\nNew Brand : ";
                getline(cin, brand);

                break;

            case 3:

                cout << "\nNew Price : ";
                cin >> price;

                break;

            case 4:

                cout << "\nNew Stock Quantity : ";
                cin >> stockQty;

                break;

            case 5:

                displayCategories(con);

                cout << "\nSelect New Category ID : ";
                cin >> categoryID;

                break;

            case 6:

                cout << "\nNew Description : ";
                getline(cin, description);

                break;

            case 7:

                clearScreen();

                cout << "==============================================================\n";
                cout << "                    PRODUCT CHANGES\n";
                cout << "==============================================================\n\n";

                cout << left
                    << setw(20) << "FIELD"
                    << setw(25) << "OLD VALUE"
                    << setw(25) << "NEW VALUE"
                    << endl;

                cout << "--------------------------------------------------------------\n";

                if (oldName != productName)
                {
                    cout << left
                        << setw(20) << "Name"
                        << setw(25) << oldName
                        << setw(25) << productName
                        << endl;
                }

                if (oldBrand != brand)
                {
                    cout << left
                        << setw(20) << "Brand"
                        << setw(25) << oldBrand
                        << setw(25) << brand
                        << endl;
                }

                if (oldPrice != price)
                {
                    cout << left
                        << setw(20) << "Price"
                        << setw(25) << oldPrice
                        << setw(25) << price
                        << endl;
                }

                if (oldStock != stockQty)
                {
                    cout << left
                        << setw(20) << "Stock Qty"
                        << setw(25) << oldStock
                        << setw(25) << stockQty
                        << endl;
                }

                if (oldCategory != categoryID)
                {
                    cout << left
                        << setw(20) << "Category ID"
                        << setw(25) << oldCategory
                        << setw(25) << categoryID
                        << endl;
                }

                if (oldDescription != description)
                {
                    cout << left
                        << setw(20) << "Description"
                        << setw(25) << oldDescription
                        << setw(25) << description
                        << endl;
                }

                cout << "\n==============================================================\n";

                char confirm;

                cout << "\nSave Changes? (Y/N): ";
                cin >> confirm;

                if (toupper(confirm) == 'Y')
                {
                    pstmt = con->prepareStatement(
                        "UPDATE PRODUCT "
                        "SET ProductName=?, "
                        "Brand=?, "
                        "Price=?, "
                        "StockQuantity=?, "
                        "ProductDescription=?, "
                        "CategoryID=? "
                        "WHERE ProductID=?"
                    );

                    pstmt->setString(1, productName);
                    pstmt->setString(2, brand);
                    pstmt->setDouble(3, price);
                    pstmt->setInt(4, stockQty);
                    pstmt->setString(5, description);
                    pstmt->setInt(6, categoryID);
                    pstmt->setString(7, productID);

                    pstmt->executeUpdate();

                    cout << "\n\n========================================\n";
                    cout << "   PRODUCT UPDATED SUCCESSFULLY\n";
                    cout << "========================================\n";
                    delete pstmt;
                }
                else
                {
                    cout << "\nChanges discarded.\n";
                }

                pauseScreen();
                return;

            case 8:

                cout << "\nUpdate cancelled.\n";
                return;

            default:

                cout << "\nInvalid option!\n";
                pauseScreen();
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

//product Management Menu
void productManagementMenu(sql::Connection* con)
{
    int choice;

    do
    {
        clearScreen();

        cout << "========================================\n";
        cout << "         PRODUCT MANAGEMENT\n";
        cout << "========================================\n";

        cout << "1. Add Product\n";
        cout << "2. Update Product\n";
        cout << "3. Delete Product\n";
        cout << "4. Restore Product\n";
        cout << "5. Search Product\n";
        cout << "6. View Products\n";
        cout << "7. Back\n";

        cout << "\nEnter Choice: ";

        while (!(cin >> choice) || choice < 1 || choice > 7)
        {
            cin.clear();
            cin.ignore(1000, '\n');

            cout << "\nInvalid input!";
            cout << "\nPlease enter a number between 1 - 7.";

            cout << "\n\nEnter Choice: ";
        }

        switch (choice)
        {
        case 1:

            clearScreen();

            addProduct(con);

            pauseScreen();

            break;

        case 2:

            clearScreen();

            updateProduct(con);

            pauseScreen();

            break;

        case 3:

            clearScreen();

            deleteProduct(con);

            pauseScreen();

            break;

        case 4:

            clearScreen();

            restoreProduct(con);

            pauseScreen();

            break;

        case 5:

            clearScreen();

            searchProduct(con);

            pauseScreen();

            break;

        case 6:

            clearScreen();

            viewProducts(con);

            pauseScreen();

            break;

        case 7:

            return;

        default:

            cout << "\nInvalid option!\n";
            pauseScreen();
        }

    } while (true);
}

//Delete Products
void deleteProduct(sql::Connection* con)
{
    string productID;

    displayHeader("DELETE PRODUCT");
    displayPath("HOME > PRODUCT MANAGEMENT > DELETE PRODUCT");

    viewProducts(con);

    cout << "\n";

    margin();
    cout << "Enter Product ID : ";

    cin >> productID;
    viewProducts(con);
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
            cout << "\nProduct not found or already inactive!\n";

            delete res;
            delete pstmt;

            return;
        }

        string productName =
            res->getString("ProductName");

        string brand =
            res->getString("Brand");

        double price =
            res->getDouble("Price");

        int stockQty =
            res->getInt("StockQuantity");

        int categoryID =
            res->getInt("CategoryID");

        delete res;
        delete pstmt;

        clearScreen();

        cout << "==================================================\n";
        cout << "                DELETE PRODUCT\n";
        cout << "==================================================\n\n";

        margin();
        cout << "Product ID     : "
            << productID
            << endl;

        margin();
        cout << "Product Name   : "
            << productName
            << endl;

        margin();
        cout << "Brand          : "
            << brand
            << endl;

        margin();
        cout << "Price          : RM "
            << fixed << setprecision(2)
            << price
            << endl;

        margin();
        cout << "Stock Quantity : "
            << stockQty
            << endl;

        margin();
        cout << "Category ID    : "
            << categoryID
            << endl;
        cout << "\n==================================================\n";

        cout << "\n";

        margin();
        cout << "[WARNING] This product will become INACTIVE.\n";

        margin();
        cout << "It will no longer appear in active products.\n";
        char confirm;

        margin();
        cout << "\nDeactivate this product? (Y/N): ";
        cin >> confirm;

        if (toupper(confirm) != 'Y')
        {
            margin();

            cout << "\nOperation cancelled.\n";
            return;
        }

        pstmt = con->prepareStatement(
            "UPDATE PRODUCT "
            "SET Status='INACTIVE' "
            "WHERE ProductID=?"
        );

        pstmt->setString(1, productID);

        pstmt->executeUpdate();

        delete pstmt;

        cout << "\n========================================\n";
        cout << "      PRODUCT DEACTIVATED\n";
        cout << "========================================\n";
    }
    catch (sql::SQLException& e)
    {
        cout << "\nDatabase Error : "
            << e.what()
            << endl;
    }
}

// =====================================
// Restore Product
// =====================================

void restoreProduct(sql::Connection* con)
{
    string productID;

    displayHeader("RESTORE PRODUCT");
    displayPath("HOME > PRODUCT MANAGEMENT > RESTORE PRODUCT");

    viewInactiveProducts(con);

    cout << "\n";

    margin();
    cout << "Enter Product ID : ";
    cin >> productID;

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM PRODUCT "
            "WHERE ProductID=? "
            "AND Status='INACTIVE'"
        );

        pstmt->setString(1, productID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            margin();
            cout << "[ERROR] Inactive product not found!\n";

            delete res;
            delete pstmt;

            return;
        }

        string productName =
            res->getString("ProductName");

        string brand =
            res->getString("Brand");

        double price =
            res->getDouble("Price");

        int stockQty =
            res->getInt("StockQuantity");

        int categoryID =
            res->getInt("CategoryID");

        delete res;
        delete pstmt;

        clearScreen();

        displayHeader("PRODUCT RECOVERY");
        displayPath("HOME > PRODUCT MANAGEMENT > RESTORE PRODUCT");

        cout << "\n";

        margin();
        cout << "============================================================\n";

        margin();
        cout << "Product Information\n";

        margin();
        cout << "============================================================\n";

        margin();
        cout << "Product ID     : "
            << productID
            << endl;

        margin();
        cout << "Product Name   : "
            << productName
            << endl;

        margin();
        cout << "Brand          : "
            << brand
            << endl;

        margin();
        cout << "Price          : RM "
            << fixed << setprecision(2)
            << price
            << endl;

        margin();
        cout << "Stock Quantity : "
            << stockQty
            << endl;

        margin();
        cout << "Category ID    : "
            << categoryID
            << endl;

        margin();
        cout << "============================================================\n";

        char confirm;

        cout << "\n";

        margin();
        cout << "Restore this product? (Y/N): ";
        cin >> confirm;

        if (toupper(confirm) != 'Y')
        {
            margin();
            cout << "\n[INFO] Operation cancelled.\n";

            return;
        }

        pstmt = con->prepareStatement(
            "UPDATE PRODUCT "
            "SET Status='ACTIVE' "
            "WHERE ProductID=?"
        );

        pstmt->setString(1, productID);

        pstmt->executeUpdate();

        delete pstmt;

        cout << "\n";

        margin();
        cout << "[SUCCESS] Product restored successfully!\n";
    }
    catch (sql::SQLException& e)
    {
        margin();

        cout << "\n[ERROR] "
            << e.what()
            << endl;
    }
}
//search Products
void searchProduct(sql::Connection* con)
{
    string productID;

    cout << "\n========== SEARCH PRODUCT ==========\n";

    cout << "Enter Product ID : ";
    cin >> productID;

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT P.ProductID, "
            "P.ProductName, "
            "P.Brand, "
            "P.Price, "
            "P.StockQuantity, "
            "P.ProductDescription, "
            "C.CategoryName "
            "FROM PRODUCT P "
            "JOIN PRODUCTCATEGORY C "
            "ON P.CategoryID = C.CategoryID "
            "WHERE P.ProductID = ? "
            "AND P.Status='ACTIVE'"
        );

        pstmt->setString(1, productID);

        res = pstmt->executeQuery();

        if (res->next())
        {
            cout << "\n========================================\n";
            cout << "         PRODUCT INFORMATION\n";
            cout << "========================================\n";

            cout << "Product ID   : "
                << res->getString("ProductID") << endl;

            cout << "Name         : "
                << res->getString("ProductName") << endl;

            cout << "Brand        : "
                << res->getString("Brand") << endl;

            cout << "Price        : "
                << res->getDouble("Price") << endl;

            cout << "Stock Qty    : "
                << res->getInt("StockQuantity") << endl;

            cout << "Category     : "
                << res->getString("CategoryName") << endl;

            cout << "Description  : "
                << res->getString("ProductDescription") << endl;
        }
        else
        {
            cout << "\nProduct not found!\n";
        }

        delete res;
        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nError: " << e.what() << endl;
    }
}

//view Products
void viewProducts(sql::Connection* con)
{
    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT P.ProductID, "
            "P.ProductName, "
            "P.Brand, "
            "P.Price, "
            "P.StockQuantity, "
            "C.CategoryName "
            "FROM PRODUCT P "
            "JOIN PRODUCTCATEGORY C "
            "ON P.CategoryID = C.CategoryID "
            "WHERE P.Status='ACTIVE'"
        );

        cout << "\n==============================================================================================\n";
        cout << left
            << setw(8) << "ID"
            << setw(30) << "Product Name"
            << setw(15) << "Brand"
            << setw(12) << "Price"
            << setw(10) << "Stock"
            << setw(15) << "Category"
            << endl;

        cout << "==============================================================================================\n";

        while (res->next())
        {
            cout << left
                << setw(8) << res->getString("ProductID")
                << setw(30) << res->getString("ProductName")
                << setw(15) << res->getString("Brand")
                << setw(12)
                << fixed << setprecision(2)
                << res->getDouble("Price")
                << setw(10) << res->getInt("StockQuantity")
                << setw(15) << res->getString("CategoryName")
                << endl;
        }

        cout << "==============================================================================================\n";

        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nError: " << e.what() << endl;
    }
}