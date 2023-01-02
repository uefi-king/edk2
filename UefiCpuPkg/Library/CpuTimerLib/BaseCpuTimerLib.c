/** @file
  CPUID Leaf 0x15 for Core Crystal Clock frequency instance as Base Timer Library.

  Copyright (c) 2019 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>

/**
  CPUID Leaf 0x15 for Core Crystal Clock Frequency.

  The TSC counting frequency is determined by using CPUID leaf 0x15. Frequency in MHz = Core XTAL frequency * EBX/EAX.
  In newer flavors of the CPU, core xtal frequency is returned in ECX or 0 if not supported.
  @return The number of TSC counts per second.

**/
UINT64
CpuidCoreClockCalculateTscFrequency (
  VOID
  );

/**
  Internal function to retrieves the 64-bit frequency in Hz.

  Internal function to retrieves the 64-bit frequency in Hz.

  @return The frequency in Hz.

**/
UINT64
InternalGetPerformanceCounterFrequency (
  VOID
  )
{
  UINT32 RegEax;
  UINT32 RegEcx;

  // after cpuid[%eax=0x1], the most significant bit of %ecx indicates whether we are in a VM
  AsmCpuid (0x1, NULL, NULL, &RegEcx, NULL);

  if (RegEcx & (1 << 31)) {
    // now we are in a VM
    // cpuid[%eax=0x15] is disabled in Qemu
    // cpuid[%eax=0x40000010] returns in %eax the CPU frequency in KHz
    //   but needs be supported in Qemu by adding -cpu flags: +invtsc,+vmware-cpuid-freq
    AsmCpuid (0x40000010, &RegEax, NULL, NULL, NULL);
    return MultU64x32 (1000, RegEax);
  }
  // we are not in a VM
  return CpuidCoreClockCalculateTscFrequency ();
}
