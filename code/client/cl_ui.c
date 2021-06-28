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

#include "client.h"

#include "../botlib/botlib.h"

extern	botlib_export_t	*botlib_export;

extern sqlInfo_t com_db;

vm_t *uivm;

/*
====================
GetClientState
====================
*/
static void GetClientState( uiClientState_t *state ) {
	state->connectPacketCount = clc.connectPacketCount;
	state->connState = cls.state;
	Q_strncpyz( state->servername, cls.servername, sizeof( state->servername ) );
	Q_strncpyz( state->updateInfoString, cls.updateInfoString, sizeof( state->updateInfoString ) );
	Q_strncpyz( state->messageString, clc.serverMessage, sizeof( state->messageString ) );
	state->clientNum = cl.snap.ps.clientNum;
}

/*
=======================
LAN_UpdateVisiblePings
=======================
*/
qboolean LAN_UpdateVisiblePings(int source ) {
	return CL_UpdateVisiblePings_f(source);
}

/*
====================
LAN_GetServerStatus
====================
*/
int LAN_GetServerStatus( char *serverAddress, char *serverStatus, int maxLen ) {
	return CL_ServerStatus( serverAddress, serverStatus, maxLen );
}

/*
====================
CL_GetGlConfig
====================
*/
static void CL_GetGlconfig( vidConfig_t *config ) {
	*config = cls.glconfig;
}

/*
====================
QGetClipboardData
====================
*/
static void QGetClipboardData( char *buf, int buflen ) {
	char	*cbd;

	cbd = Sys_GetClipboardData();

	if ( !cbd ) {
		*buf = 0;
		return;
	}

	Q_strncpyz( buf, cbd, buflen );

	Z_Free( cbd );
}

/*
====================
Key_KeynumToStringBuf
====================
*/
static void Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	Q_strncpyz( buf, Key_KeynumToString( keynum ), buflen );
}

/*
====================
Key_GetBindingBuf
====================
*/
static void Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	char	*value;

	value = Key_GetBinding( keynum );
	if ( value ) {
		Q_strncpyz( buf, value, buflen );
	}
	else {
		*buf = 0;
	}
}

/*
====================
Key_GetCatcher
====================
*/
int Key_GetCatcher( void ) {
	return cls.keyCatchers;
}

/*
====================
Ket_SetCatcher
====================
*/
void Key_SetCatcher( int catcher ) {
	cls.keyCatchers = catcher;
}


/*
====================
GetConfigString
====================
*/
static int GetConfigString(int index, char *buf, int size)
{
	int		offset;

	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		return qfalse;

	offset = cl.gameState.stringOffsets[index];
	if (!offset) {
		if( size ) {
			buf[0] = 0;
		}
		return qfalse;
	}

	Q_strncpyz( buf, cl.gameState.stringData+offset, size);
 
	return qtrue;
}

/*
====================
FloatAsInt
====================
*/
static int FloatAsInt( float f ) {
	fi_t t;
	t.f = f;
	return t.i;
}

int CL_UpdateGameState( globalState_t * gs )
{
	if ( !cl.snap.valid )
		return 0;

	// copy game state to vm
	Com_Memcpy( gs, &cl.snap.ps.gs, sizeof(globalState_t) );
	return 1;
}

/*
====================
CL_GetRadar
====================
*/
int CL_GetRadar( radarInfo_t *out_ents, int maxEnts, const radarFilter_t *filters, int numFilters, float range )
{
	int i, c;
	int count;
	clSnapshot_t *clSnap;
	float r2, invR, theta;

	//if the frame is not valid, we can't return it
	clSnap = &cl.snapshots[cl.snap.messageNum & PACKET_MASK];
	if( !clSnap->valid )
		return 0;

	//if the entities in the frame have fallen out of their
	//circular buffer, we can't return it
	if( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES )
		return 0;

	count = clSnap->numEntities;
	
	r2 = range * range;
	invR = 1.0F / range;

	theta = -DEG2RAD( cl.snap.ps.viewangles[YAW] + 90 );

	c = 0;
	for( i = 0; i < count; i++ )
	{
		int j;

		entityState_t * e = cl.parseEntities + ((clSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1));
		
		float d;
		float x = 0;
		float y = 0;

		for( j = 0; j < numFilters; j++ )
		{
			if( filters[j].type == e->eType )
			{
				float *pos = (float*)((byte*)e + filters[j].offset_to_pos);
				x = pos[0];
				y = pos[1];

				out_ents[c].ent = *e;

				break;
			}
		}

		if( j == numFilters )
			continue;

		x = x - cl.snap.ps.origin[0];
		y = y - cl.snap.ps.origin[1];

		d = x * x + y * y;

		if( d > r2 )
			continue;

		d = sqrtf( d );

		out_ents[c].r = d * invR;
		out_ents[c].theta = theta - atan2f( x, y );

		c++;

		if( c == maxEnts )
			break;
	}

	return c;
}

