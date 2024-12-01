#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "Common.hpp"
#include <sql.h>
#include <sqlext.h>

class AzureDatabase {
public:
    AzureDatabase();
    ~AzureDatabase();

    bool connect(const std::string& connectionString);
    void closeConnection();
    bool queryExecute(const std::string& query, bool display = true);
    static bool queryDatabase(const std::string& connectionString, const std::string& query, bool display = true);

private:
    SQLHENV env;     // Environment handle
    SQLHDBC dbc;     // Database connection handle
    bool isConnected; // Tracks connection state

    static void printSQLError(SQLHANDLE handle, SQLSMALLINT handleType);
};

#endif // DATABASE_HPP
