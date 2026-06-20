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

/*

#include "Sales.h"
#include "Report.h"
*/
using namespace std;

const string server = "tcp://127.0.0.1:3306";
const string username = "root";
const string password = "";




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
        displayHeader("ELECTRONIC STORE MANAGEMENT SYSTEM");

        margin();
        cout << "1. Customer Management\n";

        margin();
        cout << "2. Product Management\n";

        margin();
        cout << "3. Order Management\n";

        margin();
        cout << "4. Sales Management\n";

        margin();
        cout << "5. Report Management\n";

        if (isAdmin)
        {
            margin();
            cout << "6. Staff Management\n";

            margin();
            cout << "7. Logout\n";

            margin();
            cout << "8. Exit\n\n";

            margin();
            cout << "Enter your choice (1-8): ";
        }
        else
        {
            margin();
            cout << "6. Logout\n";

            margin();
            cout << "7. Exit\n\n";

            margin();
            cout << "Enter your choice (1-7): ";
        }
        cin >> choice;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(1000, '\n');

            margin();
            cout << "Invalid choice!\n";

            continue;
        }

        if (isAdmin)
        {
            if (choice < 1 || choice > 8)
            {
                margin();
                cout << "Invalid choice!\n";
                continue;
            }
        }
        else
        {
            if (choice < 1 || choice > 7)
            {
                margin();
                cout << "Invalid choice!\n";
                continue;
            }
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

            /*

            case 4:

                salesManagementMenu(con);
                clearScreen();

                break;

            case 5:

                reportManagementMenu(con);
                clearScreen();

                break;

            */

        case 6:

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

        case 7:

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

        case 8:

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

            margin();
            cout << "\n[ERROR] Invalid choice!\n";

            break;
        }
} while (true);
    delete con;

    cout << "\nSystem terminated successfully.\n";

    return 0;
}