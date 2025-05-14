#include "DebugManager.h"
#include <direct.h>
#include <time.h>
#include <iostream>

// 생성자
DebugManager::DebugManager() {
    InitializeLogFile();
}

DebugManager::~DebugManager() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

std::wstring DebugManager::Utf8ToWString(const std::string& input) {
    int size = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, &result[0], size);
    result.pop_back(); // null 제거
    return result;
}

void DebugManager::InitializeLogFile() {
    const char* logDir = "Log";
    _mkdir(logDir);

    time_t now = time(NULL);
    struct tm localTime;
    localtime_s(&localTime, &now);

    char fileName[256];
    sprintf_s(fileName, sizeof(fileName), "%s/Log_%04d-%02d-%02d_%02d-%02d-%02d.txt",
        logDir,
        localTime.tm_year + 1900,
        localTime.tm_mon + 1,
        localTime.tm_mday,
        localTime.tm_hour,
        localTime.tm_min,
        localTime.tm_sec);

    logFilePath = Utf8ToWString(fileName);

    m_logFile.open(logFilePath, std::ios::out | std::ios::app);
    if (!m_logFile.is_open()) {
        std::wcerr << L"Failed to open log file: " << logFilePath << std::endl;
    }
}

void DebugManager::AddOnScreenMessage(const std::string& msg, float duration) {
    DebugMessage newMsg;
    newMsg.text = msg;
    newMsg.duration = duration;
    newMsg.elapsed = 0.0f;
    m_messages.push_back(newMsg);
}

void DebugManager::LogMessage(const std::wstring& msg) {
    std::lock_guard<std::mutex> lock(logMutex);

    OutputDebugStringW(msg.c_str());
    OutputDebugStringW(L"\n");

    std::wcout << msg << std::endl;

    if (m_logFile.is_open()) {
        m_logFile << msg << std::endl;
    }
}

void DebugManager::Update(float deltaTime) {
    for (auto it = m_messages.begin(); it != m_messages.end(); ) {
        it->elapsed += deltaTime;
        if (it->elapsed > it->duration)
            it = m_messages.erase(it);
        else
            ++it;
    }
}

void DebugManager::RenderMessages() {
    ImGui::Begin("Debug Messages");
    for (const auto& msg : m_messages) {
        ImGui::Text("%s", msg.text.c_str());
    }
    ImGui::End();
}
