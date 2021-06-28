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

#define LF "\n"
#define BEGINVP "!!ARBvp1.0"
#define BEGINFP "!!ARBfp1.0"
#define ENDPROG LF "END"

static struct
{
	const char	*prog_name;

	vpFlags_t	flags;
	const char	*text;
} stdvp[SPV_MAX_PROGRAMS] = 
{
	//SPV_DIFFUSE
	{
		"per-pixel diffuse lighting",
		VPF_NONE,


		BEGINVP

		LF	"OPTION ARB_position_invariant;"

		LF	"PARAM	mv[4]			= { state.matrix.modelview };"

		LF	"ATTRIB iPos			= vertex.position;"
		LF	"ATTRIB	iNorm			= vertex.normal;"
		LF	"ATTRIB	iTc				= vertex.texcoord[0];"

		LF	"OUTPUT	oTc				= result.texcoord[0];"
		LF	"OUTPUT	oNorm			= result.texcoord[1];"

		//transfer the tex coord, assume identity texture matrix
		LF	"MOV	oTc.xy, iTc;"

		//camara-space normal
		LF	"DP3	oNorm.x,	mv[0],	iNorm;"
		LF	"DP3	oNorm.y,	mv[1],	iNorm;"
		LF	"DP3	oNorm.z,	mv[2],	iNorm;"

		ENDPROG
	},

	//SPV_SPEC
	{
		"per-pixel specular lighting",
		VPF_NONE,


		BEGINVP

		LF	"OPTION ARB_position_invariant;"

		LF	"PARAM	mv[4]			= { state.matrix.modelview };"

		LF	"ATTRIB iPos			= vertex.position;"
		LF	"ATTRIB	iNorm			= vertex.normal;"
		LF	"ATTRIB	iTc				= vertex.texcoord[0];"

		LF	"OUTPUT	oTc				= result.texcoord[0];"
		LF	"OUTPUT	oVPos			= result.texcoord[1];"
		LF	"OUTPUT	oNorm			= result.texcoord[2];"

		//transfer the tex coord, assume identity texture matrix
		LF	"MOV	oTc.xy, iTc;"

		//get view space normal and position
		LF	"DP4	oVPos.x,	mv[0],	-iPos;"
		LF	"DP4	oVPos.y,	mv[1],	-iPos;"
		LF	"DP4	oVPos.z,	mv[2],	-iPos;"

		LF	"DP3	oNorm.x,	mv[0],	iNorm;"
		LF	"DP3	oNorm.y,	mv[1],	iNorm;"
		LF	"DP3	oNorm.z,	mv[2],	iNorm;"

		ENDPROG
	},

	//SPV_SPEC_NM
	{
		"per-pixel specular lighting (with normal map)",
		VPF_NONE,


		BEGINVP

		LF	"OPTION ARB_position_invariant;"

		LF	"PARAM	mEye			= program.local[0];" //in model space

		LF	"ATTRIB	iPos			= vertex.position;"
		LF	"ATTRIB iNorm			= vertex.normal;"
		LF	"ATTRIB	iTan			= vertex.attrib[6];"
		LF	"ATTRIB	iBin			= vertex.attrib[7];"
		LF	"ATTRIB	iTc				= vertex.texcoord[0];"

		LF	"OUTPUT	oTc				= result.texcoord[0];"
		LF	"OUTPUT	oEye			= result.texcoord[1];"
		LF	"OUTPUT oTm0			= result.texcoord[2];"
		LF	"OUTPUT	oTm1			= result.texcoord[3];"
		LF	"OUTPUT	oTm2			= result.texcoord[4];"

		LF	"TEMP	eye, tan, bin;"

		//transfer the tex coord, assume identity texture matrix
		LF	"MOV	oTc.xy,		iTc;"

		//get tangent space eye vector
		LF	"SUB	oEye.xyz,	mEye,		iPos;"

		//normalize the tangent and binormal (normal done in software)
		LF	"DP3	tan.w,		iTan,		iTan;"
		LF	"RSQ	tan.w,		tan.w;"
		LF	"MUL	tan.xyz,	iTan,		tan.w;"

		LF	"DP3	bin.w,		iBin,		iBin;"
		LF	"RSQ	bin.w,		bin.w;"
		LF	"MUL	bin.xyz,	iBin,		bin.w;"

		//transpose the tangent basis
		LF	"MOV	oTm0.x,		tan.x;"
		LF	"MOV	oTm1.x,		tan.y;"
		LF	"MOV	oTm2.x,		tan.z;"

		LF	"MOV	oTm0.y,		bin.x;"
		LF	"MOV	oTm1.y,		bin.y;"
		LF	"MOV	oTm2.y,		bin.z;"

		LF	"MOV	oTm0.z,		iNorm.x;"
		LF	"MOV	oTm1.z,		iNorm.y;"
		LF	"MOV	oTm2.z,		iNorm.z;"

		ENDPROG
	},

	//SPV_DELUXEMAP_NO_SPEC
	{
		"deluxe mapped lighting (no specular)",
		VPF_NONE,

		BEGINVP

		LF	"OPTION ARB_position_invariant;"

		LF	"ATTRIB	iPos			= vertex.position;"
		LF	"ATTRIB	iTcT			= vertex.texcoord[0];"
		LF	"ATTRIB	iTcL			= vertex.texcoord[1];"

		LF	"OUTPUT oTcN			= result.texcoord[0];"
		LF	"OUTPUT	oTcD			= result.texcoord[1];"
		LF	"OUTPUT	oTcT			= result.texcoord[2];"
		LF	"OUTPUT	oTcL			= result.texcoord[3];"

		//pass the texture coords through
		LF	"MOV	oTcN.xy,	iTcT;"
		LF	"MOV	oTcD.xy,	iTcL;"
		LF	"MOV	oTcT.xy,	iTcT;"
		LF	"MOV	oTcL.xy,	iTcL;"
	
		ENDPROG
	},

	//SPV_DELUXEMAP
	{
		"deluxe mapped lighting",
		VPF_NONE,

		BEGINVP

		LF	"OPTION ARB_position_invariant;"

		LF	"PARAM	wEye			= program.local[0];"

		LF	"ATTRIB	iPos			= vertex.position;"
		LF	"ATTRIB iNorm			= vertex.normal;"
		LF	"ATTRIB	iTan			= vertex.attrib[6];"
		LF	"ATTRIB	iBin			= vertex.attrib[7];"
		LF	"ATTRIB	iTcT			= vertex.texcoord[0];"
		LF	"ATTRIB	iTcL			= vertex.texcoord[1];"

		LF	"OUTPUT	oTcT			= result.texcoord[0];"
		LF	"OUTPUT	oTcL			= result.texcoord[1];"
		LF	"OUTPUT	oEye			= result.texcoord[2];" //eye vector in tangent space

		LF	"TEMP	eye;"

		//pass the texture coords through
		LF	"MOV	oTcT.xy,	iTcT;"
		LF	"MOV	oTcL.xy,	iTcL;"
	
		//get the eye vector
		LF	"SUB	eye.xyz,	wEye,	iPos;"

		//transform the eye vector into tangent space
		LF	"DP3	oEye.x,		iTan,	eye;"
		LF	"DP3	oEye.y,		iBin,	eye;"
		LF	"DP3	oEye.z,		iNorm,	eye;"

		ENDPROG
	},
};

