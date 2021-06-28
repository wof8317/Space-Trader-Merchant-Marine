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


/*
	These are the curve types.

	CT_FREE indicates an empty curve slot and always evaluates to <0, done>.

	A curve of a given type is created by calling curve_create_<type's-base-name>.
*/
typedef enum curveType_t
{
	CT_NOT_FREE = -1,
	CT_FREE = 0,
	CT_FUNC,
	CT_CONSTANT,
	CT_LERP,
} curveType_t;

/*
	A func_t is a function that takes a [0, 1] value
	that represents the function's full "natural" period
	and returns it's "natural" value at that point.

	For example, a func_t implementing sin would return
	sin( t * M_PI * 2 ).
*/
typedef float (*func_t)( float t );

/*
	The props for the sprite and the effect. Note that the first N props must
	both sprites and effects have in common must be in the same order.
*/
typedef enum commonProp_t
{
	CP_POSITION_X,
	CP_POSITION_Y,
	
	CP_SCALE_X,
	CP_SCALE_Y,

	CP_ROTATION,
} commonProp_t;

typedef enum spriteProp_t
{
	SP_POSITION_X,
	SP_POSITION_Y,
	
	SP_SCALE_X,
	SP_SCALE_Y,

	SP_ROTATION,

	SP_COLOR_R,
	SP_COLOR_G,
	SP_COLOR_B,
	SP_COLOR_A,

	SP_NUM_PROPS,
} spriteProp_t;

typedef enum {
	EP_POSITION_X,
	EP_POSITION_Y,

	EP_SCALE_X,
	EP_SCALE_Y,

	EP_ROTATION,

	EP_NUM_PROPS,

} effectProp_t;


#define CURVE_DONE			0x00000001
#define CURVE_DELETEME		0x00000002

typedef struct
{
	curveType_t			type;

	/*
		This union holds all of the parameters to run
		the curve. Data members shall be named the same
		as the curveType_t element they correspond to,
		all lowercase and without the CT_ prefix.

		Data should be strucutred such that zero-initialization
		makes sense.
	*/
	union
	{
		struct
		{
			func_t		fcb;
			float		base_val;
			float		amplitude;

			float		phase;
			float		frequency;
		}				func;

		struct
		{
			float		val;
			float		duration;
		}				constant;

		struct
		{
			float		start;
			float		end;
			float		duration;
			qboolean	hold;
		}				lerp;

	}					data;

	/*
		Evaluation time = (toSecs(current_time - timeBase) * timeScale) % timeMod.

		The toSecs conversion is premultiplied into the timeScale
		factor upon curve creation.

		Note that timeMod is only applied if timeMod > 0.

		The timeBase subtraction is done in unsigned integer milliseconds in order
		to avoid cancellation errors on large time values. Everything beyond that
		is done in floats as we should (ideally) be in the 0-1 neighborhood.
	*/
	uint				timeBase;
	float				timeScale;
	float				timeMod;

	float				value;
	int					flags;

	/*
		This is a counter to give us a way to track lifetimes.
		It gets incremented upon acurve expiring.

		Zero is reserved as invalid - if seqNum is zero, it is
		incremented upon the curve's creation as well.
	*/
	short				seqNum;

} animCurve_t;

#define SPRITE_DELETEME		0x000000001

typedef struct sprite_t
{
	float				baseVals[SP_NUM_PROPS]; //a set of implicit non-expiring constant curves that are always in effect
	qhandle_t			bindings[SP_NUM_PROPS][8];

	float				propVals[SP_NUM_PROPS]; //the final computed values
	float				w,h;
	vec4_t				uv_coord;
	qhandle_t			shader;


	int					flags;
	short				seqNum;


} sprite_t;


typedef struct
{
	const char *	pc;
	int		i;
} stackFrame_t;

typedef struct effect_t
{
	effectDef_t *		def;
	rectDef_t			rect;
	char				text[ EFFECT_TEXT_LENGTH ];
	qhandle_t			shader;

	qhandle_t			props[EP_NUM_PROPS][8];
	float				baseVals[EP_NUM_PROPS];
	float				propVals[EP_NUM_PROPS];

	int					thread;
	qhandle_t			curves[EFFECT_TEXT_LENGTH * 4];
	int					curveCount;
	qhandle_t			sprites[EFFECT_TEXT_LENGTH * 4];
	int					spriteCount;

	int					flags;			// status flags from outside

	int					touch;			// counter that decrements (stopping at zero) on each draw

	int					waitfor;		// bitfield of stuff to wait for
	int					sleeping;

	itemDef_t *			item;

	// a small computer
	stackFrame_t		stack[ 8 ];
	int					sp;

	short				seqNum;
} effect_t;

static struct
{
	sprite_t		sprites	[ 384 ];
	sprite_t *		freelist[ 384 ];
	int				count;
} sprites;

static struct
{
	animCurve_t		curves	[ 512 ];
	animCurve_t *	freelist[ 512 ];
	int				count;
} curves;

static struct
{
	effect_t		effects	[ 64 ];
	effect_t *		freelist[ 64 ];
	int				count;
} effects;

/*
	Animation curves
**************************************************************************/

static ID_INLINE animCurve_t * curve_get( qhandle_t h ) {
	if ( h )
	{
		int	index	= (h&0xFFFF)-1;
		int seqNum	= (h>>16)&0xFFFF;
		animCurve_t * c = curves.curves + index;

		if ( c->seqNum == seqNum )
			return c;
	}

	return 0;
}

static ID_INLINE qhandle_t curve_getId( animCurve_t * c ) {
	return ((c->seqNum&0xFFFF)<<16) | ((c-curves.curves+1)&0xFFFF);
}

static ID_INLINE animCurve_t * curve_alloc( void )
{
	short seqNum;
	animCurve_t *cv;

	if( curves.count >= lengthof( curves.freelist ) )
		return 0;

	cv = curves.freelist[curves.count++];

	seqNum = cv->seqNum;
	Com_Memset( cv, 0, sizeof( *cv ) );
	cv->seqNum = seqNum;

	return cv;
}

/*
	The following functions initialize new curves.
*/

static ID_INLINE animCurve_t * curve_create_base( uint timeBase, float timeScale, float timeMod, curveType_t type )
{
	animCurve_t * c = curve_alloc();

	if( !c )
		return 0;
	
	c->type			= type;
	c->timeBase		= timeBase;
	c->timeScale	= timeScale;
	c->timeMod		= timeMod;
	c->flags		= 0;

	return c;
}

/*
	This is pretty much the cannonical curve create function.

	It calls curve_create_base to set up the common curve parameters,
	checks for an allocation failure, and then sets up the curve-type
	specific parameters. Finally it calls curve_eval to set up the initial
	curve value. One of these should exist for each curve type.
*/
qhandle_t curve_create_func( int timeBase, float timeScale, float timeMod,
	func_t func, float freq, float phase, float amp, float base )
{
	animCurve_t *c;
	
	if( !func )
		return 0;

	c = curve_create_base( timeBase, timeScale, timeMod, CT_FUNC );
	
	if( !c )
		return 0;

	c->data.func.fcb = func;
	c->data.func.frequency = freq;
	c->data.func.phase = phase;
	c->data.func.amplitude = amp;
	c->data.func.base_val = base;

	return curve_getId( c );
}

