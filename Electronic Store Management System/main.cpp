#include <iostream>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include "Admin.h"
#include "Customer.h"
#include <cstdlib>
#include "Utility.h"
#include "Product.h"
#include "Staff.h"
#include "Order.h"
#include "Report.h"
#include <cctype>
#include <string>

using namespace std;

const string server = "tcp://127.0.0.1:3306";
const string username = "root";
const string password = "";

namespace
{
    bool readMenuChoice(int& choice)
    {
        string input;

        cin >> input;

        if (input.empty())
        {
            return false;
        }

        choice = 0;

        for (char character : input)
        {
            if (!isdigit(static_cast<unsigned char>(character)))
            {
                return false;
            }

            choice =
                choice * 10 +
                (character - '0');

            if (choice > 100)
            {
                return false;
            }
        }

        return true;
    }
}

int main()
{
    sql::Driver* driver;
    sql::Connection* con;

    try
    {
        driver = get_driver_instance();

        con = driver->connect(
            server,
            username,
            password
        );

        con->setSchema("electronic_store_db");

        cout << "Database connection successful!\n";
    }
    catch (sql::SQLException& e)
    {
        cout << "\nConnection Failed: "
            << e.what() << endl;

        return 1;
    }

    while (!login(con))
    {
        cout << "\nPlease try again.\n";
    }

    int choice;

    do
    {
        clearScreen();

        displayHeader("ELECTRONIC STORE MANAGEMENT SYSTEM");
        displayPath(
            "HOME"
        );
        margin();
        cout << "1. Customer Management\n";

        margin();
        cout << "2. Product Management\n";

        margin();
        cout << "3. Order Management\n";

        margin();
        cout << "4. Report Management\n";

        if (isAdmin)
        {
            margin();
            cout << "5. Staff Management\n";

            margin();
            cout << "6. Logout\n";

            margin();
            cout << "7. Exit\n\n";

            margin();
            cout << "Enter your choice (1-7): ";
        }
        else
        {
            margin();
            cout << "5. Logout\n";

            margin();
            cout << "6. Exit\n\n";

            margin();
            cout << "Enter your choice (1-6): ";
        }
        int maxChoice =
            isAdmin ? 7 : 6;

        if (!readMenuChoice(choice) ||
            choice < 1 ||
            choice > maxChoice)
        {
            displayError(
                "Invalid choice. Please enter a number between 1 and " +
                to_string(maxChoice) +
                "."
            );

            pauseScreen();

            continue;
        }

        switch (choice)
        {
        case 1:

            customerManagementMenu(con);
            clearScreen();

            break;

        case 2:

            productManagementMenu(con);
            clearScreen();

            break;

        case 3:

            orderManagementMenu(con);
            clearScreen();

            break;

        case 4:

            reportManagementMenu(con);
            clearScreen();

            break;

        case 5:

            if (isAdmin)
            {
                staffManagementMenu(con);
                clearScreen();
            }
            else
            {
                system("cls");

                cout << "\nLogging out...\n";

                while (!login(con))
                {
                    cout << "\nPlease try again.\n";
                }
            }

            break;

        case 6:

            if (isAdmin)
            {
                system("cls");

                cout << "\nLogging out...\n";

                while (!login(con))
                {
                    cout << "\nPlease try again.\n";
                }
            }
            else
            {
                delete con;

                clearScreen();

                margin();
                cout << "System terminated successfully.\n";

                return 0;
            }

            break;

        case 7:

            if (isAdmin)
            {
                delete con;

                clearScreen();

                margin();
                cout << "System terminated successfully.\n";

                return 0;
            }

            break;

        default:

            displayError("Invalid choice.");

            break;
        }
} while (true);
    delete con;

    cout << "\nSystem terminated successfully.\n";

    return 0;
}
