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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"

qboolean	scr_initialized;		// ready to draw

cvar_t		*cl_timegraph;
cvar_t		*cl_debuggraph;
cvar_t		*cl_graphheight;
cvar_t		*cl_graphscale;
cvar_t		*cl_graphshift;
cvar_t		*cl_cornersize;


/*
================
SCR_DrawNamedPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t	hShader;

	assert( width != 0 );

	hShader = re.RegisterShader( picname );
	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, NULL, 0, 0, 1, 1, hShader );
}


/*
================
SCR_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void SCR_AdjustFrom640( float *x, float *y, float *w, float *h )
{
	if ( x ) {
		*x = *x * cls.glconfig.xscale + cls.glconfig.xbias;
	}
	if ( y ) {
		*y *= cls.glconfig.yscale;
	}
	if ( w ) {
		*w *= cls.glconfig.xscale;
	}
	if ( h ) {
		*h *= cls.glconfig.yscale;
	}
}

/*
================
SCR_FillRect

Coordinates are 640*480 virtual values
=================
*/
void SCR_FillRect( float x, float y, float w, float h, const float *color, qhandle_t shader ) {
	float	rx,ry;
	float	cw,ch;

	x -= cl_cornersize->value*0.25f;
	y -= cl_cornersize->value*0.25f;
	w += cl_cornersize->value*0.5f;
	h += cl_cornersize->value*0.5f;

	x = x*cls.glconfig.xscale + cls.glconfig.xbias;
	y *= cls.glconfig.yscale;
	w *= cls.glconfig.xscale;
	h *= cls.glconfig.yscale;

	rx = cl_cornersize->value * cls.glconfig.xscale;
	ry = cl_cornersize->value * cls.glconfig.yscale;

	// find corner size
	if ( rx * 2.0f >= w )
		rx = w*0.5f;

	if ( ry * 2.0f >= h )
		ry = h*0.5f;

	if ( rx < ry )
		ry = rx;
	if ( ry < rx )
		rx = ry;

	if ( color )
		re.SetColor( color );

	cw = w-rx*2.0f;
	ch = h-ry*2.0f;

	// corners
	re.DrawStretchPic( x,		y,		rx+cw,	ry+ch,	NULL, 0.0f, 0.0f, (rx+cw)/rx,	(ry+ch)/ry,		shader );
	re.DrawStretchPic( x,		y+h-ry,	rx+cw,	ry,		NULL, 0.0f, 1.0f, (rx+cw)/rx,	0.0f,			shader );
	re.DrawStretchPic( x+w-rx,	y,		rx,		ry+ch,	NULL, 1.0f, 0.0f, 0.0f,		(ry+ch)/ry,		shader );
	re.DrawStretchPic( x+w-rx,	y+h-ry,	rx,		ry,		NULL, 1.0f, 1.0f, 0.0f,		0.0f,			shader );
}

void SCR_FillSquareRect( float x, float y, float w, float h, const float *color ) {
	qhandle_t	fill	= cls.whiteShader;
	SCR_AdjustFrom640( &x, &y, &w, &h );
	re.SetColor( color );
	re.DrawStretchPic( x, y, w, h, NULL, 0, 0, 1, 1, fill );
}


/*
================
SCR_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, NULL, 0, 0, 1, 1, hShader );
}

/*
** SCR_Strlen -- skips color escape codes
*/
static int SCR_Strlen( const char *str ) {
	const char *s = str;
	int count = 0;

	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
		} else {
			count++;
			s++;
		}
	}

	return count;
}

//===============================================================================

/*
=================
SCR_DrawDemoRecording
=================
*/
void SCR_DrawDemoRecording( void ) {
	char	string[1024];
	int		pos;
	float r[4];

	if ( !clc.demorecording ) {
		return;
	}

	pos = FS_FTell( clc.demofile );
	sprintf( string, "RECORDING %s: %ik", clc.demoName, pos / 1024 );
	
	r[0] = 320 - strlen( string ) * 4.0f;
	r[1] = 20.0f;
	r[2] = 80.0f;
	r[3] = 16.0f;

	CL_RenderText( r, 0.3f, colorWhite, string, -1, TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER, 3, -1, 0, cls.font, 0);
}


/*
===============================================================================

_DEBUG GRAPH

===============================================================================
*/

typedef struct
{
	float	value;
	int		color;
} graphsamp_t;

