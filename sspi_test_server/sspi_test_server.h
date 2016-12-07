//  SspiExample.h
#include <sspi.h>
#include <windows.h>

BOOL SendMsg(SOCKET s, PBYTE pBuf, DWORD cbBuf);
BOOL ReceiveMsg(SOCKET s, PBYTE pBuf, DWORD cbBuf, DWORD *pcbRead);
BOOL SendBytes(SOCKET s, PBYTE pBuf, DWORD cbBuf);
BOOL ReceiveBytes(SOCKET s, PBYTE pBuf, DWORD cbBuf, DWORD *pcbRead);
void cleanup();

BOOL GenServerContext(
	BYTE *pIn,
	DWORD cbIn,
	BYTE *pOut,
	DWORD *pcbOut,
	BOOL *pfDone,
	BOOL  fNewCredential
);

void PrintHexDump(DWORD length, PBYTE buffer);

BOOL DoAuthentication(SOCKET s);
