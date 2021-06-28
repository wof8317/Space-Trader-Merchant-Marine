#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#include "win_local.h"

static ID_INLINE DWORD TranslateMemType( memType_t type )
{
	switch( type )
	{
	case MT_NORMAL:
		return PAGE_READWRITE;

	case MT_NOACCESS:
		return PAGE_NOACCESS;
		
	case MT_READONLY:
	case MT_READONLY_NTA:
		return PAGE_READONLY;
		
	case MT_WRITEONLY_WRITECOMBINED:
		return PAGE_READWRITE | PAGE_WRITECOMBINE;

	NO_DEFAULT;
	}

	return 0;
}

void* Sys_VmmAlloc( uint cb, memType_t type )
{
	void *ret;

	ret = VirtualAlloc( NULL, cb, MEM_COMMIT, TranslateMemType( type ) );

	return ret;
}

void Sys_VmmFree( void *pMem )
{
	VirtualFree( pMem, 0, MEM_RELEASE );
}

void Sys_VmmChangeType( void *pMem, uint cb, memType_t type )
{
	VirtualProtect( pMem, cb, TranslateMemType( type ), NULL );
}
