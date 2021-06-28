#ifndef _MSC_VER
#error this is VC-only hack
#endif

#ifdef HW_ST_SEGS_PUSHED
#error can't push the segs twice
#endif

#pragma data_seg( push )
#pragma data_seg( ".hwdata$n" )

#pragma bss_seg( push )
#pragma bss_seg( ".hwbss$n" )

#define HW_ST_SEGS_PUSHED