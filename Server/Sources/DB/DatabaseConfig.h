#pragma once
#include <string>

struct DatabaseConfig {
    std::string host = "127.0.0.1";   // 접속 호스트
    int         port = 3306;          // 포트
    std::string user = "appuser";     // 앱 전용 계정
    std::string password = "appsecret";
    std::string database = "appdb";
    int poolSize = 4;                 // 미리 열어둘 연결 수
};
