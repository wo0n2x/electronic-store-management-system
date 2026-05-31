#include <iomanip>
#include <iostream>
#include <cstring>
#include <string>

using namespace std;

void displayHeader(string title)
{
    cout << "\n============================================================\n";

    cout << setw(35 + title.length() / 2)
        << title
        << endl;

    cout << "============================================================\n";
}

void displaySection(string title)
{
    cout << "\n------------------------------------------------------------\n";

    cout << title << endl;

    cout << "------------------------------------------------------------\n";
}

void displaySuccess(string message)
{
    cout << "\n[SUCCESS] "
        << message
        << endl;
}

void displayError(string message)
{
    cout << "\n[ERROR] "
        << message
        << endl;
}