#include <iostream>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include "Admin.h"
#include "Customer.h"
#include <cstdlib>

#include "Product.h"

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
        cout << "\n=====================================\n";
        cout << " Electronic Store Management System\n";
        cout << "=====================================\n";

        cout << "1. Customer Management\n";
        cout << "2. Product Management\n";
        cout << "3. ORDER Management\n";
        cout << "4. Sales Management\n";
        cout << "5. Report Management\n";
        cout << "6. Logout\n";

        cout << "\nEnter your choice (1-6): ";
        cin >> choice;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(1000, '\n');

            cout << "\nInvalid choice!";
            cout << "\nPlease choose a valid option.\n";

            continue;
        }

        if (choice < 1 || choice > 6)
        {
            cout << "\nInvalid choice!";
            cout << "\nPlease choose a valid option.\n";
            continue;
        }

        switch (choice)
        {
        case 1:
            customerManagementMenu(con);
            break;

        case 2:
            productManagementMenu(con);
            break;
        case 3:
            orderManagementMenu(con);
            break;
            /*

        case 4:
            salesManagementMenu(con);
            break;

        case 5:
            reportManagementMenu(con);
            break;
*/
        case 6:

            system("cls");

            cout << "\nLogging out...\n";

            while (!login(con))
            {
                cout << "\nPlease try again.\n";
            }

            break;
        }

    } while (choice != 6);

    delete con;

    cout << "\nSystem terminated successfully.\n";

    return 0;
}