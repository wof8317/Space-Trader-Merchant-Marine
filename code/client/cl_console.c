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
// console.c

#include "client.h"


#define	NUM_CON_TIMES 4

#define		CON_TEXTSIZE		0x20000
#define		CON_BORDERSIZE		4.0F
#define		CON_SCROLLBARWIDTH	8.0F


typedef struct
{
	qboolean	initialized;

	char		text[CON_TEXTSIZE];
	int			textLen;

	float		fontScale;

	float		x, y, w, h;

	int			numLines;		//number of lines in the console
	int			dispLines;		//number of lines the console can display
	int			currLine;		//bottom of console displays this line

	int 		linewidth;		//characters across screen
	float		displayFrac;	//aproaches finalFrac at scr_conspeed
	float		finalFrac;		//0.0 to 1.0 lines of console to display

	float		promptY;

	vec4_t		color;			//for transparent notify lines
	int			time;			//time last line was printed

} console_t;

#define	DEFAULT_CONSOLE_WIDTH	78

static console_t cons[2];


cvar_t *con_conspeed;
cvar_t *con_notifytime;

const vec4_t console_color = { 1.0F, 1.0F, 1.0F, 1.0F };

static void Con_CountLines( int c )
{
	fontInfo_t *font;

	if( !re.GetFontFromFontSet )
		return;

	if( !cls.font )
		return;

	if( cls.glconfig.xscale == 0.0F )
		return;

	if( cons[c].w == 0.0F )
	{
		cons[c].x = -cls.glconfig.xbias / cls.glconfig.xscale;
		cons[c].y = 0.0F;
		cons[c].w = SCREEN_WIDTH + (cls.glconfig.xbias  / cls.glconfig.xscale) * 2.0F;
		cons[c].h = SCREEN_HEIGHT * 0.5F;
	}

	font = re.GetFontFromFontSet( cls.font, cons[c].fontScale );

	cons[c].fontScale = ((c==0)?0.28F:0.35f) / cls.glconfig.yscale;
	cons[c].dispLines = (int)(cons[c].h / (font->glyphs['A'].height * cons[c].fontScale * font->glyphScale * 1.5F));

	//count lines
	cons[c].numLines = CL_RenderText_ExtractLines( font, cons[c].fontScale * font->glyphScale, cons[c].color, cons[c].color,
		cons[c].text, -1, 0, cons[c].w - CON_SCROLLBARWIDTH, NULL, 0, -1 ); //count lines - but don't store flow info

	if( cons[c].currLine > cons[c].numLines - cons[c].dispLines - 1 )
		cons[c].currLine = cons[c].numLines - cons[c].dispLines - 1;
}

void Con_Resize( int c, int display, float x, float y, float w, float h )
{
	if( display >= 0 )
		cons[c].currLine = display;
	cons[c].x = x;
	cons[c].y = y;
	cons[c].w = w;
	cons[c].h = h;

	Con_CountLines( c );
}

void Con_HandleMouse( int c, float x, float y )
{
}

const char* Q_EXTERNAL_CALL Con_GetText( int console )
{
	if( console >= 0 && console < lengthof( cons ) && cons[console].text )
		return cons[console].text;
	else
		return NULL;
}

void Con_HandleKey( int c, int key )
{
	switch( key )
	{
	case K_HOME:
		cons[c].currLine = cons[c].numLines ? 0 : -1;
		break;

	case K_END:
		cons[c].currLine = cons[c].numLines - 1;

		if( cons[c].currLine > cons[c].numLines - cons[c].dispLines - 1 )
			cons[c].currLine = cons[c].numLines - cons[c].dispLines - 1;
		break;

	case K_PGUP:
		cons[c].currLine -= 2;
		
		if( cons[c].currLine < 0 )
			cons[c].currLine = 0;
		break;

	case K_PGDN:
		cons[c].currLine += 2;
		
		if( cons[c].currLine > cons[c].numLines - cons[c].dispLines - 1 )
			cons[c].currLine = cons[c].numLines - cons[c].dispLines - 1;
		break;
	}
}

void Con_Clear( int c )
{
	cons[c].text[0] = 0;
	cons[c].textLen = 0;
	cons[c].numLines = 0;

	Con_Bottom();
}

