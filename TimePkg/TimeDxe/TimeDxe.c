#include "TimeDxe.h"

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include "TimeLib.h"

static UINT64 startTime;

EFI_STATUS
EFIAPI
TimeDxeGetUpTimeSeconds (
  OUT    UINTN          *seconds
  )
{
  UINT64                    currentTime;
  UINT64                    freq;
  EFI_TIMESTAMP_PROPERTIES  properties;

  currentTime = TimeLibGetTimestamp ();
  TimeLibGetProperties (&properties);
  freq = properties.Frequency;

  *seconds = DivU64x64Remainder(currentTime - startTime, freq, NULL);
  
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InitializeTimeDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //EFI_STATUS                Status;
  //EFI_TIMESTAMP_PROPERTIES  Properties;

  DEBUG ((DEBUG_INFO, "InitializeTimeDxe() starts\n"));

  startTime = TimeLibGetTimestamp ();

  DEBUG ((DEBUG_INFO, "InitializeTimeDxe(): startTime = %llx\n", startTime));
  
  SystemTable->RuntimeServices->GetUpTimeSeconds = TimeDxeGetUpTimeSeconds;

  return EFI_SUCCESS;
}
