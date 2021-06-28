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

#include "../qcommon/qcommon.h"

#include "../sql/sql.h"

extern sqlInfo_t com_db;


static void GLW_InitExtensions( void );

glwstate_t glw_state;

cvar_t		*vid_xpos;				// X coordinate of window position
cvar_t		*vid_ypos;				// Y coordinate of window position
cvar_t		*r_allowSoftwareGL;		// don't abort out if the pixelformat claims software

bool glimp_suspendRender;
static uint suspendRenderCount;

static int GLW_PickPFD( PIXELFORMATDESCRIPTOR *pfds, int *samples, int numPFDs, PIXELFORMATDESCRIPTOR *pPFD )
{
	int i;
	int bestMatch = 0;
	int iSamples = 0;

	if( samples )
	{
		if( r_fsaa->integer > 0 )
			iSamples = r_fsaa->integer;
	}

	// look for a best match
	for( i = 1; i <= numPFDs; i++ )
	{
		//
		// make sure this has hardware acceleration
		//
		if ( ( pfds[i].dwFlags & PFD_GENERIC_FORMAT ) != 0 ) 
		{
			if ( !r_allowSoftwareGL->integer )
			{
				if ( r_verbose->integer )
				{
					ri.Printf( PRINT_ALL, "...PFD %d rejected, software acceleration\n", i );
				}
				continue;
			}
		}

		// verify pixel type
		if ( pfds[i].iPixelType != PFD_TYPE_RGBA )
		{
			if ( r_verbose->integer )
			{
				ri.Printf( PRINT_ALL, "...PFD %d rejected, not RGBA\n", i );
			}
			continue;
		}

		// verify proper flags
		if ( ( ( pfds[i].dwFlags & pPFD->dwFlags ) & pPFD->dwFlags ) != pPFD->dwFlags ) 
		{
			if ( r_verbose->integer )
			{
				ri.Printf( PRINT_ALL, "...PFD %d rejected, improper flags (%x instead of %x)\n", i, pfds[i].dwFlags, pPFD->dwFlags );
			}
			continue;
		}

		// verify enough bits
		if ( pfds[i].cDepthBits < 15 )
		{
			continue;
		}
		if ( ( pfds[i].cStencilBits < 4 ) && ( pPFD->cStencilBits > 0 ) )
		{
			continue;
		}

		//
		// selection criteria (in order of priority):
		// 
		//  PFD_STEREO
		//  colorBits
		//  depthBits
		//  stencilBits
		//  multisample samples
		//
		if ( bestMatch )
		{
			// check stereo
			if ( ( pfds[i].dwFlags & PFD_STEREO ) && ( !( pfds[bestMatch].dwFlags & PFD_STEREO ) ) && ( pPFD->dwFlags & PFD_STEREO ) )
			{
				bestMatch = i;
				continue;
			}
			
			if ( !( pfds[i].dwFlags & PFD_STEREO ) && ( pfds[bestMatch].dwFlags & PFD_STEREO ) && ( pPFD->dwFlags & PFD_STEREO ) )
			{
				bestMatch = i;
				continue;
			}

			// check color
			if ( pfds[bestMatch].cColorBits != pPFD->cColorBits )
			{
				// prefer perfect match
				if ( pfds[i].cColorBits == pPFD->cColorBits )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( pfds[i].cColorBits > pfds[bestMatch].cColorBits )
				{
					bestMatch = i;
					continue;
				}
			}

			// check depth
			if ( pfds[bestMatch].cDepthBits != pPFD->cDepthBits )
			{
				// prefer perfect match
				if ( pfds[i].cDepthBits == pPFD->cDepthBits )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( pfds[i].cDepthBits > pfds[bestMatch].cDepthBits )
				{
					bestMatch = i;
					continue;
				}
			}

			// check stencil
			if ( pfds[bestMatch].cStencilBits != pPFD->cStencilBits )
			{
				// prefer perfect match
				if ( pfds[i].cStencilBits == pPFD->cStencilBits )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( ( pfds[i].cStencilBits > pfds[bestMatch].cStencilBits ) && 
					 ( pPFD->cStencilBits > 0 ) )
				{
					bestMatch = i;
					continue;
				}
			}

			// check multisample
			if( samples && samples[bestMatch] != iSamples )
			{
				//prefer a perfect match
				if ( samples[i] == iSamples )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more samples than our best, use it
				else if( samples[bestMatch] < iSamples && samples[i] > samples[bestMatch] )
				{
					bestMatch = i;
					continue;
				}
			}
		}
		else
		{
			bestMatch = i;
		}
	}
	
	if ( !bestMatch )
		return 0;

	if ( ( pfds[bestMatch].dwFlags & PFD_GENERIC_FORMAT ) != 0 )
	{
		if ( !r_allowSoftwareGL->integer )
		{
			ri.Printf( PRINT_ALL, "...no hardware acceleration found\n" );
			return 0;
		}
		else
		{
			ri.Printf( PRINT_ALL, "...using software emulation\n" );
		}
	}
	else if ( pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED )
	{
		ri.Printf( PRINT_ALL, "...MCD acceleration found\n" );
	}
	else
	{
		ri.Printf( PRINT_ALL, "...hardware acceleration found\n" );
	}

	*pPFD = pfds[bestMatch];

	if( samples && samples[bestMatch] )
	{
		glConfig.fsaaSamples = samples[bestMatch];
		ri.Printf( PRINT_ALL, "...selected %i sample FSAA format\n", samples[bestMatch] );
	}
	else
		glConfig.fsaaSamples = 1;

	return bestMatch;
}

static int GLW_ChoosePFDEx( PIXELFORMATDESCRIPTOR *pPFD )
{
	//create a temporary window to grab a GL context and
	//enumerate from - SetPixelFormat restriction

	int x, y, w, h;
	int numFormats;
	int i;
	int ret = 0;

	HWND tmpWnd;
	HDC tmpDC;
	HGLRC tmpRC;

	WNDCLASS tmpClass;
	ATOM tmpClassAtom;

	WinVars_t *winVars = (WinVars_t*)ri.PlatformGetVars();

	tmpClass.cbClsExtra = 0;
	tmpClass.cbWndExtra = 0;
	tmpClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	tmpClass.hCursor = LoadCursor( NULL, IDC_ARROW );
	tmpClass.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	tmpClass.hInstance = winVars->hInstance;
	tmpClass.lpfnWndProc = &DefWindowProc;
	tmpClass.lpszClassName = "GL_ENUM_WND";
	tmpClass.lpszMenuName = NULL;
	tmpClass.style = CS_OWNDC;

	tmpClassAtom = RegisterClass( &tmpClass );

	if( winVars->hWnd )
	{
		RECT tmpRC;

		GetWindowRect( winVars->hWnd, &tmpRC );
		x = tmpRC.left;
		y = tmpRC.top;
		w = tmpRC.right - tmpRC.left;
		h = tmpRC.bottom - tmpRC.top;
	}
	else
	{
		x = CW_USEDEFAULT;
		y = CW_USEDEFAULT;
		w = CW_USEDEFAULT;
		h = CW_USEDEFAULT;
	}

	tmpWnd = CreateWindow( MAKEINTATOM( tmpClassAtom ), "GL_ENUM_WND", WS_OVERLAPPEDWINDOW,
		x, y, w, h, NULL, NULL, winVars->hInstance, NULL );
	tmpDC = GetDC( tmpWnd );

	numFormats = DescribePixelFormat( tmpDC, 1, sizeof( PIXELFORMATDESCRIPTOR ), NULL );

	for( i = 1; i <= numFormats; i++ )
	{
		PIXELFORMATDESCRIPTOR pfd;
		pfd.nSize = sizeof( pfd );
		DescribePixelFormat( tmpDC, i, sizeof( PIXELFORMATDESCRIPTOR ), &pfd );

		if( pfd.dwFlags & PFD_SUPPORT_OPENGL )
		{
			if( SetPixelFormat( tmpDC, i, &pfd ) )
				break;
		}
	}

	if( i <= numFormats )
	{
		tmpRC = wglCreateContext( tmpDC );
		wglMakeCurrent( tmpDC, tmpRC );

		if( glewInit() == GLEW_OK && WGLEW_ARB_pixel_format )
		{
			PIXELFORMATDESCRIPTOR *pfds = (PIXELFORMATDESCRIPTOR*)ri.Hunk_AllocateTempMemory( sizeof( PIXELFORMATDESCRIPTOR ) * (numFormats + 1) );
			int *samples = GLEW_ARB_multisample ? (int*)ri.Hunk_AllocateTempMemory( sizeof( int ) * (numFormats + 1) ) : NULL;

			for( i = 1; i <= numFormats; i++ )
			{
				pfds[i].nSize = sizeof( PIXELFORMATDESCRIPTOR );
				DescribePixelFormat( tmpDC, i, sizeof( PIXELFORMATDESCRIPTOR ), pfds + i );

				if( samples )
				{
					int qAttribs[2] = { WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB };
					int vAttribs[2];

					wglGetPixelFormatAttribivARB( tmpDC, i, 0, 2, qAttribs, vAttribs );
					samples[i] = vAttribs[0] ? vAttribs[1] : 0;
				}
			}

			ret = GLW_PickPFD( pfds, samples, numFormats, pPFD );

			if( samples )
				ri.Hunk_FreeTempMemory( samples );
			ri.Hunk_FreeTempMemory( pfds );
		}

		wglMakeCurrent( NULL, NULL );
		wglDeleteContext( tmpRC );
	}

	ReleaseDC( tmpWnd, tmpDC );
	DestroyWindow( tmpWnd );

	UnregisterClass( MAKEINTATOM( tmpClassAtom ), winVars->hInstance ); 

	return ret;
}

static int GLW_ChoosePFD( HDC hDC, PIXELFORMATDESCRIPTOR *pPFD )
{
	PIXELFORMATDESCRIPTOR *pfds;
	int numFormats;
	int i;
	
	ri.Printf( PRINT_ALL, "...GLW_ChoosePFD( %d, %d, %d )\n", ( int ) pPFD->cColorBits, ( int ) pPFD->cDepthBits, ( int ) pPFD->cStencilBits );

	// count number of PFDs
	numFormats = DescribePixelFormat( hDC, 1, sizeof( PIXELFORMATDESCRIPTOR ), NULL );
	
	ri.Printf( PRINT_ALL, "...%d PFDs found\n", numFormats );

	pfds = (PIXELFORMATDESCRIPTOR*)ri.Hunk_AllocateTempMemory( sizeof( PIXELFORMATDESCRIPTOR ) * (numFormats  + 1) );
	// grab information
	for( i = 1; i <= numFormats; i++ )
	{
		pfds[i].nSize = sizeof( PIXELFORMATDESCRIPTOR );
		DescribePixelFormat( hDC, i, sizeof( PIXELFORMATDESCRIPTOR ), pfds + i );
	}

	i = GLW_PickPFD( pfds, NULL, numFormats, pPFD );

	ri.Hunk_FreeTempMemory( pfds );

	return i;
}

static const PIXELFORMATDESCRIPTOR GLW_DEFAULT_PFD =
{
	sizeof( PIXELFORMATDESCRIPTOR ),	// size of this pfd
	1,									// version number
	PFD_DRAW_TO_WINDOW |				// support window
	PFD_SUPPORT_OPENGL |				// support OpenGL
	PFD_DOUBLEBUFFER,					// double buffered
	PFD_TYPE_RGBA,						// RGBA type
	24,									// 24-bit color depth
	0, 0, 0, 0, 0, 0,					// color bits ignored
	0,									// no alpha buffer
	0,									// shift bit ignored
	0,									// no accumulation buffer
	0, 0, 0, 0, 						// accum bits ignored
	24,									// 24-bit z-buffer	
	8,									// 8-bit stencil buffer
	0,									// no auxiliary buffer
	PFD_MAIN_PLANE,						// main layer
	0,									// reserved
	0, 0, 0								// layer masks ignored
};

static void GLW_CreatePFD( PIXELFORMATDESCRIPTOR *pPFD )
{
    PIXELFORMATDESCRIPTOR src = GLW_DEFAULT_PFD;
	
	src.cColorBits = (BYTE)r_colorbits->integer;
	if( src.cColorBits == 0 )
		src.cColorBits = 32;
	else if( src.cColorBits < 16 )
		src.cColorBits = 16;
	else if( src.cColorBits < 24 )
		src.cColorBits = 24;

	src.cDepthBits = (BYTE)r_depthbits->integer;
	if( src.cDepthBits == 0 )
		src.cDepthBits = 24;
	else if( src.cDepthBits < 16 )
		src.cDepthBits = 16;

	src.cStencilBits = (BYTE)r_stencilbits->integer;

	if( r_stereo->integer )
	{
		ri.Printf( PRINT_ALL, "...attempting to use stereo\n" );
		src.dwFlags |= PFD_STEREO;
		glConfig.stereoEnabled = qtrue;
	}
	else
		glConfig.stereoEnabled = qfalse;

	*pPFD = src;
}

#define MSG_ERR_NO_GL															\
"Space Trader can not run on this machine since OpenGL (1.1 or later) failed to start."

#define MSG_ERR_OLD_VIDEO_DRIVER												\
"Space Trader can not run on this machine since it is "							\
"missing one or more required OpenGL extensions. Please "						\
"update your video card drivers and try again."

static void GLW_Fail( const char *msg )
{
	ri.Error( ERR_FATAL, va( "%s", msg ) );
}

void Glimp_PrintSystemInfo( void )
{
	Win_PrintCpuInfo();
	Win_PrintMemoryInfo();
	Win_PrintOSVersion();
}

