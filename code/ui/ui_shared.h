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
#ifndef __UI_SHARED_H
#define __UI_SHARED_H


#include "../qcommon/q_shared.h"
#include "../renderer/tr_types.h"
#include "keycodes.h"

#include "menudef.h"

#define MAX_MENUNAME 32
#define MAX_ITEMTEXT 64
#define MAX_ITEMACTION 64
#define MAX_MENUDEFFILE 4096
#define MAX_MENUS 96
#define MAX_MENUITEMS 128
#define MAX_OPEN_MENUS 16

#define WINDOW_MOUSEOVER			0x00000001	// mouse is over it, non exclusive
#define WINDOW_FULLSCREEN			0x00000002	// window is stretched to fill the entire screen
#define WINDOW_VISIBLE				0x00000004	// is visible
#define WINDOW_DIMBACKGROUND		0x00000008	// draws fullscreen quad to dim background
#define WINDOW_DECORATION			0x00000010	// for decoration only, no mouse, keyboard, etc.. 
#define WINDOW_FADINGOUT			0x00000020	// fading out, non-active
#define WINDOW_FADINGIN				0x00000040	// fading in
#define WINDOW_PAUSEGAME			0x00000080	// pauses game if this window is visible
#define WINDOW_LASTMENU				0x00000100	// dont draw any menus under this one
#define WINDOW_FORECOLORSET			0x00000200	// forecolor was explicitly set ( used to color alpha images or not )
#define WINDOW_HORIZONTAL			0x00000400	// for list boxes and sliders, vertical is default this is set of horizontal
#define WINDOW_LB_LEFTARROW			0x00000800	// mouse is over left/up arrow
#define WINDOW_LB_RIGHTARROW		0x00001000	// mouse is over right/down arrow
#define WINDOW_LB_THUMB				0x00002000	// mouse is over thumb
#define WINDOW_LB_PGUP				0x00004000	// mouse is over page up
#define WINDOW_LB_PGDN				0x00008000	// mouse is over page down
#define WINDOW_ORBITING				0x00010000	// item is in orbit
#define WINDOW_OOB_CLICK			0x00020000	// close on out of bounds click
#define WINDOW_WRAPPED				0x00040000	// manually wrap text
#define WINDOW_TEXTSCROLL			0x00080000
#define WINDOW_DONTSAVE				0x00100000
#define WINDOW_POPUP				0x00200000	// popup
#define WINDOW_BACKCOLORSET			0x00400000	// backcolor was explicitly set 
#define WINDOW_FOCUSFLASH			0x01000000	// color flashes when the window has focus
#define WINDOW_NOFOCUS				0x02000000	// window can get mouse input, but can't take focus
#define WINDOW_ALIGNLEFT			0x04000000	// window snaps to left side of parent
#define WINDOW_ALIGNRIGHT			0x08000000	// window snaps to right side of parent
#define WINDOW_ALIGNWIDTH			0x10000000	// windows width becomes parents width
#define WINDOW_HASBORDER			0x20000000	// colored border around window
#define WINDOW_HASSCROLLBAR			0x40000000	// has a vertical scroll bar


// CGAME cursor type bits
#define CURSOR_NONE					0x00000001
#define CURSOR_ARROW				0x00000002
#define CURSOR_SIZER				0x00000004

#ifdef CGAME
#define STRING_POOL_SIZE 128*1024
#else
#define STRING_POOL_SIZE 384*1024
#endif
#define MAX_STRING_HANDLES 4096

#define MAX_SCRIPT_ARGS 12
#define MAX_EDITFIELD 256

#define SCROLLBAR_SIZE						20.0f
#define SLIDER_THUMB_WIDTH					32.0f
#define SLIDER_BUTTON_WIDTH					8.0f
#define	NUM_CROSSHAIRS						10

#define ASSET_PLANETMODEL					"models/planet.x42"

typedef struct
{
	const char *command;
	const char *args[MAX_SCRIPT_ARGS];
} scriptDef_t;


