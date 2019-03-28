/******************************************************
* FILENAME:
*       Driver.h
*
* DESCRIPTION:
*       Driver utility functions.
*
*       Copyright Toni Uhlig 2019. All rights reserved.
*
* AUTHOR:
*       Toni Uhlig          START DATE :    27 Mar 19
*/

#pragma once

#include "PE.h"
#include "Native.h"

#include <ntddk.h>

#define PASTDSE L"PastDSE"
#define DEVICE_NAME L"\\Device\\" PASTDSE
#define DEVICE_DOSNAME L"\\DosDevices\\" PASTDSE
#define PASTDSE_DEVICE 0x9C40
#define MMAPDRV_MAXPATH 512
#define IOCTL_PASTDSE_MMAP_DRIVER  (ULONG)CTL_CODE(PASTDSE_DEVICE, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

typedef struct MMAP_DRIVER_INFO {
	wchar_t path[MMAPDRV_MAXPATH];
} MMAP_DRIVER_INFO;


#ifdef _DEBUG_
#define KDBG(fmt, ...) DbgPrint("PastDSE: " fmt, __VA_ARGS__)
#else
#define KDBG(fmt, ...)
#endif

#define PASTDSE_POOL_TAG 'tsaP'

extern PLIST_ENTRY PsLoadedModuleList;

NTSTATUS CheckVersion(void);
PVOID GetKernelBase(OUT PULONG pSize);
NTSTATUS RandomMemory32(PVOID buf, SIZE_T siz);

NTSTATUS BBInitLdrData(IN PKLDR_DATA_TABLE_ENTRY pThisModule);
PVOID BBGetModuleExport(IN PVOID pBase,	IN PCCHAR name_ord);
PKLDR_DATA_TABLE_ENTRY BBGetSystemModule(IN PUNICODE_STRING pName, IN PVOID pAddress);
NTSTATUS BBSafeInitString(OUT PUNICODE_STRING result, IN PUNICODE_STRING source);
NTSTATUS BBResolveImageRefs(IN PVOID pImageBase);
NTSTATUS BBCreateCookie(IN PVOID imageBase);
NTSTATUS BBMapWorker(IN PVOID pArg);
NTSTATUS BBMMapDriver(IN PUNICODE_STRING pPath);
NTSTATUS LdrRelocateImage(IN PVOID NewBase);
PIMAGE_BASE_RELOCATION
LdrProcessRelocationBlock(
	IN ULONG_PTR VA,
	IN ULONG SizeOfBlock,
	IN PUSHORT NextOffset,
	IN LONG_PTR Diff
);
PIMAGE_BASE_RELOCATION
LdrProcessRelocationBlockLongLong(
	IN ULONG_PTR VA,
	IN ULONG SizeOfBlock,
	IN PUSHORT NextOffset,
	IN LONGLONG Diff
);

typedef struct tagACTCTXW
{
	ULONG  cbSize;
	ULONG  dwFlags;
	PWCH   lpSource;
	USHORT wProcessorArchitecture;
	USHORT wLangId;
	PWCH   lpAssemblyDirectory;
	PWCH   lpResourceName;
	PWCH   lpApplicationName;
	PVOID  hModule;
} ACTCTXW, *PACTCTXW;

typedef struct tagACTCTXW32
{
	ULONG  cbSize;
	ULONG  dwFlags;
	ULONG  lpSource;
	USHORT wProcessorArchitecture;
	USHORT wLangId;
	ULONG  lpAssemblyDirectory;
	ULONG  lpResourceName;
	ULONG  lpApplicationName;
	ULONG  hModule;
} ACTCTXW32, *PACTCTXW32;

typedef enum _MmapFlags
{
	KNoFlags = 0x00,    // No flags
	KManualImports = 0x01,    // Manually map import libraries
	KWipeHeader = 0x04,    // Wipe image PE headers
	KHideVAD = 0x10,    // Make image appear as PAGE_NOACESS region
	KRebaseProcess = 0x40,    // If target image is an .exe file, process base address will be replaced with mapped module value

	KNoExceptions = 0x01000, // Do not create custom exception handler
	KNoSxS = 0x08000, // Do not apply SxS activation context
	KNoTLS = 0x10000, // Skip TLS initialization and don't execute TLS callbacks
} KMmapFlags;

typedef struct _USER_CONTEXT
{
	UCHAR code[0x1000];             // Code buffer
	union
	{
		UNICODE_STRING ustr;
		UNICODE_STRING32 ustr32;
	};
	wchar_t buffer[0x400];          // Buffer for unicode string


	// Activation context data
	union
	{
		ACTCTXW actx;
		ACTCTXW32 actx32;
	};
	HANDLE hCTX;
	ULONG hCookie;

	PVOID ptr;                      // Tmp data
	union
	{
		NTSTATUS status;            // Last execution status
		PVOID retVal;               // Function return value
		ULONG retVal32;             // Function return value
	};

	//UCHAR tlsBuf[0x100];
} USER_CONTEXT, *PUSER_CONTEXT;

typedef struct _MMAP_CONTEXT
{
	PEPROCESS pProcess;     // Target process
	PVOID pWorkerBuf;       // Worker thread code buffer
	HANDLE hWorker;         // Worker thread handle
	PETHREAD pWorker;       // Worker thread object
	LIST_ENTRY modules;     // Manual module list
	PUSER_CONTEXT userMem;  // Tmp buffer in user space
	HANDLE hSync;           // APC sync handle
	PKEVENT pSync;          // APC sync object
	PVOID pSetEvent;        // ZwSetEvent address
	PVOID pLoadImage;       // LdrLoadDll address
	BOOLEAN tlsInitialized; // Static TLS was initialized
} MMAP_CONTEXT, *PMMAP_CONTEXT;