#include "Customer.h"
#include "Utility.h"
#include <iostream>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>
#include <regex>
#include <cctype>
#include <iomanip>

using namespace std;

string generateCustomerID(sql::Connection* con)
{
    sql::Statement* stmt;
    sql::ResultSet* res;

    stmt = con->createStatement();

    res = stmt->executeQuery(
        "SELECT CustomerID "
        "FROM CUSTOMER "
        "ORDER BY CustomerID DESC "
        "LIMIT 1"
    );

    string newID = "C001";

    if (res->next())
    {
        string lastID = res->getString("CustomerID");

        int number = stoi(lastID.substr(1));

        number++;

        char buffer[10];

        sprintf_s(buffer, "C%03d", number);

        newID = buffer;
    }

    delete res;
    delete stmt;

    return newID;
}

// =====================================
// Validation Functions
// =====================================

bool isValidName(string name)
{
    regex pattern("^[A-Za-z ]+$");
    return regex_match(name, pattern);
}

bool isValidEmail(string email)
{
    regex pattern(
        R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)"
    );

    return regex_match(email, pattern);
}

bool isValidContact(string contact)
{
    regex pattern("^[0-9]+$");
    return regex_match(contact, pattern);
}
// =====================================
// Add Customer
// =====================================



void addCustomer(sql::Connection* con)
{
    string customerID = generateCustomerID(con);

    string name, email, contact, address;

    cin.ignore();

    // Customer Name
    cout << "\nEnter Customer Name: ";
    getline(cin, name);

    while (!isValidName(name))
    {
        cout << "\nInvalid name!";
        cout << "\nName must contain letters only.\n";

        cout << "Enter Customer Name: ";
        getline(cin, name);
    }

    // Email
    cout << "Enter Email: ";
    getline(cin, email);

    while (!isValidEmail(email))
    {
        cout << "\nInvalid email format!\n";

        cout << "Enter Email: ";
        getline(cin, email);
    }

    // Contact Number
    cout << "Enter Contact Number: ";
    getline(cin, contact);

    while (!isValidContact(contact))
    {
        cout << "\nInvalid contact number!";
        cout << "\nNumbers only.\n";

        cout << "Enter Contact Number: ";
        getline(cin, contact);
    }

    // Address
    cout << "Enter Address: ";
    getline(cin, address);

    try
    {
        sql::PreparedStatement* pstmt;

        pstmt = con->prepareStatement(
            "INSERT INTO CUSTOMER "
            "(CustomerID, CustomerName, Email, ContactNo, Address) "
            "VALUES (?, ?, ?, ?, ?)"
        );

        pstmt->setString(1, customerID);
        pstmt->setString(2, name);
        pstmt->setString(3, email);
        pstmt->setString(4, contact);
        pstmt->setString(5, address);

        pstmt->executeUpdate();

        cout << "\nCustomer added successfully!\n";
        cout << "Customer ID : " << customerID << endl;

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nError: " << e.what() << endl;
    }
}// =====================================
// View Customers
// =====================================
void viewCustomers(sql::Connection* con)
{
    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT * FROM CUSTOMER"
        );

        cout << "\n========================================================================================================\n";
        cout << left
            << setw(8) << "ID"
            << setw(25) << "Name"
            << setw(35) << "Email"
            << setw(15) << "Contact"
            << setw(20) << "Address"
            << endl;

        cout << "========================================================================================================\n";

        int count = 0;

        while (res->next())
        {
            cout << left
                << setw(8) << res->getString("CustomerID")
                << setw(25) << res->getString("CustomerName")
                << setw(35) << res->getString("Email")
                << setw(15) << res->getString("ContactNo")
                << setw(20) << res->getString("Address")
                << endl;

            count++;
        }

        cout << "========================================================================================================\n";
        cout << "Total Customers: " << count << endl;

        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nError: " << e.what() << endl;
    }
}
// =====================================
// Update Customer
// =====================================
void updateCustomer(sql::Connection* con)
{
    string customerID;

    cout << "\nEnter Customer ID (C001): ";
    cin >> customerID;

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM CUSTOMER WHERE CustomerID=?"
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

        cout << "\nCurrent Customer Information\n";

        cout << "Name    : "
            << res->getString("CustomerName") << endl;

        cout << "Email   : "
            << res->getString("Email") << endl;

        cout << "Contact : "
            << res->getString("ContactNo") << endl;

        cout << "Address : "
            << res->getString("Address") << endl;

        delete res;
        delete pstmt;

        string name, email, contact, address;

        cin.ignore();

        // New Name
        cout << "\nEnter New Name: ";
        getline(cin, name);

        while (!isValidName(name))
        {
            cout << "\nInvalid name!";
            cout << "\nName must contain letters only.\n";

            cout << "Enter New Name: ";
            getline(cin, name);
        }

        // New Email
        cout << "Enter New Email: ";
        getline(cin, email);

        while (!isValidEmail(email))
        {
            cout << "\nInvalid email format!\n";

            cout << "Enter New Email: ";
            getline(cin, email);
        }

        // New Contact
        cout << "Enter New Contact: ";
        getline(cin, contact);

        while (!isValidContact(contact))
        {
            cout << "\nInvalid contact number!";
            cout << "\nNumbers only.\n";

            cout << "Enter New Contact: ";
            getline(cin, contact);
        }

        // New Address
        cout << "Enter New Address: ";
        getline(cin, address);

        pstmt = con->prepareStatement(
            "UPDATE CUSTOMER "
            "SET CustomerName=?, "
            "Email=?, "
            "ContactNo=?, "
            "Address=? "
            "WHERE CustomerID=?"
        );

        pstmt->setString(1, name);
        pstmt->setString(2, email);
        pstmt->setString(3, contact);
        pstmt->setString(4, address);
        pstmt->setString(5, customerID);

        pstmt->executeUpdate();

        cout << "\nCustomer updated successfully!\n";

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nError: " << e.what() << endl;
    }
}
// =====================================
// Delete Customer
// =====================================
void deleteCustomer(sql::Connection* con)
{
    string customerID;
    char confirm;

    cout << "\nEnter Customer ID: ";
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

        delete res;
        delete pstmt;

        cout << "\nAre you sure you want to delete this customer? (Y/N): ";
        cin >> confirm;

        if (toupper(confirm) != 'Y')
        {
            cout << "\nDelete cancelled.\n";
            return;
        }

        pstmt = con->prepareStatement(
            "DELETE FROM CUSTOMER "
            "WHERE CustomerID=?"
        );

        pstmt->setString(1, customerID);
        pstmt->executeUpdate();

        cout << "\nCustomer deleted successfully!\n";

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nError: " << e.what() << endl;
    }
}

