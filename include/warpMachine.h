/*** Last Changed: 2026-05-12 - 11:43 ***/
#ifndef WARP_MACHINE_H
#define WARP_MACHINE_H

#include <ctime>

//--- Get current time with optional warp speed multiplier
//--- Returns time_t representing seconds since epoch
//--- When Warp Speed is enabled, each real second is counted as 60 seconds (1 minute)
//--- When Warp Speed is disabled, returns actual current time
time_t warpMachineNow();

#endif
