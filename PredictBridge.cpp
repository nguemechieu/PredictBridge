// PredictBridge.cpp : Defines the exported functions for the DLL.
#include "pch.h"
#include "framework.h"
#include "PredictBridge.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstdio>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

constexpr int BUFFER_SIZE = 4096;
constexpr int SERVER_PORT = 9999;
const char* SERVER_IP = "127.0.0.1";
const char* LOG_FILE = "PredictBridge.log";
constexpr long MAX_LOG_SIZE = 1024 * 1024; // 1 MB

// === Logging ===
void LogToFile(const char* message) {
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

// === TCP Helpers ===
bool InitializeWinsock(WSADATA& wsaData) {
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

SOCKET CreateSocket() {
    return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

bool ConnectToServer(SOCKET& sock, const char* ip, int port) {
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server.sin_addr);
    return connect(sock, reinterpret_cast<sockaddr*>(&server), sizeof(server)) != SOCKET_ERROR;
}

void Cleanup(SOCKET& sock) {
    if (sock != INVALID_SOCKET) closesocket(sock);
    WSACleanup();
}

// === Generic TCP Send/Receive ===
bool SendAndReceive(const char* message, char* result, int resultSize, const char* logPrefix) {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    char recvBuf[BUFFER_SIZE] = { 0 };

    LogToFile((string(logPrefix) + " SENT: " + message).c_str());

    if (!InitializeWinsock(wsaData)) {
        strncpy_s(result, resultSize, "ERROR: Winsock init failed", _TRUNCATE);
        return false;
    }

    sock = CreateSocket();
    if (sock == INVALID_SOCKET) {
        strncpy_s(result, resultSize, "ERROR: Socket creation failed", _TRUNCATE);
        Cleanup(sock);
        return false;
    }

    if (!ConnectToServer(sock, SERVER_IP, SERVER_PORT)) {
        strncpy_s(result, resultSize, "ERROR: Connection failed", _TRUNCATE);
        Cleanup(sock);
        return false;
    }

    if (send(sock, message, (int)strlen(message), 0) == SOCKET_ERROR) {
        strncpy_s(result, resultSize, "ERROR: Send failed", _TRUNCATE);
        Cleanup(sock);
        return false;
    }

    int bytes = recv(sock, recvBuf, BUFFER_SIZE - 1, 0);
    if (bytes > 0) {
        recvBuf[bytes] = '\0';
        LogToFile((string(logPrefix) + " RECEIVED: " + recvBuf).c_str());
        strncpy_s(result, resultSize, recvBuf, _TRUNCATE);
    }
    else {
        strncpy_s(result, resultSize, "ERROR: No response", _TRUNCATE);
    }

    Cleanup(sock);
    return true;
}

// === Exported Functions ===

extern "C" PREDICTBRIDGE_API void __stdcall SendIndicatorSignal(
    double s1, double s2, double s3, double s4,
    const char* symbol, int time,
    double open, double close, double high, double low,
    int volume, char result[], int resultSize)
{
    char signalMsg[BUFFER_SIZE];
    char candleMsg[BUFFER_SIZE];
    char tempResult[BUFFER_SIZE];

    // Format signal message
    sprintf_s(signalMsg, sizeof(signalMsg),
        "signal:%f,%f,%f,%f,%s,%d,%f,%f,%f,%f,%d",
        s1, s2, s3, s4, symbol, time, open, close, high, low, volume);

    // Format candle message
    sprintf_s(candleMsg, sizeof(candleMsg),
        "candles:%d,%f,%f,%f,%f,%d",
        time, open, close, high, low, volume);

    // Send signal message first
    SendAndReceive(signalMsg, tempResult, sizeof(tempResult), "SIGNAL");
    strncpy_s(result, resultSize, tempResult, _TRUNCATE);

    // Send candle message second (optional: only if signal succeeded)
    char candleResult[BUFFER_SIZE];
    SendAndReceive(candleMsg, candleResult, sizeof(candleResult), "CANDLE");

    // Optionally log or combine the responses
    LogToFile(("FINAL RESULT: " + string(result) + " | " + candleResult).c_str());
}


extern "C" PREDICTBRIDGE_API void __stdcall SendCandleBatch(
    const unsigned char* input, int inputSize,
    unsigned char* output, int outputSize)
{
    string payload(reinterpret_cast<const char*>(input), inputSize);
    string message = "candles:" + payload;
    SendAndReceive(message.c_str(), (char*)output, outputSize, "BATCH");
}

extern "C" PREDICTBRIDGE_API void __stdcall GetCommand(char result[], int size) {
    LogToFile("COMMAND FETCHED");
    strncpy_s(result, size, "No command available", _TRUNCATE);
}

extern "C" PREDICTBRIDGE_API void __stdcall GetAccountInfo(char result[], int resultSize) {
    SendAndReceive("account_info", result, resultSize, "ACCOUNT_INFO");
}

extern "C" PREDICTBRIDGE_API void __stdcall GetOpenPositions(char result[], int resultSize) {
    SendAndReceive("open_positions", result, resultSize, "OPEN_POSITIONS");
}
