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

#include "../render-base/tr_local.h"
#include "win_glimp-local.h"

typedef BOOL (WINAPI *IsWow64Process_t)( HANDLE hProcess, PBOOL pIsWow64 );
 
static bool IsWow64( void )
{
	static IsWow64Process_t isWow64Process;
	
	if( !isWow64Process )
		isWow64Process = (IsWow64Process_t)GetProcAddress( GetModuleHandle( "kernel32" ), "IsWow64Process" );
 
    if( isWow64Process )
    {
		BOOL bIsWow64 = FALSE;
        if( !isWow64Process( GetCurrentProcess(), &bIsWow64 ) )
        {
            return false;
        }

		return bIsWow64 != FALSE;
    }

	return false;
}

void Win_PrintOSVersion( void )
{
	OSVERSIONINFO vi;
	char * sys_osstring = "Failed to get OS version info.\n";

	vi.dwOSVersionInfoSize = sizeof( vi );
	if( !GetVersionEx( &vi ) )
	{
		goto done;
	}

	switch( vi.dwPlatformId )
	{
	case VER_PLATFORM_WIN32_NT:
		switch( vi.dwMajorVersion )
		{
		case 4:
			switch( vi.dwMinorVersion )
			{
			case 0:
				sys_osstring = "Windows NT 4.0 ";
				break;

			default:
				sys_osstring = va( "Windows (NT Core) 4.%i ", vi.dwMinorVersion );
				break;
			}
			break;

		case 5:
			switch( vi.dwMinorVersion )
			{
			case 0:
				sys_osstring = "Windows 2000 ";
				break;

			case 1:
				sys_osstring = "Windows XP ";
				break;

			case 2:
				sys_osstring = "Windows XP Pro (x64) or 2003 ";
				break;

			default:
				sys_osstring = va( "Windows (NT Core) 5.%i ", vi.dwMinorVersion );
				break;
			}
			break;

		case 6:
			switch( vi.dwMinorVersion )
			{
			case 0:
				sys_osstring = "Windows Vista ";
				break;

			default:
				sys_osstring = va( "Windows (NT Core) 6.%i ", vi.dwMinorVersion );
				break;
			}
			break;

		default:
			sys_osstring = va( "Windows (NT Core) %i.%i ", vi.dwMajorVersion, vi.dwMinorVersion );
			break;
		}
		break;

	case VER_PLATFORM_WIN32_WINDOWS:
		switch( vi.dwMajorVersion )
		{
		case 4:
			switch( vi.dwMinorVersion )
			{
			case 0:
				sys_osstring = "Windows 95 ";
				break;

			case 10:
				sys_osstring = "Windows 98 ";
				break;

			case 90:
				sys_osstring = "Windows ME ";
				break;

			default:
				sys_osstring = "Windows (Win32 Core) 4.%i ";
				break;
			}
			break;

		default:
			sys_osstring = "Windows (Win32 Core) %i.%i ";
			break;
		}
		break;

	default:
		sys_osstring = "Unknown OS Platform ID (neither NT nor Win32)\n";
		return;
	}

done:
	ri.Printf( PRINT_ALL, "Operating System:\n    %s\n", sys_osstring );

	if( IsWow64() )
		ri.Printf( PRINT_ALL, "    Running on WoW64\n", sys_osstring );

	Cvar_Set( "sys_osstring", sys_osstring );
}

static void PrintRawHexData( const byte *buf, size_t buf_len )
{
	uint i;

	for( i = 0; i < buf_len; i++ )
		ri.Printf( PRINT_ALL, "%02X", (int)buf[i] );
}

static bool PrintCpuInfoFromRegistry( void )
{
	DWORD i, numPrinted;
	HKEY kCpus;

	char name_buf[256];
	DWORD name_buf_len;

	if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\CentralProcessor",
		0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &kCpus ) != ERROR_SUCCESS )
	{
		return false;
	}

	numPrinted = 0;
	for( i = 0; name_buf_len = lengthof( name_buf ),
		RegEnumKeyEx( kCpus, i, name_buf, &name_buf_len,
		NULL, NULL, NULL, NULL ) == ERROR_SUCCESS; i++ )
	{
		HKEY kCpu;
		
		int value_buf_i[256];
		char *value_buf = (char*)value_buf_i;
		DWORD value_buf_len;

		if( RegOpenKeyEx( kCpus, name_buf, 0, KEY_QUERY_VALUE, &kCpu ) != ERROR_SUCCESS )
			continue;

		ri.Printf( PRINT_ALL, "    Processor %i:\n", (int)i );

		value_buf_len = sizeof( value_buf_i );
		if( RegQueryValueEx( kCpu, "ProcessorNameString", NULL, NULL, value_buf, &value_buf_len ) == ERROR_SUCCESS )
		{
			ri.Printf( PRINT_ALL, "        Name: %s\n", value_buf );
		}

		value_buf_len = sizeof( value_buf_i );
		if( RegQueryValueEx( kCpu, "~MHz", NULL, NULL, value_buf, &value_buf_len ) == ERROR_SUCCESS )
		{
			ri.Printf( PRINT_ALL, "        Speed: %i MHz\n", (int)*(DWORD*)value_buf_i );
		}

		value_buf_len = sizeof( value_buf_i );
		if( RegQueryValueEx( kCpu, "VendorIdentifier", NULL, NULL, value_buf, &value_buf_len ) == ERROR_SUCCESS )
		{
			ri.Printf( PRINT_ALL, "        Vendor: %s\n", value_buf );
		}

		value_buf_len = sizeof( value_buf_i );
		if( RegQueryValueEx( kCpu, "Identifier", NULL, NULL, value_buf, &value_buf_len ) == ERROR_SUCCESS )
		{
			ri.Printf( PRINT_ALL, "        Identifier: %s\n", value_buf );
		}

		value_buf_len = sizeof( value_buf_i );
		if( RegQueryValueEx( kCpu, "FeatureSet", NULL, NULL, value_buf, &value_buf_len ) == ERROR_SUCCESS )
		{
			ri.Printf( PRINT_ALL, "        Feature Bits: %08X\n", (int)*(DWORD*)value_buf_i );
		}

		RegCloseKey( kCpu );

		numPrinted++;
	}

	RegCloseKey( kCpus );

	return numPrinted > 0;
}