// =====================================
// Customer Management Menu
// =====================================
void customerManagementMenu(sql::Connection* con)
{
    clearScreen();
    int choice;

    do
    {
        cout << "========================================\n";
        cout << "         CUSTOMER MANAGEMENT\n";
        cout << "========================================\n";
        cout << "1. Add Customer\n";
        cout << "2. Update Customer\n";
        cout << "3. Delete Customer\n";
        cout << "4. View Customer\n";
        cout << "5. Back\n";

        cout << "\nEnter Choice: ";
        cin >> choice;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(1000, '\n');

            cout << "\nInvalid choice!\n";

            pauseScreen();
            clearScreen();

            continue;
        }
        switch (choice)
        {
        case 1:

            clearScreen();

            addCustomer(con);

            pauseScreen();

            clearScreen();

            break;

        case 2:

            clearScreen();

            updateCustomer(con);

            pauseScreen();

            clearScreen();

            break;

        case 3:

            clearScreen();

            deleteCustomer(con);

            pauseScreen();

            clearScreen();

            break;

        case 4:

            clearScreen();

            viewCustomers(con);

            pauseScreen();

            clearScreen();

            break;

        case 5:
            cout << "\nReturning to Main Menu...\n";
            loadingScreen();

            clearScreen();

            return;
            break;

        default:
            cout << "\nInvalid choice!\n";
        }

    } while (choice != 5);
}