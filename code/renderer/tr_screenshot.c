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

#include "tr_local.h"

#define USE_PNG

#ifdef USE_PNG
#include "platform/pop_segs.h"
#include <png.h>
#include "platform/push_segs.h"
#endif

/* 
============================================================================== 
 
						SCREEN SHOTS 

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

============================================================================== 
*/

static int QDECL tr_upload_bug( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	fileHandle_t	f		= (fileHandle_t)notifyData;

	switch( code )
	{
	case HTTP_READ:
		return ri.FS_Read( (void*)buffer, length, f );
	case HTTP_FAILED:
	case HTTP_DONE:
		ri.FS_FCloseFile( f );
	}

	return 1;
}

#ifdef USE_PNG
static void Q_EXTERNAL_CALL_VA R_write_png_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	fileHandle_t f = (fileHandle_t)png_get_io_ptr(png_ptr);
	ri.FS_Write(data, length, f);
}
static void Q_EXTERNAL_CALL_VA R_flush_png_data(png_structp png_ptr)
{
}

static void R_WriteScreenshot_PNG( const char *fileName, const char *comment, const char *condump, uint width, uint height, byte* rgb )
{
	int i;
	png_structp png_ptr;
	png_infop info_ptr;
	png_text text[2];
	fileHandle_t f;
	
	png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr,
			(png_infopp)NULL);
		return;
	}
	
	ri.FS_FOpenFile( fileName, &f, FS_WRITE );
	if ( !f ) {
		ri.Printf ( PRINT_ERROR, "Couldn't write %s.\n", fileName );
		return;
	}

	png_set_write_fn(png_ptr, (void *)f, R_write_png_data, R_flush_png_data);
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return;
	}
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	
	text[0].compression = PNG_TEXT_COMPRESSION_NONE;
	text[0].key="Comment";
	text[0].text = (char *)comment;
	//compress the text for the condump
	text[1].compression = PNG_TEXT_COMPRESSION_zTXt;
	//text[1].compression = PNG_TEXT_COMPRESSION_NONE;
	text[1].key="Description";
	text[1].text = (char *)condump;
	png_set_text( png_ptr, info_ptr, text, lengthof(text) );

	png_write_info(png_ptr, info_ptr);
	
	png_set_packing(png_ptr);
	
	for (i=height-1; i >= 0; --i)
	{
		png_write_row( png_ptr, &rgb[i * width * 3] );
	}
	png_write_end(png_ptr, NULL);

	ri.FS_FCloseFile( f );

	if ( com_developer->integer ) {
		HTTP_PostBug(fileName);
	}
}


/* 
================== 
RB_TakeScreenshot
================== 
*/  
void RB_TakeScreenshot( int x, int y, int width, int height, char *fileName ) {
	byte * buffer = ri.Hunk_AllocateTempMemory(glConfig.vidWidth*glConfig.vidHeight*3);
	cvar_t * map = ri.Cvar_Get( "mapname", "", 0 );
	cvar_t * bugreport = ri.Cvar_Get( "r_bugreport", "", 0 );
	cvar_t * challenge = ri.Cvar_Get( "g_missiontitle", "", 0 );
	const char *condump;
	char comment[ 512 ];
	glFlush();
	glReadBuffer( GL_FRONT );
	glReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer );

	Q_strncpyz( comment, "[ ", sizeof(comment) );
	if ( challenge->string[ 0 ] )
		Q_strcat( comment, sizeof(comment), va("< %s > : ", challenge->string ) );
	
	Q_strcat( comment, sizeof(comment), (map->string[0])?map->string:"frontend" );
	Q_strcat( comment, sizeof(comment), va( " ] %s", bugreport->string ) );
	
	condump = ri.Con_GetText(0);

	R_WriteScreenshot_PNG(	fileName, comment, condump, width, height, buffer );
	ri.Hunk_FreeTempMemory( buffer );
}

#endif


/* 
================== 
RB_TakeScreenshot
================== 
*/  
void RB_TakeScreenshot_TGA( int x, int y, int width, int height, char *fileName ) {
	byte		*buffer;
	int			i, c, temp;
		
	buffer = ri.Hunk_AllocateTempMemory(glConfig.vidWidth*glConfig.vidHeight*3+18);

	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	glReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 ); 

	// swap rgb to bgr
	c = 18 + width * height * 3;
	for (i=18 ; i<c ; i+=3) {
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	// gamma correct
	if ( ( tr.overbrightBits > 0 ) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, glConfig.vidWidth * glConfig.vidHeight * 3 );
	}

	ri.FS_WriteFile( fileName, buffer, c );

	ri.Hunk_FreeTempMemory( buffer );
}