void CL_DownloadProgress( void ) {

	float const w = 4.0f;
	float const h = 6.0f;
	float const s = 2.0f;
	vec4_t color = { 0.8f, 0.8f, 0.8f, 0.8f };
	float rect[4];
	int	sofar,total;
	int i;
	float x;

	sql_prepare( &com_db, "SELECT SUM(xfer), SUM(length) FROM openfiles;" );

	if ( sql_step( &com_db ) ) {

		sofar = sql_columnasint( &com_db, 0 );
		total = sql_columnasint( &com_db, 1 );

		if ( sofar != total && total > 0 && sofar < total ) {

			rect[ 0 ] = Cvar_VariableValue( "com_downloadbar_x" );
			rect[ 1 ] = Cvar_VariableValue( "com_downloadbar_y" );
			rect[ 2 ] = Cvar_VariableValue( "com_downloadbar_w" );
			rect[ 3 ] = Cvar_VariableValue( "com_downloadbar_h" );

			if ( rect[ 2 ] > 0.0f && rect[ 3 ] > 0.0f ) {
				char * rate = Cvar_VariableString( "com_downloaded_rate" );
				int		n = (int)(rect[2] / (w+s));
				float	p = (float)sofar / (float)total;
				float	r = p * n;
			
				int		c = (int)floorf( r );

				r = r-c;

				if ( cls.font ) {
					CL_RenderText( rect, 0.3f, colorWhite, va("%d%%... %s kb/s", (int)(p*100), rate ), -1, TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER, TEXT_STYLE_SHADOWED, -1, 0, cls.font, 0 );
				}

				for ( i=0; i<c; i++ ) {
					x = ((float)i)*(w+s);
					SCR_FillSquareRect( rect[0] + x, rect[1]+rect[3]-h, w, h, color );
				}

				x = ((float)i)*(w+s);
				color[3] *= r;
				SCR_FillSquareRect( rect[0] + x, rect[1]+rect[3]-h, w, h, color );
			}
		}
	}
	sql_done( &com_db );
}


static void UI_Con_GetText( char *buf, int size, int c )
{
	size_t len, cp, ofs;
	const char *conText;
	
	conText = Con_GetText( c );
	len = strlen( conText );

	if( len > size )
	{
		cp = size - 1;
		ofs = len - cp;
	}
	else
	{
		cp = len;
		ofs = 0;
	}

	memcpy( buf, conText + ofs, cp );
	buf[len] = 0;
}

extern int select_from_server( const char * src, char * segment, char * buffer, int size );

