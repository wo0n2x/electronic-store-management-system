#include "Product.h"
#include "Utility.h"

#include <cctype>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

using namespace std;

void viewProducts(sql::Connection* con);
void searchProduct(sql::Connection* con);
void updateProduct(sql::Connection* con);
void deleteProduct(sql::Connection* con);
void restoreProduct(sql::Connection* con);

namespace
{
    void viewProducts(
        sql::Connection* con,
        string headerTitle,
        string breadcrumbPath
    );

    void viewInactiveProducts(
        sql::Connection* con,
        string headerTitle,
        string breadcrumbPath
    );
}

namespace
{
    const int PRODUCT_TABLE_WIDTH = 101;
    const int INACTIVE_PRODUCT_TABLE_WIDTH = 55;
    const int PRODUCTS_PER_PAGE = 15;

    struct ProductInfo
    {
        string productID;
        string productName;
        string brand;
        double price = 0;
        int stockQty = 0;
        string description;
        int categoryID = 0;
        string categoryName;
    };

    struct InactiveProductRow
    {
        string productID;
        string productName;
    };

    void discardLine()
    {
        cin.ignore(
            numeric_limits<streamsize>::max(),
            '\n'
        );
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

    void printLine(int width = PRODUCT_TABLE_WIDTH)
    {
        margin();
        cout << string(width, '-') << endl;
    }

    void printPrompt(string prompt)
    {
        margin();
        cout << prompt;
    }



    bool confirmAction(string prompt)
    {
        char confirm;

        printPrompt(prompt + " (Y/N): ");
        cin >> confirm;
        discardLine();

        return toupper(confirm) == 'Y';
    }

    bool readChoice(string prompt, int& choice, int minValue, int maxValue)
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

            printPrompt(prompt);
        }

        discardLine();

