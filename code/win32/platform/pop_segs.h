#ifndef _MSC_VER
#error this is VC-only hack
#endif

#ifndef HW_ST_SEGS_PUSHED
#error can't pop the segs if they haven't been pushed
#endif

#pragma data_seg( pop )
#pragma bss_seg( pop )

#undef HW_ST_SEGS_PUSHED