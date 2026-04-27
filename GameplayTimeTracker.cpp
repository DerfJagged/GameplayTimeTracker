//--------------------------------------------------------------------------------------
// GameplayTimeTracker.cpp
//
// A DashLaunch plugin to log title ID changes for use with the Gameplay Time Tracker Aurora script.
//
// Credit to Xenomega for their plugin/symlink code:
// https://github.com/Xenomega/ArchLoader/tree/master
//
// Special thanks to Byrom for finding the undocumented function GetCurrentOGXboxTitleId()
//--------------------------------------------------------------------------------------
#include <xtl.h>
#include <stdio.h>
#include <time.h>
#include "GameplayTimeTracker.h"

const char* filename = "HDD:\\Aurora\\User\\Scripts\\Utility\\GameplayTimeTracker\\session.log";
bool guide_open = true;
char tempString[30];
FILE* file;
GetXB1TID* XamLoaderGetCurrentXbox1TitleId = NULL;
static HANDLE g_hThread = NULL;
static HANDLE g_hNotify = NULL;
static DWORD  g_LastTitleId = 0;
static DWORD  g_NewTitleId = 0;
static DWORD  g_LastOriginalXboxTitleId = 0;
static DWORD  g_NewOriginalXboxTitleId = 0;
static DWORD  g_ThreadId;

static void Main01(){
	printf("\n[GTT] Gameplay Time Tracker boot plugin started!");
    StartTracker();
}

BOOL APIENTRY DllMain(HANDLE hInstDLL, DWORD reason, LPVOID lpReserved){
	if(reason == DLL_PROCESS_ATTACH){
		HANDLE hThread;
        DWORD hThreadId;
		ExCreateThread(&hThread, 0, &hThreadId, (VOID*)XapiThreadStartup , (LPTHREAD_START_ROUTINE)Main01, NULL, 0x2);
		XSetThreadProcessor(hThread, 4); ResumeThread(hThread); CloseHandle(hThread);
	}
 	return TRUE;
}

HRESULT CreateSymbolicLink(CHAR* szDrive, CHAR* szDeviceName, BOOL System) {
	// Setup our path
	CHAR szDestinationDrive[MAX_PATH];
	if(System)
		sprintf_s(szDestinationDrive, MAX_PATH, "\\System??\\%s", szDrive);
	else
		sprintf_s(szDestinationDrive, MAX_PATH, "\\??\\%s", szDrive);

	// Setup our strings
	ANSI_STRING linkname, devicename;
	RtlInitAnsiString(&linkname, szDestinationDrive);
	RtlInitAnsiString(&devicename, szDeviceName);

	// Create finally
	NTSTATUS status = ObCreateSymbolicLink(&linkname, &devicename);
	if (status >= 0)
		return S_OK;
	return S_FALSE;
}

DWORD WINAPI TitleMonitorThread(LPVOID) {
	// Heartbeat logs out every 5 minutes
	//printf("\n[GTT] Print from new thread");
    
	while(true) {
		Sleep(300000); //5 minutes
		LogTitleID(0, LOGTYPE_HEARTBEAT);
	}

	return 0;
}

void LogTitleID(DWORD g_NewTitleId, short logtype) {
	file = fopen(filename, "a");
	
    if (file) {
		if (logtype == LOGTYPE_HEARTBEAT) {
			//printf("\n[GTT] Logging heartbeat to %s\n", filename);
			sprintf(tempString, "heartbeat,%lu\n", (unsigned long)time(NULL));
		}
		else if (logtype == LOGTYPE_TITLEIDCHANGE) {
			//printf("\n[GTT] Logging title ID %08X to %s\n", g_NewTitleId, filename);
			sprintf(tempString, "%08X,%lu\n", g_NewTitleId, (unsigned long)time(NULL));
		}
		else if (logtype == LOGTYPE_CONSOLESTART) {
			sprintf(tempString, "consolestart,%lu\n", (unsigned long)time(NULL));
		}

		fputs(tempString, file);
        fclose(file);
	}
	else {
        perror("\n[GTT] Failed to open file");
    }
}

