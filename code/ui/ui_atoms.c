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

#include "ui_local.h"

/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/

// these are here so the functions in q_shared.c can link
#ifndef UI_HARD_LINKED

void QDECL Com_Error( int level, const char *error, ... )
{
	va_list		argptr;
	char		text[1024];

	REF_PARAM( level );

	va_start (argptr, error);
	vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	trap_Error( level, va("%s", text) );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Print( va("%s", text) );
}

#endif

qboolean newUI = qfalse;


/*
=================
UI_ClampCvar
=================
*/
float UI_ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

char *UI_Argv( int arg ) {
	char	buffer[MAX_STRING_CHARS];
	trap_Argv( arg, buffer, sizeof( buffer ) );
	return va("%s",buffer);
}

int UI_Argc()
{
	return trap_Argc();
}

char *UI_Cvar_VariableString( const char *var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}


static void	UI_Cache_f() {
	Display_CacheAll();
}

static void UI_Load_All_Npcs_f(){
	fileHandle_t f;
	while(sql("SELECT model, type FROM npcs", 0)) {
		const char *type =  sqltext(1);
		char 		*slash;
		char		modelName[MAX_QPATH];
		char		skinName[MAX_QPATH];
		int			model;
		
		Q_strncpyz(modelName, sqltext(0), sizeof(modelName) );

		slash = strchr( modelName, '/' );
		if ( !slash ) {
			// modelName did not include a skin name
			Q_strncpyz( skinName, "default", sizeof( skinName ) );
		} else {
			Q_strncpyz( skinName, slash + 1, sizeof( skinName ) );
			*slash = '\0';
		}

		switch (SWITCHSTRING(type)){
		case CS('B','o','s','s'):
			model = trap_R_RegisterModel(va("models/players/%s/char.x42", modelName));
			trap_FS_FOpenFile( va("models/players/%s/animation.cfg", modelName), &f, FS_READ );
			trap_FS_FCloseFile(f);
			break;
		default:
			model = trap_R_RegisterModel(va("models/players/%s/char_hub.x42", modelName));
			trap_FS_FOpenFile( va("models/players/%s/animation_hub.cfg", modelName), &f, FS_READ );
			trap_FS_FCloseFile(f);
			model = trap_R_RegisterModel(va("models/players/%s/char.x42", modelName));
			trap_FS_FOpenFile( va("models/players/%s/animation.cfg", modelName), &f, FS_READ );
			trap_FS_FCloseFile(f);
			Com_Printf("Model: %s Name: %s\n", sqltext(0), sqltext(1) );
		}
		if (!model) {
			Com_Printf("ERROR: could not load char.x42 or char_hub.x42 for %s", modelName);
		} else {
			trap_R_RegisterSkin(va("models/players/%s/char_%s.skin", modelName, skinName));
		}

	}
	trap_R_RegisterModel("models/players/moasoldier/char.x42");
	trap_FS_FOpenFile( "models/players/moasoldier/animation.cfg", &f, FS_READ );
	trap_FS_FCloseFile(f);
	trap_R_RegisterModel("models/players/moasoldier/char_hub.x42");
	trap_FS_FOpenFile( "models/players/moasoldier/animation_hub.cfg", &f, FS_READ );
	trap_FS_FCloseFile(f);
	trap_R_RegisterSkin("models/players/moasoldier/char_default.skin");
	trap_R_RegisterSkin("models/players/moasoldier/char_agent.skin");
	trap_R_RegisterSkin("models/players/moasoldier/char_auditor.skin");
	trap_R_RegisterModel("models/players/thug/char.x42");
	trap_FS_FOpenFile( "models/players/moasoldier/animation.cfg", &f, FS_READ );
	trap_FS_FCloseFile(f);
	trap_R_RegisterSkin("models/players/thug/char_default.skin");
	trap_R_RegisterSkin("models/players/thug/char_easy.skin");
	trap_R_RegisterSkin("models/players/thug/char_medium.skin");
	trap_R_RegisterSkin("models/players/thug/char_hard.skin");
}