        return true;
    }

    void readRequiredText(string prompt, string& value)
    {
        printPrompt(prompt);
        getline(cin, value);

        while (value.empty())
        {
            displayError("This field cannot be empty.");
            printPrompt(prompt);
            getline(cin, value);
        }
    }

    bool readPositiveDouble(string prompt, double& value)
    {
        printPrompt(prompt);

        while (!(cin >> value) || value <= 0)
        {
            cin.clear();
            discardLine();

            displayError("Please enter a valid amount greater than 0.");
            printPrompt(prompt);
        }

        discardLine();

        return true;
    }

    bool readNonNegativeInt(string prompt, int& value)
    {
        printPrompt(prompt);

        while (!(cin >> value) || value < 0)
        {
            cin.clear();
            discardLine();

            displayError("Please enter a whole number of 0 or higher.");
            printPrompt(prompt);
        }

        discardLine();

        return true;
    }

    bool parseDouble(string input, double& value)
    {
        istringstream stream(input);
        char extra;

        return (stream >> value) &&
            !(stream >> extra);
    }

    bool parseInt(string input, int& value)
    {
        istringstream stream(input);
        char extra;

        return (stream >> value) &&
            !(stream >> extra);
    }

    bool categoryExists(sql::Connection* con, int categoryID)
    {
        sql::PreparedStatement* stmt = nullptr;
        sql::ResultSet* res = nullptr;

        try
        {
            stmt = con->prepareStatement(
                "SELECT CategoryID "
                "FROM PRODUCTCATEGORY "
                "WHERE CategoryID = ?"
            );

            stmt->setInt(1, categoryID);
            res = stmt->executeQuery();

            bool exists = res->next();

            delete res;
            delete stmt;

            return exists;
        }
        catch (sql::SQLException&)
        {
            delete res;
            delete stmt;

            return false;
        }
    }

    string getCategoryName(sql::Connection* con, int categoryID)
    {
        sql::PreparedStatement* stmt = nullptr;
        sql::ResultSet* res = nullptr;

        try
        {
            stmt = con->prepareStatement(
                "SELECT CategoryName "
                "FROM PRODUCTCATEGORY "
                "WHERE CategoryID = ?"
            );

            stmt->setInt(1, categoryID);
            res = stmt->executeQuery();

            string categoryName =
                "Category " + to_string(categoryID);

            if (res->next())
            {
                categoryName =
                    res->getString("CategoryName");
            }

            delete res;
            delete stmt;

            return categoryName;
        }
        catch (sql::SQLException&)
        {
            delete res;
            delete stmt;

            return "Category " + to_string(categoryID);
        }
    }

    int readValidCategory(sql::Connection* con, string prompt)
    {
        int categoryID;

        printPrompt(prompt);

        while (!(cin >> categoryID) ||
            !categoryExists(con, categoryID))
        {
            cin.clear();
            discardLine();

            displayError("Please select an existing category ID.");
            printPrompt(prompt);
        }

        discardLine();

        return categoryID;
    }

    string readOptionalText(string prompt, string currentValue)
    {
        string input;

        printPrompt(prompt);
        getline(cin, input);

        if (input.empty())
        {
            return currentValue;
        }

        return input;
    }

    double readOptionalPrice(string prompt, double currentValue)
    {
        string input;
        double value;

        while (true)
        {
            printPrompt(prompt);
            getline(cin, input);

            if (input.empty())
            {
                return currentValue;
            }

            if (parseDouble(input, value) && value > 0)
            {
                return value;
            }

            displayError("Please enter a valid amount greater than 0.");
        }
    }

    int readOptionalStock(string prompt, int currentValue)
    {
        string input;
        int value;

        while (true)
        {
            printPrompt(prompt);
            getline(cin, input);

            if (input.empty())
            {
                return currentValue;
            }

            if (parseInt(input, value) && value >= 0)
            {
                return value;
            }

            displayError("Please enter a whole number of 0 or higher.");
        }
    }

    int readOptionalCategory(
        sql::Connection* con,
        string prompt,
        int currentValue
    )
    {
        string input;
        int value;

        while (true)
        {
            printPrompt(prompt);
            getline(cin, input);

            if (input.empty())
            {
                return currentValue;
            }

            if (parseInt(input, value) &&
                categoryExists(con, value))
            {
                return value;
            }

            displayError("Please select an existing category ID.");
        }
    }

    ProductInfo readProductInfo(sql::ResultSet* res)
    {
        ProductInfo product;

        product.productID =
            res->getString("ProductID");

        product.productName =
            res->getString("ProductName");

        product.brand =
            res->getString("Brand");

        product.price =
            res->getDouble("Price");

        product.stockQty =
            res->getInt("StockQuantity");

        product.description =
            res->getString("ProductDescription");

        product.categoryID =
            res->getInt("CategoryID");

        product.categoryName =
            "Category " + to_string(product.categoryID);

        return product;
    }

    void displayProductDetails(ProductInfo product)
    {
        displaySection("PRODUCT DETAILS");

        margin();
        cout << left
            << setw(18) << "Product ID"
            << ": "
            << product.productID
            << endl;

        margin();
        cout << left
            << setw(18) << "Product Name"
            << ": "
            << product.productName
            << endl;

        margin();
        cout << left
            << setw(18) << "Brand"
            << ": "
            << product.brand
            << endl;

        margin();
        cout << left
            << setw(18) << "Price"
            << ": "
            << moneyText(product.price)
            << endl;

        margin();
        cout << left
            << setw(18) << "Stock Quantity"
            << ": "
            << product.stockQty
            << endl;

        margin();
        cout << left
            << setw(18) << "Category"
            << ": "
            << product.categoryName;

        if (product.categoryID > 0)
        {
            cout << " (ID "
                << product.categoryID
                << ")";
        }

        cout << endl;

        margin();
        cout << left
            << setw(18) << "Description"
            << ": "
            << product.description
            << endl;
    }

    void displayProductTableHeader(string title)
    {
        displaySection(title);

        margin();
        cout << left
            << setw(10) << "ID"
            << setw(28) << "Product Name"
            << setw(18) << "Brand"
            << setw(13) << "Price"
            << setw(10) << "Stock"
            << setw(22) << "Category"
            << endl;

        printLine();
    }

    void displayProductTableRow(
        string productID,
        string productName,
        string brand,
        double price,
        int stockQty,
        string categoryName
    )
    {
        margin();
        cout << left
            << setw(10) << fitText(productID, 9)
            << setw(28) << fitText(productName, 27)
            << setw(18) << fitText(brand, 17)
            << setw(13) << moneyText(price)
            << setw(10) << stockQty
            << setw(22) << fitText(categoryName, 21)
            << endl;
    }

    void displayProductTotal(
        string label,
        int count,
        int width = PRODUCT_TABLE_WIDTH
    )
    {
        printLine(width);

        margin();
        cout << label
            << " : "
            << count
            << endl;
    }

    void displayInactiveProductTableHeader()
    {
        displaySection("INACTIVE PRODUCTS");

        margin();
        cout << left
            << setw(15) << "Product ID"
            << setw(40) << "Product Name"
            << endl;

        printLine(INACTIVE_PRODUCT_TABLE_WIDTH);
    }

    void displayInactiveProductTableRow(
        string productID,
        string productName
    )
    {
        margin();
        cout << left
            << setw(15) << fitText(productID, 14)
            << setw(40) << fitText(productName, 39)
            << endl;
    }

    void displayPaginationStatus(
        int currentPage,
        int totalPages,
        int firstItem,
        int lastItem,
        int totalItems
    )
    {
        margin();
        cout << "Page "
            << currentPage + 1
            << " of "
            << totalPages
            << " | Showing "
            << firstItem
            << "-"
            << lastItem
            << " of "
            << totalItems
            << endl;
    }

    char readPaginationCommand()
    {
        char command;

        cout << "\n";
        printPrompt("[N] Next  [P] Previous  [Q] Quit : ");
        cin >> command;
        discardLine();

        return static_cast<char>(
            toupper(static_cast<unsigned char>(command))
        );
    }

    void showActiveProductPage(
        const vector<ProductInfo>& products,
        int currentPage,
        string headerTitle,
        string breadcrumbPath
    )
    {
        clearScreen();
        displayHeader(headerTitle);
        displayPath(breadcrumbPath);

        int totalItems =
            static_cast<int>(products.size());

        int firstIndex =
            currentPage * PRODUCTS_PER_PAGE;

        int lastIndex =
            firstIndex + PRODUCTS_PER_PAGE;

        if (lastIndex > totalItems)
        {
            lastIndex = totalItems;
        }

        displayProductTableHeader("ACTIVE PRODUCTS");

        for (int index = firstIndex; index < lastIndex; index++)
        {
            displayProductTableRow(
                products[index].productID,
                products[index].productName,
                products[index].brand,
                products[index].price,
                products[index].stockQty,
                products[index].categoryName
            );
        }

        displayProductTotal("Total Active Products", totalItems);
    }

    void showInactiveProductPage(
        const vector<InactiveProductRow>& products,
        int currentPage,
        string headerTitle,
        string breadcrumbPath
    )
    {
        clearScreen();
        displayHeader(headerTitle);
        displayPath(breadcrumbPath);

        int totalItems =
            static_cast<int>(products.size());

        int firstIndex =
            currentPage * PRODUCTS_PER_PAGE;

        int lastIndex =
            firstIndex + PRODUCTS_PER_PAGE;

        if (lastIndex > totalItems)
        {
            lastIndex = totalItems;
        }

        displayInactiveProductTableHeader();

        for (int index = firstIndex; index < lastIndex; index++)
        {
            displayInactiveProductTableRow(
                products[index].productID,
                products[index].productName
            );
        }

        displayProductTotal(
            "Total Inactive Products",
            totalItems,
            INACTIVE_PRODUCT_TABLE_WIDTH
        );
    }

    void paginateActiveProducts(
        const vector<ProductInfo>& products,
        string headerTitle,
        string breadcrumbPath
    )
    {
        int totalItems =
            static_cast<int>(products.size());

        if (totalItems == 0)
        {
            clearScreen();
            displayHeader(headerTitle);
            displayPath(breadcrumbPath);
            displayProductTableHeader("ACTIVE PRODUCTS");
            displayProductTotal("Total Active Products", 0);
            return;
        }

        int totalPages =
            (totalItems + PRODUCTS_PER_PAGE - 1) /
            PRODUCTS_PER_PAGE;

        int currentPage = 0;

        while (true)
        {
            showActiveProductPage(
                products,
                currentPage,
                headerTitle,
                breadcrumbPath
            );

            int firstItem =
                currentPage * PRODUCTS_PER_PAGE + 1;

            int lastItem =
                (currentPage + 1) * PRODUCTS_PER_PAGE;

            if (lastItem > totalItems)
            {
                lastItem = totalItems;
            }

            displayPaginationStatus(
                currentPage,
                totalPages,
                firstItem,
                lastItem,
                totalItems
            );

            char command =
                readPaginationCommand();

            if (command == 'Q')
            {
                return;
            }

            if (command == 'N')
            {
                if (currentPage < totalPages - 1)
                {
                    currentPage++;
                }
                else
                {
                    displayInfo("You are already on the last page.");
                }
            }
            else if (command == 'P')
            {
                if (currentPage > 0)
                {
                    currentPage--;
                }
                else
                {
                    displayInfo("You are already on the first page.");
                }
            }
            else
            {
                displayError("Please enter N, P, or Q.");
            }
        }
    }

    void paginateInactiveProducts(
        const vector<InactiveProductRow>& products,
        string headerTitle,
        string breadcrumbPath
    )
    {
        int totalItems =
            static_cast<int>(products.size());

        if (totalItems == 0)
        {
            clearScreen();
            displayHeader(headerTitle);
            displayPath(breadcrumbPath);
            displayInactiveProductTableHeader();
            displayProductTotal(
                "Total Inactive Products",
                0,
                INACTIVE_PRODUCT_TABLE_WIDTH
            );

            return;
        }

        int totalPages =
            (totalItems + PRODUCTS_PER_PAGE - 1) /
            PRODUCTS_PER_PAGE;

        int currentPage = 0;

        while (true)
        {
            showInactiveProductPage(
                products,
                currentPage,
                headerTitle,
                breadcrumbPath
            );

            int firstItem =
                currentPage * PRODUCTS_PER_PAGE + 1;

            int lastItem =
                (currentPage + 1) * PRODUCTS_PER_PAGE;

            if (lastItem > totalItems)
            {
                lastItem = totalItems;
            }

            displayPaginationStatus(
                currentPage,
                totalPages,
                firstItem,
                lastItem,
                totalItems
            );

            char command =
                readPaginationCommand();

            if (command == 'Q')
            {
                return;
            }

            if (command == 'N')
            {
                if (currentPage < totalPages - 1)
                {
                    currentPage++;
                }
                else
                {
                    displayInfo("You are already on the last page.");
                }
            }
            else if (command == 'P')
            {
                if (currentPage > 0)
                {
                    currentPage--;
                }
                else
                {
                    displayInfo("You are already on the first page.");
                }
            }
            else
            {
                displayError("Please enter N, P, or Q.");
            }
        }
    }

    void displayChanges(
        ProductInfo oldProduct,
        ProductInfo newProduct
    )
    {
        displaySection("PRODUCT CHANGES");

        margin();
        cout << left
            << setw(20) << "Field"
            << setw(32) << "Old Value"
            << setw(32) << "New Value"
            << endl;

        margin();
        cout << string(84, '-') << endl;

        int changeCount = 0;

        auto printChange =
            [&changeCount](string field, string oldValue, string newValue)
            {
                if (oldValue != newValue)
                {
                    margin();
                    cout << left
                        << setw(20) << field
                        << setw(32) << fitText(oldValue, 31)
                        << setw(32) << fitText(newValue, 31)
                        << endl;

                    changeCount++;
                }
            };

        printChange(
            "Name",
            oldProduct.productName,
            newProduct.productName
        );

        printChange(
            "Brand",
            oldProduct.brand,
            newProduct.brand
        );

        printChange(
            "Price",
            moneyText(oldProduct.price),
            moneyText(newProduct.price)
        );

        printChange(
            "Stock Qty",
            to_string(oldProduct.stockQty),
            to_string(newProduct.stockQty)
        );

        printChange(
            "Category",
            oldProduct.categoryName,
            newProduct.categoryName
        );

        printChange(
            "Description",
            oldProduct.description,
            newProduct.description
        );

        if (changeCount == 0)
        {
            displayInfo("No fields were changed.");
        }
    }
}