static void GLW_CheckExtensions( void )
{
	bool good = true;
	char missingExts[4096];
	
	Q_strncpyz( missingExts, MSG_ERR_OLD_VIDEO_DRIVER "\nYour GL driver is missing support for:\n", sizeof( missingExts ) );

	if( glewInit() != GLEW_OK )
		GLW_Fail( MSG_ERR_NO_GL );

	if( !GLEW_VERSION_1_1 )
	{
		good = false;

		Q_strcat( missingExts, sizeof( missingExts ), "OpenGL 1.1" );
		ri.Printf( PRINT_ERROR, "OpenGL 1.1 is missing.\n" );
	}

	if( !(GLEW_VERSION_1_2 || GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp) )
	{
		good = false;

		Q_strcat( missingExts, sizeof( missingExts ), "GL_{EXT|SGIS}_texture_edge_clamp\n" );
		ri.Printf( PRINT_ERROR, "Missing GL_{EXT|SGIS}_texture_edge_clamp.\n" );
	}

	if( !(GLEW_VERSION_1_3 || GLEW_ARB_multitexture) )
	{
		good = false;

		Q_strcat( missingExts, sizeof( missingExts ), "GL_ARB_multitexture\n" );
		ri.Printf( PRINT_ERROR, "Missing GL_ARB_multitexture.\n" );
	}

	if( !(GLEW_VERSION_1_3 || GLEW_ARB_texture_env_combine || GLEW_EXT_texture_env_combine) )
	{
		good = false;	

		Q_strcat( missingExts, sizeof( missingExts ), "GL_{ARB|EXT}_texture_env_combine\n" );
		ri.Printf( PRINT_ERROR, "Missing GL_{ARB|EXT}_texture_env_combine.\n" );
	}

	if( !good ) {
		Glimp_PrintSystemInfo();
		ri.Printf( PRINT_ALL, "%s\n%s\n%s\n%s\n",
				(const char*)glGetString( GL_VENDOR ),
				(const char*)glGetString( GL_RENDERER ),
				(const char*)glGetString( GL_VERSION ),
				(const char*)glGetString( GL_EXTENSIONS ) );
		GLW_Fail( missingExts );
	}
}

static bool GLW_ValidateFSMonitor( const char *mon )
{
	bool goodMon;

	sql_prepare( &com_db, "SELECT gl_level FROM monitors SEARCH dev_name ?1" );
	sql_bindtext( &com_db, 1, mon );
	
	if( sql_step( &com_db ) )
	{
		goodMon = sql_columnasint( &com_db, 0 ) > 0;
	}
	else
		goodMon = false;

	sql_done( &com_db );

	return goodMon;
}

static bool GLW_ValidateFSMode( const char *mode, const char *mon )
{
	int w, h;
	bool goodMode;

	w = atoi( COM_Parse( &mode ) );
	h = atoi( COM_Parse( &mode ) );

	sql_prepare( &com_db, "SELECT COUNT( * ) FROM fsmodes SEARCH dev_name ?1 WHERE w=?2 AND h=?3" );
	sql_bindtext( &com_db, 1, mon );
	sql_bindint( &com_db, 2, w );
	sql_bindint( &com_db, 3, h );
	sql_step( &com_db );

	goodMode = sql_columnasint( &com_db, 0 ) >= 1;

	sql_done( &com_db );

	return goodMode;
}

static const char* GLW_GetDefaultFSMode( const char *mon )
{
	const char *ret;

	if( GLW_ValidateFSMode( GLW_DEFAULT_FS_RESOLUTION, mon ) )
		return GLW_DEFAULT_FS_RESOLUTION;

	sql_prepare( &com_db, "SELECT w:plain || ' ' || h:plain FROM fsmodes SEARCH dev_name ?1 LIMIT 1" );
	sql_bindtext( &com_db, 1, mon );

	if( sql_step( &com_db ) )
		ret = va( "%s", sql_columnastext( &com_db, 0 ) );
	else
		ret = GLW_DEFAULT_FS_RESOLUTION; //return it anyway??

	sql_done( &com_db );

	return ret;
}

static const char* GLW_GetDefaultFSMonitor( void )
{
	int max_level;
	const char *ret = 0;

	sql_prepare( &com_db, "SELECT MAX( gl_level ) FROM monitors" );
	if( sql_step( &com_db ) )
		max_level = sql_columnasint( &com_db, 0 );
	else
		max_level = -1;
	sql_done( &com_db );

	if( max_level >= 0 )
	{
		sql_prepare( &com_db, "SELECT dev_name FROM monitors SEARCH gl_level ?" );
		sql_bindint( &com_db, 1, max_level );
	}
	else
		sql_prepare( &com_db, "SELECT dev_name FROM monitors" );

	//pick the first monitor that supports the default resolution
	while( sql_step( &com_db ) )
	{
		const char *mon = sql_columnastext( &com_db, 0 );

		if( GLW_ValidateFSMode( GLW_DEFAULT_FS_RESOLUTION, mon ) )
		{
			ret = va( "%s", mon );
			break;
		}
	}
	sql_done( &com_db );

	//no good, try to pick the first monitor that supports the best GL support level
	if( !ret )
	{
		if( max_level < 0 )
			max_level = 0;

		sql_prepare( &com_db, "SELECT dev_name FROM monitors SEARCH gl_level ? LIMIT 1" );
		sql_bindint( &com_db, 1, max_level );

		if( sql_step( &com_db ) )
			ret = va( "%s", sql_columnastext( &com_db, 0 ) );
		
		sql_done( &com_db );
	}

	//???
	if( !ret )
		ret = "";

	return ret;
}

static void GLW_ValidateFSModeCvar()
{
	const char *mon, *val;

	if( !r_fsmode )
		return;

	mon = r_fsmonitor->latchedString ? r_fsmonitor->latchedString : r_fsmonitor->string;
	val = r_fsmode->latchedString ? r_fsmode->latchedString : r_fsmode->string;

	if( !GLW_ValidateFSMode( val, mon ) )
	{
		//trying to set this to something invalid - no!
		const char *def = GLW_GetDefaultFSMode( mon );

		//check to make sure we're not setting it each frame
		if( Q_stricmp( def, r_fsmode->string ) != 0 )
		{
			ri.Printf( PRINT_WARNING, "Invalid full-screen mode. Restoring default setting.\n" );
			ri.Cvar_Set( r_fsmode->name, def );
		}
	}
}

static void GLW_ValidateFSMonitorCvar()
{
	const char *val;

	if( !r_fsmonitor )
		return;

	val = r_fsmonitor->latchedString ? r_fsmonitor->latchedString : r_fsmonitor->string;

	if( !GLW_ValidateFSMonitor( val ) )
	{
		//trying to set this to something invalid - no!
		const char *def = GLW_GetDefaultFSMonitor();

		//check to make sure we're not setting it each frame
		if( Q_stricmp( def, r_fsmonitor->string ) != 0 )
		{
			ri.Printf( PRINT_WARNING, "Invalid full-screen monitor. Restoring default setting.\n" );
			ri.Cvar_Set( r_fsmonitor->name, def ); //set it back to what it was
			GLW_ValidateFSModeCvar();
		}
	}
}