/*
====================
CL_UISystemCalls

The ui module is making a system call
====================
*/
intptr_t CL_UISystemCalls( intptr_t *args ) {
	switch( args[0] ) {
	case UI_ERROR:
		Com_Error( args[1], "%s", VMA(2) );
		return 0;

	case UI_PRINT:
		Com_Printf( "%s", VMA(1) );
		return 0;

	case UI_MILLISECONDS:
		return Sys_Milliseconds();

	case UI_CVAR_REGISTER:
		Cvar_Register( VMA(1), VMA(2), VMA(3), args[4] ); 
		return 0;

	case UI_CVAR_UPDATE:
		Cvar_Update( VMA(1) );
		return 0;

	case UI_CVAR_SET:
		Cvar_Set( VMA(1), VMA(2) );
		return 0;

	case UI_CVAR_VARIABLEVALUE:
		return FloatAsInt( Cvar_VariableValue( VMA(1) ) );

	case UI_CVAR_VARIABLEINT:
		return Cvar_VariableIntegerValue( VMA(1) );

	case UI_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer( VMA(1), VMA(2), args[3] );
		return 0;

	case UI_CVAR_SETVALUE:
		Cvar_SetValue( VMA(1), VMF(2) );
		return 0;

	case UI_CVAR_RESET:
		Cvar_Reset( VMA(1) );
		return 0;

	case UI_CVAR_CREATE:
		Cvar_Get( VMA(1), VMA(2), args[3] );
		return 0;

	case UI_CVAR_INFOSTRINGBUFFER:
		Cvar_InfoStringBuffer( args[1], VMA(2), args[3] );
		return 0;

	case UI_ARGC:
		return Cmd_Argc();

	case UI_ARGV:
		Cmd_ArgvBuffer( args[1], VMA(2), args[3] );
		return 0;

	case UI_ARGVI:
		return atoi( Cmd_Argv( args[1] ) );

	case UI_CMD_EXECUTETEXT:
		Cbuf_ExecuteText( args[1], VMA(2) );
		return 0;

	case UI_FS_FOPENFILE:
		return FS_FOpenFileByMode( VMA(1), VMA(2), args[3] );

	case UI_FS_READ:
		FS_Read( VMA(1), args[2], args[3] );
		return 0;

	case UI_FS_WRITE:
		FS_Write( VMA(1), args[2], args[3] );
		return 0;

	case UI_FS_FCLOSEFILE:
		FS_FCloseFile( args[1] );
		return 0;

	case UI_FS_GETFILELIST:
		return FS_GetFileList( VMA(1), VMA(2), (char*)args[3], VMA(3), args[4] );

	case UI_FS_SEEK:
		return FS_Seek( args[1], args[2], args[3] );
	
	case UI_R_REGISTERMODEL:
		return re.RegisterModel( VMA(1) );

	case UI_R_REGISTERSKIN:
		return re.RegisterSkin( VMA(1) );

	case UI_R_REGISTERSHADER:
		return re.RegisterShader( VMA(1) );

	case UI_R_REGISTERSHADERNOMIP:
		return re.RegisterShaderNoMip( VMA(1) );

	case UI_R_CLEARSCENE:
		re.ClearScene();
		return 0;
	
	case UI_R_BUILDPOSE:
		{
			animGroupTransition_t trans;
			trans.animGroup = 0;
			trans.interp = 0;

			return re.BuildPose( args[1], VMA( 2 ), args[3], VMA( 4 ), args[5], NULL, 0, NULL, 0, &trans, 1 );
		}
	case UI_R_BUILDPOSE2:
		return re.BuildPose( args[1], VMA( 2 ), 2, VMA( 3 ), 2, VMA( 4 ), 2, VMA( 5 ), 2, VMA( 6 ), 2 );

	case UI_R_BUILDPOSE3:
		return re.BuildPose( args[1], VMA( 2 ), 1, 0, 0, VMA( 3 ), 1, 0, 0, VMA( 4 ), 1 );

	case UI_R_SHAPECREATE:
		{
			curve_t *	c = VMA(1);
			int			n = args[3];
			int			i;
			for( i = 0; i < n; i++ )
			{
				c[i].pts = (vec2_t*)VM_ArgPtr((intptr_t)c[i].pts);
				c[i].uvs = (vec2_t*)VM_ArgPtr((intptr_t)c[i].uvs);
				c[i].colors = (vec4_t*)VM_ArgPtr((intptr_t)c[i].colors);
				c[i].indices = (short*)VM_ArgPtr((intptr_t)c[i].indices );
			}
			return re.ShapeCreate( c, VMA( 2 ), n );
		}

	case UI_R_SHAPEDRAW:
		re.ShapeDraw( args[1], args[2], VMA(3) );
		return 0;

	case UI_R_SHAPEDRAWMULTI:
		re.ShapeDrawMulti( VMA( 1 ), args[2], args[3] );
		return 0;

	case UI_R_ADDREFENTITYTOSCENE:
		re.AddRefEntityToScene( VMA(1) );
		return 0;

	case UI_R_ADDPOLYTOSCENE:
		re.AddPolyToScene( args[1], args[2], VMA(3), 1 );
		return 0;

	case UI_R_ADDLIGHTTOSCENE:
		re.AddLightToScene( VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
		return 0;

	case UI_R_RENDERSCENE:
		re.RenderScene( VMA(1) );
		return 0;

	case UI_R_SETCOLOR:
		re.SetColor( VMA(1) );
		return 0;

	case UI_R_DRAWSTRETCHPIC:
		re.DrawStretchPic( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), NULL, VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[ 9 ] );
		return 0;

	case UI_R_DRAWSPRITE:
		{
			float * uv = VMA(6);
			re.DrawStretchPic( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMA(5), uv[0],uv[1],uv[2],uv[3], args[ 7 ] );
		} return 0;

	case UI_R_RENDERTEXT:
		CL_RenderText( VMA(1), VMF(2), VMA(3), VMA(4), args[5], (args[6])>>16, args[6]&0xFFFF, args[7], args[8], Key_GetOverstrikeMode()?'|':'_', args[9], VMA(10) );
		return 0;

	case UI_R_FLOWTEXT:
		{
			const char * offset = (const char *)args[5];
			int i,n;
			const char * text = VMA(5);
			lineInfo_t * lines = (lineInfo_t*)VMA(9);
			
			if ( text ) {
				n = CL_RenderText_ExtractLines( VMA(1), VMF(2), VMA(3), VMA(4), text, args[6], args[7], VMF(8), lines, 0, args[10] );

				if ( lines ) {
					for ( i=0; i<n; i++ ) {
						lines[ i ].text = offset + (lines[ i ].text - text);
					}
				}
			} else
				n = 0;

			return n;
		}

	case UI_R_GETFONT:
		memcpy( VMA(3), re.GetFontFromFontSet( args[1], VMF(2) ), sizeof(fontInfo_t) );
		return 0;

	case UI_R_ROUNDRECT:
		SCR_FillRect( VMF(1), VMF(2), VMF(3), VMF(4), VMA(5), args[6] );
		return 0;

	case UI_R_MODELBOUNDS:
		re.ModelBounds( args[1], VMA(2), VMA(3) );
		return 0;

	case UI_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;

	case UI_CM_LERPTAG:
		re.LerpTag( VMA(1), args[2], args[3], args[4], VMF(5), VMA(6) );
		return 0;

	case UI_S_REGISTERSOUND:
		return S_RegisterSound( VMA(1), args[2] );

	case UI_S_STARTLOCALSOUND:
		S_StartLocalSound( args[1], args[2] );
		return 0;

	case UI_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf( args[1], VMA(2), args[3] );
		return 0;

	case UI_KEY_GETBINDINGBUF:
		Key_GetBindingBuf( args[1], VMA(2), args[3] );
		return 0;

	case UI_KEY_SETBINDING:
		Key_SetBinding( args[1], VMA(2) );
		return 0;

	case UI_KEY_ISDOWN:
		return Key_IsDown( args[1] );

	case UI_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();

	case UI_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode( args[1] );
		return 0;

	case UI_KEY_CLEARSTATES:
		Key_ClearStates();
		return 0;

	case UI_KEY_GETCATCHER:
		return Key_GetCatcher();

	case UI_KEY_SETCATCHER:
		Key_SetCatcher( args[1] );
		return 0;
	case UI_KEY_GETKEY:
		return Key_GetKey( VMA(1) );

	case UI_GETCLIPBOARDDATA:
		QGetClipboardData( VMA(1), args[2] );
		return 0;

	case UI_GETCLIENTSTATE:
		GetClientState( VMA(1) );
		return 0;

	case UI_GETCLIENTNUM:
		return cl.snap.ps.clientNum;

	case UI_GETGLCONFIG:
		CL_GetGlconfig( VMA(1) );
		return 0;

	case UI_GETCONFIGSTRING:
		return GetConfigString( args[1], VMA(2), args[3] );

	case UI_UPDATEGAMESTATE:
		return CL_UpdateGameState( VMA(1) );

	case UI_LAN_UPDATEVISIBLEPINGS:
		return LAN_UpdateVisiblePings( args[1] );

	case UI_LAN_SERVERSTATUS:
		return LAN_GetServerStatus( VMA(1), VMA(2), args[3] );

	case UI_MEMORY_REMAINING:
		return Hunk_MemoryRemaining();

	case UI_SET_PBCLSTATUS:
		return 0;	

	case UI_R_REGISTERFONT:
		return re.RegisterFont( VMA(1) );

	case UI_R_GETFONTS:
		re.GetFonts( VMA(1), args[2] );
		return 0;

	case UI_MEMSET:
		Com_Memset( VMA(1), args[2], args[3] );
		return 0;

	case UI_MEMCPY:
		Com_Memcpy( VMA(1), VMA(2), args[3] );
		return 0;

	case UI_STRNCPY:
		strncpy( VMA(1), VMA(2), args[3] );
		return args[1];

	case UI_SIN:
		return FloatAsInt( sin( VMF(1) ) );

	case UI_COS:
		return FloatAsInt( cos( VMF(1) ) );

	case UI_ATAN2:
		return FloatAsInt( atan2( VMF(1), VMF(2) ) );

	case UI_SQRT:
		return FloatAsInt( sqrt( VMF(1) ) );

	case UI_FLOOR:
		return FloatAsInt( floor( VMF(1) ) );

	case UI_CEIL:
		return FloatAsInt( ceil( VMF(1) ) );

	case UI_ACOS:	return FloatAsInt( acos( VMF(1) ) );
	case UI_LOG:	return FloatAsInt( log( VMF(1) ) );
	case UI_POW:	return FloatAsInt( pow( VMF(1), VMF(2) ) );
	case UI_ATAN:	return FloatAsInt( atan( VMF(1) ) );
	case UI_FMOD:	return FloatAsInt( fmod( VMF(1), VMF(2) ) );
	case UI_TAN:	return FloatAsInt( tan( VMF(1) ) );

	case UI_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine( VMA(1) );
	case UI_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle( VMA(1) );
	case UI_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle( args[1] );
	case UI_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle( args[1], VMA(2) );
	case UI_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine( args[1], VMA(2), VMA(3) );

	case UI_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;
	case UI_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack( VMA(1), VMA(2));
		return 0;

	case UI_REAL_TIME:
		return Com_RealTime( VMA(1) );

	case UI_CIN_PLAYCINEMATIC:
	  Com_DPrintf("UI_CIN_PlayCinematic\n");
	  return CIN_PlayCinematic(VMA(1), args[2], args[3], args[4], args[5], args[6]);

	case UI_CIN_STOPCINEMATIC:
	  return CIN_StopCinematic(args[1]);

	case UI_CIN_RUNCINEMATIC:
	  return CIN_RunCinematic(args[1]);

	case UI_CIN_DRAWCINEMATIC:
	  CIN_DrawCinematic(args[1]);
	  return 0;

	case UI_CIN_SETEXTENTS:
	  CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
	  return 0;

	case UI_R_REMAP_SHADER:
		//ToDo: remove this trap!
		return 0;

	case UI_CON_PRINT:
		return Con_Print( args[ 1 ], VMA( 2 ) );

	case UI_CON_DRAW:
		Con_Draw( args[ 1 ], VMF( 2 ), qfalse, qfalse );
		return 0;
	case UI_CON_HANDLEMOUSE:
		Con_HandleMouse( args[ 1 ], VMF( 2 ), VMF( 3 ) );
		return 0;
	case UI_CON_HANDLEKEY:
		Con_HandleKey( args[ 1 ], args[ 2 ] );
		return 0;
	case UI_CON_CLEAR:
		Con_Clear( args[ 1 ] );
		return 0;
	case UI_CON_RESIZE:
		Con_Resize( args[ 1 ], args[ 2 ], VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ) );
		return 0;

	case UI_CON_GETTEXT:
		UI_Con_GetText( VMA( 1 ), args[2], args[3] );
		return 0;

	case UI_GET_RADAR:
		return CL_GetRadar( VMA( 1 ), args[2], VMA( 3 ), args[4], VMF( 5 ) );

	case UI_Q_rand:
		return Rand_NextUInt32( &cl.db.rand );

	case UI_SQL_LOADDB:
		{
			char *	buffer;
			int		length;
			char *	file = VMA(1);
			length = FS_ReadFile( file, &buffer );
			if ( length <= 0 ) {
				Com_Error( ERR_FATAL, "Can't open db: '%s'\n", file );
			}

			cl.db.create_filter = VMA(2);

			sql_exec( &cl.db, buffer );
			FS_FreeFile(buffer);

			cl.db.create_filter = 0;
			
			return 0;
		} break;

	case UI_SQL_EXEC:			return sql_exec( &cl.db, VMA(1) );
	case UI_SQL_PREPARE:		return sql_prepare( &cl.db, VMA(1) ) != 0;
	case UI_SQL_BIND:			return sql_bind( &cl.db, args );
	case UI_SQL_BINDTEXT:		return sql_bindtext( &cl.db, args[1], VMA(2) );
	case UI_SQL_BINDINT:		return sql_bindint( &cl.db, args[1], args[2] );
	case UI_SQL_BINDARGS:
		{
			int i,n = Cmd_Argc();
			for ( i=1; i<n; i++ )
			{
				if ( !sql_bindtext( &cl.db, i, Cmd_Argv( i ) ) )
					return 0;
			}
			
		} return 1;

	case UI_SQL_STEP:			return sql_step( &cl.db );
	case UI_SQL_COLUMNCOUNT:	return sql_columncount( &cl.db );
	case UI_SQL_COLUMNASTEXT:
		Q_strncpyz( VMA(1), sql_columnastext( &cl.db,args[3] ), args[2] );
		break;
	case UI_SQL_COLUMNASINT:	return sql_columnasint( &cl.db, args[1] );
	case UI_SQL_COLUMNNAME:	
		Q_strncpyz( VMA(1), sql_columnname( &cl.db, args[3] ), args[2] );
		break;
	case UI_SQL_DONE:			return sql_done( &cl.db );
	case UI_SQL_SELECT:			return sql_select( &cl.db, VMA(1), (char*)args[2], VMA(2), args[3] );
	case UI_SQL_COMPILE:		return sql_compile( &cl.db, VMA(1) );

	case UI_SQL_RUN:
		{
			char *	buffer	= VMA(1);
			int		size	= args[2];
			int		id		= args[3];
			int		i;
			formatInfo_t *	stmt = (formatInfo_t*)cl.db.stmts_byindex[ id ];

			if ( stmt && id >= 0 && id < cl.db.stmts_byindex_count ) {

				sqlData_t params[ 3 ];
				const char * r;

				for ( i=0; i<3; i++ ) {

					params[ i ].format = INTEGER;
					params[ i ].payload.integer = args[ 4+i ];
				}

				r = sql_eval( &cl.db, stmt->print, 0, 0, 0, 0, params, 0 ).string;

				Q_strncpyz( buffer, r, size );
			} else {
				buffer[ 0 ] = '\0';
			}

		} break;

		//	give ui read access to file system info
	case UI_SQL_SV_SELECT:
		{
			if ( com_sv_running->integer ) {
				return select_from_server( VMA(1), (char*)args[2], VMA(2), args[3] );
			}
		} break;

		
	default:
		Com_Error( ERR_DROP, "Bad UI system trap: %i", args[0] );

	}

	return 0;
}