typedef struct
{
	float x;    // horiz position
	float y;    // vert position
	float w;    // width
	float h;    // height;
} rectDef_t;

typedef rectDef_t Rectangle;

// FIXME: do something to separate text vs window stuff
typedef struct {

	Rectangle		rect;				// client coord rectangle
	Rectangle		rectClient;			// screen coord rectangle
	const char *	name;				//
	const char *	group;				// if it belongs to a group
	const char *	cinematicName;		// cinematic name
	int				cinematic;			// cinematic handle
	int				style;				//
	int				ownerDraw;			// ownerDraw style
	int				ownerDrawFlags;		// show flags for ownerdraw items
	float			borderSize;			// 
	int				flags;				// visible, focus, mouseover, cursor
	Rectangle		rectEffects;		// for various effects
	int				offsetTime;			// time based value for various effects
	int				nextTime;			// time next effect should cycle
	qhandle_t		background;			// background asset  
	vec4_t			foreColor;
	vec4_t			backColor;
	vec4_t			borderColor;
	vec4_t			outlineColor;
	vec4_t			color;				// current blended color

} windowDef_t;

typedef windowDef_t Window;

// FIXME: combine flags into bitfields to save space
// FIXME: consolidate all of the common stuff in one structure for menus and items
// THINKABOUTME: is there any compelling reason not to have items contain items
// and do away with a menu per say.. major issue is not being able to dynamically allocate 
// and destroy stuff.. Another point to consider is adding an alloc free call for vm's and have 
// the engine just allocate the pool for it based on a cvar
// many of the vars are re-used for different item types, as such they are not always named appropriately
// the benefits of c++ in DOOM will greatly help crap like this
// FIXME: need to put a type ptr that points to specific type info per type
// 
#define MAX_LB_COLUMNS 16

#define COL_ISSHADER	0x00000001
#define COL_ISEFFECT	0x00000002
#define COL_SQUARE		0x00000004	// effects and shaders are square
#define COL_ISSPRITE	0x00000008
#define COL_ISCOUNTER	0x00000010
#define COL_ISCLIENT	0x00000020

typedef struct columnInfo_s
{
	int			pos;
	int			width;
	int			widthPerCent;
	textAlign_e horizAlign;
	textAlign_e vertAlign;
	int			nowrap;
	const char*	text;
	int			ownerText;
	const char* footer;
	qhandle_t	font;
	float		textscale;
	float		border;
	int			flags;
	int			textStyle;
	int			format;
	qhandle_t	sprites[ 8 ];

} columnInfo_t;


#define LISTBOX_NOTSELECTABLE	0x00000001
#define LISTBOX_NOGRIDLINES		0x00000002
#define LISTBOX_NOVERTLINES		0x00000004
#define LISTBOX_NOHORIZLINES	0x00000008
#define LISTBOX_NOSCALESELECT	0x00000010
#define LISTBOX_MONITORTABLE	0x00000020		// runs ->select script if data under selection changes
#define LISTBOX_CHATLOG			0x00000040
#define LISTBOX_AUTOSCROLL		0x00000080
#define LISTBOX_DONTDRAWSELECT	0x00000100
#define LISTBOX_ALWAYSSELECT	0x00000200		// calls select script even if already selected
#define LISTBOX_SELECTENABLED	0x00000400		// highlights row that are enabled
#define LISTBOX_SCALEENABLED	0x00000800
#define LISTBOX_NOSCROLLBAR		0x00001000		// don't draw the scroll bar
#define LISTBOX_ROTATE			0x00002000
#define LISTBOX_SERVER			0x00004000


typedef struct listBoxDef_s {
	int				startPos;
	int				endPos;
	float			elementHeight;
	int				numColumns;
	int				numRows;
	columnInfo_t	columnInfo[MAX_LB_COLUMNS];
	int				flags;
	int				lastSel;
	float			timeSel;	// time the last item was selected
	struct itemDef_s * selectButton;	// this item shows up next to the selection bar on the list
	const char *	table;
	int				titlebar;		// height of title bar

	float			fade;

	qhandle_t		selectshader;

	const char *	source;
	const char *	select;

	char *			buffer;
	int				buffer_size;

	vec4_t			selectcolor;

} listBoxDef_t;

