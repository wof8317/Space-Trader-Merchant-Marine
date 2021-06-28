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

#define MAX_FONTS			8
#define MAX_FONTS_PER_SET	6

typedef struct
{
	char			name	[ MAX_QPATH ];
	int				fontCount;
	fontInfo_t		fonts	[ MAX_FONTS_PER_SET ];

} fontSetInfo_t;

static int				registeredFontCount = 0;
static fontSetInfo_t	registeredFont[ MAX_FONTS ];

fontInfo_t * Q_EXTERNAL_CALL RE_GetFontFromFontSet( qhandle_t fontSet, float fontScale )
{
	fontSetInfo_t * set = registeredFont + fontSet - 1;
	int pointSize = (int)(fontScale * 72.0f * glConfig.xscale );
	int i;

	for ( i=0; i<set->fontCount-1; i++ )
	{
		if ( set->fonts[ i ].pointSize > pointSize )
			break;
	}

	return set->fonts + i;
}

void Q_EXTERNAL_CALL RE_GetFonts( char * buffer, int size )
{
	int i;
	buffer[ 0 ] = '\0';
	for ( i=0; i<registeredFontCount; i++ )
	{
		Q_strcat( buffer, size, va( "%d \"%s\" ", i+1, registeredFont[ i ].name ) );
	}
}

static void readfont( const char * fileName, const char * fontName, fontInfo_t * font )
{
	int * buffer;
	int i;

	ri.FS_ReadFile( fileName, (void**)&buffer );

	for ( i=0; i<GLYPHS_PER_FONT; i++ )
	{
		int j = i*20;
		font->glyphs[i].height		= LittleLong(buffer[ j + 0 ]);
		font->glyphs[i].top			= LittleLong(buffer[ j + 1 ]);
		font->glyphs[i].bottom		= LittleLong(buffer[ j + 2 ]);
		font->glyphs[i].pitch		= LittleLong(buffer[ j + 3 ]);
		font->glyphs[i].xSkip		= LittleLong(buffer[ j + 4 ]);
		font->glyphs[i].imageWidth	= LittleLong(buffer[ j + 5 ]);
		font->glyphs[i].imageHeight = LittleLong(buffer[ j + 6 ]);
		font->glyphs[i].s			= LittleFloat(((float*)buffer)[ j + 7 ]);
		font->glyphs[i].t			= LittleFloat(((float*)buffer)[ j + 8 ]);
		font->glyphs[i].s2			= LittleFloat(((float*)buffer)[ j + 9 ]);
		font->glyphs[i].t2			= LittleFloat(((float*)buffer)[ j + 10 ]);
		font->glyphs[i].glyph		= buffer[ j + 11 ];

		{
			char tmp[ MAX_OSPATH ];
			Com_sprintf( tmp, sizeof(tmp), "fonts/%s%s", fontName, ((char*)(buffer + j + 12)) + 15 );
			font->glyphs[i].glyph = RE_RegisterShaderNoMip( tmp );
		}
	}

	font->glyphScale = LittleFloat(((float*)buffer)[ i*20 ]);

	ri.FS_FreeFile( buffer );
}

static int QDECL sortByPointSize( const void *a, const void *b )
{
	return ((fontInfo_t*)a)->pointSize - ((fontInfo_t*)b)->pointSize;
}

qhandle_t Q_EXTERNAL_CALL RE_RegisterFont( const char *fontName )
{
	int i,n,l;
	char **files;
	fontSetInfo_t * set;

	if (!fontName) {
		ri.Printf(PRINT_ALL, "RE_RegisterFont: called with empty name\n");
		return 0;
	}

	for ( i=0; i<registeredFontCount; i++ ) {
		if ( Q_stricmp( fontName, registeredFont[ i ].name ) == 0 )
			return (i+1);	// font is already registered
	}

	if (i >= MAX_FONTS) {
		ri.Printf(PRINT_ALL, "RE_RegisterFont: Too many fonts registered already.\n");
		return 0;
	}

	set = registeredFont + i;

	strncpy( set->name, fontName, sizeof( set->name ) ); 
	set->fontCount = 0;
		
	l		= strlen( fontName );
	files	= ri.FS_ListFiles( "fonts/", ".dat", &n );
	for ( i=0; i<n; i++ )
	{
		char * name = files[i] + 6;
		if ( Q_stricmpn( fontName, name, l )==0 )
		{
			readfont( files[i], fontName, &set->fonts[ set->fontCount ] );

			set->fonts[ set->fontCount ].pointSize	= atoi( name + l + 1 );
			set->fontCount++;
		}
	}
	qsort( set->fonts, set->fontCount, sizeof( set->fonts[0] ), sortByPointSize );
	ri.FS_FreeFileList(files);
	registeredFontCount++;
	return registeredFontCount;
}

void R_InitFreeType(void) {
  registeredFontCount = 0;
}

void R_DoneFreeType(void) {
	registeredFontCount = 0;
}