static struct
{
	const char	*prog_name;

	fpFlags_t	flags;
	const char	*text;
} stdfp[SPF_MAX_PROGRAMS] = 
{
	//SPF_BLOOMSELECT
	{
		"bloom: bright pass",
		FPF_NONE,

		BEGINFP

		LF	"OPTION	ARB_precision_hint_fastest;"

		LF	"PARAM	thresh			= program.local[0];" //thresh val, 1 - thresh val

		LF	"ATTRIB	iTc				= fragment.texcoord[0];"

		LF	"OUTPUT	oColor			= result.color;"

		LF	"TEMP	c, tc, tt;"

		/*
			Use bilinear filtering to downsample the original image, which is 4 times
			the size of the target image in each direction.
		*/
		LF	"ADD	tt.xy,	iTc,	0;"//{ 1, 1, 0, 0 };"
		LF	"TEX	c.rgb,	tt,		texture[0],		RECT;"
		
		LF	"ADD	tt.xy,	iTc,	{ 1, -1, 0, 0 };"
		LF	"TEX	tc.rgb,	tt,		texture[0],		RECT;"
		LF	"ADD	c.rgb,	tc,		c;"

		LF	"ADD	tt.xy,	iTc,	{ -1, -1, 0, 0 };"
		LF	"TEX	tc.rgb,	tt,		texture[0],		RECT;"
		LF	"ADD	c.rgb,	tc,		c;"

		LF	"ADD	tt.xy,	iTc,	{ -1, 1, 0, 0 };"
		LF	"TEX	tc.rgb,	tt,		texture[0],		RECT;"
		LF	"ADD	c.rgb,	tc,		c;"

		/*
			Finish the filter and adjust for our threshold value.
		*/

		LF	"MAD_SAT	c.rgb,	c,	0.25,	-thresh.x;"
		LF	"MUL	oColor.rgb,	c,	thresh.y;"
		LF	"MOV	oColor.a,	0;"

		ENDPROG
	},

	//SPF_BLOOMBLUR
	{
		"bloom: blur pass",
		FPF_NONE,

		BEGINFP

		LF	"OPTION	ARB_precision_hint_fastest;"

		LF	"PARAM	kern[14]		= { program.local[0..13] };"

		LF	"ATTRIB	iTc				= fragment.texcoord[0];"

		LF	"OUTPUT	oColor			= result.color;"

		LF	"TEMP	c, tc, tt;"

		//center tap (weight in w of slot 0)
		LF	"TEX	tt.rgb,	iTc,	texture[0],	RECT;"
		LF	"MUL	c.rgb,	tt,		kern[0].w;"

#define SAMPLE( i )										\
		LF	"ADD	tc.xy,	iTc,	kern["#i"];"		\
		LF	"TEX	tt.rgb,	tc,		texture[0],	RECT;"	\
		LF	"MAD	c.rgb,	tt,		kern["#i"].z,	c;"

		SAMPLE( 0 )
		SAMPLE( 1 )
		SAMPLE( 2 )
		SAMPLE( 3 )
		SAMPLE( 4 )
		SAMPLE( 5 )
		SAMPLE( 6 )
		SAMPLE( 7 )
		SAMPLE( 8 )
		SAMPLE( 9 )
		SAMPLE( 10 )
		SAMPLE( 11 )
		SAMPLE( 12 )
		
#undef SAMPLE

		//run the last one straight into oColor
		LF	"ADD	tc.xy,	iTc,	kern[13];"
		LF	"TEX	tt.rgb,	tc,		texture[0],	RECT;"
		LF	"MAD	oColor.rgb,	tt,	kern[13].z,		c;"

		LF	"MOV	oColor.a,		0;"

		ENDPROG
	},

	//SPF_BLOOMCOMBINE
	{
		"bloom: combine pass",
		FPF_NONE,
			
		BEGINFP

		LF	"OPTION	ARB_precision_hint_fastest;"

		LF	"PARAM	combine			= program.local[0];" //bloom int, base int, bloom sat, base sat
		LF	"PARAM	lumVec			= { 0.3, 0.59, 0.11, 0 };"

		LF	"ATTRIB	iTc				= fragment.texcoord[0];"

		LF	"OUTPUT	oColor			= result.color;"

		LF	"TEMP	tc, bloom, base, tmp;"

		LF	"MUL	tc.xy,			iTc,		4;"
		LF	"TEX	bloom.rgb,		iTc,		texture[0], RECT;"
		LF	"TEX	base.rgb,		tc,			texture[1],	RECT;"

		LF	"DP3	bloom.a,		bloom,		lumVec;"
		LF	"LRP	bloom.rgb,		combine.z,	bloom,		bloom.a;"
		LF	"MUL	bloom.rgb,		bloom,		combine.x;"

		LF	"DP3	base.a,			base,		lumVec;"
		LF	"LRP	base.rgb,		combine.w,	base,		base.a;"
		LF	"MUL	base.rgb,		base,		combine.y;"

		LF	"MOV_SAT	tmp.rgb,	bloom;"
		LF	"SUB	tmp.rgb,		1,			tmp;"
		LF	"MUL	base.rgb,		base,		tmp;"

		LF	"ADD	oColor.rgb,		base,		bloom;"
		LF	"MOV	oColor.a,		0;"

		ENDPROG
	},

	//SPF_DIFFUSE
	{
		"per-pixel diffuse lighting",
		FPF_NONE,
					
		BEGINFP

		//normalized light direction in view space
		LF	"PARAM	lDir			= program.local[0];"
		LF	"PARAM	lAmbient		= program.local[1];"
		LF	"PARAM	lDiffuse		= program.local[2];"
		LF	"PARAM	cMat			= program.local[3];"

		LF	"ATTRIB	iTc				= fragment.texcoord[0];"
		LF	"ATTRIB	iNorm			= fragment.texcoord[1];"

		LF	"OUTPUT	oColor			= result.color;"

		LF	"TEMP	n, texcolor, cl;"

		LF	"TEX	texcolor,	iTc,	texture[0],	2D;"

		//n = normalize( iNorm )
		LF	"DP3	n.w,	iNorm,		iNorm;"
		LF	"RSQ	n.w,	n.w;"
		LF	"MUL	n.xyz,	iNorm,		n.w;"

		LF	"DP3	n.w,	n,			lDir;"
		LF	"MAX	n.w,	n.w,		0;"
		LF	"MAD	n.w,	n.w,		cMat.x,		cMat.y;"

		//cl.rgb = ambient and diffuse incoming light
		LF	"MAD	cl.rgb,	lDiffuse,	n.w,	lAmbient;"

		//result = incoming light * texture color, identity alpha
		LF	"MUL	oColor.rgb,	cl,		texcolor;"
		LF	"MOV	oColor.a,			texcolor;"

		ENDPROG
	},

	//SPF_SPEC
	{
		"per-pixel specular lighting",
		FPF_NONE,
					
		BEGINFP

		//normalized light direction in view space
		LF	"PARAM	lDir			= program.local[0];"
		LF	"PARAM	lAmbient		= program.local[1];"
		LF	"PARAM	lDiffuse		= program.local[2];"
		LF	"PARAM	lSpecular		= program.local[3];"	//spec exp in w

		LF	"ATTRIB	iTc				= fragment.texcoord[0];"
		LF	"ATTRIB	iPos			= fragment.texcoord[1];"
		LF	"ATTRIB	iNorm			= fragment.texcoord[2];"

		LF	"OUTPUT	oColor			= result.color;"

		LF	"TEMP	n, h, v, lv, texcolor, cl;"

		LF	"TEX	texcolor,	iTc,	texture[0],	2D;"

		//n = normalize( iNorm )
		LF	"DP3	n.w,	iNorm,		iNorm;"
		LF	"RSQ	n.w,	n.w;"
		LF	"MUL	n.xyz,	iNorm,		n.w;"

		//v = normalize( iPos )
		LF	"DP3	v.w,	iPos,		iPos;"
		LF	"RSQ	v.w,	v.w;"
		LF	"MUL	v.xyz,	iPos,		v.w;"

		//h = normalize( lDir + v )
		LF	"ADD	h.xyz,	lDir,		v;"
		LF	"DP3	h.w,	h,			h;"
		LF	"RSQ	h.w,	h.w;"
		LF	"MUL	h.xyz,	h,			h.w;"

		LF	"DP3	lv.x,	n,			lDir;"
		LF	"DP3	lv.y,	n,			h;"
		LF	"MOV	lv.w,	lSpecular.w;"
		LF	"LIT	lv,		lv;"

		//cl.rgb = ambient and diffuse incoming light
		LF	"MAD	cl.rgb,	lDiffuse,	lv.y,	lAmbient;"
		//cl.rgb = incoming light * texture color
		LF	"MUL	cl.rgb,	cl,			texcolor;"
		//cl.w = specular strength
		LF	"MUL	cl.w,	texcolor.a,	lv.z;"

		//oColor.rgb = lit color + specular color * specular strength
		LF	"MAD	oColor.rgb,	lSpecular,	cl.w,	cl;"
		LF	"MOV	oColor.a,	texcolor;"

		ENDPROG
	},

	//SPF_SPEC_NM
	{
		"per-pixel specular lighting (with normal map)",
		FPF_NONE,
					
		BEGINFP

		//normalized light direction in view space
		LF	"PARAM	lAmbient		= program.local[0];"
		LF	"PARAM	lKeyDiffuse		= program.local[1];"
		LF	"PARAM	lKeySpecular	= program.local[2];"	//spec exp in w
		LF	"PARAM	lFillDiffuse	= program.local[3];"
		LF	"PARAM	lKeyDirection	= program.local[4];"
		LF	"PARAM	lBumpUnpack		= program.local[5];"	//2, -1, z-scale, z-bias

		LF	"ATTRIB	iTc				= fragment.texcoord[0];"
		LF	"ATTRIB iEye			= fragment.texcoord[1];"
		LF	"ATTRIB iTm0			= fragment.texcoord[2];"
		LF	"ATTRIB	iTm1			= fragment.texcoord[3];"
		LF	"ATTRIB	iTm2			= fragment.texcoord[4];"

		LF	"OUTPUT	oColor			= result.color;"

		LF	"TEMP	clDif, norm, tmp, view, cl, cls, h, lv, llv;"

		/*
			texture[0]		diffuse
			texture[1]		normal (spec mask in alpha)
		*/

		LF	"TEX	clDif,		iTc,		texture[0],	2D;"
		LF	"TEX	norm,		iTc,		texture[1],	2D;"

		//unpack normal
		LF	"MAD	tmp.xyz,	norm,		lBumpUnpack.xxzz,	lBumpUnpack.yyww;"
		
		//transform normal into object space
		LF	"DP3	norm.x,		iTm0,		tmp;"
		LF	"DP3	norm.y,		iTm1,		tmp;"
		LF	"DP3	norm.z,		iTm2,		tmp;"

		//normalize normal
		LF	"DP3	norm.w,		norm,		norm;"
		LF	"RSQ	norm.w,		norm.w;"
		LF	"MUL	norm.xyz,	norm,		norm.w;"

		//normalize view
		LF	"DP3	view.w,		iEye,		iEye;"
		LF	"RSQ	view.w,		view.w;"
		LF	"MUL	view.xyz,	iEye,		view.w;"

		//calculate half vector
		LF	"ADD	h.xyz,		view,		lKeyDirection;"
		LF	"DP3	h.w,		h,			h;"
		LF	"RSQ	h.w,		h.w;"
		LF	"MUL	h.xyz,		h,			h.w;"

		//set up and calculate light contribution
		LF	"DP3	lv.x,		norm,		lKeyDirection;"
		LF	"DP3	lv.y,		norm,		h;"
		LF	"MOV	lv.w,		lKeySpecular.w;"
		LF	"LIT	llv,		lv;"

		//diffuse color
		LF	"MOV_SAT	lv.x,	-lv.x;"
		LF	"MAD	cl.rgb,		lv.x,		lFillDiffuse,	lAmbient;"
		LF	"MAD	cl.rgb,		llv.y,		lKeyDiffuse,	cl;"
		LF	"MUL	cl.rgb,		cl,			clDif;"
		
		//specular color
		LF	"MUL_SAT	cls.a,	llv.z,		norm.a;"
		LF	"MAD	oColor.rgb,	cls.a,		lKeySpecular,	cl;"
		LF	"MOV	oColor.a,	clDif.a;"

		ENDPROG
	},

	//SPF_DELUXEMAP
	{
		"deluxe mapped lighting",
		FPF_NONE,

		BEGINFP

		LF	"OPTION	ARB_precision_hint_fastest;"

		LF	"ATTRIB	iTcT			= fragment.texcoord[0];"
		LF	"ATTRIB	iTcL			= fragment.texcoord[1];"
		LF	"ATTRIB	iEye			= fragment.texcoord[2];" //eye vector in tangent space

		LF	"PARAM	lSpecular		= program.local[0];"	//specular sharpness in w

		/*
			texture[0]				normal map with specular mask in a
			texture[1]				light direction
			texture[2]				diffuse map
			texture[3]				light map
		*/

		LF	"OUTPUT	oColor			= result.color;"

		LF	"TEMP	n, v, l, h, lc, dc, lv, cl;"

		//sample normal map
		LF	"TEX	n,			iTcT,	texture[0], 2D;"
		LF	"MAD	n.xyz,		n, 2, -1;"

		//normalize out any filtering errors (make sure to preserve n.a)
		//DO NOT SKIP THIS NORMALIZATION - MASSIVE UGLY AT MIP BOUNDARIES IF NOT PRESENT
		LF	"DP3	l.w,		n, n;"
		LF	"RSQ	l.w,		l.w;"
		LF	"MUL	n.xyz,		n, l.w;"

		//normalize eye
		LF	"DP3	v.w,		iEye, iEye;"
		LF	"RSQ	v.w,		v.w;"
		LF	"MUL	v.xyz,		iEye, v.w;"

		//sample light direction (deluxe map)
		LF	"TEX	l.xyz,		iTcL,	texture[1], 2D;"
		LF	"MAD	l.xyz,		l, 2, -1;"

		//*very* minor artefacts if we skip this step
#if 0
		//normalize out any filtering errors
		LF	"DP3	l.w,		l, l;"
		LF	"RSQ	l.w,		l.w;"
		LF	"MUL	l.xyz,		l, l.w;"
#endif

		//get half way vector
		//MUST normalize here, * 0.5 does *NOT* work
		LF	"ADD	h.xyz,		l, v;"
		LF	"DP3	h.w,		h, h;"
		LF	"RSQ	h.w,		h.w;"
		LF	"MUL	h.xyz,		h, h.w;"

		//get the light color
		LF	"TEX	lc.rgb,		iTcL,	texture[3], 2D;"

		//get the diffuse color
		LF	"TEX	dc.rgb,		iTcT,	texture[2], 2D;"

		//calculate the lighting coefficients
		LF	"DP3	lv.x,		n,			l;"
		LF	"DP3	lv.y,		n,			h;"
		LF	"MOV	lv.w,		lSpecular.w;"
		LF	"LIT	lv,			lv;"

		//color = lightmap * diffuse + lightmap * spec_mul * spec_pow

		//lightmap * spec_mul * spec_pow * spec_mask
		LF	"MUL_SAT	cl.rgb,		lSpecular, lv.z;"
		LF	"MUL	cl.rgb,		cl, lc;"
		LF	"MUL	cl.rgb,		cl, n.a;"

		//lightmap * diffuse
		LF	"MUL	lc.rgb,		lc, lv.y;"	
		LF	"MAD	oColor.rgb,	lc,	dc,	cl;"
	
		LF	"MOV	oColor.a,	0;"

		ENDPROG
	},
};

