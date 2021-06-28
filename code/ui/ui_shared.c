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
#include "ui_anim.h"

#ifdef _MSC_VER
#pragma warning( disable : 4100 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4702 )
#endif

// string allocation/managment

#define SCROLL_TIME_START					500
#define SCROLL_TIME_ADJUST					150
#define SCROLL_TIME_ADJUSTOFFSET			40
#define SCROLL_TIME_FLOOR					20

#define SCROLL_DEBUGMODE_MOVE		1
#define SCROLL_DEBUGMODE_LEFT		2
#define SCROLL_DEBUGMODE_RIGHT		4
#define SCROLL_DEBUGMODE_BOTTOM		8
#define SCROLL_DEBUGMODE_TOP		16


typedef struct scrollInfo_s
{
	int nextScrollTime;
	int nextAdjustTime;
	int adjustValue;
	int scrollKey;
	float xStart;
	float yStart;
	float offset;
	itemDef_t *item;
	qboolean scrollDir;
} scrollInfo_t;

static scrollInfo_t scrollInfo;

static void (*captureFunc)(void *p);
static void *captureData;
static itemDef_t *itemCapture;   // item that has the mouse captured ( if any )

displayContextDef_t *DC = NULL;

static qboolean g_waitingForKey = qfalse;

static itemDef_t *g_bindItem = NULL;

menuDef_t Menus[MAX_MENUS];      // defined menus
int menuCount = 0;               // how many

static menuStackDef_t * menuStack;

static qboolean debugMode = qfalse;

#ifdef DEVELOPER
static struct
{
	menuDef_t * menu;
	qboolean	dontsnap;
} debug;
#endif

int debug_cgmenu;

#define DOUBLE_CLICK_DELAY 300

void Item_RunScript(itemDef_t *item, const char *s);
void Menu_RunScript( menuDef_t *menu, const char *s );

int BindingIDFromName(const char *name);
qboolean Item_Bind_HandleKey(itemDef_t *item, int key );
itemDef_t *Menu_SetPrevCursorItem(menuDef_t *menu);
itemDef_t *Menu_SetNextCursorItem(menuDef_t *menu);
static void Menu_ClearFocusItem( menuDef_t * menu, itemDef_t * item );
static qboolean Menu_SetFocusItem( menuDef_t * menu, itemDef_t * item );
void Item_ListBox_UpdateColumns( itemDef_t * item );
extern void trap_R_GetFonts( char * buffer, int size );
void Item_ValidateTypeData(itemDef_t *item);
extern qboolean trap_Key_IsDown( int keynum );
extern void trap_Key_SetCatcher( int catcher );
extern int trap_Key_GetCatcher( void );
static void Menus_SetVisible();
static void Item_Slider_SetValue( itemDef_t * item, float value );



#ifdef CGAME
#define MEM_POOL_SIZE  128 * 1024
#else
#define MEM_POOL_SIZE  1024 * 1024
#endif

static char		memoryPool[MEM_POOL_SIZE];
static int		allocPoint, outOfMemory;


typedef enum {
	FIELD_INTEGER,
	FIELD_FLOAT,
	FIELD_STRING,
	FIELD_FONT,
	FIELD_SHADER,
	FIELD_COLOR,
	FIELD_BIT,
	FIELD_MODEL,
	FIELD_RECT,
	FIELD_KEY,
	FIELD_SCRIPT,
	FIELD_FUNC,
	FIELD_STATE,
	FIELD_ITEMDEF,

	FIELD_WINDOW,
	FIELD_ITEM,
	FIELD_MENU,
	FIELD_EDITFIELD,
	FIELD_LISTBOX,
	FIELD_PROGRESS,
	FIELD_COLUMNS,
	FIELD_EFFECT,

} fieldType_e;

#define FIELD_DONTSAVE		0x00000001
#define FIELD_DONTEDIT		0x00000002
#define FIELD_SECTION		0x00000004

typedef struct fieldEnum_s{
	const char *	shortname;
	const char *	name;
	float			value;
} fieldEnum_t;

typedef struct {
	char *	name;
	int		offset;
	int		bit;
	qboolean (*func)(itemDef_t *item, int handle);
	fieldType_e		type;
	int		flags;

	fieldEnum_t	fieldEnum[ MAX_MULTI_CVARS ];
	float	minVal;
	float	maxVal;
} field_t;


field_t * fields;
int fieldCount;

#define BIND_ALLOW_IN_UI 1

typedef struct {
	char	*command;
	int		id;
	int		defaultbind1;
	int		defaultbind2;
	int		bind1;
	int		bind2;
	int		flags;
} bind_t;

static bind_t g_bindings[] = 
{
	{"+button2",		 K_ENTER,			-1,		-1, -1},
	{"+speed", 			 K_SHIFT,			-1,		-1,	-1},
	{"+forward", 		 K_UPARROW,		-1,		-1, -1},
	{"+back", 			 K_DOWNARROW,	-1,		-1, -1},
	{"+moveleft", 	 ',',					-1,		-1, -1},
	{"+moveright", 	 '.',					-1,		-1, -1},
	{"+moveup",			 K_SPACE,			-1,		-1, -1},
	{"+movedown",		 'c',					-1,		-1, -1},
	{"+left", 			 K_LEFTARROW,	-1,		-1, -1},
	{"+right", 			 K_RIGHTARROW,	-1,		-1, -1},
	{"+strafe", 		 K_ALT,				-1,		-1, -1},
	{"+lookup", 		 K_PGDN,				-1,		-1, -1},
	{"+lookdown", 	 K_DEL,				-1,		-1, -1},
	{"+mlook", 			 '/',					-1,		-1, -1},
	{"centerview", 	 K_END,				-1,		-1, -1},
	{"+zoom", 			 -1,						-1,		-1, -1},
	{"weapon 1",		 '1',					-1,		-1, -1},
	{"weapon 2",		 '2',					-1,		-1, -1},
	{"weapon 3",		 '3',					-1,		-1, -1},
	{"weapon 4",		 '4',					-1,		-1, -1},
	{"weapon 5",		 '5',					-1,		-1, -1},
	{"weapon 6",		 '6',					-1,		-1, -1},
	{"weapon 7",		 '7',					-1,		-1, -1},
	{"weapon 8",		 '8',					-1,		-1, -1},
	{"weapon 9",		 '9',					-1,		-1, -1},
	{"weapon 10",		 '0',					-1,		-1, -1},
	{"weapon 11",		 -1,					-1,		-1, -1},
	{"weapon 12",		 -1,					-1,		-1, -1},
	{"weapon 13",		 -1,					-1,		-1, -1},
	{"+attack", 		 K_CTRL,				-1,		-1, -1},
	{"weapprev",		 '[',					-1,		-1, -1},
	{"weapnext", 		 ']',					-1,		-1, -1},
	{"+button2", 		 'g',				-1,		-1, -1},
	{"+button3", 		 K_MOUSE3,			-1,		-1, -1},
	{"+button4", 		 K_MOUSE4,			-1,		-1, -1},
	{"+button5", 		 K_MOUSE2,				-1,		-1, -1},
	{"scoresUp", K_KP_PGUP,			-1,		-1, -1},
	{"scoresDown", K_KP_PGDN,			-1,		-1, -1},
	{"reload", 'r',			-1,		-1, -1},
	{"mode", 'e',			-1,		-1, -1},
	{"st_ready", K_F3,			-1,		-1, -1, -1, BIND_ALLOW_IN_UI},
	// bk001205 - this one below was:  '-1' 
	{"messagemode",  't',					-1,		-1, -1, -1, BIND_ALLOW_IN_UI},
};


static qboolean isMouseKey( int key )
{
	return key == K_MOUSE1
		|| key == K_MOUSE2
		|| key == K_MOUSE3;
}

void UI_SetMenuStack( menuStackDef_t *stack )
{
	int i;
	if ( stack != menuStack ) {
		if ( menuStack ) {
			for ( i=0; i<menuStack->count; i++ ) {
				Menu_ClearFocusItem( menuStack->m[ i ], 0 );
			}
		}
		menuStack = stack;
	}
}

static itemDef_t *g_editItem = NULL;
static void UI_SetEditItem( itemDef_t *item )
{
	if( item == g_editItem )
		return;

	if( item )
	{
		if( item->cvar )
		{
			//have to test like this since CVAR_PASSWORD includes other bits (for some reason...)
			if( (item->cvarFlags & CVAR_PASSWORD) == CVAR_PASSWORD )
			{
				DC->setCVar( item->cvar, "" );
				item->cursorPos = 0;
			}
			else
			{
				char str[1024];
				DC->getCVarString( item->cvar, str, sizeof( str ) );

				item->cursorPos = strlen( str );
			}
		}
	}

	g_editItem = item;
}

/*
===============
UI_Alloc
===============
*/				  
void *UI_Alloc( int size ) {
	char	*p; 

	if ( allocPoint + size > MEM_POOL_SIZE ) {
		outOfMemory = qtrue;
		if (DC->Print) {
			DC->Print("UI_Alloc: Failure. Out of memory!\n");
		}
    //DC->trap_Print(S_COLOR_YELLOW"WARNING: UI Out of Memory!\n");
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 15 ) & ~15;

	return p;
}

/*
===============
UI_InitMemory
===============
*/
void UI_InitMemory( void ) {
	allocPoint = 0;
	outOfMemory = qfalse;
}

qboolean UI_OutOfMemory() {
	return outOfMemory;
}





#define HASH_TABLE_SIZE 2048
/*
================
return a hash value for the string
================
*/
static long hashForString(const char *str) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (str[i] != '\0') {
		letter = tolower(str[i]);
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (HASH_TABLE_SIZE-1);
	return hash;
}

typedef struct stringDef_s {
	struct stringDef_s *next;
	const char *str;
} stringDef_t;

static int strPoolIndex = 0;
static char strPool[STRING_POOL_SIZE];

static int strHandleCount = 0;
static stringDef_t *strHandle[HASH_TABLE_SIZE];


const char *String_Alloc(const char *p) {
	int len;
	long hash;
	stringDef_t *str, *last;
	static const char *staticNULL = "";

	if (p == NULL) {
		return NULL;
	}

	if (*p == 0) {
		return staticNULL;
	}

	hash = hashForString(p);

	str = strHandle[hash];
	while (str) {
		if (strcmp(p, str->str) == 0) {
			return str->str;
		}
		str = str->next;
	}

	len = strlen(p);
	if (len + strPoolIndex + 1 < STRING_POOL_SIZE) {
		int ph = strPoolIndex;
		strcpy(&strPool[strPoolIndex], p);
		strPoolIndex += len + 1;

		str = strHandle[hash];
		last = str;
		while (str && str->next) {
			last = str;
			str = str->next;
		}

		str  = UI_Alloc(sizeof(stringDef_t));
		str->next = NULL;
		str->str = &strPool[ph];
		if (last) {
			last->next = str;
		} else {
			strHandle[hash] = str;
		}
		return &strPool[ph];
	}
	return NULL;
}

void String_Report() {
	float f;
	Com_Printf("Memory/String Pool Info\n");
	Com_Printf("----------------\n");
	f = strPoolIndex;
	f /= STRING_POOL_SIZE;
	f *= 100;
	Com_Printf("String Pool is %.1f%% full, %i bytes out of %i used.\n", f, strPoolIndex, STRING_POOL_SIZE);
	f = allocPoint;
	f /= MEM_POOL_SIZE;
	f *= 100;
	Com_Printf("Memory Pool is %.1f%% full, %i bytes out of %i used.\n", f, allocPoint, MEM_POOL_SIZE);
}

/*
=================
String_Init
=================
*/
void String_Init() {
	int i;
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		strHandle[i] = 0;
	}
	strHandleCount = 0;
	strPoolIndex = 0;
	menuCount = 0;
	UI_InitMemory();
	if (DC && DC->getBindingBuf) {
		Controls_GetConfig();
	}
}

/*
=================
PC_SourceWarning
=================
*/
void PC_SourceWarning(int handle, char *format, ...) {
	int line;
	char filename[128];
	va_list argptr;
	char string[4096];

	va_start (argptr, format);
	vsnprintf (string, sizeof(string), format, argptr);
	va_end (argptr);

	filename[0] = '\0';
	line = 0;
	trap_PC_SourceFileAndLine(handle, filename, &line);

	Com_Printf(S_COLOR_YELLOW "WARNING: %s, line %d: %s\n", filename, line, string);
}

/*
=================
PC_SourceError
=================
*/
void PC_SourceError(int handle, char *format, ...) {
	int line;
	char filename[128];
	va_list argptr;
	char string[4096];

	va_start (argptr, format);
	vsnprintf (string, sizeof(string), format, argptr);
	va_end (argptr);

	filename[0] = '\0';
	line = 0;
	trap_PC_SourceFileAndLine(handle, filename, &line);

	Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);
}

/*
=================
LerpColor
=================
*/
void LerpColor(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int i;

	// lerp and clamp each component
	for (i=0; i<4; i++)
	{
		c[i] = a[i] + t*(b[i]-a[i]);
		if (c[i] < 0)
			c[i] = 0;
		else if (c[i] > 1.0f)
			c[i] = 1.0f;
	}
}

static void Item_Color_Pulse( vec4_t dst, vec4_t src )
{
	vec4_t		lowLight;
	lowLight[0] = 0.8f * src[0]; 
	lowLight[1] = 0.8f * src[1]; 
	lowLight[2] = 0.8f * src[2]; 
	lowLight[3] = 0.8f * src[3]; 
	LerpColor( src, lowLight, dst, 0.5f+0.5f*sinf( ((float)DC->realTime) / 100.0f ) );
}


/*
=================
Float_Parse
=================
*/
qboolean Float_Parse(const char **p, float *f) {
	char	*token;
	token = COM_ParseExt(p, qfalse);
	if (token && token[0] != 0) {
		*f = fatof(token);
		return qtrue;
	} else {
		return qfalse;
	}
}

/*
=================
PC_Float_Parse
=================
*/
qboolean PC_Float_Parse(int handle, float *f) {
	pc_token_t token;
	int negative = qfalse;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (token.string[0] == '-') {
		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;
		negative = qtrue;
	}
	if (token.type != TT_NUMBER) {
		PC_SourceError(handle, "expected float but found %s\n", token.string);
		return qfalse;
	}
	if (negative)
		*f = -token.floatvalue;
	else
		*f = token.floatvalue;

	if ( token.subtype & TT_HEX )
	{
		*f /= 255.0f;
	}

	return qtrue;
}

/*
=================
Color_Parse
=================
*/
qboolean Color_Parse(const char **p, vec4_t *c) {
	int i;
	float f;

	for (i = 0; i < 4; i++) {
		if (!Float_Parse(p, &f)) {
			return qfalse;
		}
		(*c)[i] = f;
	}
	return qtrue;
}

/*
=================
PC_Color_Parse
=================
*/
qboolean PC_Color_Parse(int handle, vec4_t *c) {
	int i;
	float f;

	for (i = 0; i < 4; i++) {
		if (!PC_Float_Parse(handle, &f)) {
			return qfalse;
		}
		(*c)[i] = f;
	}
	return qtrue;
}

/*
=================
Int_Parse
=================
*/
qboolean Int_Parse(const char **p, int *i) {
	char	*token;
	token = COM_ParseExt(p, qfalse);

	if (token && token[0] != 0) {
		*i = atoi(token);
		return qtrue;
	} else {
		return qfalse;
	}
}

/*
=================
PC_Int_Parse
=================
*/
qboolean PC_Int_Parse(int handle, int *i) {
	pc_token_t token;
	int negative = qfalse;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (token.string[0] == '-') {
		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;
		negative = qtrue;
	}
	if (token.type != TT_NUMBER) {
		PC_SourceError(handle, "expected integer but found %s\n", token.string);
		return qfalse;
	}
	*i = token.intvalue;
	if (negative)
		*i = - *i;
	return qtrue;
}