/*
==================
RB_TakeScreenshotCmd
==================
*/
void RB_TakeScreenshotCmd( const void *data )
{
	const screenshotCommand_t	*cmd;
	
	cmd = (const screenshotCommand_t *)data;
	RB_TakeScreenshot( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot( int x, int y, int width, int height, char *name ) {
	static char	fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
	screenshotCommand_t	*cmd;

	cmd = R_GetCommandBuffer( RB_TakeScreenshotCmd, sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	
	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	Q_strncpyz( fileName, name, sizeof(fileName) );
	cmd->fileName = fileName;
}

/* 
================== 
R_ScreenshotFilename
================== 
*/  
void R_ScreenshotFilename( int lastNumber, char *fileName ) {
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.tga" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga"
		, a, b, c, d );
}

/* 
================== 
R_ScreenshotFilename
================== 
*/  
void R_ScreenshotFilenameJPEG( int lastNumber, char *fileName ) {
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.jpg" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg"
		, a, b, c, d );
}

/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
void R_LevelShot( void ) {
	const char	*checkname;
	byte		*buffer;
	byte		*source;
	byte		*src, *dst;
	int			x, y;
	int			r, g, b;
	float		xScale, yScale;
	int			xx, yy;

	
	checkname = va( "levelshots/%s.tga", tr.world->baseName );

	source = ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight * 3 );

	buffer = ri.Hunk_AllocateTempMemory( 128 * 128*3 + 18);
	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = 128;
	buffer[14] = 128;
	buffer[16] = 24;	// pixel size

	glReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, source ); 

	// resample from source
	xScale = glConfig.vidWidth / 512.0f;
	yScale = glConfig.vidHeight / 384.0f;
	for ( y = 0 ; y < 128 ; y++ ) {
		for ( x = 0 ; x < 128 ; x++ ) {
			r = g = b = 0;
			for ( yy = 0 ; yy < 3 ; yy++ ) {
				for ( xx = 0 ; xx < 4 ; xx++ ) {
					src = source + 3 * ( glConfig.vidWidth * (int)( (y*3+yy)*yScale ) + (int)( (x*4+xx)*xScale ) );
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst = buffer + 18 + 3 * ( y * 128 + x );
			dst[0] = b / 12;
			dst[1] = g / 12;
			dst[2] = r / 12;
		}
	}

	// gamma correct
	if ( ( tr.overbrightBits > 0 ) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, 128 * 128 * 3 );
	}

	ri.FS_WriteFile( checkname, buffer, 128 * 128*3 + 18 );

	ri.Hunk_FreeTempMemory( buffer );
	ri.Hunk_FreeTempMemory( source );

	ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
}

/* 
================== 
R_ScreenShot_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
================== 
*/  
void Q_EXTERNAL_CALL R_ScreenShot_f( void )
{
	static int lastNumber = 0;

	char checkname[MAX_OSPATH];
	char *path = "screenshots/shot%i%i%i%i.png";
	qboolean silent = qfalse;
	qboolean findname = qtrue;
	qboolean cubemap = qfalse;

	if( ri.Cmd_Argc() >= 1 )
	{
		char * arg = ri.Cmd_Argv( 1 );
		int	c = 2;

		if( !strcmp( arg, "levelshot" ) )
		{
			R_LevelShot();
			return;
		}

		if( strcmp( arg, "bug" ) == 0 )
			path = "bugs/bug%i%i%i%i.png";
		else if ( !strcmp( arg, "silent" ) )
			silent = qtrue;
		else if ( !Q_stricmp( arg, "cube" ) ) {
			cubemap = qtrue;
			path = va("cubemaps/shot%%i%%i%%i%%i_%s.png", ri.Cmd_Argv(2));
			c = 3;
		} else
			c = 1;

		if( ri.Cmd_Argc() > c )
		{
			Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.png", ri.Cmd_Argv( c ) );
			findname = qfalse;
		}
	}

	if( findname )
	{
		//
		//	find a free file name
		//
		for ( ; lastNumber <= 9999 ; lastNumber++ )
		{
			Com_sprintf( checkname, MAX_OSPATH, path,
					lastNumber / 1000,
					lastNumber % 1000 / 100,
					lastNumber % 100 / 10,
					lastNumber % 10 );

			if( !ri.FS_FileExists( checkname ) )
				break;
		}

		if( lastNumber == 10000 )
		{
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n"); 
			return;
		}

		if ( !cubemap )
			lastNumber++;
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );

	if( !silent )
		ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
} 

void R_ScreenShotJPEG_f (void) {
	char		checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.jpg", ri.Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilenameJPEG( lastNumber, checkname );

      if (!ri.FS_FileExists( checkname ))
      {
        break; // file doesn't exist
      }
		}

		if ( lastNumber == 10000 ) {
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n"); 
			return;
 		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );

	if ( !silent ) {
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
}