static int GLW_GetDisplayLevel( HDC dc )
{
	int i;
	PIXELFORMATDESCRIPTOR pfd;
	
	pfd = GLW_DEFAULT_PFD;
	i = GLW_ChoosePFD( dc, &pfd );

	if( !i )
		return 0;

	pfd.nSize = sizeof( pfd );
	DescribePixelFormat( dc, i, sizeof( PIXELFORMATDESCRIPTOR ), &pfd );

	if( (pfd.dwFlags & PFD_SUPPORT_OPENGL) == 0 )
		return 0;

	if( pfd.dwFlags & PFD_GENERIC_FORMAT )
		return 1; //software GL

	return 2; //full GL
}

static BOOL CALLBACK GLW_GetMonitorDisplayModes( HMONITOR mon, HDC dc, LPRECT rc, LPARAM userData )
{
	int i;
	DEVMODE mode;
	MONITORINFOEX info;
	int gl_level;

	REF_PARAM( rc );
	REF_PARAM( userData );

	info.cbSize = sizeof( info );
	GetMonitorInfo( mon, (LPMONITORINFO)&info );

	gl_level = 3;//GLW_GetDisplayLevel( dc );

	sql_bindtext( &com_db, 1, info.szDevice );
	sql_bindint( &com_db, 2, info.rcMonitor.left );
	sql_bindint( &com_db, 3, info.rcMonitor.top );
	sql_bindint( &com_db, 4, info.rcMonitor.right - info.rcMonitor.left );
	sql_bindint( &com_db, 5, info.rcMonitor.bottom - info.rcMonitor.top );
	sql_bindint( &com_db, 6, gl_level );
	sql_step( &com_db );

	mode.dmSize = sizeof( mode );
	mode.dmDriverExtra = 0;
	for( i = 0; EnumDisplaySettingsEx( info.szDevice, i, &mode, 0 ) != 0; i++ )
	{
		int id;

		if( mode.dmBitsPerPel < 16 )
			continue;

		if( mode.dmPelsWidth < 640 || mode.dmPelsHeight < 480 )
			continue;

		sql_prepare( &com_db, "UPDATE OR INSERT fsmodes SET id=#,w=?1,h=?2,dev_name=?3 SEARCH dev_name ?3 WHERE w=?1 AND h=?2" );
		sql_bindint( &com_db, 1, (int)mode.dmPelsWidth );
		sql_bindint( &com_db, 2, (int)mode.dmPelsHeight );
		sql_bindtext( &com_db, 3, info.szDevice );
		sql_step( &com_db );
		sql_done( &com_db );

		//get the id of what we just added

		sql_prepare( &com_db, "SELECT id FROM fsmodes SEARCH dev_name ?3 WHERE w=?1 AND h=?2" );
		sql_bindint( &com_db, 1, (int)mode.dmPelsWidth );
		sql_bindint( &com_db, 2, (int)mode.dmPelsHeight );
		sql_bindtext( &com_db, 3, info.szDevice );
		sql_step( &com_db );
		id = sql_columnasint( &com_db, 0 );
		sql_done( &com_db );

		//and insert the other info into the other table

		sql_prepare( &com_db, "UPDATE OR INSERT fsmodes_ext SET id=?1,hz=?2,bpp=?3 SEARCH id ?1 WHERE hz=?2 AND bpp=?3" );
		sql_bindint( &com_db, 1, id );
		sql_bindint( &com_db, 2, (int)mode.dmDisplayFrequency );
		sql_bindint( &com_db, 3, (int)mode.dmBitsPerPel );
		sql_step( &com_db );
		sql_done( &com_db );
	}

	return TRUE;
}

static void GLW_GetDisplayModes( void )
{
	HDC dc;

	sql_exec( &com_db,

		"CREATE TABLE monitors"
		"("
			"dev_name STRING, "
			"x INTEGER, "
			"y INTEGER, "
			"w INTEGER, "
			"h INTEGER, "
			"gl_level INTEGER "
		");"

		"CREATE TABLE fsmodes"
		"("
			"id INTEGER, "
			"w INTEGER, "
			"h INTEGER, "
			"dev_name STRING " //references monitors.dev_name
		");"

		//really should be merged onto the prior - except then
		//the list gets too big and the GUI SELECT blows up and
		//I hate
		"CREATE TABLE fsmodes_ext"
		"("
			"id INTEGER, "
			"hz INTEGER, "
			"bpp INTEGER "
		");"
	);

	sql_prepare( &com_db, "INSERT INTO monitors(dev_name,x,y,w,h,gl_level) VALUES(?,?,?,?,?,?);" );

	dc = GetDC( NULL );
	EnumDisplayMonitors( dc, NULL, GLW_GetMonitorDisplayModes, 0 );
	ReleaseDC( NULL, dc );	
	
	sql_done( &com_db );
}

//try to set the r_fsmonitor field to
//whatever monitor the window is on
static void Q_EXTERNAL_CALL GLW_UseCurrentMonitor( void )
{
	RECT rc;

	int max_area = 0;
	int max_level = 0;
	char best_monitor[CCHDEVICENAME] = "";

	WinVars_t *winVars = (WinVars_t*)ri.PlatformGetVars();

	if( !winVars->hWnd )
		//no window = no clue
		return;

	GetWindowRect( winVars->hWnd, &rc );

	sql_prepare( &com_db, "SELECT x, y, w, h, gl_level, dev_name FROM monitors WHERE gl_level > 0" );
	while( sql_step( &com_db ) )
	{
		int l = sql_columnasint( &com_db, 0 );
		int t = sql_columnasint( &com_db, 1 );
		int r = l + sql_columnasint( &com_db, 2 );
		int b = t + sql_columnasint( &com_db, 3 );

		int area;

		int level = sql_columnasint( &com_db, 4 );
		const char *name = sql_columnastext( &com_db, 5 );

		if( l < rc.left )
			l = rc.left;

		if( r > rc.right )
			r = rc.right;

		if( t < rc.top )
			t = rc.top;

		if( b > rc.bottom )
			b = rc.bottom;

		area = (r - l) * (b - t);

		if( !area )
			//screen does not intersect window
			continue;

		if( level < max_level )
			continue;

		if( level == max_level && area < max_area )
			//only accept a screen of equal GL level if it owns more window area
			continue;

		max_level = level;
		max_area = area;
		Q_strncpyz( best_monitor, name, sizeof( best_monitor ) );
	}
	sql_done( &com_db );

	if( !best_monitor[0] )
		//nothing to do
		return;

	//set the monitor cvar and make sure we
	//didn't just invalidate the mode cvar
	ri.Cvar_Set( r_fsmonitor->name, best_monitor );
	GLW_ValidateFSModeCvar();
}

