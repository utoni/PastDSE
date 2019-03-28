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

#define DEVICE_NAME L"PastDSE"
#define PASTDSE_DEVICE 0x9C40
#define MMAPDRV_MAXPATH 512
#define IOCTL_PASTDSE_MMAP_DRIVER  (ULONG)CTL_CODE(PASTDSE_DEVICE, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

typedef struct MMAP_DRIVER_INFO {
	wchar_t path[MMAPDRV_MAXPATH];
} MMAP_DRIVER_INFO;