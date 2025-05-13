#pragma once

#ifdef PREDICTBRIDGE_EXPORTS
#define PREDICTBRIDGE_API __declspec(dllexport)
#else
#define PREDICTBRIDGE_API __declspec(dllimport)
#endif

#include <windows.h>
#include <string>
#include <cstdio>

// Internal class (optional)
class PREDICTBRIDGE_API CPredictBridge {
public:
    CPredictBridge(void);
};

// Optional stub exports
extern PREDICTBRIDGE_API int nPredictBridge;
PREDICTBRIDGE_API int fnPredictBridge(void);

// === Core DLL Exports for MT4 ===

#ifdef __cplusplus
extern "C" {
#endif

    // 🔹 Send one-candle indicator-based signal to prediction server
    PREDICTBRIDGE_API void __stdcall SendIndicatorSignal(
        double s1, double s2, double s3, double s4,
        const char* symbol,
        int time,
        double open, double close, double high, double low,
        int volume,
        char result[], int resultSize
    );

    // 🔹 Send multi-candle batch (e.g., serialized OHLCV CSV string)
    PREDICTBRIDGE_API void __stdcall SendCandleBatch(
        const unsigned char* input, int inputSize,
        unsigned char* output, int outputSize
    );

    // 🔹 Fetch latest command (placeholder)
    PREDICTBRIDGE_API void __stdcall GetCommand(
        char result[], int size
    );
    
        PREDICTBRIDGE_API void __stdcall GetAccountInfo(char result[], int resultSize);
        PREDICTBRIDGE_API void __stdcall GetOpenPositions(char result[], int resultSize);
    

#ifdef __cplusplus
}
#endif
