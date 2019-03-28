/******************************************************
* FILENAME:
*       BlackBoned.c
*
* DESCRIPTION:
*       Driver utility functions.
*
*       Copyright Toni Uhlig 2019. All rights reserved.
*
* AUTHOR:
*       DarthTon
*       Toni Uhlig          START DATE :    27 Mar 19
*/

#include "Driver.h"
#include "Imports.h"
#include "PE.h"
#include "Native.h"


PLIST_ENTRY PsLoadedModuleList;

#pragma alloc_test(PAGE, BBInitLdrData)
#pragma alloc_text(PAGE, BBGetModuleExport)
#pragma alloc_text(PAGE, BBGetSystemModule)
#pragma alloc_text(PAGE, BBSafeInitString)
#pragma alloc_text(PAGE, BBResolveImageRefs)
#pragma alloc_text(PAGE, BBCreateCookie)
#pragma alloc_text(PAGE, BBMapWorker)
#pragma alloc_text(PAGE, BBMMapDriver)


NTSTATUS BBInitLdrData(IN PKLDR_DATA_TABLE_ENTRY pThisModule)
{
	PVOID kernelBase = GetKernelBase(NULL);
	if (kernelBase == NULL)
	{
		KDBG("Failed to retrieve Kernel base address. Aborting\n");
		return STATUS_NOT_FOUND;
	}

	// Get PsLoadedModuleList address
	for (PLIST_ENTRY pListEntry = pThisModule->InLoadOrderLinks.Flink; pListEntry != &pThisModule->InLoadOrderLinks; pListEntry = pListEntry->Flink)
	{
		// Search for Ntoskrnl entry
		PKLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
		if (kernelBase == pEntry->DllBase)
		{
			// Ntoskrnl is always first entry in the list
			// Check if found pointer belongs to Ntoskrnl module
			if ((PVOID)pListEntry->Blink >= pEntry->DllBase && (PUCHAR)pListEntry->Blink < (PUCHAR)pEntry->DllBase + pEntry->SizeOfImage)
			{
				PsLoadedModuleList = pListEntry->Blink;
				break;
			}
		}
	}

	if (!PsLoadedModuleList)
	{
		KDBG("Failed to retrieve PsLoadedModuleList address. Aborting\n");
		return STATUS_NOT_FOUND;
	}

	return STATUS_SUCCESS;
}

PVOID BBGetModuleExport(IN PVOID pBase, IN PCCHAR name_ord)
{
	PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)pBase;
	PIMAGE_NT_HEADERS64 pNtHdr64 = NULL;
	PIMAGE_EXPORT_DIRECTORY pExport = NULL;
	ULONG expSize = 0;
	ULONG_PTR pAddress = 0;

	ASSERT(pBase != NULL);
	if (pBase == NULL)
		return NULL;

	/// Not a PE file
	if (pDosHdr->e_magic != IMAGE_DOS_SIGNATURE)
		return NULL;

	pNtHdr64 = (PIMAGE_NT_HEADERS64)((PUCHAR)pBase + pDosHdr->e_lfanew);

	// Not a PE file
	if (pNtHdr64->Signature != IMAGE_NT_SIGNATURE)
		return NULL;

	// 64 bit image
	if (pNtHdr64->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		pExport = (PIMAGE_EXPORT_DIRECTORY)(pNtHdr64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (ULONG_PTR)pBase);
		expSize = pNtHdr64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	}
	// 32 bit image
	else return NULL;

	PUSHORT pAddressOfOrds = (PUSHORT)(pExport->AddressOfNameOrdinals + (ULONG_PTR)pBase);
	PULONG  pAddressOfNames = (PULONG)(pExport->AddressOfNames + (ULONG_PTR)pBase);
	PULONG  pAddressOfFuncs = (PULONG)(pExport->AddressOfFunctions + (ULONG_PTR)pBase);

	for (ULONG i = 0; i < pExport->NumberOfFunctions; ++i)
	{
		USHORT OrdIndex = 0xFFFF;
		PCHAR  pName = NULL;

		// Find by index
		if ((ULONG_PTR)name_ord <= 0xFFFF)
		{
			OrdIndex = (USHORT)i;
		}
		// Find by name
		else if ((ULONG_PTR)name_ord > 0xFFFF && i < pExport->NumberOfNames)
		{
			pName = (PCHAR)(pAddressOfNames[i] + (ULONG_PTR)pBase);
			OrdIndex = pAddressOfOrds[i];
		}
		// Weird params
		else
			return NULL;

		if (((ULONG_PTR)name_ord <= 0xFFFF && (USHORT)((ULONG_PTR)name_ord) == OrdIndex + pExport->Base) ||
			((ULONG_PTR)name_ord > 0xFFFF && strcmp(pName, name_ord) == 0))
		{
			pAddress = pAddressOfFuncs[OrdIndex] + (ULONG_PTR)pBase;

			if (pAddress >= (ULONG_PTR)pExport && pAddress <= (ULONG_PTR)pExport + expSize)
				return NULL;
			break;
		}
	}

	return (PVOID)pAddress;
}