static struct
{
	GLuint			stdvp[SPV_MAX_PROGRAMS];
	GLuint			stdfp[SPF_MAX_PROGRAMS];

	trackedState_t	*vps;
	vpFlags_t		vpf;

	trackedState_t	*fps;
	fpFlags_t		fpf;
} ss;

static void R_SpResetVertexProgram( trackedState_t *s );
static void R_SpResetFragmentProgram( trackedState_t *s );

static void R_SpLoadStandardVertexPrograms( void );
static void R_SpLoadStandardFragmentPrograms( void );

void R_SpInit()
{
	Com_Memset( &ss, 0, sizeof( ss ) );

	ss.vps = R_StateRegisterCustomTrackedState( R_SpResetVertexProgram );
	ss.fps = R_StateRegisterCustomTrackedState( R_SpResetFragmentProgram );

	R_SpLoadStandardVertexPrograms();
	R_SpLoadStandardFragmentPrograms();
}

void R_SpKill()
{
	if( GLEW_ARB_vertex_program )
		glDeleteProgramsARB( lengthof( ss.stdvp ), ss.stdvp );

	if( GLEW_ARB_fragment_program )
		glDeleteProgramsARB( lengthof( ss.stdfp ), ss.stdfp );

	Com_Memset( &ss, 0, sizeof( ss ) );
}

