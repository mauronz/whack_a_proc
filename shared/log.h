#include <stdio.h>
#include <Windows.h>

extern BOOL bVerbose;

VOID Log(LPCSTR pFmt, ...);
VOID WLog(LPCWSTR pFmt, ...);