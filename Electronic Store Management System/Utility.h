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

void setHeaderColor();
void setSectionColor();
void setTableHeaderColor();
void setSuccessColor();
void setErrorColor();
void setWarningColor();
void setInfoColor();
void setStatusColor(string status);

void margin();
void loadingScreen();
void displayWarning(string message);

void printSuccess(string message);
void printError(string message);
void printWarning(string message);
void printInfo(string message);
void printStatus(string status);
void printTableLine(int width);

void displayHeader(string title);
void displaySection(string title);
void displaySuccess(string message);
void displayError(string message);
void displayPath(string path);
void displayInfo(string message);
#endif
