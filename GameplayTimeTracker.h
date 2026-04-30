#pragma once
#ifndef _GAMEPLAYTIMETRACKER_H
#define _GAMEPLAYTIMETRACKER_H
#define MODULE_XAM "xam.xex"
#define XamLoaderGetDvdTrayState_ORD 426
#define XNOTIFY_GUIDE_OPENED_OR_CLOSED 0x00000009
#define XNOTIFY_SYSTEM_TITLE_CHANGED 0x0000000A
#define LOGTYPE_CONSOLESTART 0
#define LOGTYPE_TITLEIDCHANGE 1
#define LOGTYPE_HEARTBEAT 2
#define INVALID_FILE_ATTRIBUTES -1

typedef long NTSTATUS;
typedef unsigned __int64 QWORD;
typedef DWORD GetXB1TID(DWORD r3, DWORD r4);

typedef struct _ANSI_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PCHAR Buffer;
} STRING, *PSTRING, ANSI_STRING, *PANSI_STRING;

// Functions
void StartTracker();
void LogTitleID(DWORD g_NewTitleId, short logtype);
BOOL DirectoryExists(const char* path);
HRESULT CreateSymbolicLink(CHAR* szDrive, CHAR* szDeviceName, BOOL System);
PDWORD ResolveFunction(char* moduleName, DWORD ordinal);
DWORD GetCurrentOGXboxTitleId();
DWORD WINAPI HeartbeatThread(LPVOID);

EXTERN_C { 
	DWORD ExCreateThread(PHANDLE pHandle, DWORD dwStackSize, LPDWORD lpThreadId, 
	VOID* apiThreadStartup, LPTHREAD_START_ROUTINE lpStartAddress, 
	LPVOID lpParameter, DWORD dwCreationFlagsMod);
	VOID  XapiThreadStartup(VOID (__cdecl *StartRoutine)(VOID*), VOID* StartContext);
	NTSTATUS ObCreateSymbolicLink(PANSI_STRING, PANSI_STRING);
	NTSTATUS ObDeleteSymbolicLink(PANSI_STRING);
	void RtlInitAnsiString(PANSI_STRING DestinationString, const char* SourceString);
	DWORD XamGetCurrentTitleId();
	HANDLE XamNotifyCreateListener(DWORD dwFlags);
}

#endif
