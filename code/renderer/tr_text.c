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

typedef struct drawCharCommand_t
{
	shader_t*	shader;
	float		x, y;
	float		w, h;
	float		s1, t1;
	float		s2, t2;
	float		italic;
} drawCharCommand_t;

/*
=============
RB_DrawChar
=============
*/
static void RB_DrawChar( const void *data )
{
	const drawCharCommand_t	*cmd;
	shader_t *shader;
	int numVerts, numIndexes;

	cmd = (const drawCharCommand_t*)data;

	if( !backEnd.projection2D )
		RB_SetGL2D();

	shader = cmd->shader;
	RB_CheckSurface( shader, 0, GL_TRIANGLES );
	
	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[numIndexes] = numVerts + 3;
	tess.indexes[numIndexes + 1] = numVerts + 0;
	tess.indexes[numIndexes + 2] = numVerts + 2;
	tess.indexes[numIndexes + 3] = numVerts + 2;
	tess.indexes[numIndexes + 4] = numVerts + 0;
	tess.indexes[numIndexes + 5] = numVerts + 1;

	*(int*)tess.vertexColors[numVerts] =
		*(int*)tess.vertexColors[numVerts + 1] =
		*(int*)tess.vertexColors[numVerts + 2] =
		*(int*)tess.vertexColors[numVerts + 3] = *(int*)backEnd.color2D;

	tess.xyz[numVerts][0] = cmd->x + cmd->italic;
	tess.xyz[numVerts][1] = cmd->y;
	tess.xyz[numVerts][2] = 0;

	tess.texCoords[numVerts][0][0] = cmd->s1;
	tess.texCoords[numVerts][0][1] = cmd->t1;

	tess.xyz[numVerts + 1][0] = cmd->x + cmd->w + cmd->italic;
	tess.xyz[numVerts + 1][1] = cmd->y;
	tess.xyz[numVerts + 1][2] = 0;

	tess.texCoords[numVerts + 1][0][0] = cmd->s2;
	tess.texCoords[numVerts + 1][0][1] = cmd->t1;

	tess.xyz[numVerts + 2][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 2][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 2][2] = 0;

	tess.texCoords[numVerts + 2][0][0] = cmd->s2;
	tess.texCoords[numVerts + 2][0][1] = cmd->t2;

	tess.xyz[numVerts + 3][0] = cmd->x;
	tess.xyz[numVerts + 3][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 3][2] = 0;

	tess.texCoords[numVerts + 3][0][0] = cmd->s1;
	tess.texCoords[numVerts + 3][0][1] = cmd->t2;
}

void Q_EXTERNAL_CALL RE_TextBeginBlock( unsigned int numChars )
{
}

void Q_EXTERNAL_CALL RE_TextEndBlock( void )
{
}

static float R_TextAppendChar( glyphInfo_t * glyph, float x, float y, float fontScale,
	float letterScaleX, float letterScaleY, float italic )
{
	float letterX, letterY, skip, top;
	float w, h;

	drawCharCommand_t	*cmd;

	if( !tr.registered )
		return 0.0F;

	skip = (float)glyph->xSkip * fontScale;

	cmd = R_GetCommandBuffer( RB_DrawChar, sizeof( *cmd ) );
	if( !cmd )
		return skip;

	top		= (float)glyph->top * fontScale;
	letterX = ((skip * letterScaleX) - skip) * -0.5f;
	letterY = -top + (((top * letterScaleY) - top) * -0.5f);

	x = (x + letterX);
	y = (y + letterY);
	w = glyph->imageWidth * letterScaleX * fontScale;
	h = glyph->imageHeight * letterScaleY * fontScale;

	x = x * glConfig.xscale + glConfig.xbias;
	y *= glConfig.yscale;
	w *= glConfig.xscale;
	h *= glConfig.yscale;

	cmd->shader = R_GetShaderByHandle( glyph->glyph );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = glyph->s;
	cmd->t1 = glyph->t;
	cmd->s2 = glyph->s2;
	cmd->t2 = glyph->t2;
	cmd->italic = italic * (float)glyph->height * glConfig.xscale;

	return skip;
}

void Q_EXTERNAL_CALL RE_TextDrawLine( const lineInfo_t * line, fontInfo_t * font, float italic, float x, float y,
	float fontScale, float letterScaleX, float letterScaleY, int smallcaps, int cursorLoc, int cursorChar, qboolean colors )
{
	glyphInfo_t *glyph;
	int i;
	
	if( line->count <= 0 && cursorLoc < 0 )
		return;

	x += line->ox;

	italic = (italic * fontScale) / (float)font->glyphs[ 'A' ].height;

	if( colors )
		RE_SetColorRGB( line->startColor );
	
	for( i = 0; i < line->count; i++ )
	{
		if( Q_IsColorString( line->text + i ) )
		{
			if( colors )
			{
				if( line->text[i + 1] == '-' )
					RE_SetColorRGB( line->defaultColor );
				else if( line->text[i + 1] == 'b' ) {
					float c[4];
					letterScaleX *= 1.1f;
					letterScaleY *= 1.1f;
					c[ 0 ] = min( 1.0f, line->defaultColor[ 0 ] * 1.1f );
					c[ 1 ] = min( 1.0f, line->defaultColor[ 1 ] * 1.1f );
					c[ 2 ] = min( 1.0f, line->defaultColor[ 2 ] * 1.1f );
					c[ 3 ] = min( 1.0f, line->defaultColor[ 3 ] * 1.1f );
					RE_SetColor( c );
				} else
					RE_SetColorRGB( (const float*)(g_color_table + ColorIndex( line->text[i + 1] )) );
			}

			i++;
			continue;
		}
		else
		{
			float step = 0.0F;

			if( line->text[i] == ' ' )
				step = (font->glyphs[' '].xSkip * fontScale) + line->sa;
			else
			{
				if ( smallcaps && line->text[i] >= 'a' && line->text[i] <= 'z' ) {
					glyph	= font->glyphs + ((line->text[i]-'a')+'A');
					step	= R_TextAppendChar( glyph, x, y+(glyph->top*fontScale*letterScaleY*0.25f*0.5f), fontScale, letterScaleX*0.75f, letterScaleY*0.75f, italic ) * 0.75;
				} else {
					glyph	= font->glyphs + line->text[i];
					step	= R_TextAppendChar( glyph, x, y, fontScale, letterScaleX, letterScaleY, italic );
				}
			}

			if( i == cursorLoc )
			{
				glyph = font->glyphs + cursorChar;
				R_TextAppendChar( glyph, x, y, fontScale, letterScaleX, letterScaleY, italic );
			}

			x += step;
		}
	}

	if( i == cursorLoc )
	{
		glyph = font->glyphs + cursorChar;
		R_TextAppendChar( glyph, x, y, fontScale, letterScaleX, letterScaleY, italic );
	}
}