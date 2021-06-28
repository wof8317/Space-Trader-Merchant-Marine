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

static const char *convVpCode =
	BEGINVP

	LF	"PARAM	scrSize			= program.local[0];"
	LF	"PARAM	srfCenter		= program.local[1];"

	LF	"ATTRIB	iTcL			= vertex.position;"
	LF	"ATTRIB iNorm			= vertex.normal;"
	LF	"ATTRIB	iTan			= vertex.attrib[6];"
	LF	"ATTRIB	iBin			= vertex.attrib[7];"

	LF	"OUTPUT	oPos			= result.position;"
	LF	"OUTPUT	oTcL			= result.texcoord[0];"
	LF	"OUTPUT	oTm0			= result.texcoord[1];"
	LF	"OUTPUT	oTm1			= result.texcoord[2];"
	LF	"OUTPUT	oTm2			= result.texcoord[3];"

	//pass the texture coords through
	LF	"MOV	oTcL.xy,	iTcL;"

	//pass the tangent basis matrix down
	LF	"MOV	oTm0.xyz,	iTan;"
	LF	"MOV	oTm1.xyz,	iBin;"
	LF	"MOV	oTm2.xyz,	iNorm;"

	//scale and bias the incoming texture coords into clip space
	LF	"MAD	oPos.xy,	iTcL,	2, -1;"
	LF	"MOV	oPos.zw,	{ 0, 0, 0, 1 };"

	ENDPROG;

static const char *convFpCode =
	BEGINFP

	LF	"OPTION	ARB_precision_hint_nicest;"

	LF	"ATTRIB	iTcL			= fragment.texcoord[0];"
	LF	"ATTRIB	iTm0			= fragment.texcoord[1];"
	LF	"ATTRIB	iTm1			= fragment.texcoord[2];"
	LF	"ATTRIB	iTm2			= fragment.texcoord[3];"

	LF	"OUTPUT	oColor			= result.color;"

	LF	"TEMP	tm0, tm1, tm2, t, l;"

	/*
		texture[0]				deluxe map
	*/

	LF	"DP3	tm0.w,		iTm0,	iTm0;"
	LF	"RSQ	tm0.w,		tm0.w;"
	LF	"MUL	tm0.xyz,	iTm0,	tm0.w;"

	LF	"DP3	tm1.w,		iTm1,	iTm1;"
	LF	"RSQ	tm1.w,		tm1.w;"
	LF	"MUL	tm1.xyz,	iTm1,	tm1.w;"

	LF	"DP3	tm2.w,		iTm2,	iTm2;"
	LF	"RSQ	tm2.w,		tm2.w;"
	LF	"MUL	tm2.xyz,	iTm2,	tm2.w;"

	//sample light direction (deluxe map)
	LF	"TEX	t.xyz,		iTcL,	texture[0], 2D;"
	LF	"MAD	t.xyz,		t, 2, -1;"

	//transform into tangent space
	LF	"DP3	l.x,		tm0,	t;"
	LF	"DP3	l.y,		tm1,	t;"
	LF	"DP3	l.z,		tm2,	t;"

	//normalize out any filtering/tangent basis errors
	LF	"DP3	l.w,		l, l;"
	LF	"RSQ	l.w,		l.w;"
	LF	"MUL	l.xyz,		l, l.w;"

	LF	"MAD	oColor.rgb,	l, 0.5, 0.5;"
	LF	"MOV	oColor.a,	1;"

	ENDPROG;

static struct
{
	GLuint	vp, fp;
} conv;

static void R_InitConverter( void )
{
	const GLubyte *pError;

	glGenProgramsARB( 1, &conv.vp );
	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, conv.vp );
	glProgramStringARB( GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
		strlen( convVpCode ), convVpCode );

	pError = glGetString( GL_PROGRAM_ERROR_STRING_ARB );
	if( pError && pError[0] )
		__debugbreak();

	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, 0 );

	glGenProgramsARB( 1, &conv.fp );
	glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, conv.fp );
	glProgramStringARB( GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
		strlen( convFpCode ), convFpCode );

	pError = glGetString( GL_PROGRAM_ERROR_STRING_ARB );
	if( pError && pError[0] )
		__debugbreak();

	glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, 0 );
}

//from tr_image.c
void Upload32( unsigned *data, qboolean lightMap, image_t *image, imageLoadFlags_t flags );

