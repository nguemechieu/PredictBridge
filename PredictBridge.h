#pragma once

#ifdef PREDICTBRIDGE_EXPORTS
#define PREDICTBRIDGE_API __declspec(dllexport)
#else
#define PREDICTBRIDGE_API __declspec(dllimport)
#endif

#include <windows.h>
#include <string>
#include <cstdio>

// Class declaration for internal/optional expansion
class PREDICTBRIDGE_API CPredictBridge {
public:
    CPredictBridge(void);
};

// Optional exported variables and stub functions
extern PREDICTBRIDGE_API int nPredictBridge;
PREDICTBRIDGE_API int fnPredictBridge(void);

// Main DLL function callable by MT4
extern "C" PREDICTBRIDGE_API void __stdcall PredictSignalAndCommands(
    double s1, double s2, double s3, double s4,
    const char* symbol,  // Use const char* for MT4 compatibility
    double open, double close, double high, double low,
    int volume, char result[], int resultSize);

// This function is a placeholder for future implementation.
extern "C" PREDICTBRIDGE_API void __stdcall GetCommand(char result[], int size);
