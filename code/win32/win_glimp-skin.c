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

#include "../render-base/tr_local.h"
#include "win_glimp-local.h"

static HBITMAP Skin_LoadImage( const char *imageName, uint *width, uint *height )
{
	uint x, y;
	int w, h, nScans;
	byte *pic;
	HBITMAP ret = NULL;

	HWND hwndDesktop;
	HDC hdcDesktop;

	BITMAPINFO bi;

	R_LoadImage( imageName, &pic, &w, &h );

	if( width )
		*width = pic ? (uint)w : 0;
	if( height )
		*height = pic ? (uint)h : 0;

	if( !pic )
		return NULL;

	hwndDesktop = GetDesktopWindow();
	hdcDesktop = GetDC( hwndDesktop );
	if( !hdcDesktop )
		goto cleanup;

	ret = CreateCompatibleBitmap( hdcDesktop, w, h );
	if( !ret )
		goto cleanup;

	//swap the image into GDI format
	for( y = 0; y < (uint)h; y++ )
		for( x = 0; x < (uint)w; x++ )
		{
			byte *c = pic + (y * w * 4) + (x * 4);

			byte t = c[0];
			c[0] = c[2];
			c[2] = t;

			c[3] = 0xFF;
		}

	Com_Memset( &bi, 0, sizeof( bi ) );
	bi.bmiHeader.biSize = sizeof( bi );

	bi.bmiHeader.biWidth = w;
	bi.bmiHeader.biHeight = -h; //so we don't have to flip the image
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;

	nScans = SetDIBits( hdcDesktop, ret, 0, h, pic, &bi, DIB_RGB_COLORS );

	if( !nScans )
	{
		DeleteObject( ret );
		ret = NULL;
		goto cleanup;
	}

cleanup:
	if( hdcDesktop )
		ReleaseDC( hwndDesktop, hdcDesktop );

	ri.Hunk_FreeTempMemory( pic );
	
	return ret;
}

static HBITMAP Skin_FindImage( const char *skinDir, const char *fileName, uint *width, uint *height )
{
	HBITMAP ret = NULL;
	char path[MAX_QPATH];

	Com_sprintf( path, sizeof( path ), "%s/%s", skinDir, fileName );

	ret = Skin_LoadImage( path, width, height );
	if( ret )
		return ret;

	ret = Skin_LoadImage( fileName, width, height );
	if( ret )
		return ret;

	//anywhere else we should look?

	return NULL;
}

static qboolean Skin_ParseBorder( char *tok, skinBorder_t *edge )
{
	if( Q_stricmp( tok, "top" ) == 0 )
		*edge = SB_Top;
	else if( Q_stricmp( tok, "left" ) == 0 )
		*edge = SB_Left;
	else if( Q_stricmp( tok, "bottom" ) == 0 )
		*edge = SB_Bottom;
	else if( Q_stricmp( tok, "right" ) == 0 )
		*edge = SB_Right;
	else
		return qfalse;

	return qtrue;
}

