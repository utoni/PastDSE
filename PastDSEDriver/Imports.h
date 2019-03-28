/******************************************************
* FILENAME:
*       Imports.h
*
* DESCRIPTION:
*       Driver utility functions.
*
*       Copyright Toni Uhlig 2019. All rights reserved.
*
* AUTHOR:
*		DarthTon
*       Toni Uhlig          START DATE :    27 Mar 19
*/

#pragma once

#include "Native.h"

#include <ntddk.h>

NTSYSAPI NTSTATUS NTAPI
ZwQueryInformationThread(
	IN HANDLE ThreadHandle,
	IN THREADINFOCLASS ThreadInformationClass,
	OUT PVOID ThreadInformation,
	IN ULONG ThreadInformationLength,
	OUT PULONG ReturnLength OPTIONAL
);

NTSYSAPI NTSTATUS NTAPI
ZwQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL
);

NTSYSAPI
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader(PVOID Base);

NTSYSAPI
PVOID
NTAPI
RtlImageDirectoryEntryToData(
	PVOID ImageBase,
	BOOLEAN MappedAsImage,
	USHORT DirectoryEntry,
	PULONG Size
);

NTSYSAPI
ULONG
NTAPI
RtlRandomEx(
	_Inout_ PULONG Seed
);