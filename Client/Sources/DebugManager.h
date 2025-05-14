#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <fstream>
#include <Windows.h>
#include <imgui.h>
#include <DirectXMath.h>

using namespace DirectX;

class DebugManager {
public:
    static DebugManager& GetInstance() {
        static DebugManager instance;
        return instance;
    }

    struct DebugMessage {
        std::string text;
        float duration;  // milliseconds
        float elapsed;
    };

    void AddOnScreenMessage(const std::string& msg, float duration);
    void Update(float deltaTime);
    void RenderMessages();

    void LogMessage(const std::wstring& msg);

public:
    XMFLOAT3 debugPosition;

private:
    DebugManager();
    ~DebugManager();
    DebugManager(const DebugManager&) = delete;
    DebugManager& operator=(const DebugManager&) = delete;

    void InitializeLogFile();
    std::wstring Utf8ToWString(const std::string& input);

    std::vector<DebugMessage> m_messages;
    std::wofstream m_logFile;
    std::mutex logMutex;
    std::wstring logFilePath;
};