//generate productID
string generateProductID(sql::Connection* con)
{
    sql::Statement* stmt;
    sql::ResultSet* res;

    stmt = con->createStatement();

    res = stmt->executeQuery(
        "SELECT ProductID "
        "FROM PRODUCT "
        "ORDER BY ProductID DESC "
        "LIMIT 1"
    );

    string newID = "P001";

    if (res->next())
    {
        string lastID = res->getString("ProductID");

        int number = stoi(lastID.substr(1));

        number++;

        char buffer[10];

        sprintf_s(buffer, "P%03d", number);

        newID = buffer;
    }

    delete res;
    delete stmt;

    return newID;
}

// =====================================
// View Inactive Products
// =====================================

void viewInactiveProducts(sql::Connection* con)
{
    viewInactiveProducts(
        con,
        "RESTORE PRODUCT",
        "HOME > PRODUCT MANAGEMENT > RESTORE PRODUCT"
    );
}

namespace
{
void viewInactiveProducts(
    sql::Connection* con,
    string headerTitle,
    string breadcrumbPath
)
{
    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT ProductID, ProductName "
            "FROM PRODUCT "
            "WHERE Status='INACTIVE' "
            "ORDER BY ProductID"
        );

        vector<InactiveProductRow> products;

        while (res->next())
        {
            InactiveProductRow product;

            product.productID =
                res->getString("ProductID");

            product.productName =
                res->getString("ProductName");

            products.push_back(product);
        }

