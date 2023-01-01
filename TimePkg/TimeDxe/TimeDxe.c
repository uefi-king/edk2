#include "TimeDxe.h"

#include <Library/DebugLib.h>
//#include <Protocol/Timestamp.h>

//static UINT64 freq;
//static UINT64 bound;

/*EFI_STATUS
EFIAPI
TimeDxeGetUpTimeSeconds (
  OUT    UINTN          *seconds
  )
{
  *seconds = 0;
  
  return EFI_SUCCESS;
}*/

EFI_STATUS
EFIAPI
InitializeTimeDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{

  DEBUG ((DEBUG_INFO, "InitializeTimeDxe()\n"));

  //SystemTable->RuntimeServices->GetUpTimeSeconds  = TimeDxeGetUpTimeSeconds;

  return EFI_SUCCESS;
}
