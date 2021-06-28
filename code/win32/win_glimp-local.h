#ifndef __win_glimp_local_h_
#define __win_glimp_local_h_

#include "resource.h"

#include "win_local.h"
#include "glw_win.h"

#include "platform/pop_segs.h"
#include <GL/wglew.h>
#include "platform/push_segs.h"

#define	WINDOW_CLASS_NAME	"ST_GAME_WND"
#define WINDOW_CAPTION		"Space Trader"

#define GLW_DEFAULT_FS_RESOLUTION "640 480"

extern cvar_t *vid_xpos; // X coordinate of window position
extern cvar_t *vid_ypos; // Y coordinate of window position

/*
	Any subclass that calls into the Glimp subclass proc
	can OR this into the message to have it skip processing
	and just pass the message through.

	This is the high bit of the WM_USER range and any such
	messages will end up in the WM_USER range. This isn't an
	issue as we're not passing this on to any system-specific
	window classes, only our own.
*/
#define WM_PASS_MESSAGE_FLAG 0x4000

typedef enum skinBorder_e
{
	SB_Top,
	SB_Left,
	SB_Bottom,
	SB_Right,
} skinBorder_t;

typedef struct skinAnchor_s
{
	skinBorder_t	edge;
	uint			amount;
} skinAnchor_t;

typedef struct skinBtn_s
{
	skinAnchor_t	topAnchor;
	skinAnchor_t	leftAnchor;

	uint			width;
	uint			height;
	
	HBITMAP			normal;
	HBITMAP			hover;
	HBITMAP			pressed;

	char			hint[64];
	char			cmd[256];
} skinBtn_t;

typedef struct skinBorderDef_s
{
	uint		size;
	HBITMAP		img;
} skinBorderDef_t;

typedef struct skinDef_s
{
	char			caption[64];

	skinBorderDef_t	top;
	skinBorderDef_t	left;
	skinBorderDef_t	bottom;
	skinBorderDef_t	right;

	HBITMAP			tlcorner;
	HBITMAP			trcorner;
	HBITMAP			blcorner;
	HBITMAP			brcorner;

	uint			roundX, roundY; //for rounding the corners

	uint			numBtns;
	skinBtn_t		btns[16];
	
	struct
	{
		uint		pBtn; //pressed button index + 1 (such that !pBtn == nothing pressed)
	}				state;
} skinDef_t;

#define SKIN_WNDEXTRA sizeof( skinDef_t* )
#define GWL_SKINDEF 0

LRESULT CALLBACK GLW_SubclassWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

skinDef_t* Skin_Load( const char *skinName );
void Skin_RegisterClass( void );
HWND Skin_CreateWnd( skinDef_t *skin, int x, int y, int w, int h );

void Skin_AdjustWindowRect( RECT *rc, const skinDef_t *skin );

void Win_PrintOSVersion( void );
void Win_PrintCpuInfo( void );
void Win_PrintMemoryInfo( void );

#endif