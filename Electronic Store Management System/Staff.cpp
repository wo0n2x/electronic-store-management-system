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

using namespace std;
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
        cout << "\n[ERROR] "
            << e.what()
            << endl;

        return "S001";
    }
}

// =====================================
// Validation Functions
// =====================================


bool isValidPassword(string password)
{
    return password.length() >= 4;
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
            "SELECT StaffID, StaffName "
            "FROM STAFF "
            "WHERE Status='ACTIVE' "
            "ORDER BY StaffID"
        );

        cout << "\n";

        margin();
        cout << "============================================================\n";

        margin();
        cout << left
            << setw(15) << "STAFF ID"
            << setw(35) << "STAFF NAME"
            << endl;

        margin();
        cout << "============================================================\n";

        int count = 0;

        while (res->next())
        {
            margin();

            cout << left
                << setw(15) << res->getString("StaffID")
                << setw(35) << res->getString("StaffName")
                << endl;

            count++;
        }

        margin();
        cout << "============================================================\n";

        margin();
        cout << "Total Active Staff : "
            << count
            << endl;

        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        margin();

        cout << "[ERROR] "
            << e.what()
            << endl;
    }
}

// =====================================
// Add Staff
// =====================================

void addStaff(sql::Connection* con)
{
    displayHeader("ADD STAFF");
    displayPath("   HOME > STAFF MANAGEMENT > ADD STAFF");

    string staffID = generateStaffID(con);

    string name;
    string email;
    string password;

    cin.ignore();

    // Staff Name
    margin();
    cout << "Enter Staff Name : ";
    getline(cin, name);

    while (!isValidName(name))
    {
        margin();
        cout << "[ERROR] Name must contain letters only.\n";

        margin();
        cout << "Enter Staff Name : ";
        getline(cin, name);
    }

    // Email
    margin();
    cout << "Enter Email      : ";
    getline(cin, email);

    while (!isValidEmail(email))
    {
        margin();
        cout << "[ERROR] Invalid email format.\n";

        margin();
        cout << "Enter Email      : ";
        getline(cin, email);
    }

    // Password
    margin();
    cout << "Enter Password   : ";
    getline(cin, password);

    while (!isValidPassword(password))
    {
        margin();
        cout << "[ERROR] Password must be at least 4 characters.\n";

        margin();
        cout << "Enter Password   : ";
        getline(cin, password);
    }

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        // Check duplicate email
        pstmt = con->prepareStatement(
            "SELECT * FROM STAFF "
            "WHERE Email=?"
        );

        pstmt->setString(1, email);

        res = pstmt->executeQuery();

        if (res->next())
        {
            margin();
            cout << "[ERROR] Email already exists!\n";

            delete res;
            delete pstmt;

            return;
        }

        delete res;
        delete pstmt;

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

        margin();
        cout << "\n[SUCCESS] Staff added successfully!\n";

        margin();
        cout << "Staff ID : "
            << staffID
            << endl;

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        margin();

        cout << "[ERROR] "
            << e.what()
            << endl;
    }
}


// =====================================
// View Staff
// =====================================