static qboolean Skin_ParseButton( char **text, skinDef_t *skin, char *dir )
{
	char *tok;
	skinBtn_t *btn = skin->btns + skin->numBtns;

	tok = COM_ParseExt( text, qfalse );
	if( !tok[0] || strlen( tok ) + 1 > sizeof( btn->hint ) )
		return qfalse;
	Q_strncpyz( btn->hint, tok, sizeof( btn->hint ) );

	tok = COM_ParseExt( text, qfalse );
	if( !tok[0] || strlen( tok ) + 1 > sizeof( btn->cmd ) )
		return qfalse;
	Q_strncpyz( btn->cmd, tok, sizeof( btn->cmd ) );

	tok = COM_ParseExt( text, qfalse );
	if( !tok[0] )
		return qfalse;
	if( !Skin_ParseBorder( tok, &btn->topAnchor.edge ) )
		return qfalse;

	tok = COM_ParseExt( text, qfalse );
	if( !tok[0] )
		return qfalse;
	btn->topAnchor.amount = atoi( tok );

	tok = COM_ParseExt( text, qfalse );
	if( !tok[0] )
		return qfalse;
	if( !Skin_ParseBorder( tok, &btn->leftAnchor.edge ) )
		return qfalse;

	tok = COM_ParseExt( text, qfalse );
	if( !tok[0] )
		return qfalse;
	btn->leftAnchor.amount = atoi( tok );

	tok = COM_ParseExt( text, qfalse );
	btn->normal = Skin_FindImage( dir, tok, &btn->width, &btn->height );
	if( !btn->normal )
		return qfalse;

	tok = COM_ParseExt( text, qfalse );
	btn->hover = Skin_FindImage( dir, tok, NULL, NULL );
	if( !btn->hover )
		return qfalse;

	tok = COM_ParseExt( text, qfalse );
	btn->pressed = Skin_FindImage( dir, tok, NULL, NULL );
	if( !btn->pressed )
		return qfalse;

	skin->numBtns++;

	return qtrue;
}

skinDef_t* Skin_Load( const char *skinName )
{
	char *def;
	char *text;
	char *tok;

	char dir[MAX_QPATH];
	char skinPath[MAX_QPATH];

	skinDef_t skin, *ret;

	Com_sprintf( dir, sizeof( dir ), "ui/skin/%s", skinName );
	Com_sprintf( skinPath, sizeof( skinPath ), "%s/skin", dir );

	ri.FS_ReadFile( skinPath, (void**)&def );
	if( !def )
		return NULL;

	Com_Memset( &skin, 0, sizeof( skin ) );

	text = def;
	
	tok = COM_ParseExt( &text, qtrue );
	if( Q_stricmp( tok, "caption" ) != 0 )
		goto error;

	tok = COM_ParseExt( &text, qfalse );
	Q_strncpyz( skin.caption, tok, sizeof( skin.caption ) );

	tok = COM_ParseExt( &text, qtrue );
	if( Q_stricmp( tok, "borders" ) != 0 )
		goto error;

	tok = COM_ParseExt( &text, qfalse );
	if( !tok[0] )
		goto error;
	skin.top.img = Skin_FindImage( dir, tok, NULL, &skin.top.size );
	if( !skin.top.img )
		goto error;

	tok = COM_ParseExt( &text, qfalse );
	if( !tok[0] )
		goto error;
	skin.left.img = Skin_FindImage( dir, tok, &skin.left.size, NULL );
	if( !skin.left.img )
		goto error;

	tok = COM_ParseExt( &text, qfalse );
	if( !tok[0] )
		goto error;
	skin.bottom.img = Skin_FindImage( dir, tok, NULL, &skin.bottom.size );
	if( !skin.bottom.img )
		goto error;

	tok = COM_ParseExt( &text, qfalse );
	if( !tok[0] )
		goto error;
	skin.right.img = Skin_FindImage( dir, tok, &skin.right.size, NULL );
	if( !skin.right.img )
		goto error;

	tok = COM_ParseExt( &text, qtrue );
	if( Q_stricmp( tok, "corners" ) != 0 )
		goto error;

	tok = COM_ParseExt( &text, qfalse );
	skin.tlcorner = Skin_FindImage( dir, tok, NULL, NULL );
	if( !skin.tlcorner )
		goto error;

	tok = COM_ParseExt( &text, qfalse );
	skin.trcorner = Skin_FindImage( dir, tok, NULL, NULL );
	if( !skin.trcorner )
		goto error;

	tok = COM_ParseExt( &text, qfalse );
	skin.blcorner = Skin_FindImage( dir, tok, NULL, NULL );
	if( !skin.blcorner )
		goto error;

	tok = COM_ParseExt( &text, qfalse );
	skin.brcorner = Skin_FindImage( dir, tok, NULL, NULL );
	if( !skin.brcorner )
		goto error;

	tok = COM_ParseExt( &text, qfalse );
	if( tok[0] )
	{
		if( Q_stricmp( tok, "rounded" ) == 0 )
		{
			tok = COM_ParseExt( &text, qfalse );
			skin.roundX = (uint)atoi( tok );

			tok = COM_ParseExt( &text, qfalse );
			skin.roundY = (uint)atoi( tok );
		}
		else
			goto error;
	}

	for( ; ; )
	{
		tok = COM_ParseExt( &text, qtrue );

		if( !tok[0] )
			break;

		if( Q_stricmp( tok, "button" ) == 0 )
		{
			if( !Skin_ParseButton( &text, &skin, dir ) )
				goto error;
		}
		else
			goto error;
	}

	ri.FS_FreeFile( def );

	ret = (skinDef_t*)ri.Malloc( sizeof( skin ) );
	Com_Memcpy( ret, &skin, sizeof( skin ) );
	return ret;

error:
	ri.Printf( PRINT_WARNING, "Failed to load skin.\n" );

	if( skin.top.img ) DeleteObject( skin.top.img );
	if( skin.left.img ) DeleteObject( skin.left.img );
	if( skin.bottom.img ) DeleteObject( skin.bottom.img );
	if( skin.right.img ) DeleteObject( skin.right.img );

	ri.FS_FreeFile( def );
	return NULL;
}