PKLDR_DATA_TABLE_ENTRY BBGetSystemModule(IN PUNICODE_STRING pName, IN PVOID pAddress)
{
	ASSERT((pName != NULL || pAddress != NULL) && PsLoadedModuleList != NULL);
	if ((pName == NULL && pAddress == NULL) || PsLoadedModuleList == NULL)
		return NULL;

	// No images
	if (IsListEmpty(PsLoadedModuleList))
		return NULL;

	// Search in PsLoadedModuleList
	for (PLIST_ENTRY pListEntry = PsLoadedModuleList->Flink; pListEntry != PsLoadedModuleList; pListEntry = pListEntry->Flink)
	{
		PKLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		// Check by name or by address
		if ((pName && RtlCompareUnicodeString(&pEntry->BaseDllName, pName, TRUE) == 0) ||
			(pAddress && pAddress >= pEntry->DllBase && (PUCHAR)pAddress < (PUCHAR)pEntry->DllBase + pEntry->SizeOfImage))
		{
			return pEntry;
		}
	}

	return NULL;
}

NTSTATUS BBSafeInitString(OUT PUNICODE_STRING result, IN PUNICODE_STRING source)
{
	ASSERT(result != NULL && source != NULL);
	if (result == NULL || source == NULL || source->Buffer == NULL)
		return STATUS_INVALID_PARAMETER;

	// No data to copy
	if (source->Length == 0)
	{
		result->Length = result->MaximumLength = 0;
		result->Buffer = NULL;
		return STATUS_SUCCESS;
	}

	result->Buffer = ExAllocatePoolWithTag(PagedPool, source->MaximumLength, PASTDSE_POOL_TAG);
	result->Length = source->Length;
	result->MaximumLength = source->MaximumLength;

	memcpy(result->Buffer, source->Buffer, source->Length);

	return STATUS_SUCCESS;
}

NTSTATUS BBResolveImageRefs(IN PVOID pImageBase)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG impSize = 0;
	PIMAGE_IMPORT_DESCRIPTOR pImportTbl = RtlImageDirectoryEntryToData(pImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &impSize);

	// No import libs
	if (pImportTbl == NULL)
		return STATUS_SUCCESS;

	for (; pImportTbl->Name && NT_SUCCESS(status); ++pImportTbl)
	{
		PVOID pThunk = ((PUCHAR)pImageBase + (pImportTbl->OriginalFirstThunk ? pImportTbl->OriginalFirstThunk : pImportTbl->FirstThunk));
		UNICODE_STRING ustrImpDll = { 0 };
		UNICODE_STRING resolved = { 0 };
		ANSI_STRING strImpDll = { 0 };
		ULONG IAT_Index = 0;
		PCCHAR impFunc = NULL;
		union
		{
			PVOID address;
			PKLDR_DATA_TABLE_ENTRY ldrEntry;
		} pModule = { 0 };

		RtlInitAnsiString(&strImpDll, (PCHAR)pImageBase + pImportTbl->Name);
		RtlAnsiStringToUnicodeString(&ustrImpDll, &strImpDll, TRUE);

		// Resolve image name
		BBSafeInitString(&resolved, &ustrImpDll);

		// Get import module
		pModule.address = BBGetSystemModule(&ustrImpDll, NULL);

		// Failed to load
		if (!pModule.address)
		{
			KDBG("Failed to load import '%wZ'. Status code: 0x%X\n", ustrImpDll, status);
			RtlFreeUnicodeString(&ustrImpDll);
			RtlFreeUnicodeString(&resolved);

			return STATUS_NOT_FOUND;
		}

		while (THUNK_VAL_T(pHeader, pThunk, u1.AddressOfData))
		{
			PIMAGE_IMPORT_BY_NAME pAddressTable = (PIMAGE_IMPORT_BY_NAME)((PUCHAR)pImageBase + THUNK_VAL_T(pHeader, pThunk, u1.AddressOfData));
			PVOID pFunc = NULL;

			// import by name
			if (THUNK_VAL_T(pHeader, pThunk, u1.AddressOfData) < IMAGE_ORDINAL_FLAG64 &&
				pAddressTable->Name[0])
			{
				impFunc = pAddressTable->Name;
			}
			// import by ordinal
			else
			{
				impFunc = (PCCHAR)(THUNK_VAL_T(pHeader, pThunk, u1.AddressOfData) & 0xFFFF);
			}

			pFunc = BBGetModuleExport(pModule.ldrEntry->DllBase, impFunc);

			// No export found
			if (!pFunc)
			{
				if (THUNK_VAL_T(pHeader, pThunk, u1.AddressOfData) < IMAGE_ORDINAL_FLAG64 && pAddressTable->Name[0])
					KDBG("Failed to resolve import '%wZ' : '%s'\n", ustrImpDll, pAddressTable->Name);
				else
					KDBG("Failed to resolve import '%wZ' : '%d'\n", ustrImpDll, THUNK_VAL_T(pHeader, pThunk, u1.AddressOfData) & 0xFFFF);

				status = STATUS_NOT_FOUND;
				break;
			}

			// Save address to IAT
			if (pImportTbl->FirstThunk)
				*(PULONG_PTR)((PUCHAR)pImageBase + pImportTbl->FirstThunk + IAT_Index) = (ULONG_PTR)pFunc;
			// Save address to OrigianlFirstThunk
			else
				*(PULONG_PTR)((PUCHAR)pImageBase + THUNK_VAL_T(pHeader, pThunk, u1.AddressOfData)) = (ULONG_PTR)pFunc;

			// Go to next entry
			pThunk = (PUCHAR)pThunk + sizeof(IMAGE_THUNK_DATA64);
			IAT_Index += sizeof(ULONGLONG);
		}

		RtlFreeUnicodeString(&ustrImpDll);
		RtlFreeUnicodeString(&resolved);
	}

	return status;
}

