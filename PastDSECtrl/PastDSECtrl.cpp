/******************************************************
* FILENAME:
*       PastDSECtrl.cpp
*
* DESCRIPTION:
*       Driver utility functions.
*
*       Copyright Toni Uhlig 2019. All rights reserved.
*
* AUTHOR:
*       Toni Uhlig          START DATE :    27 Mar 19
*/

#include "pch.h"
#include "Driver.h"

#include <iostream>
#include <windows.h>
#include <shlwapi.h>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

int main(int argc, char **argv)
{
	HANDLE hDevice;
	wchar_t wpath[MMAPDRV_MAXPATH] = { L".\\DummyDrv.sys" };
	wchar_t fullpath[MMAPDRV_MAXPATH] = { L'\0' };
	MMAP_DRIVER_INFO mmdrvinf = { { L'\0' } };
	BOOL ret;

	if (argc > 1) {
		mbstowcs_s(NULL, wpath, MMAPDRV_MAXPATH, argv[1], strlen(argv[1]));
	}

	if (!_wfullpath(mmdrvinf.path, wpath, MMAPDRV_MAXPATH)) {
		wprintf(L"Realpath failed for: %ws\n", wpath);
		return 1;
	}

	wnsprintfW(fullpath, MMAPDRV_MAXPATH, L"%s%s", L"\\??\\", mmdrvinf.path);
	memcpy(mmdrvinf.path, fullpath, MMAPDRV_MAXPATH * sizeof(wchar_t));

	wprintf(L"Driver for manual mapping: %ws\n", mmdrvinf.path);
	wprintf(L"Device file: %ws\n", L"\\\\.\\" DEVICE_NAME);

	hDevice = CreateFile(L"\\\\.\\" DEVICE_NAME, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDevice == INVALID_HANDLE_VALUE) {
		wprintf(L"CreateFile Error: 0x%X", GetLastError());
		return 1;
	}
	wprintf(L"Handle : %p\n", hDevice);

	ret = DeviceIoControl(hDevice, IOCTL_PASTDSE_MMAP_DRIVER, /* argv[1], strlen(argv[1]) */ (LPVOID)&mmdrvinf, (DWORD) sizeof(mmdrvinf), NULL, 0, NULL, NULL);
	if (!ret) {
		wprintf(L"DeviceIoControl Error: 0x%X", GetLastError());
		return 1;
	}
	wprintf(L"DeviceIoControl returned: %s , GetLastError: 0x%X\n",
		(ret ? L"TRUE" : L"FALSE"),
		GetLastError());

	CloseHandle(hDevice);

	return 0;
}