#include "Utility.h"

#include <chrono>
#include <iomanip>
#include <thread>
#include <windows.h>

using namespace std;

namespace
{
    const int CONTENT_WIDTH = 58;
    const string HEADER_LINE(CONTENT_WIDTH, '=');
    const string SECTION_LINE(CONTENT_WIDTH, '-');

    HANDLE getConsoleHandle()
    {
        return GetStdHandle(STD_OUTPUT_HANDLE);
    }

    WORD getDefaultAttributes()
    {
        static WORD defaultAttributes = []()
            {
                CONSOLE_SCREEN_BUFFER_INFO info;

                if (GetConsoleScreenBufferInfo(getConsoleHandle(), &info))
                {
                    return info.wAttributes;
                }

                return static_cast<WORD>(
                    FOREGROUND_RED |
                    FOREGROUND_GREEN |
                    FOREGROUND_BLUE
                );
            }();

        return defaultAttributes;
    }

    WORD colorToAttribute(ConsoleColor color)
    {
        switch (color)
        {
        case DARK_GRAY:
            return FOREGROUND_INTENSITY;

        case BLUE:
            return FOREGROUND_BLUE | FOREGROUND_INTENSITY;

        case CYAN:
            return FOREGROUND_GREEN |
                FOREGROUND_BLUE |
                FOREGROUND_INTENSITY;

        case GREEN:
            return FOREGROUND_GREEN | FOREGROUND_INTENSITY;

        case YELLOW:
            return FOREGROUND_RED |
                FOREGROUND_GREEN |
                FOREGROUND_INTENSITY;

        case RED:
            return FOREGROUND_RED | FOREGROUND_INTENSITY;

        case WHITE:
            return FOREGROUND_RED |
                FOREGROUND_GREEN |
                FOREGROUND_BLUE |
                FOREGROUND_INTENSITY;

        case DEFAULT_COLOR:
        default:
            return getDefaultAttributes();
        }
    }

    string centeredText(string text, int width)
    {
        if (static_cast<int>(text.length()) >= width)
        {
            return text;
        }

        int leftPadding =
            (width - static_cast<int>(text.length())) / 2;

        int rightPadding =
            width - static_cast<int>(text.length()) - leftPadding;

        return string(leftPadding, ' ') +
            text +
            string(rightPadding, ' ');
    }

    string trimText(string text)
    {
        size_t first =
            text.find_first_not_of(' ');

        if (first == string::npos)
        {
            return "";
        }

        size_t last =
            text.find_last_not_of(' ');

        return text.substr(first, last - first + 1);
    }

    void printColored(ConsoleColor color, string text)
    {
        setTextColor(color);
        cout << text;
        resetTextColor();
    }
}

void setTextColor(ConsoleColor color)
{
    SetConsoleTextAttribute(
        getConsoleHandle(),
        colorToAttribute(color)
    );
}

void resetTextColor()
{
    SetConsoleTextAttribute(
        getConsoleHandle(),
        getDefaultAttributes()
    );
}

int getConsoleWidth()
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    if (GetConsoleScreenBufferInfo(getConsoleHandle(), &info))
    {
        return info.srWindow.Right -
            info.srWindow.Left +
            1;
    }

    return 100;
}

void margin()
{
    int spaces =
        (getConsoleWidth() - CONTENT_WIDTH) / 2;

    if (spaces < 0)
    {
        spaces = 0;
    }

    cout << string(spaces, ' ');
}

void displayHeader(string title)
{
    title = trimText(title);

    margin();
    printColored(CYAN, HEADER_LINE + "\n");

    margin();
    printColored(WHITE, centeredText(title, CONTENT_WIDTH) + "\n");

    margin();
    printColored(CYAN, HEADER_LINE + "\n");
}

void displaySection(string title)
{
    title = trimText(title);

    margin();
    printColored(YELLOW, SECTION_LINE + "\n");

    margin();
    printColored(WHITE, centeredText(title, CONTENT_WIDTH) + "\n");

    margin();
    printColored(YELLOW, SECTION_LINE + "\n");
}

void displaySuccess(string message)
{
    cout << "\n";

    margin();

    printColored(GREEN, "[SUCCESS] ");

    cout << message
        << endl;
}

void displayError(string message)
{
    cout << "\n";

    margin();

    printColored(RED, "[ERROR] ");

    cout << message
        << endl;
}

void displayPath(string path)
{
    margin();
    printColored(DARK_GRAY, path + "\n");

    margin();
    printColored(YELLOW, SECTION_LINE + "\n");
}

void loadingScreen()
{
    clearScreen();

    margin();
    printColored(CYAN, "Loading System...\n\n");

    margin();

    for (int i = 0; i <= 20; i++)
    {
        cout << "\r";
        margin();
        cout << "[";

        setTextColor(GREEN);

        for (int j = 0; j < i; j++)
        {
            cout << char(219);
        }

        resetTextColor();

        for (int j = i; j < 20; j++)
        {
            cout << " ";
        }

        cout << "] "
            << setw(3)
            << i * 5
            << "%";

        cout.flush();

        this_thread::sleep_for(
            chrono::milliseconds(80)
        );
    }

    cout << "\n\n";
    margin();
    printColored(GREEN, "System Ready...");

}

void displayInfo(string message)
{
    cout << "\n";
    margin();
    setTextColor(CYAN);
    cout << "[INFO] ";
    resetTextColor();
    cout << message << endl;
}

void displayWarning(string message)
{
    cout << "\n";
    margin();
    setTextColor(YELLOW);
    cout << "[WARNING] ";
    resetTextColor();
    cout << message << endl;
}
