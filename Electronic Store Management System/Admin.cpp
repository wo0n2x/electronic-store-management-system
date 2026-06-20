#include "Admin.h"
#include "Utility.h"

#include <iostream>
#include <string>
#include <conio.h>

#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>

using namespace std;

extern bool isAdmin;
bool isAdmin = false;

// =====================================
// Password Input
// =====================================
string getPassword()
{
    string password;
    char ch;

    while ((ch = _getch()) != 13) // Enter
    {
        if (ch == 8) // Backspace
        {
            if (!password.empty())
            {
                password.pop_back();
                cout << "\b \b";
            }
        }
        else
        {
            password += ch;
            cout << '*';
        }
    }

    cout << endl;
    return password;
}

// =====================================
// Admin Login
// =====================================
bool login(sql::Connection* con)
{
    string loginInput;
    string password;

    clearScreen();
    displayHeader("ELECTRONIC STORE MANAGEMENT SYSTEM");

    margin();
    cout << "                   SYSTEM LOGIN\n\n";
    margin();
    cout << "Database Status : CONNECTED\n";
    margin();
    cout << "System Status   : READY\n\n";
    margin();
    cout << "----------------------------------------------------------\n\n";
    margin();
    cout << "Admin ID / Email : ";
    cin >> loginInput;
    margin();
    cout << "Password         : ";
    password = getPassword();

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM ADMIN "
            "WHERE (AdminID = ? OR Email = ?) "
            "AND Password = ?"
        );

        pstmt->setString(1, loginInput);
        pstmt->setString(2, loginInput);
        pstmt->setString(3, password);

        res = pstmt->executeQuery();

        if (res->next())
        {
            isAdmin = true;

            string adminName =
                res->getString("AdminName");

            delete res;
            delete pstmt;

            clearScreen();

            cout << "==========================================================\n";
            cout << "                    ACCESS GRANTED\n";
            cout << "==========================================================\n\n";

            cout << "Welcome " << adminName << "!\n\n";

            loadingScreen();

            return true;
        }

        delete res;
        delete pstmt;


        pstmt = con->prepareStatement(
            "SELECT * FROM STAFF "
            "WHERE (StaffID=? OR Email=?) "
            "AND Password=? "
            "AND Status='ACTIVE'"
        );

        pstmt->setString(1, loginInput);
        pstmt->setString(2, loginInput);
        pstmt->setString(3, password);

        res = pstmt->executeQuery();

        if (res->next())
        {
            isAdmin = false;

            string staffName =
                res->getString("StaffName");

            delete res;
            delete pstmt;

            clearScreen();

            displayHeader("ACCESS GRANTED");

            margin();
            cout << "Welcome "
                << staffName
                << "!\n\n";

            loadingScreen();

            return true;
        }

        delete res;
        delete pstmt;

        cout << "\nInvalid Admin ID/Email or Password!\n";
        pauseScreen();

        return false;
    }
    catch (sql::SQLException& e)
    {
        cout << "\nDatabase Error: "
            << e.what() << endl;

        pauseScreen();

        return false;
    }
}