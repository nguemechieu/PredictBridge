// dllmain.cpp : Defines the entry point for the DLL application.
  
#include "pch.h"  
#include "framework.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <cstring>
#include <cstdio>
#include "PredictBridge.h"
#include <iostream>
#include <fstream>
BOOL APIENTRY DllMain(HMODULE hModule,  
                            DWORD  ul_reason_for_call,  
                            LPVOID lpReserved)  
{  
   switch (ul_reason_for_call)  
   {  
   case DLL_PROCESS_ATTACH:
	   // Perform any necessary initialization here


	   break;
   case DLL_THREAD_ATTACH:
	   // Perform any necessary thread-specific initialization here
	   break;
   case DLL_THREAD_DETACH:  
						 // Perform any necessary thread-specific cleanup here
						 break;
   case DLL_PROCESS_DETACH:  
   
	   // Perform any necessary cleanup or logging here
       break;  
   }  

   return TRUE;  
}