        paginateInactiveProducts(
            products,
            headerTitle,
            breadcrumbPath
        );

        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}
}

//display Categories
void displayCategories(sql::Connection* con)
{
    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT * FROM PRODUCTCATEGORY"
        );

        displaySection("AVAILABLE CATEGORIES");

        margin();
        cout << left
            << setw(12) << "ID"
            << setw(35) << "Category Name"
            << endl;

        printLine(47);

        while (res->next())
        {
            margin();
            cout << left
                << setw(12) << res->getInt("CategoryID")
                << setw(35) << res->getString("CategoryName")
                << endl;
        }

        printLine(47);

        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}

// Add Product
void addProduct(sql::Connection* con)
{
    string productID = generateProductID(con);

    string productName;
    string brand;
    string description;

    double price;
    int stockQty;
    int categoryID;

    displayHeader("ADD PRODUCT");
    displayPath("HOME > PRODUCT MANAGEMENT > ADD PRODUCT");

    readRequiredText("Product Name      : ", productName);
    readRequiredText("Brand             : ", brand);
    readPositiveDouble("Price             : ", price);
    readNonNegativeInt("Stock Quantity    : ", stockQty);
    readRequiredText("Description       : ", description);

    cout << "\n";
    displayCategories(con);
    cout << "\n";

    categoryID =
        readValidCategory(con, "Select Category ID : ");

    try
    {
        sql::PreparedStatement* pstmt;

        pstmt = con->prepareStatement(
            "INSERT INTO PRODUCT "
            "(ProductID, ProductName, Brand, Price, "
            "StockQuantity, ProductDescription, CategoryID, Status) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
        );

        pstmt->setString(1, productID);
        pstmt->setString(2, productName);
        pstmt->setString(3, brand);
        pstmt->setDouble(4, price);
        pstmt->setInt(5, stockQty);
        pstmt->setString(6, description);
        pstmt->setInt(7, categoryID);
        pstmt->setString(8, "ACTIVE");

        pstmt->executeUpdate();

        displaySuccess("Product added successfully.");

        margin();
        cout << "Product ID : "
            << productID
            << endl;

        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}

// update Product
void updateProduct(sql::Connection* con)
{
    string productID;

    viewProducts(
        con,
        "UPDATE PRODUCT",
        "HOME > PRODUCT MANAGEMENT > UPDATE PRODUCT"
    );

    cout << "\n";
    printPrompt("Enter Product ID : ");
    cin >> productID;
    discardLine();

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM PRODUCT "
            "WHERE ProductID=? "
            "AND Status='ACTIVE'"
        );

        pstmt->setString(1, productID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            displayError("Product not found or inactive.");

            delete res;
            delete pstmt;

            return;
        }

        ProductInfo product =
            readProductInfo(res);

        ProductInfo oldProduct =
            product;

        delete res;
        delete pstmt;

        product.categoryName =
            getCategoryName(con, product.categoryID);

        oldProduct.categoryName =
            product.categoryName;

        int choice;

        do
        {
            clearScreen();

            displayHeader("PRODUCT CONTROL PANEL");
            displayPath("HOME > PRODUCT MANAGEMENT > UPDATE PRODUCT");

            displayProductDetails(product);

            cout << "\n";
            displaySection("UPDATE OPTIONS");

            margin();
            cout << "1. Product Name\n";
            margin();
            cout << "2. Brand\n";
            margin();
            cout << "3. Price\n";
            margin();
            cout << "4. Stock Quantity\n";
            margin();
            cout << "5. Category\n";
            margin();
            cout << "6. Description\n";
            margin();
            cout << "7. Save Changes\n";
            margin();
            cout << "8. Cancel\n\n";

            readChoice("Select Option : ", choice, 1, 8);

            switch (choice)
            {
            case 1:

                product.productName =
                    readOptionalText(
                        "New Product Name (Enter to keep current): ",
                        product.productName
                    );

                break;

            case 2:

                product.brand =
                    readOptionalText(
                        "New Brand (Enter to keep current)       : ",
                        product.brand
                    );

                break;

            case 3:

                product.price =
                    readOptionalPrice(
                        "New Price (Enter to keep current)       : ",
                        product.price
                    );

                break;

            case 4:

                product.stockQty =
                    readOptionalStock(
                        "New Stock (Enter to keep current)       : ",
                        product.stockQty
                    );

                break;

            case 5:

                cout << "\n";
                displayCategories(con);
                cout << "\n";

                product.categoryID =
                    readOptionalCategory(
                        con,
                        "New Category ID (Enter to keep current) : ",
                        product.categoryID
                    );

                product.categoryName =
                    getCategoryName(con, product.categoryID);

                break;

            case 6:

                product.description =
                    readOptionalText(
                        "New Description (Enter to keep current) : ",
                        product.description
                    );

                break;

            case 7:

                clearScreen();

                displayHeader("SAVE PRODUCT CHANGES");
                displayPath("HOME > PRODUCT MANAGEMENT > UPDATE PRODUCT");

                displayChanges(oldProduct, product);

                cout << "\n";

                if (confirmAction("Save changes?"))
                {
                    pstmt = con->prepareStatement(
                        "UPDATE PRODUCT "
                        "SET ProductName=?, "
                        "Brand=?, "
                        "Price=?, "
                        "StockQuantity=?, "
                        "ProductDescription=?, "
                        "CategoryID=? "
                        "WHERE ProductID=?"
                    );

                    pstmt->setString(1, product.productName);
                    pstmt->setString(2, product.brand);
                    pstmt->setDouble(3, product.price);
                    pstmt->setInt(4, product.stockQty);
                    pstmt->setString(5, product.description);
                    pstmt->setInt(6, product.categoryID);
                    pstmt->setString(7, product.productID);

                    pstmt->executeUpdate();

                    displaySuccess("Product updated successfully.");

                    delete pstmt;
                }
                else
                {
                    displayInfo("Changes discarded.");
                }

                return;

            case 8:

                displayInfo("Update cancelled.");
                return;
            }

        } while (true);
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}