LRESULT CALLBACK GLW_SubclassWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{

	case WM_ACTIVATE:
		if( glw_state.cdsFullscreen )
		{	
			if( LOWORD( wParam ) == WA_INACTIVE )
			{
				//ensure that we're *behind* the window that's being brought forward (this will clear the TOPMOST flag)
				SetWindowPos( hwnd, GetForegroundWindow(), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
			}
			else
			{
				//get into the topmost list
				SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
			}
		}
		break;

	case WM_ERASEBKGND:
		if( tr.frameCount && !glimp_suspendRender )
			//as soon as we start rendering frames
			//we can stop filling the background
			return 1;
		break;

	case WM_PAINT:
		/*
			We don't want any GDI painting, but neither do we want
			Windows to keep sending paint events. Just let Windows
			know that we've got it covered and everything will be ok.
		*/
		ValidateRect( hwnd, NULL );
		return 0;

	case WM_MOVE:
		{
			if( !r_fullscreen->integer )
			{
				POINT pt;

				pt.x = 0;
				pt.y = 0;
				ClientToScreen( hwnd, &pt );
								
				ri.Cvar_Set( "vid_xpos", va( "%i", pt.x ) );
				ri.Cvar_Set( "vid_ypos", va( "%i", pt.y ) );
			}
		}
		break;

	case WM_DESTROY:
		ri.Printf( PRINT_DEVELOPER, "Main window got WM_DESTROY\n" );
		break;

	default:
		break;
	}

	return CallWindowProc( (WNDPROC)glw_state.wndproc, hwnd, uMsg & ~WM_PASS_MESSAGE_FLAG, wParam, lParam );
}

static void GLW_RegisterClass( void )
{
	WNDCLASS wc;
	static qboolean registered = qfalse;

	WinVars_t *winVars = (WinVars_t*)ri.PlatformGetVars();

	if( registered )
		return;

	wc.style = CS_OWNDC;
	wc.lpfnWndProc = GLW_SubclassWndProc; //intended wndproc is in glw_state.wndproc, GLW proc will call it
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
		ri.Error( ERR_FATAL, "GLW: could not register window class" );

	registered = qtrue;
}

static void GLW_KillGLWnd( void )
{
	WinVars_t *winVars = (WinVars_t*)ri.PlatformGetVars();

	if( glw_state.hGLRC )
	{
		wglMakeCurrent( NULL, NULL );
		wglDeleteContext( glw_state.hGLRC );

		glw_state.hGLRC = NULL;
	}

	if( glw_state.hDC )
	{
		ReleaseDC( winVars->hWnd, glw_state.hDC );

		glw_state.hDC = NULL;
	}

	if( winVars->hWnd )
	{
		DestroyWindow( winVars->hWnd );
		winVars->hWnd = NULL;
	}

	if( glw_state.cdsFullscreen )
	{
		ChangeDisplaySettings( NULL, 0 );
		glw_state.cdsFullscreen = qfalse;
	}
}