void Con_Draw( int index, float frac, bool scrollBar, bool background )
{
	int i, n, rows, currLine, numChars;
	float lineHeight,width=0.0f;

	float cl[4];

	//const vec4_t scrollBarColor = { 0.4F, 0.4F, 0.4F, 0.8F };
	const vec4_t scrollThumbColor = { 0.7F, 0.7F, 0.7F, 0.75F };

	const char *txt;
	lineInfo_t lines[128];

	fontInfo_t *font;
	console_t *c = cons + index;

	c->fontScale = ((index==0)?0.28F:0.35f) / cls.glconfig.yscale;
	font = re.GetFontFromFontSet( cls.font, c->fontScale );

	lineHeight = font->glyphs['A'].height * c->fontScale * font->glyphScale * 1.5F;

	rows = (int)((c->h * frac) / lineHeight);

	if( rows < 1 )
		return;

	currLine = c->currLine + c->dispLines - rows;

	//
	//	draw scroll indicator
	//
	if( scrollBar )
	{
		float ty, th;

		//SCR_FillRect( c->x + c->w - CON_SCROLLBARWIDTH, c->y, CON_SCROLLBARWIDTH, c->h * frac, scrollBarColor, cls.menu );

		th = (float)c->dispLines / (float)c->numLines;
		if( th > 1 )
			th = 1;

		ty = (float)c->currLine / (float)c->numLines;

		SCR_FillRect( c->x + c->w - CON_SCROLLBARWIDTH - CON_BORDERSIZE, c->y + ty * c->h * frac, CON_SCROLLBARWIDTH, th * c->h * frac, scrollThumbColor, cls.menu );
	}

	//rip through the lines till we get to the one we want to draw from
	txt = c->text;
	Vec4_Cpy( cl, c->color );

	for( i = currLine; i > 0; )
	{
		n = CL_RenderText_ExtractLines( font, c->fontScale * font->glyphScale, c->color, c->color,
			txt, -1, 0, c->w - CON_SCROLLBARWIDTH, lines, 0, min( i, lengthof( lines ) ) );

		i -= n;
		txt = lines[n - 1].text + lines[n - 1].count;

		if( txt[0] == '\n' )
			txt++;

		while( txt[0] == ' ' )
			txt++;

		Vec4_Cpy( cl, lines[n - 1].endColor );
	}
	
	//get line info

	n = CL_RenderText_ExtractLines( font, c->fontScale * font->glyphScale, cl, c->color,
		txt, -1, 0, c->w - CON_SCROLLBARWIDTH, lines, 0, min( rows, lengthof( lines ) ) );

	numChars = 0;
	for( i = 0; i < n; i++ ) {
		numChars += lines[i].count;
		width = max( width, lines[i].width );
	}

	if ( background ) {
		float x = c->x;
		float y = c->y;
		float w = width+CON_BORDERSIZE*2.0f;
		float h = n*lineHeight;
		SCR_AdjustFrom640( &x, &y, &w, &h );
		re.DrawStretchPic( x,y,w,h, NULL, 0, 0, 1, 1, cls.whiteShader );
	}

	re.TextBeginBlock( numChars );

	for( i = 0; i < n; i++ )
	{
		re.SetColor( c->color );
		if( !lines[i].count )
			continue;
		CL_RenderText_Line( lines + i, font, c->x + CON_BORDERSIZE, c->y + (i + 1) * lineHeight, c->fontScale * font->glyphScale, 1.0F, 1.0F, 0, -1, 0, qtrue );
	}

	re.TextEndBlock();

	re.SetColor( NULL );
}

/*
================
Con_ToggleConsole_f
================
*/
void Q_EXTERNAL_CALL Con_ToggleConsole_f (void) {
	// closing a full screen console restarts the demo loop
	if ( cls.state == CA_DISCONNECTED && cls.keyCatchers == KEYCATCH_CONSOLE ) {
		CL_StartDemoLoop();
		return;
	}

	Field_Clear( &g_consoleField );

	Con_ClearNotify ();
	cls.keyCatchers ^= KEYCATCH_CONSOLE;
}

/*
================
Con_MessageMode_f
================
*/
void Q_EXTERNAL_CALL Con_MessageMode_f (void)
{
#if 0
	chat_playerNum = -1;
	chat_team = qfalse;
	Field_Clear( &chatField );

	cls.keyCatchers ^= KEYCATCH_MESSAGE;
#else
	VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_CHAT );
#endif
}

