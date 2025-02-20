#include <iostream>
#include <sql.h>
#include <sqlext.h>

void checkError(SQLRETURN ret, SQLHANDLE handle, SQLSMALLINT type) {
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        return;
    }

    SQLSMALLINT i = 0;
    SQLINTEGER native;
    SQLCHAR state[7];
    SQLCHAR text[256];
    SQLSMALLINT len;
    SQLRETURN diagRet;

    do {
        diagRet = SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len);
        if (SQL_SUCCEEDED(diagRet)) {
            std::cerr << "ODBC Error: " << state << " (" << i << "): " << text << std::endl;
        }
    } while (diagRet == SQL_SUCCESS);
}

int main() {
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;

    // Allocate an environment handle
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    checkError(ret, env, SQL_HANDLE_ENV);

    // Set the ODBC version environment attribute
    ret = SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    checkError(ret, env, SQL_HANDLE_ENV);

    // Allocate a connection handle
    ret = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    checkError(ret, dbc, SQL_HANDLE_DBC);

    // Set connection attributes
    SQLCHAR connectionString[] = "DRIVER={ODBC Driver 17 for SQL Server};SERVER=SC-SB-FAC-SQL1;DATABASE=modbus;UID=facilitysqluser;PWD=DWpziZ5F;";
    ret = SQLDriverConnect(dbc, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    if (SQL_SUCCEEDED(ret)) {
        std::cout << "Connected to the Facility SQL database successfully!" << std::endl;
    } else {
        checkError(ret, dbc, SQL_HANDLE_DBC);
        return 1;
    }

    // Allocate a statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    checkError(ret, stmt, SQL_HANDLE_STMT);

    // Execute a query
    SQLCHAR query[] = "SELECT * FROM LSS;";
    ret = SQLExecDirect(stmt, query, SQL_NTS);
    if (SQL_SUCCEEDED(ret)) {
        SQLCHAR columnName[256];
        SQLCHAR columnValue[256];
        SQLSMALLINT columns;
        SQLNumResultCols(stmt, &columns);

        while (SQLFetch(stmt) == SQL_SUCCESS) {
            for (SQLUSMALLINT i = 1; i <= columns; i++) {
                SQLGetData(stmt, i, SQL_C_CHAR, columnValue, sizeof(columnValue), NULL);
                SQLDescribeCol(stmt, i, columnName, sizeof(columnName), NULL, NULL, NULL, NULL, NULL);
                std::cout << columnName << ": " << columnValue << std::endl;
            }
            std::cout << std::endl;
        }
    } else {
        checkError(ret, stmt, SQL_HANDLE_STMT);
    }

    // Clean up
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);

    return 0;
}