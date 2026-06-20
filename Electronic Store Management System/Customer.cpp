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
// display Customer List
// =====================================

void displayCustomerList(sql::Connection* con)
{
    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT CustomerID, CustomerName "
            "FROM CUSTOMER "
            "WHERE Status='ACTIVE' "
            "ORDER BY CustomerID"
        );

        cout << "\n";

        margin();
        cout << "====================================================\n";

        margin();
        cout << left
            << setw(12) << "ID"
            << setw(30) << "Customer Name"
            << endl;

        margin();
        cout << "====================================================\n";

        while (res->next())
        {
            margin();

            cout << left
                << setw(10) << res->getString("CustomerID")
                << setw(25) << res->getString("CustomerName")
                << endl;
        }

        margin();
        cout << "====================================================\n";

        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nError: " << e.what() << endl;
    }
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
}


// =====================================


// =====================================
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
            "SELECT * FROM CUSTOMER "
            "WHERE Status='ACTIVE' "
            "ORDER BY CustomerID"
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

        cout << "Total Customers: "
            << count
            << endl;

        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nError: "
            << e.what()
            << endl;
    }
}
// =====================================
// Update Customer
// =====================================
void updateCustomer(sql::Connection* con)
{
    string customerID;

    displayHeader("UPDATE CUSTOMER");

    displayCustomerList(con);

    cout << "\n";
    margin();
    cout << "Enter Customer ID (CXXX): ";
    cin >> customerID;

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
            cout << "\nCustomer not found!\n";

            delete res;
            delete pstmt;

            return;
        }

        string currentName =
            res->getString("CustomerName");

        string currentEmail =
            res->getString("Email");

        string currentContact =
            res->getString("ContactNo");

        string currentAddress =
            res->getString("Address");

        cout << "\nCurrent Customer Information\n\n";

        cout << "Name    : "
            << currentName << endl;

        cout << "Email   : "
            << currentEmail << endl;

        cout << "Contact : "
            << currentContact << endl;

        cout << "Address : "
            << currentAddress << endl;

        delete res;
        delete pstmt;

        string contact, address;

        cin.ignore();

        cout << "\nCustomer Name : "
            << currentName
            << " (READ ONLY)\n";

        cout << "Email         : "
            << currentEmail
            << " (READ ONLY)\n";

        cout << "\nCurrent Contact : "
            << currentContact
            << endl;

        cout << "New Contact (Press Enter to keep current): ";
        getline(cin, contact);

        if (contact.empty())
        {
            contact = currentContact;
        }
        else
        {
            while (!isValidContact(contact))
            {
                cout << "\nInvalid contact number!";
                cout << "\nNumbers only.\n";

                cout << "New Contact: ";
                getline(cin, contact);
            }
        }

        cout << "\nCurrent Address : "
            << currentAddress
            << endl;

        cout << "New Address (Press Enter to keep current): ";
        getline(cin, address);

        if (address.empty())
        {
            address = currentAddress;
        }
        pstmt = con->prepareStatement(
            "UPDATE CUSTOMER "
            "SET ContactNo=?, "
            "Address=? "
            "WHERE CustomerID=?"
        );

        pstmt->setString(1, contact);
        pstmt->setString(2, address);
        pstmt->setString(3, customerID);

        pstmt->executeUpdate();

        cout << "\nCustomer updated successfully!\n";

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nError: "
            << e.what()
            << endl;
    }
}
// =====================================
// Delete Customer
// =====================================
void deleteCustomer(sql::Connection* con)
{
    string customerID;
    char confirm;

    displayHeader("DELETE CUSTOMER");

    displayCustomerList(con);

    cout << "\nEnter Customer ID: ";
    cin >> customerID;

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM CUSTOMER "
            "WHERE CustomerID=? AND Status='ACTIVE'"
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
            "UPDATE CUSTOMER "
            "SET Status='INACTIVE' "
            "WHERE CustomerID=?"
        );

        pstmt->setString(1, customerID);
        pstmt->executeUpdate();

        cout << "\nCustomer deactivated successfully!\n";

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nError: " << e.what() << endl;
    }
}

// =====================================
// Restore Customer
// =====================================
void restoreCustomer(sql::Connection* con)
{
    clearScreen();
    displayHeader("RESTORE CUSTOMER");
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
            "WHERE CustomerID=? "
            "AND Status='INACTIVE'"
        );

        pstmt->setString(1, customerID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            cout << "\nInactive customer not found!\n";

            delete res;
            delete pstmt;

            return;
        }

        cout << "\n--------------------------------------------------\n";

        cout << "Customer ID : "
            << res->getString("CustomerID")
            << endl;

        cout << "Name        : "
            << res->getString("CustomerName")
            << endl;

        cout << "Email       : "
            << res->getString("Email")
            << endl;

        cout << "Contact No  : "
            << res->getString("ContactNo")
            << endl;

        cout << "Address     : "
            << res->getString("Address")
            << endl;

        cout << "Status      : "
            << res->getString("Status")
            << endl;

        cout << "--------------------------------------------------\n";
        delete res;
        delete pstmt;

        cout << "\n\nRestore this customer? (Y/N): ";
        cin >> confirm;

        if (toupper(confirm) != 'Y')
        {
            cout << "\nRestore cancelled.\n";

            return;
        }

        pstmt = con->prepareStatement(
            "UPDATE CUSTOMER "
            "SET Status='ACTIVE' "
            "WHERE CustomerID=?"
        );

        pstmt->setString(1, customerID);

        pstmt->executeUpdate();

        cout << "\nCustomer restored successfully!\n";

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nError: "
            << e.what()
            << endl;
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
        margin();
        cout << "========================================\n";
        margin();
        cout << "         CUSTOMER MANAGEMENT\n";
        margin();
        cout << "========================================\n";
        margin();
        cout << "1. Add Customer\n";
        margin();
        cout << "2. Update Customer\n";
        margin();
        cout << "3. Delete Customer\n";
        margin();
        cout << "4. Restore Customer\n";
        margin();
        cout << "5. View Customer\n";
		margin();
        cout << "6. Back\n\n";
        margin();
        cout << "Enter Choice:\n ";
        cin >> choice;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(1000, '\n');

            margin();
            cout << "Invalid choice!\n";

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

            restoreCustomer(con);

            break;

        case 5:

            clearScreen();

            viewCustomers(con);

            pauseScreen();

            clearScreen();

            break;

        case 6:
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