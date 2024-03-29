#include "hooks.h"
#include "injector.h"
#include "communication.h"
#include "functions.h"
#include <Psapi.h>

#define MYPROC_NUM 100

typedef struct _TARGET_PROCESS {
	DWORD Pid;
	BOOL Injected;
} TARGET_PROCESS;

extern HANDLE hPipe;
extern HMODULE hGlobalModule;

TARGET_PROCESS pMyProcesses[MYPROC_NUM] = { 0 };

VOID InjectProcess(DWORD dwPid, DWORD dwTid) {
	for (int i = 0; i < MYPROC_NUM && pMyProcesses[i].Pid; i++) {
		if (dwPid == pMyProcesses[i].Pid && !pMyProcesses[i].Injected) {
			IPC_MESSAGE* pMsg = CreateIPCMessage(CODE_INJECT, 2, &dwPid, sizeof(dwPid), &dwTid, sizeof(dwTid));
			if (SendIPCMessage(pMsg, hPipe)) {
				DestroyIPCMessage(pMsg);
				IPC_MESSAGE* pResponse = ReceiveIPCMessage(hPipe);
				if (pResponse) {
					if (pResponse->Code != CODE_OK)
						MessageBoxA(NULL, "Error code!", "scan", 0);
					DestroyIPCMessage(pResponse);
				}
			}
		}
		pMyProcesses[i].Injected = TRUE;
	}
}

VOID ScanProcess(DWORD dwPid, DWORD dwTid) {
	IPC_MESSAGE* pMsg = CreateIPCMessage(CODE_SCAN, 1, &dwPid, sizeof(dwPid));
	if (SendIPCMessage(pMsg, hPipe)) {
		DestroyIPCMessage(pMsg);
		IPC_MESSAGE* pResponse = ReceiveIPCMessage(hPipe);
		if (pResponse) {
			if (pResponse->Code != CODE_OK)
				MessageBoxA(NULL, "Error code!", "scan", 0);
			DestroyIPCMessage(pResponse);
		}
	}

	if (dwTid == 0)
		return;
	InjectProcess(dwPid, dwTid);
}

NTSTATUS bh_NtCreateUserProcess(
	_Out_ PHANDLE ProcessHandle,
	_Out_ PHANDLE ThreadHandle,
	_In_ ACCESS_MASK ProcessDesiredAccess,
	_In_ ACCESS_MASK ThreadDesiredAccess,
	_In_opt_ PVOID ProcessObjectAttributes,
	_In_opt_ PVOID ThreadObjectAttributes,
	_In_ ULONG ProcessFlags, // PROCESS_CREATE_FLAGS_*
	_In_ ULONG ThreadFlags, // THREAD_CREATE_FLAGS_*
	_In_opt_ PVOID ProcessParameters, // PRTL_USER_PROCESS_PARAMETERS
	_Inout_ PVOID CreateInfo,
	_In_opt_ PVOID AttributeList
) {
	NTSTATUS status = pOrigNtCreateUserProcess(
		ProcessHandle,
		ThreadHandle,
		ProcessDesiredAccess,
		ThreadDesiredAccess,
		ProcessObjectAttributes,
		ThreadObjectAttributes,
		ProcessFlags,
		ThreadFlags | THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
		ProcessParameters,
		CreateInfo,
		AttributeList
	);
	if (!status) {
		for (int i = 0; i < MYPROC_NUM; i++) {
			if (!pMyProcesses[i].Pid) {
				pMyProcesses[i].Pid = GetProcessId(*ProcessHandle);
				break;
			}
		}
		if ((ThreadFlags & THREAD_CREATE_FLAGS_CREATE_SUSPENDED) == 0) {
			DWORD dwPid = GetProcessId(*ProcessHandle);
			DWORD dwTid = GetThreadId(*ThreadHandle);
			InjectProcess(dwPid, dwTid);
			ULONG ulSuspendCount;
			pOrigNtResumeThread(*ThreadHandle, &ulSuspendCount);
		}
	}
	return status;
}

VOID ah_ZwMapViewOfSection(
	HANDLE          SectionHandle,
	HANDLE          ProcessHandle,
	PVOID           *BaseAddress,
	ULONG_PTR       ZeroBits,
	SIZE_T          CommitSize,
	PLARGE_INTEGER  SectionOffset,
	PSIZE_T         ViewSize,
	DWORD InheritDisposition,
	ULONG           AllocationType,
	ULONG           Win32Protect,
	DWORD retvalue
) {
	WCHAR pFilename[100];
	WCHAR *pName;
	GetMappedFileNameW(ProcessHandle, *BaseAddress, pFilename, 100);
	pName = wcsrchr(pFilename, '\\');
	if (pName == NULL)
		return;
	if (!wcscmp(pName + 1, L"ntdll.dll")) {
		*BaseAddress = GetModuleHandleA("ntdll.dll");
	}
}