NTSTATUS BBCreateCookie(IN PVOID imageBase)
{
	NTSTATUS status = STATUS_SUCCESS;
	PIMAGE_NT_HEADERS pHeader = RtlImageNtHeader(imageBase);
	if (pHeader)
	{
		ULONG cfgSize = 0;
		PVOID pCfgDir = RtlImageDirectoryEntryToData(imageBase, TRUE, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &cfgSize);

		// TODO: implement proper cookie algorithm
		if (pCfgDir && CFG_DIR_VAL_T(pHeader, pCfgDir, SecurityCookie))
		{
			ULONG seed = (ULONG)(ULONG_PTR)imageBase ^ (ULONG)((ULONG_PTR)imageBase >> 32);
			ULONG_PTR cookie = (ULONG_PTR)imageBase ^  RtlRandomEx(&seed);

			// SecurityCookie value must be rebased by this moment
			*(PULONG_PTR)CFG_DIR_VAL_T(pHeader, pCfgDir, SecurityCookie) = cookie;
		}
	}
	else
		status = STATUS_INVALID_IMAGE_FORMAT;

	return status;
}

NTSTATUS BBMapWorker(IN PVOID pArg)
{
	NTSTATUS status = STATUS_SUCCESS, drvinit_ret;
	HANDLE hFile = NULL;
	PUNICODE_STRING pPath = (PUNICODE_STRING)pArg;
	OBJECT_ATTRIBUTES obAttr = { 0 };
	IO_STATUS_BLOCK statusBlock = { 0 };
	PVOID fileData = NULL;
	PIMAGE_NT_HEADERS pNTHeader = NULL;
	PVOID imageSection = NULL;
	PMDL pMDL = NULL;
	FILE_STANDARD_INFORMATION fileInfo = { 0 };

	InitializeObjectAttributes(&obAttr, pPath, OBJ_KERNEL_HANDLE, NULL, NULL);

	// Open driver file
	status = ZwCreateFile(
		&hFile, FILE_READ_DATA | SYNCHRONIZE, &obAttr,
		&statusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
		FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
	);

	if (!NT_SUCCESS(status))
	{
		KDBG("Failed to open '%wZ'. Status: 0x%X\n", pPath, status);
		PsTerminateSystemThread(status);
		return status;
	}

	// Allocate memory for file contents
	status = ZwQueryInformationFile(hFile, &statusBlock, &fileInfo, sizeof(fileInfo), FileStandardInformation);
	if (NT_SUCCESS(status))
		fileData = ExAllocatePoolWithTag(PagedPool, fileInfo.EndOfFile.QuadPart, PASTDSE_POOL_TAG);
	else
		KDBG("Failed to get '%wZ' size. Status: 0x%X\n", pPath, status);

	// Get file contents
	status = ZwReadFile(hFile, NULL, NULL, NULL, &statusBlock, fileData, fileInfo.EndOfFile.LowPart, NULL, NULL);
	if (NT_SUCCESS(status))
	{
		pNTHeader = RtlImageNtHeader(fileData);
		if (!pNTHeader)
		{
			KDBG("Failed to obtaint NT Header for '%wZ'\n", pPath);
			status = STATUS_INVALID_IMAGE_FORMAT;
		}
	}
	else
		KDBG("Failed to read '%wZ'. Status: 0x%X\n", pPath, status);

	ZwClose(hFile);

	if (NT_SUCCESS(status))
	{
		//
		// Allocate memory from System PTEs
		//
		PHYSICAL_ADDRESS start = { 0 }, end = { 0 };
		end.QuadPart = MAXULONG64;

		pMDL = MmAllocatePagesForMdl(start, end, start, pNTHeader->OptionalHeader.SizeOfImage);
		imageSection = MmGetSystemAddressForMdlSafe(pMDL, NormalPagePriority);

		if (NT_SUCCESS(status) && imageSection)
		{
			// Copy header
			RtlCopyMemory(imageSection, fileData, pNTHeader->OptionalHeader.SizeOfHeaders);

			// Copy sections
			for (PIMAGE_SECTION_HEADER pSection = (PIMAGE_SECTION_HEADER)(pNTHeader + 1);
				pSection < (PIMAGE_SECTION_HEADER)(pNTHeader + 1) + pNTHeader->FileHeader.NumberOfSections;
				pSection++)
			{
				RtlCopyMemory(
					(PUCHAR)imageSection + pSection->VirtualAddress,
					(PUCHAR)fileData + pSection->PointerToRawData,
					pSection->SizeOfRawData
				);
			}

			// Relocate image
			status = LdrRelocateImage(imageSection);
			if (!NT_SUCCESS(status))
				KDBG("Failed to relocate image '%wZ'. Status: 0x%X\n", pPath, status);

			// Fill IAT
			if (NT_SUCCESS(status))
				status = BBResolveImageRefs(imageSection);
		}
		else
		{
			KDBG("Failed to allocate memory for image '%wZ'\n", pPath);
			status = STATUS_MEMORY_NOT_ALLOCATED;
		}
	}

	// Initialize kernel security cookie
	if (NT_SUCCESS(status))
		BBCreateCookie(imageSection);

	// Call entry point
	if (NT_SUCCESS(status) && pNTHeader->OptionalHeader.AddressOfEntryPoint)
	{
		PDRIVER_INITIALIZE pEntryPoint = (PDRIVER_INITIALIZE)((ULONG_PTR)imageSection + pNTHeader->OptionalHeader.AddressOfEntryPoint);
		drvinit_ret = pEntryPoint(NULL, imageSection);
		UNREFERENCED_PARAMETER(drvinit_ret);
		KDBG("MMAP driver init returned 0x%X\n", drvinit_ret);
	}

	// Wipe header
	if (NT_SUCCESS(status) && imageSection)
		RandomMemory32(imageSection, pNTHeader->OptionalHeader.SizeOfHeaders);

	// Erase info about allocated region
	if (pMDL)
	{
		// Free image memory in case of failure
		if (!NT_SUCCESS(status)) {
			MmFreePagesFromMdl(pMDL);
		}
		ExFreePool(pMDL);
	}

	if (fileData)
		ExFreePoolWithTag(fileData, PASTDSE_POOL_TAG);

	if (NT_SUCCESS(status))
		KDBG("Successfully mapped '%wZ' at 0x%p\n", pPath, imageSection);

	PsTerminateSystemThread(status);
	return status;
}