//product Management Menu
void productManagementMenu(sql::Connection* con)
{
    int choice;

    do
    {
        clearScreen();

        displayHeader("PRODUCT MANAGEMENT");
        displayPath("HOME > PRODUCT MANAGEMENT");

        margin();
        cout << "1. Add Product\n";
        margin();
        cout << "2. Update Product\n";
        margin();
        cout << "3. Delete Product\n";
        margin();
        cout << "4. Restore Product\n";
        margin();
        cout << "5. Search Product\n";
        margin();
        cout << "6. View Products\n";
        margin();
        cout << "7. Back\n\n";

        readChoice("Enter Choice: ", choice, 1, 7);

        switch (choice)
        {
        case 1:

            clearScreen();

            addProduct(con);

            pauseScreen();

            break;

        case 2:

            clearScreen();

            updateProduct(con);

            pauseScreen();

            break;

        case 3:

            clearScreen();

            deleteProduct(con);

            pauseScreen();

            break;

        case 4:

            clearScreen();

            restoreProduct(con);

            pauseScreen();

            break;

        case 5:

            clearScreen();

            searchProduct(con);

            pauseScreen();

            break;

        case 6:

            clearScreen();

            viewProducts(con);

            pauseScreen();

            break;

        case 7:

            return;
        }

    } while (true);
}

