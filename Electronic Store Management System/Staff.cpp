#include "Staff.h"
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
    void printStaffPrompt(string prompt)
    {
        margin();
        cout << prompt;
    }

    void printStaffLine(int width)
    {
        printTableLine(width);
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

    void printStaffStatusField(string status, int width)
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

    bool staffEmailExists(
        sql::Connection* con,
        string email,
        string excludedStaffID = ""
    )
    {
        string query =
            "SELECT StaffID "
            "FROM STAFF "
            "WHERE Email=? ";

        if (!excludedStaffID.empty())
        {
            query += "AND StaffID<>? ";
        }

        query += "LIMIT 1";

        unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(query)
        );

        pstmt->setString(1, email);

        if (!excludedStaffID.empty())
        {
            pstmt->setString(2, excludedStaffID);
        }

        unique_ptr<sql::ResultSet> res(
            pstmt->executeQuery()
        );

        return res->next();
    }
}

// =====================================
// Generate Staff ID
// =====================================

string generateStaffID(sql::Connection* con)
{
    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT StaffID "
            "FROM STAFF "
            "ORDER BY StaffID DESC "
            "LIMIT 1"
        );

        string newID = "S001";

        if (res->next())
        {
            string lastID =
                res->getString("StaffID");

            int number =
                stoi(lastID.substr(1));

            number++;

            char buffer[10];

            sprintf_s(
                buffer,
                "S%03d",
                number
            );

            newID = buffer;
        }

        delete res;
        delete stmt;

        return newID;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());

        return "S001";
    }
}

// =====================================
// Validation Functions
// =====================================


bool isValidPassword(string password)
{
    return !trimInput(password).empty() &&
        password.length() >= 6;
}

// =====================================
// Display Staff List
// =====================================

void displayStaffList(sql::Connection* con)
{
    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT StaffID, StaffName, Status "
            "FROM STAFF "
            "WHERE Status='ACTIVE' "
            "ORDER BY StaffID"
        );

        cout << "\n";

        displaySection("ACTIVE STAFF");

        margin();
        setTableHeaderColor();
        cout << left
            << setw(15) << "Staff ID"
            << setw(30) << "Staff Name"
            << setw(15) << "Status"
            << endl;
        resetTextColor();

        printStaffLine(60);

        int count = 0;

        while (res->next())
        {
            margin();

            cout << left
                << setw(15) << res->getString("StaffID")
                << setw(30) << res->getString("StaffName");
            printStaffStatusField(res->getString("Status"), 15);
            cout << endl;

            count++;
        }

        printStaffLine(60);

        margin();
        cout << "Total Active Staff : "
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
// Add Staff
// =====================================

void addStaff(sql::Connection* con)
{
    displayHeader("ADD STAFF");
    displayPath("HOME > STAFF MANAGEMENT > ADD STAFF");
    displaySection("STAFF DETAILS");

    string staffID = generateStaffID(con);

    string name;
    string email;
    string password;

    cin.ignore();

    try
    {
        // Staff Name
        printStaffPrompt("Enter Staff Name : ");
        getline(cin, name);
        name = trimInput(name);

        while (!isValidName(name))
        {
            displayError("Name must contain letters only.");

            printStaffPrompt("Enter Staff Name : ");
            getline(cin, name);
            name = trimInput(name);
        }

        // Email
        printStaffPrompt("Enter Email      : ");
        getline(cin, email);
        email = trimInput(email);

        while (!isValidEmail(email) ||
            staffEmailExists(con, email))
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

            printStaffPrompt("Enter Email      : ");
            getline(cin, email);
            email = trimInput(email);
        }

        // Password
        printStaffPrompt("Enter Password   : ");
        getline(cin, password);

        while (!isValidPassword(password))
        {
            displayError("Password must be at least 6 characters.");

            printStaffPrompt("Enter Password   : ");
            getline(cin, password);
        }

        sql::PreparedStatement* pstmt;

        // Insert Staff
        pstmt = con->prepareStatement(
            "INSERT INTO STAFF "
            "(StaffID, StaffName, Email, Password, Status) "
            "VALUES (?, ?, ?, ?, 'ACTIVE')"
        );

        pstmt->setString(1, staffID);
        pstmt->setString(2, name);
        pstmt->setString(3, email);
        pstmt->setString(4, password);

        pstmt->executeUpdate();

        displaySuccess("Staff added successfully.");

        margin();
        cout << "Staff ID : "
            << staffID
            << endl;

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}


// =====================================
// View Staff
// =====================================

