#include "Admin.h"
#include "Utility.h"

#include <iostream>
#include <string>
#include <conio.h>

#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>

using namespace std;

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

    cout << "==========================================================\n";
    cout << "         ELECTRONIC STORE MANAGEMENT SYSTEM\n";
    cout << "==========================================================\n\n";

    cout << "                 ADMINISTRATOR LOGIN\n\n";

    cout << "Database Status : CONNECTED\n";
    cout << "System Status   : READY\n\n";

    cout << "----------------------------------------------------------\n\n";

    cout << "Admin ID / Email : ";
    cin >> loginInput;

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
            string adminName = res->getString("AdminName");

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