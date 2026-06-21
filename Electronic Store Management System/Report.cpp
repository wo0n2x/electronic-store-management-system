#include "Report.h"
#include "Utility.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

using namespace std;

void salesReportMenu(sql::Connection* con);
void productReportMenu(sql::Connection* con);
void customerReportMenu(sql::Connection* con);
void inventoryReportMenu(sql::Connection* con);
void salesAnalyticsMenu(sql::Connection* con);

void dailySalesReport(sql::Connection* con);
void monthlySalesReport(sql::Connection* con);
void salesByPaymentMethodReport(sql::Connection* con);

void bestSellingProductsReport(sql::Connection* con);
void productSalesSummaryReport(sql::Connection* con);
void bestSellingCategoriesReport(sql::Connection* con);

void topCustomersReport(sql::Connection* con);
void customerPurchaseSummaryReport(sql::Connection* con);

void currentInventoryReport(sql::Connection* con);
void lowStockAlertReport(sql::Connection* con);
void outOfStockProductsReport(sql::Connection* con);

void revenueByCategoryAnalytics(sql::Connection* con);
void topFiveBestSellingProducts(sql::Connection* con);

namespace
{
    const int BAR_WIDTH = 36;

    struct Metric
    {
        string label;
        string value;
    };

    struct DailySalesRow
    {
        string date;
        int orders = 0;
        double revenue = 0;
    };

    struct MonthlySalesRow
    {
        string month;
        int orders = 0;
        double revenue = 0;
    };

    struct PaymentRow
    {
        string method;
        int transactions = 0;
        double revenue = 0;
        double percentage = 0;
    };

    struct ProductUnitsRow
    {
        string productID;
        string productName;
        int unitsSold = 0;
    };

    struct ProductRevenueRow
    {
        string productID;
        string productName;
        int unitsSold = 0;
        double revenue = 0;
    };

    struct CategoryRow
    {
        string category;
        int unitsSold = 0;
        double revenue = 0;
        double percentage = 0;
    };

    struct CustomerRow
    {
        string customerID;
        string customerName;
        int orders = 0;
        double spending = 0;
    };

    struct InventoryRow
    {
        string productID;
        string productName;
        int stock = 0;
    };

    struct PieSlice
    {
        string label;
        double value = 0;
        double percentage = 0;
        char symbol = 'A';
    };

    void discardLine()
    {
        cin.ignore(
            numeric_limits<streamsize>::max(),
            '\n'
        );
    }

    string fitText(string text, int width)
    {
        if (static_cast<int>(text.length()) <= width)
        {
            return text;
        }

        if (width <= 3)
        {
            return text.substr(0, width);
        }

        return text.substr(0, width - 3) + "...";
    }

    string moneyText(double value)
    {
        ostringstream stream;

        stream << "RM "
            << fixed
            << setprecision(2)
            << value;

        return stream.str();
    }

    string numberText(double value)
    {
        ostringstream stream;

        stream << fixed
            << setprecision(0)
            << value;

        return stream.str();
    }

    string percentText(double value)
    {
        ostringstream stream;

        stream << fixed
            << setprecision(2)
            << value
            << "%";

        return stream.str();
    }

    void printPrompt(string prompt)
    {
        margin();
        cout << prompt;
    }

    void printLine(int width)
    {
        margin();
        cout << string(width, '-') << endl;
    }

    bool readChoice(
        string prompt,
        int& choice,
        int minValue,
        int maxValue
    )
    {
        printPrompt(prompt);

        while (!(cin >> choice) ||
            choice < minValue ||
            choice > maxValue)
        {
            cin.clear();
            discardLine();

            displayError(
                "Please enter a number between " +
                to_string(minValue) +
                " and " +
                to_string(maxValue) +
                "."
            );

            cout << "\n";
            printPrompt(prompt);
        }

        discardLine();

        return true;
    }

    void closeQuery(
        sql::ResultSet*& res,
        sql::PreparedStatement*& pstmt
    )
    {
        delete res;
        delete pstmt;

        res = nullptr;
        pstmt = nullptr;
    }

    void printSummary(vector<Metric> metrics)
    {
        displaySection("SUMMARY");

        for (const Metric& metric : metrics)
        {
            margin();
            cout << left
                << setw(28) << metric.label
                << ": "
                << metric.value
                << endl;
        }

        cout << "\n";
    }

    void printTableTitle(string title)
    {
        displaySection(title);
    }

    void printVisualizationTitle(string title)
    {
        displaySection("VISUALIZATION");

        margin();
        cout << title << endl;

        margin();
        cout << string(58, '-') << endl;
    }

    void printBarChart(
        string title,
        vector<pair<string, double>> values,
        bool moneyValue
    )
    {
        printVisualizationTitle(title);

        if (values.empty())
        {
            displayInfo("No data available for visualization.");
            return;
        }

        double maxValue = 0;

        for (const auto& item : values)
        {
            if (item.second > maxValue)
            {
                maxValue = item.second;
            }
        }

        if (maxValue <= 0)
        {
            displayInfo("Chart cannot be generated because all values are zero.");
            return;
        }

        for (const auto& item : values)
        {
            int length = static_cast<int>(
                round((item.second / maxValue) * BAR_WIDTH)
            );

            if (length == 0 && item.second > 0)
            {
                length = 1;
            }

            margin();
            cout << left
                << setw(22) << fitText(item.first, 21)
                << " | "
                << setw(BAR_WIDTH) << string(length, '#')
                << " "
                << (moneyValue ? moneyText(item.second) : numberText(item.second))
                << endl;
        }

        cout << "\n";
    }

