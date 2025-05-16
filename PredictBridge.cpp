#include "pch.h"
#include "framework.h"
#include "PredictBridge.h"

#include <windows.h>
#include <cstdio>
#include <string>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <sstream>
#include <random>

#pragma comment(lib, "Ws2_32.lib")

#ifdef PREDICTBRIDGE_EXPORTS
#define PREDICTBRIDGE_API __declspec(dllexport)
#else
#define PREDICTBRIDGE_API __declspec(dllimport)
#endif

constexpr int BUFFER_SIZE = 4096;
static char SHARED_BUFFER[BUFFER_SIZE] = { 0 };
static std::mutex buffer_mutex;

const char* LOG_FILE = "PredictBridge.log";
constexpr long MAX_LOG_SIZE = 1024 * 1024;

static void LogToFile(const char* message) {
    FILE* check;
    fopen_s(&check, LOG_FILE, "r");
    if (check) {
        fseek(check, 0, SEEK_END);
        long size = ftell(check);
        fclose(check);
        if (size > MAX_LOG_SIZE) remove(LOG_FILE);
    }

    FILE* file;
    fopen_s(&file, LOG_FILE, "a+");
    if (file) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, message);
        fclose(file);
    }
}

std::string GenerateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static const char* chars = "0123456789abcdef";

    std::string uuid;
    for (int i = 0; i < 32; ++i) {
        uuid += chars[dis(gen)];
        if (i == 7 || i == 11 || i == 15 || i == 19) uuid += '-';
    }
    return uuid;
}

extern "C" {

    PREDICTBRIDGE_API void __stdcall WriteToBridge(const char* input) {
        WSADATA wsaData;
        SOCKET sock = INVALID_SOCKET;
        struct sockaddr_in server;
        char recvBuf[BUFFER_SIZE] = { 0 };

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            return;

        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            WSACleanup();
            return;
        }

        server.sin_family = AF_INET;
        server.sin_port = htons(50052);
        inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

        if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
            closesocket(sock);
            WSACleanup();
            return;
        }

        std::string messageId = GenerateUUID();
        std::ostringstream jsonMessage;
        jsonMessage << "{\"type\":\"signal\",\"payload\":\"" << input << "\",\"message_id\":\"" << messageId << "\"}";

        send(sock, jsonMessage.str().c_str(), (int)jsonMessage.str().length(), 0);
        int bytes = recv(sock, recvBuf, sizeof(recvBuf) - 1, 0);

        if (bytes > 0) {
            recvBuf[bytes] = '\0';
            std::lock_guard<std::mutex> lock(buffer_mutex);
            strncpy_s(SHARED_BUFFER, recvBuf, BUFFER_SIZE - 1);
            SHARED_BUFFER[BUFFER_SIZE - 1] = '\0';
        }

        closesocket(sock);
        WSACleanup();
        LogToFile(("Write: " + std::string(input)).c_str());
    }

    PREDICTBRIDGE_API const char* __stdcall ReadSharedBuffer() {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        LogToFile(("Read: " + std::string(SHARED_BUFFER)).c_str());
        return SHARED_BUFFER;
    }

    PREDICTBRIDGE_API void __stdcall ClearBridge() {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        memset(SHARED_BUFFER, 0, BUFFER_SIZE);
        LogToFile("Buffer cleared");
    }


}