//Delete Products
void deleteProduct(sql::Connection* con)
{
    string productID;

    viewProducts(
        con,
        "DELETE PRODUCT",
        "HOME > PRODUCT MANAGEMENT > DELETE PRODUCT"
    );

    cout << "\n";
    printPrompt("Enter Product ID : ");
    cin >> productID;
    discardLine();

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM PRODUCT "
            "WHERE ProductID=? "
            "AND Status='ACTIVE'"
        );

        pstmt->setString(1, productID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            displayError("Product not found or already inactive.");

            delete res;
            delete pstmt;

            return;
        }

        ProductInfo product =
            readProductInfo(res);

        delete res;
        delete pstmt;

        product.categoryName =
            getCategoryName(con, product.categoryID);

        clearScreen();

        displayHeader("DELETE PRODUCT");
        displayPath("HOME > PRODUCT MANAGEMENT > DELETE PRODUCT");

        displayProductDetails(product);

        displayWarning("This product will become inactive.");
        displayWarning("It will no longer appear in active product lists.");

        cout << "\n";

        if (!confirmAction("Deactivate this product?"))
        {
            displayInfo("Operation cancelled.");
            return;
        }

        pstmt = con->prepareStatement(
            "UPDATE PRODUCT "
            "SET Status='INACTIVE' "
            "WHERE ProductID=?"
        );

        pstmt->setString(1, productID);

        pstmt->executeUpdate();

        delete pstmt;

        displaySuccess("Product deactivated successfully.");
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}