void viewStaff(sql::Connection* con)
{
    displayHeader("                   VIEW STAFF");
    displayPath("   HOME > STAFF MANAGEMENT > VIEW STAFF");

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
        cout << "========================================================================\n";

        margin();
        cout << left
            << setw(12) << "STAFF ID"
            << setw(25) << "STAFF NAME"
            << setw(30) << "EMAIL"
            << setw(12) << "STATUS"
            << endl;

        margin();
        cout << "========================================================================\n";

        int count = 0;

        while (res->next())
        {
            margin();

            cout << left
                << setw(12) << res->getString("StaffID")
                << setw(25) << res->getString("StaffName")
                << setw(30) << res->getString("Email")
                << setw(12) << res->getString("Status")
                << endl;

            count++;
        }

        margin();
        cout << "========================================================================\n";

        cout << "\n";

        margin();
        cout << "Total Active Staff : "
            << count
            << endl;

        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        margin();

        cout << "[ERROR] "
            << e.what()
            << endl;
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
    cout << "Enter Staff ID (SXXX): ";
    cin >> staffID;

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
            margin();
            cout << "[ERROR] Staff not found!\n";

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

        margin();
        cout << "Current Staff Information\n\n";

        margin();
        cout << "Name     : "
            << currentName
            << " (READ ONLY)\n";

        margin();
        cout << "Email    : "
            << currentEmail
            << endl;

        margin();
        cout << "Password : "
            << currentPassword
            << endl;

        cout << "\n";

        margin();
        cout << "New Email (Press Enter to keep current): ";
        getline(cin, newEmail);

        if (newEmail.empty())
        {
            newEmail = currentEmail;
        }
        else
        {
            while (!isValidEmail(newEmail))
            {
                margin();
                cout << "[ERROR] Invalid email format.\n";

                margin();
                cout << "New Email : ";
                getline(cin, newEmail);
            }

            sql::PreparedStatement* checkStmt;
            sql::ResultSet* checkRes;

            checkStmt = con->prepareStatement(
                "SELECT * FROM STAFF "
                "WHERE Email=? "
                "AND StaffID<>?"
            );

            checkStmt->setString(1, newEmail);
            checkStmt->setString(2, staffID);

            checkRes = checkStmt->executeQuery();

            if (checkRes->next())
            {
                margin();
                cout << "[ERROR] Email already exists!\n";

                delete checkRes;
                delete checkStmt;

                return;
            }

            delete checkRes;
            delete checkStmt;
        }

        margin();
        cout << "New Password (Press Enter to keep current): ";
        getline(cin, newPassword);

        if (newPassword.empty())
        {
            newPassword = currentPassword;
        }
        else
        {
            while (!isValidPassword(newPassword))
            {
                margin();
                cout << "[ERROR] Password must be at least 4 characters.\n";

                margin();
                cout << "New Password : ";
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

        cout << "\n";

        margin();
        cout << "[SUCCESS] Staff updated successfully!\n";

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        margin();

        cout << "\n[ERROR] "
            << e.what()
            << endl;
    }
}
// =====================================
// Delete Staff
// =====================================

void deleteStaff(sql::Connection* con)
{
    string staffID;
    char confirm;

    displayHeader("                 DELETE STAFF");
    displayPath("   HOME > STAFF MANAGEMENT > DELETE STAFF");

    displayStaffList(con);

    cout << "\n";

    margin();
    cout << "Enter Staff ID (SXXX): ";
    cin >> staffID;

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
            margin();
            cout << "[ERROR] Staff not found!\n";

            delete res;
            delete pstmt;

            return;
        }

        cout << "\n";

        margin();
        cout << "============================================================\n";

        margin();
        cout << "Staff Information\n";

        margin();
        cout << "============================================================\n";

        margin();
        cout << "Staff ID : "
            << res->getString("StaffID")
            << endl;

        margin();
        cout << "Name     : "
            << res->getString("StaffName")
            << endl;

        margin();
        cout << "Email    : "
            << res->getString("Email")
            << endl;

        margin();
        cout << "Status   : "
            << res->getString("Status")
            << endl;

        margin();
        cout << "============================================================\n";

        delete res;
        delete pstmt;

        cout << "\n";

        margin();
        cout << "Deactivate this staff? (Y/N): ";
        cin >> confirm;

        if (toupper(confirm) != 'Y')
        {
            margin();
            cout << "\n[INFO] Operation cancelled.\n";

            return;
        }

        pstmt = con->prepareStatement(
            "UPDATE STAFF "
            "SET Status='INACTIVE' "
            "WHERE StaffID=?"
        );

        pstmt->setString(1, staffID);

        pstmt->executeUpdate();

        margin();
        cout << "\n[SUCCESS] Staff deactivated successfully!\n";

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        margin();

        cout << "\n[ERROR] "
            << e.what()
            << endl;
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
            "SELECT StaffID, StaffName "
            "FROM STAFF "
            "WHERE Status='INACTIVE' "
            "ORDER BY StaffID"
        );

        cout << "\n";

        margin();
        cout << "============================================================\n";

        margin();
        cout << "INACTIVE STAFF LIST\n";

        margin();
        cout << "============================================================\n";

        margin();
        cout << left
            << setw(15) << "STAFF ID"
            << setw(35) << "STAFF NAME"
            << endl;

        margin();
        cout << "============================================================\n";

        int count = 0;

        while (res->next())
        {
            margin();

            cout << left
                << setw(15) << res->getString("StaffID")
                << setw(35) << res->getString("StaffName")
                << endl;

            count++;
        }

        margin();
        cout << "============================================================\n";

        margin();
        cout << "Total Inactive Staff : "
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


// =====================================
// Restore Staff
// =====================================

void restoreStaff(sql::Connection* con)
{
    clearScreen();

    displayHeader("                 RESTORE STAFF");
    displayPath("   HOME > STAFF MANAGEMENT > RESTORE STAFF");

    viewInactiveStaff(con);

    string staffID;
    char confirm;

    cout << "\n";

    margin();
    cout << "Enter Staff ID (SXXX): ";
    cin >> staffID;

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
            margin();
            cout << "[ERROR] Inactive staff not found!\n";

            delete res;
            delete pstmt;

            return;
        }

        cout << "\n";

        margin();
        cout << "============================================================\n";

        margin();
        cout << "Staff Information\n";

        margin();
        cout << "============================================================\n";

        margin();
        cout << "Staff ID : "
            << res->getString("StaffID")
            << endl;

        margin();
        cout << "Name     : "
            << res->getString("StaffName")
            << endl;

        margin();
        cout << "Email    : "
            << res->getString("Email")
            << endl;

        margin();
        cout << "Status   : "
            << res->getString("Status")
            << endl;

        margin();
        cout << "============================================================\n";

        delete res;
        delete pstmt;

        cout << "\n";

        margin();
        cout << "Restore this staff? (Y/N): ";
        cin >> confirm;

        if (toupper(confirm) != 'Y')
        {
            margin();
            cout << "\n[INFO] Restore cancelled.\n";

            return;
        }

        pstmt = con->prepareStatement(
            "UPDATE STAFF "
            "SET Status='ACTIVE' "
            "WHERE StaffID=?"
        );

        pstmt->setString(1, staffID);

        pstmt->executeUpdate();

        margin();
        cout << "\n[SUCCESS] Staff restored successfully!\n";

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        margin();

        cout << "\n[ERROR] "
            << e.what()
            << endl;
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

        displayHeader("               STAFF MANAGEMENT");
        displayPath("   HOME > STAFF MANAGEMENT");

        margin();
        cout << "[1] Add Staff\n";

        margin();
        cout << "[2] Update Staff\n";

        margin();
        cout << "[3] Delete Staff\n";

        margin();
        cout << "[4] Restore Staff\n";

        margin();
        cout << "[5] View Staff\n";

        margin();
        cout << "[6] Back\n";

        cout << "\n";

        margin();
        cout << "Enter Choice: ";
        cin >> choice;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(1000, '\n');

            margin();
            cout << "\n[ERROR] Invalid choice!\n";

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

            margin();
            cout << "\nReturning to Main Menu...\n";

           // loadingScreen();

            return;


        default:

            margin();
            cout << "\n[ERROR] Invalid choice!\n";

            pauseScreen();
        }

    } while (true);
}