/*
=================
Rect_Parse
=================
*/
qboolean Rect_Parse(const char **p, rectDef_t *r) {
	if (Float_Parse(p, &r->x)) {
		if (Float_Parse(p, &r->y)) {
			if (Float_Parse(p, &r->w)) {
				if (Float_Parse(p, &r->h)) {
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/*
=================
PC_Rect_Parse
=================
*/
qboolean PC_Rect_Parse(int handle, rectDef_t *r) {
	if (PC_Float_Parse(handle, &r->x)) {
		if (PC_Float_Parse(handle, &r->y)) {
			if (PC_Float_Parse(handle, &r->w)) {
				if (PC_Float_Parse(handle, &r->h)) {
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/*
=================
String_Parse
=================
*/
qboolean String_Parse(const char **p, const char **out) {
	char *token;

	token = COM_ParseExt(p, qfalse);
	if (token && token[0] != 0) {
		*(out) = String_Alloc(token);
		return qtrue;
	}
	return qfalse;
}

/*
=================
PC_String_Parse
=================
*/
qboolean PC_String_Parse(int handle, const char **out) {
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;

	*(out) = String_Alloc(token.string);

    return qtrue;
}

#define MAX_SCRIPT_LEN	2048

/*
=================
PC_Script_Parse
=================
*/
qboolean PC_Script_Parse(int handle, const char **out) {
	char script[MAX_SCRIPT_LEN];
	pc_token_t token;

	memset(script, 0, sizeof(script));
	// scripts start with { and have ; separated command lists.. commands are command, arg.. 
	// basically we want everything between the { } as it will be interpreted at run time
  
	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
	    return qfalse;
	}

	for( ; ; )
	{
		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			*out = String_Alloc(script);
			return qtrue;
		}

		if (token.string[1] != '\0') {
			Q_strcat(script, sizeof( script ), va("\"%s\"", token.string));
		} else {
			Q_strcat(script, sizeof( script ), token.string);
		}
		Q_strcat(script, sizeof( script ), " ");
	}
}

// display, window, menu, item code
// 

/*
==================
Init_Display

Initializes the display with a structure to all the drawing routines
 ==================
*/
void Init_Display( displayContextDef_t *dc )
{
	DC = dc;
}

/*
==================
Window_Init

Initializes a window structure ( windowDef_t ) with defaults
 
==================
*/
void Window_Init(Window *w) {
	memset(w, 0, sizeof(windowDef_t));
	w->borderSize = 1;
	w->foreColor[0] = w->foreColor[1] = w->foreColor[2] = w->foreColor[3] = 1.0;
	w->cinematic = -1;
}

void Fade(int *flags, float *f, float clamp, int *nextTime, int offsetTime, qboolean bFlags, float fadeAmount) {
  if (*flags & (WINDOW_FADINGOUT | WINDOW_FADINGIN)) {
    if (DC->realTime > *nextTime) {
      *nextTime = DC->realTime + offsetTime;
      if (*flags & WINDOW_FADINGOUT) {
        *f -= fadeAmount;
        if (bFlags && *f <= 0.0f) {
		  *f = 0.0f;
          *flags &= ~(WINDOW_FADINGOUT | WINDOW_VISIBLE);
        }
      } else {
        *f += fadeAmount;
        if (*f >= clamp) {
          *f = clamp;
          if (bFlags) {
            *flags &= ~WINDOW_FADINGIN;
          }
        }
      }
    }
  }
}

void border( rectDef_t * r, float size, const float *color, qhandle_t shader ) {
	float	rx,ry;
	float	cw,ch;
	float	x,y,w,h;

	x = (r->x-(size*0.5f))*DC->glconfig.xscale + DC->glconfig.xbias;
	y = (r->y-(size*0.5f))*DC->glconfig.yscale;
	w = (r->w+size)*DC->glconfig.xscale;
	h = (r->h+size)*DC->glconfig.yscale;

	rx = size * DC->glconfig.xscale;
	ry = size * DC->glconfig.yscale;

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
		DC->setColor( color );

	cw = w-rx*2.0f;
	ch = h-ry*2.0f;

	// corners
	DC->drawStretchPic( x,		y,		rx+cw,	ry,		 0.0f, 0.0f, (rx+cw)/rx,	1.0f,			shader );

	DC->drawStretchPic( x,		y+ry,	rx,		ry+ch,	 0.0f, (ry+ch)/ry, 1.0f,	0.0f,			shader );

	DC->drawStretchPic( x+w-rx,	y,		rx,		ry+ch,	 1.0f, 0.0f, 0.0f,		(ry+ch)/ry,		shader );
	DC->drawStretchPic( x+rx,	y+h-ry,	rx+cw,	ry,		 (rx+cw)/rx, 1.0f, 0.0f,		0.0f,			shader );
}


void Window_Paint( Window *w, int focus, int focusTime )
{
	float fade = 0.0f;
	float width;

	if( !w )
		return;

	width = (w->flags & WINDOW_HASSCROLLBAR)?w->rect.w - (SCROLLBAR_SIZE+4.0f):w->rect.w;

	//	fade hi-lighter in and out
	if ( !(w->flags & (WINDOW_NOFOCUS|WINDOW_DECORATION)) )
	{
		fade = min( 1.0f, (DC->realTime - focusTime)* ( 1.0f / (float)ITEM_FADE_TIME ) );
		if ( !focus )
			fade = 1.0f - fade;
	}

	LerpColor( w->backColor, w->outlineColor, w->color, fade );

	if ( w->flags&WINDOW_DIMBACKGROUND ) {
		const vec4_t backColor = { 0x15/255.0f, 0x27/255.0f, 0x59/255.0f, 0.7f };
		float x = DC->glconfig.xbias / DC->glconfig.xscale;
		DC->setColor( backColor );
		DC->drawHandlePic( -x,0.0f,640.0f + x*2.0f,480.0f,DC->whiteShader );
	}

	switch( w->style )
	{
	case WINDOW_STYLE_EMPTY:
		if ( fade > 0.0f )
		{
			vec4_t color;
			color[ 0 ] = w->outlineColor[ 0 ];
			color[ 1 ] = w->outlineColor[ 1 ];
			color[ 2 ] = w->outlineColor[ 2 ];
			color[ 3 ] = w->outlineColor[ 3 ] * fade;
			if ( color[ 3 ] > 0.0f ) {
				DC->fillRect( w->rect.x, w->rect.y, width, w->rect.h, color, (w->background)?w->background:DC->Assets.menu );
			}
		}
		break;

	case WINDOW_STYLE_FILLED:
		{
			DC->fillRect( w->rect.x, w->rect.y, width, w->rect.h, w->color, w->background );
		} break;

	case WINDOW_STYLE_SHADER:
		{
			float b = 0.0f;
			
			if( w->flags & WINDOW_HASBORDER )
				b = w->borderSize*0.5f;

			if (w->flags & (WINDOW_FADINGOUT | WINDOW_FADINGIN)) {
				Fade( &w->flags, &w->backColor[3], DC->Assets.fadeClamp, &w->nextTime, DC->Assets.fadeCycle, qtrue, DC->Assets.fadeAmount );
				DC->setColor( w->backColor );
			} else {
				DC->setColor( w->color );
			}

			DC->drawHandlePic( w->rect.x+b, w->rect.y+b, width-b*2.0f, w->rect.h-b*2.0f, w->background );
			DC->setColor( NULL );
		} break;

	case WINDOW_STYLE_CINEMATIC:
		if( w->cinematic == -1 )
		{
			w->cinematic = DC->playCinematic( w->cinematicName, w->rect.x, w->rect.y, width, w->rect.h );
			if( w->cinematic == -1 )
				w->cinematic = -2;
		} 
		
		if( w->cinematic >= 0 )
		{
			DC->runCinematicFrame( w->cinematic );
			DC->drawCinematic( w->cinematic, w->rect.x, w->rect.y, width, w->rect.h );
		}
		
		break;
	}

	if( w->flags & WINDOW_HASBORDER )
		border( &w->rect, w->borderSize, w->borderColor, DC->Assets.menu );
}


void Item_SetScreenCoords(itemDef_t *item, rectDef_t * r ) {

	menuDef_t * m;

	if ( !item )
		return;

	m = (menuDef_t*)item->parent;

	item->window.rect = item->window.rectClient;
	item->window.rect.y += r->y;

	if ( item->window.flags & WINDOW_ALIGNWIDTH ) {
		if ( m->window.flags & WINDOW_FULLSCREEN ) {
			item->window.rect.x = -uiInfo.uiDC.glconfig.xbias;
			item->window.rect.w = 640.0f + uiInfo.uiDC.glconfig.xbias*2.0f;
		} else {
			item->window.rect.x = r->x;
			item->window.rect.w = r->w;
		}
		
	} else if ( item->window.flags & WINDOW_ALIGNRIGHT ) {
		
		float offset = m->window.rectClient.w - (item->window.rect.x + item->window.rect.w);
		item->window.rect.x = r->x + r->w - (item->window.rect.w + offset);
	} else {
		item->window.rect.x += r->x;
	}

	if ( item->snapleft ) {
		item->window.rect.x = (item->snapleft->window.rect.x + item->snapleft->window.rect.w) - (0.5f/DC->glconfig.xscale);
	}
	if ( item->snapright ) {
		item->window.rect.w = item->snapright->window.rect.x - item->window.rect.x + (0.5f/DC->glconfig.xscale);
	}

	// force the text rects to recompute
	item->textRect.w = 0;
	item->textRect.h = 0;

	Item_ListBox_UpdateColumns( item );
}

// FIXME: consolidate this with nearby stuff
void Item_UpdatePosition(itemDef_t *item) {
  menuDef_t *menu;
  
  if (item == NULL || item->parent == NULL) {
    return;
  }

  menu = item->parent;

  Item_SetScreenCoords(item, &menu->window.rect );

}

// menus
void Menu_UpdatePosition(menuDef_t *menu) {
  int i;

  if (menu == NULL) {
    return;
  }
  
  for (i = 0; i < menu->itemCount; i++) {
    Item_SetScreenCoords(menu->items[i], &menu->window.rect);
  }
}

void Menu_PostParse(menuDef_t *menu) {
	int i;
	float x = DC->glconfig.xbias / DC->glconfig.xscale;

	if (menu == NULL) {
		return;
	}

	menu->window.rect = menu->window.rectClient;

	if (menu->window.flags & WINDOW_FULLSCREEN) {
		menu->window.rect.x = 0.0f;
		menu->window.rect.y = 0.0f;
		menu->window.rect.w = 640.0f;
		menu->window.rect.h = 480.0f;
	}
	
	if (menu->window.flags & WINDOW_ALIGNLEFT ) {
		menu->window.rect.x -= x;
	} else if (menu->window.flags & WINDOW_ALIGNRIGHT ) {
		menu->window.rect.x += x;
	} else if (menu->window.flags & WINDOW_ALIGNWIDTH ) {
		float right = x + (menu->window.rect.x + menu->window.rect.w);
		menu->window.rect.x -= x;
		menu->window.rect.w = right - menu->window.rect.x;
	}

	Menu_UpdatePosition(menu);

	//	stretch full screen windows to fill widescreen
	if ( menu->window.flags & WINDOW_FULLSCREEN )
	{
		menu->window.rect.x = -x;
		menu->window.rect.w = 640.0f + (x*2.0f);
	}

	if ( !menu->font )
		menu->font = DC->Assets.font;

	if ( !menu->window.background && menu->window.style == WINDOW_STYLE_FILLED )
		menu->window.background = DC->Assets.menu;

	trap_SQL_Prepare( "UPDATE OR INSERT feeders SET name=$ SEARCH name $;" );

	for ( i=0; i<menu->itemCount; i++ )
	{
		itemDef_t * item = menu->items[ i ];

		if ( item->text ) {
			sql( "SELECT index FROM strings SEARCH name $;", "tsId", item->text, &item->ownerText );
		}

		if ( !item->font )
			item->font = menu->font;

		if ( !item->window.background && item->window.style == WINDOW_STYLE_FILLED )
			item->window.background = DC->Assets.menu;

		if ( item->window.ownerDraw )
			item->type = ITEM_TYPE_OWNERDRAW;

		if ( (item->type == ITEM_TYPE_LISTBOX || item->type == ITEM_TYPE_SLIDER || item->window.ownerDraw == UI_TRAVELSCREEN) && item->window.name )
		{
			trap_SQL_BindText( 1, item->window.name );
			trap_SQL_Step();
		}

		if ( item->type == ITEM_TYPE_LISTBOX ) {
			int j;
			listBoxDef_t * listBox = (listBoxDef_t*)item->typeData;
			for ( j=0; j<listBox->numColumns; j++ ) {
				if ( listBox->columnInfo[ j ].text ) {
					sql( "SELECT index FROM strings SEARCH name $", "tsId", listBox->columnInfo[ j ].text, &listBox->columnInfo[ j ].ownerText );
				}
			}
			
		}

		//	stretch full screen windows to fill widescreen
		if ( item->window.flags & WINDOW_FULLSCREEN )
		{
			float x = DC->glconfig.xbias / DC->glconfig.xscale;
			item->window.rect.x = -x;
			item->window.rect.y = 0.0f;
			item->window.rect.w = 640.0f + x*2.0f;
			item->window.rect.h = 480.0f;
		}
	}

	trap_SQL_Done();

	menu->window.flags &= ~WINDOW_VISIBLE;

}

qboolean Item_HasFocus( itemDef_t * item )
{
	return ((menuDef_t*)item->parent)->focusItem == item;
}


qboolean IsVisible(int flags) {
  return (flags & WINDOW_VISIBLE && !(flags & WINDOW_FADINGOUT));
}

qboolean Rect_ContainsPoint(const rectDef_t *rect, float x, float y) {
  if (rect) {
    if (x > rect->x && x < rect->x + rect->w && y > rect->y && y < rect->y + rect->h) {
      return qtrue;
    }
  }
  return qfalse;
}

int Menu_ItemsMatchingGroup(menuDef_t *menu, const char *name) {
  int i;
  int count = 0;
  for (i = 0; i < menu->itemCount; i++) {
    if (Q_stricmp(menu->items[i]->window.name, name) == 0 || (menu->items[i]->window.group && Q_stricmp(menu->items[i]->window.group, name) == 0)) {
      count++;
    } 
  }
  return count;
}

itemDef_t *Menu_GetMatchingItemByNumber(menuDef_t *menu, int index, const char *name) {
  int i;
  int count = 0;
  for (i = 0; i < menu->itemCount; i++) {
    if (Q_stricmp(menu->items[i]->window.name, name) == 0 || (menu->items[i]->window.group && Q_stricmp(menu->items[i]->window.group, name) == 0)) {
      if (count == index) {
        return menu->items[i];
      }
      count++;
    } 
  }
  return NULL;
}



void Script_SetColor(itemDef_t *item, const char **args) {
  const char *name;
  int i;
  float f;
  vec4_t *out;
  // expecting type of color to set and 4 args for the color
  if (String_Parse(args, &name)) {
      out = NULL;
      if (Q_stricmp(name, "backcolor") == 0) {
        out = &item->window.backColor;
        item->window.flags |= WINDOW_BACKCOLORSET;
      } else if (Q_stricmp(name, "forecolor") == 0) {
        out = &item->window.foreColor;
        item->window.flags |= WINDOW_FORECOLORSET;
      } else if (Q_stricmp(name, "bordercolor") == 0) {
        out = &item->window.borderColor;
      }

      if (out) {
        for (i = 0; i < 4; i++) {
          if (!Float_Parse(args, &f)) {
            return;
          }
          (*out)[i] = f;
        }
      }
  }
}

void Script_SetAsset(itemDef_t *item, const char **args) {
  const char *name;
  // expecting name to set asset to
  if (String_Parse(args, &name)) {
    // check for a model 
    if (item->type == ITEM_TYPE_MODEL) {
    }
  }
}

void Script_SetBackground(itemDef_t *item, const char **args) {
  const char *name;
  // expecting name to set asset to
  if (String_Parse(args, &name)) {
    item->window.background = DC->registerShaderNoMip(name);
  }
}

itemDef_t *Menu_FindItemByName(menuDef_t *menu, const char *p) {
  int i;
  if (menu == NULL || p == NULL) {
    return NULL;
  }

  for (i = 0; i < menu->itemCount; i++) {
    if (Q_stricmp(p, menu->items[i]->window.name) == 0) {
      return menu->items[i];
    }
  }

  return NULL;
}

void Script_SetItemColor(itemDef_t *item, const char **args) {
  const char *itemname;
  const char *name;
  vec4_t color;
  int i;
  vec4_t *out;
  // expecting type of color to set and 4 args for the color
  if (String_Parse(args, &itemname) && String_Parse(args, &name)) {
    itemDef_t *item2;
    int j;
    int count = Menu_ItemsMatchingGroup(item->parent, itemname);

    if (!Color_Parse(args, &color)) {
      return;
    }

    for (j = 0; j < count; j++) {
      item2 = Menu_GetMatchingItemByNumber(item->parent, j, itemname);
      if (item2 != NULL) {
        out = NULL;
        if (Q_stricmp(name, "backcolor") == 0) {
          out = &item2->window.backColor;
        } else if (Q_stricmp(name, "forecolor") == 0) {
          out = &item2->window.foreColor;
          item2->window.flags |= WINDOW_FORECOLORSET;
        } else if (Q_stricmp(name, "bordercolor") == 0) {
          out = &item2->window.borderColor;
        }

        if (out) {
          for (i = 0; i < 4; i++) {
            (*out)[i] = color[i];
          }
        }
      }
    }
  }
}

void Menu_ShowItem( itemDef_t *item, qboolean bShow )
{
	if( bShow )
	{
		item->window.flags |= WINDOW_VISIBLE;
	}
	else
	{
		item->window.flags &= ~WINDOW_VISIBLE;
		// stop cinematics playing in the window
		if( item->window.cinematic >= 0 )
		{
			DC->stopCinematic( item->window.cinematic );
			item->window.cinematic = -1;
		}
	}
}

void Menu_ShowItemByName( menuDef_t *menu, const char *p, qboolean bShow )
{
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup( menu, p );
	for( i = 0; i < count; i++ )
	{
		item = Menu_GetMatchingItemByNumber( menu, i, p );
		if( item != NULL )
			Menu_ShowItem( item, bShow );
	}
}

void Menu_FadeItemByName(menuDef_t *menu, const char *p, qboolean fadeOut) {
  itemDef_t *item;
  int i;
  int count = Menu_ItemsMatchingGroup(menu, p);
  for (i = 0; i < count; i++) {
    item = Menu_GetMatchingItemByNumber(menu, i, p);
    if (item != NULL) {
      if (fadeOut) {
        item->window.flags |= (WINDOW_FADINGOUT | WINDOW_VISIBLE);
        item->window.flags &= ~WINDOW_FADINGIN;
      } else {
        item->window.flags |= (WINDOW_VISIBLE | WINDOW_FADINGIN);
        item->window.flags &= ~WINDOW_FADINGOUT;
      }
    }
  }
}

menuDef_t *Menus_FindByName(const char *p) {
  int i;
  for (i = 0; i < menuCount; i++) {
    if (Q_stricmp(Menus[i].window.name, p) == 0) {
      return &Menus[i];
    } 
  }
  return NULL;
}

void Menus_ShowByName(const char *p) {
	Menus_ActivateByName(p);
}

void Menus_OpenByName(const char *p) {
  Menus_ActivateByName(p);
}

void Menus_FadeAllOutAndFadeIn( const char * p ) {
	if ( p ) {
		menuStack->openQueue[ menuStack->openQueueCount++ ] = Menus_FindByName( p );
	}
	menuStack->openTime = DC->realTime;
	trap_Key_SetCatcher( KEYCATCH_UI );
}

static void Menu_RunCloseScript(menuDef_t *menu) {
	if( menu && menu->onClose )
		Menu_RunScript( menu, menu->onClose );
}

void Menus_Close( menuDef_t *menu )
{
	if ( menu )
	{
		int i,n;
		for ( n=0; n<menuStack->count; n++ )
		{
			if ( menuStack->m[ n ] == menu )
				break;
		}

		if ( n == menuStack->count )
			return;	// didn't find menu

		for ( i=n; i<menuStack->count; i++ )
		{
			menu = menuStack->m[ i ];
			Menu_RunCloseScript(menu);
			Menu_ClearFocusItem( menu, 0 );
		}

		menuStack->count = n;

		Menus_SetVisible();

		if ( menu->window.flags & WINDOW_PAUSEGAME ) {
			trap_Cvar_Set( "cl_paused", "0" );
		}

		// focus goes to the menu on top of the stack
		if ( menuStack->count > 0 )
		{
			menu = menuStack->m[ menuStack->count - 1 ];
			if ( menu->onFocus )
				Menu_RunScript( menu, menu->onFocus );
		} else
		{
#ifndef CGAME
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
#endif
		}
	}
}

void Menus_CloseByName( const char *p )
{
	menuDef_t *menu = Menus_FindByName(p);
	if ( menu )
		Menus_Close( menu );
}


void Menus_CloseAll()
{
	int i;
	for ( i=menuStack->count-1; i>=0; i-- )
	{
		menuDef_t * menu = menuStack->m[ i ];
		Menu_RunCloseScript( menu );
		menu->window.flags &= ~WINDOW_VISIBLE;
	}

	menuStack->count = 0;

#ifndef CGAME
	trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
#endif

}


void Script_Show( itemDef_t *item, const char **args )
{
	const char *name;
	if( String_Parse( args, &name ) )
	{
		if( Q_stricmp( name, "this" ) == 0 )
			Menu_ShowItem( item, qtrue );
		else
			Menu_ShowItemByName( item->parent, name, qtrue );
	}
}

void Script_Hide(itemDef_t *item, const char **args) {
	const char *name;
	if( String_Parse( args, &name ) )
	{
		if( Q_stricmp( name, "this" ) == 0 )
			Menu_ShowItem( item, qfalse );
		else
			Menu_ShowItemByName( item->parent, name, qfalse );
	}
}

void Script_FadeIn(itemDef_t *item, const char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    Menu_FadeItemByName(item->parent, name, qfalse);
  }
}

void Script_FadeOut(itemDef_t *item, const char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    Menu_FadeItemByName(item->parent, name, qtrue);
  }
}



void Script_Open(itemDef_t *item, const char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    Menus_OpenByName(name);
  }
}

void Script_ConditionalOpen(itemDef_t *item, const char **args) {
	const char *cvar;
	const char *name1;
	const char *name2;
	float           val;

	if ( String_Parse(args, &cvar) && String_Parse(args, &name1) && String_Parse(args, &name2) ) {
		val = DC->getCVarValue( cvar );
		if ( val == 0.f ) {
			Menus_OpenByName(name2);
		} else {
			Menus_OpenByName(name1);
		}
	}
}

void Script_Close(itemDef_t *item, const char **args) {
	const char *name;
	if ( String_Parse(args, &name) )
	{
		if ( !Q_stricmp( name, "all" ) )
			Menus_CloseAll();
		else
			Menus_CloseByName(name);
	}
}

void Menu_OrbitItemByName(menuDef_t *menu, const char *p, float x, float y, float cx, float cy, int time) {
  itemDef_t *item;
  int i;
  int count = Menu_ItemsMatchingGroup(menu, p);
  for (i = 0; i < count; i++) {
    item = Menu_GetMatchingItemByNumber(menu, i, p);
    if (item != NULL) {
      item->window.flags |= (WINDOW_ORBITING | WINDOW_VISIBLE);
      item->window.offsetTime = time;
      item->window.rectEffects.x = cx;
      item->window.rectEffects.y = cy;
      item->window.rectClient.x = x;
      item->window.rectClient.y = y;
      Item_UpdatePosition(item);
    }
  }
}

void Script_Orbit(itemDef_t *item, const char **args) {
  const char *name;
  float cx, cy, x, y;
  int time;

  if (String_Parse(args, &name)) {
    if ( Float_Parse(args, &x) && Float_Parse(args, &y) && Float_Parse(args, &cx) && Float_Parse(args, &cy) && Int_Parse(args, &time) ) {
      Menu_OrbitItemByName(item->parent, name, x, y, cx, cy, time);
    }
  }
}

void Script_SetFocus( itemDef_t *item, const char **args )
{
	const char*	name;
	itemDef_t*	focusItem;

	if( String_Parse( args, &name ) )
	{
		if( Q_stricmp( name, "this" ) == 0 )
			focusItem = item;
		else
			focusItem = Menu_FindItemByName( item->parent, name );

		Menu_SetFocusItem( item->parent, focusItem );
	}
}

void Script_SetPlayerModel(itemDef_t *item, const char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    DC->setCVar("model", name);
  }
}

void Script_SetPlayerHead(itemDef_t *item, const char **args) {
  const char *name;
  if (String_Parse(args, &name)) {
    DC->setCVar("headmodel", name);
  }
}

void Script_SetCvar(itemDef_t *item, const char **args) {
	const char *cvar, *val;
	if (String_Parse(args, &cvar) && String_Parse(args, &val)) {
		DC->setCVar(cvar, val);
	}
	
}

void Script_Exec(itemDef_t *item, const char **args) {
	const char *val;
	if (String_Parse(args, &val)) {
		DC->executeText(EXEC_APPEND, va("%s ; ", val));
	}
}

static void Script_Sql(itemDef_t * item, const char **args) {
	const char *val;
	if (String_Parse(args, &val)) {
		trap_SQL_Exec(val);
	}
}

static void Script_Set(itemDef_t * item, const char **args) {
	const char *cvar;
	const char *sql;
	if (String_Parse(args, &cvar)) {
		if (String_Parse(args, &sql)) {
			trap_SQL_Prepare( sql );
			if ( trap_SQL_Step() ) {
				char t[ MAX_INFO_STRING ];
				trap_SQL_ColumnAsText( t, sizeof(t), 0 );
				DC->setCVar( cvar, t );
			}
			trap_SQL_Done();
		}
	}
}

void Script_Play(itemDef_t *item, const char **args) {
	const char *val;
	if (String_Parse(args, &val)) {
		DC->startLocalSound(DC->registerSound(val, qfalse), CHAN_LOCAL_SOUND);
	}
}

void Script_playLooped(itemDef_t *item, const char **args) {
	const char *val;
	if (String_Parse(args, &val)) {
		DC->stopBackgroundTrack();
		DC->startBackgroundTrack(val, val);
	}
}

void Script_EditField( itemDef_t * item, const char ** args )
{
	const char *ctrl;

	if( !String_Parse( args, &ctrl ) )
		return;

	if( Q_stricmp( ctrl, "this" ) != 0 )
		item = Menu_FindItemByName( item->parent, ctrl );

	if ( item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD )
	{
		UI_SetEditItem( item );
	}
}

void Script_EditClear( itemDef_t * item, const char ** args )
{
	const char *ctrl;

	if( !String_Parse( args, &ctrl ) )
		return;

	if( Q_stricmp( ctrl, "this" ) != 0 )
		item = Menu_FindItemByName( item->parent, ctrl );

	if ( item->type == ITEM_TYPE_EDITFIELD )
	{
		editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;

		DC->setCVar( item->cvar, "" );
		item->cursorPos = 0;
		editPtr->paintOffset = 0;
	}
}


void Item_RunScript( itemDef_t *, const char * );
void Script_RunHandler( itemDef_t *item, const char **args )
{
	const char *ctrl, *script;
	itemDef_t *targetItem;

	if ( !item )
		return;

	if( !String_Parse( args, &ctrl ) )
		return;
	if( !String_Parse( args, &script ) )
		return;

	if( Q_stricmp( ctrl, "null" ) == 0 )
		targetItem = item;
	else if( Q_stricmp( ctrl, "this" ) == 0 )
		targetItem = item;
	else
		targetItem = Menu_FindItemByName( item->parent, ctrl );

	if( Q_stricmp( script, "action" ) == 0 )
	{
		if( targetItem->action )
			Item_RunScript( targetItem, targetItem->action );
	}
	else if( Q_stricmp( script, "hotkey" ) == 0 )
	{
		int i, key;
		if( !Int_Parse( args, &key ) )
			return;

		for( i = 0; i < MAX_KEY_ACTIONS; i++ )
			if( targetItem->onHotKey[ i ].key == key )
			{
				Item_RunScript( targetItem, targetItem->onHotKey[ i ].action );
				break;
			}
	}
	else if( Q_stricmp( script, "focuskey" ) == 0 )
	{
		int i, key;
		if( !Int_Parse( args, &key ) )
			return;

		for( i = 0; i < MAX_KEY_ACTIONS; i++ )
			if( targetItem->onFocusKey[ i ].key == key )
			{
				Item_RunScript( targetItem, targetItem->onFocusKey[ i ].action );
				break;
			}
	}
	else if( Q_stricmp( script, "menukey" ) == 0 )
	{
		int i, key;
		menuDef_t *menu;

		if( !Int_Parse( args, &key ) )
			return;

		menu = item->parent;

		for( i = 0; i < MAX_KEY_ACTIONS; i++ )
			if( menu->onMenuKey[ i ].key == key )
			{
				Menu_RunScript( menu, menu->onMenuKey[ i ].action );
				break;
			}
	}
}

void Script_SetItemBackground( itemDef_t *item, const char **args )
{
	const char *itemName, *shaderName;

	if( !String_Parse( args, &itemName ) )
		return;
	if( !String_Parse( args, &shaderName ) )
		return;

	item = Menu_FindItemByName( item->parent, itemName );
	if( !item )
		return;

	item->window.background = DC->registerShaderNoMip( shaderName );
}

static const commandDef_t commandList[] =
{
	{ "fadein", &Script_FadeIn },					// group/name
	{ "fadeout", &Script_FadeOut },					// group/name
	{ "show", &Script_Show },						// group/name
	{ "hide", &Script_Hide },						// group/name
	{ "setcolor", &Script_SetColor },				// works on this
	{ "open", &Script_Open },						// menu
	{ "conditionalopen", &Script_ConditionalOpen },	// menu
	{ "close", &Script_Close },						// menu
	{ "setasset", &Script_SetAsset },				// works on this
	{ "setbackground", &Script_SetBackground },		// works on this
	{ "setitembackground", &Script_SetItemBackground },
	{ "setitemcolor", &Script_SetItemColor },		// group/name
	{ "setfocus", &Script_SetFocus },				// sets this background color to team color
	{ "setplayermodel", &Script_SetPlayerModel },	// sets this background color to team color
	{ "setplayerhead", &Script_SetPlayerHead },		// sets this background color to team color
	{ "setcvar", &Script_SetCvar },					// group/name
	{ "exec", &Script_Exec },						// group/name
	{ "sql", &Script_Sql },
	{ "set", &Script_Set },
	{ "play", &Script_Play },						// group/name
	{ "playlooped", &Script_playLooped },			// group/name
	{ "orbit", &Script_Orbit },						// group/name
	{ "runhandler", &Script_RunHandler },
	{ "editfield", &Script_EditField },
	{ "editclear", &Script_EditClear }
};

static const int scriptCommandCount = sizeof(commandList) / sizeof(commandDef_t);

typedef struct
{
	itemDef_t *item;
	int runTime;
	qboolean isParentHack;
	char script[MAX_SCRIPT_LEN - 8];	//script is max MAX_SCRIPT_LEN,
										//less 5 chars for 'sleep', less a space,
										//less at least one digit of timeout, less a semicolon
} delayScript_t;

#define DELAY_SCRIPT_COUNT 3
static delayScript_t delayScriptList[DELAY_SCRIPT_COUNT] = { {0},{0},{0} };

//horrible, HORRIBLE, hackery to get around cases where a menuDef_t is "cast" into an itemDef_t...sort of - PD
static qboolean g_Item_RunScriptParentHack = qfalse;
static qboolean Item_AddDelayedScript( itemDef_t *item, int delay, qboolean isParentHack, const char *s )
{
	int i, l;
	for( i = 0; i < DELAY_SCRIPT_COUNT; i++ )
		if( delayScriptList[ i ].item == 0 )
		{
			delayScriptList[ i ].item = (itemDef_t*)item->parent;
			delayScriptList[ i ].runTime = DC->realTime + delay;
			delayScriptList[ i ].isParentHack = isParentHack;
		
			l = strlen( s );
			memcpy( delayScriptList[ i ].script, s, l );
			delayScriptList[ i ].script[ l ] = '\0';

			return qtrue;
		}

	return qfalse;
}

static void Item_RunDelayedScripts()
{
	int i;

	for( i = 0; i < DELAY_SCRIPT_COUNT; i++ )
		if( delayScriptList[ i ].item && delayScriptList[ i ].runTime <= DC->realTime )
		{
			itemDef_t hackItem;
			itemDef_t *item = delayScriptList[ i ].item;
			qboolean saveIsHack = qfalse;
			delayScriptList[ i ].item = 0;

			if( delayScriptList[ i ].isParentHack )
			{
				hackItem.parent = item;
				item = &hackItem;

				saveIsHack = g_Item_RunScriptParentHack;
				g_Item_RunScriptParentHack = qtrue;
			}
            
			Item_RunScript( item, delayScriptList[ i ].script );

			if( delayScriptList[i].isParentHack )
				g_Item_RunScriptParentHack = saveIsHack;
		}
}

void Item_RunScript( itemDef_t *item, const char * s )
{
	const char * p = s;
	int i;
	qboolean bRan;

	if ( debugMode )
		return;

	
	if( item && s && s[0] )
	{
		for( ; ; )
		{
			const char *command;
			// expect command then arguments, ; ends command, NULL ends script
			if( !String_Parse( &p, &command ) )
			{
				return;
			}

			if( command[0] == ';' && command[1] == '\0' )
			{
				continue;
			}

			if( Q_stricmp( command, "sleep" ) == 0 )
			{
				int delay;
				const char *tmp;
                const char *s;
                
				if( !String_Parse( &p, &s ) )
					return;

				delay = atoi( s );

				//do a little dance to skip the following ;
				tmp = p;
				if( !String_Parse( &tmp, &s ) )
					return;

				if( s[0] == ';' )
					p = tmp;
				else if( s[0] == '\0' )
					return;

				Item_AddDelayedScript( item, delay, g_Item_RunScriptParentHack, p );

				//stop processing this, the rest of the script will run on a refresh
				return;
			}

			bRan = qfalse;
			for( i = 0; i < scriptCommandCount; i++ )
			{
				if ( Q_stricmp( command, commandList[i].name ) == 0 )
				{
					commandList[i].handler( item, &p );
					bRan = qtrue;
					break;
				}
			}
			// not in our auto list, pass to handler
			if( !bRan )
			{
				DC->runScript( &p );
			}
		}
	}
}

void Menu_RunScript( menuDef_t *menu, const char *s )
{
	itemDef_t item;
	item.parent = menu;

	g_Item_RunScriptParentHack = qtrue;
	Item_RunScript( &item, s );
	g_Item_RunScriptParentHack = qfalse;
}

qboolean Item_EnableShowViaCvar(itemDef_t *item, int flag)
{
	if ( !item ) {
		return qtrue;
	}

	if ( item->showif && *item->showif ) {

		int r = 0;
		trap_SQL_Prepare( item->showif );
		if ( trap_SQL_Step() ) {
			r = trap_SQL_ColumnAsInt(0);
		}
		trap_SQL_Done();
		return r;
	}

	if ( item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest )
	{
		const char *p;
		char	script	[ MAX_SCRIPT_LEN ];
		char	buff	[ MAX_CVAR_VALUE_STRING ];
		DC->getCVarString(item->cvarTest, buff, sizeof(buff));

		Q_strncpyz( script, item->enableCvar, sizeof( script ) );
		p = script;
		for( ; ; )
		{
			const char *val;

			// expect value then ; or NULL, NULL ends list
			if (!String_Parse(&p, &val))
			{
				return (item->cvarFlags & flag) ? qfalse : qtrue;
			}

			if ( val[0] == ';' && val[1] == '\0' )
			{
				continue;
			}

			// enable it if any of the values are true
			if ( item->cvarFlags & flag )
			{
				if ( Q_stricmp(buff, val) == 0 )
				{
					return qtrue;
				}
			} else
			{
				// disable it if any of the values are true
				if (Q_stricmp(buff, val) == 0)
				{
					return qfalse;
				}
			}

		}
	}

	return qtrue;
}

static void Item_SetFocusTime( itemDef_t * item, int time )
{
	if ( item )
	{
		int t = time-item->focusTime;
		//	this item is losing the focus before it was completely gained
		if ( t < ITEM_FADE_TIME )
		{
			item->focusTime = time - ( ITEM_FADE_TIME - t );
		} else
			item->focusTime = time;
	}
}


static void Menu_ClearFocusItem( menuDef_t * menu, itemDef_t * item )
{
	if ( menu->focusItem && menu->focusItem->leaveFocus )
		Item_RunScript( menu->focusItem, menu->focusItem->leaveFocus );

	//	if the focus item has changed record the time it did
	if ( menu->focusItem != item )
	{
		Item_SetFocusTime( item, DC->realTime );
		Item_SetFocusTime( menu->focusItem, DC->realTime );
		DC->toolTip( item );
	}

	menu->focusItem		= item;
}

static qboolean canHaveFocus( itemDef_t * item ) {

	if ( !item )
		return qfalse;

	// items can be enabled and disabled based on cvars
	if (item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar(item, CVAR_ENABLE)) {
		return qfalse;
	}

	if (item->cvarFlags & (CVAR_SHOW | CVAR_HIDE) && !Item_EnableShowViaCvar(item, CVAR_SHOW)) {
		return qfalse;
	}

	return !(item->window.flags & (WINDOW_DECORATION | WINDOW_NOFOCUS)) && (item->window.flags & WINDOW_VISIBLE);
}

static itemDef_t * Menu_GetNextFocusItem( menuDef_t * menu )
{
	int i,start;

	for ( start=0; start<menu->itemCount; start++ ) {
		if ( menu->items[ start ] == menu->focusItem )
			break;
	}

	for ( i=start+1; i<menu->itemCount; i++ ) {
		if ( canHaveFocus( menu->items[ i ] ) )
			return menu->items[ i ];
	}

	for ( i=0; i<=start; i++ ) {
		if ( canHaveFocus( menu->items[ i ] ) )
			return menu->items[ i ];
	}

	return 0;
}


static itemDef_t * Menu_GetPrevFocusItem( menuDef_t * menu )
{
	int i,start;

	for ( start=0; start<menu->itemCount; start++ ) {
		if ( menu->items[ start ] == menu->focusItem )
			break;
	}

	for ( i=start-1; i>=0; i-- ) {
		if ( canHaveFocus( menu->items[ i ] ) ) {
			return menu->items[ i ];
		}
	}

	for ( i=menu->itemCount-1; i>=start; i-- ) {
		if ( canHaveFocus( menu->items[ i ] ) ) {
			return menu->items[ i ];
		}
	}

	return 0;
}



static qboolean Menu_SetFocusItem( menuDef_t * menu, itemDef_t * item )
{
	if ( item == menu->focusItem )
		return qtrue;

	if ( !canHaveFocus( item ) )
		return qfalse;

	//
	//	unfocus old item
	//
	Menu_ClearFocusItem( menu, item );

	if ( item && item->onFocus )
		Item_RunScript( item, item->onFocus );

	return qtrue;
}

static void Item_Text_GetDataRect( itemDef_t *item, Rectangle * r, int offset )
{
	if ( item->textdivx )
	{
		r->x = item->window.rect.x + item->textdivx + offset;
		r->w = item->window.rect.w - item->textdivx - item->window.borderSize - offset;
		r->y = item->window.rect.y;
		r->h = item->window.rect.h;

	} else if ( item->textdivy )
	{
		r->x = item->window.rect.x;
		r->y = item->window.rect.y + item->textdivy + offset;
		r->w = item->window.rect.w;
		r->h = item->window.rect.h - item->textdivy - item->window.borderSize - offset;
	} else
	{
		r->x = item->textRect.x + item->textRect.w + offset;
		r->w = item->window.rect.w;
		r->y = item->textRect.y;
		r->h = item->textRect.h;
	}
}

static void Item_Slider_Incr( itemDef_t * item ) {
	editFieldDef_t *editDef = item->typeData;
	float v = DC->getCVarValue( item->cvar );
	float s = (editDef->flags&EDITFIELD_INTEGER)?1.0f:0.1f;
	if ( v < editDef->maxVal )
		Item_Slider_SetValue( item, min( v+s, editDef->maxVal ) );
}

static void Item_Slider_Decr( itemDef_t * item ) {
	editFieldDef_t *editDef = item->typeData;
	float v = DC->getCVarValue( item->cvar );
	float s = (editDef->flags&EDITFIELD_INTEGER)?1.0f:0.1f;
	if ( v > editDef->minVal )
		Item_Slider_SetValue( item, max( v-s, editDef->minVal ) );
}


float Item_Slider_ThumbPosition(itemDef_t *item) {

	float value, range;
	editFieldDef_t *editDef = item->typeData;
	rectDef_t * r = &item->textRect;

	if (editDef == NULL && item->cvar) {
		return r->x;
	}

	value = min( editDef->maxVal, max( editDef->minVal, DC->getCVarValue(item->cvar) ) );
	if ( editDef->maxVal == editDef->minVal )
		range = 0.5f;
	else
		range = (value - editDef->minVal) / (editDef->maxVal-editDef->minVal);

	return r->x + SLIDER_BUTTON_WIDTH + (r->w - (SLIDER_THUMB_WIDTH+SLIDER_BUTTON_WIDTH*2.0f)) * range;
}

int Item_Slider_OverSlider(itemDef_t *item, float x, float y) {
	rectDef_t * r = &item->textRect;
	if (Rect_ContainsPoint(r, x, y)) {
		float thumbX = Item_Slider_ThumbPosition( item );
		if ( x >= thumbX && x <= thumbX + SLIDER_THUMB_WIDTH )
			return WINDOW_LB_THUMB;

		if ( x-r->x <= SLIDER_BUTTON_WIDTH )
			return WINDOW_LB_LEFTARROW;

		if ( x >= r->x + r->w - SLIDER_BUTTON_WIDTH )
			return WINDOW_LB_RIGHTARROW;

		return WINDOW_HORIZONTAL;
	}
	return 0;
}

void Item_ListBox_UpdateColumns( itemDef_t * item )
{
	if ( item->type == ITEM_TYPE_LISTBOX )
	{
		int i;
		listBoxDef_t * listPtr = (listBoxDef_t*)item->typeData;

		for ( i=0; i<listPtr->numColumns; i++ )
		{
			if ( listPtr->flags & LISTBOX_ROTATE ) {
				listPtr->columnInfo[ i ].width	= listPtr->columnInfo[ i ].widthPerCent*0.01f*(item->window.rect.h-listPtr->titlebar);
				listPtr->columnInfo[ i ].pos	= (i==0)?0:listPtr->columnInfo[ i-1 ].pos + listPtr->columnInfo[ i-1 ].width;
			} else {
				listPtr->columnInfo[ i ].width	= listPtr->columnInfo[ i ].widthPerCent*0.01f*item->window.rect.w;
				listPtr->columnInfo[ i ].pos	= (i==0)?0:listPtr->columnInfo[ i-1 ].pos + listPtr->columnInfo[ i-1 ].width;
			}
		}
	}
}

void Item_ListBox_GetScrollBar( itemDef_t * item, listBoxDef_t * listPtr, rectDef_t * bar, rectDef_t * thumb, int rows, int count )
{
	float ty	= ((float)listPtr->startPos / (float)count);			// thumbs relative position in entire list
	float th	= ((float)rows / (float)count);							// relative amount that is being viewed ( thumb height )

	bar->x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;	// left side of scroll bar
	bar->y = item->window.rect.y + listPtr->titlebar + 2.0f;				// top of scroll bar
	bar->h = item->window.rect.h - listPtr->titlebar - 4.0f;				// height of scroll bar
	bar->w = SCROLLBAR_SIZE - 4.0f;

	thumb->x = bar->x + 2.0f;
	thumb->y = bar->y + ty*bar->h;
	thumb->w = bar->w - 4.0f;
	thumb->h = th * bar->h;
}

static int Item_ListBox_NumRows( itemDef_t * item, listBoxDef_t * listPtr )
{
	rectDef_t r;
	Item_TableCell_Rect( item, 0, -1, qfalse, &r );

	if ( listPtr->flags & LISTBOX_ROTATE ) {
		return (int)(r.w / listPtr->elementHeight);
	} else {
		return (int)(r.h / listPtr->elementHeight);
	}
}

int Item_ListBox_OverLB( itemDef_t *item, float x, float y )
{
	listBoxDef_t*	listPtr = (listBoxDef_t*)item->typeData;
	int				count	= listPtr->numRows;
	int				rows	= Item_ListBox_NumRows( item, listPtr );

	if( count <= rows )
		return 0;

	if (item->window.flags & WINDOW_HORIZONTAL)
	{
		return 0;	// fix me
	} else
	{
		rectDef_t bar, thumb;
		Item_ListBox_GetScrollBar( item, listPtr, &bar, &thumb, rows, count );

		if ( Rect_ContainsPoint( &bar, x, y ) )
		{
			if ( y < thumb.y )
				return WINDOW_LB_PGUP;

			if ( y < thumb.y + thumb.h )
				return WINDOW_LB_THUMB;

			return WINDOW_LB_PGDN;
		}
	}
	return 0;
}

void Item_MouseEnter(itemDef_t *item, float x, float y)
{
	Item_RunScript(item, item->mouseEnter);
	item->window.flags |= WINDOW_MOUSEOVER;
}

void Item_MouseLeave(itemDef_t *item)
{
	Item_RunScript(item, item->mouseExit);
	item->window.flags &= ~(WINDOW_MOUSEOVER);
}

itemDef_t *Menu_HitTest(menuDef_t *menu, float x, float y) {
  int i;
  for (i = 0; i < menu->itemCount; i++) {
    if (Rect_ContainsPoint(&menu->items[i]->window.rect, x, y)) {
      return menu->items[i];
    }
  }
  return NULL;
}

qboolean Item_OwnerDraw_HandleMouseMove( itemDef_t *item, float x, float y )
{
	if( item && DC->ownerDrawHandleMouseMove )
		return DC->ownerDrawHandleMouseMove( item, x,y );

	return qfalse;
}

qboolean Item_ListBox_SingleSelect_HandleKey( itemDef_t *item, int key )
{
	int count, rows, scroll, sel, cursel;
	listBoxDef_t *lp = (listBoxDef_t*)item->typeData;

	count	= lp->numRows;
	rows	= Item_ListBox_NumRows( item, lp );
	scroll	= max( 0, count-rows );

	trap_SQL_Prepare	( "SELECT sel FROM feeders SEARCH name $;" );
	trap_SQL_BindText	( 1, item->window.name );
	if ( !trap_SQL_Step() ) {
		return qfalse;
	}
	cursel	=
	sel		= trap_SQL_ColumnAsInt(0);
	trap_SQL_Done();

	switch( key )
	{
	case K_MOUSE1:
	case K_MOUSE2:	
		{
			static int lastClickTime = 0;
			int mouseItem = -1;
			int i;
			float y = DC->cursory - lp->titlebar - item->window.rect.y;

			if ( y < 0.0f )
				return qfalse;	// clicked above list

			for ( i=0; i<count; i++ ) {
				rectDef_t r;
				Item_TableCell_Rect( item, -1, i, qfalse, &r );
				r.y -= 1.0f;
				r.h += 2.0f;
				if ( Rect_ContainsPoint( &r, DC->cursorx, DC->cursory ) ) {
					mouseItem = lp->startPos+i;
					break;
				}
			}

			if ( mouseItem == -1 )
				return qfalse;	// click off the end of the list

			if( DC->realTime < lastClickTime && item->action && sel == mouseItem )
			{								  
				Item_RunScript( item, item->action );

				lastClickTime = 0; //no chaining double clicks
			}
			else
			{
				lastClickTime = DC->realTime + DOUBLE_CLICK_DELAY;
				sel = mouseItem;
			}		
		}
		break;

	case K_UPARROW:
	case K_KP_UPARROW:
		sel--;
		break;

	case K_MWHEELUP:
		{
			lp->startPos--;
			if ( lp->startPos < 0 )
				lp->startPos = 0;
		} return qtrue;
	case K_MWHEELDOWN:
		{
			lp->startPos++;
			if ( lp->startPos > scroll )
				lp->startPos = scroll;
		} return qtrue;


	case K_DOWNARROW:
	case K_KP_DOWNARROW:
		sel++;
		break;

	case K_PGUP:
	case K_KP_PGUP:

		return qtrue;

	case K_PGDN:
	case K_KP_PGDN:

		return qtrue;

	case K_HOME:
	case K_KP_HOME:	
		
		return qtrue;

	case K_END:
	case K_KP_END:

		return qtrue;

	default:
		return qfalse;
	}
	
	if( sel < 0 )
		sel = 0;
	if( sel >= count )
		sel = count - 1;

	if( sel < lp->startPos )
		lp->startPos = sel;
	if( sel >= lp->startPos + rows )
		lp->startPos = sel - rows + 1;

	if( lp->startPos < 0 )
		lp->startPos = 0;
	if( lp->startPos > scroll )
		lp->startPos = scroll;

	if ( cursel != sel && lp->buffer ) {

		cell_t * cells = (cell_t*)lp->buffer;

		trap_SQL_Prepare	( "UPDATE feeders SET sel=?1,id=?2,enabled=?3 SEARCH name $4;" );
		trap_SQL_BindInt	( 1, sel );
		trap_SQL_BindInt	( 2, cells[ sel * (lp->numColumns+2) ].integer );
		trap_SQL_BindInt	( 3, cells[ sel * (lp->numColumns+2) + 1 ].integer );
		trap_SQL_BindText	( 4, item->window.name );
		trap_SQL_Step		();
		trap_SQL_Done		();

		lp->timeSel = DC->realTime;
	}

	if ( lp->select && ((cursel != sel) || (lp->flags&LISTBOX_ALWAYSSELECT)) ) {
		Item_RunScript( item, lp->select );
	}

	return qtrue;
}

qboolean Item_ListBox_HandleKey( itemDef_t *item, int key, qboolean force )
{													 
	listBoxDef_t *lp = (listBoxDef_t*)item->typeData;
	int		count	= lp->numRows;
	int		rows	= Item_ListBox_NumRows( item, lp );
	int		scroll	= max( 0, count-rows );

	if ( !force && !Item_HasFocus(item) )
		return qfalse;
	
	if( isMouseKey( key ) )
	{
		//handle the scroll bar
		if( item->window.flags & WINDOW_LB_LEFTARROW )
		{
			if( --lp->startPos < 0 )
				lp->startPos = 0;

			return qtrue;
		}
		else if( item->window.flags & WINDOW_LB_RIGHTARROW )
		{
			if( ++lp->startPos > scroll )
				lp->startPos = scroll;

			return qtrue;
		}
		else if( item->window.flags & WINDOW_LB_PGUP )
		{
			if( (lp->startPos -= rows) < 0 )
				lp->startPos = 0;

			return qtrue;
		}
		else if( item->window.flags & WINDOW_LB_PGDN )
		{
			if( (lp->startPos += rows) > scroll )
				lp->startPos = scroll;

			return qtrue;
		}
		else if( item->window.flags & WINDOW_LB_THUMB )
		{
			return qtrue;
		}
	}

	if ( key == K_ENTER && item->action )
	{
		Item_RunScript(item, item->action);
		return qtrue;
	}


	return Item_ListBox_SingleSelect_HandleKey( item, key );
}

int Item_Multi_CountSettings(itemDef_t *item) {
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if (multiPtr == NULL) {
		return 0;
	}
	return multiPtr->count;
}

int Item_Multi_FindCvarByValue(itemDef_t *item) {
	char buff[1024];
	float value = 0;
	int i;
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if (multiPtr) {
		if (multiPtr->strDef) {
	    DC->getCVarString(item->cvar, buff, sizeof(buff));
		} else {
			value = DC->getCVarValue(item->cvar);
		}
		for (i = 0; i < multiPtr->count; i++) {
			if (multiPtr->strDef) {
				if (Q_stricmp(buff, multiPtr->cvarStr[i]) == 0) {
					return i;
				}
			} else {
 				if (multiPtr->cvarValue[i] == value) {
 					return i;
 				}
 			}
 		}
	}
	return 0;
}

const char *Item_Multi_Setting(itemDef_t *item) {
	char buff[1024];
	float value = 0;
	int i;
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if (multiPtr) {
		if (multiPtr->strDef) {
	    DC->getCVarString(item->cvar, buff, sizeof(buff));
		} else {
			value = DC->getCVarValue(item->cvar);
		}
		for (i = 0; i < multiPtr->count; i++) {
			if (multiPtr->strDef) {
				if (Q_stricmp(buff, multiPtr->cvarStr[i]) == 0) {
					return multiPtr->cvarList[i];
				}
			} else {
 				if (multiPtr->cvarValue[i] == value) {
					return multiPtr->cvarList[i];
 				}
 			}
 		}
	}
	return "";
}

static void setnumericfield( itemDef_t * item, editFieldDef_t * editPtr, int v )
{
	if ( v < editPtr->minVal )
		v = editPtr->minVal;
	if ( v > editPtr->maxVal )
		v = editPtr->maxVal;

	DC->setCVar( item->cvar, va( "%i", v ) );
}


void Item_TextField_HandleKey(itemDef_t *item, int key) {
	char buff[1024];
	int len;
	//itemDef_t *newItem = NULL;
	editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;

	if ( !item->cvar )
		return;

	memset(buff, 0, sizeof(buff));
	DC->getCVarString(item->cvar, buff, sizeof(buff));
	len = strlen(buff);
	if (editPtr->maxChars && len > editPtr->maxChars) {
		len = editPtr->maxChars;
	}
	if ( item->cursorPos > len )
		item->cursorPos = len;

	if ( key & K_CHAR_FLAG ) {
		key &= ~K_CHAR_FLAG;


		if (key == 'h' - 'a' + 1 )	{	// ctrl-h is backspace
			if ( item->cursorPos > 0 ) {
				memmove( &buff[item->cursorPos - 1], &buff[item->cursorPos], len + 1 - item->cursorPos);
				item->cursorPos--;
				if (item->cursorPos < editPtr->paintOffset) {
					editPtr->paintOffset--;
				}
			}
			DC->setCVar(item->cvar, buff);
    		return;
		}

		//
		// ignore any non printable chars
		//
		if ( key < 32 || !item->cvar) {
		    return;
	    }

		if (item->type == ITEM_TYPE_NUMERICFIELD) {
			if (key < '0' || key > '9') {
				return;
			}
		}

		if (!DC->getOverstrikeMode()) {
			if (( len == MAX_EDITFIELD - 1 ) || (editPtr->maxChars && len >= editPtr->maxChars)) {
				return;
			}
			memmove( &buff[item->cursorPos + 1], &buff[item->cursorPos], len + 1 - item->cursorPos );
		} else {
			if (editPtr->maxChars && item->cursorPos >= editPtr->maxChars) {
				return;
			}
		}

		buff[item->cursorPos] = key;

		if (item->type == ITEM_TYPE_NUMERICFIELD) {
			setnumericfield( item, editPtr, atoi(buff) );
			DC->getCVarString(item->cvar, buff, sizeof(buff));
			len = strlen(buff);
		} else
			DC->setCVar(item->cvar, buff);

		if (item->cursorPos < len + 1) {
			item->cursorPos++;
			if (editPtr->maxPaintChars && item->cursorPos > editPtr->maxPaintChars) {
				editPtr->paintOffset++;
			}
		}

	} else {

		switch ( key )
		{
			case K_DEL:
				if ( item->cursorPos < len ) {
					memmove( buff + item->cursorPos, buff + item->cursorPos + 1, len - item->cursorPos);
					DC->setCVar(item->cvar, buff);
				}
				break;

			case K_RIGHTARROW:
				if ( item->cursorPos < len )
				{
					item->cursorPos++;

					if ( editPtr->maxPaintChars && (item->cursorPos-editPtr->paintOffset) >= editPtr->maxPaintChars )
						editPtr->paintOffset++;
				}
				break;

			case K_LEFTARROW:
				if ( item->cursorPos > 0 ) item->cursorPos--;
				if ( item->cursorPos < editPtr->paintOffset ) editPtr->paintOffset--;
				break;

			case K_HOME:
				if ( item->type == ITEM_TYPE_NUMERICFIELD )
				{
					setnumericfield( item, editPtr, editPtr->minVal );

				} else
				{
					item->cursorPos			= 0;
					editPtr->paintOffset	= 0;
				}
				break;

			case K_END:
				if ( item->type == ITEM_TYPE_NUMERICFIELD )
				{
					setnumericfield( item, editPtr, editPtr->maxVal );

				} else
				{
					item->cursorPos		= len;
					if ( item->cursorPos > editPtr->maxPaintChars )
						editPtr->paintOffset = len - editPtr->maxPaintChars;
				}
				break;

			case K_INS:
				DC->setOverstrikeMode(!DC->getOverstrikeMode());
				break;

			case K_UPARROW:
			case K_MWHEELUP:
				if ( item->type == ITEM_TYPE_NUMERICFIELD )
				{
					setnumericfield( item, editPtr, atoi( buff )+1 );
				} 
				break;

			case K_DOWNARROW:
			case K_MWHEELDOWN:
				if ( item->type == ITEM_TYPE_NUMERICFIELD )
				{
					setnumericfield( item, editPtr, atoi( buff )-1 );
				}
				break;

			case K_PGUP:
				if ( item->type == ITEM_TYPE_NUMERICFIELD )
				{
					setnumericfield( item, editPtr, atoi( buff )-10 );
				}
				break;

			case K_PGDN:
				if ( item->type == ITEM_TYPE_NUMERICFIELD )
				{
					setnumericfield( item, editPtr, atoi( buff )+10 );
				}
				break;

			case K_MOUSE1:
				{
					// find cursor position
					//trap_R_RenderText
				} break;

			case K_ENTER:
			case K_KP_ENTER:
				Item_Action( item );
				return;
		}
	}
}

static void Scroll_ListBox_AutoFunc(void *p) {
	scrollInfo_t *si = (scrollInfo_t*)p;
	if (DC->realTime > si->nextScrollTime) { 
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		Item_ListBox_HandleKey(si->item, si->scrollKey, qfalse);
		si->nextScrollTime = DC->realTime + si->adjustValue; 
	}

	if (DC->realTime > si->nextAdjustTime) {
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if (si->adjustValue > SCROLL_TIME_FLOOR) {
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

static void Scroll_ListBox_ThumbFunc(void *p)
{
	scrollInfo_t *si = (scrollInfo_t*)p;

	listBoxDef_t *listPtr = (listBoxDef_t*)si->item->typeData;
	if (si->item->window.flags & WINDOW_HORIZONTAL)
		return; // fix me


	if ( si->item->window.flags & WINDOW_LB_THUMB )
	{
		if (DC->cursory != si->yStart)
		{
			rectDef_t b,t;
			int count	= listPtr->numRows;
			int rows	= Item_ListBox_NumRows( si->item, listPtr );
			int move;

			Item_ListBox_GetScrollBar( si->item, listPtr, &b, &t, rows, count ); 

			move = (DC->cursory-si->yStart) / ((b.h-t.h) / (float)count);

			if ( move != 0 )
			{
				
				int scroll	= max( 0, count-rows );

				listPtr->startPos += move;

				if ( listPtr->startPos < 0	)		listPtr->startPos = 0;
				if ( listPtr->startPos > scroll)	listPtr->startPos = scroll;
				si->yStart = DC->cursory;
			}
		}
	} else 
	{
		if (DC->realTime > si->nextScrollTime)
		{ 
			// need to scroll which is done by simulating a click to the item
			// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
			// so it calls it directly
			Item_ListBox_HandleKey(si->item, si->scrollKey, qfalse);
			si->nextScrollTime = DC->realTime + si->adjustValue; 
		}

		if (DC->realTime > si->nextAdjustTime)
		{
			si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
			if (si->adjustValue > SCROLL_TIME_FLOOR) {
				si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
			}
		}
	}
}

static float Item_Slider_ValueFromCursor( itemDef_t * item, float x )
{
	editFieldDef_t *	editDef	= item->typeData;
	rectDef_t *			r		= &item->textRect;
	float value;
	float left;
	float width;

	left	= r->x + SLIDER_BUTTON_WIDTH;
	width	= r->w - (SLIDER_THUMB_WIDTH + SLIDER_BUTTON_WIDTH*2.0f);

	value	= (x-left) / (width);

	if ( value < 0.0f )
		value = 0.0f;

	if ( value > 1.0f )
		value = 1.0f;

	//	scale and bias to slider value
	value *= (editDef->maxVal - editDef->minVal);
	value += editDef->minVal;

	return value;
}

static void Item_Slider_SetValue( itemDef_t * item, float value ) {
	editFieldDef_t*	editDef = item->typeData;
	DC->setCVar(item->cvar, (editDef->flags&EDITFIELD_INTEGER)?va("%d",(int)value):va("%f", value));
}


static void Scroll_Slider_ThumbFunc(void *p) {
	scrollInfo_t*	si		= (scrollInfo_t*)p;
	float value = Item_Slider_ValueFromCursor( si->item, DC->cursorx + si->offset );
	Item_Slider_SetValue( si->item, value );
	Item_RunScript( si->item, si->item->action );
}

static void Item_UpdateData( itemDef_t * item, field_t * field, byte ** data )
{
	switch( field->type )
	{
	case FIELD_WINDOW:		*data = (byte*)&item->window;		break;
	case FIELD_ITEM:		*data = (byte*)item;				break;
	case FIELD_MENU:		*data = 0;   break;
	case FIELD_EDITFIELD:
		{
			switch ( item->type )
			{
				case ITEM_TYPE_EDITFIELD:
				case ITEM_TYPE_NUMERICFIELD:
				case ITEM_TYPE_YESNO:
				case ITEM_TYPE_BIND:
				case ITEM_TYPE_SLIDER:
				case ITEM_TYPE_TEXT:
					Item_ValidateTypeData(item);
					*data = (byte*)item->typeData;
					break;
				default:
					*data = 0;
					break;
			}
		} break;

	case FIELD_LISTBOX:
		{
			if ( item->type == ITEM_TYPE_LISTBOX )
			{
				Item_ValidateTypeData(item);
				*data = (byte*)item->typeData;
			} else
				*data = 0;
		} break;

	case FIELD_PROGRESS:
		Item_ValidateTypeData(item);
		*data = (byte*)item->typeData;
		break;

	case FIELD_COLUMNS:
	case FIELD_EFFECT:
		*data = 0;	break;
	}
}

static void Menu_UpdateData( menuDef_t * menu, field_t * field, byte ** data )
{
	switch( field->type )
	{
	case FIELD_WINDOW:
		*data = (byte*)&menu->window;
		break;

	case FIELD_MENU:
		*data = (byte*)menu;
		break;

	case FIELD_ITEM:
	case FIELD_EDITFIELD:
	case FIELD_LISTBOX:
	case FIELD_COLUMNS:
	case FIELD_EFFECT:
		*data = 0;
		break;
	}
}



#ifdef DEVELOPER
static void snaptorect( rectDef_t * r, int * x, int * y, int w, int h, int box )
{
	int X1 = (int)r->x;
	int X2 = (int)(r->x + r->w );
	int Y1 = (int)r->y;
	int Y2 = (int)(r->y + r->h );
	int X = *x;
	int Y = *y;

	if ( abs( X-X1 ) < 4 )
		*x = X1;
	else if ( abs( X-X2 ) < 4 )
		*x = X2;
	else if ( box && (abs( X+w-X1 ) < 4) )
		*x = X1-w;
	else if ( box && (abs( X+w-X2 ) < 4) )
		*x = X2-w;
	
	if ( abs( Y-Y1 ) < 4 )
		*y = Y1;
	else if ( abs( Y-Y2 ) < 4 )
		*y = Y2;
	else if ( box && (abs( Y+h-Y1 ) < 4) )
		*y = Y1-h;
	else if ( box && (abs( Y+h-Y2 ) < 4) )
		*y = Y2-h;
}

static void DebugMode_Snap( itemDef_t * item, int * x, int * y, int box )
{
	int i;
	menuDef_t * menu = (menuDef_t *)item->parent;

	for ( i=0; i<menu->itemCount; i++ )
	{
		if ( menu->items[ i ] == item )
			continue;

		snaptorect( &menu->items[ i ]->window.rect, x,y, item->window.rect.w, item->window.rect.h, box );
	}

	snaptorect( &menu->window.rect, x,y, item->window.rect.w, item->window.rect.h, box );
}

static void DebugMode_MoveControl_AutoFunc(void *p) {
	scrollInfo_t *si = (scrollInfo_t*)p;
	rectDef_t * r = &si->item->window.rect;
	menuDef_t * menu = (menuDef_t *)si->item->parent;

	int x = (DC->cursorx - (int)si->xStart);
	int y = (DC->cursory - (int)si->yStart);

	if ( debug.dontsnap == 0 ) {
		x = (x >> 1) << 1;
		y = (y >> 1) << 1;

		DebugMode_Snap( si->item, &x, &y, si->scrollKey & SCROLL_DEBUGMODE_MOVE );
	}


	if ( si->scrollKey & SCROLL_DEBUGMODE_MOVE )
	{
		r->x = x;
		r->y = y;
	}

	if ( si->scrollKey & SCROLL_DEBUGMODE_LEFT )
	{
		int right = r->x + r->w;
		r->x = x;
		r->w = right - r->x;
	}

	if ( si->scrollKey & SCROLL_DEBUGMODE_RIGHT )
	{
		r->w = x - r->x;
	}

	if ( si->scrollKey & SCROLL_DEBUGMODE_TOP )
	{
		int bottom = r->y + r->h;
		r->y = y;
		r->h = bottom - r->y;
	}

	if ( si->scrollKey & SCROLL_DEBUGMODE_BOTTOM )
	{
		r->h = y - r->y;
	}

	si->item->window.rectClient.x = r->x - menu->window.rect.x;
	si->item->window.rectClient.y = r->y - menu->window.rect.y;
	si->item->window.rectClient.w = r->w;
	si->item->window.rectClient.h = r->h;

	Item_ListBox_UpdateColumns( si->item );
}

static void DebugMode_MoveMenu_AutoFunc(void *p) {
	scrollInfo_t *si = (scrollInfo_t*)p;
	menuDef_t * menu = (menuDef_t*)si->item;
	rectDef_t * r = &menu->window.rect;
	

	int x = (DC->cursorx - (int)si->xStart);
	int y = (DC->cursory - (int)si->yStart);
	int i;

	x = (x >> 1) << 1;
	y = (y >> 1) << 1;

	if ( debug.dontsnap == 0 )
	{
		float sx = DC->glconfig.xbias / DC->glconfig.xscale;
		float sw = 640.0f + (2.0f*x);

		rectDef_t screen;
		screen.x = 0.0f;
		screen.y = 0.0f;
		screen.w = 640.0f;
		screen.h = 480.0f;

		for ( i=0; i<menuStack->count; i++ )
		{
			if ( menuStack->m[ i ] == menu )
				continue;

			snaptorect( &menuStack->m[ i ]->window.rect, &x,&y, menu->window.rect.w, menu->window.rect.h, ( si->scrollKey & SCROLL_DEBUGMODE_MOVE ) );
		}

		snaptorect( &screen, &x,&y, menu->window.rect.w, menu->window.rect.h, ( si->scrollKey & SCROLL_DEBUGMODE_MOVE ) );
		screen.x = -sx;
		screen.w = sw;
		snaptorect( &screen, &x,&y, menu->window.rect.w, menu->window.rect.h, ( si->scrollKey & SCROLL_DEBUGMODE_MOVE ) );
	}

	if ( si->scrollKey & SCROLL_DEBUGMODE_MOVE )
	{
		r->x = x;
		r->y = y;
	}

	if ( si->scrollKey & SCROLL_DEBUGMODE_LEFT )
	{
		int right = r->x + r->w;
		r->x = x;
		r->w = right - r->x;
	}

	if ( si->scrollKey & SCROLL_DEBUGMODE_RIGHT )
	{
		r->w = x - r->x;
	}

	if ( si->scrollKey & SCROLL_DEBUGMODE_TOP )
	{
		int bottom = r->y + r->h;
		r->y = y;
		r->h = bottom - r->y;
	}

	if ( si->scrollKey & SCROLL_DEBUGMODE_BOTTOM )
	{
		r->h = y - r->y;
	}

	Menu_UpdatePosition( menu );
}

int DebugMode_GetScroll( rectDef_t * r, int x, int y )
{
	int s = 0;
	const float b = 3.0f;
	if ( Rect_ContainsPoint( r, (float)x,(float)y ) )
	{
		if ( x - r->x < b )
			s |= SCROLL_DEBUGMODE_LEFT;

		if ( r->x + r->w - x < b )
			s |= SCROLL_DEBUGMODE_RIGHT;

		if ( y - r->y < b )
			s |= SCROLL_DEBUGMODE_TOP;

		if ( r->y + r->h - y < b )
			s |= SCROLL_DEBUGMODE_BOTTOM;

		if ( s == 0 )
			s = SCROLL_DEBUGMODE_MOVE;
	}

	return s;
}

static itemDef_t * ed_item;

static char * ed_cvar( const char * section, const char * field_name ) {
	return va( "ed_%s_%s", section, field_name );
}

void UI_Editor_UpdateItem()
{
	byte *		data = 0;
	byte *		b;
	char *		section = 0;
	char *		cvar;
	int i;
	for ( i=0; i<fieldCount; i++ )
	{
		if ( fields[ i ].flags & FIELD_DONTEDIT || !fields[ i ].name )
			continue;

		if ( fields[ i ].flags & FIELD_SECTION )
			section = fields[ i ].name;

		Item_UpdateData( ed_item, fields + i, &data );

		if ( data == 0 )
			continue;

		b = data + fields[ i ].offset;

		cvar = ed_cvar( section, fields[ i ].name );

		switch( fields[ i ].type )
		{

		//case FIELD_SCRIPT:
		case FIELD_STRING:
			{
				char tmp[ MAX_INFO_STRING ];
				DC->getCVarString( cvar, tmp, sizeof(tmp) );
				*(const char**)( b ) = (tmp[0]=='\0')?0:String_Alloc( tmp );
			} break;

		case FIELD_INTEGER:
			{
				*(int*)( b ) = (int)DC->getCVarValue( cvar );
			} break;

		case FIELD_FLOAT:
			{
				*(float*)( b ) = DC->getCVarValue( cvar );
			} break;
		case FIELD_FONT:
			{
				char tmp[ MAX_INFO_STRING ];
				DC->getCVarString( cvar, tmp, sizeof(tmp) );
				*(qhandle_t*)( b ) = atoi(tmp);
			} break;
		case FIELD_BIT:
			{
				int		v = (int)DC->getCVarValue( cvar );
				int *	d = (int*)( b );

				if ( v )
					*d |= fields[ i ].bit;
				else
					*d &= ~fields[ i ].bit;

			} break;
		default:
			{
				//return;
			} break;
		}
	}
}

void UI_Editor_EditItem( itemDef_t * item )
{
	byte * data = 0;
	byte * b;
	char * section = 0;
	char cvar[ 256 ];
	int i;

	ed_item = item;

	for ( i=0; i<fieldCount; i++ )
	{
		field_t * f = fields + i;
		if ( f->flags & FIELD_DONTEDIT || !f->name)
			continue;

		if ( f->flags & FIELD_SECTION )
			section = f->name;

		Q_strncpyz( cvar, ed_cvar( section, f->name ), sizeof(cvar) );

		DC->setCVar( cvar, "" );

		Item_UpdateData( item, f, &data );

		if ( !data )
			continue;

		b = data + f->offset;

		switch( f->type )
		{
		//case FIELD_SCRIPT:
		case FIELD_STRING:
			{
				DC->setCVar( cvar, (b)?*(const char**)(b):"" );
			} break;
		case FIELD_FONT:
		case FIELD_INTEGER:
			{
				DC->setCVar( cvar, va("%d", *(int*)(b) ) );
			} break;
		case FIELD_FLOAT:
			{
				DC->setCVar( cvar, va("%f", *(float*)(b) ) );
			} break;
		case FIELD_BIT:
			{
				DC->setCVar( cvar, va("%d", *(int*)(b) & f->bit ) );
			} break;
		}
	}

	Menus_OpenByName( "editor" );
}

static char * getfontname( qhandle_t font )
{
	char info[ MAX_INFO_STRING ];
	char * s = info;
	trap_R_GetFonts( info, sizeof(info) );


	for ( ;; )
	{
		int		h = atoi( COM_ParseExt( &s, qfalse ) );
		char *	n = COM_ParseExt( &s, qfalse );

		if ( h == font )
			return n;
	}

	return 0;
}

extern void trap_FS_Write( const void *buffer, int len, fileHandle_t f );

static void escapesequence( char * buffer, int size, const char * src )
{
	int i,j;
	for ( i=0,j=0; i<size && src[j]; i++, j++ )
	{
		if ( src[ j ] == 9 )
		{
			buffer[ i++ ] = '\\';
			buffer[ i ] = 't';
		} else if ( src[ j ] == 10 )
		{
			buffer[ i++ ] = '\\';
			buffer[ i ] = 'n';
		} else
			buffer[ i ] = src[ j ];
	}
	buffer[ i ] = '\0';
}

void UI_Editor_SaveField( fileHandle_t f, itemDef_t * item, byte * data, field_t * field )
{
	const char * t = 0;

	data += field->offset;

	switch( field->type )
	{
	case FIELD_SCRIPT:
		{
			//const char * s = *(const char**)( (byte*)item + itemFields[ i ].offset );
			//t = (!s)?0:va("\t\t\t%s\t\t{ %s }\n", itemFields[ i ].name, s ); 
		} break;
	case FIELD_STRING:
		{
			const char * s = *(const char**)(data);
			if ( s && s[ 0 ] != '\0' )
			{
				char buffer[ MAX_INFO_STRING ];
				escapesequence( buffer, sizeof(buffer), s );
				t = va("\t\t\t%s\t\t\"%s\"\n", field->name, buffer );
			}

		} break;
	case FIELD_FLOAT:
		{
			t = va("\t\t\t%s\t\t%f\n", field->name, *(float*)(data) );
		} break;

	case FIELD_RECT:
		{
			rectDef_t * r = (rectDef_t*)( data ); 
			t = va("\t\t\t%s\t\t%d %d %d %d\n", field->name, (int)r->x, (int)r->y, (int)r->w, (int)r->h );
		} break;
	case FIELD_BIT:
		{
			if ( *(int*)( data ) & field->bit )
				t = va("\t\t\t%s\n", field->name );
		} break;
	case FIELD_FONT:
		{
			if ( item->font != ((menuDef_t*)item->parent)->font )
				t = va("\t\t\t%s\t\t\"%s\"\n", field->name, getfontname( *(int*)( data ) ) ); 
		} break;
	case FIELD_FUNC:
		break;

	case FIELD_INTEGER:
		{
			int v = *(int*)( data );
			if ( v != 0 )
			{
				int i;
				for ( i=0; i<MAX_MULTI_CVARS; i++ )
				{
					if ( field->fieldEnum[ i ].name && field->fieldEnum[ i ].value == v )
					{
						t = va("\t\t\t%s\t\t%s\n", field->name, field->fieldEnum[ i ].name );
						break;
					}
				}

				if ( !t )
					t = va("\t\t\t%s\t\t%d\n", field->name, v );
			}
		} break;
	}

	if ( t )
		trap_FS_Write( t, strlen(t), f );
}

void Save_Item( fileHandle_t f, itemDef_t * item )
{
	int		i;
	byte *	data = 0;

	for ( i=0; i<fieldCount; i++ )
	{
		field_t * field = fields + i;

		Item_UpdateData( item, field, &data );

		if ( !data )
			continue;

		if ( !(field->flags & FIELD_DONTSAVE) )
			UI_Editor_SaveField( f, item, data, field );
	}
}
#endif

void Item_StartCapture(itemDef_t *item, int key)
{
#ifdef DEVELOPER
	if ( debugMode )
	{
		if ( !debug.menu || ((menuDef_t*)item->parent) == debug.menu )
		{
			if ( key == K_MOUSE1 || key == K_MOUSE3 )
			{
				scrollInfo.xStart = DC->cursorx - item->window.rect.x;
				scrollInfo.yStart = DC->cursory - item->window.rect.y;
				scrollInfo.scrollKey = DebugMode_GetScroll( &item->window.rect, DC->cursorx, DC->cursory );
				if ( scrollInfo.scrollKey & SCROLL_DEBUGMODE_RIGHT )
					scrollInfo.xStart -= item->window.rect.w;
				if ( scrollInfo.scrollKey & SCROLL_DEBUGMODE_BOTTOM )
					scrollInfo.yStart -= item->window.rect.h;
				scrollInfo.item = item;
				captureData = &scrollInfo;
				captureFunc = &DebugMode_MoveControl_AutoFunc;
				itemCapture = item;

				debug.dontsnap = trap_Key_IsDown( K_SHIFT );

			} else if ( key == K_MOUSE2 )
			{
				UI_Editor_EditItem( item );
				itemCapture = item;
			}
		}
		return;
	}
#endif

	switch( item->type )
	{
	case ITEM_TYPE_EDITFIELD:
	case ITEM_TYPE_NUMERICFIELD:
	case ITEM_TYPE_LISTBOX:
		{
			item->window.flags &= ~(WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN);
			item->window.flags |= Item_ListBox_OverLB(item, DC->cursorx, DC->cursory);
			
			if( item->window.flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW) )
			{
				scrollInfo.nextScrollTime = DC->realTime + SCROLL_TIME_START;
				scrollInfo.nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
				scrollInfo.adjustValue = SCROLL_TIME_START;
				scrollInfo.scrollKey = key;
				scrollInfo.scrollDir = (item->window.flags & WINDOW_LB_LEFTARROW) ? qtrue : qfalse;
				scrollInfo.item = item;
				captureData = &scrollInfo;
				captureFunc = &Scroll_ListBox_AutoFunc;
				itemCapture = item;
			}
			else if( item->window.flags & WINDOW_LB_THUMB )
			{
				scrollInfo.scrollKey = key;
				scrollInfo.item = item;
				scrollInfo.xStart = DC->cursorx;
				scrollInfo.yStart = DC->cursory;
				captureData = &scrollInfo;
				captureFunc = &Scroll_ListBox_ThumbFunc;
				itemCapture = item;
			}
			break;
		}
	case ITEM_TYPE_SLIDER:
		{
			int flags = Item_Slider_OverSlider( item, DC->cursorx, DC->cursory );
			if( flags )
			{
				if ( flags & WINDOW_LB_LEFTARROW ) {
					Item_Slider_Decr( item );

				} else if ( flags & WINDOW_LB_RIGHTARROW ) {
					Item_Slider_Incr( item );

				} else
				{
					scrollInfo.scrollKey = key;
					scrollInfo.item = item;
					scrollInfo.xStart = DC->cursorx;
					scrollInfo.yStart = DC->cursory;
					captureData = &scrollInfo;
					captureFunc = &Scroll_Slider_ThumbFunc;
					itemCapture = item;

					// if the click missed the thumb, snap the thumb there and then start the drag
					if ( flags & WINDOW_HORIZONTAL )
					{
						float value = Item_Slider_ValueFromCursor( item, DC->cursorx );
						Item_Slider_SetValue( item, value );
						scrollInfo.offset = 0.0f;
					} else
						scrollInfo.offset = Item_Slider_ThumbPosition( item ) - DC->cursorx;
				}
			} else 
			{

			}
			break;
		}
	}
}

void Item_StopCapture( itemDef_t *item )
{
	item->window.flags &= ~(WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN);
}

void Item_Action(itemDef_t *item) {
  if (item) {
    Item_RunScript(item, item->action);
  }
}

itemDef_t * Menu_NearItem( menuDef_t * menu, itemDef_t * focus, int direction ) {
	int i;
	itemDef_t * best	= NULL;

	// scan the items from top to bottom
	for ( i=0; i<menu->itemCount; i++ ) {

		itemDef_t*	item = menu->items[ i ];

		if ( item == focus )
			continue;

		if( !(item->window.flags & (WINDOW_VISIBLE) ) )
			continue;

		if ( (item->window.flags & (WINDOW_DECORATION | WINDOW_FADINGOUT)) )
			continue;

		// items can be enabled and disabled based on cvars
		if( item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE)
			&& !Item_EnableShowViaCvar( item, CVAR_ENABLE ) )
			continue;

		if( item->cvarFlags & (CVAR_SHOW | CVAR_HIDE)
			&& !Item_EnableShowViaCvar( item, CVAR_SHOW ) )
			continue;

		if ( !focus ) {
			return item;
		}

		switch ( direction ) {
			case K_UPARROW:
				if ( item->window.rect.y > focus->window.rect.y )
					continue;
				if ( !best || item->window.rect.y > best->window.rect.y )
					best = item;
				break;

			case K_RIGHTARROW:
				if ( item->window.rect.x < focus->window.rect.x )
					continue;
				if ( !best || item->window.rect.x < best->window.rect.x )
					best = item;
				break;

			case K_DOWNARROW:
				if ( item->window.rect.y < focus->window.rect.y )
					continue;
				if ( !best || item->window.rect.y < best->window.rect.y )
					best = item;
				break;

			case K_LEFTARROW:
				if ( item->window.rect.x > focus->window.rect.x )
					continue;
				if ( !best || item->window.rect.x > best->window.rect.x )
					best = item;
				break;
		}
	}

	return best;
}


itemDef_t*	Menu_SetPrevCursorItem( menuDef_t *menu )
{
	itemDef_t * item = Menu_GetPrevFocusItem( menu );

	if ( item )
	{
		Menu_SetFocusItem( menu, item );
		item = menu->focusItem;

		if ( item && (item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD) )
		{
			UI_SetEditItem( item );
		}
	}

	return 0;
}

itemDef_t *Menu_SetNextCursorItem(menuDef_t *menu)
{
	itemDef_t * item = Menu_GetNextFocusItem( menu );

	if ( item )
	{
		Menu_SetFocusItem( menu, item );
		item = menu->focusItem;

		if ( item && (item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD) )
		{
			UI_SetEditItem( item );
		}
	}

	return 0;
}

//	set all windows on the menu stack to visible, all menus on the stack and UNDER a fullscreen menu
//	are set to invisible
static void Menus_SetVisible() {
	int i;

	for ( i=0; i<menuCount; i++ ) 
		Menus[ i ].window.flags &= ~WINDOW_VISIBLE;

	for ( i=menuStack->count-1; i>=0; i-- ) {

		menuStack->m[ i ]->window.flags |= WINDOW_VISIBLE;

		if( menuStack->m[ i ]->window.flags&WINDOW_FULLSCREEN )
			break;
	}
}

void Menus_Activate( menuDef_t *menu )
{
	if ( !menu )
		return;

	//	if this is the first menu then enable the keycatcher
	if ( menuStack->count == 0 )
	{
#ifndef CGAME
		trap_Key_SetCatcher( KEYCATCH_UI );
#endif
		// was originially added to try to fix the double warp to npc bug
		// but the caused menus not to show up after a vid restart in some
		// situations
		//trap_Key_ClearStates();
	}

	if ( menu->window.flags & WINDOW_PAUSEGAME ) {
		trap_Cvar_Set( "cl_paused", "1" );
	}

	menu->time = DC->realTime;

	//	push this window onto the stack
	menuStack->m[ menuStack->count++ ] = menu;
	Menus_SetVisible();


	if ( menu->onOpen )
		Menu_RunScript( menu, menu->onOpen );

	//	this window has the focus
	if ( menu->onFocus )
		Menu_RunScript( menu, menu->onFocus );

	if( menu->soundName && *menu->soundName )
	{
		DC->startBackgroundTrack( menu->soundName, menu->soundName );
	}
}

static itemDef_t * Menu_GetItemUnderCursor( menuDef_t * menu, float x, float y )
{
	int i;

	// scan the items from top to bottom
	for ( i=menu->itemCount-1; i>=0; i-- )
	{
		itemDef_t*	item = menu->items[ i ];

		if ( !debugMode )
		{
			if( !(item->window.flags & (WINDOW_VISIBLE) ) )
				continue;

			if ( (item->window.flags & (WINDOW_DECORATION | WINDOW_FADINGOUT)) )
				continue;

			// items can be enabled and disabled based on cvars
			if( item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE)
				&& !Item_EnableShowViaCvar( item, CVAR_ENABLE ) )
				continue;

			if( item->cvarFlags & (CVAR_SHOW | CVAR_HIDE)
				&& !Item_EnableShowViaCvar( item, CVAR_SHOW ) )
				continue;
		}

		// first item hit, this one get the focus, really there shouldn't be overlaping items in well designed gui's
		if ( Rect_ContainsPoint( &item->window.rect, x, y ) )
			return item;
	}

	return 0;
}

//	key is assumed to be down and mouse is assumed to over item
qboolean Item_HandleKeyDown( itemDef_t * item, int key )
{
	int i;
	qboolean actionKey;

	// see if it's a focus key
	for( i=0; i<MAX_KEY_ACTIONS; i++ )
	{
		if( item->onFocusKey[ i ].key == key )
		{
			Item_RunScript( item, item->onFocusKey[ i ].action );
			return qtrue;
		}
	}

	actionKey = (key == K_MOUSE1) || (key==K_MOUSE2) || (key==K_MOUSE3) || (key==K_ENTER);


	//	play click sound
	if( actionKey && DC->Assets.itemFocusSound )
	{
		switch( item->type )
		{
		case ITEM_TYPE_LISTBOX:
			{
				listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
				if( listPtr->flags & LISTBOX_NOTSELECTABLE )
					break;
			}

		case ITEM_TYPE_YESNO:
		case ITEM_TYPE_MULTI:
		case ITEM_TYPE_BUTTON:
		case ITEM_TYPE_OWNERDRAW:
			DC->startLocalSound( DC->Assets.itemFocusSound, CHAN_LOCAL_SOUND );
			break;
		}
	}

	switch( item->type )
	{
	case ITEM_TYPE_EDITFIELD:
	case ITEM_TYPE_NUMERICFIELD:
		{
			if ( actionKey )
			{
				UI_SetEditItem( item );
				DC->setOverstrikeMode(qfalse);
				return qtrue;
			}
		} break;

	case ITEM_TYPE_LISTBOX:
		if ( Item_ListBox_HandleKey( item, key, qfalse ) )
			return qtrue;

		break;

	case ITEM_TYPE_YESNO:
		if ( item->cvar && actionKey )
		{
			DC->setCVar(item->cvar, va("%i", !DC->getCVarValue(item->cvar)));
			Item_RunScript(item, item->action);
			return qtrue;
		}
		break;
	case ITEM_TYPE_MULTI:
		{
			multiDef_t *multiPtr = (multiDef_t*)item->typeData;
			if ( multiPtr && item->cvar && actionKey ) {
				int current	= Item_Multi_FindCvarByValue(item);
				int max		= Item_Multi_CountSettings	(item);

				if ( key == K_MOUSE2 ) {
					current--;
					if ( current < 0 ) {
						current = max-1;
					}
				} else {
					current++;
					if ( current >= max ) {
						current = 0;
					}
				}

				if (multiPtr->strDef) {
					DC->setCVar(item->cvar, multiPtr->cvarStr[current]);
				} else {
					float value = multiPtr->cvarValue[current];
					if (((float)((int) value)) == value) {
						DC->setCVar(item->cvar, va("%i", (int) value ));
					} else {
						DC->setCVar(item->cvar, va("%f", value ));
					}
				}
				Item_RunScript(item, item->action);
				return qtrue;
			}
		} break;
	case ITEM_TYPE_OWNERDRAW:
		{
			if ( DC->ownerDrawHandleKey( item->window.ownerDraw, item->window.ownerDrawFlags, key ) || actionKey )
			{
				Item_RunScript(item, item->action);
				return qtrue;
			}

		} break;

	case ITEM_TYPE_BIND:
		{
			if ( actionKey )
			{
				DC->setCVar( "ui_waitingForKey", "1" );
				g_waitingForKey = qtrue;
				g_bindItem = item;
				return qtrue;

			} else if ( key == K_BACKSPACE )
			{
				int id = BindingIDFromName(item->cvar);
				if (id != -1) {
					g_bindings[id].bind1 = -1;
					g_bindings[id].bind2 = -1;
				}
				Controls_SetConfig(qtrue);
				DC->setCVar( "ui_waitingForKey", "0" );
				g_waitingForKey = qfalse;
				g_bindItem = NULL;
				return qtrue;
			}

		} break;

	case ITEM_TYPE_SLIDER:
		{
			if ( key == K_MWHEELDOWN )
				Item_Slider_Decr( item );
			else if ( key == K_MWHEELUP )
				Item_Slider_Incr( item );

		} break;
	}

	return qfalse;
}

//
//	sends input into a menu.  returns false if input should be passed on to the next menu.
//
qboolean Menu_HandleKeyDown( menuDef_t *menu, int key )
{
	int i;
	itemDef_t *item = menu->focusItem;

	//	key bind
	if( g_waitingForKey )
		return Item_Bind_HandleKey(g_bindItem, key );

	//	edit field
	if( g_editItem )
	{
		Item_TextField_HandleKey(g_editItem, key);

		if (	(isMouseKey( key ) && !Rect_ContainsPoint( &g_editItem->window.rect, DC->cursorx, DC->cursory )) ||	// clicked out side edit field
				(key == K_TAB)		||	// tabbed to next control
				(key == K_ESCAPE)	||	// escaped out of edit field
				(key == K_ENTER)	||	// finalized input
				(key == K_KP_ENTER) )
		{
			if( key != K_KP_ENTER && key != K_ENTER )
				UI_SetEditItem( NULL );

			//	all focus has been starved because this field has been capturing the focus, now
			//	that the lock has been released let the menu reset under the cursor
			Display_MouseMove( DC->cursorx, DC->cursory );

			//	all keys in this case are allowed to continue outside of this control except for
			//	escape.  escape just loses focus. to escape from the menu you have to press twice.
			if ( key == K_ESCAPE )
				return qtrue;

		} else
			return qtrue;	// input consumed.
	}

	// get the item with focus or the item being clicked
	if( isMouseKey( key ) )
	{
		itemDef_t *tmp;

		if ( !Rect_ContainsPoint( &menu->window.rect, DC->cursorx, DC->cursory ) ) {
			return qfalse;	// click outside window
		}

		tmp = Menu_GetItemUnderCursor( menu, DC->cursorx, DC->cursory );
		if ( tmp )
		{
			item = tmp;
		}
	}

	if ( item )
	{
		if ( isMouseKey( key ) )
		{
			if( !itemCapture )
			{
				Item_StartCapture( item, key );
			}
		}

		if ( debugMode )
			return qfalse;

		if ( Item_HandleKeyDown( item, key ) )
			return qtrue;

	} 
#ifdef DEVELOPER
	else if ( debugMode && !itemCapture && key == K_MOUSE1 )
	{
		//	hack to edit menus
		if ( !debug.menu || debug.menu == menu ) {
			scrollInfo.xStart = DC->cursorx - menu->window.rect.x;
			scrollInfo.yStart = DC->cursory - menu->window.rect.y;
			scrollInfo.scrollKey = DebugMode_GetScroll( &menu->window.rect, DC->cursorx, DC->cursory );
			if ( scrollInfo.scrollKey & SCROLL_DEBUGMODE_RIGHT )
				scrollInfo.xStart -= menu->window.rect.w;
			if ( scrollInfo.scrollKey & SCROLL_DEBUGMODE_BOTTOM )
				scrollInfo.yStart -= menu->window.rect.h;
			scrollInfo.item = (itemDef_t *)menu;
			captureData = &scrollInfo;
			captureFunc = &DebugMode_MoveMenu_AutoFunc;
			itemCapture = (itemDef_t*)menu;
		}
	}
#endif

	// see if it's a menu hot key
	for( i = 0; i < menu->itemCount; i++ )
	{
		int j;
		if( !(menu->items[ i ]->window.flags & WINDOW_VISIBLE ) )
			continue;

		for( j = 0; j < MAX_KEY_ACTIONS; j++ )
			if( menu->items[ i ]->onHotKey[ j ].key == key )
			{
				const char * script = menu->items[ i ]->onHotKey[ j ].action;

				if ( script && script[0] ) {
					Item_RunScript( menu->items[ i ], script );
				} else {
					Item_RunScript( menu->items[ i ], menu->items[ i ]->action );
				}

				return qtrue;
			}
	}

	//	check menu key scripts
	for( i = 0; i < MAX_KEY_ACTIONS; i++ )
		if( menu->onMenuKey[i].key == key )
		{
			if( menu->onMenuKey[i].action )
				Menu_RunScript( menu, menu->onMenuKey[i].action );

			return qtrue;
		}


	// check for tabbing
	if ( key == K_TAB ) {
		(( trap_Key_IsDown( K_SHIFT ) )?Menu_SetPrevCursorItem:Menu_SetNextCursorItem)( menu );
		return qtrue;
	}

#if 0
	// this was added for rudimentary gamepad support.  although it messes with lists and keying up and down
	switch ( key ) {
		case K_UPARROW:
		case K_DOWNARROW:
		case K_LEFTARROW:
		case K_RIGHTARROW:
			Menu_SetFocusItem( menu, Menu_NearItem( menu, menu->focusItem, key ) );
			return qtrue;

		case K_JOY15:
			if ( menu->focusItem ) {
				itemDef_t * item = menu->focusItem;
				if ( (item->type == ITEM_TYPE_BUTTON || item->type == ITEM_TYPE_TEXT) ) {
					Item_RunScript(item, item->action);
				} else {
					Item_HandleKeyDown( menu->focusItem, K_MOUSE1 );
				}
			}
			return qtrue;
	}
#endif

	return qfalse;
}

void Item_TextColor( itemDef_t *item, vec4_t *newColor )
{
	menuDef_t *parent = (menuDef_t*)item->parent;

	Fade( &item->window.flags, &item->window.foreColor[3], parent->fadeClamp,
		&item->window.nextTime, parent->fadeCycle, qtrue, parent->fadeAmount );

	if( Item_HasFocus( item ) && item->window.flags & WINDOW_FOCUSFLASH )
	{
		Item_Color_Pulse( *newColor, parent->focusColor );
	}
	else if( item->textStyle & ITEM_TEXTSTYLE_BLINK )
	{
		Item_Color_Pulse( *newColor, item->window.foreColor );
	}
	else
	{
		memcpy( newColor, &item->window.foreColor, sizeof( vec4_t ) );
		// items can be enabled and disabled based on cvars
	}

	if( item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest )
	{
		if( item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar( item, CVAR_ENABLE ) )
		{
			memcpy( newColor, &parent->disableColor, sizeof( vec4_t ) );
		}
	}
}

void Item_Text_PaintMore( itemDef_t *item, float offset, const char* text, vec4_t color )
{
	Rectangle r;

	Item_Text_GetDataRect( item, &r, offset );

	trap_R_RenderText	(	&r,
							item->textscale,
							color,
							text,
							-1,
							TEXT_ALIGN_LEFT,
							item->textaligny,
							item->textStyle,
							item->font,
							-1,
							&item->textRect
						);
}

void Item_TextRect( itemDef_t * item, rectDef_t * r )
{
	r->x = item->window.rect.x + item->window.borderSize;
	r->y = item->window.rect.y + item->window.borderSize;
	r->w = item->window.rect.w - item->window.borderSize*2;
	r->h = item->window.rect.h - item->window.borderSize*2;

	if ( item->textdivx )
		r->w = item->textdivx - item->window.borderSize;
	else if ( item->textdivy )
		r->h = item->textdivy - item->window.borderSize;
}

void Item_Text_Paint( itemDef_t *item )
{
	char		tmp[ MAX_INFO_STRING ];
	const char*	text = 0;
	vec4_t		color;
	Rectangle	r;

	if ( item->ownerText && DC->getOwnerText )
	{
		text = DC->getOwnerText( item->ownerText );

	} else if ( item->text )
	{
		text = item->text;

	} else if ( item->cvar && item->type != ITEM_TYPE_NUMERICFIELD && item->type != ITEM_TYPE_EDITFIELD )
	{
		DC->getCVarString( item->cvar, tmp, sizeof(tmp) );
		text = tmp;

	}

	if ( !text || *text == '\0' )
	{
		item->textRect = item->window.rect;
		item->textRect.w = 0;

		return;
	}

	Item_TextColor( item, &color );
	Item_TextRect( item, &r );

	if ( item->window.flags&WINDOW_TEXTSCROLL) {

		menuDef_t * menu = item->parent;
		float t = min( r.h*3.1f, (DC->realTime - menu->time) * 0.03f ) - item->window.rect.h;
		r.y -= t;
	}

	trap_R_SetColor( item->window.backColor );
	trap_R_RenderText(		&r,
							item->textscale,
							color,
							text,
							-1,
							item->textalignx,
							item->textaligny,
							item->textStyle,
							item->font,
							-1,
							&item->textRect
						);
}

void AdjustFrom640(float *x, float *y, float *w, float *h);

void Item_TextField_Paint(itemDef_t *item)
{
	editFieldDef_t*	editPtr = (editFieldDef_t*)item->typeData;
	Rectangle		r;	// edit region

	Item_Text_Paint(item);

	if (item->cvar)
	{
		char tmp[ MAX_INFO_STRING ];
		int cursor = -1;

		DC->getCVarString( item->cvar, tmp, sizeof(tmp) );
		if (item->cvarFlags & CVAR_PASSWORD)
		{
			int i;
			for( i=0; tmp[i]; i++)
				tmp[i]='*';
		}

		if ( item == g_editItem )
		{
			cursor = item->cursorPos - editPtr->paintOffset;
			if ( cursor < 0 )
				cursor = 0;
		}

		if (editPtr->flags&EDITFIELD_CDKEY) {
			char t[ 32 ];
			int i;
			for ( i=0; i<16; i++ ) {
				if ( tmp[i] >= 'a' && tmp[i] <= 'z' ) {
					t[ i ] = (tmp[i]-'a') + 'A';
				} else {
					t[ i ] = tmp[ i ];
				}
			}

			Com_Memcpy( tmp, t, 4 );
			tmp[ 4 ] = '-';
			Com_Memcpy( tmp+5, t+4, 4 );
			tmp[ 9 ] = '-';
			Com_Memcpy( tmp+10, t+8, 4 );
			tmp[ 14 ] = '-';
			Com_Memcpy( tmp+15, t+12, 4 );

			cursor += cursor/4;
		}

		Item_Text_GetDataRect( item, &r, 8 );

		if ( item->window.flags & WINDOW_NOFOCUS ) {
			float b = item->window.borderSize;
			DC->setColor( item->window.backColor );
			DC->drawHandlePic( r.x-b, r.y, r.w+b*2.0f, r.h, uiInfo.uiDC.whiteShader );
			DC->drawRect( r.x-b, r.y, r.w+b*2.0f, r.h, 2.0f, item->window.outlineColor );
		}

		trap_R_RenderText	(	&r,
								item->textscale,
								item->window.foreColor,
								((editPtr->flags&EDITFIELD_MONEY)?fn( atoi(tmp), FN_CURRENCY ):
								(tmp + editPtr->paintOffset)),
								editPtr->maxPaintChars,
								ITEM_ALIGN_LEFT,
								item->textaligny,
								item->textStyle,
								item->font,
								cursor,
								&item->textRect
							);
	}
}

void Item_YesNo_Paint(itemDef_t *item)
{
	float	value = (item->cvar) ? DC->getCVarValue(item->cvar) : 0.0f;
	if ( item->text )
		Item_Text_Paint( item );

	Item_Text_PaintMore( item, (item->text)?8:0, DC->getOwnerText((value!=0.0f)?uiInfo.T_Yes:uiInfo.T_No), item->window.foreColor );
}

void Item_Multi_Paint(itemDef_t *item) {

	if ( item->values ) {

		multiDef_t *multiPtr;
		char tmp[ 8192 ];
		cell_t* cells;
		int i;

		Item_ValidateTypeData(item);
		multiPtr = (multiDef_t*)item->typeData;

		multiPtr->count = trap_SQL_Select( item->values, tmp, sizeof(tmp) );
		cells	= (cell_t*)tmp;

		for ( i=0; i<multiPtr->count; i++ ) {
			multiPtr->cvarStr[ i ]	= String_Alloc(cells[i*2+0].string);
			multiPtr->cvarList[ i ] = String_Alloc(cells[i*2+1].string);
		}
		multiPtr->strDef = qtrue;
	}

	if (item->text)
		Item_Text_Paint(item);

	Item_Text_PaintMore( item, (item->text)?8:0, Item_Multi_Setting(item), item->window.foreColor );  
}

static void drawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader )
{
	AdjustFrom640( &x,&y,&w,&h );
	DC->drawStretchPic( x,y,w,h, s1,t1, s2,t2, hShader );
}

void Item_Progress_Paint( itemDef_t * item )
{
	progressDef_t * pd = (progressDef_t*)item->typeData;
	float	x		= item->window.rect.x + item->window.borderSize;
	float	y		= item->window.rect.y;
	float	w		= item->window.rect.w - item->window.borderSize*2.0f;
	float	h		= item->window.rect.h;
	float	u;
	int		used	= 0;
	int		total	= 0;


	if ( item->values ) {
		if ( trap_SQL_Prepare( item->values ) ) {
			trap_SQL_Step();
			used	= trap_SQL_ColumnAsInt( 0 );
			total	= trap_SQL_ColumnAsInt( 1 );
			trap_SQL_Done();
		}
	}

	if ( total <= 0 ) {
		return;
	}

	DC->setColor( item->window.foreColor );

	u = (w*used)/total;

	if ( u > 0.0f ) {
		drawStretchPic( x,y,u,h, 0.0f, 0.0f, (float)used, 1.0f, pd->full );
	}

	drawStretchPic( x+u,y,w-u,h, 0.0f, 0.0f, (float)(total-used), 1.0f, pd->empty );

	Item_Text_Paint( item );
}


typedef struct
{
	char*	name;
	float	defaultvalue;
	float	value;	
} configcvar_t;

/*
=================
Controls_GetKeyAssignment
=================
*/
static void Controls_GetKeyAssignment (char *command, int *twokeys)
{
	int		count;
	int		j;
	char	b[256];

	twokeys[0] = twokeys[1] = -1;
	count = 0;

	for ( j = 0; j < 256; j++ )
	{
		DC->getBindingBuf( j, b, 256 );
		if ( *b == 0 ) {
			continue;
		}
		if ( !Q_stricmp( b, command ) ) {
			twokeys[count] = j;
			count++;
			if (count == 2) {
				break;
			}
		}
	}
}

/*
=================
Controls_GetConfig
=================
*/
void Controls_GetConfig( void )
{
	int		i;
	int		twokeys[2];

	// iterate each command, get its numeric binding
	for (i=0; i < lengthof( g_bindings ); i++)
	{

		Controls_GetKeyAssignment(g_bindings[i].command, twokeys);

		g_bindings[i].bind1 = twokeys[0];
		g_bindings[i].bind2 = twokeys[1];
	}

	//s_controls.invertmouse.curvalue  = DC->getCVarValue( "m_pitch" ) < 0;
	//s_controls.smoothmouse.curvalue  = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "m_filter" ) );
	//s_controls.alwaysrun.curvalue    = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cl_run" ) );
	//s_controls.autoswitch.curvalue   = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cg_autoswitch" ) );
	//s_controls.sensitivity.curvalue  = UI_ClampCvar( 2, 30, Controls_GetCvarValue( "sensitivity" ) );
	//s_controls.joyenable.curvalue    = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "in_joystick" ) );
	//s_controls.joythreshold.curvalue = UI_ClampCvar( 0.05, 0.75, Controls_GetCvarValue( "joy_threshold" ) );
	//s_controls.freelook.curvalue     = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cl_freelook" ) );
}

/*
=================
Controls_SetConfig
=================
*/
void Controls_SetConfig(qboolean restart)
{
	int		i;

	// iterate each command, get its numeric binding
	for (i=0; i < lengthof( g_bindings ); i++)
	{

		if (g_bindings[i].bind1 != -1)
		{	
			DC->setBinding( g_bindings[i].bind1, g_bindings[i].command );

			if (g_bindings[i].bind2 != -1)
				DC->setBinding( g_bindings[i].bind2, g_bindings[i].command );
		}
	}

	//if ( s_controls.invertmouse.curvalue )
	//	DC->setCVar("m_pitch", va("%f),-fabs( DC->getCVarValue( "m_pitch" ) ) );
	//else
	//	trap_Cvar_SetValue( "m_pitch", fabs( trap_Cvar_VariableValue( "m_pitch" ) ) );

	//trap_Cvar_SetValue( "m_filter", s_controls.smoothmouse.curvalue );
	//trap_Cvar_SetValue( "cl_run", s_controls.alwaysrun.curvalue );
	//trap_Cvar_SetValue( "cg_autoswitch", s_controls.autoswitch.curvalue );
	//trap_Cvar_SetValue( "sensitivity", s_controls.sensitivity.curvalue );
	//trap_Cvar_SetValue( "in_joystick", s_controls.joyenable.curvalue );
	//trap_Cvar_SetValue( "joy_threshold", s_controls.joythreshold.curvalue );
	//trap_Cvar_SetValue( "cl_freelook", s_controls.freelook.curvalue );
	DC->executeText(EXEC_APPEND, "in_restart\n");
	//trap_Cmd_ExecuteText( EXEC_APPEND, "in_restart\n" );
}

/*
=================
Controls_SetDefaults
=================
*/
void Controls_SetDefaults( void )
{
	int	i;

	// iterate each command, set its default binding
  for (i=0; i < lengthof( g_bindings ); i++)
	{
		g_bindings[i].bind1 = g_bindings[i].defaultbind1;
		g_bindings[i].bind2 = g_bindings[i].defaultbind2;
	}

	//s_controls.invertmouse.curvalue  = Controls_GetCvarDefault( "m_pitch" ) < 0;
	//s_controls.smoothmouse.curvalue  = Controls_GetCvarDefault( "m_filter" );
	//s_controls.alwaysrun.curvalue    = Controls_GetCvarDefault( "cl_run" );
	//s_controls.autoswitch.curvalue   = Controls_GetCvarDefault( "cg_autoswitch" );
	//s_controls.sensitivity.curvalue  = Controls_GetCvarDefault( "sensitivity" );
	//s_controls.joyenable.curvalue    = Controls_GetCvarDefault( "in_joystick" );
	//s_controls.joythreshold.curvalue = Controls_GetCvarDefault( "joy_threshold" );
	//s_controls.freelook.curvalue     = Controls_GetCvarDefault( "cl_freelook" );
}

int BindingIDFromName(const char *name) {
	int i;
  for (i=0; i < lengthof( g_bindings ); i++)
	{
		if (Q_stricmp(name, g_bindings[i].command) == 0) {
			return i;
		}
	}
	return -1;
}

char g_nameBind1[32];
char g_nameBind2[32];

void BindingFromName(const char *cvar) {
	int	i, b1, b2;

	// iterate each command, set its default binding
	for (i=0; i < lengthof( g_bindings ); i++)
	{
		if (Q_stricmp(cvar, g_bindings[i].command) == 0) {
			b1 = g_bindings[i].bind1;
			if (b1 == -1) {
				break;
			}
				DC->keynumToStringBuf( b1, g_nameBind1, 32 );
				Q_strupr(g_nameBind1);

				b2 = g_bindings[i].bind2;
				if (b2 != -1)
				{
					DC->keynumToStringBuf( b2, g_nameBind2, 32 );
					Q_strupr(g_nameBind2);
					strcat( g_nameBind1, " or " );
					strcat( g_nameBind1, g_nameBind2 );
				}
			return;
		}
	}
	strcpy(g_nameBind1, "???");
}

void Item_PaintModel( qhandle_t model, qhandle_t shader,
							float fovx, float fovy, float angle,
							const rectDef_t *rc )
{
	float x, y, w, h;
	refdef_t refdef;
	refEntity_t ent;
	vec3_t mins, maxs, origin;
	vec3_t angles;
	float mW, mH, mD, l;
	
	// setup the refdef
	memset( &refdef, 0, sizeof( refdef ) );
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear( refdef.viewaxis );
	
	x = rc->x;
	y = rc->y;
	w = rc->w;
	h = rc->h;

	AdjustFrom640( &x, &y, &w, &h );

	refdef.x = (int)x;
	refdef.y = (int)y;
	refdef.width = (int)w;
	refdef.height = (int)h;

	DC->modelBounds( model, mins, maxs );

	mW = fabsf( maxs[0] - mins[0] );
	mD = fabsf( maxs[1] - mins[1] );
	mH = fabsf( maxs[2] - mins[2] );
	if ( mW > mD )
	{
		if ( mW > mH )
			l = mW;
		else
			l = mH;
	} else
	{
		if ( mD > mH )
			l = mD;
		else
			l = mH;
	}

	origin[2] = -0.5f * (mins[2] + maxs[2]);
	origin[1] = 0.5f * (mins[1] + maxs[1]);

	// calculate distance so the model nearly fills the box
	origin[0] = l*0.7f / tanf( 45.0f * 0.5f );

	refdef.fov_x = 45.0f;
	refdef.fov_y = 45.0f;


	DC->clearScene();

	refdef.time = DC->realTime;

	// add the model

	memset( &ent, 0, sizeof(ent) );

	VectorSet( angles, 0, angle, 0 );
	AnglesToAxis( angles, ent.axis );

	ent.hModel = model;
	ent.customShader = shader;
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );

	DC->addRefEntityToScene( &ent );
	DC->renderScene( &refdef );
}



void Item_Slider_Paint(itemDef_t *item) {
	float			x;
	editFieldDef_t*	editDef = item->typeData;
	float			value, range;

	//	draw label
	if (item->text) {
		Item_Text_Paint(item);
	}

	if ( editDef == NULL && item->cvar) {
		value = 0.0f;
		range = 0.0f;
	} else
	{
		value = min( editDef->maxVal, max( editDef->minVal, DC->getCVarValue(item->cvar) ) );
		range = (value - editDef->minVal) / (editDef->maxVal-editDef->minVal);
	}

	if ( item->window.name ) {
		trap_SQL_Prepare	( "UPDATE feeders SET sel=? SEARCH name $;" );
		trap_SQL_BindInt	( 1, (int)value );
		trap_SQL_BindText	( 2, item->window.name );
		trap_SQL_Step();
		trap_SQL_Done();
	}


	Item_Text_GetDataRect( item, &item->textRect, (item->text)?8.0f:0.0f );

	//	draw slider background
	DC->fillRect(
		item->textRect.x, item->textRect.y + 6.0F,
		item->textRect.w, item->textRect.h - 12.0F,
		item->window.backColor, DC->Assets.menu );

	//	draw + - buttons
	trap_R_RenderText( &item->textRect, item->textscale, item->window.foreColor, "<<", -1, ITEM_ALIGN_LEFT, ITEM_ALIGN_CENTER, item->textStyle, item->font, -1, 0 );
	trap_R_RenderText( &item->textRect, item->textscale, item->window.foreColor, ">>", -1, ITEM_ALIGN_RIGHT, ITEM_ALIGN_CENTER, item->textStyle, item->font, -1, 0 );

	//	draw thumb
	x = Item_Slider_ThumbPosition( item );
	DC->setColor( g_color_table[ ColorIndex( COLOR_WHITE ) ] );
	drawStretchPic( x, item->textRect.y + 2.0f, SLIDER_THUMB_WIDTH, item->textRect.h - 4.0f,
		0, 0, 1, 1, DC->Assets.sliderThumb );

	//	draw value
	if ( editDef )
	{
		rectDef_t r;
		char * t;
		r.x = x-16.0f;
		r.y = item->textRect.y;
		r.h = item->textRect.h;
		r.w = SLIDER_THUMB_WIDTH + 32.0f;

		if ( editDef->flags&EDITFIELD_PERCENT )
			t = va( "%d", (int)(range*100.0f) );
		else if ( editDef->flags&EDITFIELD_MONEY )
			t = fn( (int)value, FN_CURRENCY | FN_SHORT );
		else if ( editDef->flags&EDITFIELD_INTEGER )
			t = fn( (int)value, FN_NORMAL );
		else
			t = va( "%.2f", value );

		trap_R_RenderText( &r, item->textscale*0.75f, item->window.foreColor, t, -1, ITEM_ALIGN_CENTER, ITEM_ALIGN_CENTER, 3, item->font, -1, 0 );
	}

}

void Item_Bind_Paint(itemDef_t *item)
{
	if ( item->text )
	{
		Item_Text_Paint(item);
		BindingFromName(item->cvar);
		Item_Text_PaintMore( item, 8, g_nameBind1, item->window.foreColor );
	}
}

qboolean Display_KeyBindPending( void )
{
	return g_waitingForKey;
}

qboolean Item_Bind_HandleKey(itemDef_t *item, int key ) {
	int			id;
	int			i;

	if ( g_waitingForKey )
	{
		if (!g_waitingForKey || g_bindItem == NULL) {
			return qtrue;
		}

		if (key & K_CHAR_FLAG) {
			return qtrue;
		}

		switch (key)
		{
			case K_ESCAPE:
				DC->setCVar( "ui_waitingForKey", "0" );
				g_waitingForKey = qfalse;
				return qtrue;

			case '`':
				return qtrue;
		}
	}

	if (key != -1)
	{

		for (i=0; i < lengthof( g_bindings ); i++)
		{

			if (g_bindings[i].bind2 == key) {
				g_bindings[i].bind2 = -1;
			}

			if (g_bindings[i].bind1 == key)
			{
				g_bindings[i].bind1 = g_bindings[i].bind2;
				g_bindings[i].bind2 = -1;
			}
		}
	}


	id = BindingIDFromName(item->cvar);

	if (id != -1) {
		if (key == -1) {
			if( g_bindings[id].bind1 != -1 ) {
				DC->setBinding( g_bindings[id].bind1, "" );
				g_bindings[id].bind1 = -1;
			}
			if( g_bindings[id].bind2 != -1 ) {
				DC->setBinding( g_bindings[id].bind2, "" );
				g_bindings[id].bind2 = -1;
			}
		}
		else if (g_bindings[id].bind1 == -1) {
			g_bindings[id].bind1 = key;
		}
		else if (g_bindings[id].bind1 != key && g_bindings[id].bind2 == -1) {
			g_bindings[id].bind2 = key;
		}
		else {
			DC->setBinding( g_bindings[id].bind1, "" );
			DC->setBinding( g_bindings[id].bind2, "" );
			g_bindings[id].bind1 = key;
			g_bindings[id].bind2 = -1;
		}						
	}

	Controls_SetConfig(qtrue);	
	DC->setCVar( "ui_waitingForKey", "0" );
	g_waitingForKey = qfalse;

	return qtrue;
}



void AdjustFrom640(float *x, float *y, float *w, float *h) {
	//*x = *x * DC->scale + DC->bias;
	*x = *x * DC->glconfig.xscale + DC->glconfig.xbias;
	*y *= DC->glconfig.yscale;
	*w *= DC->glconfig.xscale;
	*h *= DC->glconfig.yscale;
}

void Item_Model_Paint( itemDef_t *item )
{
	modelDef_t *md = (modelDef_t*)item->typeData;

	// use item storage to track
	if( md->rotationSpeed )
	{
		if( DC->realTime > item->window.nextTime )
		{
			item->window.nextTime = DC->realTime + md->rotationSpeed;
			md->angle = (int)(md->angle + 1) % 360;
		}
	}

	Item_PaintModel( item->asset, item->assetShader, md->fov_x,
		md->fov_y, md->angle, &item->window.rect );
}


void Item_Image_Paint(itemDef_t *item) {
	if (item == NULL) {
		return;
	}
	DC->drawHandlePic(item->window.rect.x+1, item->window.rect.y+1, item->window.rect.w-2, item->window.rect.h-2, item->asset);
}

void Item_ListBox_ValidateScroll( itemDef_t *item, int rows, int count )
{
	listBoxDef_t *lp = (listBoxDef_t*)item->typeData;
	int scroll = max( 0, count-rows );

	if( lp->startPos < 0 )
		lp->startPos = 0;
	if( lp->startPos > scroll )
		lp->startPos = scroll;
}

void Item_TableCell_Rect( itemDef_t * item, int x, int y, qboolean scrollBar, rectDef_t * r )
{
	listBoxDef_t *	listPtr = (listBoxDef_t*)item->typeData;

	float oy = 0.0f; //fmodf( (item->window.rect.h - listPtr->titlebar), listPtr->elementHeight ) * 0.5f;

	if ( listPtr->flags & LISTBOX_ROTATE ) {
		if ( x == -1 ) { 
			r->y = item->window.rect.y + listPtr->titlebar;
			r->h = item->window.rect.h - listPtr->titlebar;
		} else {
			columnInfo_t *	colInfo = listPtr->columnInfo + x;
			r->y = item->window.rect.y + colInfo->pos;
			r->h = colInfo->width;
		}

		if ( y == -1 ) {
			r->x = item->window.rect.x;
			r->w = item->window.rect.w;
		} else {
			r->x = item->window.rect.x + (y * listPtr->elementHeight );
			r->w = listPtr->elementHeight;
		}
		return;
	}

	if ( x == -1 )
	{
		r->x = item->window.rect.x;
		r->w = item->window.rect.w - ((scrollBar)?SCROLLBAR_SIZE+4.0f:0.0f);
	} else
	{
		columnInfo_t *	colInfo = listPtr->columnInfo + x;

		r->x = item->window.rect.x + colInfo->pos + item->window.borderSize*0.5f;
		r->w = colInfo->width - item->window.borderSize;

		if ( x == 0 )	// first column
		{
			r->x += item->window.borderSize;
			r->w -= item->window.borderSize;
		}
		
		if ( x == listPtr->numColumns-1 )	// last column
		{
			float edge = item->window.rect.x + item->window.rect.w - (((scrollBar)?SCROLLBAR_SIZE+6.0f:0.0f)+ item->window.borderSize);
			if ( r->x + r->w > edge ) {
				r->w = edge-r->x;
			}
		}
	}

	if ( y == -1 )
	{
		r->y = oy+item->window.rect.y + listPtr->titlebar;// + item->window.borderSize;
		r->h = item->window.rect.h - listPtr->titlebar;// - item->window.borderSize*2.0f;
	} else
	{
		r->y = oy+item->window.rect.y + listPtr->titlebar + (y * listPtr->elementHeight );
		r->h = listPtr->elementHeight;
	}
}

void Item_TableCellText_PaintColor( itemDef_t * item, rectDef_t * cell, int column, const char * text, vec4_t color, float scale )
{
	listBoxDef_t *	listPtr = (listBoxDef_t*)item->typeData;
	columnInfo_t *	colInfo = listPtr->columnInfo + column;

	if ( column < listPtr->numColumns )
	{
		trap_R_SetColor		(	item->window.backColor );
		trap_R_RenderText	(	cell,
								((colInfo->textscale>0.0f)?colInfo->textscale:item->textscale) * scale,
								color,
								text,
								-1,
								colInfo->horizAlign | colInfo->nowrap,
								colInfo->vertAlign,
								(colInfo->textStyle)?colInfo->textStyle:item->textStyle,
								(colInfo->font)?colInfo->font:item->font,
								-1,
								0
							);
	}
}

void Item_TableCellText_Paint( itemDef_t * item, rectDef_t * cell, int column, const char * text )
{
	Item_TableCellText_PaintColor( item, cell, column, text, item->window.foreColor, 1.0f );
}

static void Item_ListBox_Paint_Empty( itemDef_t * item, listBoxDef_t * listPtr )
{
	const char * text = item->text;
	if ( item->ownerText ) {
		text = DC->getOwnerText( item->ownerText );
	}

	if ( text && text[ 0 ] )
	{
		trap_R_RenderText(	&item->window.rect,
							item->textscale,
							item->window.foreColor,
							text,
							-1,
							TEXT_ALIGN_CENTER,
							TEXT_ALIGN_CENTER,
							item->textStyle,
							item->font,
							-1,
							0 );
	}

	if ( listPtr->selectButton )
		listPtr->selectButton->window.flags &= ~WINDOW_VISIBLE;
}

static void Item_ListBox_Paint_ScrollBar( itemDef_t * item, listBoxDef_t * listPtr, int rows, int count )
{
	rectDef_t b, t, ti;

	Item_ListBox_GetScrollBar( item, listPtr, &b, &t, rows, count );
	
	/*
		If we need to switch between small/med/tall images
		this is the spot to check the aspect ratio on t.
	*/

	ti.x = t.x;
	ti.w = t.w;
	ti.h = t.w * 2.0F;

	if( ti.h > t.h )
		ti.h = t.h;

	ti.y = (t.y + t.y + t.h - ti.h) * 0.5F;

	DC->fillRect( b.x, b.y, b.w, b.h, item->window.backColor, DC->Assets.menu );	//	draw bar
	DC->fillRect( t.x, t.y, t.w, t.h, item->window.outlineColor, DC->Assets.menu );	//	draw thumb
	DC->drawHandlePic( ti.x, ti.y, ti.w, ti.h, DC->Assets.sbThumb ); //	draw arrows
}

static void Item_ListBox_Paint_Grid( itemDef_t * item, listBoxDef_t * listPtr, int rows, int scrollBar )
{
	int			i;
	rectDef_t	r,a,b;
	DC->setColor( item->window.color );

	//	columns
	if ( !(listPtr->flags & LISTBOX_NOVERTLINES) ) {
		for ( i=1; i<listPtr->numColumns; i++ )
		{
			Item_TableCell_Rect( item, i-1, -1, scrollBar, &a );
			Item_TableCell_Rect( item, i, -1, scrollBar, &b );

			r.x = (a.x + a.w + b.x - 1.5f) * 0.5f;
			r.w = 1.5f;
			r.y = a.y + 6.0f - listPtr->titlebar;
			r.h = a.h - 12.0f + listPtr->titlebar;
			AdjustFrom640( &r.x, &r.y, &r.w, &r.h );
			DC->drawStretchPic( r.x, r.y, r.w, r.h, 0.0f, 0.0f, 1.0f, 1.0f, DC->whiteShader );
		}
	}

	if ( listPtr->titlebar > 0 )
	{
		Item_TableCell_Rect( item, -1, 0, scrollBar, &r );
		r.x += 2.0f;
		r.w -= 4.0f;
		r.y = item->window.rect.y + listPtr->titlebar - (1.5f)*0.5f;
		r.h = 1.5f;
		AdjustFrom640( &r.x, &r.y, &r.w, &r.h );
		DC->drawStretchPic( r.x, r.y, r.w, r.h, 0.0f, 0.0f, 1.0f, 1.0f, DC->whiteShader );
	}

	if ( !(listPtr->flags & LISTBOX_NOHORIZLINES) ) {
		for ( i=1; i<rows; i++ )
		{
			Item_TableCell_Rect( item, -1, i-1, scrollBar, &a );
			Item_TableCell_Rect( item, -1, i, scrollBar, &b );

			r.x = a.x+2.0f;
			r.y = (a.y + a.h + b.y - 1.5f) * 0.5f;
			r.w = a.w-4.0f;
			r.h = 1.5f;

			AdjustFrom640( &r.x, &r.y, &r.w, &r.h );
			DC->drawStretchPic( r.x, r.y, r.w, r.h, 0.0f, 0.0f, 1.0f, 1.0f, DC->whiteShader );
		}
	}
}

float Item_TableSelection_GetFade( itemDef_t * item, int i, int cursel )
{
	listBoxDef_t * listPtr = (listBoxDef_t*)item->typeData;

	if ( listPtr->flags & LISTBOX_NOTSELECTABLE )
		return 0.0f;
	else if ( i == cursel )
		return listPtr->fade;
	else if ( listPtr->fade < 1.0f && i == listPtr->lastSel )
		return 1.0f - listPtr->fade;
	else
		return 0.0f;
}

static void SCR_FillBar( float x, float y, float w, float h, float by, float bh, qhandle_t shader ) {
	float	rx,ry;
	float	cw,ch;
	float	y1,y2;
	float	tx,ty;
	
	x -= cl_cornersize.value*0.25f;
	y -= cl_cornersize.value*0.25f;
	w += cl_cornersize.value*0.5f;
	h += cl_cornersize.value*0.5f;
	

	x = x*DC->glconfig.xscale + DC->glconfig.xbias;
	y *= DC->glconfig.yscale;
	w *= DC->glconfig.xscale;
	h *= DC->glconfig.yscale;

	by *= DC->glconfig.yscale;
	bh *= DC->glconfig.yscale;

	rx = cl_cornersize.value * DC->glconfig.xscale;
	ry = cl_cornersize.value * DC->glconfig.yscale;

	// find corner size
	if ( rx * 2.0f >= w )
		rx = w*0.5f;

	if ( ry * 2.0f >= h )
		ry = h*0.5f;

	if ( rx < ry )
		ry = rx;
	if ( ry < rx )
		rx = ry;


	cw = w-rx*2.0f;
	ch = h-ry*2.0f;

	tx = (rx+cw)/rx;
	ty = (ry+ch)/ry;

	y1 = ((by-y)/(ry+ch));
	y2 = (((by+bh)-y)/(ry+ch));

	// corners
	if ( y1 < 1.0f && y2 < 1.0f ) {
		DC->drawStretchPic( x,		by,		rx+cw,	bh,		0.0f,	y1*ty, tx,		y2*ty,		shader );
		DC->drawStretchPic( x+w-rx,	by,		rx,		bh,		1.0f,	y1*ty, 0.0f,	y2*ty,		shader );
	} else {

		if ( y1 < 1.0f ) {
			DC->drawStretchPic( x,		by,		rx+cw,	y+ry+ch-by,	0.0f,	y1*ty, tx,	ty,		shader );
			DC->drawStretchPic( x+w-rx,	by,		rx,		y+ry+ch-by,	1.0f,	y1*ty, 0.0f,ty,		shader );
			bh = (by+bh)-(y+ry+ch);
			by = y+ry+ch;
		}

		y1 = 1.0f - (((by-y)-(ry+ch))/ry);
		y2 = 1.0f - ((((by+bh)-y)-(ry+ch))/ry);

		DC->drawStretchPic( x,		by,		rx+cw,	bh,		0.0f,	y1, tx,		y2,		shader );
		DC->drawStretchPic( x+w-rx,	by,		rx,		bh,		1.0f,	y1, 0.0f,	y2,		shader );
	}


	//DC->drawStretchPic( x,		y,		rx+cw,	ry+ch,	0.0f, 0.0f, (rx+cw)/rx,	(ry+ch)/ry,		shader );
	//DC->drawStretchPic( x,		y+h-ry,	rx+cw,	ry,		0.0f, 1.0f, (rx+cw)/rx,	0.0f,			shader );

	//DC->drawStretchPic( x+w-rx,	y,		rx,		ry+ch,	1.0f, 0.0f, 0.0f,		(ry+ch)/ry,		shader );
	//DC->drawStretchPic( x+w-rx,	y+h-ry,	rx,		ry,		1.0f, 1.0f, 0.0f,		0.0f,			shader );
}


static void Item_ListBox_Paint_Selection( itemDef_t * item, listBoxDef_t * listPtr, int rows, int scrollBar, int sel, float fade )
{
	vec4_t		color;
	int			i;
	rectDef_t	r;

	color[0] = item->window.color[0];
	color[1] = item->window.color[1];
	color[2] = item->window.color[2];
	color[3] = item->window.color[3]*0.75f;

	//	Fade in new selection
	i = sel - listPtr->startPos;
	if ( i >= 0 && i < rows )
	{
		color[ 3 ] *= fade;
		Item_TableCell_Rect	( item, -1, i, scrollBar, &r ); 
		//r.x += 2.0f;
		//r.w -= 4.0f;
		DC->setColor( color );
		//DC->drawHandlePic( r.x, r.y, r.w, r.h, (listPtr->selectshader)?listPtr->selectshader:DC->whiteShader );
		SCR_FillBar( r.x, item->window.rect.y, r.w, item->window.rect.h, r.y, r.h, (listPtr->selectshader)?listPtr->selectshader:DC->Assets.menubar );

		if ( listPtr->selectButton )
		{
			if ( listPtr->flags & LISTBOX_ROTATE ) {
				listPtr->selectButton->window.rect.y = r.y + r.h - (listPtr->selectButton->window.rect.h + 4.0f);
				listPtr->selectButton->window.rect.x = r.x + ((r.w - listPtr->selectButton->window.rect.w) * 0.5f);
			} else {
				listPtr->selectButton->window.rect.x = r.x + r.w - (listPtr->selectButton->window.rect.w + 4.0f);
				listPtr->selectButton->window.rect.y = r.y + ((r.h - listPtr->selectButton->window.rect.h) * 0.5f);
			}

			listPtr->selectButton->window.flags |= WINDOW_VISIBLE;
		}

	} else if ( listPtr->selectButton )
	{
		listPtr->selectButton->window.flags &= ~WINDOW_VISIBLE;
	}

	//	Fade out old selection
	if ( fade < 1.0f )
	{
		i = listPtr->lastSel - listPtr->startPos;
		if ( i >=0 && i < rows )
		{
			color[ 3 ] *= (1.0f - fade);
			Item_TableCell_Rect	( item, -1, i, scrollBar, &r ); 
			//r.x += 2.0f;
			//r.w -= 4.0f;
			DC->setColor( color );
			//DC->drawHandlePic( r.x, r.y, r.w, r.h, (listPtr->selectshader)?listPtr->selectshader:DC->whiteShader );
			SCR_FillBar( r.x, item->window.rect.y, r.w, item->window.rect.h, r.y, r.h, (listPtr->selectshader)?listPtr->selectshader:DC->Assets.menubar );
		}
	} else
		listPtr->lastSel = sel;
}

static void Item_ListBox_Paint_TitleBar( itemDef_t * item, listBoxDef_t * listPtr, int scrollBar, int rows )
{
	int i;
	for ( i=0; i<listPtr->numColumns; i++ )
	{
		const columnInfo_t * colInfo = listPtr->columnInfo + i;
		const char * text = colInfo->text;

		if ( colInfo->ownerText ) {
			text = DC->getOwnerText( colInfo->ownerText );
		}

		if ( text && listPtr->titlebar > 0 )
		{
			rectDef_t r;
			Item_TableCell_Rect	(	item, i,0, scrollBar, &r );
			r.y = item->window.rect.y;
			r.h = listPtr->titlebar;
			trap_R_RenderText	(	&r,
									item->textscale,
									item->window.foreColor,
									text,
									-1,
									colInfo->horizAlign,
									ITEM_ALIGN_CENTER,
									item->textStyle | ITEM_TEXTSTYLE_ITALIC,
									item->font,
									-1,0 );
		}

		if ( colInfo->footer )
		{
			rectDef_t r;
			const char * text = colInfo->footer;

			if ( sql( "SELECT index FROM strings SEARCH name $;", "t", text ) ) {
				int index = sqlint(0);
				sqldone();
				text = DC->getOwnerText( index );
			}

			Item_TableCell_Rect	(	item, i,rows, scrollBar, &r );
			trap_R_RenderText	(	&r,
									item->textscale,
									item->window.foreColor,
									text,
									-1,
									colInfo->horizAlign,
									ITEM_ALIGN_CENTER,
									item->textStyle,
									item->font,
									-1,0 );
		}
	}
}

static int Item_ListBox_UpdateSelection( itemDef_t * item, listBoxDef_t * listPtr ) {

	int sel;
	cell_t *	cells = (cell_t*)listPtr->buffer;

	if ( listPtr->flags & LISTBOX_NOTSELECTABLE ) {
		return -1;
	}

	trap_SQL_Prepare	( "SELECT sel FROM feeders SEARCH name $;" );
	trap_SQL_BindText	( 1, item->window.name );
	if ( trap_SQL_Step() ) {
		sel = trap_SQL_ColumnAsInt(0);
		trap_SQL_Done();
	} else
		return -1;

	if ( sel >= listPtr->numRows ) {
		sel = listPtr->numRows-1;
	}

	trap_SQL_Prepare	( "UPDATE feeders SET sel=?1,id=?2,enabled=?3 SEARCH name $4 WHERE (sel!=?1) OR (id!=?2);" );
	trap_SQL_BindInt	( 1, sel );
	if ( sel == -1 ) {
		trap_SQL_BindInt	( 2, -1	);
		trap_SQL_BindInt	( 3, -1	);
	} else {
		trap_SQL_BindInt	( 2, cells[ sel * (listPtr->numColumns+2) ].integer	);
		trap_SQL_BindInt	( 3, cells[ sel * (listPtr->numColumns+2)+1 ].integer );
	}
	trap_SQL_BindText	( 4, item->window.name );
	trap_SQL_Step		();

	if ( trap_SQL_Done() && listPtr->select && listPtr->flags&LISTBOX_MONITORTABLE ) {
		Item_RunScript( item, listPtr->select );
	}

	return sel;
}

static int Item_Chat_NumRows( const char *chatText, itemDef_t * item )
{
	fontInfo_t 		font;
	rectDef_t		r;
	listBoxDef_t *	listPtr		= (listBoxDef_t*)item->typeData;
	float			lineHeight;

	trap_R_GetFont	( item->font, item->textscale, &font );

	lineHeight = (font.glyphs[ 'A' ].height * font.glyphScale * item->textscale) * 1.4f;
	
	listPtr->elementHeight = lineHeight;

	Item_TableCell_Rect( item, 0, -1, qtrue, &r );

	return trap_R_FlowText(	&font, item->textscale * font.glyphScale, item->window.foreColor, item->window.foreColor, chatText, -1, item->textalignx, r.w, NULL, -1 );
}

static void Item_Chat_Draw( const char *chatText, itemDef_t * item, int start, int count )
{
	rectDef_t		r;

	Item_TableCell_Rect( item, 0, -1, qtrue, &r );

	trap_R_RenderText( &r, item->textscale, item->window.foreColor, chatText, -1, start, count, item->textStyle | ITEM_TEXTSTYLE_MULTILINE, item->font, -1, 0 );
}

static void drawclient( rectDef_t * r, char * model, int spin )
{
	playerInfo_t info;
	vec3_t	viewangles;
	vec3_t	moveangles;

	memset( &info, 0, sizeof(playerInfo_t) );
		
	viewangles[YAW]   = (spin)?((DC->realTime%3600) * 0.1f):(180.0f - 10.0f);
	viewangles[PITCH] = 0.0f;
	viewangles[ROLL]  = 0.0f;
	
	VectorClear( moveangles );
		
	UI_PlayerInfo_SetModel( &info, model, "", 0 );
	UI_PlayerInfo_SetInfo( &info, LEGS_IDLE, LEGS_IDLE, viewangles, vec3_origin, WP_PISTOL, qfalse );
	UI_DrawPlayer( r, &info, DC->realTime, 1.0f );
}


static char chatbuf[1024 * 4];

void Item_ListBox_Paint( itemDef_t *item )
{
	int			rows, count, i,j;
	qboolean	scrollBar = qfalse;
	int			view;
	int			sel = -1;
	int			last_rowcount;
	menuDef_t *	menu;

	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
	cell_t *	cells;

	last_rowcount = listPtr->numRows;

	if( listPtr->flags & LISTBOX_CHATLOG )
	{
		trap_Con_GetText( chatbuf, sizeof( chatbuf ), 1 );
	}

	if ( listPtr->flags & LISTBOX_CHATLOG ) {
		listPtr->numRows = Item_Chat_NumRows( chatbuf, item );
	} else {
		if ( !listPtr->source )
			return;
		if ( !listPtr->buffer ) {
			if ( !listPtr->buffer_size ) {
				listPtr->buffer_size = 1024;
			}
			listPtr->buffer = UI_Alloc( listPtr->buffer_size );
		}
		if ( listPtr->flags&LISTBOX_SERVER ) {
			listPtr->numRows = trap_SQL_SelectFromServer( listPtr->source, listPtr->buffer, listPtr->buffer_size );
		} else {
			listPtr->numRows = trap_SQL_Select( listPtr->source, listPtr->buffer, listPtr->buffer_size );
		}
	}

	cells	= (cell_t*)listPtr->buffer;
	count	= listPtr->numRows;
	sel		= Item_ListBox_UpdateSelection( item, listPtr );
	if ( count <= 0 )
	{
		Item_ListBox_Paint_Empty( item, listPtr );
		return;
	}
	rows	= Item_ListBox_NumRows( item, listPtr );		// number of rows that can be seen

	Item_ListBox_ValidateScroll( item, rows, count );

	if ( listPtr->flags & LISTBOX_AUTOSCROLL ) {
		if ( listPtr->startPos + rows == (last_rowcount-1) ) {
			listPtr->startPos = (listPtr->numRows-1) - rows;
		}
	}

	//	Scroll Bar
	if( count > rows )
	{
		item->window.flags |= WINDOW_HASSCROLLBAR;
		if ( !(listPtr->flags & LISTBOX_NOSCROLLBAR) ) {
			Item_ListBox_Paint_ScrollBar( item, listPtr, rows, count );
		}
		scrollBar = qtrue;
	} else {
		item->window.flags &= ~WINDOW_HASSCROLLBAR;
	}

	//	grid
	if ( !(listPtr->flags & LISTBOX_NOGRIDLINES) )
		Item_ListBox_Paint_Grid( item, listPtr, min( rows, count+1 ), scrollBar );

	//	selection
	if ( !(listPtr->flags & LISTBOX_NOTSELECTABLE) && !(listPtr->flags & LISTBOX_DONTDRAWSELECT) )
	{
		listPtr->fade	= min( 1.0f, (DC->realTime - listPtr->timeSel)* ( 1.0f / (float)(ITEM_FADE_TIME) ) );
		Item_ListBox_Paint_Selection( item, listPtr, rows, scrollBar, sel, listPtr->fade );

	} else
	{
		listPtr->fade	= 0.0f;
	}

	//	title bar
	Item_ListBox_Paint_TitleBar( item, listPtr, scrollBar, rows );

	view = min( rows, count-listPtr->startPos );

	if ( listPtr->flags & LISTBOX_CHATLOG ) {
		Item_Chat_Draw( chatbuf, item, listPtr->startPos, view );
		return;
	}
	
	menu = ((menuDef_t*)item->parent);
	for ( i=0; i<view; i++ )
	{
		cell_t *	row		= &cells[ (listPtr->startPos+i)*(listPtr->numColumns+2) ];
		float		offset;
		float		scale;
		float *		color;
		
		if ( listPtr->flags & LISTBOX_NOSCALESELECT ) {
			offset	= 0.0f;
			scale	= 1.0f;
		} else if ( listPtr->flags & LISTBOX_SCALEENABLED && row[ 1 ].integer ) {
			offset	= -0.15f;
			scale	= 1.15f;
		} else {
			offset	= Item_TableSelection_GetFade( item, listPtr->startPos + i, sel ) * 0.15f;
			scale	= 1.0f + offset;
		}

		if ( listPtr->flags & LISTBOX_SELECTENABLED ) {
			if ( row[ 1 ].integer ) {
				vec4_t		color;
				rectDef_t r;

				color[0] = item->window.color[0];
				color[1] = item->window.color[1];
				color[2] = item->window.color[2];
				color[3] = item->window.color[3]*0.75f;
				Item_TableCell_Rect( item, -1,i, scrollBar, &r );
				r.x += 2.0f;
				r.w -= 4.0f;
				DC->setColor( color );
				DC->drawHandlePic( r.x, r.y, r.w, r.h, DC->whiteShader );
			}
			color = item->window.foreColor;
		} else {
			color = ( row[ 1 ].integer == 0 )?menu->disableColor:item->window.foreColor;
		}

		for ( j=0; j<listPtr->numColumns; j++ )
		{
			rectDef_t r;
			columnInfo_t * c = listPtr->columnInfo + j;
			cell_t * cell = &row[ j + 2 ];

			Item_TableCell_Rect( item, j,i, scrollBar, &r );

			r.x += c->border;
			r.w -= c->border*2.0f;
			r.y += c->border;
			r.h -= c->border*2.0f;

			if( c->flags & (COL_ISSHADER|COL_ISEFFECT|COL_ISSPRITE) )
			{
				float w = r.w;
				float h = r.h;

				if ( c->flags & COL_SQUARE ) {
					if ( r.w > r.h ) {
						r.w = r.h;
					} else if ( r.h > r.w ) {
						r.h = r.w;
					}
				}

				switch ( c->horizAlign ) {
					case ITEM_ALIGN_CENTER:	r.x += (w-r.w)*0.5f;	break;
					case ITEM_ALIGN_RIGHT:	r.x += (w-r.w);			break;
				}

				switch ( c->vertAlign ) {
					case ITEM_ALIGN_CENTER:	r.y += (h-r.h)*0.5f;	break;
					case ITEM_ALIGN_RIGHT:	r.y += (h-r.h);			break;
				}

				if ( c->flags & COL_ISEFFECT ) {
					UI_Effect_SetRect( cell->integer, &r, item );
				} else {
					qhandle_t s = -1;

					if ( c->flags&COL_ISSPRITE && cell->integer >= 0 && cell->integer < lengthof(c->sprites) ) {
						s = c->sprites[ cell->integer ];
					} else
						s = cell->integer;

					if ( s >= 0 ) {
						DC->setColor( color );
						DC->drawHandlePic( r.x-r.h*offset*0.5f, r.y-r.h*offset*0.5f, r.h*scale, r.h*scale, s );
					}
				}

			} else if ( c->flags & COL_ISCOUNTER ) {

				int v = cell->integer;
				float t = (float)(DC->realTime - menu->time)/2000.0f;

				if ( t < 1.0f ) {
					v = (int)( (float)v * t );
				}

				Item_TableCellText_PaintColor( item, &r, j, fn( v, c->format ), color, scale );

			} else if ( c->flags & COL_ISCLIENT ) {

				drawclient( &r, cell->string, (sel - listPtr->startPos) == i );

			} else {
				Item_TableCellText_PaintColor( item, &r, j, cell->string, color, scale );
			}
		}
	}
}


void Item_OwnerDraw_Paint( itemDef_t *item )
{
	menuDef_t *parent;

	if( !item )
		return;

	parent = (menuDef_t*)item->parent;

	if( DC->ownerDrawItem )
	{
		vec4_t color;
		menuDef_t *parent = (menuDef_t*)item->parent;
		
		Fade( &item->window.flags, &item->window.foreColor[3], parent->fadeClamp, &item->window.nextTime, parent->fadeCycle, qtrue, parent->fadeAmount );
		memcpy( &color, &item->window.foreColor, sizeof( color ) );

		if( Item_HasFocus( item ) )
		{
			Item_Color_Pulse( color, parent->focusColor );
		}
		else if( item->textStyle & ITEM_TEXTSTYLE_BLINK && !((DC->realTime / BLINK_DIVISOR) & 1) )
		{
			Item_Color_Pulse( color, item->window.foreColor );
		}

		if( item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE) && !Item_EnableShowViaCvar( item, CVAR_ENABLE ) )
		{
			memcpy( color, parent->disableColor, sizeof( vec4_t ) ); // bk001207 - FIXME: Com_Memcpy
		}

		if( item->text )
		{
			Item_Text_Paint( item );
		}

		DC->ownerDrawItem( item, color );
	}
}

#ifdef DEVELOPER
void DebugMode_PaintWindowSize( const char * name, rectDef_t * r, rectDef_t * parent, int s )
{
	const float b = 1.0f;
	DC->drawRect(	r->x,r->y,r->w,r->h, b, ( s==SCROLL_DEBUGMODE_MOVE )?colorGreen:((parent)?colorRed:colorYellow) );

	DC->setColor( colorGreen );
	if ( s & SCROLL_DEBUGMODE_LEFT )
		DC->drawHandlePic( r->x, r->y, b, r->h, DC->whiteShader );

	if ( s & SCROLL_DEBUGMODE_RIGHT )
		DC->drawHandlePic( r->x + r->w - b, r->y, 1.0f, r->h, DC->whiteShader );

	if ( s & SCROLL_DEBUGMODE_TOP )
		DC->drawHandlePic( r->x, r->y, r->w, b, DC->whiteShader );

	if ( s & SCROLL_DEBUGMODE_BOTTOM )
		DC->drawHandlePic( r->x, r->y+r->h-1.0f, r->w, b, DC->whiteShader );

	DC->setColor( 0 );

	if ( s )
	{
		rectDef_t b = *r;
		int px = r->x - ((parent)?parent->x:0);
		int py = r->y - ((parent)?parent->y:0);
		b.x += 4.0f;
		b.y += 4.0f;
		b.w -= 8.0f;
		b.h -= 8.0f;
		trap_R_RenderText(	&b, 0.18f, colorRed, va( "%d, %d", px, py ),
								-1, ITEM_ALIGN_LEFT, ITEM_ALIGN_LEFT, 3, DC->Assets.font, -1, 0 );

		trap_R_RenderText(	&b, 0.18f, colorRed, va( "%dx%d", (int)r->w, (int)r->h ),
								-1, ITEM_ALIGN_RIGHT, ITEM_ALIGN_RIGHT, 3, DC->Assets.font, -1, 0 );

		if ( name ) {
			trap_R_RenderText( r, 0.18f, colorYellow, name, -1, ITEM_ALIGN_CENTER, ITEM_ALIGN_CENTER, 3, DC->Assets.font, -1, 0 ); 
		}
	}
}

void Item_PaintDebug( itemDef_t * item, int i )
{
	int s = 0;

	if ( itemCapture == item )
		s = scrollInfo.scrollKey;
	else if ( !itemCapture )
		s = DebugMode_GetScroll( &item->window.rect, DC->cursorx, DC->cursory );

	DebugMode_PaintWindowSize( item->window.name, &item->window.rect, &((menuDef_t*)item->parent)->window.rect, s );

	if ( s )
	{
		//trap_R_RenderText(	&item->window.rect, 0.21f, colorRed, va( "%d", i ),
		//					-1, ITEM_ALIGN_RIGHT, ITEM_ALIGN_LEFT, 3, DC->Assets.font, -1, 0 );
	}
}
#endif

void Item_Paint( itemDef_t *item )
{
	menuDef_t *parent = (menuDef_t*)item->parent;

	if( item == NULL )
		return;

	if( item->window.flags & WINDOW_ORBITING )
	{
		if( DC->realTime > item->window.nextTime )
		{
			float rx, ry, a, c, s, w, h;

			item->window.nextTime = DC->realTime + item->window.offsetTime;
			// translate
			w = item->window.rectClient.w / 2;
			h = item->window.rectClient.h / 2;
			rx = item->window.rectClient.x + w - item->window.rectEffects.x;
			ry = item->window.rectClient.y + h - item->window.rectEffects.y;
			a = 3.0f * M_PI / 180.0f;
			c = cosf(a);
			s = sinf(a);
			item->window.rectClient.x = (rx * c - ry * s) + item->window.rectEffects.x - w;
			item->window.rectClient.y = (rx * s + ry * c) + item->window.rectEffects.y - h;
			Item_UpdatePosition(item);

		}
	}

	if( item->cvarFlags & (CVAR_SHOW | CVAR_HIDE) )
	{
		if( !Item_EnableShowViaCvar( item, CVAR_SHOW ) )
		{
			return;
		}
	}

	if( !(item->window.flags & WINDOW_VISIBLE) )
	{
		return;
	}

	Window_Paint( &item->window, parent->focusItem == item, item->focusTime );

	//
	//	HiLight Edit Field
	//
	if ( item == parent->focusItem )
	{
		switch( item->type )
		{
		case ITEM_TYPE_BIND:
		case ITEM_TYPE_MULTI:
		case ITEM_TYPE_YESNO:
		case ITEM_TYPE_EDITFIELD:
		case ITEM_TYPE_NUMERICFIELD:
			{
				vec4_t newColor;
				if ( item->window.outlineColor[3] < 1.0f )
				{
					memcpy(newColor, &parent->focusColor, sizeof(vec4_t));
				} else
				{
					Item_Color_Pulse( newColor, parent->focusColor );
				}

				newColor[ 3 ] *= item->window.outlineColor[3];
				DC->fillRect( item->textRect.x-4.0f, item->window.rect.y, item->textRect.w+8.0f, item->window.rect.h, newColor, DC->Assets.menu );
			}
		}
	}

	if ( item->type == ITEM_TYPE_SLIDER || item->type == ITEM_TYPE_NUMERICFIELD ) {

		if ( item->values ) {
			if ( trap_SQL_Prepare( item->values ) ) {
				editFieldDef_t*	editDef = item->typeData;
				trap_SQL_Step();
				editDef->minVal = (float)trap_SQL_ColumnAsInt( 0 );
				editDef->maxVal = (float)trap_SQL_ColumnAsInt( 1 );
				editDef->defVal = (float)trap_SQL_ColumnAsInt( 2 );
				trap_SQL_Done();
			}
		}
	}

	switch( item->type )
	{
	case ITEM_TYPE_OWNERDRAW:
		Item_OwnerDraw_Paint( item );
		break;

	case ITEM_TYPE_TEXT:
	case ITEM_TYPE_BUTTON:
		Item_Text_Paint( item );
		break;
									 
	case ITEM_TYPE_EDITFIELD:
	case ITEM_TYPE_NUMERICFIELD:
		Item_TextField_Paint( item );
		break;

	case ITEM_TYPE_LISTBOX:
		Item_ListBox_Paint( item );
		break;

/*
	case ITEM_TYPE_IMAGE:
		Item_Image_Paint( item );
		break;
*/

	case ITEM_TYPE_YESNO:
		Item_YesNo_Paint( item );
		break;

	case ITEM_TYPE_MULTI:
		Item_Multi_Paint( item );
		break;

	case ITEM_TYPE_BIND:
		Item_Bind_Paint( item );
		break;

	case ITEM_TYPE_SLIDER:
		Item_Slider_Paint( item );
		break;

	case ITEM_TYPE_PROGRESS:
		Item_Progress_Paint( item );
		break;

	default:
		break;
	}
}

void Menu_Init(menuDef_t *menu)
{
	memset(menu, 0, sizeof(menuDef_t));
	menu->fadeAmount	= DC->Assets.fadeAmount;
	menu->fadeClamp		= DC->Assets.fadeClamp;
	menu->fadeCycle		= DC->Assets.fadeCycle;
	Window_Init(&menu->window);
}

itemDef_t *Menu_GetFocusedItem(menuDef_t *menu)
{
	return menu->focusItem;
}

menuDef_t *Menu_GetFocused()
{
	return (menuStack->count)?menuStack->m[ menuStack->count - 1 ]:0;
}

qboolean Menus_AnyFullScreenVisible() {
	int i;
	for ( i=0; i<menuStack->count; i++ )
	{
		if ( menuStack->m[ i ]->window.flags & WINDOW_VISIBLE && menuStack->m[ i ]->window.flags & WINDOW_FULLSCREEN )
			return qtrue;
	}
	return qfalse;
}

void Menus_ActivateByName(const char *p)
{
	Menus_Activate( Menus_FindByName( p ) );
}


void Item_Init(itemDef_t *item) {
	memset(item, 0, sizeof(itemDef_t));
	item->textscale = 0.55f;
	Window_Init(&item->window);
}

void Menu_HandleMouseMove( menuDef_t *menu, float x, float y )
{
	int			i;
	qboolean	focusItem	= qfalse;
	
	if( !menu || !(menu->window.flags & (WINDOW_VISIBLE)) )
		return;

	if( itemCapture )
		return;

	if( debugMode )
		return;

	if( g_waitingForKey || g_editItem )
		return;

	// scan the items from top to bottom
	for ( i=menu->itemCount-1; i>=0; i-- )
	{
		itemDef_t*	item = menu->items[ i ];

		if( !(item->window.flags & (WINDOW_VISIBLE) ) )
			continue;

		if ( item->window.flags & (WINDOW_DECORATION | WINDOW_FADINGOUT) )
			continue;

		// items can be enabled and disabled based on cvars
		if( item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE)
			&& !Item_EnableShowViaCvar( item, CVAR_ENABLE ) )
			continue;

		if( item->cvarFlags & (CVAR_SHOW | CVAR_HIDE)
			&& !Item_EnableShowViaCvar( item, CVAR_SHOW ) )
			continue;

		// first item hit, this one get the focus, really there shouldn't be overlaping items in well designed gui's
		if ( Rect_ContainsPoint( &item->window.rect, x, y ) )
		{
			if ( !(item->window.flags & WINDOW_MOUSEOVER) )
				Item_MouseEnter( item, x, y );

			if ( !focusItem )
				focusItem = Menu_SetFocusItem( menu, item );


			if ( item->type == ITEM_TYPE_OWNERDRAW )
				Item_OwnerDraw_HandleMouseMove( item,x,y );

			if ( item->window.flags & WINDOW_POPUP )
				break;	// don't let any controls under this one get messages

		} else
		{
			if ( item->window.flags & WINDOW_MOUSEOVER )
				Item_MouseLeave( item );
		}
	}

	// the mouse is not over an item, lose the focus on the old item
	if ( !focusItem && menu->focusItem )
	{
		Menu_ClearFocusItem( menu, 0 );
	}
}

