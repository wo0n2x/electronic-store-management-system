#ifndef UTILITY_H
#define UTILITY_H
#include <thread>
#include <chrono>
#include <iostream>
#include <cstdlib>
using namespace std;
inline void clearScreen()
{
    system("cls");
}

inline void pauseScreen()
{
    system("pause");
}

#endif


inline void loadingScreen()
{
    clearScreen();

    cout << "Loading System...\n\n";

    for (int i = 0; i <= 20; i++)
    {
        cout << "\r[";
        for (int j = 0; j < i; j++)
            cout << char(219);

        for (int j = i; j < 20; j++)
            cout << " ";

        cout << "] " << i * 5 << "%";
        cout.flush();

        std::this_thread::sleep_for(
            std::chrono::milliseconds(80)
        );
    }

    cout << "\n\nSystem Ready...";
    std::this_thread::sleep_for(
        std::chrono::milliseconds(500)
    );

    clearScreen();
}


void displayHeader(string title);

void displaySection(string title);

void displaySuccess(string message);

void displayError(string message);