void viewStaff(sql::Connection* con)
{
    displayHeader("VIEW STAFF");
    displayPath("HOME > STAFF MANAGEMENT > VIEW STAFF");

    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT * FROM STAFF "
            "WHERE Status='ACTIVE' "
            "ORDER BY StaffID"
        );

        margin();
        setTableHeaderColor();
        cout << left
            << setw(12) << "Staff ID"
            << setw(25) << "Staff Name"
            << setw(35) << "Email"
            << setw(15) << "Status"
            << endl;
        resetTextColor();

        printStaffLine(87);

        int count = 0;

        while (res->next())
        {
            margin();

            cout << left
                << setw(12) << res->getString("StaffID")
                << setw(25) << res->getString("StaffName")
                << setw(35) << res->getString("Email");
            printStaffStatusField(res->getString("Status"), 15);
            cout << endl;

            count++;
        }

        printStaffLine(87);

        margin();
        cout << "Total Active Staff : "
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
// Update Staff
// =====================================

void updateStaff(sql::Connection* con)
{
    string staffID;

    displayHeader("UPDATE STAFF");
    displayPath("HOME > STAFF MANAGEMENT > UPDATE STAFF");

    displayStaffList(con);

    cout << "\n";

    margin();
    cout << "Enter Staff ID (0 to cancel): ";
    cin >> staffID;

    if (staffID == "0")
    {
        displayInfo("Update cancelled.");
        return;
    }

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM STAFF "
            "WHERE StaffID=? "
            "AND Status='ACTIVE'"
        );

        pstmt->setString(1, staffID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            displayError("Staff not found.");

            delete res;
            delete pstmt;

            return;
        }

        string currentName =
            res->getString("StaffName");

        string currentEmail =
            res->getString("Email");

        string currentPassword =
            res->getString("Password");

        delete res;
        delete pstmt;

        string newEmail;
        string newPassword;

        cin.ignore();

        cout << "\n";

        displaySection("CURRENT STAFF INFORMATION");

        margin();
        cout << left << setw(12) << "Name" << ": "
            << currentName << " (READ ONLY)" << endl;

        margin();
        cout << left << setw(12) << "Email" << ": "
            << currentEmail << endl;

        margin();
        cout << left << setw(12) << "Password" << ": "
            << currentPassword << endl;

        cout << "\n";

        printStaffPrompt("New Email (Press Enter to keep current): ");
        getline(cin, newEmail);
        newEmail = trimInput(newEmail);

        if (newEmail.empty())
        {
            newEmail = currentEmail;
        }
        else
        {
            while (!isValidEmail(newEmail) ||
                staffEmailExists(con, newEmail, staffID))
            {
                if (!isValidEmail(newEmail))
                {
                    displayError("Invalid email format.");
                }
                else
                {
                    displayError(
                        "Email already exists. Please enter a different email."
                    );
                }

                printStaffPrompt("New Email : ");
                getline(cin, newEmail);
                newEmail = trimInput(newEmail);
            }
        }

        printStaffPrompt("New Password (Press Enter to keep current): ");
        getline(cin, newPassword);

        if (newPassword.empty())
        {
            newPassword = currentPassword;
        }
        else
        {
            while (!isValidPassword(newPassword))
            {
                displayError("Password must be at least 6 characters.");

                printStaffPrompt("New Password : ");
                getline(cin, newPassword);
            }
        }

        pstmt = con->prepareStatement(
            "UPDATE STAFF "
            "SET Email=?, "
            "Password=? "
            "WHERE StaffID=?"
        );

        pstmt->setString(1, newEmail);
        pstmt->setString(2, newPassword);
        pstmt->setString(3, staffID);

        pstmt->executeUpdate();

        displaySuccess("Staff updated successfully.");

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}
// =====================================
// Delete Staff
// =====================================

void deleteStaff(sql::Connection* con)
{
    string staffID;
    char confirm;

    displayHeader("DELETE STAFF");
    displayPath("HOME > STAFF MANAGEMENT > DELETE STAFF");

    displayStaffList(con);

    cout << "\n";

    margin();
    cout << "Enter Staff ID (0 to cancel): ";
    cin >> staffID;

    if (staffID == "0")
    {
        displayInfo("Delete cancelled.");
        return;
    }

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM STAFF "
            "WHERE StaffID=? "
            "AND Status='ACTIVE'"
        );

        pstmt->setString(1, staffID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            displayError("Staff not found.");

            delete res;
            delete pstmt;

            return;
        }

        displaySection("STAFF DETAILS");

        margin();
        cout << left << setw(12) << "Staff ID" << ": "
            << res->getString("StaffID") << endl;

        margin();
        cout << left << setw(12) << "Name" << ": "
            << res->getString("StaffName") << endl;

        margin();
        cout << left << setw(12) << "Email" << ": "
            << res->getString("Email") << endl;

        margin();
        cout << left << setw(12) << "Status" << ": ";
        printStatus(res->getString("Status"));
        cout << endl;

        delete res;
        delete pstmt;

        displayWarning("This staff will become inactive.");
        cout << "\n";
        printStaffPrompt("Deactivate this staff? (Y/N): ");
        cin >> confirm;

        if (toupper(confirm) != 'Y')
        {
            displayInfo("Operation cancelled.");

            return;
        }

        pstmt = con->prepareStatement(
            "UPDATE STAFF "
            "SET Status='INACTIVE' "
            "WHERE StaffID=?"
        );

        pstmt->setString(1, staffID);

        pstmt->executeUpdate();

        displaySuccess("Staff deactivated successfully.");

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}

