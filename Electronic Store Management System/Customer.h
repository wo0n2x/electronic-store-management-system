#pragma once
#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <mysql_connection.h>

void customerManagementMenu(sql::Connection* con);

void restoreCustomer(sql::Connection* con);

void displayCustomerList(sql::Connection* con); 
#endif