/*
====================
CL_ShutdownUI
====================
*/
void CL_ShutdownUI( qboolean save_menustack ) {
	cls.keyCatchers &= ~KEYCATCH_UI;
	cls.uiStarted = qfalse;
	if ( !uivm ) {
		return;
	}
	VM_Call( uivm, UI_SHUTDOWN, save_menustack );
	VM_Free( uivm );
	uivm = NULL;
}

/*
====================
CL_InitUI
====================
*/
void CL_InitUI( void ) {
	int		v;
	vmInterpret_t		interpret;

	// dont start ui if no config file present
	//if ( Cvar_VariableIntegerValue( "fs_noconfig" ) == 1 ) {
	//	return;
	//}

	// load the dll or bytecode
	if ( cl_connectedToPureServer != 0 ) {
		// if sv_pure is set we only allow qvms to be loaded
		interpret = VMI_COMPILED;
	}
	else {
		interpret = Cvar_VariableValue( "vm_ui" );
	}
	uivm = VM_Create( "ui", CL_UISystemCalls, interpret );
	if ( uivm ) {
		//Com_Error( ERR_FATAL, "VM_Create on UI failed" );

		// sanity check
		v = VM_Call( uivm, UI_GETAPIVERSION );
		if (v != UI_API_VERSION) {
			Com_Error( ERR_DROP, "User Interface is version %d, expected %d", v, UI_API_VERSION );
			cls.uiStarted = qfalse;
		}
		else {
#ifdef USE_WEBAUTH
			botlib_export->PC_AddGlobalDefine( "USE_WEBAUTH" );
#endif

			// init for this gamestate
			VM_Call( uivm, UI_INIT, (cls.state >= CA_AUTHORIZING && cls.state <= CA_ACTIVE), clc.clientNum );
		}

#ifdef USE_WEBAUTH
		//	if the session id has something in it, then assume that the browser sent it from the
		//	command line and tell ui we're already logged in.  
		if ( com_sessionid->string[0] ) {
			VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_OK );
		}
#endif
	}
}

/*
====================
UI_GameCommand

See if the current console command is claimed by the ui
====================
*/
qboolean UI_GameCommand( void ) {
	if ( !uivm ) {
		return qfalse;
	}

	return VM_Call( uivm, UI_CONSOLE_COMMAND, cls.realtime );
}