// =====================================
// Restore Product
// =====================================

void restoreProduct(sql::Connection* con)
{
    string productID;

    viewInactiveProducts(
        con,
        "RESTORE PRODUCT",
        "HOME > PRODUCT MANAGEMENT > RESTORE PRODUCT"
    );

    cout << "\n";
    printPrompt("Enter Product ID : ");
    cin >> productID;
    discardLine();

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT * FROM PRODUCT "
            "WHERE ProductID=? "
            "AND Status='INACTIVE'"
        );

        pstmt->setString(1, productID);

        res = pstmt->executeQuery();

        if (!res->next())
        {
            displayError("Inactive product not found.");

            delete res;
            delete pstmt;

            return;
        }

        ProductInfo product =
            readProductInfo(res);

        delete res;
        delete pstmt;

        product.categoryName =
            getCategoryName(con, product.categoryID);

        clearScreen();

        displayHeader("PRODUCT RECOVERY");
        displayPath("HOME > PRODUCT MANAGEMENT > RESTORE PRODUCT");

        displayProductDetails(product);

        cout << "\n";

        if (!confirmAction("Restore this product?"))
        {
            displayInfo("Operation cancelled.");
            return;
        }

        pstmt = con->prepareStatement(
            "UPDATE PRODUCT "
            "SET Status='ACTIVE' "
            "WHERE ProductID=?"
        );

        pstmt->setString(1, productID);

        pstmt->executeUpdate();

        delete pstmt;

        displaySuccess("Product restored successfully.");
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}

