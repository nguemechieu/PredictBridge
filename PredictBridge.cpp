// PredictBridge.cpp : Defines the exported functions for the DLL.
#include "pch.h"
#include "framework.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <sstream>
#include "PredictBridge.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

constexpr int BUFFER_SIZE = 4096;
constexpr int SERVER_PORT = 9999;
const char* SERVER_IP = "127.0.0.1";
const char* LOG_FILE = "PredictBridge.log";
constexpr long MAX_LOG_SIZE = 1024 * 1024; // 1 MB

// === Utility Functions ===
void LogToFile(const char* message)
{
    FILE* check;
    fopen_s(&check, LOG_FILE, "r");
    if (check)
    {
        fseek(check, 0, SEEK_END);
        long size = ftell(check);
        fclose(check);
        if (size > MAX_LOG_SIZE)
            remove(LOG_FILE);
    }

    FILE* file;
    fopen_s(&file, LOG_FILE, "a+");
    if (file)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond,
            message);
        fclose(file);
    }
}

bool InitializeWinsock(WSADATA& wsaData)
{
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

SOCKET CreateSocket()
{
    return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

bool ConnectToServer(SOCKET& sock, const char* ip, int port)
{
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server.sin_addr);

    return connect(sock, reinterpret_cast<sockaddr*>(&server), sizeof(server)) != SOCKET_ERROR;
}

void Cleanup(SOCKET& sock)
{
    if (sock != INVALID_SOCKET)
        closesocket(sock);
    WSACleanup();
}

// === EXPORTS ===

extern "C" PREDICTBRIDGE_API void __stdcall SendIndicatorSignal(
    double s1, double s2, double s3, double s4,
    const char* symbol, int time,
    double open, double close, double high, double low,
    int volume, char result[], int resultSize)
{
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    char recvBuf[BUFFER_SIZE] = { 0 };
    char message[BUFFER_SIZE];

    sprintf_s(message, sizeof(message),
        "%f,%f,%f,%f,%s,%d,%f,%f,%f,%f,%d",
        s1, s2, s3, s4, symbol, time, open, close, high, low, volume);

    LogToFile(("SENT SIGNAL: " + string(message)).c_str());

    if (!InitializeWinsock(wsaData))
    {
        strncpy_s(result, resultSize, "ERROR: Winsock init failed", _TRUNCATE);
        return;
    }

    sock = CreateSocket();
    if (sock == INVALID_SOCKET)
    {
        strncpy_s(result, resultSize, "ERROR: Socket creation failed", _TRUNCATE);
        Cleanup(sock);
        return;
    }

    if (!ConnectToServer(sock, SERVER_IP, SERVER_PORT))
    {
        strncpy_s(result, resultSize, "ERROR: Connection failed", _TRUNCATE);
        Cleanup(sock);
        return;
    }

    if (send(sock, message, (int)strlen(message), 0) == SOCKET_ERROR)
    {
        strncpy_s(result, resultSize, "ERROR: Send failed", _TRUNCATE);
        Cleanup(sock);
        return;
    }

    int bytes = recv(sock, recvBuf, BUFFER_SIZE - 1, 0);
    if (bytes > 0)
    {
        recvBuf[bytes] = '\0';
        LogToFile(("RECEIVED SIGNAL: " + string(recvBuf)).c_str());
        strncpy_s(result, resultSize, recvBuf, _TRUNCATE);
    }
    else
    {
        strncpy_s(result, resultSize, "ERROR: No response", _TRUNCATE);
    }

    Cleanup(sock);
}

extern "C" PREDICTBRIDGE_API void __stdcall SendCandleBatch(
    const unsigned char* input, int inputSize,
    unsigned char* output, int outputSize)
{
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    char recvBuf[BUFFER_SIZE] = { 0 };

    string payload(reinterpret_cast<const char*>(input), inputSize);
    LogToFile(("SENT CANDLE BATCH: " + payload.substr(0, 200) + "...").c_str()); // log first part

    if (!InitializeWinsock(wsaData))
    {
        strncpy_s((char*)output, outputSize, "ERROR: Winsock init failed", _TRUNCATE);
        return;
    }

    sock = CreateSocket();
    if (sock == INVALID_SOCKET)
    {
        strncpy_s((char*)output, outputSize, "ERROR: Socket creation failed", _TRUNCATE);
        Cleanup(sock);
        return;
    }

    if (!ConnectToServer(sock, SERVER_IP, SERVER_PORT))
    {
        strncpy_s((char*)output, outputSize, "ERROR: Connection failed", _TRUNCATE);
        Cleanup(sock);
        return;
    }

    if (send(sock, payload.c_str(), (int)payload.length(), 0) == SOCKET_ERROR)
    {
        strncpy_s((char*)output, outputSize, "ERROR: Send failed", _TRUNCATE);
        Cleanup(sock);
        return;
    }

    int bytes = recv(sock, recvBuf, BUFFER_SIZE - 1, 0);
    if (bytes > 0)
    {
        recvBuf[bytes] = '\0';
        LogToFile(("RECEIVED BATCH: " + string(recvBuf)).c_str());
        strncpy_s((char*)output, outputSize, recvBuf, _TRUNCATE);
    }
    else
    {
        strncpy_s((char*)output, outputSize, "ERROR: No response", _TRUNCATE);
    }

    Cleanup(sock);
}

extern "C" PREDICTBRIDGE_API void __stdcall GetCommand(char result[], int size)
{
    const char* placeholder = "No command available";
    LogToFile("COMMAND FETCHED");
    strncpy_s(result, size, placeholder, _TRUNCATE);
}

// === Generic Command Sender ===
void SendSimpleCommand(const char* command, char* result, int resultSize, const char* logPrefix)
{
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    char recvBuf[BUFFER_SIZE] = { 0 };

    LogToFile((string(logPrefix) + " SENT: " + command).c_str());

    if (!InitializeWinsock(wsaData))
    {
        strncpy_s(result, resultSize, "ERROR: Winsock init failed", _TRUNCATE);
        return;
    }

    sock = CreateSocket();
    if (sock == INVALID_SOCKET)
    {
        strncpy_s(result, resultSize, "ERROR: Socket creation failed", _TRUNCATE);
        Cleanup(sock);
        return;
    }

    if (!ConnectToServer(sock, SERVER_IP, SERVER_PORT))
    {
        strncpy_s(result, resultSize, "ERROR: Connection failed", _TRUNCATE);
        Cleanup(sock);
        return;
    }

    if (send(sock, command, (int)strlen(command), 0) == SOCKET_ERROR)
    {
        strncpy_s(result, resultSize, "ERROR: Send failed", _TRUNCATE);
        Cleanup(sock);
        return;
    }

    int bytes = recv(sock, recvBuf, BUFFER_SIZE - 1, 0);
    if (bytes > 0)
    {
        recvBuf[bytes] = '\0';
        LogToFile((string(logPrefix) + " RECEIVED: " + recvBuf).c_str());
        strncpy_s(result, resultSize, recvBuf, _TRUNCATE);
    }
    else
    {
        strncpy_s(result, resultSize, "ERROR: No response", _TRUNCATE);
    }

    Cleanup(sock);
}

// === Exported Account Info Fetcher ===
extern "C" PREDICTBRIDGE_API void __stdcall GetAccountInfo(char result[], int resultSize)
{
    SendSimpleCommand("account_info", result, resultSize, "ACCOUNT_INFO");
}

// === Exported Open Positions Fetcher ===
extern "C" PREDICTBRIDGE_API void __stdcall GetOpenPositions(char result[], int resultSize)
{
    SendSimpleCommand("open_positions", result, resultSize, "OPEN_POSITIONS");
}