static void Skin_Free( skinDef_t *pSkin )
{
	uint i;

	DeleteObject( pSkin->top.img );
	DeleteObject( pSkin->left.img );
	DeleteObject( pSkin->bottom.img );
	DeleteObject( pSkin->right.img );

	DeleteObject( pSkin->tlcorner );
	DeleteObject( pSkin->trcorner );
	DeleteObject( pSkin->blcorner );
	DeleteObject( pSkin->brcorner );

	for( i = 0; i < pSkin->numBtns; i++ )
	{
		DeleteObject( pSkin->btns[i].normal );
		DeleteObject( pSkin->btns[i].hover );
		DeleteObject( pSkin->btns[i].pressed );
	}

	ri.Free( pSkin );
}

static void Skin_FillRect( HDC dc, HDC memDC, HBITMAP bmp, RECT *rc )
{
	HBRUSH br;

	if( !bmp )
		return;

	br = CreatePatternBrush( bmp );

	SetBrushOrgEx( dc, rc->left, rc->top, NULL );

	FillRect( dc, rc, br );
	DeleteObject( br );
}

static void Skin_DrawImage( HDC dc, HDC memDC, HBITMAP bmp, const RECT *rc )
{
	HGDIOBJ oldBmp;

	if( !bmp )
		return;

	oldBmp = SelectObject( memDC, bmp );
	BitBlt( dc, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, memDC, 0, 0, SRCCOPY ); 
	SelectObject( memDC, oldBmp );
}

static LONG Skin_GetOffsetPos( const RECT *rc, const skinAnchor_t *ofs )
{
	switch( ofs->edge )
	{
	case SB_Top:
		return rc->top + ofs->amount;

	case SB_Left:
		return rc->left + ofs->amount;

	case SB_Bottom:
		return rc->bottom - ofs->amount;

	case SB_Right:
		return rc->right - ofs->amount;
	}

	return 0;
}

static RECT Skin_GetBtnRect( const RECT *rcWin, uint iBtn, const skinDef_t *skin )
{										  
	RECT rc;
	const skinBtn_t *btn = skin->btns + iBtn;

	rc.top = Skin_GetOffsetPos( rcWin, &btn->topAnchor );
	rc.left = Skin_GetOffsetPos( rcWin, &btn->leftAnchor );
	rc.bottom = rc.top + btn->height;
	rc.right = rc.left + btn->width;

	return rc;
}