qhandle_t curve_create_constant( int timeBase, float val, float duration )
{
	animCurve_t *c;

	c = curve_create_base( timeBase, 1, 0, CT_CONSTANT );
	
	if( !c )
		return 0;

	c->data.constant.val = val;
	c->data.constant.duration = duration;

	return curve_getId( c );
}

static qhandle_t curve_create_lerp( int timeBase, float start, float end, float duration, qboolean hold )
{
	animCurve_t *c;

	c = curve_create_base( timeBase, 1.0F, 0.0F, CT_LERP );
	
	if( !c )
		return 0;

	c->data.lerp.start = start;
	c->data.lerp.end = end;
	c->data.lerp.duration = duration;
	c->data.lerp.hold = hold;

	return curve_getId( c );
}

/*
	Curve destruction.
*/
void curve_destroy( qhandle_t h )
{
	animCurve_t * c = curve_get( h );
	if ( c )
		c->flags |= CURVE_DELETEME;
}

/*
	Curve evaluation. Fun stuff.
*/

float fracf( float f ) { return f - floorf( f ); }

static void curve_eval( animCurve_t *cv, uint eval_time )
{
	float t;

	if( !cv )
		return;

	if( eval_time >= cv->timeBase )
		eval_time -= cv->timeBase;
	else
		eval_time = 0;

	t  = (float)eval_time * cv->timeScale;

	if( cv->timeMod > 0 )
		t = fmodf( t, cv->timeMod );

	switch( cv->type )
	{
	case CT_FUNC:
		if( t > 1 )
		{
			t = 1;
			cv->flags |= CURVE_DONE;
		}
		else
			cv->flags &= ~CURVE_DONE;

		t = fracf( t * cv->data.func.frequency + cv->data.func.phase );

		cv->value = cv->data.func.fcb( t ) * cv->data.func.amplitude + cv->data.func.base_val;
		break;

	case CT_CONSTANT:
		cv->value	= cv->data.constant.val;
		break;

	case CT_LERP:

		t = t / cv->data.lerp.duration;

		if( t > 1.0f )
		{
			t = 1.0f;
			cv->flags |= CURVE_DONE;
		}
		else
			cv->flags &= ~CURVE_DONE;

		cv->value = cv->data.lerp.start * (1.0F - t) + cv->data.lerp.end * t;
		break;

	default:
		cv->flags |= CURVE_DONE;
		cv->value = 0;
		break;
	}
}

void curves_update()
{
	int i;
	for ( i=0; i<curves.count; i++ )
	{
		animCurve_t * c = curves.freelist[ i ];

		if ( c->flags & CURVE_DELETEME )
		{
			curves.freelist[ i-- ] = curves.freelist[ --curves.count ];
			curves.freelist[ curves.count ] = c;
			c->seqNum++;
		}
	}
}

float curve_get_value( qhandle_t h )
{
	animCurve_t * c = curve_get( h );
	if ( c )
		return c->value;

	return 0.0f;
}

qboolean curve_is_done( qhandle_t h )
{
	animCurve_t * c = curve_get( h );
	return (c)?c->flags&CURVE_DONE:1;
}

/*
	Sprites
**************************************************************************/

static ID_INLINE sprite_t * sprite_get( qhandle_t h ) {
	if ( h )
	{
		int	index	= (h&0xFFFF)-1;
		int seqNum	= (h>>16)&0xFFFF;
		sprite_t * s = sprites.sprites + index;

		if ( s->seqNum == seqNum )
			return s;
	}

	return 0;
}

static ID_INLINE qhandle_t sprite_getId( sprite_t * s ) {
	return ((s->seqNum&0xFFFF)<<16) | ((s-sprites.sprites+1)&0xFFFF);
}

static ID_INLINE sprite_t * sprite_alloc( void ) {
	if ( sprites.count >= lengthof( sprites.freelist ) )
		return 0;

	return sprites.freelist[ sprites.count++ ];
}

static void sprite_eval( sprite_t *spr )
{
	int i;

	for ( i=0; i<lengthof( spr->propVals ); i++ )
	{
		int j;
		spr->propVals[i] = spr->baseVals[i];

		for ( j=0; j<lengthof( spr->bindings[i] ); j++ )
		{
			animCurve_t *cv = curve_get( spr->bindings[i][j] );
			if( !cv )
				continue;

			switch ( i )
			{
			case SP_POSITION_X:
			case SP_POSITION_Y:
			case SP_ROTATION:
				spr->propVals[i] += cv->value;
				break;

			case SP_SCALE_X:
			case SP_SCALE_Y:
			case SP_COLOR_R:
			case SP_COLOR_G:
			case SP_COLOR_B:
			case SP_COLOR_A:
				spr->propVals[i] *= cv->value;
				break;
			}
		}
	}
}

void sprite_bake( qhandle_t h, spriteProp_t prop )
{
	sprite_t * s = sprite_get( h );
	if( !s )
		return;

	s->baseVals[ prop ] = s->propVals[ prop ];
}

void sprites_update( void )
{
	int i;

	for ( i=0; i<sprites.count; i++ )
	{
		sprite_t * s = sprites.freelist[ i ];

		if ( s->flags & SPRITE_DELETEME )
		{
			sprites.freelist[ i-- ] = sprites.freelist[ --sprites.count ];
			sprites.freelist[ sprites.count ] = s;
			s->seqNum++;
		}
	}
}

void sprite_set_base_prop( qhandle_t h, spriteProp_t prop, float val )
{
	sprite_t *spr = sprite_get( h );

	if( !spr )
		return;

	spr->baseVals[prop] = val;
}

void sprite_set_base_props( qhandle_t h, float vals[SP_NUM_PROPS] )
{
	sprite_t *spr = sprite_get( h );

	if( !spr )
		return;

	Com_Memcpy( spr->baseVals, vals, sizeof( spr->baseVals ) );
}

void sprite_attach_curve( qhandle_t h, spriteProp_t prop, qhandle_t curve )
{
	uint i;
	sprite_t *spr = sprite_get( h );

	if( !spr )
		return;

	for( i=0; i<lengthof( spr->bindings[prop] ); i++ )
	{
		if( !curve_get( spr->bindings[ prop ][ i ] ) )
		{
			spr->bindings[prop][i] = curve;
			break;
		}
	}
}

const float* sprite_get_props( qhandle_t h )
{
	sprite_t *spr = sprite_get( h );

	if( !spr )
		return NULL;

	return spr->propVals;
}