//search Products
void searchProduct(sql::Connection* con)
{
    string productID;

    displayHeader("SEARCH PRODUCT");
    displayPath("HOME > PRODUCT MANAGEMENT > SEARCH PRODUCT");

    printPrompt("Enter Product ID : ");
    cin >> productID;
    discardLine();

    try
    {
        sql::PreparedStatement* pstmt;
        sql::ResultSet* res;

        pstmt = con->prepareStatement(
            "SELECT P.ProductID, "
            "P.ProductName, "
            "P.Brand, "
            "P.Price, "
            "P.StockQuantity, "
            "P.ProductDescription, "
            "C.CategoryName "
            "FROM PRODUCT P "
            "JOIN PRODUCTCATEGORY C "
            "ON P.CategoryID = C.CategoryID "
            "WHERE P.ProductID = ? "
            "AND P.Status='ACTIVE'"
        );

        pstmt->setString(1, productID);

        res = pstmt->executeQuery();

        if (res->next())
        {
            ProductInfo product;

            product.productID =
                res->getString("ProductID");

            product.productName =
                res->getString("ProductName");

            product.brand =
                res->getString("Brand");

            product.price =
                res->getDouble("Price");

            product.stockQty =
                res->getInt("StockQuantity");

            product.description =
                res->getString("ProductDescription");

            product.categoryName =
                res->getString("CategoryName");

            displayProductDetails(product);
        }
        else
        {
            displayError("Product not found.");
        }

        delete res;
        delete pstmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}

//view Products
void viewProducts(sql::Connection* con)
{
    viewProducts(
        con,
        "VIEW PRODUCTS",
        "HOME > PRODUCT MANAGEMENT > VIEW PRODUCTS"
    );
}

namespace
{
void viewProducts(
    sql::Connection* con,
    string headerTitle,
    string breadcrumbPath
)
{
    try
    {
        sql::Statement* stmt;
        sql::ResultSet* res;

        stmt = con->createStatement();

        res = stmt->executeQuery(
            "SELECT P.ProductID, "
            "P.ProductName, "
            "P.Brand, "
            "P.Price, "
            "P.StockQuantity, "
            "C.CategoryName "
            "FROM PRODUCT P "
            "JOIN PRODUCTCATEGORY C "
            "ON P.CategoryID = C.CategoryID "
            "WHERE P.Status='ACTIVE'"
        );

        vector<ProductInfo> products;

        while (res->next())
        {
            ProductInfo product;

            product.productID =
                res->getString("ProductID");

            product.productName =
                res->getString("ProductName");

            product.brand =
                res->getString("Brand");

            product.price =
                res->getDouble("Price");

            product.stockQty =
                res->getInt("StockQuantity");

            product.categoryName =
                res->getString("CategoryName");

            products.push_back(product);
        }

        paginateActiveProducts(
            products,
            headerTitle,
            breadcrumbPath
        );

        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e)
    {
        displayError(e.what());
    }
}
}
