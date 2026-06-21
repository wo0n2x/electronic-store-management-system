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

namespace
{
    void printCustomerPrompt(string prompt)
    {
        margin();
        cout << prompt;
    }

    void printCustomerLine(int width)
    {
        printTableLine(width);
    }
}

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

        printCustomerLine(52);

        margin();
        setTableHeaderColor();
        cout << left
            << setw(12) << "ID"
            << setw(30) << "Customer Name"
            << endl;
        resetTextColor();

        printCustomerLine(52);

        while (res->next())
        {
            margin();

            cout << left
                << setw(10) << res->getString("CustomerID")
                << setw(25) << res->getString("CustomerName")
                << endl;
        }

        printCustomerLine(52);

        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
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

    displayHeader("ADD CUSTOMER");
    displayPath("HOME > CUSTOMER MANAGEMENT > ADD CUSTOMER");
    displaySection("CUSTOMER DETAILS");

    // Customer Name
    printCustomerPrompt("Enter Customer Name: ");
    getline(cin, name);

    while (!isValidName(name))
    {
        displayError("Name must contain letters only.");

        printCustomerPrompt("Enter Customer Name: ");
        getline(cin, name);
    }

    // Email
    printCustomerPrompt("Enter Email: ");
    getline(cin, email);

    while (!isValidEmail(email))
    {
        displayError("Invalid email format.");

        printCustomerPrompt("Enter Email: ");
        getline(cin, email);
    }

    // Contact Number
    printCustomerPrompt("Enter Contact Number: ");
    getline(cin, contact);

    while (!isValidContact(contact))
    {
        displayError("Contact number must contain numbers only.");

        printCustomerPrompt("Enter Contact Number: ");
        getline(cin, contact);
    }

    // Address
    printCustomerPrompt("Enter Address: ");
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

        displaySuccess("Customer added successfully.");
        margin();
        cout << "Customer ID : " << customerID << endl;

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}


// =====================================


// =====================================
// View Customers
// =====================================
void viewCustomers(sql::Connection* con)
{
    displayHeader("VIEW CUSTOMERS");
    displayPath("HOME > CUSTOMER MANAGEMENT > VIEW CUSTOMERS");

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

        printCustomerLine(103);

        margin();
        setTableHeaderColor();
        cout << left
            << setw(8) << "ID"
            << setw(25) << "Name"
            << setw(35) << "Email"
            << setw(15) << "Contact"
            << setw(20) << "Address"
            << endl;
        resetTextColor();

        printCustomerLine(103);

        int count = 0;

        while (res->next())
        {
            margin();
            cout << left
                << setw(8) << res->getString("CustomerID")
                << setw(25) << res->getString("CustomerName")
                << setw(35) << res->getString("Email")
                << setw(15) << res->getString("ContactNo")
                << setw(20) << res->getString("Address")
                << endl;

            count++;
        }

        printCustomerLine(103);

        margin();
        cout << "Total Customers : "
            << count
            << endl;

        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}
