/******************************************************
* FILENAME:
*       Driver.c
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

#include <ntddk.h>
#include <Ntstrsafe.h>

DRIVER_INITIALIZE DriverEntry;
#pragma alloc_text(INIT, DriverEntry)
DRIVER_UNLOAD DriverUnload;
DRIVER_DISPATCH IODispatch;
#pragma alloc_test(PAGE, IODispatch);


NTSTATUS DriverEntry(
	_In_  struct _DRIVER_OBJECT *DriverObject,
	_In_  PUNICODE_STRING RegistryPath
)
{
	PEPROCESS Process;
	NTSTATUS status;
	UNICODE_STRING deviceName, deviceDosName;
	PDEVICE_OBJECT deviceObject = NULL;

	UNREFERENCED_PARAMETER(RegistryPath);

	status = CheckVersion();
	if (!NT_SUCCESS(status))
		return status;

	KDBG("Initializing ..\n");
	KDBG("System range start: %p\n", MmSystemRangeStart);
	KDBG("Code mapped at....: %p\n", DriverEntry);
	KDBG("DriverObject......: %p\n", DriverObject);

	Process = PsGetCurrentProcess();
	KDBG("Process...........: %lu (%p)\n", PsGetCurrentProcessId(), Process);

	status = BBInitLdrData((PKLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection);
	if (!NT_SUCCESS(status))
		return status;

	DriverObject->MajorFunction[IRP_MJ_CREATE] =
		DriverObject->MajorFunction[IRP_MJ_CLOSE] =
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IODispatch;
	DriverObject->DriverUnload = DriverUnload;

	RtlUnicodeStringInit(&deviceName, DEVICE_NAME);
	RtlUnicodeStringInit(&deviceDosName, DEVICE_DOSNAME);

	status = IoCreateDevice(DriverObject, 0, &deviceName, PASTDSE_DEVICE, FILE_DEVICE_UNKNOWN, FALSE, &deviceObject);
	if (!NT_SUCCESS(status)) {
		KDBG("IoCreateDevice failed with: 0x%X\n", status);
		return status;
	}

	status = IoCreateSymbolicLink(&deviceDosName, &deviceName);
	if (!NT_SUCCESS(status)) {
		KDBG("IoCreateSymbolicLink failed with: 0x%X\n", status);
		return status;
	}

	return STATUS_SUCCESS;
}

VOID
DriverUnload(
	_In_ struct _DRIVER_OBJECT  *DriverObject
)
{
	UNICODE_STRING deviceDosName;

	KDBG("Unloading KMDF ManualDriverMapper with DriverObject: %p\n", DriverObject);

	RtlInitUnicodeString(&deviceDosName, DEVICE_DOSNAME);
	IoDeleteSymbolicLink(&deviceDosName);

	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS IODispatch(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpStack;
	PVOID ioBuffer;
	ULONG inputBufferLength;
	ULONG outputBufferLength;
	ULONG ioControlCode = 0;

	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	irpStack = IoGetCurrentIrpStackLocation(Irp);
	ioBuffer = Irp->AssociatedIrp.SystemBuffer;
	inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
	outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

	KDBG("DriverDispatch....: %u\n", irpStack->MajorFunction);
	switch (irpStack->MajorFunction) {
	case IRP_MJ_DEVICE_CONTROL:
	{
		ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
		KDBG("Dispatch CtrlCode.: 0x%X\n", ioControlCode);

		switch (ioControlCode) {
		case IOCTL_PASTDSE_MMAP_DRIVER:
			if (inputBufferLength == sizeof(MMAP_DRIVER_INFO) && ioBuffer) {
				KDBG("MMAP driver size..: %lu\n", inputBufferLength);
				MMAP_DRIVER_INFO *pMmapDrvInf = (MMAP_DRIVER_INFO *)ioBuffer;
				wchar_t buf[sizeof(pMmapDrvInf->path)];
				UNICODE_STRING ustrPath;

				RtlCopyMemory(buf, pMmapDrvInf->path, sizeof(pMmapDrvInf->path));
				buf[sizeof(pMmapDrvInf->path) - sizeof(wchar_t)] = L'\0';
				RtlUnicodeStringInit(&ustrPath, buf);
				KDBG("MMAP driver path..: %wZ\n", ustrPath);

				Irp->IoStatus.Status = BBMMapDriver(&ustrPath);
			}
			else Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
			break;
		default:
			KDBG("Unknown device control: 0x%X\n", ioControlCode);
			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			break;
		}
	}
	}

	status = Irp->IoStatus.Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}