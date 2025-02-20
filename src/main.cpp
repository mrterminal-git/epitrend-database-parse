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
    SQLCHAR connectionString[] = "DRIVER={ODBC Driver 18 for SQL Server};SERVER=SC-SB-FAC-SQL1;DATABASE=modbus;UID=facilitysqluser;PWD=DWpziZ5F;Encrypt=yes;TrustServerCertificate=yes;";
    SQLCHAR outConnectionString[1024];
    SQLSMALLINT outConnectionStringLen;
    ret = SQLDriverConnect(dbc, NULL, connectionString, SQL_NTS, outConnectionString, sizeof(outConnectionString), &outConnectionStringLen, SQL_DRIVER_NOPROMPT);
    if (SQL_SUCCEEDED(ret)) {
        std::cout << "Connected to the Facility SQL database successfully!" << std::endl;
    } else {
        checkError(ret, dbc, SQL_HANDLE_DBC);
        return 1;
    }

    // Allocate a statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    checkError(ret, stmt, SQL_HANDLE_STMT);

    // // Retrieve and print all table names
    // ret = SQLTables(stmt, NULL, 0, NULL, 0, NULL, 0, (SQLCHAR*)"TABLE", SQL_NTS);
    // if (SQL_SUCCEEDED(ret)) {
    //     SQLCHAR tableCatalog[256];
    //     SQLCHAR tableSchema[256];
    //     SQLCHAR tableName[256];
    //     SQLCHAR tableType[256];
    //     while (SQLFetch(stmt) == SQL_SUCCESS) {
    //         SQLGetData(stmt, 1, SQL_C_CHAR, tableCatalog, sizeof(tableCatalog), NULL);
    //         SQLGetData(stmt, 2, SQL_C_CHAR, tableSchema, sizeof(tableSchema), NULL);
    //         SQLGetData(stmt, 3, SQL_C_CHAR, tableName, sizeof(tableName), NULL);
    //         SQLGetData(stmt, 4, SQL_C_CHAR, tableType, sizeof(tableType), NULL);
    //         std::cout << "Catalog: " << tableCatalog << ", Schema: " << tableSchema << ", Table: " << tableName << ", Type: " << tableType << std::endl;
    //     }
    // } else {
    //     checkError(ret, stmt, SQL_HANDLE_STMT);
    // }

    // Retrieve and print the schema of the "LSS" table
    ret = SQLColumns(stmt, NULL, 0, NULL, 0, (SQLCHAR*)"LSS", SQL_NTS, NULL, 0);
    if (SQL_SUCCEEDED(ret)) {
        SQLCHAR columnName[256];
        SQLCHAR columnType[256];
        SQLINTEGER columnSize;
        SQLSMALLINT decimalDigits;
        SQLSMALLINT nullable;
        while (SQLFetch(stmt) == SQL_SUCCESS) {
            SQLGetData(stmt, 4, SQL_C_CHAR, columnName, sizeof(columnName), NULL);
            SQLGetData(stmt, 6, SQL_C_CHAR, columnType, sizeof(columnType), NULL);
            SQLGetData(stmt, 7, SQL_C_LONG, &columnSize, 0, NULL);
            SQLGetData(stmt, 9, SQL_C_SHORT, &decimalDigits, 0, NULL);
            SQLGetData(stmt, 11, SQL_C_SHORT, &nullable, 0, NULL);
            std::cout << "Column Name: " << columnName << ", Type: " << columnType
                      << ", Size: " << columnSize << ", Decimal Digits: " << decimalDigits
                      << ", Nullable: " << (nullable == SQL_NULLABLE ? "YES" : "NO") << std::endl;
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