VOID bh_NtCreateThread(
	OUT PHANDLE             ThreadHandle,
	IN ACCESS_MASK          DesiredAccess,
	IN PVOID   ObjectAttributes OPTIONAL,
	IN HANDLE               ProcessHandle,
	OUT PVOID          ClientId,
	IN PCONTEXT             ThreadContext,
	IN PVOID         InitialTeb,
	IN BOOLEAN              CreateSuspended) {
	DWORD dwPid;
	DWORD dwTid;
	if (!CreateSuspended) {
		dwPid = GetProcessId(ProcessHandle);
		dwTid = GetThreadId(ThreadHandle);
		ScanProcess(dwPid, dwTid);
	}
}

VOID bh_NtCreateThreadEx(
	OUT  PHANDLE ThreadHandle,
	IN  ACCESS_MASK DesiredAccess,
	IN  PVOID ObjectAttributes OPTIONAL,
	IN  HANDLE ProcessHandle,
	IN  PVOID StartRoutine,
	IN  PVOID Argument OPTIONAL,
	IN  ULONG CreateFlags,
	IN  ULONG_PTR ZeroBits,
	IN  SIZE_T StackSize OPTIONAL,
	IN  SIZE_T MaximumStackSize OPTIONAL,
	IN  PVOID AttributeList OPTIONAL
) {
	DWORD dwPid;
	DWORD dwTid;
	if ((CreateFlags & CREATE_SUSPENDED) == 0) {
		dwPid = GetProcessId(ProcessHandle);
		dwTid = GetThreadId(ThreadHandle);
		ScanProcess(dwPid, dwTid);
	}
}

VOID bh_NtResumeThread(HANDLE ThreadHandle, PULONG SuspendCount) {
	DWORD dwPid = GetProcessIdOfThread(ThreadHandle);
	DWORD dwTid = GetThreadId(ThreadHandle);
	ScanProcess(dwPid, dwTid);
}

VOID bh_LoadLibraryA(LPCSTR LibName) {
	LPVOID pRetAddress;
	HMODULE hModule;
	__asm {
		mov eax, [ebp]
		mov eax, [eax + 4]
		mov pRetAddress, eax
	}
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)pRetAddress, &hModule) || hModule == GetModuleHandleA(NULL)) {
		ScanProcess(GetCurrentProcessId(), GetCurrentThreadId());
	}
}

VOID bh_LoadLibraryW(LPCWSTR LibName) {
	LPVOID pRetAddress;
	HMODULE hModule;
	__asm {
		mov eax, [ebp]
		mov eax, [eax + 4]
		mov pRetAddress, eax
	}
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)pRetAddress, &hModule) || hModule == GetModuleHandleA(NULL)) {
		ScanProcess(GetCurrentProcessId(), GetCurrentThreadId());
	}
}

VOID ah_NtProtectVirtualMemory(
	IN HANDLE               ProcessHandle,
	IN OUT PVOID            *BaseAddress,
	IN OUT PULONG           NumberOfBytesToProtect,
	IN ULONG                NewAccessProtection,
	OUT PULONG              OldAccessProtection,
	DWORD retvalue) {
	LPVOID pRetAddress, pStackFrame;
	HMODULE hModule;
	BOOL bDoScan = FALSE;

	HANDLE hMainHandle = GetModuleHandleA(NULL);

	if (NewAccessProtection == PAGE_EXECUTE || NewAccessProtection == PAGE_EXECUTE_READ || NewAccessProtection == PAGE_EXECUTE_READWRITE || NewAccessProtection == PAGE_EXECUTE_WRITECOPY) {
		__asm {
			mov eax, [ebp]
			mov pStackFrame, eax
		}

		while (!bDoScan) {
			__asm {
				mov eax, pStackFrame
				mov eax, [eax]
				mov pStackFrame, eax
				mov eax, [eax + 4]
				mov pRetAddress, eax
			}
			if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)pRetAddress, &hModule)) {
				if (hModule == hMainHandle)
					bDoScan = TRUE;
				if (hModule == hGlobalModule)
					break;
			}
			else
				bDoScan = TRUE;
		}
		if (bDoScan)
			ScanProcess(GetProcessId(ProcessHandle), 0);
	}
}