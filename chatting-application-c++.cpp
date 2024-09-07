#include <iostream>
#include <string>
#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <iomanip>
#include <chrono>
#include <thread>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <random>
#include <thread>

using namespace std;

SQLHANDLE sqlEnvHandle;
SQLHANDLE sqlConnectionHandle;
SQLHANDLE sqlStatementHandle{};
SQLRETURN retcode;
SQLINTEGER prevChatId = 0;
wstring user1Number = L"1234567890";
wstring user2Number = L"0987654321";

void showSQLError(unsigned int handleType, const SQLHANDLE& handle) {
    SQLWCHAR SQLState[1024];
    SQLWCHAR message[1024];
    if (SQL_SUCCESS == SQLGetDiagRec(handleType, handle, 1, SQLState, NULL, message, 1024, NULL)) {
        wcerr << L"SQL Error: " << message << L"\nSQL State: " << SQLState << endl;
    }
}

void insertChatData(SQLHANDLE& sqlConnectionHandle, const wstring& user1Number, const wstring& user2Number, const wstring& messages) {
    wstring insertSQL = L"INSERT INTO chats (number, end_number, messages) VALUES (?, ?, ?)";

    SQLHANDLE sqlStatementHandle;
    SQLRETURN retcode{};

    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnectionHandle, &sqlStatementHandle);

    SQLPrepareW(sqlStatementHandle, (SQLWCHAR*)insertSQL.c_str(), SQL_NTS);

    SQLLEN user1NumberLength = user1Number.length() * sizeof(wchar_t);
    SQLLEN user2NumberLength = user2Number.length() * sizeof(wchar_t);
    SQLLEN messagesLength = messages.length() * sizeof(wchar_t);

    SQLBindParameter(sqlStatementHandle, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 0, 0, (SQLPOINTER)user1Number.c_str(), user1NumberLength, &user1NumberLength);
    SQLBindParameter(sqlStatementHandle, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 0, 0, (SQLPOINTER)user2Number.c_str(), user2NumberLength, &user2NumberLength);
    SQLBindParameter(sqlStatementHandle, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 0, 0, (SQLPOINTER)messages.c_str(), messagesLength, &messagesLength);

    retcode = SQLExecute(sqlStatementHandle);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        showSQLError(SQL_HANDLE_STMT, sqlStatementHandle);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStatementHandle);
}

void checkForNewMessages(SQLHANDLE& sqlConnectionHandle, const wstring& user1Number, const wstring& user2Number) {
    wstring selectSQL = L"SELECT chat_id, number, messages FROM chats WHERE ((number = ? AND end_number = ?) OR (number = ? AND end_number = ?)) AND chat_id > ?";
    SQLHANDLE sqlStatementHandle;
    SQLRETURN retcode{};
    SQLINTEGER chatId{};
    SQLLEN chatIdInd, numberInd, messagesLengthInd;
    SQLWCHAR messagesBuffer[1024];
    SQLWCHAR numberBuffer[20];

    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnectionHandle, &sqlStatementHandle);

    while (true) {
        SQLPrepareW(sqlStatementHandle, (SQLWCHAR*)selectSQL.c_str(), SQL_NTS);

        SQLLEN user1NumberLength = user1Number.length() * sizeof(wchar_t);
        SQLLEN user2NumberLength = user2Number.length() * sizeof(wchar_t);

        SQLBindParameter(sqlStatementHandle, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 0, 0, (SQLPOINTER)user1Number.c_str(), user1NumberLength, &user1NumberLength);
        SQLBindParameter(sqlStatementHandle, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 0, 0, (SQLPOINTER)user2Number.c_str(), user2NumberLength, &user2NumberLength);
        SQLBindParameter(sqlStatementHandle, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 0, 0, (SQLPOINTER)user2Number.c_str(), user2NumberLength, &user2NumberLength);
        SQLBindParameter(sqlStatementHandle, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 0, 0, (SQLPOINTER)user1Number.c_str(), user1NumberLength, &user1NumberLength);
        SQLBindParameter(sqlStatementHandle, 5, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &prevChatId, 0, NULL);

        retcode = SQLExecute(sqlStatementHandle);
        if (retcode == SQL_SUCCESS || SQL_SUCCESS_WITH_INFO) {
            while (SQLFetch(sqlStatementHandle) == SQL_SUCCESS) {
                SQLGetData(sqlStatementHandle, 1, SQL_C_SLONG, &chatId, sizeof(chatId), &chatIdInd);
                SQLGetData(sqlStatementHandle, 2, SQL_C_WCHAR, numberBuffer, sizeof(numberBuffer), &numberInd);
                SQLGetData(sqlStatementHandle, 3, SQL_C_WCHAR, messagesBuffer, sizeof(messagesBuffer), &messagesLengthInd);

                wstring number(numberBuffer, numberInd / sizeof(SQLWCHAR));
                wstring message(messagesBuffer, messagesLengthInd / sizeof(SQLWCHAR));

                if (number == user2Number) {
                    wcout << "Recieved : " << message << endl;
                }
                else {
                    wcout << setw(100) << "Sent : " << message << endl;
                }

                if (chatId > prevChatId) {
                    prevChatId = chatId;
                }
            }
        }
        else {
            showSQLError(SQL_HANDLE_STMT, sqlStatementHandle);
        }

        this_thread::sleep_for(chrono::seconds(1));
    }

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStatementHandle);
}


int main() {
    string welcome = "=============== WELCOME TO THE SMART CHATTING APPLICATION OF PAKISTAN ====================\n\n";

    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle);
    SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnectionHandle);

    SQLWCHAR retconstring[1024];
    SQLWCHAR* connectionString = (SQLWCHAR*)L"DRIVER={MySQL ODBC 8.4 Unicode Driver};SERVER=localhost;DATABASE=smartchat;USER=root;PASSWORD=Maasaa229$;OPTION=3;";

    retcode = SQLDriverConnectW(sqlConnectionHandle, NULL, connectionString, SQL_NTS, retconstring, 1024, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        cout << welcome;
    }
    else {
        wcout << L"Failed to connect to Open \n Please Check Your Network Connection." << endl << endl;
        showSQLError(SQL_HANDLE_DBC, sqlConnectionHandle);
    }

    wstring messages;
    int no = 1;

    thread messageChecker(checkForNewMessages, ref(sqlConnectionHandle), ref(user1Number), ref(user2Number));

    while (no == 1) {
        cout << "Type Your Message : ";
        getline(wcin, messages);

        if (messages == L"exit") {
            no = 0;
            break;
        }

        insertChatData(sqlConnectionHandle, user1Number, user2Number, messages);
    }

    messageChecker.join();

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStatementHandle);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConnectionHandle);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);

    return 0;
}
