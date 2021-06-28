#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../client/client.h"

#include "win_local.h"

#include "platform/pop_segs.h"

#include "resource.h"

void GAME_CALL Host_RecordError( const char *msg )
{

}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	return Game_Main( hInstance, 0, lpCmdLine, nCmdShow );
}