static uint Skin_GetHotButton( HWND hwnd, const skinDef_t *skin )
{
	uint i;
	RECT rcScreen;
	POINT pt;

	GetWindowRect( hwnd, &rcScreen );
	GetCursorPos( &pt );
	
	for( i = 0; i < skin->numBtns; i++ )
	{
		RECT rc = Skin_GetBtnRect( &rcScreen, i, skin );
		if( PtInRect( &rc, pt ) )
			return i;
	}

	return (uint)~0;
}

static void Skin_PaintButtons( HWND hwnd, HDC dc, const RECT *rcWin, const skinDef_t *skin, HDC memDC )
{
	uint i;
	qboolean delDC = qfalse;
	uint mouseBtn;

	if( !memDC )
	{
		memDC = CreateCompatibleDC( dc );
		delDC = qtrue;
	}

	mouseBtn = Skin_GetHotButton( hwnd, skin );

	//buttons
	for( i = 0; i < skin->numBtns; i++ )
	{
		const skinBtn_t *btn = skin->btns + i;
		HBITMAP hb = btn->normal;
		RECT rcBtn = Skin_GetBtnRect( rcWin, i, skin );

		if( skin->state.pBtn )
		{
			if( i == skin->state.pBtn - 1 )
				hb = btn->pressed;

		}
		else if( i == mouseBtn )
			hb = btn->hover;

		Skin_DrawImage( dc, memDC, hb, &rcBtn );
	}

	if( delDC )
		DeleteDC( memDC );
}

static void Skin_Paint( HWND hwnd, HDC dc, const skinDef_t *skin )
{
	RECT rc;
	RECT rcWin;

	HDC memDC;

	GetWindowRect( hwnd, &rcWin );
	OffsetRect( &rcWin, -rcWin.left, -rcWin.top );

	//set up for raw image drawing
	memDC = CreateCompatibleDC( dc );

	//fill top
	rc.top = rcWin.top;
	rc.left = rcWin.left + skin->left.size;
	rc.bottom = rcWin.top + skin->top.size;
	rc.right = rcWin.right - skin->right.size;
	Skin_FillRect( dc, memDC, skin->top.img, &rc );

	//fill left
	rc.top = rcWin.top + skin->top.size;
	rc.left = rcWin.left;
	rc.bottom = rcWin.bottom - skin->bottom.size;
	rc.right = rcWin.left + skin->left.size;
	Skin_FillRect( dc, memDC, skin->left.img, &rc );

	//fill bottom
	rc.top = rcWin.bottom - skin->bottom.size;
	rc.left = rcWin.left + skin->left.size;
	rc.bottom = rcWin.bottom;
	rc.right = rcWin.right - skin->right.size;
	Skin_FillRect( dc, memDC, skin->bottom.img, &rc );

	//fill right
	rc.top = rcWin.top + skin->top.size;
	rc.left = rcWin.right - skin->right.size;
	rc.bottom = rcWin.bottom - skin->bottom.size;
	rc.right = rcWin.right;
	Skin_FillRect( dc, memDC, skin->right.img, &rc );

	//top left
	rc.top = rcWin.top;
	rc.left = rcWin.left;
	rc.bottom = rcWin.top + skin->top.size;
	rc.right = rcWin.left + skin->left.size;
	Skin_DrawImage( dc, memDC, skin->tlcorner, &rc );

	//top right
	rc.top = rcWin.top;
	rc.left = rcWin.right - skin->right.size;
	rc.bottom = rcWin.top + skin->top.size;
	rc.right = rcWin.right;
	Skin_DrawImage( dc, memDC, skin->trcorner, &rc );

	//bottom left
	rc.top = rcWin.bottom - skin->bottom.size;
	rc.left = rcWin.left;
	rc.bottom = rcWin.bottom;
	rc.right = rcWin.left + skin->left.size;
	Skin_DrawImage( dc, memDC, skin->blcorner, &rc );

	//bottom right
	rc.top = rcWin.bottom - skin->bottom.size;
	rc.left = rcWin.right - skin->right.size;
	rc.bottom = rcWin.bottom;
	rc.right = rcWin.right;
	Skin_DrawImage( dc, memDC, skin->brcorner, &rc );

	Skin_PaintButtons( hwnd, dc, &rcWin, skin, memDC );

	//done drawing bitmaps
	DeleteDC( memDC );
}