void Menu_Paint(menuDef_t *menu, qboolean forcePaint) {
	int i;

	if (menu == NULL) {
		return;
	}

	if (!(menu->window.flags & WINDOW_VISIBLE) &&  !forcePaint) {
		return;
	}

	if ( menu->showif && *menu->showif ) {

		int r = 0;
		trap_SQL_Prepare( menu->showif );
		if ( trap_SQL_Step() ) {
			r = trap_SQL_ColumnAsInt(0);
			trap_SQL_Done();
		}
		if ( r == 0 )
			return;
	}


	// paint the background and or border
	Window_Paint( &menu->window, 0, 0 );

	for (i = 0; i < menu->itemCount; i++) {
		Item_Paint(menu->items[i]);
#ifdef DEVELOPER
		if ( debugMode && (!debug.menu || debug.menu==menu) )
			Item_PaintDebug( menu->items[ i ], i );
#endif
	}

	UI_Effects_Draw( menu );

#ifdef DEVELOPER
	if (debugMode && (!debug.menu || debug.menu == menu) ) {
		int s = 0;

		if ( itemCapture == (itemDef_t*)menu )
			s = scrollInfo.scrollKey;
		else if ( !itemCapture )
			s = DebugMode_GetScroll( &menu->window.rect, DC->cursorx, DC->cursory );

		DebugMode_PaintWindowSize( NULL, &menu->window.rect, 0, s );

		trap_R_RenderText(	&menu->window.rect, 0.18f, colorYellow, va( "[%s] - %s", menu->window.name, menu->filename ),
								-1, ITEM_ALIGN_LEFT, ITEM_ALIGN_LEFT, 3, DC->Assets.font, -1, 0 );
	}
#endif
}