// =====================================
// View Inactive Staff
// =====================================

void viewInactiveStaff(sql::Connection* con)
{
    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT StaffID, StaffName, Status "
            "FROM STAFF "
            "WHERE Status='INACTIVE' "
            "ORDER BY StaffID"
        );

        cout << "\n";

        displaySection("INACTIVE STAFF");

        margin();
        setTableHeaderColor();
        cout << left
            << setw(15) << "Staff ID"
            << setw(30) << "Staff Name"
            << setw(15) << "Status"
            << endl;
        resetTextColor();

        printStaffLine(60);

        int count = 0;

        while (res->next())
        {
            margin();

            cout << left
                << setw(15) << res->getString("StaffID")
                << setw(30) << res->getString("StaffName");
            printStaffStatusField(res->getString("Status"), 15);
            cout << endl;

            count++;
        }

        printStaffLine(60);

        margin();
        cout << "Total Inactive Staff : "
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
// Restore Staff
// =====================================

void restoreStaff(sql::Connection* con)
{
    clearScreen();

    displayHeader("RESTORE STAFF");
    displayPath("HOME > STAFF MANAGEMENT > RESTORE STAFF");

    viewInactiveStaff(con);

    string staffID;
    char confirm;

    cout << "\n";

    margin();
    cout << "Enter Staff ID (0 to cancel): ";
    cin >> staffID;

    if (staffID == "0")
    {
        displayInfo("Restore cancelled.");
        return;
    }

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM STAFF "
            "WHERE StaffID=? "
            "AND Status='INACTIVE'"
        );

        pstmt->setString(1, staffID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            displayError("Inactive staff not found.");

            delete res;
            delete pstmt;

            return;
        }

        displaySection("STAFF DETAILS");

        margin();
        cout << left << setw(12) << "Staff ID" << ": "
            << res->getString("StaffID") << endl;

        margin();
        cout << left << setw(12) << "Name" << ": "
            << res->getString("StaffName") << endl;

        margin();
        cout << left << setw(12) << "Email" << ": "
            << res->getString("Email") << endl;

        margin();
        cout << left << setw(12) << "Status" << ": ";
        printStatus(res->getString("Status"));
        cout << endl;

        delete res;
        delete pstmt;

        cout << "\n";

        printStaffPrompt("Restore this staff? (Y/N): ");
        cin >> confirm;

        if (toupper(confirm) != 'Y')
        {
            displayInfo("Restore cancelled.");

            return;
        }

        pstmt = con->prepareStatement(
            "UPDATE STAFF "
            "SET Status='ACTIVE' "
            "WHERE StaffID=?"
        );

        pstmt->setString(1, staffID);

        pstmt->executeUpdate();

        displaySuccess("Staff restored successfully.");

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
// =====================================
// Staff Management Menu
// =====================================

void staffManagementMenu(sql::Connection* con)
{
    int choice;

    do
    {
        clearScreen();

        displayHeader("STAFF MANAGEMENT");
        displayPath("HOME > STAFF MANAGEMENT");

        margin();
        cout << "1. Add Staff\n";

        margin();
        cout << "2. Update Staff\n";

        margin();
        cout << "3. Delete Staff\n";

        margin();
        cout << "4. Restore Staff\n";

        margin();
        cout << "5. View Staff\n";

        margin();
        cout << "6. Back\n";

        cout << "\n";

        margin();
        cout << "Enter Choice : ";
        cin >> choice;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(1000, '\n');

            displayError(
                "Invalid choice. Please enter a number between 1 and 6."
            );

            pauseScreen();

            continue;
        }

        switch (choice)
        {
        case 1:

            clearScreen();

            addStaff(con);

            pauseScreen();

            break;

        case 2:

            clearScreen();

            updateStaff(con);

            pauseScreen();

            break;

        case 3:

            clearScreen();

            deleteStaff(con);

            pauseScreen();

            break;

        case 4:

            clearScreen();

            restoreStaff(con);

            pauseScreen();

            break;

        case 5:

            clearScreen();

            viewStaff(con);

            pauseScreen();

            break;

        case 6:

            return;


        default:

            displayError(
                "Invalid choice. Please enter a number between 1 and 6."
            );

            pauseScreen();
        }

    } while (true);
}
