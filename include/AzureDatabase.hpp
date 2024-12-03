#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "Common.hpp"
#include "EpitrendBinaryData.hpp"

#include <chrono>
#include <sql.h>
#include <sqlext.h>

class AzureDatabase {
public:
    AzureDatabase();
    ~AzureDatabase();

    bool connect(const std::string& connectionString);
    void closeConnection();
    bool queryExecute(const std::string& query, bool display = true);
    bool copyToSQL(const std::string& tableName, EpitrendBinaryData data);

    static bool queryDatabase(const std::string& connectionString, const std::string& query, bool display = true);

private:
    SQLHENV env;     // Environment handle
    SQLHDBC dbc;     // Database connection handle
    bool isConnected; // Tracks connection state

    static void printSQLError(SQLHANDLE handle, SQLSMALLINT handleType);

    // Helper function
    std::string convertDoubleToDateTime(double excelDays);
};


#endif // DATABASE_HPP