qhandle_t sprite_create_rect( rectDef_t * r, qhandle_t shader, vec4_t color, vec4_t uv_coord )
{
	sprite_t *ret;

	ret = sprite_alloc();

	if( !ret )
		return 0;

	ret->flags = 0;
	ret->baseVals[ SP_POSITION_X ] = r->x + r->w*0.5f;
	ret->baseVals[ SP_POSITION_Y ] = r->y + r->h*0.5f;
	ret->baseVals[ SP_SCALE_X ] = 1.0f;
	ret->baseVals[ SP_SCALE_Y ] = 1.0f;
	ret->baseVals[ SP_COLOR_R ] = color[ 0 ];
	ret->baseVals[ SP_COLOR_G ] = color[ 1 ];
	ret->baseVals[ SP_COLOR_B ] = color[ 2 ];
	ret->baseVals[ SP_COLOR_A ] = color[ 3 ];

	ret->w = r->w;
	ret->h = r->h;

	ret->uv_coord[ 0 ]	= uv_coord[ 0 ];
	ret->uv_coord[ 1 ]	= uv_coord[ 1 ];
	ret->uv_coord[ 2 ]	= uv_coord[ 2 ];
	ret->uv_coord[ 3 ]	= uv_coord[ 3 ];
	ret->shader = shader;

	return sprite_getId( ret );
}

void sprite_destroy( qhandle_t h )
{
	//do any sprite-specific data cleanup
	sprite_t * s = sprite_get( h );
	if ( s )
		s->flags |= SPRITE_DELETEME;
}

void sprite_draw( sprite_t * s, vec3_t m[2] )
{
	float w = s->w * s->propVals[SP_SCALE_X];
	float h = s->h * s->propVals[SP_SCALE_Y];

	float Tx = s->propVals[SP_POSITION_X];
	float Ty = s->propVals[SP_POSITION_Y];

	float R = DEG2RAD( s->propVals[SP_ROTATION] );
	float Rc = cosf( R );
	float Rs = sinf( R );

	float Sx = DC->glconfig.xscale;
	float Sy = DC->glconfig.yscale;
	float Bx = DC->glconfig.xbias;
	float By = 0; //y bias, if we ever get one

	float mat[2][3];

	/*
		Set the matrix equal to S * M * T * R where:

			S	is a matrix that adjusts for screen resolution.
			M	is the incoming effect matrix.
			T	is the sprite's translation matrix.
			R	is the sprite's rotation matrix.

		Note that the sprite comes in pre-scaled.
	*/

	mat[0][0] = Sx * (Rc * m[0][0] + Rs * m[0][1]);
	mat[0][1] = Sx * (Rc * m[0][1] - Rs * m[0][0]);
	mat[0][2] = Sx * (Tx * m[0][0] + Ty * m[0][1] + m[0][2]) + Bx;
	mat[1][0] = Sy * (Rc * m[1][0] + Rs * m[1][1]);
	mat[1][1] = Sy * (Rc * m[1][1] - Rs * m[1][0]);
	mat[1][2] = Sy * (Tx * m[1][0] + Ty * m[1][1] + m[1][2]) + By;

	DC->setColor( &s->propVals[ SP_COLOR_R ] );
	trap_R_DrawSprite( -w * 0.5F, -h * 0.5F, w, h, mat, s->uv_coord, s->shader );
}


static qhandle_t effect_getId( effect_t * e ) {
	return ((e->seqNum&0xFFFF)<<16) | ((e-effects.effects+1)&0xFFFF);
}

static effect_t * effect_get( qhandle_t h ) {
	if ( h!=0 && h!=-1 )
	{
		int	index	= (h&0xFFFF)-1;
		int seqNum	= (h>>16)&0xFFFF;
		effect_t * e = effects.effects + index;

		if ( e->seqNum == seqNum )
			return e;
	}

	return 0;
}

void UI_Effect_Init()
{
	int i;
	for ( i=0; i<lengthof( curves.freelist ); i++ )
		curves.freelist[ i ] = curves.curves + i;

	for ( i=0; i<lengthof( sprites.freelist ); i++ )
		sprites.freelist[ i ] = sprites.sprites + i;

	for ( i=0; i<lengthof( effects.freelist ); i++ )
		effects.freelist[ i ] = effects.effects + i;
}

static effect_t * effect_alloc()
{
	if ( effects.count >= lengthof( effects.freelist ) )
		return 0;

	return effects.freelist[ effects.count++ ];
}

void UI_Effect_SetFlag( qhandle_t h, int flags )
{
	effect_t *e = effect_get( h );
	if ( e )
		e->flags |= flags;
}

void UI_Effect_ClearFlag( qhandle_t h, int flags )
{
	effect_t * e = effect_get( h );
	if( e )
		e->flags &= ~flags;
}

int UI_Effect_GetFlags( qhandle_t h )
{
	effect_t *e = effect_get( h );
	return e ? e->flags : 0;
}

int UI_Effect_GetTouchCount( qhandle_t h )
{
	effect_t *e = effect_get( h );

	if( !e )
		return 0;

	return e->touch;
}

int UI_Effect_SetTouchCount( qhandle_t h, int newTouch )
{
	int ret;
	effect_t *e = effect_get( h );

	if( !e )
		return 0;

	ret = e->touch;
	e->touch = newTouch;

	return ret;
}

effectDef_t * UI_Effect_Find( const char * name )
{
	int	lower = 0;
	int upper = DC->Assets.effectCount;
	int i;

	if ( DC->Assets.effectCount == 0 )
		return 0;

	for ( i=(lower+upper)>>1; ( lower<=upper ); )
	{
		int c = Q_stricmp( DC->Assets.effects[ i ]->name, name );

		if ( c == 0 )
			break;
		else if ( c > 0 )
			upper = i-1;
		else
			lower = i+1;

		i = (lower+upper)>>1;
	}

	if ( lower <= upper )
		return DC->Assets.effects[ i ];

	return 0;
}


static void UI_Effect_PrintLine( effect_t * e, const lineInfo_t * line, fontInfo_t * font, float italic,
	float x, float y, float fontScale )
{
	int i;
	float rgba[4];

	x += line->ox;

	italic = (italic * fontScale) / (float)font->glyphs[ 'A' ].height;

	Vec4_Cpy( rgba, line->startColor );

	for( i = 0; i < line->count; i++ )
	{
		if( Q_IsColorString( line->text + i ) )
		{
			i++;

			if( line->text[i] == '-' )
				Vec4_Cpy( rgba, line->defaultColor );
			else
				Vec4_Cpy( rgba, g_color_table[ColorIndex( line->text[i] )] );
	
			continue;
		}
		else
		{
			float step = 0.0f;

			if( line->text[i] == ' ' )
				step = (font->glyphs[' '].xSkip * fontScale) + line->sa;
			else
			{
				glyphInfo_t * glyph	= font->glyphs + line->text[i];
				rectDef_t r;
				vec4_t uv;

				if( e->spriteCount >= lengthof( e->sprites ) )
					break;

				r.x = x;
				r.y = y - (float)glyph->top * fontScale;
				r.w = glyph->imageWidth * fontScale;
				r.h = glyph->imageHeight * fontScale;

				uv[ 0 ] = glyph->s;
				uv[ 1 ] = glyph->t;
				uv[ 2 ] = glyph->s2;
				uv[ 3 ] = glyph->t2;

				e->sprites[ e->spriteCount++ ] = sprite_create_rect( &r, glyph->glyph, rgba, uv );

				step = glyph->xSkip * fontScale;
			}

			x += step;
		}
	}
}