//
//	---------------------------------------------------------------------------
//				MENU PARSING
//	---------------------------------------------------------------------------
//


/*
===============
Item_ValidateTypeData
===============
*/
void Item_ValidateTypeData(itemDef_t *item) {
	if (item->typeData) {
		return;
	}

	switch( item->type )
	{
	case ITEM_TYPE_LISTBOX:
		{
			item->typeData = UI_Alloc(sizeof(listBoxDef_t));
			memset(item->typeData, 0, sizeof(listBoxDef_t));
		} break;

	case ITEM_TYPE_EDITFIELD:
	case ITEM_TYPE_NUMERICFIELD:
	case ITEM_TYPE_YESNO:
	case ITEM_TYPE_BIND:
	case ITEM_TYPE_SLIDER:
	case ITEM_TYPE_TEXT:
		{
			item->typeData = UI_Alloc(sizeof(editFieldDef_t));
			memset(item->typeData, 0, sizeof(editFieldDef_t));
			if (item->type == ITEM_TYPE_EDITFIELD) {
				if (!((editFieldDef_t *) item->typeData)->maxPaintChars) {
					((editFieldDef_t *) item->typeData)->maxPaintChars = MAX_EDITFIELD;
				}
			}
			((editFieldDef_t *) item->typeData)->minVal = -1e5f;
			((editFieldDef_t *) item->typeData)->maxVal = +1e5f;
		} break;
	case ITEM_TYPE_MULTI:
		{
			item->typeData = UI_Alloc(sizeof(multiDef_t));
		} break;
	case ITEM_TYPE_MODEL:
		{
			item->typeData = UI_Alloc(sizeof(modelDef_t));
		} break;
	case ITEM_TYPE_PROGRESS:
		{
			item->typeData = UI_Alloc(sizeof(progressDef_t));
		} break;
	}
}

