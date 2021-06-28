/*
===========================================================================
Copyright (C) 2006 HermitWorks Entertainment Corporation

This file is part of Space Trader source code.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
===========================================================================
*/

#include "ui_local.h"

int a;

#if 1

typedef struct
{
	int			used;
	int			key;

	int			start,end;
	float		low,high;

	struct
	{
		qhandle_t	shape;
		qhandle_t	shader;
	} graphs[ 16 ];
	int			graphCount;

	struct
	{
		char		text[ MAX_SAY_TEXT ];
		float		x,y;

		qhandle_t	fx;
	} labels[ 16 ];

	int			labelCount;

} chart_t;

qhandle_t UI_Chart_Create( int key, int start, int end, float low, float high )
{
	chart_t * chart = UI_Alloc( sizeof(chart_t) );

	chart->start = start;
	chart->end = end;
	chart->low = low;
	chart->high = high;

	chart->used = 1;
	chart->key = key;

	return (qhandle_t)chart;
}

void UI_Graph_Init( graphGen_t * gg )
{
	gg->n			= 0;
	gg->low			= +1E6;
	gg->high		= -1E6;
}

void UI_Graph_Line( graphGen_t * gg, int x1, int y1, int x2, int y2, vec4_t color )
{
	gg->pts[ gg->n ][ 0 ] = (float)x1;
	gg->pts[ gg->n ][ 1 ] = (float)y1;
	gg->uvs[ gg->n ][ 0 ] =
	gg->uvs[ gg->n ][ 1 ] = 0.0f;
	memcpy( gg->colors[ gg->n ], color, sizeof(vec4_t) );
	gg->n++;

	gg->pts[ gg->n ][ 0 ] = (float)x2;
	gg->pts[ gg->n ][ 1 ] = (float)y2;
	gg->uvs[ gg->n ][ 0 ] =
	gg->uvs[ gg->n ][ 1 ] = 1.0f;
	memcpy( gg->colors[ gg->n ], color, sizeof(vec4_t) );
	gg->n++;

}

void UI_Graph_Plot( graphGen_t * gg, float p, vec4_t colorLow, vec4_t colorHigh )
{
	float	X		= -1E5f;
	int		i,n		= gg->n;

	while ( trap_SQL_Step() )
	{
		int		y	= trap_SQL_ColumnAsInt( 0 );
		int		x	= trap_SQL_ColumnAsInt( 1 );
		float	px	= x*p;

		gg->low		= min( gg->low, y );
		gg->high	= max( gg->high, y );

		if ( (px-X) < 3.0f )
			continue;	// skip points until they're at least 3 pixels apart

		X = px;

		if ( n == MAX_GRAPH_POINTS )
			break;

		gg->pts[n][0] = (float)x;
		gg->pts[n][1] = (float)y;
		n++;
	}
	trap_SQL_Done();

	for ( i=gg->n; i<n; i++ )
	{
		gg->uvs		[ i ][0] = (float)i / (float)(n-gg->n);
		gg->uvs		[ i ][1] = 0.0f;

		Vec4_Lrp( gg->colors[ i ], colorLow, colorHigh, (gg->pts[ i ][1]-gg->low) / (float)(gg->high-gg->low) );
	}

	gg->n = n;
}

void UI_Chart_AddGraph( qhandle_t h, graphGen_t * gg, shapeGen_t gen, qhandle_t shader )
{
	chart_t*	chart = (chart_t*)h;
	curve_t		cv;
	int i;
	float cx,cy;

	if ( !h )
		return;

	cx = 1.0f / ( chart->end - chart->start );
	cy = 1.0f / ( chart->high - chart->low );

	for ( i=0; i<gg->n; i++ )
	{
		gg->pts		[ i ][0] = (gg->pts[ i ][0] - chart->start) * cx;
		gg->pts		[ i ][1] = 1.0f - ((gg->pts[ i ][1] - chart->low) * cy);
	}

	Com_Memset( &cv, 0, sizeof( cv ) );

	cv.pts		= gg->pts;
	cv.colors	= gg->colors;
	cv.uvs		= gg->uvs;
	cv.numPts	= gg->n;

	chart->graphs[ chart->graphCount ].shape = trap_R_ShapeCreate( &cv, &gen, 1 );
	chart->graphs[ chart->graphCount ].shader = shader;
	chart->graphCount++;
}

void UI_Chart_AddLabel( qhandle_t h, int x, float y, const char * text )
{
	chart_t * chart = (chart_t*)h;

	if ( !h )
		return;

	chart->labels[ chart->labelCount ].x = (float)(x - chart->start) / ( chart->end - chart->start );
	chart->labels[ chart->labelCount ].y = 1.0f - ((y - chart->low) / ( chart->high - chart->low ));
	Q_strncpyz( chart->labels[ chart->labelCount ].text, text, MAX_SAY_TEXT );

	chart->labels[ chart->labelCount ].fx = 0;

	chart->labelCount++;
}

void UI_Chart_Paint( itemDef_t * item, qhandle_t chart )
{
	int i;
	chart_t * c = (chart_t*)chart;
	rectDef_t s = item->window.rect;

	if ( !c )
		return;

	for ( i=0; i<c->labelCount; i++ )
	{
		//if( !UI_Effect_IsAlive( c->labels[i].fx ) )
		{
			rectDef_t rr;
			rr.x = item->window.rect.x + c->labels[ i ].x * item->window.rect.w;
			rr.y = (item->window.rect.y + c->labels[ i ].y * item->window.rect.h) - 8.0f;
			rr.w = 64.0f;
			rr.h = 16.0f;

			trap_R_RenderText( &rr, item->textscale, item->window.foreColor, c->labels[i].text, -1, item->textalignx, item->textaligny, item->textStyle, item->font, -1, 0 );


			//c->labels[i].fx = UI_Effect_SpawnText( &rr, UI_Effect_Find( "graph_label" ), c->labels[i].text, 0 );
			//UI_Effect_SetFlag( c->labels[i].fx, EF_ONEFRAME_NOFOCUS );
		}
		
		//UI_Effect_ClearFlag( c->labels[i].fx, EF_NOFOCUS );
	}

	s.x += 36.0f;
	s.w -= 38.0f;
	
	trap_R_SetColor( colorWhite );

	for ( i=0; i<c->graphCount; i++ )
		DrawShape( &s, c->graphs[ i ].shape, uiInfo.uiDC.Assets.lineshadow );

	s.x -= 3.0f;
	s.y -= 3.0f;

	for ( i=0; i<c->graphCount; i++ )
		DrawShape( &s, c->graphs[ i ].shape, c->graphs[ i ].shader );
}

#endif