/*
================
Con_MessageMode2_f
================
*/
void Q_EXTERNAL_CALL Con_MessageMode2_f (void) {
	chat_playerNum = -1;
	chat_team = qtrue;
	Field_Clear( &chatField );
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode3_f
================
*/
void Q_EXTERNAL_CALL Con_MessageMode3_f (void) {
	chat_playerNum = VM_Call( cgvm, CG_CROSSHAIR_PLAYER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode4_f
================
*/
void Q_EXTERNAL_CALL Con_MessageMode4_f (void) {
	chat_playerNum = VM_Call( cgvm, CG_LAST_ATTACKER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_Clear_f
================
*/
void Q_EXTERNAL_CALL Con_Clear_f (void) {

	Con_Clear( 0 );
}

						
/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Q_EXTERNAL_CALL Con_Dump_f (void)
{
	fileHandle_t	f;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Com_Printf ("Dumped console text to %s.\n", Cmd_Argv(1) );

	f = FS_FOpenFileWrite( Cmd_Argv( 1 ) );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	FS_Write( cons[ 0 ].text, strlen( cons[ 0 ].text ), f );
	FS_FCloseFile( f );
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void )
{

}

/*
================
Con_Init
================
*/
void Con_Init (void) {
	int		i;
	con_notifytime = Cvar_Get ("con_notifytime", "3", 0);
	con_conspeed = Cvar_Get ("scr_conspeed", "6", 0);

	Field_Clear( &g_consoleField );
	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &historyEditLines[i] );
	}

	Cmd_AddCommand( "toggleconsole", Con_ToggleConsole_f );
	Cmd_AddCommand( "messagemode", Con_MessageMode_f );
//	Cmd_AddCommand( "messagemode2", Con_MessageMode2_f );
//	Cmd_AddCommand( "messagemode3", Con_MessageMode3_f );
//	Cmd_AddCommand( "messagemode4", Con_MessageMode4_f );
	Cmd_AddCommand( "clear", Con_Clear_f );
	Cmd_AddCommand( "condump", Con_Dump_f );
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
================
*/
int Con_Print( int index, const char *txt )
{
	int len;
	bool notify = false;
	bool colorClear, autoScroll;
	console_t *c = cons + index;

	if( Q_strncmp( txt, "[NOTICE]", 8 ) == 0 )
	{
		notify = true;
		txt += 8;
	}

	c->time = cls.realtime;

	if ( index == 1 ) {
		if ( c->finalFrac < 0.25f )
			c->finalFrac = 0.25f;
	}

	// for some demos we don't want to ever show anything on the console
	if( cl_noprint && cl_noprint->integer )
		return 0;

	if( !c->initialized )
	{
		c->color[0] = 
		c->color[1] = 
		c->color[2] =
		c->color[3] = 1.0F;
		c->linewidth = -1;

		Con_ClearNotify();

		c->initialized = qtrue;
		c->fontScale = 0.19F;
		c->text[0] = 0;
		c->textLen = 0;

		c->numLines = 0;
		c->currLine = 0;
		c->dispLines = 0;
	}

	len = strlen( txt );

	colorClear = false;

	if( txt[len - 1] == '\n' )
	{
		//find out if we need to append a color reset code

		int i;

		for( i = 0; i < len; i++ )
			if( Q_IsColorString( txt + i ) )
			{
				colorClear = txt[i + 1] != '-';
				i++;
			}
	}

	if( colorClear )
		len += 2;

	if( len > lengthof( c->text ) - 1 )
	{
		//line's too long - pretend it isn't

		len = lengthof( c->text ) - 1;
	}

	if( len > lengthof( c->text ) - 1 - c->textLen )
	{
		//not enough room in the buffer - move stuff back

		const char *ch;
		int numRem = len - (lengthof( c->text ) - 1 - c->textLen);

		ch = c->text + numRem;
		while( ch[0] != 0 && ch[0] != '\n' ) //find a null or newline
			ch++;

		c->textLen -= (ch - c->text);
		memmove( c->text, ch, c->textLen );
		c->text[c->textLen] = 0;
	}

	//move the new text into the buffer
	if( colorClear )
	{
		memmove( c->text + c->textLen, txt, len - 3 );

		c->text[c->textLen + len - 3] = '^';
		c->text[c->textLen + len - 2] = '-';
		c->text[c->textLen + len - 1] = '\n';
	}
	else
	{
		memmove( c->text + c->textLen, txt, len );
	}
	c->textLen += len;
	c->text[c->textLen] = 0;

	autoScroll = c->currLine == c->numLines - c->dispLines - 1;

	Con_CountLines( index );

	//update position
	if( autoScroll )
		c->currLine = c->numLines - c->dispLines - 1;

	return c->numLines;
}

void CL_ConsolePrint( char *txt ) {
	Con_Print( 0, txt );
#if 0
#ifdef USE_IRC
	if ( !(txt[0] == 'I' && txt[1] == 'R' && txt[2] == 'C') )
		Net_IRC_SendChat("#consolespam", txt);
#endif
#endif
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput ( float y )
{
	float rect[ 4 ];
	if ( cls.state != CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_CONSOLE ) )
		return;

	rect[ 0 ] = 0.0f;
	rect[ 1 ] = y;
	rect[ 2 ] = SCREEN_WIDTH;
	rect[ 3 ] = 200.0f;

	Field_Draw( &g_consoleField, rect, "]", cons[ 0 ].fontScale, cons[ 0 ].color );
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify( void )
{
#if 0

	int		i;
	int		time;

	static float border[4] = { 0.0f,0.0f,0.0f,0.0f };
	float	rect[4];
	float	textColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float	backColor[4] = { 0.2f, 0.2f, 0.2f, 0.65f };

	//
	//	define chat box
	//
	rect[ 0 ] = cl_conXOffset->value;
	rect[ 1 ] = cl_conYOffset->value;
	rect[ 2 ] = cl_conWidth->value;
	rect[ 3 ] = cl_conHeight->value;

	//
	//	find the first line to display
	//
	for ( i=cons[ 0 ].current-NUM_CON_TIMES+1; i<=cons[ 0 ].current ; i++ )
	{
		if (i < 0)
			continue;
		time = cons[ 0 ].times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->integer*1000)
			continue;
		if (cl.snap.ps.pm_type != PM_INTERMISSION && cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME) )
			continue;

		//	attempt to fade out background
		backColor[ 3 ] = min( 0.65f, 1.0f - ( (float)time/(float)((con_notifytime->integer+1)*1000)) );

		//
		//	draw a window around the text, text can grow this window if needed
		//
		SCR_FillRect( border[ 0 ]-8.0f, border[ 1 ] - 4.0f, rect[ 2 ]+16.0f, border[ 3 ] + 8.0f, backColor, cls.menu );

		//
		//	draw chat text
		//
		CL_RenderText(	rect,
						cons[ 0 ].fontScale * 1.25f,
						textColor,
						cons[ 0 ].lines[ i ].text,
						-1,
						0,
						2,
						3,
						-1,
						0,
						cls.font,
						border );

		break;
	}

	if (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
		return;
	}

	// draw the chat line
	if ( cls.keyCatchers & KEYCATCH_MESSAGE )
	{
		float	rect[4];

		rect[ 0 ] = SCREEN_WIDTH * 0.25f,
		rect[ 1 ] = SCREEN_HEIGHT * 0.35f;
		rect[ 2 ] = SCREEN_WIDTH * 0.5f;
		rect[ 3 ] = 0.0f;

		Field_Draw( &chatField, rect, (chat_team)?"say_team:":"say:", cons[ 0 ].fontScale * 2.5f, cons[ 0 ].color );
	}
#endif
}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole( float frac )
{
	int lines;
	vec4_t color = { 0.2f,0.2f,0.2f,0.8f };
	float screenLeft, screenWidth, screenRight;
	console_t *c = cons + 0;

	lines = c->h * frac;
	if( lines < 24.0F )
		return;

	screenLeft = -cls.glconfig.xbias / cls.glconfig.xscale;
	screenWidth	= c->w + (cls.glconfig.xbias  / cls.glconfig.xscale)*2.0f;
	screenRight = screenLeft + screenWidth;

	SCR_FillRect( screenLeft, 0.0f, screenWidth - CON_SCROLLBARWIDTH, c->h * frac, color, cls.menu );

	//
	// draw the version number
	//
	{
		float rect[4];
		rect[0] = 0.0f;
		rect[1] = 8.0F;
		rect[2] = 600.0F;
		rect[3] = 16.0F;
		//SCR_FillRect( rect[0], rect[1], rect[2], rect[3], color, cls.menu );
		CL_RenderText(	rect, 0.3F, colorRed, Q3_VERSION, -1, 2, 1, 3, -1, 0, cls.font*2.0f, 0 ); 
	}

#if 0
	//draw error indicator
	{
		int i;
		stretchRect_t *scrollColors;
		int numScrollColors;

		numScrollColors = 0;
		scrollColors = Hunk_AllocateTempMemory( sizeof( stretchRect_t ) * cons[0].numLines );

		for( i = 0; i < cons[0].numLines; i++ )
		{
			int j = 0;
			float x = screenRight - CON_SCROLLBARWIDTH;
			float y = ((float)i / (float)cons[0].totallines) * SCREEN_HEIGHT * frac;
			float w = CON_SCROLLBARWIDTH;
			float h = (float)j / (float)cons[0].totallines * SCREEN_HEIGHT * frac;

			if( h * cls.glconfig.yscale < 1.0F )
				h = 1.0F / cls.glconfig.yscale + 0.000001F;

			SCR_AdjustFrom640( &x, &y, &w, &h );

			//Color the scroll bar based off the color of the text on that line.
			if( Q_IsColorString( cons[0].lines[ i ].text ) )
			{
				float * color = (float*)(g_color_table + ColorIndex( cons[0].lines[ i ].text[1] ));

				scrollColors[numScrollColors].pos[0] = x;
				scrollColors[numScrollColors].pos[1] = y;
				scrollColors[numScrollColors].size[0] = w;
				scrollColors[numScrollColors].size[1] = h;
				scrollColors[numScrollColors].tc0[0] = scrollColors[numScrollColors].tc0[1] = 0;
				scrollColors[numScrollColors].tc1[0] = scrollColors[numScrollColors].tc1[1] = 1;
				Com_Memcpy( scrollColors[numScrollColors].color, color, sizeof( vec4_t ) );

				numScrollColors++;
			}
		}

		re.DrawStretchPicMulti( scrollColors, numScrollColors, cls.whiteShader );

		Hunk_FreeTempMemory( scrollColors );
	}
#endif

	Con_Draw( 0, frac, qtrue, qfalse );

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput( (float)lines + 2.0f );
}

/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void )
{
	// if disconnected, render console full screen
	if( cls.state == CA_DISCONNECTED )
	{
		if( !(cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)) )
		{
			Con_Resize( 0, cons[0].currLine,
				-cls.glconfig.xbias / cls.glconfig.xscale, 0.0F,
				SCREEN_WIDTH + (cls.glconfig.xbias  / cls.glconfig.xscale) * 2.0F,
				SCREEN_HEIGHT - 20.0F );

			Con_DrawSolidConsole( 1.0F );
			return;
		}
	}
	else if( cons[0].h == SCREEN_HEIGHT - 20.0F )
	{
		Con_Resize( 0, cons[0].currLine, 0, 0, 0, 0 ); //force defaults
	}

	if( cons[0].displayFrac )
	{
		Con_DrawSolidConsole( cons[0].displayFrac );
	}
	else
	{
#if 1
		// draw notify lines
		if ( cls.state == CA_ACTIVE && cons[1].displayFrac>0.0f ) {
			vec4_t color = { 0.3f,0.3f,0.3f,0.75f };
			re.SetColor( color );
			cons[1].currLine = cons[1].numLines - cons[1].dispLines - 1;
			Con_Draw( 1, cons[1].displayFrac, qfalse, qtrue );
		}
#endif
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole (void) {

	int i;

	// decide on the destination height of the console
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
		cons[0].finalFrac = 1;
	else
		cons[0].finalFrac = 0;

	if ( (cls.realtime - cons[1].time) > con_notifytime->integer*1000 ) {
		cons[1].finalFrac = 0.0f;
	}
	
	for ( i=0; i<2; i++ ) {
		// scroll towards the destination height
		if (cons[i].finalFrac < cons[i].displayFrac)
		{
			cons[i].displayFrac -= con_conspeed->value*cls.realFrametime*0.002f;
			if (cons[i].finalFrac > cons[i].displayFrac)
				cons[i].displayFrac = cons[i].finalFrac;

		}
		else if (cons[i].finalFrac > cons[i].displayFrac)
		{
			cons[i].displayFrac += con_conspeed->value*cls.realFrametime*0.002f;
			if (cons[i].finalFrac < cons[i].displayFrac)
				cons[i].displayFrac = cons[i].finalFrac;
		}
	}
}


void Con_PageUp( void ) {
	Con_HandleKey( 0, K_PGUP );
}

void Con_PageDown( void ) {
	Con_HandleKey( 0, K_PGDN );
}

void Con_Top( void ) {
	Con_HandleKey( 0, K_HOME );
}

void Con_Bottom( void ) {
	Con_HandleKey( 0, K_END );
}


void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &g_consoleField );
	Con_ClearNotify ();
	cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	cons[ 0 ].finalFrac = 0;				// none visible
	cons[ 0 ].displayFrac = 0;
}