static void GLW_CreateGLWnd( void )
{
	int x, y, w, h;
	HWND hParent;
	float aspect;
	DWORD s, es;
	skinDef_t *skin;

	int refresh = 0;
	int colorDepth = 0;

	WinVars_t *winVars = (WinVars_t*)ri.PlatformGetVars();

	if( winVars->hWnd )
		return;

	x = 0;
	y = 0;

	if( r_fullscreen->integer )
	{
		int mode_id;

		char * res = r_fsmode->string;
		w = atoi( COM_Parse( &res ) );
		h = atoi( COM_Parse( &res ) );
		aspect = (float)w / (float)h;

		sql_prepare( &com_db, "SELECT x, y FROM monitors SEARCH dev_name ?" );
		sql_bindtext( &com_db, 1, r_fsmonitor->string );
		if( sql_step( &com_db ) )
		{
			x = sql_columnasint( &com_db, 0 );
			y = sql_columnasint( &com_db, 1 );
		}
		sql_done( &com_db );

		//find the settings mode id
		mode_id = -1;
		sql_prepare( &com_db, "SELECT id FROM fsmodes SEARCH dev_name ?1 WHERE w=?2 AND h=?3" );
		sql_bindtext( &com_db, 1, r_fsmonitor->string );
		sql_bindint( &com_db, 2, w );
		sql_bindint( &com_db, 3, h );
		if( sql_step( &com_db ) )
			mode_id = sql_columnasint( &com_db, 0 );
		sql_done( &com_db );

		//get a matching color mode
		sql_prepare( &com_db, "SELECT bpp FROM fsmodes_ext SEARCH id ?1" );
		sql_bindint( &com_db, 1, mode_id );

		while( sql_step( &com_db ) )
		{
			int bpp = sql_columnasint( &com_db, 0 );
			if( r_colorbits->integer )
			{
				if( bpp == r_colorbits->integer )
				{
					//take an exact match
					colorDepth = bpp;
					break;
				}

				if( bpp > r_colorbits->integer )
				{
					if( colorDepth < r_colorbits->integer || bpp < colorDepth )
						//if we must go over, take the smallest value that goes over
						colorDepth = bpp;
				}
				else if( bpp > colorDepth )
					colorDepth = bpp;
			}
			else if( bpp > colorDepth )
				colorDepth = bpp;
		}

		sql_done( &com_db );

		//get a matching refresh rate
		sql_prepare( &com_db, "SELECT hz FROM fsmodes_ext SEARCH id ?1 WHERE bpp=?2" );
		sql_bindint( &com_db, 1, mode_id );
		sql_bindint( &com_db, 2, colorDepth );

		while( sql_step( &com_db ) )
		{
			int hz = sql_columnasint( &com_db, 0 );
			if( r_displayRefresh->integer )
			{
				if( hz == r_displayRefresh->integer )
				{
					//take an exact match
					refresh = hz;
					break;
				}

				if( hz > r_displayRefresh->integer )
				{
					if( refresh < r_displayRefresh->integer || hz < refresh )
						//if we must go over, take the smallest value that goes over
						refresh = hz;
				}
				else if( hz > refresh )
					refresh = hz;
			}
			else if( hz > refresh )
				//take the highest refresh rate
				refresh = hz;
		}

		sql_done( &com_db );
	}
	else
	{
		if( !R_GetModeInfo( &w, &h, &aspect, r_mode->integer ) )
		{
			//fall back to special modes
			w = 0;
			h = 0;
		}
	}

	/*
		Clean out old display mode changes.

		Note that we *don't* skip this when we're going back into fullscreen
		as it tends to produce some aweful bugs on some Windows installations.
	*/
	if( glw_state.cdsFullscreen )
		ChangeDisplaySettings( NULL, 0 );

	//window style bits
	es = 0;
	s = 0;

	skin = NULL;

	if( r_fullscreen->integer )
	{
		//go into full screen mode

		RECT rc;
		HMONITOR monitor;
		MONITORINFOEX monitorInfo;

		hParent = 0;

		//make sure we're set up for multimon goodness
		if( winVars->hWndHost )
		{
			GetWindowRect( winVars->hWndHost, &rc );
		}
		else
		{
			rc.left = x;
			rc.top = y;
			rc.right = x + w;
			rc.bottom = y + h;
		}

		monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTONEAREST );

		monitorInfo.cbSize = sizeof( monitorInfo );
		GetMonitorInfo( monitor, (LPMONITORINFO)&monitorInfo );

		//if we got an invalid mode then use desktop resolution
		if( w == 0 )
			w = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
		if( h == 0 )
			h = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

		//change that monitor's display size to <w, h>
		//set the window rect to cover the display
		//skip the festival of desktop flickering if not changing resolution
		if( (monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left) != w ||
			(monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top) != h )
		{
			DEVMODE dm;
			memset( &dm, 0, sizeof( dm ) );

			dm.dmSize = sizeof( dm );
			
			dm.dmPelsWidth = w;
			dm.dmPelsHeight = h;
			dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

			if( refresh != 0 )
			{
				dm.dmDisplayFrequency = refresh;
				dm.dmFields |= DM_DISPLAYFREQUENCY;
			}

			if( colorDepth != 0 )
			{
				dm.dmBitsPerPel = colorDepth;
				dm.dmFields |= DM_BITSPERPEL;
			}

			if( ChangeDisplaySettingsEx( monitorInfo.szDevice, &dm, NULL, CDS_FULLSCREEN, NULL )
				!= DISP_CHANGE_SUCCESSFUL )
			{
				//try again without color bits and frequency
				dm.dmFields &= ~(DM_BITSPERPEL | DM_DISPLAYFREQUENCY);
				dm.dmBitsPerPel = 0;
				dm.dmDisplayFrequency = 0;

				if( ChangeDisplaySettingsEx( monitorInfo.szDevice, &dm, NULL, CDS_FULLSCREEN, NULL )
					!= DISP_CHANGE_SUCCESSFUL )
					//failed...
					ri.Printf( PRINT_WARNING, "Invalid fullscreen resolution, running at desktop resolution" );
			}
		}

		//get the new monitor info
		monitorInfo.cbSize = sizeof( monitorInfo );
		GetMonitorInfo( monitor, (LPMONITORINFO)&monitorInfo );

		x = monitorInfo.rcMonitor.left;
		y = monitorInfo.rcMonitor.top;
		w = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
		h = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

		s = WS_POPUP;
		es = WS_EX_APPWINDOW | WS_EX_TOPMOST;

		glw_state.cdsFullscreen = qtrue;
	}
	else if( winVars->hWndHost )
	{
		RECT rc;

		hParent = winVars->hWndHost;

		GetClientRect( winVars->hWndHost, &rc );
		x = rc.left;
		y = rc.top;
		w = rc.right - rc.left;
		h = rc.bottom - rc.top;

		s = WS_CHILD;
		es = WS_EX_NOPARENTNOTIFY;
	}
	else
	{
		RECT rc;
		HMONITOR monitor;
		MONITORINFO monitorInfo;
		qboolean usedefault = qfalse;

		if( w == 0 )
			w = 640;
		if( h == 0 )
			h = 480;

		vid_xpos = ri.Cvar_Get( "vid_xpos", va("%d",CW_USEDEFAULT), CVAR_ARCHIVE );
		vid_ypos = ri.Cvar_Get( "vid_ypos", "0", CVAR_ARCHIVE );

		x = vid_xpos->integer;
		y = vid_ypos->integer;

		if ( x == CW_USEDEFAULT ) {
			x = 0;
			usedefault = qtrue;
		}

		rc.left = x;
		rc.top = y;
		rc.right = x + w;
		rc.bottom = y + h;

		hParent = 0;

		if( r_skin->string[0] )
			skin = Skin_Load( r_skin->string );
		
		//account for the border frame
		if( skin )
		{
			s = WS_POPUP;
			es = WS_EX_APPWINDOW;

			Skin_AdjustWindowRect( &rc, skin );
			AdjustWindowRectEx( &rc, s, FALSE, es );
		}
		else
		{
			s = WS_OVERLAPPED | WS_SYSMENU | WS_BORDER | WS_CAPTION;
			es = WS_EX_APPWINDOW;

			AdjustWindowRectEx( &rc, s, FALSE, es );
		}

		x = rc.left;
		y = rc.top;
		w = rc.right - rc.left;
		h = rc.bottom - rc.top;

		//constrain to a monitor
		//this is important as we can't get a
		//pixel format if we're entirely off screen
		monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTONEAREST );

		monitorInfo.cbSize = sizeof( monitorInfo );
		GetMonitorInfo( monitor, &monitorInfo );

		//if we're not actually intersecting the monitor
		//(shoved off of the screen I guess) move back onto it

		if( x > monitorInfo.rcWork.right )
			//left window edge past right screen edge
			x = monitorInfo.rcWork.right - w;
		if( x + w < monitorInfo.rcWork.left )
			//right window edge past left screen edge
			x = monitorInfo.rcWork.left;

		if( y > monitorInfo.rcWork.bottom )
			//top window edge past bottom screen edge
			y = monitorInfo.rcWork.bottom - h;
		if( y + h < monitorInfo.rcWork.top )
			//bottom window edge past top screen edge
			y = monitorInfo.rcWork.top;

		glw_state.cdsFullscreen = qfalse;


		if ( usedefault ) {
			x = monitorInfo.rcMonitor.left + ((monitorInfo.rcMonitor.right-monitorInfo.rcMonitor.left)-w)/2;
			y = monitorInfo.rcMonitor.top + ((monitorInfo.rcMonitor.bottom-monitorInfo.rcMonitor.top)-h)/2;
		}
	}

	winVars->hWnd = NULL;

	if( skin )
	{
		Skin_RegisterClass();
		winVars->hWnd = Skin_CreateWnd( skin, x, y, w, h );
	}
	
	if( !winVars->hWnd )
	{
		GLW_RegisterClass();
		winVars->hWnd = CreateWindowEx( es, WINDOW_CLASS_NAME, WINDOW_CAPTION,
			s, x, y, w, h, hParent, NULL, winVars->hInstance, NULL );
	}

	//the window now owns the skin and will free it

	if( !winVars->hWnd )
		ri.Error( ERR_FATAL, "GLW: could not create window" );

	SetWindowPos( winVars->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW );

	ShowWindow( winVars->hWnd, SW_SHOWNORMAL );
	SetFocus( winVars->hWnd );

	//get the window to draw once (get rid of graphical clutter)
	UpdateWindow( winVars->hWnd );

	//fire up the GL
	glw_state.hDC = GetDC( winVars->hWnd );
	if( !glw_state.hDC )
		ri.Error( ERR_FATAL, "GLW: could not get window DC" );

	//set up the pixel format
	{
		int pixelFormat;
		PIXELFORMATDESCRIPTOR pfd;

		GLW_CreatePFD( &pfd );
		pixelFormat = GLW_ChoosePFDEx( &pfd );

		if( !pixelFormat )
		{
			pixelFormat = GLW_ChoosePFD( glw_state.hDC, &pfd );
			glConfig.fsaaSamples = 1;
		}

		if( !pixelFormat )
			ri.Error( ERR_FATAL, "GLW: no valid pixel format" );

		pfd.nSize = sizeof( pfd );
		DescribePixelFormat( glw_state.hDC, pixelFormat, sizeof( pfd ), &pfd );
		if( !SetPixelFormat( glw_state.hDC, pixelFormat, &pfd ) )
			ri.Error( ERR_FATAL, "GLW: could not set pixel format" );

		glConfig.colorBits = pfd.cColorBits;
		glConfig.depthBits = pfd.cDepthBits;
		glConfig.stencilBits = pfd.cStencilBits;
		glConfig.stereoEnabled = (pfd.dwFlags & PFD_STEREO) ? qtrue : qfalse;

		ri.Printf( PRINT_ALL, "Using Pixel Format %i\n", pixelFormat );
	}
		
	glw_state.hGLRC = wglCreateContext( glw_state.hDC );
	if( !glw_state.hGLRC || !wglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) )
		ri.Error( ERR_FATAL, "GLW: could not initialize GL" );

	GLW_CheckExtensions();

	//R_PerfInit();

	{
		//get the actual sizes, in case Windows constrained our window
		RECT rc;
		GetClientRect( winVars->hWnd, &rc );
		w = rc.right - rc.left;
		h = rc.bottom - rc.top;

		//fill in some glConfig stuff which *will* be referenced
		//by various subsystem init functions (i.e. Cg, bloom)
		glConfig.vidWidth = w;
		glConfig.vidHeight = h;
		glConfig.windowAspect = aspect;
	}

	glConfig.deviceSupportsGamma = qfalse;
	glConfig.isFullscreen = glw_state.cdsFullscreen ? qtrue : qfalse;

	glConfig.xscale = glConfig.vidWidth / 640.0f;
	glConfig.yscale = glConfig.vidHeight / 480.0f;

	if( glConfig.vidWidth * 480 > glConfig.vidHeight * 640 )
	{
		// wide screen
		glConfig.xscale	= glConfig.yscale;
		glConfig.xbias	= ((float)glConfig.vidWidth - (640.0F * glConfig.xscale)) * 0.5F;
	}
	else
	{
		// no wide screen
		glConfig.xbias	= 0.0f;
	}
}