static qboolean Skin_MouseMove( HWND hwnd, skinDef_t *skin )
{
	RECT rcWin;
	TRACKMOUSEEVENT tme;

	uint iBtn;

	HDC dc = GetDCEx( hwnd, NULL, DCX_WINDOW | DCX_CLIPSIBLINGS | DCX_CACHE );
	
	GetWindowRect( hwnd, &rcWin );
	OffsetRect( &rcWin, -rcWin.left, -rcWin.top );
	
	Skin_PaintButtons( hwnd, dc, &rcWin, skin, NULL );

	ReleaseDC( hwnd, dc );

	iBtn = Skin_GetHotButton( hwnd, skin );
	
	tme.cbSize = sizeof( tme );
	tme.hwndTrack = hwnd;

	if( iBtn == (uint)~0 )
	{
		tme.dwFlags = TME_CANCEL | TME_HOVER | TME_NONCLIENT;

		TrackMouseEvent( &tme );
	}
	else
	{
		tme.dwFlags = TME_HOVER | TME_NONCLIENT;
		tme.dwHoverTime = HOVER_DEFAULT;

		TrackMouseEvent( &tme );
	}

	tme.dwFlags = TME_LEAVE | TME_NONCLIENT;
	TrackMouseEvent( &tme );

	return qfalse; //send it on so DefWndProc moves the window for us
}

static qboolean Skin_MouseDown( HWND hwnd, skinDef_t *skin )
{
	RECT rcWin;
	uint iBtn;

	HDC dc;
	
	iBtn = Skin_GetHotButton( hwnd, skin );
	if( iBtn != (uint)~0 )
	{
		SetCapture( hwnd );
		skin->state.pBtn = iBtn + 1;
	}
	else
		skin->state.pBtn = 0;

	dc = GetDCEx( hwnd, NULL, DCX_WINDOW | DCX_CLIPSIBLINGS | DCX_CACHE );

	GetWindowRect( hwnd, &rcWin );
	OffsetRect( &rcWin, -rcWin.left, -rcWin.top );

	Skin_PaintButtons( hwnd, dc, &rcWin, skin, NULL );

	ReleaseDC( hwnd, dc );

	return qfalse; //send it on so DefWndProc moves the window for us
}

static qboolean Skin_MouseUp( HWND hwnd, skinDef_t *skin )
{
	RECT rcWin;

	HDC dc;
	
	if( skin->state.pBtn )
	{
		uint iBtn = Skin_GetHotButton( hwnd, skin );
		if( iBtn == skin->state.pBtn - 1 && skin->btns[iBtn].cmd[0] )
			//mouse up on hot button, do action
			ri.Cmd_ExecuteText( EXEC_APPEND, skin->btns[iBtn].cmd );

		SetCapture( NULL );

		skin->state.pBtn = 0;
	}

	dc = GetDCEx( hwnd, NULL, DCX_WINDOW | DCX_CLIPSIBLINGS | DCX_CACHE );

	GetWindowRect( hwnd, &rcWin );
	OffsetRect( &rcWin, -rcWin.left, -rcWin.top );

	Skin_PaintButtons( hwnd, dc, &rcWin, skin, NULL );

	ReleaseDC( hwnd, dc );

	return qfalse; //send it on so DefWndProc moves the window for us
}

static qboolean Skin_MouseLeave( HWND hwnd, const skinDef_t *skin )
{
	RECT rcWin;
	HDC dc = GetDCEx( hwnd, NULL, DCX_WINDOW | DCX_CLIPSIBLINGS | DCX_CACHE );

	GetWindowRect( hwnd, &rcWin );
	OffsetRect( &rcWin, -rcWin.left, -rcWin.top );
	
	Skin_PaintButtons( hwnd, dc, &rcWin, skin, NULL );

	ReleaseDC( hwnd, dc );

	return qtrue;
}