// =====================================
// Update Customer
// =====================================
void updateCustomer(sql::Connection* con)
{
    string customerID;

    displayHeader("UPDATE CUSTOMER");
    displayPath("HOME > CUSTOMER MANAGEMENT > UPDATE CUSTOMER");

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
            displayError("Customer not found.");

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

        displaySection("CURRENT CUSTOMER INFORMATION");

        margin();
        cout << left << setw(12) << "Name" << ": "
            << currentName << endl;

        margin();
        cout << left << setw(12) << "Email" << ": "
            << currentEmail << endl;

        margin();
        cout << left << setw(12) << "Contact" << ": "
            << currentContact << endl;

        margin();
        cout << left << setw(12) << "Address" << ": "
            << currentAddress << endl;

        delete res;
        delete pstmt;

        string contact, address;

        cin.ignore();

        cout << "\n";
        printCustomerPrompt("Customer Name : ");
        cout
            << currentName
            << " (READ ONLY)\n";

        printCustomerPrompt("Email         : ");
        cout
            << currentEmail
            << " (READ ONLY)\n";

        cout << "\n";
        printCustomerPrompt("Current Contact : ");
        cout
            << currentContact
            << endl;

        printCustomerPrompt("New Contact (Press Enter to keep current): ");
        getline(cin, contact);

        if (contact.empty())
        {
            contact = currentContact;
        }
        else
        {
            while (!isValidContact(contact))
            {
                displayError("Contact number must contain numbers only.");

                printCustomerPrompt("New Contact: ");
                getline(cin, contact);
            }
        }

        cout << "\n";
        printCustomerPrompt("Current Address : ");
        cout
            << currentAddress
            << endl;

        printCustomerPrompt("New Address (Press Enter to keep current): ");
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

        displaySuccess("Customer updated successfully.");

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
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
    displayPath("HOME > CUSTOMER MANAGEMENT > DELETE CUSTOMER");

    displayCustomerList(con);

    cout << "\n";
    printCustomerPrompt("Enter Customer ID: ");
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
            displayError("Customer not found.");

            delete res;
            delete pstmt;
            return;
        }

        delete res;
        delete pstmt;

        displayWarning("This customer will become inactive.");
        cout << "\n";
        printCustomerPrompt("Deactivate this customer? (Y/N): ");
        cin >> confirm;

        if (toupper(confirm) != 'Y')
        {
            displayInfo("Delete cancelled.");
            return;
        }

        pstmt = con->prepareStatement(
            "UPDATE CUSTOMER "
            "SET Status='INACTIVE' "
            "WHERE CustomerID=?"
        );

        pstmt->setString(1, customerID);
        pstmt->executeUpdate();

        displaySuccess("Customer deactivated successfully.");

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}

// =====================================
// Restore Customer
// =====================================
void restoreCustomer(sql::Connection* con)
{
    clearScreen();
    displayHeader("RESTORE CUSTOMER");
    displayPath("HOME > CUSTOMER MANAGEMENT > RESTORE CUSTOMER");
    string customerID;
    char confirm;

    cout << "\n";
    printCustomerPrompt("Enter Customer ID: ");
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
            displayError("Inactive customer not found.");

            delete res;
            delete pstmt;

            return;
        }

        displaySection("CUSTOMER DETAILS");

        margin();
        cout << left << setw(14) << "Customer ID" << ": "
            << res->getString("CustomerID")
            << endl;

        margin();
        cout << left << setw(14) << "Name" << ": "
            << res->getString("CustomerName")
            << endl;

        margin();
        cout << left << setw(14) << "Email" << ": "
            << res->getString("Email")
            << endl;

        margin();
        cout << left << setw(14) << "Contact No" << ": "
            << res->getString("ContactNo")
            << endl;

        margin();
        cout << left << setw(14) << "Address" << ": "
            << res->getString("Address")
            << endl;

        margin();
        cout << left << setw(14) << "Status" << ": ";
        printStatus(res->getString("Status"));
        cout << endl;
        delete res;
        delete pstmt;

        cout << "\n";
        printCustomerPrompt("Restore this customer? (Y/N): ");
        cin >> confirm;

        if (toupper(confirm) != 'Y')
        {
            displayInfo("Restore cancelled.");

            return;
        }

        pstmt = con->prepareStatement(
            "UPDATE CUSTOMER "
            "SET Status='ACTIVE' "
            "WHERE CustomerID=?"
        );

        pstmt->setString(1, customerID);

        pstmt->executeUpdate();

        displaySuccess("Customer restored successfully.");

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}

// =====================================
// Customer Management Menu
// =====================================
void customerManagementMenu(sql::Connection* con)
{
    int choice;

    do
    {
        clearScreen();

        displayHeader("CUSTOMER MANAGEMENT");
        displayPath("HOME > CUSTOMER MANAGEMENT");

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
        cout << "Enter Choice : ";
        cin >> choice;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(1000, '\n');

            displayError("Invalid choice.");

            pauseScreen();

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
            displayInfo("Returning to Main Menu.");

            return;


        default:
            displayError("Invalid choice.");
            pauseScreen();
        }

    } while (true);
}
