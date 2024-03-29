// injector.cpp : Defines the exported functions for the DLL application.
//

#include "injector.h"
#include "hooks.h"
#include "APIhooklib.h"
#include "communication.h"
#include "functions.h"

extern HMODULE hGlobalModule;
extern INJECT_CONFIG config;

TypedefNtCreateUserProcess pOrigNtCreateUserProcess = NULL;
TypedefNtResumeThread pOrigNtResumeThread = NULL;

BOOL SetHooks() {
	SetHookByName("ntdll.dll", "NtCreateThread", 8, CV_STDCALL, (FARPROC)bh_NtCreateThread, NULL, TRUE, FALSE, FALSE);
	SetHookByName("ntdll.dll", "NtCreateThreadEx", 11, CV_STDCALL, (FARPROC)bh_NtCreateThreadEx, NULL, TRUE, FALSE, FALSE);
	pOrigNtResumeThread = (TypedefNtResumeThread)SetHookByName("ntdll.dll", "NtResumeThread", 2, CV_STDCALL, (FARPROC)bh_NtResumeThread, NULL, TRUE, FALSE, FALSE);
	pOrigNtCreateUserProcess = (TypedefNtCreateUserProcess)SetHookByName("ntdll.dll", "NtCreateUserProcess", 11, CV_STDCALL, (FARPROC)bh_NtCreateUserProcess, NULL, FALSE, TRUE, FALSE);
	if (config.ProtectHook) {
		SetHookByName("ntdll.dll", "ZwMapViewOfSection", 10, CV_STDCALL, NULL, (FARPROC)ah_ZwMapViewOfSection, TRUE, FALSE, FALSE);
	}
	if (config.Level >= LEVEL_MEDIUM) {
		SetHookByName("ntdll.dll", "NtProtectVirtualMemory", 5, CV_STDCALL, NULL, (FARPROC)ah_NtProtectVirtualMemory, TRUE, FALSE, FALSE);
	}
	if (config.Level >= LEVEL_HIGH) {
		SetHookByName("kernel32.dll", "LoadLibraryW", 2, CV_STDCALL, (FARPROC)bh_LoadLibraryW, NULL, TRUE, FALSE, FALSE);
		SetHookByName("kernel32.dll", "LoadLibraryA", 2, CV_STDCALL, (FARPROC)bh_LoadLibraryA, NULL, TRUE, FALSE, FALSE);
	}
	return TRUE;
}