#define EDITFIELD_MONEY		0x00000001	
#define EDITFIELD_PERCENT	0x00000002		// for sliders, if true then slider displays 0 - 100, else the actual value
#define EDITFIELD_INTEGER	0x00000004
#define EDITFIELD_CDKEY		0x00000008

typedef struct editFieldDef_s {
	float	minVal;
	float	maxVal;
	float	defVal;
	int		maxChars;
	int		maxPaintChars;
	int		paintOffset;
	int		flags;

} editFieldDef_t;

#define MAX_MULTI_CVARS 32

typedef struct multiDef_s {
	const char *cvarList[MAX_MULTI_CVARS];
	const char *cvarStr[MAX_MULTI_CVARS];
	float cvarValue[MAX_MULTI_CVARS];
	int count;
	qboolean strDef;
} multiDef_t;

typedef struct modelDef_s {
	int angle;
	vec3_t origin;
	float fov_x;
	float fov_y;
	int rotationSpeed;
} modelDef_t;

typedef struct {
	qhandle_t empty;
	qhandle_t full;
} progressDef_t;

#define CVAR_ENABLE		0x00000001
#define CVAR_DISABLE	0x00000002
#define CVAR_SHOW		0x00000004
#define CVAR_HIDE		0x00000008
#define CVAR_PASSWORD	0x00000012

//ToDo: value of 16 blows memory, consider reducing further
#define MAX_KEY_ACTIONS		6
#define MAX_TABS			4

typedef struct keyAction_s
{
	int key;											// the key to bind to
	const char *action;									// the action to execute
} keyAction_t;

#define ITEM_FADE_TIME		200

#define ED_DIEWITHMENU	0x000000001
#define ED_ALWAYSONTOP	0x000000002
#define ED_NODRAW_INUI	0x000000004

#define EFFECT_TEXT_LENGTH 64

typedef struct
{
	rectDef_t		rect;
	const char *	name;
	qhandle_t		shader;
	vec4_t			forecolor;
	vec4_t			backcolor;
	const char *	action;
	qhandle_t		font;
	float			textscale;
	int				textalignx;
	int				textaligny;
	int				textStyle;
	int				flags;
	int				maxChars;
	
} effectDef_t;

typedef struct itemDef_s
{
	Window window;										// common positional, border, style, layout info
	Rectangle textRect;									// rectangle the text ( if any ) consumes     
	int type;											// text, button, radiobutton, checkbox, textfield, listbox, combo
	int textdivx;										// divider between text and ownertext
	int textdivy;										// divider between text and ownertext
	textAlign_e textalignx;								// horizontal text alignment
	textAlign_e textaligny;								// vertical text alignment
	float textscale;									// scale percentage from 72pts
	int textStyle;										// ( optional ) style, normal and shadowed are it for now
	qhandle_t font;										// text font, if null uses parent->font
	const char *text;									// display text
	const char *tooltip;								// tool tip text
	void *parent;										// menu owner
	qhandle_t asset;									// handle to asset
	qhandle_t assetShader;								// custom shader for asset
	const char *mouseEnter;								// mouse enter script
	const char *mouseExit;								// mouse exit script 
	const char *action;									// select script
	keyAction_t onHotKey[MAX_KEY_ACTIONS];				// hotkey, can be invoked from any point in the GUI
	keyAction_t onFocusKey[MAX_KEY_ACTIONS];			// focus key, can only be invoked when the control has focus
	const char *onFocus;								// select script
	const char *leaveFocus;								// select script
	const char *cvar;									// associated cvar 
	const char *cvarTest;								// associated cvar for enable actions
	const char *enableCvar;								// enable, disable, show, or hide based on value, this can contain a list
	const char *showif;								// show or hide based on an sql expression
	int cvarFlags;										// what type of action to take on cvarenables
	int ownerText;
	int ownerAlpha;
	sfxHandle_t focusSound;
	int numColors;										// number of color ranges
	int cursorPos;										// cursor position in characters
	void *typeData;										// type specific data ptr's
	int	focusTime;										// the time at which the focusItem changed
	struct itemDef_s * snapleft;
	struct itemDef_s * snapright;

	const char * values;	// sql select statement, selection depends on item type

} itemDef_t;

