#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>

#define ENCRYPTION_KEY  0xbf

#define WRITE_BYTE_CMD           0x10
#define CLEAR_STATUS_CMD         0x50
#define READ_STATUS_CMD          0x70
#define READ_ARRAY_CMD           0xff

#define CLEARED_ARRAY_STATUS  0x00

STATIC UINT8  *mFlashBase = NULL;
STATIC UINTN  mFlashSize  = 0;
STATIC UINTN  mFdBlockSize  = 0;
STATIC UINTN  mFdBlockCount = 0;
STATIC EFI_EVENT  mSetVirtualAddressMapEvent  = NULL;

EFI_STATUS
EFIAPI
FlashServiceGetFlashSize (
  OUT    UINTN          *FlashSize
  )
{
  DEBUG((DEBUG_INFO, "GetFlashSize called\n"));
  *FlashSize = mFlashSize;
  
  return EFI_SUCCESS;
}

STATIC
volatile UINT8 *
QemuFlashPtr (
  IN        EFI_LBA  Lba,
  IN        UINTN    Offset
  )
{
  return mFlashBase + ((UINTN)Lba * mFdBlockSize) + Offset;
}

/**
  Determines if the QEMU flash memory device is present.

  @retval FALSE   The QEMU flash device is not present.
  @retval TRUE    The QEMU flash device is present.

**/
STATIC
BOOLEAN
QemuFlashDetected (
  VOID
  )
{
  BOOLEAN         FlashDetected;
  volatile UINT8  *Ptr;

  UINTN  Offset;
  UINT8  OriginalUint8;
  UINT8  ProbeUint8;

  FlashDetected = FALSE;
  Ptr           = QemuFlashPtr (0, 0);

  for (Offset = 0; Offset < mFdBlockSize; Offset++) {
    Ptr        = QemuFlashPtr (0, Offset);
    ProbeUint8 = *Ptr;
    if ((ProbeUint8 != CLEAR_STATUS_CMD) &&
        (ProbeUint8 != READ_STATUS_CMD) &&
        (ProbeUint8 != CLEARED_ARRAY_STATUS))
    {
      break;
    }
  }

  if (Offset >= mFdBlockSize) {
    DEBUG ((DEBUG_INFO, "QEMU Flash: Failed to find probe location\n"));
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "QEMU Flash: Attempting flash detection at %p\n", Ptr));

  OriginalUint8 = *Ptr;
  *Ptr          = CLEAR_STATUS_CMD;
  ProbeUint8    = *Ptr;
  if ((OriginalUint8 != CLEAR_STATUS_CMD) &&
      (ProbeUint8 == CLEAR_STATUS_CMD))
  {
    DEBUG ((DEBUG_INFO, "QemuFlashDetected => FD behaves as RAM\n"));
    *Ptr = OriginalUint8;
  } else {
    *Ptr       = READ_STATUS_CMD;
    ProbeUint8 = *Ptr;
    if (ProbeUint8 == OriginalUint8) {
      DEBUG ((DEBUG_INFO, "QemuFlashDetected => FD behaves as ROM\n"));
    } else if (ProbeUint8 == READ_STATUS_CMD) {
      DEBUG ((DEBUG_INFO, "QemuFlashDetected => FD behaves as RAM\n"));
      *Ptr = OriginalUint8;
    } else if (ProbeUint8 == CLEARED_ARRAY_STATUS) {
      *Ptr       = WRITE_BYTE_CMD;
      *Ptr       = OriginalUint8;
      *Ptr       = READ_STATUS_CMD;
      ProbeUint8 = *Ptr;
      *Ptr       = READ_ARRAY_CMD;
      if (ProbeUint8 & 0x10 /* programming error */) {
        DEBUG ((DEBUG_INFO, "QemuFlashDetected => FD behaves as FLASH, write-protected\n"));
      } else {
        DEBUG ((DEBUG_INFO, "QemuFlashDetected => FD behaves as FLASH, writable\n"));
        FlashDetected = TRUE;
      }
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "QemuFlashDetected => %a\n",
    FlashDetected ? "Yes" : "No"
    ));
  return FlashDetected;
}