static field_t * Find_FieldRange( field_t * fields, int count, int type, int * n )
{
	int i;
	int s;
	for ( i=0; i<count; i++ )
	{
		if ( fields[ i ].type == type )
		{
			i++;
			break;
		}
	}

	if ( i == count )
		return 0;

	s = i;

	for ( ; i<count; i++ )
	{
		if ( fields[ i ].type >= FIELD_WINDOW  )
			break;
	}

	*n = i - s;
	return fields + s;
}

static field_t * Find_Field( const char * name, field_t * fields, int count )
{
	int i;
	for ( i=0; i<count; i++ )
	{
		if ( !Q_stricmp( fields[ i ].name, name ) )
			return fields + i;
	}

	return 0;
}

static qboolean Parse_Field( int handle, byte* data, field_t * field )
{
	switch ( field->type )
	{
	case FIELD_INTEGER:
		{
			if (!PC_Int_Parse(handle, (int*)( data + field->offset ))) {
				return qfalse;
			}

		} break;

	case FIELD_FLOAT:
		{
			if (!PC_Float_Parse(handle, (float*)( data + field->offset ))) {
				return qfalse;
			}

		} break;

	case FIELD_STRING:
		{
			if (!PC_String_Parse(handle, (const char**)( data + field->offset ))) {
				return qfalse;
			}

		} break;
	case FIELD_COLOR:
		{
			int j;
			for ( j=0; j<4; j++ )
			{
				if (!PC_Float_Parse(handle, ((float*)( data + field->offset ))+j )) {
					return qfalse;
				}
			}
		} break;

	case FIELD_RECT:
		{
			if (!PC_Rect_Parse(handle, (rectDef_t*)( data + field->offset ))) {
				return qfalse;
			}
		} break;

	case FIELD_STATE:
		{
			if (!PC_String_Parse(handle, (const char**)( data + field->offset ))) {
				return qfalse;
			}
		} break;

	case FIELD_BIT:
		{
			*(int*)( data + field->offset ) |= field->bit;
		} break;

	case FIELD_FONT:
		{
			const char * font;
			if (!PC_String_Parse(handle, &font)) {
				return qfalse;
			}
			*(qhandle_t*)( data + field->offset ) = DC->registerFont( font );
			return qtrue;
		} break;

	case FIELD_SHADER:
		{
			const char *temp;
			if (!PC_String_Parse(handle, &temp)) {
				return qfalse;
			}
			*(qhandle_t*)( data + field->offset ) = DC->registerShaderNoMip(temp);

		} break;
	case FIELD_SCRIPT:
		{
			if (!PC_Script_Parse(handle, (const char**)( data + field->offset ))) {
				return qfalse;
			}
		} break;
	case FIELD_KEY:
		{
			int i;
			keyAction_t *keys	= (keyAction_t*)( data + field->offset );

			// find a spot to stick it
			for( i=0; i<MAX_KEY_ACTIONS; i++ )
				if( keys[ i ].key == 0 )
				{
					if( !PC_Int_Parse( handle, &keys[ i ].key ) )
						return qfalse;

					if ( !PC_Script_Parse( handle, &keys[ i ].action ) )
						return qfalse;

					break;
				}

			if ( i==MAX_KEY_ACTIONS )
				return qfalse;

		} break;
	case FIELD_FUNC:
		{
			field->func( (itemDef_t*)data, handle );
		} break;

	default:
		return qfalse;
	}

	return qtrue;
}