typedef struct
{
	Window window;
	int itemCount;										// number of items;
	qhandle_t font;
	int fadeCycle;										//
	float fadeClamp;									//
	float fadeAmount;									//
	const char *onOpen;									// run when the menu is first opened
	const char *onClose;								// run when the menu is closed
	const char *onFocus;
	keyAction_t onMenuKey[MAX_KEY_ACTIONS];
	const char *soundName;								// background loop sound for menu

	vec4_t focusColor;									// focus color for items
	vec4_t disableColor;								// focus color for items
	itemDef_t *items[MAX_MENUITEMS];					// items this menu contains   
	itemDef_t *focusItem;
	int	time;

	const char * showif;

	// debug
	const char * filename;

} menuDef_t;

typedef struct
{
	menuDef_t*	m[ MAX_OPEN_MENUS ];
	int			count;

	menuDef_t*	openQueue[ 4 ];		// menu waits until others are faded out before activating
	int			openQueueCount;
	int			openTime;

} menuStackDef_t;


typedef struct
{
	const char *cursorStr;
	const char *gradientStr;
	qhandle_t cursor;
	sfxHandle_t menuEnterSound;
	sfxHandle_t menuExitSound;
	sfxHandle_t menuBuzzSound;
	sfxHandle_t itemFocusSound;
	float fadeClamp;
	int fadeCycle;
	float fadeAmount;

	// player settings
	qhandle_t	crosshairShader[NUM_CROSSHAIRS];
			   
	qhandle_t menu;
	qhandle_t menubar;
	qhandle_t sbThumb; //array this if we need short/med/tall

    qhandle_t shipModel;
	qhandle_t planetModel;

	qhandle_t orbit;
	qhandle_t orbitSel;

	qhandle_t font;

	qhandle_t sliderThumb;

	qhandle_t star;

	qhandle_t lineshadow;
	
	qhandle_t redbar;

	sfxHandle_t	dings[4];

	effectDef_t	*	effects[64];
	int				effectCount;

	qhandle_t	radar_player;
	qhandle_t	radar_bot;

} cachedAssets_t;

typedef struct {
  const char *name;
  void (*handler) (itemDef_t *item, const char** args);
} commandDef_t;