static qboolean Skin_MouseHover( HWND hwnd, skinDef_t *skin )
{
	REF_PARAM( hwnd );
	REF_PARAM( skin );

	return qtrue;
}

static LRESULT CALLBACK Skin_SubclassWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_NCCREATE:
		{
			skinDef_t *sd = (skinDef_t*)((CREATESTRUCT*)lParam)->lpCreateParams;
			SetWindowLongPtr( hwnd, GWL_SKINDEF, (LONG_PTR)sd );
		}
		break;

	case WM_NCDESTROY:
		{
			skinDef_t *skin = (skinDef_t*)GetWindowLongPtr( hwnd, GWL_SKINDEF );

			if( skin )
				Skin_Free( skin );

			SetWindowLongPtr( hwnd, GWL_SKINDEF, 0 );
		}
		break;

	case WM_NCCALCSIZE:
		{
			skinDef_t *sd = (skinDef_t*)GetWindowLongPtr( hwnd, GWL_SKINDEF );
			if( !sd )
				//no skin, go to standard Windows frames
				break;

			if( wParam )
			{
				NCCALCSIZE_PARAMS *ncp = (NCCALCSIZE_PARAMS*)lParam;
				RECT *rc = ncp->rgrc + 0;

				rc->top += sd->top.size;
				rc->left += sd->left.size;
				rc->right -= sd->right.size;
				rc->bottom -= sd->bottom.size;

				return WVR_REDRAW;
			}
			else
			{
				RECT *rc = (RECT*)lParam;

				rc->top += sd->top.size;
				rc->left += sd->left.size;
				rc->right -= sd->right.size;
				rc->bottom -= sd->bottom.size;
				
				return 0;
			}
		}
		break;

	case WM_NCPAINT:
		{
			skinDef_t *sd = (skinDef_t*)GetWindowLongPtr( hwnd, GWL_SKINDEF );

			HDC dc;

			if( !sd )
				//no skin, go to standard drawing
				break;

			dc = GetDCEx( hwnd, NULL, DCX_WINDOW | DCX_CLIPSIBLINGS | DCX_CACHE );
			if( !dc )
				//maybe DefWndProc can salvage the border...
				break;
														
			Skin_Paint( hwnd, dc, sd );

			ReleaseDC( hwnd, dc );
			return 0;
		}
		break;

	case WM_NCHITTEST:
		{
			skinDef_t *sd = (skinDef_t*)GetWindowLongPtr( hwnd, GWL_SKINDEF );

			uint i;
			RECT rc;
			POINT pt = { 0, 0 };

			if( !sd )
				//no skin, go to standard Windows stuff
				break;

			GetClientRect( hwnd, &rc );
			ClientToScreen( hwnd, &pt );
			OffsetRect( &rc, pt.x, pt.y );

			pt.x = GET_X_LPARAM( lParam );
			pt.y = GET_Y_LPARAM( lParam );

			if( PtInRect( &rc, pt ) )
				return HTCLIENT;

			GetWindowRect( hwnd, &rc );
			GetCursorPos( &pt );

			for( i = 0; i < sd->numBtns; i++ )
			{
				RECT btnRC = Skin_GetBtnRect( &rc, i, sd );
				if( PtInRect( &btnRC, pt ) )
					return HTBORDER;
			}

			return HTCAPTION;
		}
		break;

	case WM_NCMOUSEMOVE:
		{
			skinDef_t *sd = (skinDef_t*)GetWindowLongPtr( hwnd, GWL_SKINDEF );
			if( !sd )
				//no skin, go to standard Windows stuff
				break;

			if( Skin_MouseMove( hwnd, sd ) )
				return 0;
		}
		break;

	case WM_NCLBUTTONDOWN:
		{
			skinDef_t *sd = (skinDef_t*)GetWindowLongPtr( hwnd, GWL_SKINDEF );
			if( !sd )
				//no skin, go to standard Windows stuff
				break;

			if( Skin_MouseDown( hwnd, sd ) )
				return 0;
		}
		break;

	case WM_NCLBUTTONUP:
		{
			skinDef_t *sd = (skinDef_t*)GetWindowLongPtr( hwnd, GWL_SKINDEF );
			if( !sd )
				//no skin, go to standard Windows stuff
				break;

			if( Skin_MouseUp( hwnd, sd ) )
				return 0;
		}
		break;

	case WM_LBUTTONUP:
		{
			skinDef_t *sd = (skinDef_t*)GetWindowLongPtr( hwnd, GWL_SKINDEF );
			if( !sd )
				//no skin, go to standard Windows stuff
				break;

			//hackery, windows won't send NCLBUTTONDOWN when there's capture,
			//use this instead
			if( sd->state.pBtn )
				if( Skin_MouseUp( hwnd, sd ) )
					return 0;
		}
		break;

	case WM_NCMOUSELEAVE:
		{
			skinDef_t *sd = (skinDef_t*)GetWindowLongPtr( hwnd, GWL_SKINDEF );
			if( !sd )
				//no skin, go to standard Windows stuff
				break;

			if( Skin_MouseLeave( hwnd, sd ) )
				return 0;
		}
		break;

	case WM_NCMOUSEHOVER:
		{
			skinDef_t *sd = (skinDef_t*)GetWindowLongPtr( hwnd, GWL_SKINDEF );
			if( !sd )
				//no skin, go to standard Windows stuff
				break;

			if( Skin_MouseHover( hwnd, sd ) )
				return 0;
		}
		break;

	default:
		break;
	}

	return CallWindowProc( GLW_SubclassWndProc, hwnd, uMsg, wParam, lParam );
}