/*
	Standard program setup.
*/
static bool R_SpLoadProgram( GLenum target, GLuint progNum, const char *program, const char *prog_name )
{
	GLint isNative;
	const GLubyte *pError;
	
	glBindProgramARB( target, progNum );
	glProgramStringARB( target, GL_PROGRAM_FORMAT_ASCII_ARB, strlen( program ), program );
		
	pError = glGetString( GL_PROGRAM_ERROR_STRING_ARB );
	if( pError && strlen( pError ) )
	{
		const char *target_name;
		switch( target )
		{		
		case GL_VERTEX_PROGRAM_ARB:
			target_name = "vertex ";
			break;

		case GL_FRAGMENT_PROGRAM_ARB:
			target_name = "fragment ";
			break;

		default:
			target_name = "";
			break;
		}

		ri.Printf( PRINT_ALL,
#ifdef _DEBUG
			S_COLOR_RED
#endif
			"GL %sprogram '%s' is not supported on this platform; "
			"skipping any related effects. Error details follow:\n"S_COLOR_GRAY"%s\n",
			target_name, prog_name, pError );

		return qfalse;
	}

	glGetProgramivARB( target, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative );

	glBindProgramARB( target, 0 ); 

	if( !isNative )
		return false;

	return true;
}

static void R_SpLoadStandardVertexPrograms( void )
{
	uint i;

	if( !GLEW_ARB_vertex_program )
		return;

	glGenProgramsARB( SPV_MAX_PROGRAMS, ss.stdvp );
	for( i = 0; i < SPV_MAX_PROGRAMS; i++ )
	{
		if(	stdvp[i].flags & VPF_DISABLE ||
			!R_SpLoadProgram( GL_VERTEX_PROGRAM_ARB, ss.stdvp[i], stdvp[i].text, stdvp[i].prog_name ) )
		{
			glDeleteProgramsARB( 1, ss.stdvp + i );
			ss.stdvp[i] = 0;
		}
	}
}

