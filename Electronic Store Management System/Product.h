#ifndef PRODUCT_H
#define PRODUCT_H

#include <mysql_connection.h>

void productManagementMenu(sql::Connection* con);

void viewInactiveProducts(sql::Connection* con);
#endif