/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand( int realTime ) {
	char	*cmd;

	uiInfo.uiDC.frameTime = realTime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realTime;

	cmd = UI_Argv( 0 );


	//	all commands prefixed with st_ are destined for the rules engine.
	if (	cmd[ 0 ] == 's' &&
			cmd[ 1 ] == 't' &&
			cmd[ 2 ] == '_' )
	{
		if ( UI_ST_exec( UI_ST_CONSOLECOMMAND, cmd+3 ) )
			return qtrue;

	} else 	if (	cmd[ 0 ] == 'u' &&
					cmd[ 1 ] == 'i' &&
					cmd[ 2 ] == '_' )
	{

		switch ( SWITCHSTRING( cmd+3 ) )
		{
			//	ui_reportbug
		case CS('r','e','p','o'):
			if (trap_Cvar_VariableInt("developer") == 1) {
				trap_Key_SetCatcher( KEYCATCH_BUG );
				Menus_OpenByName( "reportbug" );
			}
			return qtrue;

			//	ui_cache
		case CS('c','a','c','h'):
			UI_Cache_f();
			return qtrue;

			// ui_vote
		case CS('v','o','t','e'):
			UI_Vote( trap_ArgvI( 1 ), UI_Argv( 2 ) );
			return qtrue;
			// ui_load_npcs
		case CS('l','o','a','d'):
			UI_Load_All_Npcs_f();
			return qtrue;
		}
	} else
	{
		switch( SWITCHSTRING(cmd) )
		{
			// remapShader
		case CS('r','e','m','a'):
			{
				if (trap_Argc() == 4) {
					char shader1[MAX_QPATH];
					char shader2[MAX_QPATH];
					trap_Argv( 1, shader1, sizeof(shader1) );
					trap_Argv( 2, shader2, sizeof(shader2) );
					trap_R_RemapShader( shader1, shader2 );
					return qtrue;
				}
			} break;
		case CS('c','s',0,0):
			{
				UI_ST_exec( UI_ST_CONFIGSTRING, trap_ArgvI(1) );
			} break;
		}
	}

	return qfalse;
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void ) {
}

/*
================
UI_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void UI_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	// expect valid pointers
	*x = *x * uiInfo.uiDC.glconfig.xscale + uiInfo.uiDC.glconfig.xbias;
	*y *= uiInfo.uiDC.glconfig.yscale;
	*w *= uiInfo.uiDC.glconfig.xscale;
	*h *= uiInfo.uiDC.glconfig.yscale;
}

void UI_AdjustTo640( int *x, int *y )
{
	float X = (float)*x;
	float Y = (float)*y;

	X = (X - uiInfo.uiDC.glconfig.xbias) / uiInfo.uiDC.glconfig.xscale;
	Y = Y / uiInfo.uiDC.glconfig.yscale;

	*x = (int)X;
	*y = (int)Y;
}

void UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ) {
	float	s0;
	float	s1;
	float	t0;
	float	t1;

	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}
	
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}

void UI_UpdateScreen( void ) {
	trap_UpdateScreen();
}

qboolean UI_CursorInRect (int x, int y, int width, int height)
{
	if (uiInfo.uiDC.cursorx < x ||
		uiInfo.uiDC.cursory < y ||
		uiInfo.uiDC.cursorx > x+width ||
		uiInfo.uiDC.cursory > y+height)
		return qfalse;

	return qtrue;
}

void UI_ActivateEditDlg( const char* menuName, const char * itemName, float val, float minVal, float maxVal, qboolean activate )
{
	menuDef_t * menu = Menus_FindByName( menuName );
	if ( menu )
	{
		itemDef_t * item = Menu_FindItemByName( menu, itemName );
		if ( item )
		{
			editFieldDef_t * edit = (editFieldDef_t*)item->typeData;
			if ( edit )
			{
				trap_Cvar_SetValue( item->cvar, val );
				edit->minVal = minVal;
				edit->maxVal = maxVal;

				if ( activate )
					Menus_Activate( menu );
			}
		}
	}
}

void DrawShape( rectDef_t * r, qhandle_t shape, qhandle_t shader ) {

	vec3_t m[ 2 ];
	m[ 0 ][ 0 ] = r->w * uiInfo.uiDC.glconfig.xscale;
	m[ 0 ][ 1 ] = 0.0f;
	m[ 0 ][ 2 ] = r->x * uiInfo.uiDC.glconfig.xscale + uiInfo.uiDC.glconfig.xbias;

	m[ 1 ][ 0 ] = 0.0f;
	m[ 1 ][ 1 ] = r->h * uiInfo.uiDC.glconfig.yscale;
	m[ 1 ][ 2 ] = r->y * uiInfo.uiDC.glconfig.yscale;
	
	trap_R_ShapeDraw( shape, shader, (float*)m );
}