typedef struct
{
	qhandle_t (*registerShaderNoMip) (const char *p);
	void (*setColor) (const vec4_t v);
	void (*drawHandlePic) (float x, float y, float w, float h, qhandle_t asset);
	void (*drawStretchPic) (float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
	qhandle_t (*registerModel) (const char *p);
	void (*modelBounds) (qhandle_t model, vec3_t min, vec3_t max);
	void (*fillRect) ( float x, float y, float w, float h, const vec4_t color, qhandle_t shader );
	void (*drawRect) ( float x, float y, float w, float h, float size, const vec4_t color);
	void (*drawSides) (float x, float y, float w, float h, float size);
	void (*drawTopBottom) (float x, float y, float w, float h, float size);
	void (*clearScene) ();
	void (*addRefEntityToScene) (const refEntity_t *re );
	void (*renderScene) ( const refdef_t *fd );
	qhandle_t (*registerFont) (const char *pFontname );
	void (*ownerDrawItem)( itemDef_t * item, vec4_t color );
	void (*runScript)(const char **p);
	void (*getCVarString)(const char *cvar, char *buffer, int bufsize);
	float (*getCVarValue)(const char *cvar);
	void (*setCVar)(const char *cvar, const char *value);
	void (*setOverstrikeMode)(qboolean b);
	qboolean (*getOverstrikeMode)();
	void (*startLocalSound)( sfxHandle_t sfx, int channelNum );
	qboolean (*ownerDrawHandleKey)(int ownerDraw, int flags, int key);
	qboolean (*ownerDrawHandleMouseMove)( itemDef_t * item, float x, float y );
	void (*keynumToStringBuf)( int keynum, char *buf, int buflen );
	void (*getBindingBuf)( int keynum, char *buf, int buflen );
	void (*setBinding)( int keynum, const char *binding );
	void (*executeText)(int exec_when, const char *text );	
	void (*Error)(int level, const char *error, ...);
	void (*Print)(const char *msg, ...);
	sfxHandle_t (*registerSound)(const char *name, qboolean compressed);
	void (*startBackgroundTrack)( const char *intro, const char *loop);
	void (*stopBackgroundTrack)();
	int (*playCinematic)(const char *name, float x, float y, float w, float h);
	void (*stopCinematic)(int handle);
	void (*drawCinematic)(int handle, float x, float y, float w, float h);
	void (*runCinematicFrame)(int handle);
	int (*getClientNum)(void);
	void (*toolTip)( itemDef_t * item );

	const char* (*getOwnerText)( int id );

	int				realTime;
	int				frameTime;
	int				cursorx;
	int				cursory;
	rectDef_t		tooltip;
	qboolean	debug;

	cachedAssets_t Assets;

	vidConfig_t	glconfig;
	qhandle_t	whiteShader;
	qhandle_t	cursor;
	float FPS;

} displayContextDef_t;

extern displayContextDef_t *DC;

const char *String_Alloc(const char *p);
void String_Init();
void String_Report();
void Init_Display(displayContextDef_t *dc);
void Display_ExpandMacros(char * buff);
void Menu_Init(menuDef_t *menu);
void Item_Init(itemDef_t *item);
void Item_PaintModel( qhandle_t model, qhandle_t shader,
							float fovx, float fovy, float angle,
							const rectDef_t *rc );
void Item_Action(itemDef_t *item);
void Item_TextRect( itemDef_t * item, rectDef_t * r );
void Item_TableCell_Rect( itemDef_t * item, int x, int y, qboolean scrollBar, rectDef_t * r );
void Item_TableCellText_Paint( itemDef_t * item, rectDef_t * cell, int column, const char * text );
void Item_TableCellText_PaintColor( itemDef_t * item, rectDef_t * cell, int column, const char * text, vec4_t color, float scale );
void Item_ListBox_AutoScroll( itemDef_t * item, int n );
void Menu_PostParse(menuDef_t *menu);
menuDef_t *Menu_GetFocused();
itemDef_t *Menu_GetFocusedItem(menuDef_t *menu);
qboolean Menu_HandleKeyDown(menuDef_t *menu, int key );
void Menu_HandleMouseMove(menuDef_t *menu, float x, float y);
qboolean Float_Parse(const char **p, float *f);
qboolean Color_Parse(const char **p, vec4_t *c);
qboolean Int_Parse(const char **p, int *i);
qboolean Rect_Parse(const char **p, rectDef_t *r);
qboolean String_Parse(const char **p, const char **out);
qboolean Script_Parse(const char **p, const char **out);
qboolean PC_Float_Parse(int handle, float *f);
qboolean PC_Color_Parse(int handle, vec4_t *c);
qboolean PC_Int_Parse(int handle, int *i);
qboolean PC_Rect_Parse(int handle, rectDef_t *r);
qboolean PC_String_Parse(int handle, const char **out);
qboolean PC_Script_Parse(int handle, const char **out);
int Menu_Count();
void Menu_New(int handle, const char * filename );
void Menu_PaintAll();
void Menus_ActivateByName(const char *p);
void Menu_Reset();
qboolean Menus_AnyFullScreenVisible();
void  Menus_Activate(menuDef_t *menu);
qboolean Effect_Parse(int handle, effectDef_t * effect );

displayContextDef_t *Display_GetContext();
void *Display_CaptureItem(int x, int y);
int Display_MouseMove(int x, int y);
int Display_CursorType(int x, int y);
qboolean Display_KeyBindPending();
void Menus_OpenByName(const char *p);
void Menus_FadeAllOutAndFadeIn( const char * p );
menuDef_t *Menus_FindByName(const char *p);
void Menus_ShowByName(const char *p);
void Menus_CloseByName(const char *p);
void Menus_Close( menuDef_t *menu );
void Display_HandleKey(int key, qboolean down, int x, int y);
void LerpColor(vec4_t a, vec4_t b, vec4_t c, float t);
void Menus_CloseAll();
void Menu_Paint(menuDef_t *menu, qboolean forcePaint);
void Display_CacheAll();
void Item_RunScript( itemDef_t *item, const char *s );
qboolean Rect_ContainsPoint(const rectDef_t *rect, float x, float y);

#ifdef DEVELOPER
void UI_Editor_Init();
void UI_Editor_UpdateItem();
#endif

itemDef_t* Menu_FindItemByName( menuDef_t *menu, const char *p );

void UI_SetMenuStack( menuStackDef_t *stack );

void *UI_Alloc( int size );
void UI_InitMemory( void );
qboolean UI_OutOfMemory();

void Controls_GetConfig( void );
void Controls_SetConfig(qboolean restart);
void Controls_SetDefaults( void );

int			trap_PC_AddGlobalDefine( char *define );
int			trap_PC_LoadSource( const char *filename );
int			trap_PC_FreeSource( int handle );
int			trap_PC_ReadToken( int handle, pc_token_t *pc_token );
int			trap_PC_SourceFileAndLine( int handle, char *filename, int *line );
void		trap_Cmd_ExecuteText( int exec_when, const char *text );	// don't use EXEC_NOW!


#define trap_R_RenderText(rect,scale,rgba,text,limit,horiz,vert,style,font,cursor,textRect) trap_R_DrawText(rect,scale,rgba,text,limit,((horiz)<<16)+(vert),style,cursor,font,textRect)

void			trap_R_DrawText(	Rectangle*		rect,		// render within this rectangle
						float			scale,		// scale the text
						const float*	rgba,
						const char*		text,
						int				limit,		// don't draw more than this many characters
						int				align,
						int				style,		// shadow etc
						int				cursor,		// >= 0 draws a flashing cusor at text location
						qhandle_t		font,
						const rectDef_t*textRect	// return the rectangle that fits tight around the text
					);

int				trap_R_FlowText( fontInfo_t * font,
						float fontScale,
						const float startColor[4],
						const float baseColor[4],
						const char * text,
						int limit,
						int align,
						float width,
						lineInfo_t * lines,
						int max
					);

void trap_R_GetFont( qhandle_t font, float scale, fontInfo_t * buffer );

void trap_R_RenderRoundRect( float x, float y, float w, float h, const float * rgba, qhandle_t shader );

//
//	SQL interface
//

void		trap_SQL_LoadDB( const char * filename, const char * filter );

void		trap_SQL_Exec( const char* statement );
qhandle_t	trap_SQL_Prepare( const char* statement );

void		trap_SQL_BindText( int i, const char* text );
void		trap_SQL_BindInt( int i, int v );
void		trap_SQL_BindArgs();
qboolean	trap_SQL_Step();

int			trap_SQL_ColumnCount();
void		trap_SQL_ColumnAsText( char * buffer, int size, int i );
int			trap_SQL_ColumnAsInt( int i );
void		trap_SQL_ColumnName( char * buffer, int size, int i );
int			trap_SQL_Done();

int			trap_SQL_Compile	( const char * src );
void		trap_SQL_Run		( char * buffer, int size, int index, int p1, int p2, int p3 );

typedef union {
	int		integer;
	char *	string;
} cell_t;
int			trap_SQL_Select( const char * statement, char * buffer, int size );
int			trap_SQL_SelectFromServer( const char * statement, char * buffer, int size );


#endif
