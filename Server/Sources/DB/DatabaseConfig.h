#pragma once
#include <string>

struct DatabaseConfig {
    std::string host = "127.0.0.1";   // ���� ȣ��Ʈ
    int         port = 3306;          // ��Ʈ
    std::string user = "appuser";     // �� ���� ����
    std::string password = "appsecret";
    std::string database = "appdb";
    int poolSize = 4;                 // �̸� ����� ���� ��
};