static void UI_Effect_Print2( effect_t * e, fontInfo_t * font, const char * text, float x, float y )
{
	int i;
	float			fontScale	= e->def->textscale * font->glyphScale;
	float			oy = 0.0f;
	float			h;
	float			lineHeight	= font->glyphs[ 'A' ].height * fontScale;
	float			lineSpace;
	float			italic;
	lineInfo_t		lines[ 16 ];
	int				lineCount;
	int				lim = e->def->maxChars ? e->def->maxChars : -1;

	lineCount = trap_R_FlowText( font, fontScale, e->def->forecolor, e->def->forecolor, text, lim, e->def->textalignx, e->rect.w - x, lines, lengthof(lines) );
	lineSpace = lineHeight * 0.4f;

	h = lineCount * lineHeight + ((lineCount-1)*lineSpace);

	//	vertically justify text
	switch ( e->def->textaligny )
	{
		case TEXT_ALIGN_CENTER:		oy = (e->rect.h - h)*0.5f; break; // middle
		case TEXT_ALIGN_RIGHT:		oy = (e->rect.h - h); break; // bottom
		case TEXT_ALIGN_JUSTIFY:	lineHeight += (e->rect.h - h) / (float)lineCount; break;
	}

	italic = (e->def->textStyle&ITEM_TEXTSTYLE_ITALIC)?4.0f:0.0f;

	for( i = 0; i < lineCount; i++ )
	{
		float ly = y + oy + (lineHeight * (i+1)) + (lineSpace * i);
		UI_Effect_PrintLine( e, lines + i, font, italic, x, ly, fontScale );
	}
}

static void UI_Effect_Print( effect_t * e, const char * text, float x, float y ) {
	fontInfo_t f;
	trap_R_GetFont( e->def->font, e->def->textscale, &f );
	UI_Effect_Print2( e, &f, text, x, y );
}

static void AttachCurveToSprite( effect_t * e, int sprite, spriteProp_t prop, qhandle_t curve )
{
	if ( sprite == -1 )
	{
		int i;
		for ( i=0; i<e->spriteCount; i++ )
			sprite_attach_curve( e->sprites[ i ], prop, curve ); 

	}  else
		sprite_attach_curve( e->sprites[ sprite ], prop, curve ); 
}

static void SetSpriteProp( effect_t * e, int sprite, spriteProp_t prop, float value )
{
	if ( sprite == -1 )
	{
		int i;
		for ( i=0; i<e->spriteCount; i++ )
			sprite_set_base_prop( e->sprites[ i ], prop, value ); 

	}  else
		sprite_set_base_prop( e->sprites[ sprite ], prop, value ); 
}

static void BakeSprite( effect_t * e, int sprite, spriteProp_t prop )
{
	if ( sprite == -1 )
	{
		int i;
		for ( i=0; i<e->spriteCount; i++ )
			sprite_bake( e->sprites[ i ], prop ); 

	}  else
		sprite_bake( e->sprites[ sprite ], prop ); 
}


static void effect_eval( effect_t *e )
{
	int i;
	for( i = 0; i < EP_NUM_PROPS; i++ )
	{
		int j;

		e->propVals[i] = e->baseVals[i];

		for( j = 0; j < lengthof( e->props[i] ); j++ )
		{
			animCurve_t *cv = curve_get( e->props[i][j] );
			if( !cv )
				continue;
		
			switch( i )
			{
			case EP_SCALE_X:
			case EP_SCALE_Y:
				e->propVals[i] *= curve_get_value( e->props[i][j] );
				break;

			default:
				e->propVals[i] += curve_get_value( e->props[i][j] );
				break;
			}
		}
	}
}

static void effect_attach_curve( effect_t *e, effectProp_t prop, qhandle_t curve )
{
	int i;
	for( i = 0; i < lengthof( e->props[prop] ); i++ )
	{
		if( !curve_get( e->props[prop][i] ) )
		{
			e->props[prop][i] = curve;
			break;
		}
	}
}

typedef struct duration_t
{
	float		time;
	qboolean	hold;
} duration_t;

static duration_t ParseDuration( const char **s )
{
	duration_t ret;
	const char *tmp;

	ret.time = fatof( COM_ParseExt( s, qfalse ) );

	tmp = *s;
	if( Q_stricmp( COM_ParseExt( &tmp, qfalse ), "hold" ) == 0 )
	{
		ret.hold = qtrue;
		*s = tmp;
	}
	else
		ret.hold = qfalse;

	return ret;
}

static float ParseAngle( const char **s )
{
	char *tok = COM_ParseExt( s, qfalse );

	if( Q_stricmp( tok, "rand" ) == 0 )
	{
		return Q_random() * M_PI * 2;
	}

	return DEG2RAD( fatof( tok ) );
}

/*
	Isct_RayLineComp

	Find the intersection between a ray originating at o,
	with a vector v, and a perpendicular line that crosses l.
*/
qboolean Isct_RayLineComp( float o, float d, float l, float *isctT )
{
	float t;

	if( fabsf( d ) < 0.001F )
	{
		if( o == l )
		{
			if( isctT ) *isctT = 0;
			return qtrue;
		}

		return qfalse;
	}

	t = (l - o) / d;

	if( isctT ) *isctT = t;
	return t >= 0 ? qtrue : qfalse;
}

qboolean Isct_RayLeavingBox( const vec2_t ro, const vec2_t rd, const rectDef_t *rc, float *isctT )
{
	float tmin = 0;

	//ASSUME: that the ray originates inside the rectangle.
	//ASSUME: that rd != < 0, 0 >

	if( fabsf( rd[0] ) > 0.001F )
	{
		if( rd[0] < 0 )
		{
			//only check the left side
			if( !Isct_RayLineComp( ro[0], rd[0], rc->x, &tmin ) )
				return qfalse;
		}
		else
		{
			//only the right
			if( !Isct_RayLineComp( ro[0], rd[0], rc->x + rc->w, &tmin ) )
				return qfalse;
		}
	}

	if( fabsf( rd[1] ) > 0.001F )
	{
		float t;

		if( rd[1] < 0 )
		{
			//only check the top side
			if( !Isct_RayLineComp( ro[1], rd[1], rc->y, &t ) )
				return qfalse;
		}
		else
		{
			//only the right
			if( !Isct_RayLineComp( ro[1], rd[1], rc->y + rc->h, &t ) )
				return qfalse;
		}

		if( t < tmin ) tmin = t;
	}

	if( isctT ) *isctT = tmin;
	return qtrue;
}

