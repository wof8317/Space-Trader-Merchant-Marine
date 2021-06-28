#ifndef INC_GAMEHOST_H
#define INC_GAMEHOST_H

typedef enum gameStatus_t
{
	GS_INVALID,		//host error
	GS_STARTING,	//host is firing up

	GS_WAITING,		//waiting for a prior game instance to close
	GS_RUNNING,		//game is up and running
	GS_STOPPED,		//game has terminated normally
	GS_CRASHED,		//game crashed

	GS_ABORTED,		//game was aborted before it got to GS_RUNNING
} gameStatus_t;

typedef struct gameState_t gameState_t;

#ifdef __cplusplus
extern "C"
{
#endif

gameState_t* Host_StartGame( HINSTANCE hostInst, HWND hostWnd, const char *cmdLine );
void Host_StopGame( gameState_t *gs );

gameStatus_t Host_GetGameStatus( gameState_t *gs, char *exitMsg, size_t exitMsg_size );

void Host_ReleaseGame( gameState_t *gs );

void Host_Shutdown( void );

#ifdef __cplusplus
};
#endif

#endif