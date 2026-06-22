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
#include <memory>

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

    void printCustomerStatusField(string status, int width)
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

    string trimInput(string value)
    {
        size_t first =
            value.find_first_not_of(" \t\n\r\f\v");

        if (first == string::npos)
        {
            return "";
        }

        size_t last =
            value.find_last_not_of(" \t\n\r\f\v");

        return value.substr(first, last - first + 1);
    }

    bool customerValueExists(
        sql::Connection* con,
        string columnName,
        string value,
        string excludedCustomerID = ""
    )
    {
        string query =
            "SELECT CustomerID "
            "FROM CUSTOMER "
            "WHERE " + columnName + "=? ";

        if (!excludedCustomerID.empty())
        {
            query += "AND CustomerID<>? ";
        }

        query += "LIMIT 1";

        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(query)
        );

        pstmt->setString(1, value);

        if (!excludedCustomerID.empty())
        {
            pstmt->setString(2, excludedCustomerID);
        }

        unique_ptr<sql::ResultSet> res(
            pstmt->executeQuery()
        );

        return res->next();
    }

    bool customerEmailExists(sql::Connection* con, string email)
    {
        return customerValueExists(con, "Email", email);
    }

    bool customerContactExists(
        sql::Connection* con,
        string contact,
        string excludedCustomerID = ""
    )
    {
        return customerValueExists(
            con,
            "ContactNo",
            contact,
            excludedCustomerID
        );
    }

    bool isValidCustomerID(string customerID)
    {
        regex pattern("^C[0-9]{3}$");

        return regex_match(customerID, pattern);
    }

    void displayInactiveCustomerList(sql::Connection* con)
    {
        sql::Statement* stmt = nullptr;
        sql::ResultSet* res = nullptr;

        try
        {
            stmt = con->createStatement();

            res = stmt->executeQuery(
                "SELECT CustomerID, CustomerName, Status "
                "FROM CUSTOMER "
                "WHERE Status='INACTIVE' "
                "ORDER BY CustomerID"
            );

            cout << "\n";
            displaySection("INACTIVE CUSTOMERS");

            margin();
            setTableHeaderColor();
            cout << left
                << setw(15) << "Customer ID"
                << setw(30) << "Customer Name"
                << setw(15) << "Status"
                << endl;
            resetTextColor();

            printCustomerLine(60);

            int count = 0;

            while (res->next())
            {
                margin();

                cout << left
                    << setw(15) << res->getString("CustomerID")
                    << setw(30) << res->getString("CustomerName");
                printCustomerStatusField(res->getString("Status"), 15);
                cout << endl;

                count++;
            }

            printCustomerLine(60);

            if (count == 0)
            {
                displayInfo("No inactive customers available to restore.");
            }

            delete res;
            delete stmt;
        }
        catch (sql::SQLException& e)
        {
            delete res;
            delete stmt;

            displayError(e.what());
        }
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
    name = trimInput(name);

    regex pattern("^[A-Za-z ]+$");
    return !name.empty() &&
        regex_match(name, pattern);
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
            "SELECT CustomerID, CustomerName, Status "
            "FROM CUSTOMER "
            "WHERE Status='ACTIVE' "
            "ORDER BY CustomerID"
        );

        cout << "\n";

        displaySection("ACTIVE CUSTOMERS");

        margin();
        setTableHeaderColor();
        cout << left
            << setw(15) << "Customer ID"
            << setw(30) << "Customer Name"
            << setw(15) << "Status"
            << endl;
        resetTextColor();

        printCustomerLine(60);

        while (res->next())
        {
            margin();

            cout << left
                << setw(15) << res->getString("CustomerID")
                << setw(30) << res->getString("CustomerName");
            printCustomerStatusField(res->getString("Status"), 15);
            cout << endl;
        }

        printCustomerLine(60);

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

    try
    {
        // Customer Name
        printCustomerPrompt("Enter Customer Name: ");
        getline(cin, name);
        name = trimInput(name);

        while (!isValidName(name))
        {
            displayError("Name must contain letters only.");

            printCustomerPrompt("Enter Customer Name: ");
            getline(cin, name);
            name = trimInput(name);
        }

        // Email
        printCustomerPrompt("Enter Email: ");
        getline(cin, email);
        email = trimInput(email);

        while (!isValidEmail(email) ||
            customerEmailExists(con, email))
        {
            if (!isValidEmail(email))
            {
                displayError("Invalid email format.");
            }
            else
            {
                displayError(
                    "Email already exists. Please enter a different email."
                );
            }

            printCustomerPrompt("Enter Email: ");
            getline(cin, email);
            email = trimInput(email);
        }

        // Contact Number
        printCustomerPrompt("Enter Contact Number: ");
        getline(cin, contact);
        contact = trimInput(contact);

        while (!isValidContact(contact) ||
            customerContactExists(con, contact))
        {
            if (!isValidContact(contact))
            {
                displayError("Contact number must contain numbers only.");
            }
            else
            {
                displayError(
                    "Contact number already exists. Please enter a different contact number."
                );
            }

            printCustomerPrompt("Enter Contact Number: ");
            getline(cin, contact);
            contact = trimInput(contact);
        }

        // Address
        printCustomerPrompt("Enter Address: ");
        getline(cin, address);

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
    cout << "Enter Customer ID (0 to cancel): ";
    cin >> customerID;

    if (customerID == "0")
    {
        displayInfo("Update cancelled.");
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
        contact = trimInput(contact);

        if (contact.empty())
        {
            contact = currentContact;
        }
        else
        {
            while (!isValidContact(contact) ||
                customerContactExists(con, contact, customerID))
            {
                if (!isValidContact(contact))
                {
                    displayError("Contact number must contain numbers only.");
                }
                else
                {
                    displayError(
                        "Contact number already exists. Please enter a different contact number."
                    );
                }

                printCustomerPrompt("New Contact: ");
                getline(cin, contact);
                contact = trimInput(contact);
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
    printCustomerPrompt("Enter Customer ID (0 to cancel): ");
    cin >> customerID;

    if (customerID == "0")
    {
        displayInfo("Delete cancelled.");
        return;
    }

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
    string customerID;
    char confirm;

    while (true)
    {
        clearScreen();
        displayHeader("RESTORE CUSTOMER");
        displayPath("HOME > CUSTOMER MANAGEMENT > RESTORE CUSTOMER");

        displayInactiveCustomerList(con);

        cout << "\n";
        printCustomerPrompt("Enter Customer ID (0 to cancel): ");
        cin >> customerID;
        customerID = trimInput(customerID);

        if (customerID == "0")
        {
            displayInfo("Restore cancelled.");
            return;
        }

        if (!isValidCustomerID(customerID))
        {
            displayError("Invalid Customer ID format. Please use CXXX.");
            pauseScreen();
            continue;
        }

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
                displayError("Customer ID not found.");

                delete res;
                delete pstmt;

                pauseScreen();
                continue;
            }

            string status =
                res->getString("Status");

            if (status == "ACTIVE")
            {
                displayError("Customer is already active.");

                delete res;
                delete pstmt;

                pauseScreen();
                continue;
            }

            if (status != "INACTIVE")
            {
                displayError("Only inactive customers can be restored.");

                delete res;
                delete pstmt;

                pauseScreen();
                continue;
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
            printStatus(status);
            cout << endl;

            delete res;
            delete pstmt;

            cout << "\n";
            printCustomerPrompt("Restore this customer? (Y/N): ");
            cin >> confirm;

            while (toupper(confirm) != 'Y' &&
                toupper(confirm) != 'N')
            {
                displayError("Please enter Y or N.");
                printCustomerPrompt("Restore this customer? (Y/N): ");
                cin >> confirm;
            }

            if (toupper(confirm) == 'N')
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

            return;
        }
        catch (sql::SQLException& e)
        {
            displayError(e.what());
            pauseScreen();
        }
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