static void GLW_InitExtensions( void )
{
	GLint glint;

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	if( GLEW_EXT_texture_env_add )
	{
		glConfig.textureEnvAddAvailable = qtrue;
		ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
	}
	else
	{
		glConfig.textureEnvAddAvailable = qfalse;
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}

	if( WGLEW_EXT_swap_control )
	{
		ri.Printf( PRINT_ALL, "...using WGL_EXT_swap_control\n" );
		r_swapInterval->modified = qtrue;	// force a set next frame
	}
	else
		ri.Printf( PRINT_ALL, "...WGL_EXT_swap_control not found\n" );

	if( GLEW_ARB_multitexture )
	{
		glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glint );
		glConfig.maxActiveTextures = glint;
	}

	glGetIntegerv( GL_MAX_TEXTURE_SIZE, &glint );
	glConfig.maxTextureSize = glint;
}

static void GLW_CheckForBustedDrivers()
{
	const char *failCode = 0;

	HDC dc = GetDC( NULL );
	__try
	{
		PIXELFORMATDESCRIPTOR tmp;
		memset( &tmp, 0, sizeof( tmp ) );
		tmp.nSize = sizeof( tmp );
		tmp.nVersion = 1;

		DescribePixelFormat( dc, 1, sizeof( PIXELFORMATDESCRIPTOR ), &tmp );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		failCode = "Broken DescribePixelFormat API.";
	}
	ReleaseDC( NULL, dc );

	if( failCode )
	{
		const char *msg = va(
			"An error occurred while initializing OpenGL.\n"
			"Please make sure that your drivers are up to date.\n\n"
			"GL Error Details: '%s'", failCode );
		GLW_Fail( msg );
	}
}

/*
** GLimp_Init
**
** This is the platform specific OpenGL initialization function.  It
** is responsible for loading OpenGL, initializing it, setting
** extensions, creating a window of the appropriate size, doing
** fullscreen manipulations, etc.  Its overall responsibility is
** to make sure that a functional OpenGL subsystem is operating
** when it returns to the ref.
*/
void GLimp_Init( void )
{
	char	buf[1024];
	cvar_t *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );
	cvar_t	*cv;

	ri.Printf( PRINT_ALL, "Initializing OpenGL subsystem\n" );

	GLW_CheckForBustedDrivers();

	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );

	// save off hInstance and wndproc
	cv = ri.Cvar_Get( "win_hinstance", "", 0 );
	sscanf( cv->string, "%i", (int*)&((WinVars_t*)ri.PlatformGetVars())->hInstance ); //FixMe: this is NOT 64-bit safe

	cv = ri.Cvar_Get( "win_wndproc", "", 0 );
	sscanf( cv->string, "%i", (int*)&glw_state.wndproc ); //FixMe: this is NOT 64-bit safe

	GLW_GetDisplayModes();

	r_mode = ri.Cvar_Get( "r_mode", "4", CVAR_ARCHIVE | CVAR_LATCH );
	r_fullscreen = ri.Cvar_Get( "r_fullscreen", "0", CVAR_ARCHIVE | CVAR_LATCH );
	
	r_fsmonitor = ri.Cvar_Get( "r_fsmonitor", GLW_GetDefaultFSMonitor(), CVAR_ARCHIVE | CVAR_LATCH );	
	GLW_ValidateFSMonitorCvar();

	r_fsmode = ri.Cvar_Get( "r_fsmode", GLW_GetDefaultFSMode( r_fsmonitor->string ), CVAR_ARCHIVE | CVAR_LATCH );
	GLW_ValidateFSModeCvar();

	GLW_CreateGLWnd();

	ri.Cmd_AddCommand( "vid_fsUseCurMon", GLW_UseCurrentMonitor );

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, (const char*)glGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (const char*)glGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );
	Q_strncpyz( glConfig.version_string, (const char*)glGetString( GL_VERSION ), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, (const char*)glGetString( GL_EXTENSIONS ), sizeof( glConfig.extensions_string ) );

	//
	// chipset specific configuration
	//
	Q_strncpyz( buf, glConfig.renderer_string, sizeof(buf) );
	Q_strlwr( buf );

	//
	// NOTE: if changing cvars, do it within this block.  This allows them
	// to be overridden when testing driver fixes, etc. but only sets
	// them to their default state when the hardware is first installed/run.
	//
	if( Q_stricmp( lastValidRenderer->string, glConfig.renderer_string ) )
		ri.Cvar_Set( "r_textureMode", "LinearMipLinear" );
	
	//
	// this is where hardware specific workarounds that should be
	// detected/initialized every startup should go.
	//

	/*
	if( strstr( buf, "banshee" ) || strstr( buf, "voodoo3" ) )
		;
	else if( strstr( buf, "voodoo graphics/1 tmu/2 mb" ) )
		;
	else if( strstr( buf, "glzicd" ) )
		;
	else if( strstr( buf, "rage pro" ) || strstr( buf, "Rage Pro" ) || strstr( buf, "ragepro" ) )
		;
	else if( strstr( buf, "rage 128" ) )
		;
	else if( strstr( buf, "permedia2" ) )
		;
	else if( strstr( buf, "riva 128" ) )
		;
	else if( strstr( buf, "riva tnt " ) )
		;
	*/

	ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );

	GLW_InitExtensions();

	glimp_suspendRender = false;
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.
*/
void GLimp_Shutdown( void )
{
	ri.Cmd_RemoveCommand( "vid_fsUseCurMon" );

	GLW_KillGLWnd();

	memset( &glConfig, 0, sizeof( glConfig ) );
}