qboolean Rect_PlaceOutsideOfRect( rectDef_t *rc, const rectDef_t *ro, const vec2_t v, float *isctT )
{
	float t;
	vec2_t o;
	
	//first find the spot on rc that's opposite the center along -v
	o[0] = rc->x + rc->w * 0.5F;
	o[1] = rc->y + rc->h * 0.5F;

	//use v instead of -v (just negate t after), works since ray origin = center of rect
	if( !Isct_RayLeavingBox( o, v, rc, &t ) )
		return qfalse;

	o[0] += v[0] * -t;
	o[1] += v[1] * -t;
	
	//now find out how far along we have to go to get this point outside the outer rect
	if( !Isct_RayLeavingBox( o, v, ro, &t ) )
	{
		//the rect is already outside
		if( isctT ) *isctT = 0;
		return qtrue;
	}

	if( isctT ) *isctT = t;

	//move the rectangle	
	rc->x += v[0] * t;
	rc->y += v[1] * t;

	return qtrue;
}

static void AttachCurveToSpriteCB( effect_t *e, stackFrame_t *sf, int prop, qhandle_t curve )
{
	AttachCurveToSprite( e, sf->i, prop, curve );
}

static void AttachCurveToEffectCB( effect_t *e, stackFrame_t *sf, int prop, qhandle_t curve )
{
	REF_PARAM( sf );

	effect_attach_curve( e, prop, curve );
}

#define EnsureCurveCount( newCount, stmt )								\
	if( e->curveCount + (newCount) >= lengthof( e->curves ) )			\
		stmt;															\
	else																\
		(void)0
#define EnsureCurveCount_return( newCount ) EnsureCurveCount( newCount, return )
#define EnsureCurveCount_break( newCount ) EnsureCurveCount( newCount, break )

typedef void (*AttachCurves_t)( effect_t *e, stackFrame_t *sf, int prop, qhandle_t curve );

static void AnimEffect_SlideDirected( AttachCurves_t attach, effect_t *e, stackFrame_t *sf, qboolean in )
{
	vec2_t v;
	rectDef_t ro, ri;

	qhandle_t c;

	float angle = ParseAngle( &sf->pc );
	duration_t dur = ParseDuration( &sf->pc );

	EnsureCurveCount_return( 2 );

	ro.x = -(DC->glconfig.xbias / DC->glconfig.xscale);
	ro.w = 640 + (DC->glconfig.xbias / DC->glconfig.xscale) * 2.0F;

	ro.y = 0;
	ro.h = 480;

	v[0] = cosf( angle );
	v[1] = -sinf( angle );

	ri = e->rect;

	if( !Rect_PlaceOutsideOfRect( &ri, &ro, v, NULL ) )
		//degenerate rectangle...
		return;

	if( in )
	{
		c = curve_create_lerp( DC->realTime, ri.x - e->rect.x, 0.0F, dur.time, dur.hold );
		if( c )
		{
			e->curves[ e->curveCount++ ] = c;
			attach( e, sf, CP_POSITION_X, c );
		}

		c = curve_create_lerp( DC->realTime, ri.y - e->rect.y, 0.0F, dur.time, dur.hold );
		if( c )
		{
			e->curves[ e->curveCount++ ] = c;
			attach( e, sf, CP_POSITION_Y, c );
		}
	}
	else
	{
		c = curve_create_lerp( DC->realTime, 0.0F, ri.x - e->rect.x, dur.time, dur.hold );
		if( c )
		{
			e->curves[ e->curveCount++ ] = c;
			attach( e, sf, CP_POSITION_X, c );
		}

		c = curve_create_lerp( DC->realTime, 0.0F, ri.y - e->rect.y, dur.time, dur.hold );
		if( c )
		{
			e->curves[ e->curveCount++ ] = c;
			attach( e, sf, CP_POSITION_Y, c );
		}
	}
}

