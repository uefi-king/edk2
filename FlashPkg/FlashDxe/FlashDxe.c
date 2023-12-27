#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#define WRITE_BYTE_CMD       0x10
#define READ_ARRAY_CMD       0xff
#define CODE_BASE            0x0
#define BLOCK_BASE           0x3C0000

#define mFlashSec L"FlashSec"
#define mFlashSecGuid { 0xbf52cb64, 0x8fb9, 0x46e8, { 0x80, 0x22, 0xdd, 0xd7, 0x85, 0x4b, 0xc4, 0x53 } }

STATIC UINT8                 *mFlashBase                 = NULL;
STATIC UINTN                 mFlashSize                  = 0;
STATIC EFI_EVENT             mSetVirtualAddressMapEvent  = NULL;

EFI_STATUS
EFIAPI
FlashServiceGetFlashSize (
  OUT    UINTN          *FlashSize
  )
{
  *FlashSize = mFlashSize;
  
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FlashServiceReadFlash (
  IN     UINTN          Offset,
  IN OUT UINTN          *DataSize,
  OUT    VOID           *Data
  )
{ 
  DEBUG ((DEBUG_INFO, "FlashServiceReadFlash(%lld, %lld, %p)\n",
          Offset, *DataSize, Data));

  EFI_STATUS             Status;
  UINTN                  BufferSize;
  EFI_GUID MyVarGuid = mFlashSecGuid;
  BufferSize = sizeof(UINTN);

  UINTN    FlashSec;

  Status = gRT->GetVariable(
                  mFlashSec,
                  &MyVarGuid,
                  NULL,
                  &BufferSize,
                  &FlashSec
                  );
  ASSERT_EFI_ERROR (Status);

  if (FlashSec == 0) {
    Offset += CODE_BASE;
  }
  else {
    Offset += BLOCK_BASE;
  }

  if (Offset >= mFlashSize) {
    *DataSize = 0;
  } else if (Offset + *DataSize > mFlashSize) {
    *DataSize = mFlashSize - Offset;
  }

  CopyMem (Data, mFlashBase + Offset, *DataSize);
  
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

  EFI_STATUS             Status;
  UINTN                  BufferSize;
  EFI_GUID MyVarGuid = mFlashSecGuid;
  BufferSize = sizeof(UINTN);

  UINTN    FlashSec;

  Status = gRT->GetVariable(
                  mFlashSec,
                  &MyVarGuid,
                  NULL,
                  &BufferSize,
                  &FlashSec
                  );
  ASSERT_EFI_ERROR (Status);

  if (FlashSec == 0) {
    Offset += CODE_BASE;
  }
  else {
    Offset += BLOCK_BASE;
  }

  if (Offset >= mFlashSize) {
    *DataSize = 0;
  } else if (Offset + *DataSize > mFlashSize) {
    *DataSize = mFlashSize - Offset;
  }

  Ptr = mFlashBase + Offset;
  for (Index = 0; Index < *DataSize; Index++, Ptr++) {
    *Ptr = WRITE_BYTE_CMD;
    *Ptr = ((UINT8 *)Data)[Index];
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
  EFI_STATUS  Status;

  Status = EfiConvertPointer (0x0, (VOID **)&mFlashBase);
  ASSERT_EFI_ERROR (Status);
  DEBUG ((DEBUG_INFO, "Flash base changed: %p\n", mFlashBase));
}

EFI_STATUS
EFIAPI
CreateNVVariable()
{
    EFI_STATUS             Status;
    UINTN                  BufferSize;
    EFI_GUID FlashSecGuid = mFlashSecGuid;
    UINTN    FlashSec = 1;

    BufferSize = sizeof(UINTN);

    Status = gRT->GetVariable(
                    mFlashSec,
                    &FlashSecGuid,
                    NULL,
                    &BufferSize,
                    &FlashSec
                    );
    if (EFI_ERROR(Status)) {
        if (Status == EFI_NOT_FOUND) {
            Status = gRT->SetVariable(
                            mFlashSec,
                            &FlashSecGuid,
                            EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                            BufferSize,
                            &FlashSec
                            );

            DEBUG((DEBUG_INFO, "%s(): Variable %s created in NVRam Var\r\n", mFlashSec));

            return EFI_SUCCESS;
        }
    }
    // already defined once 
    DEBUG((DEBUG_INFO, "%s(): Variable %s already defined in NVRam Var\r\n", mFlashSec));

    return EFI_SUCCESS;
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
  
  mFlashBase = (VOID *)(UINTN)PcdGet32 (PcdOvmfFdBaseAddress);
  mFlashSize = PcdGet32 (PcdOvmfFirmwareFdSize);
  DEBUG ((DEBUG_INFO, "Flash base: %p\n", mFlashBase));
  DEBUG ((DEBUG_INFO, "Flash size: %lld\n", mFlashSize));

  SystemTable->RuntimeServices->GetFlashSize  = FlashServiceGetFlashSize;
  SystemTable->RuntimeServices->ReadFlash     = FlashServiceReadFlash;
  SystemTable->RuntimeServices->WriteFlash    = FlashServiceWriteFlash;

  Status = CreateNVVariable();
  ASSERT_EFI_ERROR (Status);

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