static int Field_Parse( int handle, const char * name, byte * data, field_t * fields, int count )
{
	field_t * field = Find_Field( name, fields, count );
	if ( field )
	{
		if ( !Parse_Field( handle, data, field ) )
		{
			PC_SourceError(handle, "couldn't parse menu item keyword %s", name);
			return 2;	// found, but error
		}
		return 1;	// found
	}

	return 0;	// not found
}


/*
===============
Column_Parse
===============
*/
qboolean Column_Parse(int handle, columnInfo_t * col) {
	pc_token_t token;
	field_t * columnFields;
	int	columnFieldsCount;

	col->border = 1.0f;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (*token.string != '{') {
		return qfalse;
	}

	columnFields = Find_FieldRange( fields, fieldCount, FIELD_COLUMNS, &columnFieldsCount );

	for( ; ; )
	{
		if (!trap_PC_ReadToken(handle, &token)) {
			PC_SourceError(handle, "end of file inside column\n");
			return qfalse;
		}

		if (*token.string == '}') {
			return qtrue;
		}

		if ( Field_Parse( handle, token.string, (byte*)col, columnFields, columnFieldsCount ) )
			continue;

		PC_SourceError(handle, "unknown column keyword %s", token.string);
		break;
	}
	return qfalse; 	// bk001205 - LCC missing return value
}


/*
===============
Item Keyword Parse functions
===============
*/

// asset_model <string>
qboolean ItemParse_asset_model( itemDef_t *item, int handle ) {
	const char *temp;
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!PC_String_Parse(handle, &temp)) {
		return qfalse;
	}
	item->asset = DC->registerModel(temp);
	modelPtr->angle = 90;
	return qtrue;
}

// asset_shader <string>
qboolean ItemParse_asset_shader( itemDef_t *item, int handle ) {
	const char *temp;

	if (!PC_String_Parse(handle, &temp)) {
		return qfalse;
	}
	item->asset = DC->registerShaderNoMip(temp);
	return qtrue;
}

// model_origin <number> <number> <number>
qboolean ItemParse_model_origin( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (PC_Float_Parse(handle, &modelPtr->origin[0])) {
		if (PC_Float_Parse(handle, &modelPtr->origin[1])) {
			if (PC_Float_Parse(handle, &modelPtr->origin[2])) {
				return qtrue;
			}
		}
	}
	return qfalse;
}

// model_fovx <number>
qboolean ItemParse_model_fovx( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!PC_Float_Parse(handle, &modelPtr->fov_x)) {
		return qfalse;
	}
	return qtrue;
}

// model_fovy <number>
qboolean ItemParse_model_fovy( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!PC_Float_Parse(handle, &modelPtr->fov_y)) {
		return qfalse;
	}
	return qtrue;
}

// model_rotation <integer>
qboolean ItemParse_model_rotation( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!PC_Int_Parse(handle, &modelPtr->rotationSpeed)) {
		return qfalse;
	}
	return qtrue;
}

// model_angle <integer>
qboolean ItemParse_model_angle( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t*)item->typeData;

	if (!PC_Int_Parse(handle, &modelPtr->angle)) {
		return qfalse;
	}
	return qtrue;
}

// columns sets a number of columns and an x pos and width per.. 
qboolean ItemParse_columnDef( itemDef_t *item, int handle )
{
	listBoxDef_t * listPtr;

	Item_ValidateTypeData( item );
	if( !item->typeData )
		return qfalse;

	listPtr = (listBoxDef_t*)item->typeData;

	if ( listPtr->numColumns >= MAX_LB_COLUMNS )
		return qfalse;

	if ( Column_Parse( handle, listPtr->columnInfo + listPtr->numColumns ) ) {
		listPtr->numColumns++;
	} else {
		PC_SourceError(handle, "couldn't parse column in list '%s' from menu '%s'", item->window.name, ((menuDef_t*)item->parent)->window.name );
	}

	return qtrue;
}

