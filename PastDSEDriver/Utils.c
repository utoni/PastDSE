/******************************************************
* FILENAME:
*       Utils.c
*
* DESCRIPTION:
*       Driver utility functions.
*
*       Copyright Toni Uhlig 2019. All rights reserved.
*
* AUTHOR:
*       Toni Uhlig          START DATE :    27 Mar 19
*/

#include "Driver.h"
#include "Imports.h"

#include <ntstrsafe.h>


#pragma alloc_text(PAGE, CheckVersion)
#pragma alloc_text(PAGE, GetKernelBase)
#pragma alloc_text(PAGE, RandomMemory32)

PVOID g_KernelBase = NULL;
ULONG g_KernelSize = 0;

NTSTATUS CheckVersion(void)
{
	NTSTATUS status;
	RTL_OSVERSIONINFOW osver = { 0 };

	status = RtlGetVersion(&osver);

	if (NT_SUCCESS(status))
	{
		KDBG("Os version........: %d.%d.%d",
			osver.dwMajorVersion,
			osver.dwMinorVersion,
			osver.dwBuildNumber);

		if (osver.dwMajorVersion != 10 ||
			osver.dwMinorVersion != 0 ||
			osver.dwBuildNumber != 17134)
		{
			/* TODO: Verify on other builds */
			KDBG("WARNING: ONLY Windows 10.0.17134 (1803/RS4) supported at the moment!\n");
			return STATUS_ACCESS_DENIED;
		}
	}

	return status;
}

PVOID GetKernelBase(OUT PULONG pSize)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG bytes = 0;
	PRTL_PROCESS_MODULES pMods = NULL;
	PVOID checkPtr = NULL;
	UNICODE_STRING routineName;

	// Already found
	if (g_KernelBase != NULL)
	{
		if (pSize)
			*pSize = g_KernelSize;
		return g_KernelBase;
	}

	RtlUnicodeStringInit(&routineName, L"NtOpenFile");

	checkPtr = MmGetSystemRoutineAddress(&routineName);
	if (checkPtr == NULL)
		return NULL;

	// Protect from UserMode AV
	status = ZwQuerySystemInformation(SystemModuleInformation, 0, bytes, &bytes);
	if (bytes == 0)
	{
		KDBG("Invalid SystemModuleInformation size\n");
		return NULL;
	}

	pMods = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, bytes, PASTDSE_POOL_TAG);
	RtlZeroMemory(pMods, bytes);

	status = ZwQuerySystemInformation(SystemModuleInformation, pMods, bytes, &bytes);

	if (NT_SUCCESS(status))
	{
		PRTL_PROCESS_MODULE_INFORMATION pMod = pMods->Modules;

		for (ULONG i = 0; i < pMods->NumberOfModules; i++)
		{
			// System routine is inside module
			if (checkPtr >= pMod[i].ImageBase &&
				checkPtr < (PVOID)((PUCHAR)pMod[i].ImageBase + pMod[i].ImageSize))
			{
				g_KernelBase = pMod[i].ImageBase;
				g_KernelSize = pMod[i].ImageSize;
				if (pSize)
					*pSize = g_KernelSize;
				break;
			}
		}
	}

	if (pMods)
		ExFreePoolWithTag(pMods, PASTDSE_POOL_TAG);

	return g_KernelBase;
}

NTSTATUS RandomMemory32(PVOID buf, SIZE_T siz)
{
	PUINT32 ptr = (PUINT32)buf;
	SIZE_T i;
	ULONG seed = RtlRandomEx(buf);

	if (siz < 4)
		return STATUS_INFO_LENGTH_MISMATCH;
	for (i = 0; i < siz; i += 4) {
		ptr[i] = seed;
		seed = RtlRandomEx(&seed);
	}
	ptr[i - 4] = seed;

	return STATUS_SUCCESS;
}