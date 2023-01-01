#ifndef __TIMEDXE_H__
#define __TIMEDXE_H__

#include <Uefi.h>

EFI_STATUS
EFIAPI
InitializeTimeDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif
