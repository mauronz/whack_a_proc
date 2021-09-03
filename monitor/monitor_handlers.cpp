#include "communication.h"
#include "log.h"
#include "include/pe_sieve_api.h"
#include "monitor.h"
#include <stdio.h>

#define BUFSIZE 1024

extern INJECT_CONFIG config;

LPCSTR GetCodeString(MessageCode code) {
	switch (code) {
	case CODE_ERROR:
		return "ERROR";
	case CODE_OK:
		return "OK";
	case CODE_SCAN:
		return "SCAN";
	case CODE_INJECT:
		return "INJECT";
	case CODE_THREAD:
		return "THREAD";
	case CODE_INIT:
		return "INIT";
	}
	return NULL;
}

IPC_MESSAGE* DoScan(IPC_MESSAGE* pMsg) {
	DWORD dwPid = DWORD_PARAM(pMsg, 0);
	pesieve::t_params params = { 0 };
	params.pid = dwPid;
	params.quiet = true;
	params.no_hooks = true;
	PESieve_scan(params);
	return CreateSimpleIPCMessage(CODE_OK);
}

IPC_MESSAGE* DoInject(IPC_MESSAGE* pMsg) {
	MessageCode code = CODE_ERROR;
	DWORD dwPid = DWORD_PARAM(pMsg, 0);
	DWORD dwTid = DWORD_PARAM(pMsg, 1);
	HANDLE hProcess;
	DWORD dwSize;
	WCHAR pImageFilename[MAX_PATH], pMessage[200];
	BOOL bInject = FALSE;
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	if (hProcess != INVALID_HANDLE_VALUE) {
		if (config.AllProcesses) {
			bInject = TRUE;
		}
		else {
			dwSize = sizeof(pImageFilename);
			QueryFullProcessImageNameW(hProcess, 0, pImageFilename, &dwSize);
			wsprintfW(pMessage, L"New process %s (PID: %d). Do you want to inject into it?", pImageFilename, dwPid);
			bInject = MessageBoxW(NULL, pMessage, L"Injector", MB_YESNO) == IDYES;
		}

		if (bInject) {
			if (SetEntrypointHook(hProcess) && CreateWorkerThread(dwPid, dwTid))
				code = CODE_OK;
			else
				code = CODE_ERROR;
		}
		else
			code = CODE_OK;

		CloseHandle(hProcess);
	}
	return CreateSimpleIPCMessage(code);
}

IPC_MESSAGE* DoThread(IPC_MESSAGE* pMsg) {
	MessageCode code = CODE_ERROR;
	DWORD dwPid = DWORD_PARAM(pMsg, 0);
	DWORD dwTid = DWORD_PARAM(pMsg, 1);
	if (CreateWorkerThread(dwPid, dwTid))
		code = CODE_OK;
	return CreateSimpleIPCMessage(code);
}

IPC_MESSAGE* DoInit(IPC_MESSAGE* pMsg) {
	IPC_MESSAGE* pResponse = CreateIPCMessage(CODE_OK, 1, &config, sizeof(config));
	return pResponse;
}

BOOL Communicate(HANDLE hPipe) {
	if (hPipe == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
		while (TRUE) {
			IPC_MESSAGE* pMsg = ReceiveIPCMessage(hPipe);
			if (!pMsg)
				break;
			Log("Monitor message received: %s\n", GetCodeString(pMsg->Code));

			IPC_MESSAGE* pResponse = NULL;
			switch (pMsg->Code) {
			case CODE_SCAN:
				pResponse = DoScan(pMsg);
				break;
			case CODE_INJECT:
				pResponse = DoInject(pMsg);
				break;
			case CODE_THREAD:
				pResponse = DoThread(pMsg);
				break;
			case CODE_INIT:
				pResponse = DoInit(pMsg);
				break;
			default:
				pResponse = CreateSimpleIPCMessage(CODE_ERROR);
			}

			DestroyIPCMessage(pMsg);
			if (pResponse) {
				SendIPCMessage(pResponse, hPipe);
				DestroyIPCMessage(pResponse);
			}
			else
				break;
		}
	}
	CloseHandle(hPipe);
	return TRUE;
}