void R_RemapDeluxeImages( world_t *world )
{
	int i;

	if( !tr.deluxemaps[0] )
		return;

	R_InitConverter();

	R_StateSetActiveTmuUntracked( GL_TEXTURE0 );
	R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );

	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glPushClientAttrib( GL_CLIENT_ALL_ATTRIB_BITS );

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glEnableVertexAttribArrayARB( 6 );
	glEnableVertexAttribArrayARB( 7 );

	glClearColor( 0, 0, 0, 0 );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_STENCIL_TEST );
	glDisable( GL_ALPHA_TEST );
	glDisable( GL_BLEND );
	glDisable( GL_CULL_FACE );
	glDisable( GL_MULTISAMPLE );
	glEnable( GL_POLYGON_SMOOTH );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glEnable( GL_VERTEX_PROGRAM_ARB );
	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, conv.vp );
	glEnable( GL_FRAGMENT_PROGRAM_ARB );
	glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, conv.fp );

	glEnable( GL_TEXTURE_2D );

	for( i = 0; i < tr.numLightmaps; i++ )
	{
		int j, y;
		unsigned *buf0, *buf1;

		image_t *img = tr.deluxemaps[i];
		glBindTexture( GL_TEXTURE_2D, img->texnum );

		glClear( GL_COLOR_BUFFER_BIT );

		glViewport( 0, img->uploadHeight, img->uploadWidth, img->uploadHeight );

		glDisable( GL_VERTEX_PROGRAM_ARB );
		glDisable( GL_FRAGMENT_PROGRAM_ARB );

		glBegin( GL_QUADS );
		{
			glTexCoord2f( 0, 0 );
			glVertex2f( -1, -1 );

			glTexCoord2f( 0, 1 );
			glVertex2f( -1, 1 );

			glTexCoord2f( 1, 1 );
			glVertex2f( 1, 1 );

			glTexCoord2f( 1, 0 );
			glVertex2f( 1, -1 );
		}
		glEnd();

		glEnable( GL_VERTEX_PROGRAM_ARB );
		glEnable( GL_FRAGMENT_PROGRAM_ARB );

		glViewport( 0, 0, img->uploadWidth, img->uploadHeight );

		glProgramLocalParameter4fARB( GL_VERTEX_PROGRAM_ARB, 0,
			img->uploadWidth, img->uploadHeight,
			1.0F / img->uploadWidth, 1.0F / img->uploadHeight );

		for( j = 0; j < world->numsurfaces; j++ )
		{
			const msurface_t *srf = world->surfaces + j;
			const shader_t *shader = srf->shader;
			const msurface_ex_t *exsrf = srf->redirect;
			
			if( !shader->stages[0] || !shader->stages[0]->active )
				continue;
			if( shader->stages[0]->deluxeMap != img )
				continue;

			if( !exsrf )
				continue;

			glVertexPointer( 2, GL_FLOAT, sizeof( drawVert_ex_t ),
				&exsrf->verts[0].uvL );
			glNormalPointer( GL_FLOAT, sizeof( drawVert_ex_t ),
				&exsrf->verts[0].norm );
			glVertexAttribPointerARB( 6, 3, GL_FLOAT, GL_FALSE,
				sizeof( drawVert_ex_t ), &exsrf->verts[0].tan );
			glVertexAttribPointerARB( 7, 3, GL_FLOAT, GL_FALSE,
				sizeof( drawVert_ex_t ), &exsrf->verts[0].bin );

			glDrawElements( exsrf->primType, exsrf->numIndices,
				GL_UNSIGNED_SHORT, exsrf->indices );
		}

		glFinish();
		
		buf0 = (unsigned*)ri.Hunk_AllocateTempMemory( img->uploadWidth * img->uploadHeight * 4 );
		buf1 = (unsigned*)ri.Hunk_AllocateTempMemory( img->uploadWidth * img->uploadHeight * 4 );

		//can't just copy to the texture since we
		//need the custom mipmap generator
		glReadPixels( 0, 0, img->uploadWidth, img->uploadHeight,
			GL_RGBA, GL_UNSIGNED_BYTE, buf0 );

#define DELUXEL( buf, x, y ) ((byte*)(buf) + (((y) * img->uploadWidth + (x)) * 4))

		Com_Memcpy( buf1, buf0, img->uploadWidth * img->uploadHeight * 4 );

		for( j = 0; j < 4; j++ )
		{
			for( y = 0; y < img->uploadHeight; y++ )
			{
				int x;

				for( x = 0; x < img->uploadWidth; x++ )
				{
					static int neighbors[8][2] =
					{
						{ 0, 1 },
						{ 1, 1 },
						{ 1, 0 },
						{ 1, -1 },
						{ 0, -1 },
						{ -1, -1 },
						{ -1, 0 },
						{ -1, 1 }
					};

					int i;
					int sum[3], c;

					byte *cIn = DELUXEL( buf0, x, y );
					byte *cOut = DELUXEL( buf1, x, y );

					cOut[3] = cIn[3];

					if( cIn[2] )
					{
						//if it has some Z value
						//then it's already good

						cOut[0] = cIn[0];
						cOut[1] = cIn[1];
						cOut[2] = cIn[2];

						continue;
					}

					c = 0;
					sum[0] = sum[1] = sum[2] = 0;

					for( i = 0; i < lengthof( neighbors ); i++ )
					{
						int nx = x + neighbors[i][0];
						int ny = y + neighbors[i][1];

						if( nx >= 0 && nx < img->uploadWidth &&
							ny >= 0 && ny < img->uploadHeight )
						{
							byte *n = DELUXEL( buf0, nx, ny );

							if( !n[2] )
								continue;

							sum[0] += n[0];
							sum[1] += n[1];
							sum[2] += n[2];

							c++;
						}
					}

					if( c )
					{
						cOut[0] = sum[0] / c;
						cOut[1] = sum[1] / c;
						cOut[2] = sum[2] / c;
					}
				}
			}

			Com_Memcpy( buf0, buf1, img->uploadWidth * img->uploadHeight * 4 );
		}

		for( y = 0; y < img->uploadHeight; y++ )
		{
			int x;

			for( x = 0; x < img->uploadWidth; x++ )
			{
				byte *d = DELUXEL( buf1, x, y );

				if( !d[2] )
				{
					d[0] = 0;
					d[1] = 0;
					d[2] = 0xFF;
				}
			}
		}

		//write it out to file
		{
			int size;
			char path[MAX_QPATH];
			byte *out_buf;
			
			Com_sprintf( path, sizeof( path ), "deluxe/%s/dm_%04d.tga",
				world->baseName, i );

			size = 18 + img->uploadWidth * img->uploadHeight * 3;
			out_buf = (byte*)ri.Hunk_AllocateTempMemory( size );

		   	Com_Memset( out_buf, 0, 18 );
			out_buf[2] = 2;		// uncompressed type
			out_buf[12] = img->uploadWidth & 255;
			out_buf[13] = img->uploadWidth >> 8;
			out_buf[14] = img->uploadHeight & 255;
			out_buf[15] = img->uploadHeight >> 8;
			out_buf[16] = 24;	// pixel size
			out_buf[17] = 0x20; // reverse row order

			for( y = 0; y < img->uploadHeight; y++ )
			{
				int x;
				for( x = 0; x < img->uploadWidth; x++ )
				{
					byte *d = DELUXEL( buf1, x, y );
					out_buf[18 + (y * img->uploadWidth + x) * 3 + 0] = d[2];
					out_buf[18 + (y * img->uploadWidth + x) * 3 + 1] = d[1];
					out_buf[18 + (y * img->uploadWidth + x) * 3 + 2] = d[0];
				}
			}
			
			ri.FS_WriteFile( path, out_buf, size );
			
			ri.Hunk_FreeTempMemory( out_buf );
		}

#undef DELUXEL

		Upload32( buf1, qfalse, img, ILF_VECTOR_SB3 );

#ifdef _DEBUG
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, img->texnum );

		glViewport( img->uploadWidth, 0, img->uploadWidth, img->uploadHeight );

		glDisable( GL_VERTEX_PROGRAM_ARB );
		glDisable( GL_FRAGMENT_PROGRAM_ARB );

		glBegin( GL_QUADS );
		{
			glTexCoord2f( 0, 0 );
			glVertex2f( -1, -1 );

			glTexCoord2f( 0, 1 );
			glVertex2f( -1, 1 );

			glTexCoord2f( 1, 1 );
			glVertex2f( 1, 1 );

			glTexCoord2f( 1, 0 );
			glVertex2f( 1, -1 );
		}
		glEnd();

		glEnable( GL_VERTEX_PROGRAM_ARB );
		glEnable( GL_FRAGMENT_PROGRAM_ARB );

		GLimp_EndFrame();
#endif

		ri.Hunk_FreeTempMemory( buf1 );
		ri.Hunk_FreeTempMemory( buf0 );
	}

	glPopClientAttrib();
	glPopAttrib();

	glDeleteProgramsARB( 1, &conv.vp );
	glDeleteProgramsARB( 1, &conv.fp );
}