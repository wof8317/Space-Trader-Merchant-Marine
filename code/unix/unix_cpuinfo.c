#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#ifdef MACOS_X
void Sys_GetCpuInfo( cpuInfo_t *cpuInfo )
{
	Com_Memset( cpuInfo, 0, sizeof( cpuInfo_t ) );
	
	
}

#else
cpuInfo_t cpuInfo;

void __cpuid(int *regs, int ax)
{
	__asm__ (
		"pushal\n"
		"mov	%0, %%eax\n"
		"cpuid\n"
		
		"movl	%%eax, 0( %1 )\n"
		"movl	%%ebx, 4( %1 )\n"
		"movl	%%ecx, 8( %1 )\n"
		"movl	%%edx, 12( %1 )\n"
		"popal\n"
		: : "a"(ax), "D"( (int*)regs ) : "memory"
	);
}

void Sys_GetCpuInfo( cpuInfo_t *cpuInfo )
{
	byte canCpuid;
	int info[4];

	Com_Memset( cpuInfo, 0, sizeof( cpuInfo_t ) );
	
	//detect whether we can run CUPID
	asm("pushfl\n");
	asm("pushfl\n");
	asm("pop		%eax\n");
	asm("mov		%eax, %ecx\n");
	asm("xor		$0x200000, %eax\n");
	asm("push	%eax\n");
	asm("popfl\n");
	asm("pushfl\n");
	asm("pop		%eax\n");
	asm("popfl\n");
	asm("xor		%ecx, %eax\n");
	asm("setnz	%0\n" : "=r"(canCpuid) : : "memory");

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
			byte tmp[512] __attribute__( ( aligned( 16 ) ) );
			byte *tmp2 = tmp;
			uint mxcsr_mask;

			Com_Memset( tmp, 0, sizeof( tmp ) );

			asm(" fxsave (%0)" : "=a"(tmp2) : : "memory");

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
#endif