static void effect_exec( effect_t * e )
{
	stackFrame_t * sf;

	if( e->def->flags & ED_DIEWITHMENU )
	{
		if ( e->item && !(((menuDef_t*)e->item->parent)->window.flags & WINDOW_VISIBLE) )
		{
			e->flags |= EF_DELETEME;
			return;
		}
	}

	if ( e->flags & EF_NORECT )
		return;

	if ( e->sleeping > 0 )
	{
		e->sleeping -= DC->frameTime;

		if ( e->sleeping > 0 )
			return;
	}

	if ( e->waitfor )
	{
		e->waitfor &= ~e->flags;
		if ( e->waitfor )
			return;
	}

	sf = &e->stack[ e->sp - 1 ];

	for ( ;; )
	{
		char * t = COM_ParseExt( &sf->pc, qtrue );
		if ( t[ 0 ] == '\0' )
			break;

		switch( SWITCHSTRING( t ) )
		{
			//	for
		case CS('f','o','r',0):
			{
				const char * pc;
				int start = 0;

				// eat '<'
				pc = COM_ParseExt( &sf->pc, qfalse );
				if ( pc[0] != '<' )
				{
					start = atoi( pc );
					COM_ParseExt( &sf->pc, qfalse );
				}

				pc = sf->pc;

				sf++;
				sf->i	= start;
				sf->pc	= pc;
			}
			break;

			//	>	- end of loop
		case CS('>',0,0,0):
			{
				sf->i++;

				if ( sf->i < e->spriteCount )
					sf->pc = sf[ -1 ].pc;
				else
				{
					const char * pc = sf->pc;
					sf--;
					sf->pc = pc;
				}	
			}
			break;

			//	wait
		case CS('w','a','i','t'):
			{
				char * t = COM_ParseExt( &sf->pc, qfalse );
				switch ( SWITCHSTRING( t ) )
				{
					//curves
				case CS('c','u','r','v'):
					e->waitfor |= EF_NOANIM;
					break;
					
					//nofocus
				case CS('n','o','f','o'):
					e->waitfor |= EF_NOFOCUS;
					break;

					//forever
				case CS('f','o','r','e'):
					e->waitfor |= EF_FOREVER;
					break;

				default:
					e->sleeping	= atoi( t );
				}

				e->sp = sf - e->stack + 1;
				return;
			}
			break;

			//	die_if_nofocus
		case CS('d','i','e','_'):
			{
				if ( e->flags & EF_NOFOCUS )
					e->flags |= EF_DELETEME;	
			}
			break;

			//	offset_from_cursor
		case CS('o','f','f','s'):
			{
				float xb = DC->glconfig.xbias / DC->glconfig.xscale;

				e->rect.x = e->def->rect.x + (float)DC->cursorx;
				e->rect.y = e->def->rect.y + (float)DC->cursory;
				e->rect.w = e->def->rect.w;
				e->rect.h = e->def->rect.h;

				//	don't let rect go off screen
				if ( e->rect.x < -xb )	e->rect.x = -xb;
				if ( e->rect.y < 0.0f ) e->rect.y = 0.0f;
				if ( e->rect.x + e->rect.w > 640.0f + xb ) e->rect.x = 640.0f + xb - e->rect.w;
				if ( e->rect.y + e->rect.h > 480.0f ) e->rect.y = 480.0f - e->rect.h;

			} break;

			//	sprite
		case CS('s','p','r','i'):
			{
				vec4_t uv = { 0.0f, 0.0f, 1.0f, 1.0f };
				rectDef_t r;

				if( e->spriteCount >= lengthof( e->sprites ) )
					break;

				r.x = 0.0f;
				r.y = 0.0f;
				r.w = e->rect.w;
				r.h = e->rect.h;
				e->sprites[ e->spriteCount++ ] = sprite_create_rect( &r, (e->def->shader)?e->def->shader:e->shader, e->def->backcolor, uv );
			}
			break;

		case CS('d','r','a','w'):
			{
				vec4_t uv = { 0.0f, 0.0f, 1.0f, 1.0f };
				rectDef_t r;
				qhandle_t s = (e->def->shader)?e->def->shader:e->shader;
				r.x = fatof( COM_ParseExt( &sf->pc, qfalse ) );
				r.y = fatof( COM_ParseExt( &sf->pc, qfalse ) );
				r.w = fatof( COM_ParseExt( &sf->pc, qfalse ) );
				r.h = fatof( COM_ParseExt( &sf->pc, qfalse ) );
				
				if( e->spriteCount >= lengthof( e->sprites ) )
					break;


				if ( s )
					e->sprites[ e->spriteCount++ ] = sprite_create_rect( &r, s, e->def->backcolor, uv );
			} break;

			//	print
		case CS('p','r','i','n'):
			{
				float x = fatof( COM_ParseExt( &sf->pc, qfalse ) );
				float y = fatof( COM_ParseExt( &sf->pc, qfalse ) );
				UI_Effect_Print( e, e->text, x,y );
			}
			break;
			
		case CS('s','e','t',0):
			{
				switch( SWITCHSTRING( COM_ParseExt( &sf->pc, qfalse ) ) )
				{
				case CS('a',0,0,0):
					SetSpriteProp( e, sf->i, SP_COLOR_A, fatof( COM_ParseExt( &sf->pc, qfalse ) ) );
					break;

				case CS( 'r', 'o', 't', 0 ):
					SetSpriteProp( e, sf->i, SP_ROTATION, fatof( COM_ParseExt( &sf->pc, qfalse ) ) );
					break;
				}
			}
			break;

		case CS('b','a','k','e'):
			{
				switch( SWITCHSTRING( COM_ParseExt( &sf->pc, qfalse ) ) )
				{
				case CS('s','c','a','l'):
					BakeSprite( e, sf->i, SP_SCALE_X );
					BakeSprite( e, sf->i, SP_SCALE_Y );
					break;
				}
			}
			break;

			/*
				Animation launchers.

				The commands that actuall attach curves and kick off animations go here:
			*/

			//	move
		case CS('m','o','v','e'):
			{
				qhandle_t c;
				float x = fatof( COM_ParseExt( &sf->pc, qfalse ) );
				float y = fatof( COM_ParseExt( &sf->pc, qfalse ) );
				
				EnsureCurveCount_break( 2 );

				c = curve_create_constant( DC->realTime, x, 0.0f );
				if( c )
				{
					e->curves[ e->curveCount++ ] = c;
					AttachCurveToSprite( e, sf->i, SP_POSITION_X, c );
				}

				c = curve_create_constant( DC->realTime, y, 0.0f );
				if( c )
				{
					e->curves[ e->curveCount++ ] = c;
					AttachCurveToSprite( e, sf->i, SP_POSITION_Y, c );
				}

			}
			break;

			//	fade
		case CS('f','a','d','e'):
			{
				switch( SWITCHSTRING( COM_ParseExt( &sf->pc, qfalse ) ) )
				{
					//	out
				case CS('o','u','t',0):
					{
						duration_t dur = ParseDuration( &sf->pc );					
						qhandle_t c;
						
						EnsureCurveCount_break( 1 );

						c = curve_create_lerp( DC->realTime, 1.0f, 0.0f, dur.time, dur.hold );
						if( c )
						{
							e->curves[ e->curveCount++ ] = c;
							AttachCurveToSprite( e, sf->i, SP_COLOR_A, c );
						}
					}
					break;

					//	in
				case CS('i','n',0,0):
					{
						duration_t dur = ParseDuration( &sf->pc );
						qhandle_t c;
						
						EnsureCurveCount_break( 1 );

						c = curve_create_lerp( DC->realTime, 0.0f, 1.0f, dur.time, dur.hold );
						if( c )
						{
							e->curves[ e->curveCount++ ] = c;
							AttachCurveToSprite( e, sf->i, SP_COLOR_A, c );
						}
					}
					break;
				}
			}
			break;

			//	slide
		case CS('s','l','i','d'):
			{
				switch( SWITCHSTRING( COM_ParseExt( &sf->pc, qfalse ) ) )
				{
					//	in_from_right
				case CS('i','n','_','f'):
					{
						float		x = 640.0f + (DC->glconfig.xbias / DC->glconfig.xscale)*2.0f;
						duration_t dur = ParseDuration( &sf->pc );
						qhandle_t c;
							
						EnsureCurveCount_break( 1 );

						c = curve_create_lerp( DC->realTime, x - e->rect.x, 0.0f, dur.time, dur.hold );
						if( c )
						{
							e->curves[ e->curveCount++ ] = c;
							AttachCurveToSprite( e, sf->i, SP_POSITION_X, c );
						}
					}
					break;

					//	up
				case CS('u','p',0,0):
					{
						float distance	= fatof( COM_ParseExt( &sf->pc, qfalse ) );
						duration_t dur = ParseDuration( &sf->pc );
						qhandle_t	c;

						EnsureCurveCount_break( 1 );

						c = curve_create_lerp( DC->realTime, 0.0f, -distance, dur.time, dur.hold );
						if( c )
						{
							e->curves[ e->curveCount++ ] = c;
							AttachCurveToSprite( e, sf->i, SP_POSITION_Y, c );
						}
					}
					break;

				//efx_slide down
				case CS('d','o','w','n'):
					{
						float distance	= fatof( COM_ParseExt( &sf->pc, qfalse ) );
						duration_t dur = ParseDuration( &sf->pc );
						qhandle_t c;
						
						EnsureCurveCount_break( 1 );

						c = curve_create_lerp( DC->realTime, 0.0f, distance, dur.time, dur.hold );
						if( c )
						{
							e->curves[ e->curveCount++ ] = c;
							AttachCurveToSprite( e, sf->i, SP_POSITION_Y, c );
						}
					}
					break;

				case CS( 'f', 'r', 'o', 'm' ):
					AnimEffect_SlideDirected( AttachCurveToSpriteCB, e, sf, qtrue );
					break;

				case CS( 't', 'o', 0, 0 ):
					AnimEffect_SlideDirected( AttachCurveToSpriteCB, e, sf, qfalse );
					break;
				}
			}
			break;

			//	scale
		case CS('s','c','a','l'):
			{
				int	cmd	= SWITCHSTRING( COM_ParseExt( &sf->pc, qfalse ) );
				float start		= fatof( COM_ParseExt( &sf->pc, qfalse ) );
				float end		= fatof( COM_ParseExt( &sf->pc, qfalse ) );
				duration_t dur = ParseDuration( &sf->pc );
				qhandle_t c;

				EnsureCurveCount_break( 1 );
				
				c = curve_create_lerp( DC->realTime, start, end, dur.time, dur.hold );
				if( !c )
					break;

				e->curves[ e->curveCount++ ] = c;

				switch( cmd )
				{
				case CS('x',0,0,0):
					AttachCurveToSprite( e, sf->i, SP_SCALE_X, c );
					break;
				
				case CS('y',0,0,0):
					AttachCurveToSprite( e, sf->i, SP_SCALE_Y, c );
					break;

				case CS('x','y',0,0):
					AttachCurveToSprite( e, sf->i, SP_SCALE_X, c );
					AttachCurveToSprite( e, sf->i, SP_SCALE_Y, c );
					break;
				}

			}
			break;

		case CS( 'r', 'o', 't', 'a' ):
			{
				float start		= fatof( COM_ParseExt( &sf->pc, qfalse ) );
				float end		= fatof( COM_ParseExt( &sf->pc, qfalse ) );
				duration_t dur	= ParseDuration( &sf->pc );
				qhandle_t c;
				
				EnsureCurveCount_break( 1 );

				c = curve_create_lerp( DC->realTime, start, end, dur.time, dur.hold );
				if( c )
				{
					e->curves[ e->curveCount++ ] = c;		
					AttachCurveToSprite( e, sf->i, SP_ROTATION, c );
				}
			}
			break;

			/*
				Effect animation launchers.

				Same as above only with the efx_ prefix. These apply to the entire effect.
			*/
		case CS( 'e', 'f', 'x', '_' ):
			switch( SWITCHSTRING( t + 4 ) )
			{
				//efx_move
			case CS('m','o','v','e'):
				{
					qhandle_t c;
					float x = fatof( COM_ParseExt( &sf->pc, qfalse ) );
					float y = fatof( COM_ParseExt( &sf->pc, qfalse ) );

					EnsureCurveCount_break( 2 );

					c = curve_create_constant( DC->realTime, x, 0.0f );
					if( c )
					{
						e->curves[e->curveCount++] = c;
						effect_attach_curve( e, EP_POSITION_X, c );
					}

					c = curve_create_constant( DC->realTime, y, 0.0f );
					if( c )
					{
						e->curves[e->curveCount++] = c;
						effect_attach_curve( e, EP_POSITION_Y, c );
					}
				}
				break;

				//efx_slide
			case CS('s','l','i','d'):
				{
					switch( SWITCHSTRING( COM_ParseExt( &sf->pc, qfalse ) ) )
					{
						//efx_slide in_from_right
					case CS('i','n','_','f'):
						{
							float x = 640.0f + (DC->glconfig.xbias / DC->glconfig.xscale) * 2.0F;
							duration_t dur = ParseDuration( &sf->pc );						
							qhandle_t c;
							
							EnsureCurveCount_break( 1 );

							c = curve_create_lerp( DC->realTime, x - e->rect.x, 0.0F, dur.time, dur.hold );
							if( c )
							{
								e->curves[ e->curveCount++ ] = c;						
								effect_attach_curve( e, EP_POSITION_X, c );
							}
						}
						break;

						//efx_slide up
					case CS('u','p',0,0):
						{
							float distance	= fatof( COM_ParseExt( &sf->pc, qfalse ) );
							duration_t dur = ParseDuration( &sf->pc );
							qhandle_t c;
							
							EnsureCurveCount_break( 1 );

							c = curve_create_lerp( DC->realTime, 0.0f, -distance, dur.time, dur.hold );
							if( c )
							{
								e->curves[ e->curveCount++ ] = c;
								effect_attach_curve( e, EP_POSITION_Y, c );
							}
						}
						break;

					//efx_slide down
					case CS('d','o','w','n'):
						{
							float distance	= fatof( COM_ParseExt( &sf->pc, qfalse ) );
							duration_t dur = ParseDuration( &sf->pc );
							qhandle_t c;

							EnsureCurveCount_break( 1 );

							c = curve_create_lerp( DC->realTime, 0.0f, distance, dur.time, dur.hold );
							if( c )
							{
								e->curves[ e->curveCount++ ] = c;
								effect_attach_curve( e, EP_POSITION_Y, c );
							}
						}
						break;

				case CS( 'f', 'r', 'o', 'm' ):
					AnimEffect_SlideDirected( AttachCurveToEffectCB, e, sf, qtrue );
					break;

				case CS( 't', 'o', 0, 0 ):
					AnimEffect_SlideDirected( AttachCurveToEffectCB, e, sf, qfalse );
					break;
					}
				} break;

				//efx_scale
			case CS('s','c','a','l'):
				{
					int	cmd	= SWITCHSTRING( COM_ParseExt( &sf->pc, qfalse ) );
					float start = fatof( COM_ParseExt( &sf->pc, qfalse ) );
					float end = fatof( COM_ParseExt( &sf->pc, qfalse ) );
					duration_t dur = ParseDuration( &sf->pc );
					qhandle_t c;
					
					EnsureCurveCount_break( 1 );

					c = curve_create_lerp( DC->realTime, start, end, dur.time, dur.hold );
					if( !c )
						break;

					e->curves[ e->curveCount++ ] = c;

					switch( cmd )
					{
					case CS('x',0,0,0):
						effect_attach_curve( e, EP_SCALE_X, c );
						break;

					case CS('y',0,0,0):
						effect_attach_curve( e, EP_SCALE_Y, c );
						break;

					case CS('x','y',0,0):
						effect_attach_curve( e, EP_SCALE_X, c );
						effect_attach_curve( e, EP_SCALE_Y, c );
						break;
					}
				}

			case CS( 'r', 'o', 't', 'a' ):
				{
					float start		= fatof( COM_ParseExt( &sf->pc, qfalse ) );
					float end		= fatof( COM_ParseExt( &sf->pc, qfalse ) );
					duration_t dur = ParseDuration( &sf->pc );
					qhandle_t c;
					
					EnsureCurveCount_break( 1 );

					c = curve_create_lerp( DC->realTime, start, end, dur.time, dur.hold );
					if( c )
					{
						e->curves[ e->curveCount++ ] = c;			
						effect_attach_curve( e, EP_ROTATION, c );
					}
				}
				break;
			}
			break;
		}
	}

	sf--;
	e->sp = sf - e->stack + 1;
}

