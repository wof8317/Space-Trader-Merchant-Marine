#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#include "platform/pop_segs.h"
#include <intrin.h>
#include "platform/push_segs.h"

void Sys_GetCpuInfo( cpuInfo_t *cpuInfo )
{
	byte canCpuid;
	int info[4];

	Com_Memset( cpuInfo, 0, sizeof( cpuInfo_t ) );
	
	//detect whether we can run CUPID
	__asm
	{
		pushfd
		pushfd
		pop			eax
		mov			ecx, eax
		xor			eax, 0x200000
		push		eax
		popfd
		pushfd
		pop			eax
		popfd
		xor			eax, ecx
		setnz		canCpuid
	}

	if( !canCpuid )
		//can't get more info, leave everything disabled
		return;

	__cpuid( info, 1 );

	if( info[3] & (1 << 23) )
		cpuInfo->mmx.enabled = qtrue;

	if( info[3] & (1 << 24) )
		cpuInfo->fxsr = qtrue;

	if( info[3] & (1 << 25) )
	{
		cpuInfo->sse.enabled = qtrue;

		//find out if we can use DAZ
		if( cpuInfo->fxsr )
		{
			uint mxcsr_mask;

			__declspec( align( 16 ) ) byte tmp[512];
			Com_Memset( tmp, 0, sizeof( tmp ) );
						
			__asm { fxsave tmp }

			mxcsr_mask = *(uint*)(tmp + 28);
			if( !mxcsr_mask )
				mxcsr_mask = 0xFFBF;

			cpuInfo->sse.mxcsr_mask = mxcsr_mask;
			cpuInfo->sse.hasDAZ = (mxcsr_mask & (1 << 6)) ? qtrue : qfalse;
		}
	}

	if( info[3] & (1 << 26) )
		cpuInfo->sse2.enabled = qtrue;
}