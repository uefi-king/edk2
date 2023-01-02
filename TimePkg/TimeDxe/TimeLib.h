#ifndef __TIMELIB_H__
#define __TIMELIB_H__

#include <Uefi.h>

typedef struct {
  UINT64    Frequency;
  UINT64    EndValue;
} EFI_TIMESTAMP_PROPERTIES;

UINT64
EFIAPI
TimeLibGetTimestamp (
  VOID
  );

EFI_STATUS
EFIAPI
TimeLibGetProperties (
  OUT   EFI_TIMESTAMP_PROPERTIES  *Properties
  );

EFI_STATUS
EFIAPI
TimeLibInitialize (
  VOID
  );

#endif