static void R_SpLoadStandardFragmentPrograms( void )
{
	uint i;

	if( !GLEW_ARB_fragment_program )
		return;

	glGenProgramsARB( SPF_MAX_PROGRAMS, ss.stdfp );
	for( i = 0; i < SPF_MAX_PROGRAMS; i++ )
	{
		if(	stdfp[i].flags & FPF_DISABLE ||
			!R_SpLoadProgram( GL_FRAGMENT_PROGRAM_ARB, ss.stdfp[i], stdfp[i].text, stdfp[i].prog_name ) )
		{
			glDeleteProgramsARB( 1, ss.stdfp + i );
			ss.stdfp[i] = 0;
		}
	}
}

bool R_SpIsStandardVertexProgramSupported( stdVertexProgs_t prog )
{
	return ss.stdvp[prog] ? true : false;
}

void R_SpSetStandardVertexProgram( stdVertexProgs_t prog )
{
	R_SpSetVertexProgram( ss.stdvp[prog], stdvp[prog].flags );
}

bool R_SpIsStandardFragmentProgramSupported( stdFragmentProgs_t prog )
{
	return ss.stdfp[prog] ? true : false;
}

void R_SpSetStandardFragmentProgram( stdFragmentProgs_t prog )
{
	R_SpSetFragmentProgram( ss.stdfp[prog], stdfp[prog].flags );
}