#define FLASH_RW_SIZE(Offset, DataSize) \
  ((Offset >= mFlashSize) \
    ? 0 \
    : ((Offset + DataSize > mFlashSize) \
      ? mFlashSize - Offset \
      : DataSize))


EFI_STATUS
EFIAPI
FlashServiceReadFlash (
  IN     UINTN          Offset,
  IN OUT UINTN          *DataSize,
  OUT    VOID           *Data
  )
{ 
  UINTN                 Index;

  DEBUG ((DEBUG_INFO, "FlashServiceReadFlash(%lld, %lld, %p)\n",
          Offset, *DataSize, Data));

  *DataSize = FLASH_RW_SIZE(Offset, *DataSize);

  CopyMem (Data, mFlashBase + Offset, *DataSize);

  for (Index = 0; Index < *DataSize; Index++) {
    ((UINT8 *)Data)[Index] = ((UINT8 *)Data)[Index] ^ (UINT8)ENCRYPTION_KEY;
  }
  
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FlashServiceWriteFlash (
  IN     UINTN          Offset,
  IN OUT UINTN          *DataSize,
  OUT    VOID           *Data
  )
{
  UINT8                 *Ptr;
  UINTN                 Index;

  DEBUG ((DEBUG_INFO, "FlashServiceWriteFlash(%lld, %lld, %p)\n",
          Offset, *DataSize, Data));

  *DataSize = FLASH_RW_SIZE(Offset, *DataSize);

  Ptr = mFlashBase + Offset;
  for (Index = 0; Index < *DataSize; Index++, Ptr++) {
    *Ptr = WRITE_BYTE_CMD;
    *Ptr = ((UINT8 *)Data)[Index] ^ (UINT8)ENCRYPTION_KEY;
  }
  
  if (*DataSize > 0) {
    *(Ptr - 1) = READ_ARRAY_CMD;
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
OnSetVirtualAddressMap (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  ASSERT_EFI_ERROR (EfiConvertPointer (0x0, (VOID **)&mFlashBase));
}

EFI_STATUS
EFIAPI
FlashServiceInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;

  DEBUG ((DEBUG_INFO, "FlashServiceInitialize()\n"));
  
  mFlashBase   = (UINT8 *)(UINTN)PcdGet32 (PcdOvmfFdBaseAddress);
  mFdBlockSize = PcdGet32 (PcdOvmfFirmwareBlockSize);
  mFlashSize = PcdGet32 (PcdOvmfFirmwareFdSize);
  ASSERT (mFlashSize % mFdBlockSize == 0);
  mFdBlockCount = PcdGet32 (PcdOvmfFirmwareFdSize) / mFdBlockSize; 
  
  DEBUG ((DEBUG_INFO, "[FlashService] Flash base:    %p\n", mFlashBase));
  DEBUG ((DEBUG_INFO, "[FlashService] FdBlock size:  %p\n", mFdBlockSize));
  DEBUG ((DEBUG_INFO, "[FlashService] FdBlock count: %p\n", mFdBlockCount));
  DEBUG ((DEBUG_INFO, "[FlashService] Flash size:    %lld\n", mFlashSize));

  if (!QemuFlashDetected()) {
    ASSERT (!FeaturePcdGet (PcdSmmSmramRequire));
    return EFI_WRITE_PROTECTED;
  }

  SystemTable->RuntimeServices->GetFlashSize  = FlashServiceGetFlashSize;
  SystemTable->RuntimeServices->ReadFlash     = FlashServiceReadFlash;
  SystemTable->RuntimeServices->WriteFlash    = FlashServiceWriteFlash;

  Status = gBS->CreateEvent (
    EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
    TPL_NOTIFY,
    OnSetVirtualAddressMap,
    NULL,
    &mSetVirtualAddressMapEvent
  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}