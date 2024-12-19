#include "AzureDatabase.hpp"

AzureDatabase::AzureDatabase()
    : env(NULL), dbc(NULL), isConnected(false) {
    // Allocate environment handle
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env) != SQL_SUCCESS) {
        throw std::runtime_error("Failed to allocate environment handle.");
    }

    // Set the ODBC version
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    // Allocate connection handle
    if (SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        throw std::runtime_error("Failed to allocate connection handle.");
    }
}

AzureDatabase::~AzureDatabase() {
    closeConnection();
}

// Connect to Azure SQL DB
bool AzureDatabase::connect(const std::string& connectionString) {
    if (isConnected) {
        return true; // Already connected
    }

    // Connect to the database
    SQLRETURN ret = SQLDriverConnect(dbc, NULL, (SQLCHAR*)connectionString.c_str(),
                                     SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to connect to database." << std::endl;
        printSQLError(dbc, SQL_HANDLE_DBC);
        return false;
    }

    isConnected = true;
    return true;
}

// Disconnect from current Azure SQL DB
void AzureDatabase::closeConnection() {
    if (isConnected) {
        SQLDisconnect(dbc);
        isConnected = false;
    }
    if (dbc) SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    if (env) SQLFreeHandle(SQL_HANDLE_ENV, env);
}

// Query and execute to current Azure SQL DB connection
bool AzureDatabase::queryExecute(const std::string& query, bool display) {
    if (!isConnected) {
        std::cerr << "No active database connection. Please connect first." << std::endl;
        return false;
    }

    SQLHSTMT stmt;
    // Allocate statement handle
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to allocate statement handle." << std::endl;
        printSQLError(dbc, SQL_HANDLE_DBC);
        return false;
    }

    // Execute the SQL query
    ret = SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        if (display) {
            SQLCHAR columnData[256];
            SQLSMALLINT columns;
            SQLNumResultCols(stmt, &columns);

            // Display column headers
            for (SQLUSMALLINT i = 1; i <= columns; ++i) {
                SQLCHAR columnName[256];
                SQLSMALLINT nameLength;
                SQLDescribeCol(stmt, i, columnName, sizeof(columnName), &nameLength, NULL, NULL, NULL, NULL);
                std::cout << columnName << "\t";
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
        }
    } else {
        std::cerr << "Failed to execute query." << std::endl;
        printSQLError(stmt, SQL_HANDLE_STMT);
    }

    // Free statement handle
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return true;
}

// Helper function
std::string AzureDatabase::convertDoubleToDateTime(double excelDays) {
    // Excel epoch (1899-12-30) to Unix epoch (1970-01-01) offset in days
    constexpr double excelEpochOffset = 25569.0;

    // Convert Excel days to milliseconds since Unix epoch
    auto millis = std::chrono::milliseconds(static_cast<int64_t>((excelDays - excelEpochOffset) * 86400000));

    // Convert to time_point
    auto timePoint = std::chrono::system_clock::time_point(millis);

    // Convert to time_t for date and time
    std::time_t rawTime = std::chrono::system_clock::to_time_t(timePoint);

    // Get the fractional milliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(millis).count() % 1000;

    // Format the datetime string with milliseconds
    std::tm* tmTime = std::gmtime(&rawTime);
    std::ostringstream oss;
    oss << std::put_time(tmTime, "%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(3) << ms;

    return oss.str();
}

// copy from EpitrendBinaryData into SQL table
bool AzureDatabase::copyToSQL(const std::string& tableName, EpitrendBinaryData data) {
    if (!isConnected) {
        std::cerr << "No active database connection. Please connect first." << std::endl;
        return false;
    }

    auto allData = data.getAllTimeSeriesData();

    for (const auto& [name, timeSeries] : allData) {
        for (const auto& [time, value] : timeSeries) {
            // Convert double time to DATETIME2 format
            std::time_t rawTime = static_cast<std::time_t>((time - 25569.0) * 86400); // Excel Epoch = 1899-12-30
            std::tm* tmTime = std::gmtime(&rawTime);

            std::ostringstream oss;
            oss << std::put_time(tmTime, "%Y-%m-%d %H:%M:%S");

            std::string dateTime = convertDoubleToDateTime(time);

            // Construct query with deduplication rule
            std::string query = 
                "IF NOT EXISTS ("
                "SELECT 1 FROM " + tableName +
                " WHERE name = ? AND date_time = ?"
                ") INSERT INTO " + tableName + " (name, date_time, value) VALUES (?, ?, ?)";

            SQLHSTMT stmt;
            SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
            if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
                std::cerr << "Failed to allocate statement handle." << std::endl;
                printSQLError(dbc, SQL_HANDLE_DBC);
                return false;
            }

            // Bind parameters
            std::cout << "Inserting name: " << name << ", time: " << dateTime << ", value: " << value << " into " << tableName << "\n";
            SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, name.size(), 0, (SQLPOINTER)name.c_str(), name.size(), NULL);
            SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, dateTime.size(), 0, (SQLPOINTER)dateTime.c_str(), dateTime.size(), NULL);
            SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, name.size(), 0, (SQLPOINTER)name.c_str(), name.size(), NULL);
            SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, dateTime.size(), 0, (SQLPOINTER)dateTime.c_str(), dateTime.size(), NULL);
            SQLBindParameter(stmt, 5, SQL_PARAM_INPUT, SQL_C_FLOAT, SQL_FLOAT, 0, 0, (SQLPOINTER)&value, sizeof(float), NULL);

            // Execute query
            ret = SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS);
            if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
                std::cerr << "Failed to execute query for name: " << name << ", time: " << dateTime << ", value: " << value << std::endl;
                printSQLError(stmt, SQL_HANDLE_STMT);
                SQLFreeHandle(SQL_HANDLE_STMT, stmt);
                return false;
            }

            // Free statement handle
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        }
    }

    return true;
}

// Connect, query and execute static function
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