static	int			current;
static	graphsamp_t	values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph (float value, int color)
{
	values[current&1023].value = value;
	values[current&1023].color = color;
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph (void)
{
	int		a, x, y, w, i, h;
	float	v;
	int		color;

	//
	// draw the graph
	//
	w = cls.glconfig.vidWidth;
	x = 0;
	y = cls.glconfig.vidHeight;
	re.SetColor( g_color_table[0] );
	re.DrawStretchPic(x, y - cl_graphheight->integer, 
		w, cl_graphheight->integer, NULL, 0, 0, 0, 0, cls.whiteShader );
	re.SetColor( NULL );

	for (a=0 ; a<w ; a++)
	{
		i = (current-1-a+1024) & 1023;
		v = values[i].value;
		color = values[i].color;
		v = v * cl_graphscale->integer + cl_graphshift->integer;
		
		if (v < 0)
			v += cl_graphheight->integer * (1+(int)(-v / cl_graphheight->integer));
		h = (int)v % cl_graphheight->integer;
		re.DrawStretchPic( x+w-1-a, y - h, 1, h, NULL, 0, 0, 0, 0, cls.whiteShader );
	}
}

//=============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init( void ) {
	cl_timegraph = Cvar_Get ("timegraph", "0", CVAR_CHEAT);
	cl_debuggraph = Cvar_Get ("debuggraph", "0", CVAR_CHEAT);
	cl_graphheight = Cvar_Get ("graphheight", "32", CVAR_CHEAT);
	cl_graphscale = Cvar_Get ("graphscale", "1", CVAR_CHEAT);
	cl_graphshift = Cvar_Get ("graphshift", "0", CVAR_CHEAT);
	cl_cornersize = Cvar_Get( "cl_cornersize", "20", CVAR_ARCHIVE );

	scr_initialized = qtrue;
}


//=======================================================


extern int cmd_waitfordownload;

/*
==================
SCR_DrawScreenField

This will be called twice if rendering in stereo mode
==================
*/
void SCR_DrawScreenField( stereoFrame_t stereoFrame )
{
	int uiFullScreen;
	re.BeginFrame( stereoFrame );

	// wide aspect ratio screens need to have the sides cleared
	// unless they are displaying game renderings
	if( cls.state != CA_ACTIVE )
	{
		if( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 )
		{
			re.SetColor( g_color_table[0] );
			re.DrawStretchPic( 0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, NULL, 0, 0, 0, 0, cls.whiteShader );
			re.SetColor( NULL );
		}
	}

if( uivm )
{
		//Com_DPrintf("draw screen without UI loaded\n");

	// if the menu is going to cover the entire screen, we
	// don't need to render anything under it
	uiFullScreen = VM_Call( uivm, UI_IS_FULLSCREEN, 0 );
	//if( !uiFullScreen )
	{
		switch( cls.state )
		{
		case CA_CINEMATIC:
			SCR_DrawCinematic();
			break;
		case CA_DISCONNECTED:
			// force menu up
			if ( !uiFullScreen )
			{
				S_StopAllSounds();
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
			}
			VM_Call( uivm, UI_REFRESH, 0, cls.realtime );
			break;
		case CA_CONNECTING:
		case CA_CHALLENGING:
		case CA_CONNECTED:
			// connecting clients will only show the connection dialog
			// refresh to update the time
			//VM_Call( uivm, UI_REFRESH, 0, cls.realtime );
			VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qfalse );
			Sys_SetCursor( 2 );
			break;
		case CA_LOADING:
		case CA_PRIMED:
			// draw the game information screen and loading progress
			CL_CGameRendering( stereoFrame, qfalse );

			//if( re.ApplyBloom )
			//	re.ApplyBloom();

			// also draw the connection information, so it doesn't
			// flash away too briefly on local or lan games
			// refresh to update the time
			VM_Call( uivm, UI_REFRESH, 0, cls.realtime );
			VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qtrue );
			break;
		case CA_ACTIVE:
			CL_CGameRendering( stereoFrame, uiFullScreen );
			re.PostProcess();
			SCR_DrawDemoRecording();
			VM_Call( uivm, UI_REFRESH, 0, cls.realtime );
			CL_CGameRenderOverlays( stereoFrame );
			break;
		default:
			Com_Error( ERR_FATAL, "SCR_DrawScreenField: bad cls.state" );
			break;
		}
	}

	// console draws next
	Con_DrawConsole();
}

	// show download progress
	if ( Cvar_VariableIntegerValue( "com_downloading" ) == 1 ) {
		CL_DownloadProgress();
	}

	// debug graph can be drawn on top of anything
	if( cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer )
		SCR_DrawDebugGraph ();
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen( void ) {
	static int	recursive;

	if ( !scr_initialized ) {
		return;				// not initialized yet
	}

	if ( ++recursive > 2 ) {
		Com_Error( ERR_FATAL, "SCR_UpdateScreen: recursively called" );
	}
	recursive = 1;

	// if running in stereo, we need to draw the frame twice
	if ( cls.glconfig.stereoEnabled ) {
		SCR_DrawScreenField( STEREO_LEFT );
		SCR_DrawScreenField( STEREO_RIGHT );
	} else {
		SCR_DrawScreenField( STEREO_CENTER );
	}

	if ( com_speeds->integer ) {
		re.EndFrame( &time_frontend, &time_backend );
	} else {
		re.EndFrame( NULL, NULL );
	}

	recursive = 0;
}