void Win_PrintCpuInfo( void )
{
	SYSTEM_INFO si;

	GetSystemInfo( &si );

	if( si.dwNumberOfProcessors == 1 )
	{
		ri.Printf( PRINT_ALL, "Processor:\n" );
	}
	else
	{
		ri.Printf( PRINT_ALL, "Processors (%i):\n", (int)si.dwNumberOfProcessors );
	}

	ri.Printf( PRINT_ALL, "    Detected Caps: " );
	if( cpuInfo.fxsr )
		ri.Printf( PRINT_ALL, "FXSR " );
	if( cpuInfo.mmx.enabled )
		ri.Printf( PRINT_ALL, "MMX " );
	if( cpuInfo.sse.enabled )
	{
		ri.Printf( PRINT_ALL, "SSE " );

		ri.Printf( PRINT_ALL, "MXCSR:0x%X ", cpuInfo.sse.mxcsr_mask );
		if( cpuInfo.sse.hasDAZ )
			ri.Printf( PRINT_ALL, "DAZ " );
	}
	if( cpuInfo.sse2.enabled )
		ri.Printf( PRINT_ALL, "SSE2 " );
	ri.Printf( PRINT_ALL, "\n" );

	if( PrintCpuInfoFromRegistry() )
		return;

	ri.Printf( PRINT_ALL, "        Architecture: " );

	switch( si.wProcessorArchitecture )
	{
	case PROCESSOR_ARCHITECTURE_INTEL:
		ri.Printf( PRINT_ALL, "x86" );
		break;

	case PROCESSOR_ARCHITECTURE_MIPS:
		ri.Printf( PRINT_ALL, "MIPS" );
		break;

	case PROCESSOR_ARCHITECTURE_ALPHA:
		ri.Printf( PRINT_ALL, "ALPHA" );
		break;

	case PROCESSOR_ARCHITECTURE_PPC:
		ri.Printf( PRINT_ALL, "PPC" );
		break;

	case PROCESSOR_ARCHITECTURE_SHX:
		ri.Printf( PRINT_ALL, "SHX" );
		break;

	case PROCESSOR_ARCHITECTURE_ARM:
		ri.Printf( PRINT_ALL, "ARM" );
		break;

	case PROCESSOR_ARCHITECTURE_IA64:
		ri.Printf( PRINT_ALL, "IA64" );
		break;

	case PROCESSOR_ARCHITECTURE_ALPHA64:
		ri.Printf( PRINT_ALL, "ALPHA64" );
		break;

	case PROCESSOR_ARCHITECTURE_MSIL:
		ri.Printf( PRINT_ALL, "MSIL" );
		break;

	case PROCESSOR_ARCHITECTURE_AMD64:
		ri.Printf( PRINT_ALL, "x64" );
		break;

	case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
		ri.Printf( PRINT_ALL, "WoW64" );
		break;

	default:
		ri.Printf( PRINT_ALL, "UNKNOWN: %i", (int)si.wProcessorArchitecture );
		break;
	}

	ri.Printf( PRINT_ALL, "\n" );

	ri.Printf( PRINT_ALL, "        Revision: %04X\n", (int)si.wProcessorRevision );
}

void Win_PrintMemoryInfo( void )
{
	SYSTEM_INFO si;
	MEMORYSTATUS ms;

	GetSystemInfo( &si );
	GlobalMemoryStatus( &ms );

	ri.Printf( PRINT_ALL, "Memory:\n" );
	ri.Printf( PRINT_ALL, "    Total Physical: %i MB\n", ms.dwTotalPhys / 1024 / 1024 );
	ri.Printf( PRINT_ALL, "    Total Page File: %i MB\n", ms.dwTotalPageFile / 1024 / 1024 );
	ri.Printf( PRINT_ALL, "    Load: %i%%\n", ms.dwMemoryLoad );
	ri.Printf( PRINT_ALL, "    Page Size: %i K\n", si.dwPageSize / 1024 );
}