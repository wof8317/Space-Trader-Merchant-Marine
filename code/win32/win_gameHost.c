#if _MSC_VER >= 1400
#define _CRT_SECURE_NO_DEPRECATE
#endif

#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <process.h>

#include "win_gameHost.h"

typedef struct gameState_t
{
	volatile LONG			freeFlag;			//host and game - Interlocked access
	volatile LONG			status;				//host and game - Interlocked access

	DWORD					threadId;			//host setup, then host xor game - protected by login on freeFlag
	HANDLE					threadHandle;		//host setup, then host xor game - protected by login on freeFlag

	HINSTANCE				hostInst;
	HWND					hostWnd;
	char					cmdLine[2048];
	char					retStr[2048];
} gameState_t;

static int host_initialized;

#define GAME_THREAD_SLOTS 2
static gameState_t gameStates[GAME_THREAD_SLOTS];

static CRITICAL_SECTION oneToRuleThemAll;

void Host_Init( void )
{
	InitializeCriticalSection( &oneToRuleThemAll );
}

void Host_Kill( void )
{
	DeleteCriticalSection( &oneToRuleThemAll );
	host_initialized = 0;
}

/**************************************
	Game thread launch/run code:
*/

#include "../win32/win_host.h"

void GAME_CALL Host_RecordError( const char *msg )
{
	msg;
}

