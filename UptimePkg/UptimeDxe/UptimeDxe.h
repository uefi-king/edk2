#ifndef __EDK2_UPTIME_DXE_H_
#define __EDK2_UPTIME_DXE_H_

#include <Uefi.h>

EFI_STATUS
EFIAPI
UptimeInitialize (
    IN EFI_HANDLE         ImageHandle,
    IN EFI_SYSTEM_TABLE   *SystemTable
    );

#endif // __EDK2_UPTIME_DXE_H_