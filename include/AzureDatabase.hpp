#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "Common.hpp"
#include <sql.h>
#include <sqlext.h>

class AzureDatabase {
public:
    static bool queryDatabase(const std::string& connectionString, const std::string& query, bool display = true);

private:
    static void printSQLError(SQLHANDLE handle, SQLSMALLINT handleType);
};

#endif // DATABASE_HPP
