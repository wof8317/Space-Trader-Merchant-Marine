#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

void DRM_Init(){
}

static int QDECL DRM_Register_response( httpInfo_e code, const char * buffer, int length, void * notifyData ) {

	if ( code == HTTP_WRITE ) {

		fileHandle_t	f;
		char * code		= COM_Parse( &buffer );
		char * sauce	= COM_Parse( &buffer );
		char * msg		= COM_Parse( &buffer );

		if ( !Q_stricmp( code, "1" ) ) {

			Cvar_Set( "com_keyvalid", "1" );
			Cvar_Set( "com_registermessage", "4" );
		} else {

			Cvar_Set( "com_keyvalid", "0" );
			Cvar_Set( "com_registermessage", "5" );
			Cvar_Set( "ui_registermessage", msg );
		}

		f = FS_FOpenFileWrite( "~/stkey" );

		if( f )
		{
			FS_Write( sauce, strlen(sauce), f );
			FS_FCloseFile( f );
		}
		else
		{
			Com_Printf( S_COLOR_YELLOW "Failed to write 'stkey' file.\n" );
		}
	}

	return length;
}

int DRM_Register(char *key) {
	
	char  server[ 64 ];

	HTTP_PostUrl( va("http://%s/game/register", "www.playspacetrader.com"), DRM_Register_response, 0, "game[key]=%s&hash=%s", key, Sys_SecretSauce( server, sizeof(server) ) );

	return 0;
}

int DRM_Validate() {

	char server[ 64 ];
	char client[ 64 ];
	fileHandle_t f;

#ifdef USE_ALWAYS_VALID
	return 4;
#endif
	//
	//	read what the game thinks it is
	//
	if ( FS_FOpenFileRead( "stkey", &f, qfalse ) == -1 ) {
		return 0;
	}

	FS_Read( client, sizeof(client), f );
	FS_FCloseFile( f );


	//
	//	read the actual machine
	//
	Sys_SecretSauce( server, sizeof(server) );

	return memcmp( client, server, 32 )?0:4;
}

void DRM_Kill(){
}

void DRM_Cancel() {

	fileHandle_t	f;
	char client[ 64 ];
	Com_Memset( client, 0 , sizeof(client) );
	
	f = FS_FOpenFileWrite( "~/stkey" );
	FS_Write( client, strlen(client), f );
	FS_FCloseFile( f );
}