static void effect_setrect( effect_t * e, rectDef_t *r )
{
	e->rect = *r;
}

static void effect_buildmatrix( effect_t * e, vec3_t m[2] )
{
	float Sx, Sy, R, Tx, Ty;
	float Rs, Rc;
	float Cx, Cy;

	/*
		Scale, rotate, translate.

		Build a matrix that:
			-	scales by e->props[EP_SCALE_X/Y] from the center of e->rect
			-	rotates by e->props[EP_ROTATION] degress around the center of e->rect
			-	translates by e->props[EP_POSITION_X/Y]
	*/
			
	Sx = e->propVals[EP_SCALE_X];
	Sy = e->propVals[EP_SCALE_Y];

	R = DEG2RAD( e->propVals[EP_ROTATION] );
	Rs = sinf( R );
	Rc = cosf( R );

	Tx = e->propVals[EP_POSITION_X] + e->rect.x;
	Ty = e->propVals[EP_POSITION_Y] + e->rect.y;

	Cx = -e->rect.w * 0.5F;
	Cy = -e->rect.h * 0.5F;

	/*
		This nasty beast is the matrix M = T * Tc-1 * R * S * Tc where:

			T	is the required translation
			Tc	is a translation that centers e->rect on the origin
			R	is the required rotation
			S	is the required scale
	*/

	m[0][0] = Rc * Sx;	m[0][1] = -Rs * Sy;		m[0][2] = Rc * Sx * Cx - Rs * Sy * Cy + Tx - Cx;
	m[1][0] = Rs * Sx;	m[1][1] = Rc * Sy;		m[1][2] = Rs * Sx * Cx + Rc * Cy * Sy + Ty - Cy;
}

