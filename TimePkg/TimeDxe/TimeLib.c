#include "TimeLib.h"

#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseMemoryLib.h>

// We use rdtsc as our time counter implementation, so
// we can assume that startValue = 0 and endValue = 0xFFFFFFFFFFFFFFFF.

static BOOLEAN initialized = FALSE;

static UINT64  mTimerLibStartValue = 0;
static UINT64  mTimerLibEndValue = 0xffffffffffffffffULL;

//
// The properties of timestamp
//
EFI_TIMESTAMP_PROPERTIES  mTimestampProperties = {
  0,
  0
};

EFI_STATUS
EFIAPI
TimeLibInitializeIfNot (
  VOID
  )
{
  if (!initialized) {
    //
    // Get frequency in Timerlib
    //
    mTimestampProperties.Frequency = GetPerformanceCounterProperties (&mTimerLibStartValue, &mTimerLibEndValue);
    mTimestampProperties.EndValue = mTimerLibEndValue - mTimerLibStartValue;

    DEBUG ((DEBUG_INFO, "TimeLibInitialize(): TimerFrequency:0x%llx, TimerLibStartTime:0x%llx, TimerLibEndtime:0x%llx\n", mTimestampProperties.Frequency, mTimerLibStartValue, mTimerLibEndValue));
    initialized = TRUE;
  }
  return EFI_SUCCESS;
}

UINT64
EFIAPI
TimeLibGetTimestamp (
  VOID
  )
{
  //
  // The timestamp of Timestamp Protocol
  //
  UINT64  TimestampValue;

  TimestampValue = 0;

  //
  // Get the timestamp
  //
  if (mTimerLibStartValue > mTimerLibEndValue) {
    TimestampValue = mTimerLibStartValue - GetPerformanceCounter ();
  } else {
    TimestampValue = GetPerformanceCounter () - mTimerLibStartValue;
  }

  return TimestampValue;
}

EFI_STATUS
EFIAPI
TimeLibGetProperties (
  OUT   EFI_TIMESTAMP_PROPERTIES  *Properties
  )
{
  if (Properties == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TimeLibInitializeIfNot ();
  //
  // Get timestamp properties
  //
  CopyMem ((VOID *)Properties, (VOID *)&mTimestampProperties, sizeof (mTimestampProperties));

  return EFI_SUCCESS;
}
