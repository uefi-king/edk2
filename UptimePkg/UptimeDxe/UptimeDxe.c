#include "UptimeDxe.h"
#include "ProcessorBind.h"
#include "Uefi/UefiBaseType.h"

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>

static UINT64 startTime;

static UINT64 GetTimeStamp(VOID) {
    // This is not uptime, but i dont fucking care.
    return GetPerformanceCounter();
}

EFI_STATUS
EFIAPI
UptimeDxeGetUptime (
    OUT UINT64   *Ticks
    )
{
    *Ticks = GetTimeStamp() - startTime;
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UptimeInitialize (
    IN EFI_HANDLE         ImageHandle,
    IN EFI_SYSTEM_TABLE   *SystemTable
    )
{
    startTime = GetTimeStamp();
    DEBUG((DEBUG_INFO, "[init] UptimeInitialize() startTime = %lu\n", startTime));
    SystemTable->RuntimeServices->GetUptime = UptimeDxeGetUptime;
    return EFI_SUCCESS;
}