void UI_Effect_SetJustRect( qhandle_t h, rectDef_t * r )
{
	effect_t * e = effect_get( h );
	if ( e )
	{
		effect_setrect( e, r );
		e->flags &= ~EF_NORECT;
	}
}

void UI_Effect_SetRect( qhandle_t h, rectDef_t * r, itemDef_t * item )
{
	effect_t * e = effect_get( h );
	if ( e )
	{
		effect_setrect( e, r );
		e->item = item;
		e->flags &= ~EF_NORECT;
	}
}

void UI_Effect_SetItem( qhandle_t h, itemDef_t * item )
{
	effect_t * e = effect_get( h );
	if ( e )
		e->item = item;
}

static effect_t* effect_create( effectDef_t *effect )
{
	effect_t * e = effect_alloc();
	short seqNum;

	if( !e )
		return NULL;

	seqNum = e->seqNum;
	
	Com_Memset( e, 0, sizeof(*e) );

	e->seqNum = seqNum;
	e->def = effect;

	e->baseVals[EP_SCALE_X] = 1;
	e->baseVals[EP_SCALE_Y] = 1;

	return e;
}

qhandle_t UI_Effect_SpawnText( rectDef_t * r, effectDef_t * effect, const char * text, qhandle_t shader )
{
	if ( effect )
	{
		effect_t * e = effect_create( effect );

		if( !e )
			return 0;

		if ( r )
			effect_setrect( e, r );
		else
			e->flags |= EF_NORECT;

		if ( text )
			Q_strncpyz( e->text, text, sizeof(e->text) );

		e->shader = shader;

		e->stack[ 0 ].pc	= (char*)e->def->action;
		e->stack[ 0 ].i		= -1;
		e->sp				= 1;

		effect_exec( e );

		return effect_getId( e );
	}

	return 0;
}

qboolean UI_Effect_IsAlive( qhandle_t h )
{
	return effect_get(h) != 0;
}

effectDef_t* UI_Effect_GetEffect( qhandle_t h )
{
	effect_t *e = effect_get( h );

	if( !e )
		return NULL;

	return e->def;
}

void UI_Effects_Update()
{
	curves_update();
	sprites_update();
}


extern int trap_Key_GetCatcher( void );

void UI_Effects_Draw( menuDef_t * menu )
{
	int i,j;
	int ui_active =  trap_Key_GetCatcher() & KEYCATCH_UI;
	for ( i=0; i<effects.count; i++ )
	{
		vec3_t m[ 2 ];
		effect_t * e = effects.freelist[ i ];

		if( e->def->flags & ED_DIEWITHMENU )
		{
			if( e->item && !(((menuDef_t*)e->item->parent)->window.flags & WINDOW_VISIBLE) )
				e->flags |= EF_DELETEME;
		}

		// delete effect
		if( e->sp == 0 || e->flags & EF_DELETEME )
		{
			for ( j=0; j<e->curveCount; j++ )
				curve_destroy( e->curves[ j ] );

			for ( j=0; j<e->spriteCount; j++ )
				sprite_destroy( e->sprites[ j ] );

			effects.freelist[ i-- ] = effects.freelist[ --effects.count ];
			effects.freelist[ effects.count ] = e;
			e->seqNum++;
			continue;
		}

		if ( (e->def->flags & ED_NODRAW_INUI) && ui_active )
			continue;

		if( menu )
		{
			//only want items attached to menu, excluding ALWAYSONTOP
			if( !e->item || e->item->parent != menu || e->def->flags & ED_ALWAYSONTOP )
				continue;	// do not draw if not attached to the current menu, or always on top
		}
		else
		{
			//want unattached items except where always on top
			if( e->item ) {
				if ( !(e->def->flags & ED_ALWAYSONTOP) )
					continue;	// do not draw if attached to a menu and not on top

				if ( !(((menuDef_t*)e->item->parent)->window.flags & WINDOW_VISIBLE) )
					continue;	// do not draw if associated menu is not drawing
			}
		}

		if ( e->sp > 0 )
			effect_exec( e );

		if( e->sp == 0 || e->flags & EF_DELETEME )
			//effect just died, skip (will get eaten on next frame)
			continue;
		
		e->flags |= EF_NOANIM;	// flag as nothing animating
		for ( j=0; j<e->curveCount; j++ )
		{
			animCurve_t * c = curve_get( e->curves[ j ] );

			if ( !c )
				continue;

			if ( !c->flags&CURVE_DONE )
			{
				if ( c->type != CT_CONSTANT )
					e->flags &= ~EF_NOANIM;	// something is animating

				curve_eval( c, DC->realTime );
				continue;
			}

			// don't delete curves set to hold
			if ( c->type == CT_LERP && c->data.lerp.hold )
				continue;

			c->flags |= CURVE_DELETEME;
			e->curves[ j-- ] = e->curves[ --e->curveCount ];
		}

		effect_eval( e );
		effect_buildmatrix( e, m );

		for ( j=0; j<e->spriteCount; j++ )
		{
			if ( !(e->flags & EF_NORECT) ) {
				sprite_t * s = sprite_get( e->sprites[ j ] );
				if ( s ) {
					sprite_eval( s );
					sprite_draw( s, m );
				}
			}
		}

		switch( e->flags & EF_ONEFRAME_BITS )
		{
		case EF_ONEFRAME_RECT:
			e->flags |= EF_NORECT;
			break;
		case EF_ONEFRAME_NOFOCUS:
			e->flags |= EF_NOFOCUS;
			break;
		}

		if( e->touch )
			e->touch--;
	}
}
