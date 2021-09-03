// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#include "injector.h"
#include "communication.h"

HMODULE hGlobalModule;
HANDLE hPipe;
INJECT_CONFIG config;


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	int argc;
	WCHAR **argv;
	WCHAR pPipeName[64];
	DWORD dwMode;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: {
		hGlobalModule = hModule;
		argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		wsprintfW(pPipeName, PIPE_TEMPLATE, GetCurrentProcessId(), GetCurrentThreadId());
		hPipe = CreateFileW(pPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		dwMode = PIPE_READMODE_MESSAGE;
		SetNamedPipeHandleState(
			hPipe,    // pipe handle 
			&dwMode,  // new pipe mode 
			NULL,     // don't set maximum bytes 
			NULL);    // don't set maximum time

		IPC_MESSAGE* pMsg = CreateSimpleIPCMessage(CODE_INIT);
		if (SendIPCMessage(pMsg, hPipe)) {
			DestroyIPCMessage(pMsg);
			IPC_MESSAGE* pResponse = ReceiveIPCMessage(hPipe);
			if (pResponse) {
				if (pResponse->Code == CODE_OK && pResponse->ArgCount == 1) {
					memcpy(&config, pResponse->Args[0].Buf, sizeof(config));
					SetHooks();
				}
				DestroyIPCMessage(pResponse);
			}
		}
	}
		
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