void Skin_AdjustWindowRect( RECT *rc, const skinDef_t *skin )
{
	rc->top -= skin->top.size;
	rc->left -= skin->left.size;
	rc->right += skin->right.size;
	rc->bottom += skin->bottom.size;
}

void Skin_RegisterClass( void )
{
	WNDCLASS wc;

	WinVars_t *winVars = (WinVars_t*)ri.PlatformGetVars();

	wc.style = 0;
	wc.lpfnWndProc = Skin_SubclassWndProc; //intended wndproc is in glw_state.wndproc, GLW proc will call it
	wc.cbClsExtra = 0;
	wc.cbWndExtra = SKIN_WNDEXTRA;
	wc.hInstance = winVars->hInstance;
	wc.hIcon = LoadIcon( winVars->hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WINDOW_CLASS_NAME;

	UnregisterClass( WINDOW_CLASS_NAME, winVars->hInstance );

	if( !RegisterClass( &wc ) )
		ri.Error( ERR_FATAL, "GLW-skin: could not register window class" );
}

HWND Skin_CreateWnd( skinDef_t *skin, int x, int y, int w, int h )
{
	HWND hWnd;
	WinVars_t *winVars = (WinVars_t*)ri.PlatformGetVars();

	hWnd = CreateWindowEx( WS_EX_APPWINDOW, WINDOW_CLASS_NAME, skin->caption, WS_POPUP,
		x, y, w, h, NULL, NULL, winVars->hInstance, skin );

	if( !hWnd )
		return NULL;

	if( skin->roundX && skin->roundY )
	{
		RECT rc;
		HRGN rgn;
		
		GetWindowRect( hWnd, &rc );

		rgn = CreateRoundRectRgn( 0, 0, rc.right - rc.left, rc.bottom - rc.top, skin->roundX, skin->roundY );
		
		if( rgn )
			SetWindowRgn( hWnd, rgn, TRUE );
	}

	return hWnd;
}