    vector<PieSlice> buildPieSlices(vector<pair<string, double>> values)
    {
        vector<PieSlice> slices;
        double total = 0;
        const char symbols[] = { 'A', 'B', 'C', 'D' };

        sort(
            values.begin(),
            values.end(),
            [](const pair<string, double>& leftValue,
                const pair<string, double>& rightValue)
            {
                return leftValue.second > rightValue.second;
            }
        );

        for (const auto& value : values)
        {
            total += value.second;
        }

        if (total <= 0)
        {
            return slices;
        }

        double others = 0;

        for (int i = 0; i < static_cast<int>(values.size()); i++)
        {
            if (i < 3)
            {
                PieSlice slice;

                slice.label = values[i].first;
                slice.value = values[i].second;
                slice.percentage = (values[i].second / total) * 100;
                slice.symbol = symbols[i];

                slices.push_back(slice);
            }
            else
            {
                others += values[i].second;
            }
        }

        if (others > 0)
        {
            PieSlice slice;

            slice.label = "Others";
            slice.value = others;
            slice.percentage = (others / total) * 100;
            slice.symbol = symbols[3];

            slices.push_back(slice);
        }

        return slices;
    }

    vector<PieSlice> buildDirectPieSlices(vector<pair<string, double>> values)
    {
        vector<PieSlice> slices;
        double total = 0;
        const char symbols[] = { 'A', 'B', 'C', 'D' };

        for (const auto& value : values)
        {
            total += value.second;
        }

        if (total <= 0)
        {
            return slices;
        }

        for (int i = 0; i < static_cast<int>(values.size()) && i < 4; i++)
        {
            PieSlice slice;

            slice.label = values[i].first;
            slice.value = values[i].second;
            slice.percentage = (values[i].second / total) * 100;
            slice.symbol = symbols[i];

            slices.push_back(slice);
        }

        return slices;
    }

    vector<CategoryRow> groupCategoriesByUnits(vector<CategoryRow> rows)
    {
        vector<CategoryRow> groupedRows;
        CategoryRow others;

        others.category = "Others";

        for (int i = 0; i < static_cast<int>(rows.size()); i++)
        {
            if (i < 3)
            {
                groupedRows.push_back(rows[i]);
            }
            else
            {
                others.unitsSold += rows[i].unitsSold;
                others.percentage += rows[i].percentage;
            }
        }

        if (others.unitsSold > 0)
        {
            groupedRows.push_back(others);
        }

        return groupedRows;
    }

    vector<CategoryRow> groupCategoriesByRevenue(vector<CategoryRow> rows)
    {
        vector<CategoryRow> groupedRows;
        CategoryRow others;

        others.category = "Others";

        for (int i = 0; i < static_cast<int>(rows.size()); i++)
        {
            if (i < 3)
            {
                groupedRows.push_back(rows[i]);
            }
            else
            {
                others.revenue += rows[i].revenue;
                others.percentage += rows[i].percentage;
            }
        }

        if (others.revenue > 0)
        {
            groupedRows.push_back(others);
        }

        return groupedRows;
    }

    char symbolForAngle(
        double angle,
        const vector<PieSlice>& slices
    )
    {
        double cursor = 0;

        for (const PieSlice& slice : slices)
        {
            cursor += slice.percentage;

            if (angle <= cursor)
            {
                return slice.symbol;
            }
        }

        return slices.back().symbol;
    }

    void printPieChart(
        string title,
        vector<PieSlice> slices,
        bool moneyValue
    )
    {
        const double PI = 3.14159265358979323846;

        printVisualizationTitle(title);

        if (slices.empty())
        {
            displayInfo("No data available for pie chart.");
            return;
        }

        for (int y = -7; y <= 7; y++)
        {
            margin();
            cout << "      ";

            for (int x = -15; x <= 15; x++)
            {
                double normalized =
                    (x * x) / 225.0 +
                    (y * y) / 49.0;

                if (normalized > 1.0)
                {
                    cout << " ";
                    continue;
                }

                if (normalized < 0.12)
                {
                    cout << " ";
                    continue;
                }

                double angle = atan2(
                    static_cast<double>(-y),
                    static_cast<double>(x)
                );

                if (angle < 0)
                {
                    angle += 2 * PI;
                }

                double percentageAngle =
                    (angle / (2 * PI)) * 100;

                cout << symbolForAngle(
                    percentageAngle,
                    slices
                );
            }

            cout << endl;
        }

        cout << "\n";

        for (const PieSlice& slice : slices)
        {
            margin();
            cout << slice.symbol
                << " = "
                << left
                << setw(24) << fitText(slice.label, 23)
                << setw(15)
                << (moneyValue ? moneyText(slice.value) : numberText(slice.value))
                << percentText(slice.percentage)
                << endl;
        }

        cout << "\n";
    }
}

//=====================================================
// Report Management Menu
//=====================================================

void reportManagementMenu(sql::Connection* con)
{
    int choice;

    do
    {
        clearScreen();

        displayHeader("REPORT MANAGEMENT");
        displayPath("HOME > REPORT MANAGEMENT");

        margin();
        cout << "1. Sales Report\n";

        margin();
        cout << "2. Product Report\n";

        margin();
        cout << "3. Customer Report\n";

        margin();
        cout << "4. Inventory Report\n";

        margin();
        cout << "5. Sales Analytics\n";

        margin();
        cout << "6. Back\n\n";

        readChoice("Enter Choice : ", choice, 1, 6);

        switch (choice)
        {
        case 1:
            salesReportMenu(con);
            break;

        case 2:
            productReportMenu(con);
            break;

        case 3:
            customerReportMenu(con);
            break;

        case 4:
            inventoryReportMenu(con);
            break;

        case 5:
            salesAnalyticsMenu(con);
            break;

        case 6:
            return;
        }

    } while (true);
}

