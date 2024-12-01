#include "AzureDatabase.hpp"

bool AzureDatabase::queryDatabase(const std::string& connectionString, const std::string& query, bool display) {
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;

    // Allocate environment handle
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to allocate environment handle." << std::endl;
        return false;
    }

    // Set the ODBC version environment attribute
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    // Allocate connection handle
    ret = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to allocate connection handle." << std::endl;
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        return false;
    }

    // Connect to the database
    ret = SQLDriverConnect(dbc, NULL, (SQLCHAR*)connectionString.c_str(),
                           SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to connect to database." << std::endl;
        if (display) printSQLError(dbc, SQL_HANDLE_DBC);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        return false;
    }

    // Allocate statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to allocate statement handle." << std::endl;
        if (display) printSQLError(dbc, SQL_HANDLE_DBC);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        return false;
    }

    // Execute SQL query
    ret = SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        SQLCHAR columnData[256];
        SQLSMALLINT columns;
        SQLNumResultCols(stmt, &columns);

        // Display column headers
        std::vector<std::string> columnHeaders;
        for (SQLUSMALLINT i = 1; i <= columns; ++i) {
            SQLCHAR columnName[256];
            SQLSMALLINT nameLength;
            SQLDescribeCol(stmt, i, columnName, sizeof(columnName), &nameLength, NULL, NULL, NULL, NULL);
            columnHeaders.push_back((char*)columnName);
        }
        
        // Print headers
        for (const auto& header : columnHeaders) {
            std::cout << header << "\t";
        }
        std::cout << std::endl;

        // Fetch and display data
        while (SQLFetch(stmt) == SQL_SUCCESS) {
            for (SQLUSMALLINT i = 1; i <= columns; ++i) {
                SQLGetData(stmt, i, SQL_C_CHAR, columnData, sizeof(columnData), NULL);
                std::cout << columnData << "\t";
            }
            std::cout << std::endl;
        }
    } else {
        std::cerr << "Failed to execute query." << std::endl;
        if (display) printSQLError(stmt, SQL_HANDLE_STMT);
    }

    // Free statement handle
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    // Disconnect and free handles
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);

    return true;
}

// Function to retrieve and display error messages
void AzureDatabase::printSQLError(SQLHANDLE handle, SQLSMALLINT handleType) {
    SQLCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER nativeError;
    SQLSMALLINT textLength;
    SQLRETURN ret;

    SQLSMALLINT i = 1;
    while ((ret = SQLGetDiagRec(handleType, handle, i++, sqlState, &nativeError, message, sizeof(message), &textLength)) != SQL_NO_DATA) {
        std::cerr << "SQLSTATE: " << sqlState
                  << ", Native Error Code: " << nativeError
                  << ", Message: " << message << std::endl;
    }
}