/*
	State tracking and management API.
*/

void R_SpSetVertexProgram( GLuint prog, vpFlags_t flags )
{
	if( prog == 0 )
	{
		R_StateRestoreState( ss.vps->name );
		return;
	}

	if( ss.vps->data.asgluint != prog )
	{
		glEnable( GL_VERTEX_PROGRAM_ARB );
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, prog );

		ss.vps->data.asgluint = prog;
	}

	if( ss.vpf != flags )
	{
		if( flags & VPF_POINSIZE )
			glEnable( GL_VERTEX_PROGRAM_POINT_SIZE_ARB );

		ss.vpf = flags;
	}

	R_StateJoinGroup( ss.vps->name );
}

static void R_SpResetVertexProgram( trackedState_t *s )
{
	if( ss.vpf & VPF_POINSIZE )
		glDisable( GL_VERTEX_PROGRAM_POINT_SIZE_ARB );

	glDisable( GL_VERTEX_PROGRAM_ARB );

	s->data.asgluint = 0;
	ss.vpf = VPF_NONE;
}

void R_SpSetFragmentProgram( GLuint prog, fpFlags_t flags )
{
	if( prog == 0 )
	{
		R_StateRestoreState( ss.fps->name );
		return;
	}

	if( prog != ss.fps->data.asgluint )
	{
		glEnable( GL_FRAGMENT_PROGRAM_ARB );
		glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, prog );

		ss.fps->data.asgluint = prog;
	}

	if( flags != ss.fpf )
	{
		//handle flags

		ss.fpf = flags;
	}

	R_StateJoinGroup( ss.fps->name );
}

static void R_SpResetFragmentProgram( trackedState_t *s )
{
	glDisable( GL_FRAGMENT_PROGRAM_ARB );

	ss.fpf = FPF_NONE;
	s->data.asglenum = 0;
}