void salesReportMenu(sql::Connection* con)
{
    int choice;

    do
    {
        clearScreen();

        displayHeader("SALES REPORT");
        displayPath("HOME > REPORT MANAGEMENT > SALES REPORT");

        margin();
        cout << "1. Daily Sales Report\n";

        margin();
        cout << "2. Monthly Sales Report\n";

        margin();
        cout << "3. Sales By Payment Method\n";

        margin();
        cout << "4. Back\n\n";

        readChoice("Enter Choice : ", choice, 1, 4);

        switch (choice)
        {
        case 1:
            dailySalesReport(con);
            pauseScreen();
            break;

        case 2:
            monthlySalesReport(con);
            pauseScreen();
            break;

        case 3:
            salesByPaymentMethodReport(con);
            pauseScreen();
            break;

        case 4:
            return;
        }

    } while (true);
}

void productReportMenu(sql::Connection* con)
{
    int choice;

    do
    {
        clearScreen();

        displayHeader("PRODUCT REPORT");
        displayPath("HOME > REPORT MANAGEMENT > PRODUCT REPORT");

        margin();
        cout << "1. Best Selling Products\n";

        margin();
        cout << "2. Product Sales Summary\n";

        margin();
        cout << "3. Best Selling Categories\n";

        margin();
        cout << "4. Back\n\n";

        readChoice("Enter Choice : ", choice, 1, 4);

        switch (choice)
        {
        case 1:
            bestSellingProductsReport(con);
            pauseScreen();
            break;

        case 2:
            productSalesSummaryReport(con);
            pauseScreen();
            break;

        case 3:
            bestSellingCategoriesReport(con);
            pauseScreen();
            break;

        case 4:
            return;
        }

    } while (true);
}

void customerReportMenu(sql::Connection* con)
{
    int choice;

    do
    {
        clearScreen();

        displayHeader("CUSTOMER REPORT");
        displayPath("HOME > REPORT MANAGEMENT > CUSTOMER REPORT");

        margin();
        cout << "1. Top Customers\n";

        margin();
        cout << "2. Customer Purchase Summary\n";

        margin();
        cout << "3. Back\n\n";

        readChoice("Enter Choice : ", choice, 1, 3);

        switch (choice)
        {
        case 1:
            topCustomersReport(con);
            pauseScreen();
            break;

        case 2:
            customerPurchaseSummaryReport(con);
            pauseScreen();
            break;

        case 3:
            return;
        }

    } while (true);
}

void inventoryReportMenu(sql::Connection* con)
{
    int choice;

    do
    {
        clearScreen();

        displayHeader("INVENTORY REPORT");
        displayPath("HOME > REPORT MANAGEMENT > INVENTORY REPORT");

        margin();
        cout << "1. Current Inventory\n";

        margin();
        cout << "2. Low Stock Alert\n";

        margin();
        cout << "3. Out Of Stock Products\n";

        margin();
        cout << "4. Back\n\n";

        readChoice("Enter Choice : ", choice, 1, 4);

        switch (choice)
        {
        case 1:
            currentInventoryReport(con);
            pauseScreen();
            break;

        case 2:
            lowStockAlertReport(con);
            pauseScreen();
            break;

        case 3:
            outOfStockProductsReport(con);
            pauseScreen();
            break;

        case 4:
            return;
        }

    } while (true);
}

void salesAnalyticsMenu(sql::Connection* con)
{
    int choice;

    do
    {
        clearScreen();

        displayHeader("SALES ANALYTICS");
        displayPath("HOME > REPORT MANAGEMENT > SALES ANALYTICS");

        margin();
        cout << "1. Revenue By Category\n";

        margin();
        cout << "2. Top 5 Best Selling Products\n";

        margin();
        cout << "3. Back\n\n";

        readChoice("Enter Choice : ", choice, 1, 3);

        switch (choice)
        {
        case 1:
            revenueByCategoryAnalytics(con);
            pauseScreen();
            break;

        case 2:
            topFiveBestSellingProducts(con);
            pauseScreen();
            break;

        case 3:
            return;
        }

    } while (true);
}

//=====================================================
// Sales Reports
//=====================================================

