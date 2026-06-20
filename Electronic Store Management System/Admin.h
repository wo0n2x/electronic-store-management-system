#ifndef ADMIN_H
#define ADMIN_H
#include <string>
#include <mysql_connection.h>
using namespace std;

bool login(sql::Connection* con);
string getPassword();
extern bool isAdmin;
#endif