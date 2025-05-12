// PredictBridge.cpp : Defines the exported functions for the DLL.
// Copyright (c) 2025 Noel Nguemechieu, Sopotek, Inc.
// All rights reserved.
#include "pch.h"
#include "framework.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <cstring>
#include <cstdio>
#include "PredictBridge.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the PREDICTBRIDGE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project

// === Exported dummy variable and function ===
PREDICTBRIDGE_API int nPredictBridge = 0;

PREDICTBRIDGE_API int fnPredictBridge(void)
{
    return 0;
}

CPredictBridge::CPredictBridge() {}

namespace
{
    constexpr int BUFFER_SIZE = 128;
    constexpr int SERVER_PORT = 9999;
    const char* SERVER_IP = "127.0.0.1";
    const char* LOG_FILE = "PredictBridge.log";
    constexpr long MAX_LOG_SIZE = 1024 * 1024; // 1 MB

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

    void LogToFile(const char* message)
    {
        // Check if log file exceeds max size and remove if necessary
        FILE* check;
        fopen_s(&check, LOG_FILE, "r");
        if (check)
        {
            fseek(check, 0, SEEK_END);
            long size = ftell(check);
            fclose(check);
            if (size > MAX_LOG_SIZE)
                remove(LOG_FILE);  // Remove the file if it exceeds max size
        }

        // Append new log entry
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
}

// === Exported function for MT4 DLL call ===
extern "C" PREDICTBRIDGE_API void __stdcall PredictSignalAndCommands(double s1, double s2, double s3, double s4, const char* symbol, double open, double close, double high, double low, int volume, char result[], int resultSize)
{
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    char recvBuf[BUFFER_SIZE] = { 0 };
    char message[BUFFER_SIZE];

    // Create the message to send
    sprintf_s(message, sizeof(message),
        "%f,%f,%f,%f,%s,%f,%f,%f,%f,%d",
        s1, s2, s3, s4, symbol, open, close, high, low, volume);

    // Log the sent message
    LogToFile(("SENT: " + std::string(message)).c_str());

    // Initialize Winsock
    if (!InitializeWinsock(wsaData))
    {
        const char* err = "ERROR: Winsock init failed";
        LogToFile(err);
        strncpy_s(result, resultSize, err, _TRUNCATE);
        return;
    }

    // Create the socket
    sock = CreateSocket();
    if (sock == INVALID_SOCKET)
    {
        const char* err = "ERROR: Socket creation failed";
        LogToFile(err);
        strncpy_s(result, resultSize, err, _TRUNCATE);
        Cleanup(sock);
        return;
    }

    // Connect to the server
    if (!ConnectToServer(sock, SERVER_IP, SERVER_PORT))
    {
        const char* err = "ERROR: Connection failed";
        LogToFile(err);
        strncpy_s(result, resultSize, err, _TRUNCATE);
        Cleanup(sock);
        return;
    }

    // Send the message
    if (send(sock, message, static_cast<int>(strlen(message)), 0) == SOCKET_ERROR)
    {
        const char* err = "ERROR: Send failed";
        LogToFile(err);
        strncpy_s(result, resultSize, err, _TRUNCATE);
        Cleanup(sock);
        return;
    }

    // Receive the response
    int bytes = recv(sock, recvBuf, BUFFER_SIZE - 1, 0);
    if (bytes > 0)
    {
        recvBuf[bytes] = '\0';  // Null-terminate the received buffer
        LogToFile(("RECEIVED: " + std::string(recvBuf)).c_str());
        strncpy_s(result, resultSize, recvBuf, _TRUNCATE);
    }
    else
    {
        const char* err = "ERROR: No response from server";
        LogToFile(err);
        strncpy_s(result, resultSize, err, _TRUNCATE);
    }

    // Cleanup the socket
    Cleanup(sock);
}

// Function to fetch the most recent command from the Python server
extern "C" PREDICTBRIDGE_API void __stdcall GetCommand(char result[], int size)
{
    const char* command = "No command available";  // Placeholder for the actual implementation
    // Log the command
    LogToFile(command);

    // Copy the command to the result buffer
    strncpy_s(result, size, command, _TRUNCATE);
}
