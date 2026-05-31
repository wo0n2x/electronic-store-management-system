#pragma once

#include <mysql_driver.h>
#include <mysql_connection.h>

void orderManagementMenu(sql::Connection* con);

void cancelOrder(sql::Connection* con);

void restoreOrder(sql::Connection* con);