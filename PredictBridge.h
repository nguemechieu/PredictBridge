#pragma once

#ifdef PREDICTBRIDGE_EXPORTS
#define PREDICTBRIDGE_API __declspec(dllexport)
#else
#define PREDICTBRIDGE_API __declspec(dllimport)
#endif

extern "C" {

    PREDICTBRIDGE_API void __stdcall WriteToBridge(const char* input);
    PREDICTBRIDGE_API const char* __stdcall ReadSharedBuffer();
    PREDICTBRIDGE_API void __stdcall ClearBridge();
}
