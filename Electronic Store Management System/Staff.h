#pragma once

#ifndef STAFF_H
#define STAFF_H

#include <mysql_connection.h>
#include <string>

using namespace std;

// =====================================
// Validation Functions
// =====================================

bool isValidName(string name);
bool isValidEmail(string email);
bool isValidPassword(string password);

// =====================================
// Staff Functions
// =====================================

string generateStaffID(sql::Connection* con);

void displayStaffList(sql::Connection* con);
void viewInactiveStaff(sql::Connection* con);

void addStaff(sql::Connection* con);
void viewStaff(sql::Connection* con);
void updateStaff(sql::Connection* con);
void deleteStaff(sql::Connection* con);
void restoreStaff(sql::Connection* con);

// =====================================
// Menu
// =====================================

void staffManagementMenu(sql::Connection* con);

#endif