void StartTracker() {
	CreateSymbolicLink("HDD:", "\\Device\\Harddisk0\\Partition1", TRUE);
	g_LastTitleId = XamGetCurrentTitleId();
	
	LogTitleID(0, LOGTYPE_CONSOLESTART);
	LogTitleID(g_LastTitleId, LOGTYPE_TITLEIDCHANGE);
	//printf("\nInitial title ID: %08X", g_LastTitleId);

	// Setup system event listener for title launches
    g_hNotify = XamNotifyCreateListener(XNOTIFY_SYSTEM);
    if (!g_hNotify) {
		printf("\n[GTT] hNotify creation failed");
		return;
	}

	// Create heartbeat thread
	ExCreateThread(&g_hThread, 0, &g_ThreadId, NULL, TitleMonitorThread, NULL, 0x2);
	XSetThreadProcessor(g_hThread, 4);
	ResumeThread(g_hThread);

	// Loop system event listener
	while (true) {
		DWORD notifyId = 0;
		ULONG_PTR param = 0;

		// Block until a notification is available
		WaitForSingleObject(g_hNotify, INFINITE);

		if (XNotifyGetNext(g_hNotify, 0, &notifyId, &param)) {
			//printf("\n[GTT] SYSTEM EVENT: %lu", notifyId);
			if (notifyId == XNOTIFY_SYSTEM_TITLE_CHANGED) {
				// New title launched
				g_NewTitleId = XamGetCurrentTitleId();
				if (g_NewTitleId != g_LastTitleId) {
					//printf("\n[GTT] Title ID change detected");
					//printf("\n[GTT] Last title: %08X", g_LastTitleId);
					//printf("\n[GTT] New title: %08X", g_NewTitleId);
					if (g_NewTitleId == 0xFFFE07D2) {
						// Original Xbox game
						g_NewOriginalXboxTitleId = GetCurrentOGXboxTitleId();
						if (g_NewOriginalXboxTitleId != g_LastOriginalXboxTitleId) {
							//printf("\nOriginal Xbox Title ID detected: %08X", g_NewOriginalXboxTitleId);
							LogTitleID(g_NewOriginalXboxTitleId, LOGTYPE_TITLEIDCHANGE);
							g_LastOriginalXboxTitleId = g_NewOriginalXboxTitleId;
						}
					}
					else {
						LogTitleID(g_NewTitleId, LOGTYPE_TITLEIDCHANGE);
						g_LastOriginalXboxTitleId = 0;
					}
					g_LastTitleId = g_NewTitleId;
				}
			}
			else if (notifyId == XNOTIFY_GUIDE_OPENED_OR_CLOSED) {
				// Log heartbeat on guide opening to capture time between last one and shutdown
				// If you know how to properly hook shutdowns, please let Derf know!
				
				// Two events always fire - one for open, one for close; skip every other one
				if (guide_open) {
					g_NewTitleId = XamGetCurrentTitleId();
					if (g_NewTitleId != 0x00000000 && // Aurora / homebrew
						g_NewTitleId != 0xF5D10000 && // Aurora / FSD
						g_NewTitleId != 0xFFFF0055 && // XeXmenu
						g_NewTitleId != 0xFFFE07D1 && // Dashboard
						g_NewTitleId != 0xFFFF011D) { // DashLaunch
						LogTitleID(0, LOGTYPE_HEARTBEAT);
					}
					guide_open = false;
				}
				else {
					guide_open = true;
				}
			}
		}
	}

	printf("\n[GTT] GameplayTimeTracker shutting down...");
}

DWORD GetCurrentOGXboxTitleId() {
    if (!XamLoaderGetCurrentXbox1TitleId) { // Check if we've resolved the address yet
        // The function we need isn't exported but a function just after is (XamLoaderGetDvdTrayState).
        // We can resolve the address for that and minus the number of bytes to go back to the start of the function we want (XamLoaderGetCurrentXbox1TitleId).
        DWORD XamLoaderGetDvdTrayState_Addr = (DWORD)ResolveFunction(MODULE_XAM, XamLoaderGetDvdTrayState_ORD);
        if (XamLoaderGetDvdTrayState_Addr) {
            DWORD XamLoaderGetCurrentXbox1TitleId_Addr = XamLoaderGetDvdTrayState_Addr - 24;
            // A few checks to confirm before we call or hook
            // lis       r11, _g_XamTitleLoader__3PAVCXamTitleLoader__A@ha
            // mr        r5, r4
            // mr        r4, r3
            if ((*(WORD*)XamLoaderGetCurrentXbox1TitleId_Addr == 0x3D60) && ((*(QWORD*)(XamLoaderGetCurrentXbox1TitleId_Addr + 4) == 0x7C8523787C641B78))) {
                printf("\n[GTT] Successfully resolved correct address of XamLoaderGetCurrentXbox1TitleId!");
                XamLoaderGetCurrentXbox1TitleId = (GetXB1TID*)XamLoaderGetCurrentXbox1TitleId_Addr;
            }
            else {
                printf("\n[GTT] Failed to resolve correct address of XamLoaderGetCurrentXbox1TitleId!");
                return 0;
            }
        }
        else {
            printf("\n[GTT] [GetCurrentOGXboxTitleId] Failed to resolve correct address of XamLoaderGetCurrentXbox1TitleId! [2]");
            return 0;
        }
    }

    // All good to call the function
    return XamLoaderGetCurrentXbox1TitleId(0, 0);
}

PDWORD ResolveFunction(char* moduleName, DWORD ordinal) {
    HMODULE moduleHandle = GetModuleHandle(moduleName);

    if (!moduleHandle) {
        return 0;
	}

    return (PDWORD)GetProcAddress(moduleHandle, (LPCSTR)ordinal);
}
