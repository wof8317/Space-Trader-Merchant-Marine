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

#include "client.h"

#define CPU_TEXT_SHADOW

typedef struct
{
	float x;    // horiz position
	float y;    // vert position
	float w;    // width
	float h;    // height;

} rectDef_t;


void CL_RenderText_Line( lineInfo_t * line, fontInfo_t * font, float x, float y, float fontScale,
	float letterScaleX, float letterScaleY, int smallcaps, int cursorLoc, int cursorChar, qboolean colors )
{
	re.SetColor( line->startColor );
	re.TextDrawLine( line, font, 0.0f, x, y, fontScale, letterScaleX, letterScaleY, smallcaps, cursorLoc, cursorChar, colors );
}

void CL_RenderText_FindBounds( const lineInfo_t * lines, int count, float * left, float * right )
{
	if ( count > 0 )
	{
		float l = lines[ 0 ].ox;
		float r = lines[ 0 ].ox + lines[ 0 ].width;

		int i;
		for ( i=1; i<count; i++ )
		{
			if ( lines[i].ox < l )
				l = lines[i].ox;

			if ( lines[i].ox + lines[i].width > r )
				r = lines[i].ox + lines[i].width;
		}

		*left	= l;
		*right	= r;

	} else
	{
		*left	= 0.0f;
		*right	= 0.0f;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CL_RenderText_ExtractLines( fontInfo_t * font, float fontScale, float startColor[4], float baseColor[4], const char * text,
	int limit, int align, float width, lineInfo_t * lines, int line_offset, int line_limit )
{
	lineInfo_t dummyLine;

	int lineCount = 0;
	const char* s = text;
	float rgba[4];
	qboolean clipToWidth;

	Vec4_Cpy( rgba, startColor );

	if( align & TEXT_ALIGN_NOCLIP )
	{
		clipToWidth = qfalse;
		align &= ~TEXT_ALIGN_NOCLIP;
	}
	else
		clipToWidth = qtrue;

	for( ; ; )
	{
		int		spaceCount	= 0;
		int		lastSpace	= 0;
		int		widthAtLastSpace = 0;
		int		count		= 0;
		lineInfo_t* line = lines ? lines + lineCount : &dummyLine;

		line->text		= s;
		line->sa		= 0.0f;
		line->ox		= 0.0f;
		line->width		= 0.0f;
		line->height	= 0.0f;

		Vec4_Cpy( line->defaultColor, baseColor );

		Vec4_Cpy( line->startColor, rgba );
		Vec4_Cpy( line->endColor, rgba );

		if( lines && lineCount > 0 )
			Vec4_Cpy( lines[lineCount - 1].endColor, rgba );

		for( ; ; )
		{
			int				c = s[ count ];
			glyphInfo_t*	g = font->glyphs + c;

			// line has ended
			if ( c == '\n' || c == '\0' )
				break;

			if ( Q_IsColorString( s + count) )
			{
				count++;

				if( s[count] == '-' )
					Vec4_Cpy( rgba, baseColor );
				else
					Vec4_Cpy( rgba, g_color_table[ColorIndex( s[count] )] );
		
				count++;

				continue;
			}

			// record white space
			if ( c == ' ' )
			{
				lastSpace = count;
				widthAtLastSpace = line->width;
				spaceCount++;
			}

			// line is too long
			if( clipToWidth && (line->width + (g->xSkip*fontScale) > width) )
			{
				if ( limit == -2 )
					break;

				if ( spaceCount > 0 )
				{
					count		= lastSpace;
					line->width	= widthAtLastSpace;
					spaceCount--;

					if ( align == TEXT_ALIGN_JUSTIFY )
						line->sa = (width-line->width) / (float)spaceCount;
				}

				if ( count == 0 )	// width is less than 1 character?
				{
					line->width = g->xSkip*fontScale;
					count = 1;
				}

				break;
			}

			// record height
			if ( g->height > line->height )
				line->height = g->height;

			// move along
			if ( c == '\t')
			{
			} else
				line->width += ((float)g->xSkip*fontScale);

			count++;

			if ( limit>0 && count >= limit )
				break;
		}

		line->count	= count;

		if ( align == TEXT_ALIGN_RIGHT )
		{
			line->ox = width-line->width;

		} else if ( align == TEXT_ALIGN_CENTER )
		{
			line->ox = (width-line->width)/2;
		}

		if ( line_offset > 0 ) {
			line_offset--;
		} else
			lineCount++;

		if( line_limit > 0 && lineCount >= line_limit )
			break;

		s += count;

		if ( limit == -2 )
			break;

		if ( *s == '\0' )
			break;

		if ( limit>0 )
		{
			if ( count >= limit )
				break;

			limit -= count;
		}

		if ( *s == '\n' )
			s++;

		if ( *s == ' ' )
			s++;
	}

	return lineCount;
}


#define ITEM_TEXTSTYLE_SHADOWED		2		// drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_OUTLINED		4		// drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_BLINK		8		// fast blinking
#define ITEM_TEXTSTYLE_ITALIC		16
#define ITEM_TEXTSTYLE_MULTILINE	32
#define ITEM_TEXTSTYLE_SMALLCAPS	64

//-----------------------------------------------------------------------------
//			R e n d e r T e x t
//-----------------------------------------------------------------------------
#define MAX_LINES 128
void CL_RenderText(	float*			_rect,
					float			scale,
					vec4_t			color,
					const char*		text,
					int				limit,
					int				horiz,
					int				vert,
					int				style,
					int				cursor,
					int				cursorChar,
					qhandle_t		fontSet,
					float*			_textRect
				)
{
	lineInfo_t		lines[ MAX_LINES ];
	int				lineCount;
	int				i, totalChars;
	rectDef_t*		rect		= (rectDef_t*)_rect;
	rectDef_t*		textRect	= (rectDef_t*)_textRect;
	fontInfo_t*		font		= re.GetFontFromFontSet( fontSet, scale );
	float			fontScale	= scale * font->glyphScale;
	float			oy = 0.0f;
	float			h;
	float			lineHeight	= font->glyphs[ 'A' ].height * fontScale;
	float			lineSpace;
	float			italic;
	int				line_offset = 0;
	int				line_limit	= lengthof( lines );

	if( (unsigned int)text < 0x1000 )
		text = "<null>";
	else if( !text[0] && (cursor < 0) )
		return;

	cursor = ((int)(cls.realtime >> 8) & 1) ? -1 : cursor;

	if ( style & ITEM_TEXTSTYLE_MULTILINE ) {
		line_offset	= horiz;
		line_limit	= vert;
		horiz	= TEXT_ALIGN_LEFT;
		vert	= TEXT_ALIGN_LEFT;
	}

	//
	//	extract lines
	//
	lineCount	= CL_RenderText_ExtractLines( font, fontScale, color, color, text, limit, horiz, rect->w, lines, line_offset, line_limit );

	totalChars	= 0;
	//lineHeight	= 0.0f;
	for( i = 0; i < lineCount; i++ )
	{
		totalChars += lines[i].count;
		//lineHeight = max( lineHeight, lines[i].height * fontScale );
	}
	if( cursor >= 0 )
	{
		//if ( totalChars == 0 )
		//	lineHeight = font->glyphs[ 'A' ].height * fontScale;
		totalChars++;
	}

	lineSpace = lineHeight * 0.4f;

	h = lineCount * lineHeight + ((lineCount-1)*lineSpace);

#ifdef CPU_TEXT_SHADOW
	if( style & ITEM_TEXTSTYLE_SHADOWED )
		totalChars *= 2;	//account for shadow
#endif

	//
	//	vertically justify text
	//
	switch ( vert )
	{
		case TEXT_ALIGN_CENTER:		oy = (rect->h - h)*0.5f; break; // middle
		case TEXT_ALIGN_RIGHT:		oy = (rect->h - h); break; // bottom
		case TEXT_ALIGN_JUSTIFY:	lineHeight += (rect->h - h) / (float)lineCount; break;
	}

	//
	//	compute bounds
	//
	if ( textRect || style & ITEM_TEXTSTYLE_OUTLINED )
	{
		float left, right;
		CL_RenderText_FindBounds( lines, lineCount, &left, &right );

		if ( textRect )
		{
			textRect->x = rect->x + left;
			textRect->y = rect->y + oy;
			textRect->w = right - left;
			textRect->h = h;
		}

		if ( style & ITEM_TEXTSTYLE_OUTLINED )
		{
			float b = lineHeight * 0.5f;
			SCR_FillRect( rect->x + left - b*0.5f, rect->y + oy - b*0.5f, right - left + b, h + b, 0, cls.menu );
		}
	}

	if ( style == -1 )
		return;		// style null, just getting text rect

	italic = (style&ITEM_TEXTSTYLE_ITALIC)?4.0f:0.0f;

	re.TextBeginBlock( totalChars );

	//
	//	Render Text Shadow
	//
#ifdef CPU_TEXT_SHADOW
	if ( style & ITEM_TEXTSTYLE_SHADOWED )
	{
		const float bx = font->glyphs[ 'A' ].height * fontScale * (1.0f/6.0f);
		const float by = font->glyphs[ 'A' ].height * fontScale * (1.0f/8.0f);
		vec4_t shadowColor = { 0.0f, 0.0f, 0.0f, 0.5f };
		shadowColor[ 3 ] *= color[ 3 ];
		re.SetColor( shadowColor );

		for ( i=0; i<lineCount; i++ )
		{
			float y = rect->y + oy + (lineHeight * (float)(i+1)) + (lineSpace * (float)i);
			re.TextDrawLine( lines + i, font, italic, rect->x+bx, y+by, fontScale, 1.1f, 1.15f, style&ITEM_TEXTSTYLE_SMALLCAPS, -1, 0, qfalse );
		}
	}
#endif

	//
	//	Render Normal Text
	//

	for( i=0; i<lineCount; i++ )
	{
		float y = rect->y + oy + (lineHeight * (i+1)) + (lineSpace * i);

		re.SetColor( lines[i].startColor );
		re.TextDrawLine( lines + i, font, italic, rect->x, y, fontScale, 1.0f, 1.0f, style&ITEM_TEXTSTYLE_SMALLCAPS, cursor - (lines[i].text-text), cursorChar, qtrue );
	}

	re.TextEndBlock();
}