void dailySalesReport(sql::Connection* con)
{
    clearScreen();

    displayHeader("DAILY SALES REPORT");
    displayPath(
        "HOME > REPORT MANAGEMENT > SALES REPORT > DAILY SALES REPORT"
    );

    vector<DailySalesRow> rows;
    int totalOrders = 0;
    double totalRevenue = 0;

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT DATE(OrderDate) AS SalesDate, "
            "COUNT(OrderID) AS TotalOrders, "
            "COALESCE(SUM(TotalAmount), 0) AS Revenue "
            "FROM ORDERS "
            "WHERE OrderStatus = ? "
            "GROUP BY DATE(OrderDate) "
            "ORDER BY SalesDate DESC"
        );

        pstmt->setString(1, "COMPLETED");
        res = pstmt->executeQuery();

        while (res->next())
        {
            DailySalesRow row;

            row.date = res->getString("SalesDate");
            row.orders = res->getInt("TotalOrders");
            row.revenue = res->getDouble("Revenue");

            rows.push_back(row);
            totalOrders += row.orders;
            totalRevenue += row.revenue;
        }

        closeQuery(res, pstmt);

        double averagePerDay =
            rows.empty() ? 0 : totalRevenue / rows.size();

        printSummary({
            { "Total Revenue", moneyText(totalRevenue) },
            { "Total Orders", to_string(totalOrders) },
            { "Average Revenue Per Day", moneyText(averagePerDay) }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(18) << "Date"
            << setw(18) << "Total Orders"
            << setw(18) << "Revenue"
            << endl;

        printLine(54);

        for (const DailySalesRow& row : rows)
        {
            margin();
            cout << left
                << setw(18) << row.date
                << setw(18) << row.orders
                << setw(18) << moneyText(row.revenue)
                << endl;
        }

        printLine(54);

        if (rows.empty())
        {
            displayInfo("No completed daily sales found.");
        }

        vector<pair<string, double>> chartData;

        for (const DailySalesRow& row : rows)
        {
            chartData.push_back({ row.date, row.revenue });
        }

        cout << "\n";
        printBarChart("Revenue Per Day", chartData, true);
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}

void monthlySalesReport(sql::Connection* con)
{
    clearScreen();

    displayHeader("MONTHLY SALES REPORT");
    displayPath(
        "HOME > REPORT MANAGEMENT > SALES REPORT > MONTHLY SALES REPORT"
    );

    vector<MonthlySalesRow> rows;
    int totalOrders = 0;
    double totalRevenue = 0;
    string bestMonth = "N/A";
    double bestRevenue = 0;

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT DATE_FORMAT(OrderDate, '%Y-%m') AS SalesMonth, "
            "COUNT(OrderID) AS TotalOrders, "
            "COALESCE(SUM(TotalAmount), 0) AS Revenue "
            "FROM ORDERS "
            "WHERE OrderStatus = ? "
            "GROUP BY DATE_FORMAT(OrderDate, '%Y-%m') "
            "ORDER BY SalesMonth DESC"
        );

        pstmt->setString(1, "COMPLETED");
        res = pstmt->executeQuery();

        while (res->next())
        {
            MonthlySalesRow row;

            row.month = res->getString("SalesMonth");
            row.orders = res->getInt("TotalOrders");
            row.revenue = res->getDouble("Revenue");

            if (row.revenue > bestRevenue || rows.empty())
            {
                bestRevenue = row.revenue;
                bestMonth = row.month;
            }

            rows.push_back(row);
            totalOrders += row.orders;
            totalRevenue += row.revenue;
        }

        closeQuery(res, pstmt);

        printSummary({
            { "Total Revenue", moneyText(totalRevenue) },
            { "Total Orders", to_string(totalOrders) },
            { "Best Month", bestMonth }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(18) << "Month"
            << setw(18) << "Orders"
            << setw(18) << "Revenue"
            << endl;

        printLine(54);

        for (const MonthlySalesRow& row : rows)
        {
            margin();
            cout << left
                << setw(18) << row.month
                << setw(18) << row.orders
                << setw(18) << moneyText(row.revenue)
                << endl;
        }

        printLine(54);

        if (rows.empty())
        {
            displayInfo("No completed monthly sales found.");
        }

        vector<pair<string, double>> chartData;

        for (const MonthlySalesRow& row : rows)
        {
            chartData.push_back({ row.month, row.revenue });
        }

        cout << "\n";
        printBarChart("Monthly Revenue Trend", chartData, true);
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}

void salesByPaymentMethodReport(sql::Connection* con)
{
    clearScreen();

    displayHeader("SALES BY PAYMENT METHOD");
    displayPath(
        "HOME > REPORT MANAGEMENT > SALES REPORT > SALES BY PAYMENT METHOD"
    );

    vector<PaymentRow> rows;
    int totalTransactions = 0;
    double totalRevenue = 0;

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT PaymentMethod, "
            "COUNT(OrderID) AS Transactions, "
            "COALESCE(SUM(TotalAmount), 0) AS Revenue "
            "FROM ORDERS "
            "WHERE OrderStatus = ? "
            "GROUP BY PaymentMethod "
            "ORDER BY Revenue DESC"
        );

        pstmt->setString(1, "COMPLETED");
        res = pstmt->executeQuery();

        while (res->next())
        {
            PaymentRow row;

            row.method = res->getString("PaymentMethod");
            row.transactions = res->getInt("Transactions");
            row.revenue = res->getDouble("Revenue");

            rows.push_back(row);
            totalTransactions += row.transactions;
            totalRevenue += row.revenue;
        }

        closeQuery(res, pstmt);

        for (PaymentRow& row : rows)
        {
            row.percentage =
                totalRevenue > 0 ? (row.revenue / totalRevenue) * 100 : 0;
        }

        printSummary({
            { "Total Revenue", moneyText(totalRevenue) },
            { "Total Transactions", to_string(totalTransactions) }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(25) << "Payment Method"
            << setw(18) << "Transactions"
            << setw(18) << "Revenue"
            << setw(15) << "Percentage"
            << endl;

        printLine(76);

        for (const PaymentRow& row : rows)
        {
            margin();
            cout << left
                << setw(25) << fitText(row.method, 24)
                << setw(18) << row.transactions
                << setw(18) << moneyText(row.revenue)
                << setw(15) << percentText(row.percentage)
                << endl;
        }

        printLine(76);

        if (rows.empty())
        {
            displayInfo("No completed payment transactions found.");
        }

        vector<pair<string, double>> pieData;

        for (const PaymentRow& row : rows)
        {
            pieData.push_back({ row.method, row.revenue });
        }

        cout << "\n";
        printPieChart(
            "Large ASCII Pie Chart - Revenue Distribution",
            buildDirectPieSlices(pieData),
            true
        );
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}

//=====================================================
// Product Reports
//=====================================================

void bestSellingProductsReport(sql::Connection* con)
{
    clearScreen();

    displayHeader("BEST SELLING PRODUCTS");
    displayPath(
        "HOME > REPORT MANAGEMENT > PRODUCT REPORT > BEST SELLING PRODUCTS"
    );

    vector<ProductUnitsRow> rows;
    int totalProductsSold = 0;
    string topProduct = "N/A";

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT p.ProductID, "
            "p.ProductName, "
            "COALESCE(SUM(od.Quantity), 0) AS UnitsSold "
            "FROM ORDERDETAILS od "
            "INNER JOIN ORDERS o ON od.OrderID = o.OrderID "
            "INNER JOIN PRODUCT p ON od.ProductID = p.ProductID "
            "WHERE o.OrderStatus = ? "
            "GROUP BY p.ProductID, p.ProductName "
            "ORDER BY UnitsSold DESC, p.ProductName"
        );

        pstmt->setString(1, "COMPLETED");
        res = pstmt->executeQuery();

        while (res->next())
        {
            ProductUnitsRow row;

            row.productID = res->getString("ProductID");
            row.productName = res->getString("ProductName");
            row.unitsSold = res->getInt("UnitsSold");

            if (rows.empty())
            {
                topProduct = row.productName;
            }

            rows.push_back(row);
            totalProductsSold += row.unitsSold;
        }

        closeQuery(res, pstmt);

        printSummary({
            { "Total Products Sold", to_string(totalProductsSold) },
            { "Top Product", topProduct }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(15) << "Product ID"
            << setw(35) << "Product Name"
            << setw(15) << "Units Sold"
            << endl;

        printLine(65);

        for (const ProductUnitsRow& row : rows)
        {
            margin();
            cout << left
                << setw(15) << row.productID
                << setw(35) << fitText(row.productName, 34)
                << setw(15) << row.unitsSold
                << endl;
        }

        printLine(65);

        if (rows.empty())
        {
            displayInfo("No completed product sales found.");
        }

        vector<pair<string, double>> chartData;

        for (const ProductUnitsRow& row : rows)
        {
            chartData.push_back({ row.productName, row.unitsSold });
        }

        cout << "\n";
        printBarChart("Products Ranked By Quantity Sold", chartData, false);
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}

void productSalesSummaryReport(sql::Connection* con)
{
    clearScreen();

    displayHeader("PRODUCT SALES SUMMARY");
    displayPath(
        "HOME > REPORT MANAGEMENT > PRODUCT REPORT > PRODUCT SALES SUMMARY"
    );

    vector<ProductRevenueRow> rows;
    int totalUnitsSold = 0;
    double totalRevenue = 0;

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT p.ProductID, "
            "p.ProductName, "
            "COALESCE(SUM(od.Quantity), 0) AS UnitsSold, "
            "COALESCE(SUM(od.Subtotal), 0) AS Revenue "
            "FROM ORDERDETAILS od "
            "INNER JOIN ORDERS o ON od.OrderID = o.OrderID "
            "INNER JOIN PRODUCT p ON od.ProductID = p.ProductID "
            "WHERE o.OrderStatus = ? "
            "GROUP BY p.ProductID, p.ProductName "
            "ORDER BY Revenue DESC, p.ProductName"
        );

        pstmt->setString(1, "COMPLETED");
        res = pstmt->executeQuery();

        while (res->next())
        {
            ProductRevenueRow row;

            row.productID = res->getString("ProductID");
            row.productName = res->getString("ProductName");
            row.unitsSold = res->getInt("UnitsSold");
            row.revenue = res->getDouble("Revenue");

            rows.push_back(row);
            totalUnitsSold += row.unitsSold;
            totalRevenue += row.revenue;
        }

        closeQuery(res, pstmt);

        printSummary({
            { "Total Revenue", moneyText(totalRevenue) },
            { "Total Units Sold", to_string(totalUnitsSold) }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(15) << "Product ID"
            << setw(35) << "Product Name"
            << setw(15) << "Units Sold"
            << setw(18) << "Revenue"
            << endl;

        printLine(83);

        for (const ProductRevenueRow& row : rows)
        {
            margin();
            cout << left
                << setw(15) << row.productID
                << setw(35) << fitText(row.productName, 34)
                << setw(15) << row.unitsSold
                << setw(18) << moneyText(row.revenue)
                << endl;
        }

        printLine(83);

        if (rows.empty())
        {
            displayInfo("No completed product revenue found.");
        }

        vector<pair<string, double>> chartData;

        for (const ProductRevenueRow& row : rows)
        {
            chartData.push_back({ row.productName, row.revenue });
        }

        cout << "\n";
        printBarChart("Product Revenue Ranking", chartData, true);
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}

void bestSellingCategoriesReport(sql::Connection* con)
{
    clearScreen();

    displayHeader("BEST SELLING CATEGORIES");
    displayPath(
        "HOME > REPORT MANAGEMENT > PRODUCT REPORT > BEST SELLING CATEGORIES"
    );

    vector<CategoryRow> rows;
    int totalUnitsSold = 0;
    string mostPopularCategory = "N/A";

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT c.CategoryName, "
            "COALESCE(SUM(od.Quantity), 0) AS UnitsSold "
            "FROM ORDERDETAILS od "
            "INNER JOIN ORDERS o ON od.OrderID = o.OrderID "
            "INNER JOIN PRODUCT p ON od.ProductID = p.ProductID "
            "INNER JOIN PRODUCTCATEGORY c ON p.CategoryID = c.CategoryID "
            "WHERE o.OrderStatus = ? "
            "GROUP BY c.CategoryName "
            "ORDER BY UnitsSold DESC, c.CategoryName"
        );

        pstmt->setString(1, "COMPLETED");
        res = pstmt->executeQuery();

        while (res->next())
        {
            CategoryRow row;

            row.category = res->getString("CategoryName");
            row.unitsSold = res->getInt("UnitsSold");

            if (rows.empty())
            {
                mostPopularCategory = row.category;
            }

            rows.push_back(row);
            totalUnitsSold += row.unitsSold;
        }

        closeQuery(res, pstmt);

        for (CategoryRow& row : rows)
        {
            row.percentage =
                totalUnitsSold > 0
                ? (static_cast<double>(row.unitsSold) / totalUnitsSold) * 100
                : 0;
        }

        vector<CategoryRow> groupedRows =
            groupCategoriesByUnits(rows);

        printSummary({
            { "Most Popular Category", mostPopularCategory }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(30) << "Category"
            << setw(18) << "Units Sold"
            << setw(15) << "Percentage"
            << endl;

        printLine(63);

        for (const CategoryRow& row : groupedRows)
        {
            margin();
            cout << left
                << setw(30) << fitText(row.category, 29)
                << setw(18) << row.unitsSold
                << setw(15) << percentText(row.percentage)
                << endl;
        }

        printLine(63);

        if (groupedRows.empty())
        {
            displayInfo("No completed category sales found.");
        }

        vector<pair<string, double>> pieData;

        for (const CategoryRow& row : groupedRows)
        {
            pieData.push_back({ row.category, row.unitsSold });
        }

        cout << "\n";
        printPieChart(
            "Top 3 Categories + Others",
            buildDirectPieSlices(pieData),
            false
        );
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}

//=====================================================
// Customer Reports
//=====================================================

void topCustomersReport(sql::Connection* con)
{
    clearScreen();

    displayHeader("TOP CUSTOMERS");
    displayPath(
        "HOME > REPORT MANAGEMENT > CUSTOMER REPORT > TOP CUSTOMERS"
    );

    vector<CustomerRow> rows;
    string highestCustomer = "N/A";
    double totalSpending = 0;

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT c.CustomerID, "
            "c.CustomerName, "
            "COUNT(o.OrderID) AS Orders, "
            "COALESCE(SUM(o.TotalAmount), 0) AS Spending "
            "FROM CUSTOMER c "
            "INNER JOIN ORDERS o ON c.CustomerID = o.CustomerID "
            "WHERE o.OrderStatus = ? "
            "GROUP BY c.CustomerID, c.CustomerName "
            "ORDER BY Spending DESC, Orders DESC"
        );

        pstmt->setString(1, "COMPLETED");
        res = pstmt->executeQuery();

        while (res->next())
        {
            CustomerRow row;

            row.customerID = res->getString("CustomerID");
            row.customerName = res->getString("CustomerName");
            row.orders = res->getInt("Orders");
            row.spending = res->getDouble("Spending");

            if (rows.empty())
            {
                highestCustomer = row.customerName;
            }

            rows.push_back(row);
            totalSpending += row.spending;
        }

        closeQuery(res, pstmt);

        printSummary({
            { "Highest Spending Customer", highestCustomer },
            { "Total Spending", moneyText(totalSpending) }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(15) << "Customer ID"
            << setw(35) << "Customer Name"
            << setw(12) << "Orders"
            << setw(18) << "Spending"
            << endl;

        printLine(80);

        for (const CustomerRow& row : rows)
        {
            margin();
            cout << left
                << setw(15) << row.customerID
                << setw(35) << fitText(row.customerName, 34)
                << setw(12) << row.orders
                << setw(18) << moneyText(row.spending)
                << endl;
        }

        printLine(80);

        if (rows.empty())
        {
            displayInfo("No completed customer purchases found.");
        }

        vector<pair<string, double>> chartData;

        for (const CustomerRow& row : rows)
        {
            chartData.push_back({ row.customerName, row.spending });
        }

        cout << "\n";
        printBarChart("Customers Ranked By Spending", chartData, true);
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}

void customerPurchaseSummaryReport(sql::Connection* con)
{
    clearScreen();

    displayHeader("CUSTOMER PURCHASE SUMMARY");
    displayPath(
        "HOME > REPORT MANAGEMENT > CUSTOMER REPORT > CUSTOMER PURCHASE SUMMARY"
    );

    vector<CustomerRow> rows;
    int totalOrders = 0;

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT c.CustomerID, "
            "c.CustomerName, "
            "COUNT(o.OrderID) AS Orders, "
            "COALESCE(SUM(o.TotalAmount), 0) AS Spending "
            "FROM CUSTOMER c "
            "LEFT JOIN ORDERS o ON c.CustomerID = o.CustomerID "
            "AND o.OrderStatus = ? "
            "GROUP BY c.CustomerID, c.CustomerName "
            "ORDER BY Spending DESC, c.CustomerID"
        );

        pstmt->setString(1, "COMPLETED");
        res = pstmt->executeQuery();

        while (res->next())
        {
            CustomerRow row;

            row.customerID = res->getString("CustomerID");
            row.customerName = res->getString("CustomerName");
            row.orders = res->getInt("Orders");
            row.spending = res->getDouble("Spending");

            rows.push_back(row);
            totalOrders += row.orders;
        }

        closeQuery(res, pstmt);

        printSummary({
            { "Total Customers", to_string(rows.size()) },
            { "Total Orders", to_string(totalOrders) }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(15) << "Customer ID"
            << setw(35) << "Customer Name"
            << setw(12) << "Orders"
            << setw(18) << "Spending"
            << endl;

        printLine(80);

        for (const CustomerRow& row : rows)
        {
            margin();
            cout << left
                << setw(15) << row.customerID
                << setw(35) << fitText(row.customerName, 34)
                << setw(12) << row.orders
                << setw(18) << moneyText(row.spending)
                << endl;
        }

        printLine(80);

        if (rows.empty())
        {
            displayInfo("No customers found.");
        }

        vector<pair<string, double>> chartData;

        for (const CustomerRow& row : rows)
        {
            chartData.push_back({ row.customerName, row.spending });
        }

        cout << "\n";
        printBarChart("Customer Spending Ranking", chartData, true);
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}

//=====================================================
// Inventory Reports
//=====================================================

void currentInventoryReport(sql::Connection* con)
{
    clearScreen();

    displayHeader("CURRENT INVENTORY");
    displayPath(
        "HOME > REPORT MANAGEMENT > INVENTORY REPORT > CURRENT INVENTORY"
    );

    vector<InventoryRow> rows;
    int totalStockUnits = 0;

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT ProductID, "
            "ProductName, "
            "StockQuantity "
            "FROM PRODUCT "
            "WHERE Status = ? "
            "ORDER BY ProductName"
        );

        pstmt->setString(1, "ACTIVE");
        res = pstmt->executeQuery();

        while (res->next())
        {
            InventoryRow row;

            row.productID = res->getString("ProductID");
            row.productName = res->getString("ProductName");
            row.stock = res->getInt("StockQuantity");

            rows.push_back(row);
            totalStockUnits += row.stock;
        }

        closeQuery(res, pstmt);

        printSummary({
            { "Active Products", to_string(rows.size()) },
            { "Total Stock Units", to_string(totalStockUnits) }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(15) << "Product ID"
            << setw(35) << "Product Name"
            << setw(12) << "Stock"
            << endl;

        printLine(62);

        for (const InventoryRow& row : rows)
        {
            margin();
            cout << left
                << setw(15) << row.productID
                << setw(35) << fitText(row.productName, 34)
                << setw(12) << row.stock
                << endl;
        }

        printLine(62);

        if (rows.empty())
        {
            displayInfo("No active products found.");
        }

        vector<pair<string, double>> chartData;

        for (const InventoryRow& row : rows)
        {
            chartData.push_back({ row.productName, row.stock });
        }

        cout << "\n";
        printBarChart("Stock Levels By Product", chartData, false);
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}

void lowStockAlertReport(sql::Connection* con)
{
    clearScreen();

    displayHeader("LOW STOCK ALERT");
    displayPath(
        "HOME > REPORT MANAGEMENT > INVENTORY REPORT > LOW STOCK ALERT"
    );

    vector<InventoryRow> rows;

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT ProductID, "
            "ProductName, "
            "StockQuantity "
            "FROM PRODUCT "
            "WHERE Status = ? "
            "AND StockQuantity <= ? "
            "ORDER BY StockQuantity ASC, ProductName"
        );

        pstmt->setString(1, "ACTIVE");
        pstmt->setInt(2, 7);
        res = pstmt->executeQuery();

        while (res->next())
        {
            InventoryRow row;

            row.productID = res->getString("ProductID");
            row.productName = res->getString("ProductName");
            row.stock = res->getInt("StockQuantity");

            rows.push_back(row);
        }

        closeQuery(res, pstmt);

        printSummary({
            { "Number Of Low Stock Products", to_string(rows.size()) }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(15) << "Product ID"
            << setw(35) << "Product Name"
            << setw(12) << "Stock"
            << endl;

        printLine(62);

        for (const InventoryRow& row : rows)
        {
            margin();
            cout << left
                << setw(15) << row.productID
                << setw(35) << fitText(row.productName, 34)
                << setw(12) << row.stock
                << endl;
        }

        printLine(62);

        if (rows.empty())
        {
            displaySuccess("No low stock products found.");
        }

        vector<pair<string, double>> chartData;

        for (const InventoryRow& row : rows)
        {
            chartData.push_back({ row.productName, row.stock });
        }

        cout << "\n";
        printBarChart("Low Stock Level Comparison", chartData, false);
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}

void outOfStockProductsReport(sql::Connection* con)
{
    clearScreen();

    displayHeader("OUT OF STOCK PRODUCTS");
    displayPath(
        "HOME > REPORT MANAGEMENT > INVENTORY REPORT > OUT OF STOCK PRODUCTS"
    );

    vector<InventoryRow> rows;

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT ProductID, "
            "ProductName, "
            "StockQuantity "
            "FROM PRODUCT "
            "WHERE Status = ? "
            "AND StockQuantity = ? "
            "ORDER BY ProductName"
        );

        pstmt->setString(1, "ACTIVE");
        pstmt->setInt(2, 0);
        res = pstmt->executeQuery();

        while (res->next())
        {
            InventoryRow row;

            row.productID = res->getString("ProductID");
            row.productName = res->getString("ProductName");
            row.stock = res->getInt("StockQuantity");

            rows.push_back(row);
        }

        closeQuery(res, pstmt);

        printSummary({
            { "Total Out Of Stock Products", to_string(rows.size()) }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(15) << "Product ID"
            << setw(35) << "Product Name"
            << endl;

        printLine(50);

        for (const InventoryRow& row : rows)
        {
            margin();
            cout << left
                << setw(15) << row.productID
                << setw(35) << fitText(row.productName, 34)
                << endl;
        }

        printLine(50);

        cout << "\n";
        displaySection("VISUALIZATION");

        if (rows.empty())
        {
            displaySuccess("No out of stock active products found.");
        }
        else
        {
            margin();
            cout << "Out Of Stock Alert Board" << endl;

            printLine(58);

            for (const InventoryRow& row : rows)
            {
                margin();
                cout << "[OUT] "
                    << left
                    << setw(12) << row.productID
                    << fitText(row.productName, 34)
                    << endl;
            }

            printLine(58);
        }
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}

//=====================================================
// Sales Analytics
//=====================================================

void revenueByCategoryAnalytics(sql::Connection* con)
{
    clearScreen();

    displayHeader("REVENUE BY CATEGORY");
    displayPath(
        "HOME > REPORT MANAGEMENT > SALES ANALYTICS > REVENUE BY CATEGORY"
    );

    vector<CategoryRow> rows;
    double totalRevenue = 0;

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT c.CategoryName, "
            "COALESCE(SUM(od.Subtotal), 0) AS Revenue "
            "FROM ORDERDETAILS od "
            "INNER JOIN ORDERS o ON od.OrderID = o.OrderID "
            "INNER JOIN PRODUCT p ON od.ProductID = p.ProductID "
            "INNER JOIN PRODUCTCATEGORY c ON p.CategoryID = c.CategoryID "
            "WHERE o.OrderStatus = ? "
            "GROUP BY c.CategoryName "
            "ORDER BY Revenue DESC, c.CategoryName"
        );

        pstmt->setString(1, "COMPLETED");
        res = pstmt->executeQuery();

        while (res->next())
        {
            CategoryRow row;

            row.category = res->getString("CategoryName");
            row.revenue = res->getDouble("Revenue");

            rows.push_back(row);
            totalRevenue += row.revenue;
        }

        closeQuery(res, pstmt);

        for (CategoryRow& row : rows)
        {
            row.percentage =
                totalRevenue > 0 ? (row.revenue / totalRevenue) * 100 : 0;
        }

        vector<CategoryRow> groupedRows =
            groupCategoriesByRevenue(rows);

        printSummary({
            { "Total Revenue", moneyText(totalRevenue) }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(30) << "Category"
            << setw(18) << "Revenue"
            << setw(15) << "Percentage"
            << endl;

        printLine(63);

        for (const CategoryRow& row : groupedRows)
        {
            margin();
            cout << left
                << setw(30) << fitText(row.category, 29)
                << setw(18) << moneyText(row.revenue)
                << setw(15) << percentText(row.percentage)
                << endl;
        }

        printLine(63);

        if (groupedRows.empty())
        {
            displayInfo("No completed category revenue found.");
        }

        vector<pair<string, double>> pieData;

        for (const CategoryRow& row : groupedRows)
        {
            pieData.push_back({ row.category, row.revenue });
        }

        cout << "\n";
        printPieChart(
            "Top 3 Categories + Others",
            buildDirectPieSlices(pieData),
            true
        );
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}

void topFiveBestSellingProducts(sql::Connection* con)
{
    clearScreen();

    displayHeader("TOP 5 BEST SELLING PRODUCTS");
    displayPath(
        "HOME > REPORT MANAGEMENT > SALES ANALYTICS > TOP 5 BEST SELLING PRODUCTS"
    );

    vector<ProductUnitsRow> rows;
    int totalUnitsSold = 0;

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* res = nullptr;

    try
    {
        pstmt = con->prepareStatement(
            "SELECT p.ProductID, "
            "p.ProductName, "
            "COALESCE(SUM(od.Quantity), 0) AS UnitsSold "
            "FROM ORDERDETAILS od "
            "INNER JOIN ORDERS o ON od.OrderID = o.OrderID "
            "INNER JOIN PRODUCT p ON od.ProductID = p.ProductID "
            "WHERE o.OrderStatus = ? "
            "GROUP BY p.ProductID, p.ProductName "
            "ORDER BY UnitsSold DESC, p.ProductName "
            "LIMIT 5"
        );

        pstmt->setString(1, "COMPLETED");
        res = pstmt->executeQuery();

        while (res->next())
        {
            ProductUnitsRow row;

            row.productID = res->getString("ProductID");
            row.productName = res->getString("ProductName");
            row.unitsSold = res->getInt("UnitsSold");

            rows.push_back(row);
            totalUnitsSold += row.unitsSold;
        }

        closeQuery(res, pstmt);

        printSummary({
            { "Total Units Sold", to_string(totalUnitsSold) }
            });

        printTableTitle("DETAILED DATA TABLE");

        margin();
        cout << left
            << setw(8) << "Rank"
            << setw(15) << "Product ID"
            << setw(35) << "Product Name"
            << setw(15) << "Units Sold"
            << endl;

        printLine(73);

        int rank = 1;

        for (const ProductUnitsRow& row : rows)
        {
            margin();
            cout << left
                << setw(8) << rank
                << setw(15) << row.productID
                << setw(35) << fitText(row.productName, 34)
                << setw(15) << row.unitsSold
                << endl;

            rank++;
        }

        printLine(73);

        if (rows.empty())
        {
            displayInfo("No completed product sales found.");
        }

        vector<pair<string, double>> chartData;

        for (const ProductUnitsRow& row : rows)
        {
            chartData.push_back({ row.productName, row.unitsSold });
        }

        cout << "\n";
        printBarChart(
            "Large ASCII Bar Chart - Top 5 Products",
            chartData,
            false
        );
    }
    catch (sql::SQLException& e)
    {
        closeQuery(res, pstmt);
        displayError(e.what());
    }
}