static bool Glimp_WindowIsVisible( HWND hwnd )
{
	RECT rc;

	if( !IsWindowVisible( hwnd ) )
		return false;

	if( IsIconic( hwnd ) )
		return false;

	GetClientRect( hwnd, &rc );
	if( rc.right == rc.left ||
		rc.bottom == rc.top )
		return false;

	return true;
}

static bool Glimp_WindowAndParentsAreVisible( HWND wnd )
{
	while( wnd )
	{
		if( !Glimp_WindowIsVisible( wnd ) )
			return false;

		wnd = GetParent( wnd );
	}

	return true;
}

void GLimp_EndFrame( void )
{
	BOOL bSwap;

	WinVars_t *wv = (WinVars_t*)ri.PlatformGetVars();

	if( !wv->hWnd )
		return;

	if( r_fsmonitor->modified )
	{
		r_fsmonitor->modified = qfalse;

		GLW_ValidateFSMonitorCvar();
	}

	if( r_fsmode->modified )
	{
		//keep this valid at all times
		r_fsmode->modified = qfalse;

		GLW_ValidateFSModeCvar();
	}

	if( r_swapInterval->modified )
	{
		r_swapInterval->modified = qfalse;

		if( !glConfig.stereoEnabled )
		{	// why?
			if( WGLEW_EXT_swap_control )
				wglSwapIntervalEXT( r_swapInterval->integer );
		}
	}

	{
#if 0
		LARGE_INTEGER start, end, freq;

		QueryPerformanceCounter( &start );
#endif

		bSwap = SwapBuffers( glw_state.hDC );

#if 0
		QueryPerformanceCounter( &end );

		QueryPerformanceFrequency( &freq );

		ri.Printf( PRINT_ALL, S_COLOR_CYAN"%10f\n", (float)((double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart) );
#endif
	}

	if( bSwap )
	{
		int i = suspendRenderCount;
		if( i )
			suspendRenderCount = i - 1;
	}
	else
	{
		DWORD err;
		int i = suspendRenderCount;

		if( i > 3 )
			ri.Error( ERR_FATAL, "GLimp_EndFrame() - SwapBuffers() failed!\n" );

		err = GetLastError();

		if( Glimp_WindowAndParentsAreVisible( wv->hWnd ) )
		{
			suspendRenderCount = i + 1;		
			ri.Cmd_ExecuteText( EXEC_INSERT, "vid_restart;" );
		}
		else if( !suspendRenderCount )
			suspendRenderCount = 1;
	}

	glimp_suspendRender = suspendRenderCount != 0;
}

#ifdef _DEBUG
void GLimp_LogComment( int level, char *comment, ... ) 
{
	if( level && level > r_logLevel->integer )
		return;

	if( GLEW_GREMEDY_string_marker )
	{
		size_t len;
		char msg[2048];
		va_list vl;

		va_start( vl, comment );
		len = vsnprintf( msg, sizeof( msg ) - 1, comment, vl );
		va_end( vl );

		if( len == -1 )
			len = sizeof( msg ) - 1;

		glStringMarkerGREMEDY( len, msg ); 
	}
}
#endif


/*
===========================================================

SMP acceleration

===========================================================
*/

HANDLE	renderCommandsEvent;
HANDLE	renderCompletedEvent;
HANDLE	renderActiveEvent;

void (*glimpRenderThread)( void );

void GLimp_RenderThreadWrapper( void ) {
	glimpRenderThread();

	// unbind the context before we die
	wglMakeCurrent( glw_state.hDC, NULL );
}

/*
=======================
GLimp_SpawnRenderThread
=======================
*/
HANDLE	renderThreadHandle;
int		renderThreadId;
qboolean GLimp_SpawnRenderThread( void (*function)( void ) ) {

	renderCommandsEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	renderCompletedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	renderActiveEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	glimpRenderThread = function;

	renderThreadHandle = CreateThread(
	   NULL,	// LPSECURITY_ATTRIBUTES lpsa,
	   0,		// DWORD cbStack,
	   (LPTHREAD_START_ROUTINE)GLimp_RenderThreadWrapper,	// LPTHREAD_START_ROUTINE lpStartAddr,
	   0,			// LPVOID lpvThreadParm,
	   0,			//   DWORD fdwCreate,
	   &renderThreadId );

	if ( !renderThreadHandle ) {
		return qfalse;
	}

	return qtrue;
}

static	void	*smpData;
static	int		wglErrors;

void *GLimp_RendererSleep( void ) {
	void	*data;

	if ( !wglMakeCurrent( glw_state.hDC, NULL ) ) {
		wglErrors++;
	}

	ResetEvent( renderActiveEvent );

	// after this, the front end can exit GLimp_FrontEndSleep
	SetEvent( renderCompletedEvent );

	WaitForSingleObject( renderCommandsEvent, INFINITE );

	if ( !wglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) ) {
		wglErrors++;
	}

	ResetEvent( renderCompletedEvent );
	ResetEvent( renderCommandsEvent );

	data = smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	SetEvent( renderActiveEvent );

	return data;
}


void GLimp_FrontEndSleep( void ) {
	WaitForSingleObject( renderCompletedEvent, INFINITE );

	if ( !wglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) ) {
		wglErrors++;
	}
}


void GLimp_WakeRenderer( void *data ) {
	smpData = data;

	if ( !wglMakeCurrent( glw_state.hDC, NULL ) ) {
		wglErrors++;
	}

	// after this, the renderer can continue through GLimp_RendererSleep
	SetEvent( renderCommandsEvent );

	WaitForSingleObject( renderActiveEvent, INFINITE );
}