qboolean ItemParse_cvarFloat( itemDef_t *item, int handle ) {
	editFieldDef_t *editPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
		return qfalse;
	editPtr = (editFieldDef_t*)item->typeData;
	if (PC_String_Parse(handle, &item->cvar) &&
		PC_Float_Parse(handle, &editPtr->defVal) &&
		PC_Float_Parse(handle, &editPtr->minVal) &&
		PC_Float_Parse(handle, &editPtr->maxVal)) {
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_cvarStrList( itemDef_t *item, int handle ) {
	pc_token_t token;
	multiDef_t *multiPtr;
	int pass;
	
	Item_ValidateTypeData(item);
	if (!item->typeData)
		return qfalse;
	multiPtr = (multiDef_t*)item->typeData;
	multiPtr->count = 0;
	multiPtr->strDef = qtrue;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (*token.string != '{') {
		return qfalse;
	}

	pass = 0;
	for( ; ; )
	{
		if (!trap_PC_ReadToken(handle, &token)) {
			PC_SourceError(handle, "end of file inside menu item\n");
			return qfalse;
		}

		if (*token.string == '}') {
			return qtrue;
		}

		if (*token.string == ',' || *token.string == ';') {
			continue;
		}

		if (pass == 0) {
			if ( sql( "SELECT index FROM strings SEARCH name $;", "t", token.string ) ) {
				int index = sqlint(0);
				sqldone();
				trap_SQL_Run( token.string, sizeof(token.string), index, 0,0,0 );
			}

			multiPtr->cvarList[multiPtr->count] = String_Alloc(token.string);
			pass = 1;
		} else {
			multiPtr->cvarStr[multiPtr->count] = String_Alloc(token.string);
			pass = 0;
			multiPtr->count++;
			if (multiPtr->count >= MAX_MULTI_CVARS) {
				return qfalse;
			}
		}

	}
}

qboolean ItemParse_cvarFloatList( itemDef_t *item, int handle ) {
	pc_token_t token;
	multiDef_t *multiPtr;
	
	Item_ValidateTypeData(item);
	if (!item->typeData)
		return qfalse;
	multiPtr = (multiDef_t*)item->typeData;
	multiPtr->count = 0;
	multiPtr->strDef = qfalse;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (*token.string != '{') {
		return qfalse;
	}

	for( ; ; )
	{
		if (!trap_PC_ReadToken(handle, &token)) {
			PC_SourceError(handle, "end of file inside menu item\n");
			return qfalse;
		}

		if (*token.string == '}') {
			return qtrue;
		}

		if (*token.string == ',' || *token.string == ';') {
			continue;
		}

		if ( sql( "SELECT index FROM strings SEARCH name $;", "t", token.string ) ) {
			int index = sqlint(0);
			sqldone();
			trap_SQL_Run( token.string, sizeof(token.string), index, 0,0,0 );
		}

		multiPtr->cvarList[multiPtr->count] = String_Alloc(token.string);
		if (!PC_Float_Parse(handle, &multiPtr->cvarValue[multiPtr->count])) {
			return qfalse;
		}

		multiPtr->count++;
		if (multiPtr->count >= MAX_MULTI_CVARS) {
			return qfalse;
		}

	}
}

qboolean ItemParse_ownerdrawFlag( itemDef_t *item, int handle ) {
	int i;
	if (!PC_Int_Parse(handle, &i)) {
		return qfalse;
	}
	item->window.ownerDrawFlags |= i;
	return qtrue;
}

qboolean ItemParse_enableCvar( itemDef_t *item, int handle ) {
	if (PC_Script_Parse(handle, &item->enableCvar)) {
		item->cvarFlags = CVAR_ENABLE;
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_disableCvar( itemDef_t *item, int handle ) {
	if (PC_Script_Parse(handle, &item->enableCvar)) {
		item->cvarFlags = CVAR_DISABLE;
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_showCvar( itemDef_t *item, int handle ) {
	if (PC_Script_Parse(handle, &item->enableCvar)) {
		item->cvarFlags = CVAR_SHOW;
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_hideCvar( itemDef_t *item, int handle ) {
	if (PC_Script_Parse(handle, &item->enableCvar)) {
		item->cvarFlags = CVAR_HIDE;
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_showif( itemDef_t *item, int handle ) {
	if ( PC_String_Parse(handle, &item->showif)) {
		item->cvarFlags = CVAR_SHOW;
		return qtrue;
	}
	return qfalse;
}

qboolean ItemParse_passwordCvar( itemDef_t *item, int handle ) {
	item->cvarFlags = CVAR_PASSWORD;
	return qtrue;
}

qboolean ItemParse_AttachToList( itemDef_t *item, int handle )
{
	const char*		list;
	itemDef_t*		listItem;
	listBoxDef_t*	listPtr;

	if ( !PC_String_Parse( handle, &list ) )
		return qfalse;

	listItem = Menu_FindItemByName( item->parent, list );
	if ( !listItem )
		return qfalse;

	listPtr = (listBoxDef_t*)listItem->typeData;

	listPtr->selectButton = item;

	return qtrue;
}

qboolean ItemParse_snapleft( itemDef_t *item, int handle )
{
	const char*		other;

	if ( !PC_String_Parse( handle, &other ) )
		return qfalse;

	item->snapleft = Menu_FindItemByName( item->parent, other );

	return qtrue;
}

qboolean ItemParse_snapright( itemDef_t *item, int handle )
{
	const char*		other;

	if ( !PC_String_Parse( handle, &other ) )
		return qfalse;

	item->snapright = Menu_FindItemByName( item->parent, other );

	return qtrue;
}


/*
===============
Item_Parse
===============
*/
qboolean Item_Parse(int handle, itemDef_t *item) {
	pc_token_t token;
	char last[ 64 ];
	int i = 0;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (*token.string != '{') {
		return qfalse;
	}
	for( ; ; )
	{
		byte * data = 0;
		int j;

		Q_strncpyz( last, (i>0)?token.string:"itemDef", sizeof(last) );

		if (!trap_PC_ReadToken(handle, &token)) {
			PC_SourceError(handle, "end of file inside menu item\n");
			return qfalse;
		}
		i++;

		if (*token.string == '}') {
			Item_ValidateTypeData( item );
			return qtrue;
		}


		for ( j=0; j<fieldCount; j++ )
		{
			Item_UpdateData( item, fields + j, &data );

			if ( !data )
				continue;

			if ( !Q_stricmp( fields[ j ].name, token.string ) )
			{
				if ( !Parse_Field( handle, data, fields + j ) )
				{
					PC_SourceError(handle, "couldn't parse menu item keyword %s", token.string);
					return qfalse;
				}
				break;
			}
		}

		if ( j==fieldCount )
			PC_SourceError(handle, "unknown menu item keyword %s after %s", token.string, last );
	}
}


/*
===============
Menu Keyword Parse functions
===============
*/
qboolean MenuParse_ownerdrawFlag( itemDef_t *item, int handle ) {
	int i;
	menuDef_t *menu = (menuDef_t*)item;

	if (!PC_Int_Parse(handle, &i)) {
		return qfalse;
	}
	menu->window.ownerDrawFlags |= i;
	return qtrue;
}

qboolean MenuParse_itemDef( menuDef_t * menu, int handle )
{
	if ( menu->itemCount < MAX_MENUITEMS )
	{
		itemDef_t * item = UI_Alloc(sizeof(itemDef_t));

		menu->items[ menu->itemCount ] = item;
		Item_Init( item );
		menu->items[menu->itemCount]->parent = menu;

		if (!Item_Parse(handle, item ))
			return qfalse;

		menu->itemCount++;
	}
	return qtrue;
}

qboolean Effect_Parse(int handle, effectDef_t * effect ) {
	pc_token_t token;
	int i;
	int s;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (*token.string != '{') {
		return qfalse;
	}

	for ( s=0; s<fieldCount; s++ )
	{
		if ( fields[ s ].type == FIELD_EFFECT )
		{
			s++;
			break;
		}
	}

	if ( s == fieldCount )
		return qfalse;

	i=0;
	for( ; ; )
	{
		int j;

		if (!trap_PC_ReadToken(handle, &token)) {
			PC_SourceError(handle, "end of file inside menu effect\n");
			return qfalse;
		}
		i++;

		if (*token.string == '}') {
			return qtrue;
		}

		for ( j=s; j<fieldCount; j++ )
		{
			if ( fields[ j ].type >= FIELD_WINDOW )
				break;

			if ( !Q_stricmp( fields[ j ].name, token.string ) )
			{
				if ( !Parse_Field( handle, (byte*)effect, fields + j ) )
				{
					PC_SourceError(handle, "couldn't parse menu effect keyword %s", token.string);
					return qfalse;
				}
				break;
			}
		}

		if ( j==fieldCount )
			PC_SourceError(handle, "unknown menu effect keyword %s", token.string );
	}
}

/*
===============
Menu_Parse
===============
*/
qboolean Menu_Parse(int handle, menuDef_t *menu) {
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (*token.string != '{') {
		return qfalse;
	}
    
	for( ; ; )
	{

		byte * data = 0;
		int j;

		memset(&token, 0, sizeof(pc_token_t));
		if (!trap_PC_ReadToken(handle, &token)) {
			PC_SourceError(handle, "end of file inside menu\n");
			return qfalse;
		}

		if (*token.string == '}') {
			return qtrue;
		}

		if ( !Q_stricmp( token.string, "itemDef" ) )
		{
			MenuParse_itemDef( menu, handle );
			continue;
		}

		for ( j=0; j<fieldCount; j++ )
		{
			Menu_UpdateData( menu, fields + j, &data );

			if ( !data ) 
				continue;

			if ( !Q_stricmp( fields[ j ].name, token.string ) )
			{
				if ( !Parse_Field( handle, data, fields + j ) )
				{
					PC_SourceError(handle, "couldn't parse menu (%s) keyword %s", menu->window.name, token.string);
					return qfalse;
				}

				break;
			}
		}

		if ( j == fieldCount )
			PC_SourceError(handle, "(%s) unknown menu keyword %s", menu->window.name, token.string);
	}
}

/*
===============
Menu_New
===============
*/
void Menu_New(int handle, const char * filename ) {
	menuDef_t *menu = &Menus[menuCount];

	if (menuCount < MAX_MENUS) {
		Menu_Init(menu);
		if (Menu_Parse(handle, menu)) {
			Menu_PostParse(menu);
			menu->filename = String_Alloc( filename );
			menuCount++;
		}
	}
}

int Menu_Count() {
	return menuCount;
}

void Menu_PaintAll() {
	float fade;
	int i;

	if (captureFunc) {
		captureFunc(captureData);
	}

	Item_RunDelayedScripts();

	Menus_SetVisible();

	if ( menuStack->count && !Q_stricmp(menuStack->m[ menuStack->count-1]->window.name, "editor" ) )
		debugMode = 0;

	//
	//	draw the menus that are stack from the bottom up
	//

	// start with the last full screen menu
	for( i = menuStack->count - 1; i >= 0; i-- )
		if( menuStack->m[ i ]->window.flags&(WINDOW_FULLSCREEN|WINDOW_LASTMENU) )
		{
			i--;
			break;
		}

	for( i++; i < menuStack->count; i++ )
		Menu_Paint( menuStack->m[i], qtrue );

	fade = (DC->realTime - menuStack->openTime ) / 550.0f;

	if ( menuStack->openQueueCount )	// menu in queue waiting for current menu stack to fade out
	{
		if ( fade > 1.0f )
		{
			Menus_CloseAll();
			menuStack->openTime = DC->realTime;
			for ( i=0; i<menuStack->openQueueCount; i++ )
				Menus_Activate( menuStack->openQueue[i] );
			menuStack->openQueueCount = 0;
			fade = 1.0f;
		}
	} else
		fade = 1.0f - fade;


	if ( fade >= 0.0f && fade <= 1.0f )
	{
		float x = DC->glconfig.xbias / DC->glconfig.xscale;
		vec4_t color = { 0.0f, 0.0f, 0.0f, 0.0f };
		color[ 3 ] = fade;

		DC->setColor( color );
		DC->drawHandlePic( -x, 0.0f, 640.0f + x*2.0f, 480.0f, DC->whiteShader );
	}

	// draw cursor
	if ( menuStack->count ) {
		if ( trap_Cvar_VariableInt( "journal" ) == 2 ) {
			if ( !uiInfo.uiDC.Assets.cursor ) {
				uiInfo.uiDC.Assets.cursor = trap_R_RegisterShaderNoMip( "ui/assets/cursornotarget" );
			}
			DC->setColor( colorWhite );
			DC->drawHandlePic( uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory, 32, 32, uiInfo.uiDC.Assets.cursor);
		}
	}

}

void Menu_Reset() {
	menuCount = 0;
}

displayContextDef_t *Display_GetContext() {
	return DC;
}

int Display_MouseMove(int x, int y)
{
	int i;
	int cursor = 0;

	for ( i=menuStack->count-1; i>=0; i-- )
	{
		menuDef_t *menu = menuStack->m[ i ];

		if ( Rect_ContainsPoint( &menu->window.rect, x,y ) ) {
			Menu_HandleMouseMove( menu, x,y );
		} else {
			Menu_ClearFocusItem( menu, 0 );
		}

		if ( menu->focusItem ) {

			itemDef_t * item = menu->focusItem;
			if ( item->type == ITEM_TYPE_LISTBOX ) {
				int i;

				listBoxDef_t * listPtr = (listBoxDef_t*)item->typeData;
				int count = listPtr->numRows;

				for ( i=0; i<count; i++ ) {
					rectDef_t r;
					Item_TableCell_Rect( item, -1, i, qfalse, &r );
					if ( Rect_ContainsPoint( &r, x, y ) ) {
						cursor = 1;
						break;
					}
				}
			} else
				cursor = 1;
		}

		if ( menu->window.flags & (WINDOW_FULLSCREEN|WINDOW_LASTMENU) )
			return cursor;

		if ( menu->window.flags & WINDOW_POPUP )
			break;
	}

	//	anything under a pop window must lose focus
	for ( i=i-1; i>=0; i-- )
	{
		menuDef_t *menu = menuStack->m[ i ];
		Menu_ClearFocusItem( menu, 0 );
	}

 	return cursor;
}

#ifdef DEVELOPER
extern void Save_Menu( menuDef_t * menu, const char * filename );
void Save_ActiveMenus()
{
	int i;
	for ( i=0; i<menuStack->count; i++ )
	{
		if ( !(menuStack->m[ i ]->window.flags&WINDOW_DONTSAVE) ) {
			Save_Menu( menuStack->m[ i ], menuStack->m[ i ]->filename );
		}
	}
}
#endif

void Display_HandleKey(int key, qboolean down, int x, int y)
{
	int i;

	if ( !down )
	{
		if ( (key == K_MOUSE1 || key == K_MOUSE2) ) {

			if ( itemCapture ) {
				Item_StopCapture( itemCapture );
				itemCapture = 0;
				captureFunc = 0;
				captureData = 0;
			} else {
				//	search from the top of the menu stack down...
				for( i = menuStack->count - 1; i >= 0; i-- )
				{
					menuDef_t *	menu = menuStack->m[i];
					if ( Rect_ContainsPoint( &menu->window.rect, x, y ) ) {
						itemDef_t * item = Menu_GetItemUnderCursor( menu, x, y );
						if ( item && (item->type == ITEM_TYPE_BUTTON || item->type == ITEM_TYPE_TEXT) ) {
							Item_RunScript(item, item->action);
						}
					}
					if ( menu->window.flags & WINDOW_POPUP )
						break;
				}
			}
		}

		return;
	}

	//	search from the top of the menu stack down...
	for( i = menuStack->count - 1; i >= 0; i-- )
	{
		menuDef_t *	menu = menuStack->m[i];

		
		if( Menu_HandleKeyDown( menu, key ) || (menu->window.flags & WINDOW_POPUP && !debugMode) ) {
			//either menu consumed key or was popup which blocks all down-stack menus
			break;
		}
			
	}

	//	global hot keys for all menus
	switch ( key )
	{
#ifdef DEVELOPER
		case K_F11:
			{
				if ( debugMode )
					Save_ActiveMenus();

				debugMode ^= 1;
				debug.menu = 0;
			} break;
		case K_F9:
			{
				debug_cgmenu = !debug_cgmenu;
			} break;
		case K_MWHEELDOWN:
			{
				if ( !debug.menu )
					debug.menu = menuStack->m[ menuStack->count-1 ];
				else
				{
					int i;
					for ( i=menuStack->count-1; i>=0; i-- )
					{
						if( menuStack->m[ i ]->window.flags&WINDOW_FULLSCREEN )
							break;

						if ( debug.menu == menuStack->m[ i ] )
							break;
					}
					i--;

					debug.menu = (i<0)?0:menuStack->m[ i ];
				}
			} break;

		case K_MWHEELUP:
			{
				if ( !debug.menu )
					debug.menu = menuStack->m[ 0 ];
				else
				{
					int i;
					for ( i=0; i<menuStack->count; i++ )
					{
						if ( debug.menu == menuStack->m[ i ] )
							break;
					}
					i++;

					debug.menu = (i>=menuStack->count)?0:menuStack->m[ i ];
				}
			} break;
#endif
		case K_F8:
			{
				trap_Cmd_ExecuteText( EXEC_INSERT, "screenshot" );
			}break;

		case K_F7:
			{
				if ( trap_Cvar_VariableInt( "sv_cheats" ) == 1 ) {
					Menus_CloseAll();
					Menus_ActivateByName( "mission_editor" );
				}

			} break;

		case K_F3:
		case K_F10:
			if ( !debugMode && trap_Cvar_VariableInt( "developer" ) ) {
				Menus_OpenByName( "reportbug" );
			}
			break;

		default:
			{
				if( DC->getCVarValue( "cl_spacetrader" ) == 1.0F && !g_editItem )
				{
					int i;
					char binding[32];

					DC->getBindingBuf( key, binding, 32 );
					for( i=0; i < lengthof(g_bindings); i++ )
					{
						if( Q_stricmp( g_bindings[i].command, binding ) == 0 && (g_bindings[i].flags & BIND_ALLOW_IN_UI) )
							trap_Cmd_ExecuteText( EXEC_INSERT, binding );
					}
				}
			} break;
	}
}

static void Window_CacheContents(windowDef_t *window) {
	if (window) {
		if (window->cinematicName) {
			int cin = DC->playCinematic(window->cinematicName, 0, 0, 0, 0);
			DC->stopCinematic(cin);
		}
	}
}


static void Item_CacheContents(itemDef_t *item) {
	if (item) {
		Window_CacheContents(&item->window);
	}

}

static void Menu_CacheContents(menuDef_t *menu) {
	if (menu) {
		int i;
		Window_CacheContents(&menu->window);
		for (i = 0; i < menu->itemCount; i++) {
			Item_CacheContents(menu->items[i]);
		}

		if (menu->soundName && *menu->soundName) {
			DC->registerSound(menu->soundName, qfalse);
		}
	}

}

void Display_CacheAll() {
	int i;
	for (i = 0; i < menuCount; i++) {
		Menu_CacheContents(&Menus[i]);
	}
}

#ifdef DEVELOPER
static itemDef_t * UI_Editor_CreateControl( menuDef_t * menu, field_t * field, rectDef_t * r, const char * name )
{
	itemDef_t * item = UI_Alloc(sizeof(itemDef_t));

	Item_Init( item );

	item->window.rectClient = *r;
	item->action			= String_Alloc( "uiScript editorupdate" );
	item->text				= String_Alloc( field->name );
	item->cvar				= String_Alloc( ed_cvar( name, field->name ) );
	item->window.group		= String_Alloc( "grpControls" );
	item->window.name		= String_Alloc( name );
	item->textscale			= 0.19f;
	item->textdivx			= (int)(r->w * 0.45f);
	item->textaligny		= ITEM_ALIGN_RIGHT;
	item->textaligny		= ITEM_ALIGN_CENTER;
	item->window.outlineColor[ 0 ] = 
	item->window.outlineColor[ 1 ] = 
	item->window.outlineColor[ 2 ] = 0.3f;
	item->window.outlineColor[ 3 ] = 1.0f;

	menu->items[ menu->itemCount ] = item;
	menu->items[menu->itemCount]->parent = menu;
	menu->itemCount++;

	return item;
}

static void UI_Editor_MakeSlider( itemDef_t * item, float minVal, float maxVal )
{
	editFieldDef_t * editPtr;
	item->type = ITEM_TYPE_SLIDER;
	Item_ValidateTypeData(item);
	editPtr = (editFieldDef_t*)item->typeData;
	editPtr->defVal = minVal;
	editPtr->minVal = minVal;
	editPtr->maxVal = maxVal;
}

static void UI_Editor_AddField( menuDef_t * menu, const char * name, field_t * field, rectDef_t * r )
{
	// create a new control to edit this field
	itemDef_t * item = UI_Editor_CreateControl( menu, field, r, name );

	switch( field->type )
	{
	case FIELD_INTEGER:
		{
			if ( field->fieldEnum[ 0 ].name )
			{
				multiDef_t *multiPtr;
				int i;
				item->type = ITEM_TYPE_MULTI;
				Item_ValidateTypeData(item);
				multiPtr = (multiDef_t*)item->typeData;
				
				multiPtr->strDef = qfalse;

				for ( i=0; i<lengthof( field->fieldEnum ) && field->fieldEnum[ i ].name; i++ )
				{
					multiPtr->cvarList[ i ]		= String_Alloc( field->fieldEnum[ i ].shortname );
					multiPtr->cvarValue[ i ]	= field->fieldEnum[ i ].value;
				}
				multiPtr->count = i;
			} else
				item->type = ITEM_TYPE_NUMERICFIELD;
				
		} break;

	case FIELD_BIT:
		{
			item->type = ITEM_TYPE_YESNO;
		} break;

	case FIELD_FLOAT:
		{
			if ( field->minVal != field->maxVal )
			{
				UI_Editor_MakeSlider( item, field->minVal, field->maxVal );
			} else
				item->type = ITEM_TYPE_NUMERICFIELD;
		} break;
	case FIELD_FONT:
		{
			char info[ MAX_INFO_STRING ];
			multiDef_t *multiPtr;
			char * s;

			trap_R_GetFonts( info, sizeof(info ) );
			s = info;

			item->type = ITEM_TYPE_MULTI;
			Item_ValidateTypeData(item);
			multiPtr = (multiDef_t*)item->typeData;

			multiPtr->strDef = qfalse;
			multiPtr->count	= 0;

			for ( ; ; )
			{
				char * t = COM_ParseExt( &s, qfalse );
				if ( !t || t[ 0 ] == '\0' )
					break;

				multiPtr->cvarValue[ multiPtr->count ]	= atoi( t );
				multiPtr->cvarList[ multiPtr->count ]	= String_Alloc( COM_ParseExt( &s, qfalse ) );
				multiPtr->count++;
			}

		} break;
	//case FIELD_SCRIPT:
	case FIELD_STRING:
		{
			item->type = ITEM_TYPE_EDITFIELD;
			Item_ValidateTypeData(item);

			((editFieldDef_t*)item->typeData)->maxPaintChars = 32;

		} break;

	case FIELD_COLOR:
		{
#if 0
			rectDef_t a;

			item->window.rectClient.w = item->textdivx + (r->w-item->textdivx)* 0.25f;
			UI_Editor_MakeSlider( item, 0.0f, 1.0f );			// r
			item->cvar = String_Alloc( va( "ed_%s_r", field->name ) );
			a = item->window.rectClient;
			a.h *= 0.5f;

			a.x += a.w;
			item = UI_Editor_CreateControl( menu, field, &a, name );	// g
			UI_Editor_MakeSlider( item, 0.0f, 1.0f );
			item->cvar = String_Alloc( va( "ed_%s_g", field->name ) );
			item->text = 0;
			item->textdivx = 4;

			a.x += a.w;
			item = UI_Editor_CreateControl( menu, field, &a, name );	// b
			UI_Editor_MakeSlider( item, 0.0f, 1.0f );
			item->cvar = String_Alloc( va( "ed_%s_b", field->name ) );
			item->text = 0;
			item->textdivx = 4;

			a.x += a.w;
			item = UI_Editor_CreateControl( menu, field, &a, name );	// a
			UI_Editor_MakeSlider( item, 0.0f, 1.0f );
			item->cvar = String_Alloc( va( "ed_%s_a", field->name ) );
			item->text = 0;
			item->textdivx = 4;
#endif


		} break;



	default:
		menu->itemCount--;
		return;
	}
	Item_ValidateTypeData( item );
}

static void UI_Editor_GetControlRect( menuDef_t * menu, rectDef_t * r, int i )
{
	int n = (menu->window.rect.h-64.0f) / 18.0f;
	
	r->w = menu->window.rect.w / 3.0f;
	r->h = 16.0f;

	r->x = (i/n) * r->w;
	r->y = 16.0f + (i%n) * 18.0f;
}

void UI_Editor_Init()
{
	menuDef_t * menu = Menus_FindByName( "editor" );
	int i,t;
	int h = 0;
	for ( i=0,t=0; i<fieldCount; i++ )
	{
		rectDef_t r;

		if ( fields[ i ].flags & FIELD_SECTION ) {
			h = i;
			continue;
		}

		if ( !fields[ h ].name || !fields[ i ].name || fields[ i ].flags & FIELD_DONTEDIT )
			continue;
		
		UI_Editor_GetControlRect( menu, &r, i-h );
		UI_Editor_AddField( menu, fields[ h ].name, fields + i, &r );
	}

	Menu_PostParse( menu );
}
#endif





#define WINDOWF(x)  (int)&((windowDef_t*)0)->x,0,0
#define WINDOWB(x,b)  (int)&((windowDef_t*)0)->x,b,0

#define ENUMF(n,v) { n,#v,v }

#define	ITEMF(x) (int)&((itemDef_t*)0)->x,0,0
#define ITEMB(x,b)  (int)&((itemDef_t*)0)->x,b,0
#define FUNC(x) 0,0,x,FIELD_FUNC

#define	MENUF(x)	(int)&((menuDef_t*)0)->x,0,0
#define MENUB(x,b)  (int)&((menuDef_t*)0)->x,b,0

#define	EDITF(x)	(int)&((editFieldDef_t*)0)->x,0,0
#define	EDITB(x,b)	(int)&((editFieldDef_t*)0)->x,b,0

#define LISTF(x)	(int)&((listBoxDef_t*)0)->x,0,0
#define LISTB(x,b)	(int)&((listBoxDef_t*)0)->x,b,0

#define COLF(x)	(int)&((columnInfo_t*)0)->x,0,0
#define COLB(x,b)	(int)&((columnInfo_t*)0)->x,b,0

#define EFFECTF(x)	(int)&((effectDef_t*)0)->x,0,0
#define	EFFECTB(x,b) (int)&((effectDef_t*)0)->x,b,0

#define PROGRESSF(x) (int)&((progressDef_t*)0)->x,0,0


static field_t	_fields[] = {

//	----------------------------------------------------------------------------
	{ "window", 0,0,0,	FIELD_WINDOW, FIELD_SECTION },

	{ "name",				WINDOWF(name),							FIELD_STRING	},
	{ "rect",				WINDOWF(rectClient),					FIELD_RECT		},
	{ "group",				WINDOWF(group),							FIELD_STRING	},
	{ "bordersize",			WINDOWF(borderSize),					FIELD_FLOAT		},
	{ "background",			WINDOWF(background),					FIELD_SHADER	},
	{ "forecolor",			WINDOWF(foreColor),						FIELD_COLOR		},
	{ "backcolor",			WINDOWF(backColor),						FIELD_COLOR		},
	{ "bordercolor",		WINDOWF(borderColor),					FIELD_COLOR		},
	{ "outlinecolor",		WINDOWF(outlineColor),					FIELD_COLOR		},
	{ "cinematic",			WINDOWF(cinematicName),					FIELD_STRING	},
	{ "style",				WINDOWF(style),							FIELD_INTEGER, 0,
		{
			ENUMF( "empty",		WINDOW_STYLE_EMPTY ),
			ENUMF( "filled",	WINDOW_STYLE_FILLED ),
			ENUMF( "shader",	WINDOW_STYLE_SHADER ),
			{ 0,0,0 },
		}
	},
	{ "visible",			WINDOWB(flags, WINDOW_VISIBLE ),		FIELD_BIT		},
	{ "decoration",			WINDOWB(flags, WINDOW_DECORATION ),		FIELD_BIT		},
	{ "outOfBoundsClick",	WINDOWB(flags, WINDOW_OOB_CLICK ),		FIELD_BIT		},
	{ "popup",				WINDOWB(flags, WINDOW_POPUP ),			FIELD_BIT		},
	{ "wrapped",			WINDOWB(flags, WINDOW_WRAPPED ),		FIELD_BIT		},
	{ "horizontalscroll",	WINDOWB(flags, WINDOW_HORIZONTAL ),		FIELD_BIT		},
	{ "nofocus",			WINDOWB(flags, WINDOW_NOFOCUS ),		FIELD_BIT		},
	{ "focusflash",			WINDOWB(flags, WINDOW_FOCUSFLASH ),		FIELD_BIT		},
	{ "fullscreen",			WINDOWB(flags, WINDOW_FULLSCREEN ),		FIELD_BIT		},
	{ "lastmenu",			WINDOWB(flags, WINDOW_LASTMENU ),		FIELD_BIT		},
	{ "dimbackground",		WINDOWB(flags, WINDOW_DIMBACKGROUND ),	FIELD_BIT		},
	{ "border",				WINDOWB(flags, WINDOW_HASBORDER ),		FIELD_BIT		},
	{ "pause",				WINDOWB(flags, WINDOW_PAUSEGAME ),		FIELD_BIT		},
	{ "alignleft",			WINDOWB(flags, WINDOW_ALIGNLEFT ),		FIELD_BIT		},
	{ "alignright",			WINDOWB(flags, WINDOW_ALIGNRIGHT ),		FIELD_BIT		},
	{ "alignwidth",			WINDOWB(flags, WINDOW_ALIGNWIDTH ),		FIELD_BIT		},
	{ "textscroll",			WINDOWB(flags, WINDOW_TEXTSCROLL ),		FIELD_BIT		},
	{ "dontsave",			WINDOWB(flags, WINDOW_DONTSAVE ),		FIELD_BIT		},
	{ "ownerdraw",			WINDOWF(ownerDraw),						FIELD_INTEGER, FIELD_DONTSAVE|FIELD_DONTEDIT },


//	----------------------------------------------------------------------------
	{ "item", 0,0,0,	FIELD_ITEM, FIELD_SECTION },

	{ "text",			ITEMF(text),								FIELD_STRING	},
	{ "textalignx",		ITEMF(textalignx),							FIELD_INTEGER, 0,
		{
			ENUMF( "left",		 ITEM_ALIGN_LEFT	),
			ENUMF( "center",	 ITEM_ALIGN_CENTER	),
			ENUMF( "right",		ITEM_ALIGN_RIGHT	),
			ENUMF( "justify",	 ITEM_ALIGN_JUSTIFY	),
			{ 0,0,0 },
		}
	},
	{ "textaligny",		ITEMF(textaligny),							FIELD_INTEGER, 0,
		{
			ENUMF( "left",		 ITEM_ALIGN_LEFT	),
			ENUMF( "center",	 ITEM_ALIGN_CENTER	),
			ENUMF( "right",		ITEM_ALIGN_RIGHT	),
			ENUMF( "justify",	 ITEM_ALIGN_JUSTIFY	),
			{ 0,0,0 },
		}
	},
	{ "textscale",		ITEMF(textscale),							FIELD_FLOAT,	0, { 0 },	0.1f, 1.0f },
	{ "cvarTest",		ITEMF(cvarTest),							FIELD_STRING	},
	{ "textdivx",		ITEMF(textdivx),							FIELD_INTEGER	},
	{ "textdivy",		ITEMF(textdivy),							FIELD_INTEGER	},
	{ "font",			ITEMF(font),								FIELD_FONT		},
	{ "tooltip",		ITEMF(tooltip),								FIELD_STRING, FIELD_DONTSAVE|FIELD_DONTEDIT },
	{ "mouseEnter",		ITEMF(mouseEnter),							FIELD_SCRIPT	},
	{ "mouseExit",		ITEMF(mouseExit),							FIELD_SCRIPT	},
	{ "action",			ITEMF(action),								FIELD_SCRIPT	},
	{ "onFocus",		ITEMF(onFocus),								FIELD_SCRIPT	},
	{ "leaveFocus",		ITEMF(leaveFocus),							FIELD_SCRIPT	},
	{ "cvar",			ITEMF(cvar),								FIELD_STRING	},
	{ "textshadow",		ITEMB(textStyle, ITEM_TEXTSTYLE_SHADOWED ),	FIELD_BIT		},
	{ "textoutline",	ITEMB(textStyle, ITEM_TEXTSTYLE_OUTLINED ),	FIELD_BIT		},
	{ "textblink",		ITEMB(textStyle, ITEM_TEXTSTYLE_BLINK ),	FIELD_BIT		},
	{ "textitalic",		ITEMB(textStyle, ITEM_TEXTSTYLE_ITALIC ),	FIELD_BIT		},
	{ "smallcaps",		ITEMB(textStyle, ITEM_TEXTSTYLE_SMALLCAPS ),FIELD_BIT		},
	{ "type",			ITEMF(type),								FIELD_INTEGER, FIELD_DONTSAVE|FIELD_DONTEDIT },
	{ "ownertext",		ITEMF(ownerText),							FIELD_INTEGER, FIELD_DONTSAVE|FIELD_DONTEDIT },
	{ "owneralpha",		ITEMF(ownerAlpha),							FIELD_INTEGER, FIELD_DONTSAVE|FIELD_DONTEDIT },
	{ "focuskey",		ITEMF(onFocusKey),							FIELD_KEY		},
	{ "hotKey",			ITEMF(onHotKey),							FIELD_KEY		},
	{ "values",			ITEMF(values),								FIELD_STRING	},
	{ "asset_model",	FUNC( ItemParse_asset_model )								},
	{ "asset_shader",	FUNC( ItemParse_asset_shader )								},
	{ "model_origin",	FUNC( ItemParse_model_origin )								},
	{ "model_fovx",		FUNC( ItemParse_model_fovx )								},
	{ "model_fovy",		FUNC( ItemParse_model_fovy )								},
	{ "model_rotation",	FUNC( ItemParse_model_rotation )							},
	{ "model_angle",	FUNC( ItemParse_model_angle )								},
	{ "columnDef",		FUNC( ItemParse_columnDef )									},
	{ "cvarFloat",		FUNC( ItemParse_cvarFloat )									},
	{ "cvarStrList",	FUNC( ItemParse_cvarStrList )								},
	{ "cvarFloatList",	FUNC( ItemParse_cvarFloatList )								},
	{ "ownerdrawFlag",	FUNC( ItemParse_ownerdrawFlag )								},
	{ "enableCvar",		FUNC( ItemParse_enableCvar )								},
	{ "disableCvar",	FUNC( ItemParse_disableCvar )								},
	{ "showCvar",		FUNC( ItemParse_showCvar )									},
	{ "hideCvar",		FUNC( ItemParse_hideCvar )									},
	{ "showif",			FUNC( ItemParse_showif )									},
	{ "passwordCvar",	FUNC( ItemParse_passwordCvar )								},
	{ "attachtolist",	FUNC( ItemParse_AttachToList  )								},
	{ "snapleft",		FUNC( ItemParse_snapleft )									},
	{ "snapright",		FUNC( ItemParse_snapright )									},

//	----------------------------------------------------------------------------
	{ "menu", 0,0,0,	FIELD_MENU, FIELD_SECTION },

	{"font",				MENUF(font),							FIELD_FONT		},
	{"onOpen",				MENUF(onOpen),							FIELD_SCRIPT	},
	{"onFocus",				MENUF(onFocus),							FIELD_SCRIPT	},
	{"onClose",				MENUF(onClose),							FIELD_SCRIPT	},
	{"focuscolor",			MENUF(focusColor),						FIELD_COLOR		},
	{"disablecolor",		MENUF(disableColor),					FIELD_COLOR		},
	{"soundLoop",			MENUF(soundName),						FIELD_STRING	},
	{"fadeClamp",			MENUF(fadeClamp),						FIELD_FLOAT		},
	{"fadeCycle",			MENUF(fadeCycle),						FIELD_FLOAT		},
	{"fadeAmount",			MENUF(fadeAmount),						FIELD_FLOAT		},
	{"menuKey",				MENUF(onMenuKey),						FIELD_KEY		},
	{"fadeClamp",			MENUF(fadeClamp),						FIELD_FLOAT		},
	{"fadeCycle",			MENUF(fadeCycle),						FIELD_FLOAT		},
	{"fadeAmount",			MENUF(fadeAmount),						FIELD_FLOAT		},
	{"showif",				MENUF(showif),							FIELD_STRING	},
	{"ownerdrawFlag",		FUNC( MenuParse_ownerdrawFlag )							},

//	----------------------------------------------------------------------------
	{ "editfield", 0,0,0,	FIELD_EDITFIELD, FIELD_SECTION },

	{"maxChars",			EDITF(maxChars),						FIELD_INTEGER	},
	{"maxPaintChars",		EDITF(maxPaintChars),					FIELD_INTEGER	},
	{"money",				EDITB(flags, EDITFIELD_MONEY),			FIELD_BIT		},
	{"percent",				EDITB(flags, EDITFIELD_PERCENT),		FIELD_BIT		},
	{"integer",				EDITB(flags, EDITFIELD_INTEGER),		FIELD_BIT		},
	{"cdkey",				EDITB(flags, EDITFIELD_CDKEY),			FIELD_BIT		},


//	----------------------------------------------------------------------------
	{ "listbox", 0,0,0,	FIELD_LISTBOX, FIELD_SECTION },

	// listBox
	{"notselectable",		LISTB(flags,LISTBOX_NOTSELECTABLE),		FIELD_BIT		},
	{"dontdrawselect",		LISTB(flags,LISTBOX_DONTDRAWSELECT),	FIELD_BIT		},
	{"selectenabled",		LISTB(flags,LISTBOX_SELECTENABLED),		FIELD_BIT		},
	{"alwaysselect",		LISTB(flags,LISTBOX_ALWAYSSELECT),		FIELD_BIT		},
	{"nogridlines",			LISTB(flags,LISTBOX_NOGRIDLINES),		FIELD_BIT		},
	{"novertlines",			LISTB(flags,LISTBOX_NOVERTLINES),		FIELD_BIT		},
	{"nohorizlines",		LISTB(flags,LISTBOX_NOHORIZLINES),		FIELD_BIT		},
	{"monitortable",		LISTB(flags,LISTBOX_MONITORTABLE),		FIELD_BIT		},
	{"chatlog",				LISTB(flags,LISTBOX_CHATLOG),			FIELD_BIT		},
	{"autoscroll",			LISTB(flags,LISTBOX_AUTOSCROLL),		FIELD_BIT		},
	{"scaleenabled",		LISTB(flags,LISTBOX_SCALEENABLED),		FIELD_BIT		},
	{"noscrollbar",			LISTB(flags,LISTBOX_NOSCROLLBAR),		FIELD_BIT		},
	{"server",				LISTB(flags,LISTBOX_SERVER),			FIELD_BIT		},
	{"elementheight",		LISTF(elementHeight),					FIELD_FLOAT		},
	{"titlebar",			LISTF(titlebar),						FIELD_INTEGER	},
	{"selectshader",		LISTF(selectshader),					FIELD_SHADER	},
	{"source",				LISTF(source),							FIELD_STRING, FIELD_DONTSAVE|FIELD_DONTEDIT },
	{"select",				LISTF(select),							FIELD_SCRIPT	},
	{"noscaleselect",		LISTB(flags,LISTBOX_NOSCALESELECT),		FIELD_BIT		},
	{"buffer_size",			LISTF(buffer_size),						FIELD_INTEGER, FIELD_DONTSAVE|FIELD_DONTEDIT },
	{"selectcolor",			LISTF(selectcolor),						FIELD_COLOR		},
	{"rotate",				LISTB(flags,LISTBOX_ROTATE),			FIELD_BIT		},

//	----------------------------------------------------------------------------
	{ "progress", 0,0,0,	FIELD_PROGRESS, FIELD_SECTION },

	// progress
	{"empty",				PROGRESSF(empty),						FIELD_SHADER, FIELD_DONTSAVE|FIELD_DONTEDIT },
	{"full",				PROGRESSF(full),						FIELD_SHADER, FIELD_DONTSAVE|FIELD_DONTEDIT },


//	----------------------------------------------------------------------------
	{ "column", 0,0,0,	FIELD_COLUMNS, FIELD_SECTION },

	{"pos",					COLF(pos),								FIELD_INTEGER	},
	{"width",				COLF(widthPerCent),						FIELD_INTEGER	},
	{"textalignx",			COLF(horizAlign),						FIELD_INTEGER	},
	{"textaligny",			COLF(vertAlign),						FIELD_INTEGER	},
	{"nowrap",				COLB(nowrap, TEXT_ALIGN_NOCLIP),		FIELD_BIT		},
	{"text",				COLF(text),								FIELD_STRING	},
	{"footer",				COLF(footer),							FIELD_STRING	},
	{"font",				COLF(font),								FIELD_FONT		},
	{"format",				COLF(format),							FIELD_INTEGER	},
	{"textscale",			COLF(textscale),						FIELD_FLOAT		},
	{"border",				COLF(border),							FIELD_FLOAT		},
	{"isshader",			COLB(flags, COL_ISSHADER),				FIELD_BIT		},
	{"iseffect",			COLB(flags, COL_ISEFFECT),				FIELD_BIT		},
	{"square",				COLB(flags, COL_SQUARE),				FIELD_BIT		},
	{"issprite",			COLB(flags, COL_ISSPRITE),				FIELD_BIT		},
	{"iscounter",			COLB(flags, COL_ISCOUNTER),				FIELD_BIT		},
	{"isclient",			COLB(flags, COL_ISCLIENT),				FIELD_BIT		},
	{"textshadow",			COLB(textStyle, ITEM_TEXTSTYLE_SHADOWED),	FIELD_BIT		},
	{"textoutline",			COLB(textStyle, ITEM_TEXTSTYLE_OUTLINED),	FIELD_BIT		},
	{"textblink",			COLB(textStyle, ITEM_TEXTSTYLE_BLINK),	FIELD_BIT		},
	{"textitalic",			COLB(textStyle, ITEM_TEXTSTYLE_ITALIC),	FIELD_BIT		},
	{"sprite0",				COLF(sprites[0]),						FIELD_SHADER	},
	{"sprite1",				COLF(sprites[1]),						FIELD_SHADER	},
	{"sprite2",				COLF(sprites[2]),						FIELD_SHADER	},
	{"sprite3",				COLF(sprites[3]),						FIELD_SHADER	},
	{"sprite4",				COLF(sprites[4]),						FIELD_SHADER	},
	{"sprite5",				COLF(sprites[5]),						FIELD_SHADER	},
	{"sprite6",				COLF(sprites[6]),						FIELD_SHADER	},
	{"sprite7",				COLF(sprites[7]),						FIELD_SHADER	},


//	----------------------------------------------------------------------------
	{ "effect", 0,0,0,	FIELD_EFFECT, FIELD_SECTION },

	{"name",				EFFECTF(name),							FIELD_STRING	},
	{"shader",				EFFECTF(shader),						FIELD_SHADER	},
	{"forecolor",			EFFECTF(forecolor),						FIELD_COLOR		},
	{"backcolor",			EFFECTF(backcolor),						FIELD_COLOR		},
	{"action",				EFFECTF(action),						FIELD_SCRIPT	},
	{"font",				EFFECTF(font),							FIELD_FONT		},
	{"textscale",			EFFECTF(textscale),						FIELD_FLOAT		},
	{"diewithmenu",			EFFECTB(flags, ED_DIEWITHMENU),			FIELD_BIT		},
	{"alwaysontop",			EFFECTB(flags, ED_ALWAYSONTOP),			FIELD_BIT		},
	{"nodraw_inui",			EFFECTB(flags, ED_NODRAW_INUI),			FIELD_BIT		},
	{"textalignx",		EFFECTF(textalignx),						FIELD_INTEGER, 0,
		{
			ENUMF( "left",		ITEM_ALIGN_LEFT	),
			ENUMF( "center",	ITEM_ALIGN_CENTER	),
			ENUMF( "right",		ITEM_ALIGN_RIGHT	),
			ENUMF( "justify",	ITEM_ALIGN_JUSTIFY	),
			{ 0,0,0 },
		}
	},
	{"textaligny",		EFFECTF(textaligny),						FIELD_INTEGER, 0,
		{
			ENUMF( "left",		ITEM_ALIGN_LEFT	),
			ENUMF( "center",	ITEM_ALIGN_CENTER	),
			ENUMF( "right",		ITEM_ALIGN_RIGHT	),
			ENUMF( "justify",	ITEM_ALIGN_JUSTIFY	),
			{ 0,0,0 },
		}
	},
	{ "textitalic",		EFFECTB( textStyle, ITEM_TEXTSTYLE_ITALIC ),	FIELD_BIT		},
	{ "rect",			EFFECTF( rect ),								FIELD_RECT		},
	{ "maxchars",		EFFECTF( maxChars ),							FIELD_INTEGER	},

};

field_t * fields = _fields;
int fieldCount	= lengthof(_fields );