NTSTATUS BBMMapDriver(IN PUNICODE_STRING pPath)
{
	HANDLE hThread = NULL;
	CLIENT_ID clientID = { 0 };
	OBJECT_ATTRIBUTES obAttr = { 0 };
	PETHREAD pThread = NULL;
	OBJECT_HANDLE_INFORMATION handleInfo = { 0 };

	InitializeObjectAttributes(&obAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

	ASSERT(pPath != NULL);
	if (pPath == NULL)
		return STATUS_INVALID_PARAMETER;

	NTSTATUS status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, &obAttr, NULL, &clientID, &BBMapWorker, pPath);
	if (!NT_SUCCESS(status))
	{
		KDBG("Failed to create worker thread. Status: 0x%X\n", status);
		return status;
	}

	// Wait on worker thread
	status = ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &pThread, &handleInfo);
	if (NT_SUCCESS(status))
	{
		THREAD_BASIC_INFORMATION info = { 0 };
		ULONG bytes = 0;

		status = KeWaitForSingleObject(pThread, Executive, KernelMode, TRUE, NULL);
		status = ZwQueryInformationThread(hThread, ThreadBasicInformation, &info, sizeof(info), &bytes);
		if (NT_SUCCESS(status));
		status = info.ExitStatus;
	}

	if (pThread)
		ObDereferenceObject(pThread);

	return status;
}