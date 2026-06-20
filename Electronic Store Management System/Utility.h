#ifndef UTILITY_H
#define UTILITY_H

#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;

enum ConsoleColor
{
    DEFAULT_COLOR,
    DARK_GRAY,
    BLUE,
    CYAN,
    GREEN,
    YELLOW,
    RED,
    WHITE
};

inline void clearScreen()
{
    system("cls");
}

inline void pauseScreen()
{
    system("pause");
}

void setTextColor(ConsoleColor color);
void resetTextColor();
int getConsoleWidth();

void margin();
void loadingScreen();

void displayHeader(string title);
void displaySection(string title);
void displaySuccess(string message);
void displayError(string message);
void displayPath(string path);

#endif
