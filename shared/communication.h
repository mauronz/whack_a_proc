#pragma once
#include <Windows.h>

#define PIPE_TEMPLATE L"\\\\.\\pipe\\whack%08x%08x"

#define HT_BEFORE 0
#define HT_AFTER 1

enum MessageCode {
	CODE_ERROR=0x2000,
	CODE_OK,
	CODE_SCAN,
	CODE_INJECT,
	CODE_THREAD,
	CODE_INIT,
};

enum HookLevel {
	LEVEL_LOW,
	LEVEL_MEDIUM,
	LEVEL_HIGH
};

typedef struct _IPC_ARG {
	int Size;
	PVOID Buf;
} IPC_ARG;

typedef struct _IPC_MESSAGE {
	MessageCode Code;
	int ArgCount;
	IPC_ARG* Args;
} IPC_MESSAGE;

#define DWORD_PARAM(pMsg, num) *(DWORD*)(pMsg->Args[num].Buf)
#define PTR_PARAM(pMsg, num) *(LPVOID*)(pMsg->Args[num].Buf)

typedef struct _inject_config {
	BOOL ProtectHook;
	BOOL AllProcesses;
	HookLevel Level;
} INJECT_CONFIG;

PBYTE SerializeIPCMessage(IPC_MESSAGE* pMsg, int* serialSize);
MessageCode PeekIPCMessageCode(PBYTE pBuf);
IPC_MESSAGE* ParseIPCMessage(PBYTE pBuf);
BOOL SendIPCMessage(IPC_MESSAGE* pMsg, HANDLE hPipe);
IPC_MESSAGE* ReceiveIPCMessage(HANDLE hPipe);
IPC_MESSAGE* CreateSimpleIPCMessage(MessageCode code);
IPC_MESSAGE* CreateEmptyIPCMessage();
IPC_MESSAGE* CreateIPCMessage(MessageCode code, int argCount, ...);
VOID DestroyIPCMessage(IPC_MESSAGE* msg);
HANDLE CreateThreadPipe(DWORD dwPid, DWORD dwTid, LPCWSTR pTemplate);