static int CrashFilter( DWORD excCode, PEXCEPTION_POINTERS pExcPtrs )
{	
	excCode; pExcPtrs;

	if( IsDebuggerPresent() )
	{
		if( MessageBoxA( NULL, "An exception has occurred: press Yes to debug, No to shut down.",
			"Error", MB_ICONERROR | MB_YESNO | MB_DEFBUTTON1 ) == IDYES )
			return EXCEPTION_CONTINUE_SEARCH; 
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

static unsigned RunGame( HINSTANCE hInst, HWND hHostWnd, LPSTR cmdLine )
{
	__try
	{
		return Game_Main( hInst, hHostWnd, cmdLine, 0 );
	}
	__except( CrashFilter( GetExceptionCode(), GetExceptionInformation() ) )
	{
		//eat the crash so as not to kill the browser
		return (DWORD)-1;
	}
}

static unsigned WINAPI GameProc( void *lpParameter )
{	
	gameState_t *gs = (gameState_t*)lpParameter;
	DWORD retcode;

	if( InterlockedCompareExchange( &gs->status, (LONG)GS_WAITING, (LONG)GS_STARTING ) != GS_STARTING )
	{
		/*
			This game instance was aborted before we got here.
		*/

		return (DWORD)-2;
	}

	/*
		One Thread to Rule Them All Rule:

		Since the game procedure isn't thread safe we have
		to limit it to one instance at a time. We *could* do
		this with a critical section...and in fact we will.

		The thread can sit here for a *long* time before whichever
		other thread is running releases the lock. Darn.

		Awful, ain't it?
	*/

	EnterCriticalSection( &oneToRuleThemAll );

	if( InterlockedCompareExchange( &gs->status, (LONG)GS_RUNNING, (LONG)GS_WAITING ) != (LONG)GS_WAITING )
	{
		/*
			The thread was released before we got the critical section
			(caller probably got sick of waiting for us). Just return.
		*/

		LeaveCriticalSection( &oneToRuleThemAll );
		return (DWORD)-2;
	}

	retcode = RunGame( gs->hostInst, gs->hostWnd, gs->cmdLine );

	if( retcode == 0 )
	{
		InterlockedCompareExchange( &gs->status, (LONG)GS_STOPPED, (LONG)GS_RUNNING );
	}
	else
	{
		InterlockedCompareExchange( &gs->status, (LONG)GS_CRASHED, (LONG)GS_RUNNING );
		Game_GetExitState( NULL, gs->retStr, sizeof( gs->retStr ) );
	}

	LeaveCriticalSection( &oneToRuleThemAll );

	if( InterlockedDecrement( &gs->freeFlag ) >= 0 )
	{
		/*
			The host has released us before we got here,
			mark the gs slot as free and return. The thread
			handle will be closed next time the startup
			routine wants to use the gs slot.
		*/
		
		memset( gs, 0, sizeof( gameState_t ) );
	}
	/*
		The else is that the host isn't done with the gs slot
		yet, and we should leave it open. Note that we've now
		set the flag to one so when the host decides to release
		the gs it will know to clean stuff up.
	*/

	return retcode;
}

static BOOL StartGameThread( gameState_t *gs )
{
	HANDLE handle;
	unsigned id;

	handle = (HANDLE)_beginthreadex( NULL, 0, GameProc, gs, 0, &id );

	if( !handle )
		return FALSE;

	gs->threadHandle = handle;
	gs->threadId = id;

	return TRUE;
}

/**************************************
	Public interface functions:
*/

gameState_t* Host_StartGame( HINSTANCE hostInst, HWND hostWnd, const char *cmdLine )
{
	int i;
	gameState_t *gs = NULL;

	if( !host_initialized )
	{
		Host_Init();
		host_initialized = 1;
	}

	for( i = 0; i < GAME_THREAD_SLOTS; i++ )
	{
		gameStatus_t stat = gameStates[i].status;
		
		switch( stat )
		{
		case GS_INVALID:
			gs = gameStates + i;
			break;

			/********************************
				The one-instance rule strikes again...
				
				If we're abiding by the one-instance only rule (and until we clean up the
				game globals we have to) then we can, probably, just stop looking once we
				find a running instance.

				The only caveat to this is that if we happen to test the state just as the
				instance is shutting down (maybe it just got the quit message) we'll fail
				where we could have brought the new instance up and had it kick in just
				as the other one goes down.

				So, do we do this or no? Decisions, decisions...

		case GS_RUNNING:
			return NULL;
			*/
		}

		if( gs )
			break;
	}

	if( !gs )
		return NULL;

	if( gs->threadHandle )
		//see comments in Host_ReleaseGame
		CloseHandle( gs->threadHandle );

	memset( gs, 0, sizeof( gameState_t ) );

	gs->status = GS_STARTING;

	gs->hostInst = hostInst;
	gs->hostWnd = hostWnd;
	strncpy( gs->cmdLine, cmdLine, sizeof( gs->cmdLine ) );

	if( !StartGameThread( gs ) )
	{
		gs->status = GS_INVALID;
		return NULL;
	}

	return gs;
}

gameStatus_t Host_GetGameStatus( gameState_t *gs, char *exitMsg, size_t exitMsg_size )
{
	gameStatus_t stat = gs->status;

	if( exitMsg )
	{
		if( stat == GS_CRASHED )
			strncpy( exitMsg, gs->retStr, exitMsg_size );
		else if( exitMsg_size >= 1 )
			exitMsg[0] = 0;
	}

	return stat;
}

void Host_StopGame( gameState_t *gs )
{
	if( InterlockedCompareExchange( &gs->status, (LONG)GS_ABORTED, (LONG)GS_STARTING ) == (LONG)GS_STARTING )
		return;

	if( InterlockedCompareExchange( &gs->status, (LONG)GS_ABORTED, (LONG)GS_WAITING ) == (LONG)GS_WAITING )
		return;

	PostThreadMessage( gs->threadId, WM_QUIT, 0, 0 );
}

void Host_ReleaseGame( gameState_t *gs )
{
	if( InterlockedIncrement( &gs->freeFlag ) <= 0 )
	{
		/*
			Thread is essentially done at this point. All
			that's left in the thread proc is a return so
			really all that's left is wihndows tearing its
			internal data structures down.
		*/
		CloseHandle( gs->threadHandle );
		
		memset( gs, 0, sizeof( gameState_t ) );
	}
	/*
		The else is that the game thread is still running, in which
		case InterlockedCompareExchange will have stored a 1 in freeFlag
		which will cause the game thread to clean its data up on exit
		and the thread handle will be closed on the next start call
		(remember - the handle is really only valid in *this* thread).
	*/
} 

static void SpinWaitForThread( HANDLE hThread )
{
	DWORD waitCode;

	/*
		Spinnity spin spin...

		We *absolutely must* keep the message loop moving here. If we
		don't we blow up when the child window goes down as it will block
		waiting for this thread to process window messages.

		Wheeeee....
	*/

	waitCode = WaitForSingleObject( hThread, 10 );
	while( waitCode == WAIT_TIMEOUT )						
	{
		MSG msg;

		while( PeekMessage( &msg, 0, 0, 0, PM_NOREMOVE ) )
		{
			if( GetMessage( &msg, 0, 0, 0 ) != -1 )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			else
				break;
		}

		waitCode = WaitForSingleObject( hThread, 0 );
	}
}

/*
	Tear down all active games - don't return until they're all down.
*/
void Host_Shutdown( void )
{
	int i;
	BOOL anyAlive;

	anyAlive = FALSE;
	for( i = 0; i < GAME_THREAD_SLOTS; i++ )
	{
		gameState_t *gs = gameStates + i;

		if( gs->status != GS_INVALID )
		{
			Host_StopGame( gs );
			Host_ReleaseGame( gs );

			anyAlive = TRUE;
		}
	}

	if( anyAlive )
	{
		for( i = 0; i < GAME_THREAD_SLOTS; i++ )
		{
			gameState_t *gs = gameStates + i;

			if( gs->status != GS_INVALID && gs->threadHandle )
			{
				SpinWaitForThread( gs->threadHandle );						
				CloseHandle( gs->threadHandle );
			}
		}
	}

	Host_Kill();
}