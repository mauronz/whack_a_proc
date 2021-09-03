#include "log.h"

BOOL bVerbose = FALSE;

VOID Log(LPCSTR pFmt, ...) {
	if (!bVerbose)
		return;

	va_list argp;
	va_start(argp, pFmt);
	vprintf(pFmt, argp);
	va_end(argp);
}

VOID WLog(LPCWSTR pFmt, ...) {
	if (!bVerbose)
		return;

	va_list argp;
	va_start(argp, pFmt);
	vwprintf(pFmt, argp);
	va_end(argp);
}