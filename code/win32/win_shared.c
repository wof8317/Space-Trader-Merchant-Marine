/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2007 HermitWorks Entertainment Corporation

This file is part of the Space Trader source code.

The Space Trader source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

The Space Trader source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with the Space Trader source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"

#include "platform/pop_segs.h"
#include <shlobj.h>
#include "platform/push_segs.h"

/*
================
Sys_Milliseconds
================
*/
int			sys_timeBase;
int Sys_Milliseconds (void)
{
	int			sys_curtime;
	static qboolean	initialized = qfalse;

	if (!initialized) {
		sys_timeBase = timeGetTime();
		initialized = qtrue;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

#ifndef __GNUC__ //see snapvectora.s
/*
================
Sys_SnapVector
================
*/
void Sys_SnapVector( float *v )
{
	int i;
	float f;

	f = *v;
	__asm
	{
		fld		f;
		fistp	i;
	}
	*v = i;

	v++;
	
	f = *v;
	__asm
	{	
		fld		f;
		fistp	i;
	}
	*v = i;

	v++;

	f = *v;
	__asm
	{
		fld		f;
		fistp	i;
	}
	*v = i;
}
#endif

static int __stdcall IsPentium( void )
{
	int retVal;

	__asm 
	{
		pushfd						// save eflags
		pop		eax
		test	eax, 0x00200000		// check ID bit
		jz		set21				// bit 21 is not set, so jump to set_21
		and		eax, 0xffdfffff		// clear bit 21
		push	eax					// save new value in register
		popfd						// store new value in flags
		pushfd
		pop		eax
		test	eax, 0x00200000		// check ID bit
		jz		good
		jmp		err					// cpuid not supported
set21:
		or		eax, 0x00200000		// set ID bit
		push	eax					// store new value
		popfd						// store new value in EFLAGS
		pushfd
		pop		eax
		test	eax, 0x00200000		// if bit 21 is on
		jnz		good
err:
		mov		retVal, 0
		jmp		end
good:
		mov		retVal, 1
end:
	}

	return retVal;
}

static int Is3DNOW( void )
{
	int regs[4];
	char pstring[16];
	char processorString[13];

	// get name of processor
	__cpuid( (int*)pstring, 0 );
	processorString[0] = pstring[4];
	processorString[1] = pstring[5];
	processorString[2] = pstring[6];
	processorString[3] = pstring[7];
	processorString[4] = pstring[12];
	processorString[5] = pstring[13];
	processorString[6] = pstring[14];
	processorString[7] = pstring[15];
	processorString[8] = pstring[8];
	processorString[9] = pstring[9];
	processorString[10] = pstring[10];
	processorString[11] = pstring[11];
	processorString[12] = 0;

//  REMOVED because you can have 3DNow! on non-AMD systems
//	if ( strcmp( processorString, "AuthenticAMD" ) )
//		return qfalse;

	// check AMD-specific functions
	__cpuid( regs, 0x80000000 );
	if ( regs[0] < 0x80000000 )
		return qfalse;

	// bit 31 of EDX denotes 3DNOW! support
	__cpuid( regs, 0x80000001 );
	if ( regs[3] & ( 1 << 31 ) )
		return qtrue;

	return qfalse;
}

static int IsKNI( void )
{
	unsigned regs[4];

	// get CPU feature bits
	__cpuid( regs, 1 );

	// bit 25 of EDX denotes KNI existence
	if ( regs[3] & ( 1 << 25 ) )
		return qtrue;

	return qfalse;
}

static int IsMMX( void )
{
	unsigned regs[4];

	// get CPU feature bits
	__cpuid( regs, 1 );

	// bit 23 of EDX denotes MMX existence
	if ( regs[3] & ( 1 << 23 ) )
		return qtrue;
	return qfalse;
}

int Sys_GetProcessorId( void )
{
#if defined _M_ALPHA
	return CPUID_AXP;
#elif !defined _M_IX86
	return CPUID_GENERIC;
#else

	// verify we're at least a Pentium or 486 w/ CPUID support
	if ( !IsPentium() )
		return CPUID_INTEL_UNSUPPORTED;

	// check for MMX
	if ( !IsMMX() )
	{
		// Pentium or PPro
		return CPUID_INTEL_PENTIUM;
	}

	// see if we're an AMD 3DNOW! processor
	if ( Is3DNOW() )
	{
		return CPUID_AMD_3DNOW;
	}

	// see if we're an Intel Katmai
	if ( IsKNI() )
	{
		return CPUID_INTEL_KATMAI;
	}

	// by default we're functionally a vanilla Pentium/MMX or P2/MMX
	return CPUID_INTEL_MMX;

#endif
}

//============================================

char *Sys_GetCurrentUser( char * buffer, int size )
{
	if ( !GetUserName( buffer, &size ) ) {
		Q_strncpyz( buffer, "player", size );
	}

	return buffer;
}

char * Sys_DefaultHomePath( char * buffer, int size )
{
	if( SHGetSpecialFolderPath( NULL, buffer, CSIDL_PERSONAL, TRUE ) != NOERROR )
	{
		Q_strcat( buffer, size, "\\st" );
	} else
	{
		Com_Error( ERR_FATAL, "couldn't find home path.\n" );
		buffer[0] = 0;
	}

	return buffer;
}

char *Sys_DefaultInstallPath( char * buffer, int size )
{
#ifdef USE_BOOTWITHNOFILES
	if( SHGetSpecialFolderPath( NULL, buffer, CSIDL_COMMON_APPDATA, TRUE ) != NOERROR )
	{
		Q_strcat( buffer, size, "\\HermitWorks\\SpaceTrader" );
		return buffer;
	}

	return Sys_Cwd( buffer, size );
#else
	return Sys_Cwd( buffer, size );
#endif
}

#if 0
typedef struct macAddr_t
{
	UINT	len;
	BYTE	addr[MAX_ADAPTER_ADDRESS_LENGTH];
} macAddr_t;

static int QDECL MacAddr_Cmp( const void *a, const void *b )
{
	int i, d;
	const macAddr_t *ma = a;
	const macAddr_t *mb = b;

	d = ma->len - mb->len;

	if( d )
		return d;

	for( i = 0; i < ma->len; i++ )
	{
		d = ma->addr[i] - mb->addr[i];
		if( d )
			return d;
	}

	return 0;
}
#endif

char * Sys_SecretSauce( char * buffer, int size )
{
	int len, bufLen;
	char *buf;

	bufLen = 4096;
	buf = Hunk_AllocateTempMemory( bufLen );

	len = 0;

	if( bufLen - len >= sizeof( OSVERSIONINFO ) )
	{
		OSVERSIONINFO vinfo;

		Com_Memset( &vinfo, 0, sizeof( vinfo ) ); //some bytes aren't written by the GetVersionEx
		vinfo.dwOSVersionInfoSize = sizeof( vinfo );

		GetVersionEx( &vinfo );

		Com_Memcpy( buf + len, &vinfo, sizeof( vinfo ) );
		len += sizeof( vinfo );
	}

#if 0		// don't rely on mac address, if user disconects or disables then game becomes unregistered
	{
		DWORD cb, err;
		IP_ADAPTER_INFO *pInfos;

		cb = sizeof( IP_ADAPTER_INFO ) * 4;
		pInfos = Hunk_AllocateTempMemory( cb );

		err = GetAdaptersInfo( pInfos, &cb );
		if( err == ERROR_BUFFER_OVERFLOW )
		{
			Hunk_FreeTempMemory( pInfos );
			pInfos = Hunk_AllocateTempMemory( cb );

			err = GetAdaptersInfo( pInfos, &cb );
		}

		if( err == ERROR_SUCCESS )
		{
			int i;
			int numAddrs = 0;
			int maxAddrs = cb / sizeof( IP_ADAPTER_INFO );
			macAddr_t *addrs = Hunk_AllocateTempMemory( sizeof( macAddr_t ) * maxAddrs );

			IP_ADAPTER_INFO *p;
			for( p = pInfos; p; p = p->Next )
			{
				if( p->Type != MIB_IF_TYPE_ETHERNET )
					continue;

				if( numAddrs == maxAddrs )
					break;

				addrs[numAddrs].len = p->AddressLength;
				Com_Memcpy( &addrs[numAddrs].addr, p->Address, p->AddressLength );
				numAddrs++;
			}

			qsort( addrs, numAddrs, sizeof( macAddr_t ), MacAddr_Cmp );

			for( i = 0; i < numAddrs; i++ )
			{
				if( addrs[i].len > bufLen - len )
					break;

				Com_Memcpy( buf + len, addrs[i].addr, addrs[i].len );
				len += addrs[i].len;
			}

			Hunk_FreeTempMemory( addrs );
		}

		Hunk_FreeTempMemory( pInfos );
	}
#endif

	if( sizeof( cpuInfo_t ) <= bufLen - len )
	{
		cpuInfo_t cpuInfo;
		Sys_GetCpuInfo( &cpuInfo );

		Com_Memcpy( buf + len, &cpuInfo, sizeof( cpuInfo ) );
		len += sizeof( cpuInfo );
	}

	//MD5 the blob up
	Com_MD5Buffer( buffer, buf, len );
	Hunk_FreeTempMemory( buf );

	return buffer;
}
