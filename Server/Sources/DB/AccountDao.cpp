#include "AccountDao.h"
#include <mysql/mysql.h>
#include <vector>
#include <cstring>

bool AccountDao::FindByUsername(const std::string& username,
    long long& outAccountId,
    std::string& outPasswordHash)
{
    const char* sql =
        "SELECT id, password_hash FROM accounts WHERE username=? LIMIT 1";

    MYSQL_STMT* statement = mysql_stmt_init(connection);
    if (!statement) return false;

    if (mysql_stmt_prepare(statement, sql,
        static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(statement);
        return false;
    }

    // 입력 파라미터 바인딩 (문자열 길이 변수는 실행 완료까지 생존해야 함)
    std::vector<char> usernameBuffer(username.begin(), username.end());
    unsigned long usernameLength =
        static_cast<unsigned long>(usernameBuffer.size());

    MYSQL_BIND parameterBinding{};
    std::memset(&parameterBinding, 0, sizeof(parameterBinding));
    parameterBinding.buffer_type = MYSQL_TYPE_STRING;
    parameterBinding.buffer = usernameLength ? usernameBuffer.data() : nullptr;
    parameterBinding.buffer_length = usernameLength;
    parameterBinding.length = &usernameLength;

    if (mysql_stmt_bind_param(statement, &parameterBinding) != 0) {
        mysql_stmt_close(statement);
        return false;
    }

    if (mysql_stmt_execute(statement) != 0) {
        mysql_stmt_close(statement);
        return false;
    }

    // 결과 바인딩
    long long accountId = 0;
    char passwordBuffer[128] = { 0 }; // bcrypt 60자 기준 여유
    unsigned long passwordLength = 0;

    MYSQL_BIND resultBindings[2]{};
    std::memset(resultBindings, 0, sizeof(resultBindings));

    resultBindings[0].buffer_type = MYSQL_TYPE_LONGLONG;
    resultBindings[0].buffer = &accountId;

    resultBindings[1].buffer_type = MYSQL_TYPE_STRING;
    resultBindings[1].buffer = passwordBuffer;
    resultBindings[1].buffer_length = sizeof(passwordBuffer);
    resultBindings[1].length = &passwordLength;

    if (mysql_stmt_bind_result(statement, resultBindings) != 0) {
        mysql_stmt_close(statement);
        return false;
    }

    bool found = false;
    int fetchCode = mysql_stmt_fetch(statement);
    if (fetchCode == 0) { // 성공적으로 1행 가져옴
        found = true;
        outAccountId = accountId;
        outPasswordHash.assign(passwordBuffer, passwordLength);
    }
    else if (fetchCode == 1) {
        // 에러
        mysql_stmt_close(statement);
        return false;
    } // MYSQL_NO_DATA(=100)은 found=false 그대로

    mysql_stmt_close(statement);
    return found;
}

bool AccountDao::CreateAccount(const std::string& username,
    const std::string& passwordHash,
    long long& outAccountId)
{
    const char* sql =
        "INSERT INTO accounts(username, password_hash, created_at) "
        "VALUES(?, ?, NOW())";

    MYSQL_STMT* statement = mysql_stmt_init(connection);
    if (!statement) return false;

    if (mysql_stmt_prepare(statement, sql,
        static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(statement);
        return false;
    }

    // 입력 파라미터 2개 (문자열 길이 변수는 지역 변수로 유지)
    std::vector<char> usernameBuffer(username.begin(), username.end());
    unsigned long usernameLength =
        static_cast<unsigned long>(usernameBuffer.size());

    std::vector<char> passwordBuffer(passwordHash.begin(), passwordHash.end());
    unsigned long passwordLength =
        static_cast<unsigned long>(passwordBuffer.size());

    MYSQL_BIND parameterBindings[2]{};
    std::memset(parameterBindings, 0, sizeof(parameterBindings));

    parameterBindings[0].buffer_type = MYSQL_TYPE_STRING;
    parameterBindings[0].buffer = usernameLength ? usernameBuffer.data() : nullptr;
    parameterBindings[0].buffer_length = usernameLength;
    parameterBindings[0].length = &usernameLength;

    parameterBindings[1].buffer_type = MYSQL_TYPE_STRING;
    parameterBindings[1].buffer = passwordLength ? passwordBuffer.data() : nullptr;
    parameterBindings[1].buffer_length = passwordLength;
    parameterBindings[1].length = &passwordLength;

    if (mysql_stmt_bind_param(statement, parameterBindings) != 0) {
        mysql_stmt_close(statement);
        return false;
    }

    if (mysql_stmt_execute(statement) != 0) {
        mysql_stmt_close(statement);
        return false;
    }

    outAccountId = static_cast<long long>(mysql_insert_id(connection));
    mysql_stmt_close(statement);
    return outAccountId > 0;
}

bool AccountDao::UpdateLastLogin(long long accountId)
{
    const char* sql = "UPDATE accounts SET last_login=NOW() WHERE id=?";

    MYSQL_STMT* statement = mysql_stmt_init(connection);
    if (!statement) return false;

    if (mysql_stmt_prepare(statement, sql,
        static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(statement);
        return false;
    }

    MYSQL_BIND parameterBinding{};
    std::memset(&parameterBinding, 0, sizeof(parameterBinding));
    parameterBinding.buffer_type = MYSQL_TYPE_LONGLONG;
    parameterBinding.buffer = &accountId;

    if (mysql_stmt_bind_param(statement, &parameterBinding) != 0) {
        mysql_stmt_close(statement);
        return false;
    }

    if (mysql_stmt_execute(statement) != 0) {
        mysql_stmt_close(statement);
        return false;
    }

    mysql_stmt_close(statement);
    return true;
}
