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

//from tr_image_dds.c
image_t* R_LoadDDSImage( const char* name, qboolean mipmap );

static void LoadTGA( const char *name, byte **pic, int *width, int *height );

static byte			 s_intensitytable[256];
static unsigned char s_gammatable[256];

#define FILE_HASH_SIZE		1024
static	image_t*		hashTable[FILE_HASH_SIZE];

/*
** R_GammaCorrect
*/
void R_GammaCorrect( byte *buffer, int bufSize ) {
	int i;

	for ( i = 0; i < bufSize; i++ ) {
		buffer[i] = s_gammatable[buffer[i]];
	}
}

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname )
{
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while( fname[i] != '\0' )
	{
		letter = (char)tolower( fname[i] );

		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
	return hash;
}

/*
===============
R_SumOfUsedImages
===============
*/
int R_SumOfUsedImages( void ) {
	int	total;
	int i;

	total = 0;
	for ( i = 0; i < tr.numImages; i++ ) {
		if ( tr.images[i]->frameUsed == tr.frameCount ) {
			total += tr.images[i]->uploadWidth * tr.images[i]->uploadHeight;
		}
	}

	return total;
}

/*
===============
R_ImageList_f
===============
*/
void Q_EXTERNAL_CALL R_ImageList_f( void )
{
	int		i;
	image_t	*image;
	int		texels;
	const char *yesno[] = {
		"no ", "yes"
	};

	ri.Printf (PRINT_ALL, "\n      -w-- -h-- -mm- -if-- --name-------\n");
	texels = 0;

	for ( i = 0 ; i < tr.numImages ; i++ ) {
		image = tr.images[ i ];

		texels += image->uploadWidth*image->uploadHeight;
		ri.Printf (PRINT_ALL,  "%4i: %4i %4i  %s   ",
			i, image->uploadWidth, image->uploadHeight, yesno[image->mipmap] );
		switch ( image->internalFormat )
		{		
		case 1:
			ri.Printf( PRINT_ALL, "I    " );
			break;
		
		case 2:
			ri.Printf( PRINT_ALL, "IA   " );
			break;
		
		case 3:
			ri.Printf( PRINT_ALL, "RGB  " );
			break;
		
		case 4:
			ri.Printf( PRINT_ALL, "RGBA " );
			break;
		
		case GL_RGBA8:
			ri.Printf( PRINT_ALL, "RGBA8" );
			break;
		
		case GL_RGB8:
			ri.Printf( PRINT_ALL, "RGB8" );
			break;
		
		case GL_RGB4_S3TC:
			ri.Printf( PRINT_ALL, "S3TC " );
			break;

		case GL_RGBA4:
			ri.Printf( PRINT_ALL, "RGBA4" );
			break;

		case GL_RGB5:
			ri.Printf( PRINT_ALL, "RGB5 " );
			break;

		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			ri.Printf( PRINT_ALL, "DXT1 " );
			break;

		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			ri.Printf( PRINT_ALL, "DXT1a " );
			break;

		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			ri.Printf( PRINT_ALL, "DXT3 " );
			break;

		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			ri.Printf( PRINT_ALL, "DXT5 " );
			break;

		default:
			ri.Printf( PRINT_ALL, "???? " );
			break;
		}
		
		ri.Printf( PRINT_ALL, " %s\n", image->imgName );
	}
	ri.Printf (PRINT_ALL, " ---------\n");
	ri.Printf (PRINT_ALL, " %i total texels (not including mipmaps)\n", texels);
	ri.Printf (PRINT_ALL, " %i total images\n\n", tr.numImages );
}

/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void R_LightScaleTexture (unsigned *in, int inwidth, int inheight, qboolean only_gamma )
{
	if ( only_gamma )
	{
		if ( !glConfig.deviceSupportsGamma )
		{
			int		i, c;
			byte	*p;

			p = (byte *)in;

			c = inwidth*inheight;
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	}
	else
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;

		if ( glConfig.deviceSupportsGamma )
		{
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_intensitytable[p[0]];
				p[1] = s_intensitytable[p[1]];
				p[2] = s_intensitytable[p[2]];
			}
		}
		else
		{
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_gammatable[s_intensitytable[p[0]]];
				p[1] = s_gammatable[s_intensitytable[p[1]]];
				p[2] = s_gammatable[s_intensitytable[p[2]]];
			}
		}
	}
}

typedef void (*mipmapFunc_t)( byte * RESTRICT out, uint wo, uint ho, const byte * RESTRICT lv0, uint w0, uint h0, const byte * RESTRICT lvp, uint wp, uint hp );

static void R_MipVectorSB3(
	byte * RESTRICT out, uint wo, uint ho,			//output level
	const byte * RESTRICT lv0, uint w0, uint h0,	//base level
	const byte * RESTRICT lvp, uint wp, uint hp		//prior level
	)
{
	uint y;

	for( y = 0; y < ho; y++ )
	{
		uint x;
		for( x = 0; x < wo; x++ )
		{
			uint i;
			uint ix[4], iy[4];

			vec3_t taps[4], sum;

			byte wTaps[4];
			uint wSum;

			byte *oV = out + (y * wo + x) * 4;

			//get the tap points
			ix[0] = x * 2;
			iy[0] = y * 2;

			ix[1] = x * 2 + 1;
			iy[1] = y * 2;

			ix[2] = x * 2;
			iy[2] = y * 2 + 1;

			ix[3] = x * 2 + 1;
			iy[3] = y * 2 + 1;

			//clamp to edge
			for( i = 0;	i < 4; i++ )
			{
				if( ix[i] >= wp ) ix[i] = wp - 1;
				if( iy[i] >= hp ) iy[i] = hp - 1;
			}

			//sample the previous level
			for( i = 0; i < 4; i++ )
			{
				uint j;
				const byte *iV = lvp + (iy[i] * wp + ix[i]) * 4;

				for( j = 0; j < 3; j++ )
					taps[i][j] = (float)iV[j] * (2.0F / 0xFF) - 1.0F;

				if( VectorNormalize( taps[i] ) < 0.0001F )
					VectorSet( taps[i], 0, 0, 1 );

				wTaps[i] = iV[3];
			}

			//add them up
			VectorCopy( taps[0], sum );
			for( i = 1; i < 4; i++ )
				VectorAdd( sum, taps[i], sum );

			wSum = wTaps[0];
			for( i = 1; i < 4; i++ )
				wSum += wTaps[i];

			//complete the average
			if( VectorNormalize( sum ) < 0.0001F )
				VectorSet( sum, 0, 0, 1 );
			
			wSum /= 4;

			//write it back
			for( i = 0; i < 3; i++ )
				oV[i] = (byte)((sum[i] + 1.0F) * (0xFF / 2.0F));

			oV[3] = (byte)wSum;
		}
	}
}

static void R_MipColor4(
	byte * RESTRICT out, uint wo, uint ho,			//output level
	const byte * RESTRICT lv0, uint w0, uint h0,	//base level
	const byte * RESTRICT lvp, uint wp, uint hp		//prior level
	)
{
	uint y;

	for( y = 0; y < ho; y++ )
	{
		uint x;
		for( x = 0; x < wo; x++ )
		{
			uint i;
			uint ix[4], iy[4];

			byte taps[4][4];

			byte *oV = out + (y * wo + x) * 4;

			//get the tap points
			ix[0] = x * 2;
			iy[0] = y * 2;

			ix[1] = x * 2 + 1;
			iy[1] = y * 2;

			ix[2] = x * 2;
			iy[2] = y * 2 + 1;

			ix[3] = x * 2 + 1;
			iy[3] = y * 2 + 1;

			//clamp to edge
			for( i = 0;	i < 4; i++ )
			{
				if( ix[i] >= wp ) ix[i] = wp - 1;
				if( iy[i] >= hp ) iy[i] = hp - 1;
			}

			//sample the previous level
			for( i = 0; i < 4; i++ )
			{
				uint j;
				const byte *iV = lvp + (iy[i] * wp + ix[i]) * 4;

				for( j = 0; j < 4; j++ )
					taps[i][j] = iV[j];
			}

			//add them up
			for( i = 1; i < 4; i++ )
			{
				uint j;

				for( j = 0; j < 4; j++ )
					taps[0][j] += taps[i][j];
			}

			//complete the average and write it back
			for( i = 0; i < 4; i++ )
				oV[i] = taps[0][i] / 4;
		}
	}
}

static void R_GenerateMips( const image_t *image, mipmapFunc_t mipFunc, const byte *data )
{
	uint i, w, h;
	byte *bufs[2];

	w = image->width;
	h = image->height;

	if( !w || !h )
		return;

	bufs[0] = (byte*)ri.Hunk_AllocateTempMemory( (w / 2) * (h / 2) * 4 );
	bufs[1] = (byte*)ri.Hunk_AllocateTempMemory( (w / 4) * (h / 4) * 4 );

	i = 0;
	do
	{
		uint wp = w;
		uint hp = h;

		if( (w /= 2) == 0 ) w = 1;
		if( (h /= 2) == 0 ) h = 1;
		
		mipFunc( bufs[i & 1], w, h, data, image->width, image->height, i ? bufs[(i - 1) &1] : data, wp, hp );
		glTexImage2D( image->texTarget, i + 1, image->internalFormat, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, bufs[i & 1] );

		i++;
	} while( w > 1 || h > 1 );

	ri.Hunk_FreeTempMemory( bufs[1] );
	ri.Hunk_FreeTempMemory( bufs[0] );
}

void Upload32( unsigned *data, qboolean lightMap, image_t *image, imageLoadFlags_t flags )
{
	int samples;
	int i, c;
	byte *scan;
	GLenum internalFormat = GL_RGB;
	mipmapFunc_t mipFunc = NULL;
	qboolean pow2 = (image->width & (image->width - 1) || image->height & (image->height - 1)) ? qfalse : qtrue;
	
	//
	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	//
	c = image->width * image->height;
	scan = (byte*)data;
	samples = 3;
	if( !lightMap )
	{
		for( i = 0; i < c; i++ )
			if( scan[i * 4 + 3] != 0xFF ) 
			{
				samples = 4;
				break;
			}

		// select proper internal format
		if( samples == 3 )
		{
			internalFormat = GL_RGB8;
			image->hasAlpha = qfalse;
		}
		else if( samples == 4 )
		{
			internalFormat = GL_RGBA8;
			image->hasAlpha = qtrue;
		}
	}
	else
		internalFormat = GL_RGB8;

	if( pow2 )
	{
		image->texTarget = GL_TEXTURE_2D;
		image->addrMode = TAM_Normalized;
	}
	else
	{
		if( GLEW_ARB_texture_non_power_of_two )
		{
			image->texTarget = GL_TEXTURE_2D;
			image->addrMode = TAM_Normalized;
		}
		else
		{
			image->texTarget = R_StateGetRectTarget();
			image->addrMode = TAM_Dimensions;
		}
	}

	R_StateSetTexture( image, GL_TEXTURE0_ARB );

	if( image->mipmap && pow2 )
	{
		if( flags & ILF_VECTOR_SB3 )
			mipFunc = R_MipVectorSB3;
		else if( GLEW_SGIS_generate_mipmap )
			glTexParameteri( image->texTarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
		else
			mipFunc = R_MipColor4;
	}

	if( image->mipmap && !pow2 && r_clobberNPOTtex->integer )
	{
		//clobber the image data up a bit
		int c;
		byte *pd = (byte*)ri.Hunk_AllocateTempMemory( image->width * image->height * 3 );
		for( c = image->width * image->height; c-- > 0; )
		{
			pd[c * 3 + 0] = 0xFF;
			pd[c * 3 + 1] = 0x00;
			pd[c * 3 + 2] = 0xEF;
		}

		glTexImage2D( image->texTarget, 0, internalFormat, image->width, image->height,
			0, GL_RGB, GL_UNSIGNED_BYTE, pd );

		ri.Hunk_FreeTempMemory( pd );
	}
	else
		glTexImage2D( image->texTarget, 0, internalFormat, image->width, image->height,
			0, GL_RGBA, GL_UNSIGNED_BYTE, data );

	image->uploadWidth = image->width;
	image->uploadHeight = image->height;
	image->internalFormat = internalFormat;

	if( mipFunc )
		R_GenerateMips( image, mipFunc, (byte*)data );

	R_StateSetDefaultTextureModesUntracked( image->texTarget, image->mipmap, GL_TEXTURE0_ARB );

	R_StateSetTexture( 0, GL_TEXTURE0_ARB );

	GL_CheckErrors();
}

image_t* R_MakeImage( const char *name )
{
	image_t* ret;
	long hash;

	ret = tr.images[tr.numImages] = (image_t*)ri.Hunk_Alloc( sizeof( image_t ), h_low );
	Com_Memset( ret, 0, sizeof( image_t ) );
	glGenTextures( 1, &ret->texnum );
	tr.numImages++;

	COM_StripExtension( name, ret->imgName, sizeof( ret->imgName ) );
	hash = generateHashValue( ret->imgName );
	ret->next = hashTable[ hash ];
	hashTable[ hash ] = ret;

	return ret;
}

/*
================
R_CreateImage

image_t is created here and in R_MakeImage (on behalf of R_LoadDDSImage)
================
*/
image_t *R_CreateImage( const char *name, const byte *pic, int width, int height, qboolean mipmap, imageLoadFlags_t flags )
{
	image_t		*image;
	qboolean	isLightmap = qfalse;
	long		hash;

	if( strlen( name ) >= MAX_QPATH )
		ri.Error (ERR_DROP, "R_CreateImage: \"%s\" is too long\n", name);
	
	if( strncmp( name, "*lightmap", 9 ) == 0 )
		isLightmap = qtrue;

	if( tr.numImages == MAX_DRAWIMAGES )
		ri.Error( ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit\n");

	//this code is replicated in R_MakeImage:
	image = tr.images[tr.numImages] = (image_t*)ri.Hunk_Alloc( sizeof( image_t ), h_low );
	Com_Memset( image, 0, sizeof( image_t ) );
	glGenTextures( 1, &image->texnum );
	tr.numImages++;

	image->mipmap = mipmap;

	strcpy( image->imgName, name );

	image->width = width;
	image->height = height;

	Upload32( (uint*)pic, isLightmap, image, flags );

	R_StateForceRestoreState( TSN_Texture0 );

	hash = generateHashValue(name);
	image->next = hashTable[hash];
	hashTable[hash] = image;

	return image;
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

/*
=============
LoadTGA
=============
*/
static void LoadTGA ( const char *name, byte **pic, int *width, int *height)
{
	unsigned columns, rows;
	byte *pixbuf;
	TargaHeader targa_header;
	byte buf_t[2048 * 4];
	byte *buf_p;
	byte *targa_rgba;
	fileHandle_t fh;
	uint row;
	int nc;

	*pic = NULL;

	// load the file
	if( ri.FS_FOpenFile( name, &fh, FS_READ ) < 0 )
		return;

	buf_p = buf_t;

	ri.FS_Read( buf_p, 18, fh );

	targa_header.id_length = buf_p[0];
	targa_header.colormap_type = buf_p[1];
	targa_header.image_type = buf_p[2];
	
	targa_header.colormap_index = *(short*)(buf_p + 3);
	targa_header.colormap_length = *(short*)(buf_p + 5);
	targa_header.colormap_size = buf_p[7];
	targa_header.x_origin = *(short*)(buf_p + 8);
	targa_header.y_origin = *(short*)(buf_p + 10);
	targa_header.width = *(short*)(buf_p + 12);
	targa_header.height = *(short*)(buf_p + 14);
	targa_header.pixel_size = buf_p[16];
	targa_header.attributes = buf_p[17];

	targa_header.colormap_index = LittleShort( targa_header.colormap_index );
	targa_header.colormap_length = LittleShort( targa_header.colormap_length );
	targa_header.x_origin = LittleShort( targa_header.x_origin );
	targa_header.y_origin = LittleShort( targa_header.y_origin );
	targa_header.width = LittleShort( targa_header.width );
	targa_header.height = LittleShort( targa_header.height );

	if( targa_header.image_type != 2 &&
		targa_header.image_type != 3 &&
		targa_header.image_type != 10 )
	{
		ri.Error (ERR_DROP, "LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n");
	}

	if ( targa_header.colormap_type != 0 )
	{
		ri.Error( ERR_DROP, "LoadTGA: colormaps not supported\n" );
	}

	if( targa_header.image_type == 2 &&
		(targa_header.pixel_size != 32 && targa_header.pixel_size != 24) )
	{
		ri.Error( ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n" );
	}

	if( targa_header.width > 2048 )
		ri.Error( ERR_DROP, "LoadTGA: Image wider than 2048.\n" );

	columns = targa_header.width;
	rows = targa_header.height;

	if( width )
		*width = columns;
	if( height )
		*height = rows;

	if( !columns || !rows )
	{
		ri.Error( ERR_DROP, "LoadTGA: %s has an invalid image size\n", name );
	}

	targa_rgba = (byte*)ri.Hunk_AllocateTempMemory( rows * columns * 4 );
	*pic = targa_rgba;

	if( targa_header.id_length != 0 )
	{
		if( targa_header.id_length > sizeof( buf_t ) )
			ri.Error( ERR_DROP, "LoadTGA: %s has a ridiculously large image comment\n", name );

		//we do this because FS_Seek is broken
		ri.FS_Read( buf_t, targa_header.id_length, fh );
	}
	
	nc = targa_header.pixel_size / 8;

	// Uncompressed RGB or gray scale image
	for( row = 0; row < rows; row++ )
	{
		uint column;

		buf_p = buf_t;
		ri.FS_Read( buf_p, columns * nc, fh );

		pixbuf = targa_rgba +
			((targa_header.attributes & 0x20) ? row : (rows - row - 1)) * columns * 4;

		for( column = 0; column < columns; column++ ) 
		{
			pixbuf[3] = 0xFF;

			switch( nc )
			{
			case 4:
				pixbuf[3] = buf_p[3];
				//fallthrough

			case 3:
				pixbuf[0] = buf_p[2];
				pixbuf[1] = buf_p[1];
				pixbuf[2] = buf_p[0];
				break;

			case 1:
				pixbuf[0] = buf_p[0];
				pixbuf[1] =	buf_p[0];
				pixbuf[2] = buf_p[0];
				break;

			NO_DEFAULT;
			}

			pixbuf += 4;
			buf_p += nc;
		}
	}

	ri.FS_FCloseFile( fh );
}

void R_LoadImage( const char *name, byte **pic, int *width, int *height )
{
	size_t len;

	*pic = NULL;
	*width = 0;
	*height = 0;

	len = strlen( name );
	if( len < 5 )
		return;

	if( Q_stricmp( name + len - 4, ".tga" ) == 0 )
		LoadTGA( name, pic, width, height );
}

/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
image_t* R_FindImageFile( const char *name, qboolean mipmap, imageLoadFlags_t flags )
{
	long hash;
	image_t	*image;
	char basePath[MAX_QPATH];
	int width, height;
	byte *pic;

	if( !name )
		return NULL;

	if( Q_stricmp( name, "$whiteimage.tga" ) == 0 )
		name = "*white";
	else if( Q_stricmp( name, "$whiteimage" ) == 0 )
		name = "*white";
	else if( Q_stricmp( name, "*white.tga" ) == 0 )
		name = "*white";
	else if( Q_stricmp( name, "*black.tga" ) == 0 )
		name = "*black";

	COM_StripExtension( name, basePath, sizeof( basePath ) );

	hash = generateHashValue( basePath );

	for( image = hashTable[hash]; image; image = image->next )
	{
		if( !strcmp( basePath, image->imgName ) )
		{
			if( strcmp( basePath, "*white" ) != 0 && strcmp( basePath, "*black" ) != 0 )
			{
				//the black and white images can be used with any set of parms
				//but other mismatches are errors
				if( image->mipmap != mipmap )
					ri.Printf( PRINT_DEVELOPER, "WARNING: reused image %s with mixed mipmap parm\n", name );
			}
			return image;
		}
	}

	//.DDS is next in line
	if( basePath[0] )
	{
		image_t* img;
		char path[MAX_QPATH];
		Com_sprintf( path, sizeof( path ), "%s.dds", basePath );

		img = R_LoadDDSImage( path, mipmap );
		if( img )
			return img;
	}

	// load the pic from disk

	R_LoadImage( name, &pic, &width, &height );

	if( pic )
	{
		image = R_CreateImage( basePath, pic, width, height, mipmap, flags );
		ri.Hunk_FreeTempMemory( pic );
	}
	else
	{
#ifdef PRINT_CONSOLE_SPAM
		ri.Printf( PRINT_WARNING, "Can't find texture: %s\n", name );
#endif

		if( flags & ILF_ALLOW_DEFAULT )
		{
			if( flags & ILF_VECTOR_SB3 )
				return tr.defNormMap;
			else
				return tr.imageNotFoundImage;
		}

		return NULL;
	}
	
	return image;
}


qhandle_t Q_EXTERNAL_CALL RE_RegisterImage( const char *path, qboolean mipMap )
{
	return (qhandle_t)R_FindImageFile( path, mipMap ? qtrue : qfalse, ILF_ALLOW_DEFAULT );
}

/*
================
R_CreateDlightImage
================
*/
#define	DLIGHT_SIZE	16
static void R_CreateDlightImage( void )
{
	byte	data[DLIGHT_SIZE][DLIGHT_SIZE][4];
	int		b;
	int		x;

	// make a centered inverse-square falloff blob for dynamic lighting
	for( x = 0; x < DLIGHT_SIZE; x++ )
	{
		int y;
		for( y = 0; y < DLIGHT_SIZE; y++ )
		{
			float d;

			d = ( DLIGHT_SIZE/2 - 0.5f - x ) * ( DLIGHT_SIZE/2 - 0.5f - x ) +
				( DLIGHT_SIZE/2 - 0.5f - y ) * ( DLIGHT_SIZE/2 - 0.5f - y );
			b = (int)(4000 / d);
			if( b > 0xFF )
				b = 0xFF;
			else if( b < 75 )
				b = 0;

			data[y][x][0] = 
			data[y][x][1] = 
			data[y][x][2] = (byte)b;
			data[y][x][3] = 0xFF;
		}
	}

	tr.dlightImage = R_CreateImage("*dlight", (byte *)data, DLIGHT_SIZE, DLIGHT_SIZE, qfalse, ILF_NONE );
}


/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable( void ) {
	int		i;
	float	d;
	float	exp;
	
	exp = 0.5;

	for ( i = 0 ; i < FOG_TABLE_SIZE ; i++ ) {
		d = pow ( (float)i/(FOG_TABLE_SIZE-1), exp );

		tr.fogTable[i] = d;
	}
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float	R_FogFactor( float s, float t ) {
	float	d;

	s -= 1.0/512;
	if ( s < 0 ) {
		return 0;
	}
	if ( t < 1.0/32 ) {
		return 0;
	}
	if ( t < 31.0/32 ) {
		s *= (t - 1.0f/32.0f) / (30.0f/32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if ( s > 1.0 ) {
		s = 1.0;
	}

	d = tr.fogTable[ (int)(s * (FOG_TABLE_SIZE-1)) ];

	return d;
}

/*
================
R_CreateFogImage
================
*/
#define	FOG_S	256
#define	FOG_T	32
static void R_CreateFogImage( void ) {
	int		x,y;
	byte	*data;
	float	g;
	float	d;
	float	borderColor[4];

	data = (byte*)ri.Hunk_AllocateTempMemory( FOG_S * FOG_T * 4 );

	g = 2.0;

	// S is distance, T is depth
	for (x=0 ; x<FOG_S ; x++) {
		for (y=0 ; y<FOG_T ; y++) {
			d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

			data[(y*FOG_S+x)*4+0] = 
			data[(y*FOG_S+x)*4+1] = 
			data[(y*FOG_S+x)*4+2] = 0xFF;
			data[(y*FOG_S+x)*4+3] = (byte)(d * 0xFF);
		}
	}
	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage("*fog", (byte *)data, FOG_S, FOG_T, qfalse, ILF_NONE );
	ri.Hunk_FreeTempMemory( data );

	borderColor[0] = 1.0;
	borderColor[1] = 1.0;
	borderColor[2] = 1.0;
	borderColor[3] = 1;

	glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
}

/*
==================
R_CreateDefaultImage
==================
*/
#define	DEFAULT_SIZE	16
static void R_CreateDefaultImage( void ) {
	int		x;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	Com_Memset( data, 32, sizeof( data ) );
	for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
		data[0][x][0] =
		data[0][x][1] =
		data[0][x][2] =
		data[0][x][3] = 255;

		data[x][0][0] =
		data[x][0][1] =
		data[x][0][2] =
		data[x][0][3] = 255;

		data[DEFAULT_SIZE-1][x][0] =
		data[DEFAULT_SIZE-1][x][1] =
		data[DEFAULT_SIZE-1][x][2] =
		data[DEFAULT_SIZE-1][x][3] = 255;

		data[x][DEFAULT_SIZE-1][0] =
		data[x][DEFAULT_SIZE-1][1] =
		data[x][DEFAULT_SIZE-1][2] =
		data[x][DEFAULT_SIZE-1][3] = 255;
	}
	tr.defaultImage = R_CreateImage("*default", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qtrue, ILF_NONE );
}

void R_CreateBuiltinImages( void )
{
	uint x;
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	R_CreateDefaultImage();

	Com_Memset( data, 0x00, sizeof( data ) );
	tr.blackImage = R_CreateImage( "*black", (const byte*)data, 8, 8, qfalse, ILF_NONE );

	// we use a solid white image instead of disabling texturing
	Com_Memset( data, 0xFF, sizeof( data ) );
	tr.whiteImage = R_CreateImage( "*white", (const byte*)data, 8, 8, qfalse, ILF_NONE );

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for( x = 0; x < DEFAULT_SIZE; x++ )
	{
		uint y;
		for( y = 0; y < DEFAULT_SIZE; y++ )
		{
			data[y][x][0] = 
			data[y][x][1] = 
			data[y][x][2] = (byte)tr.identityLightByte;
			data[y][x][3] = 0xFF;			
		}
	}

	tr.identityLightImage = R_CreateImage("*identityLight", (const byte*)data, 8, 8, qfalse, ILF_NONE );

	for( x = 0; x < DEFAULT_SIZE; x++ )
	{
		uint y;
		for( y = 0; y < DEFAULT_SIZE; y++ )
		{
			data[y][x][0] = 0x7F;
			data[y][x][1] = 0x7F;
			data[y][x][2] = 0xFF;
			data[y][x][3] = 0xFF;
		}
	}
	tr.defNormMap = R_CreateImage("*defnormmap", (const byte*)data, 8, 8, qtrue, ILF_VECTOR_SB3 );

	for( x = 0; x < 32; x++ )
	{
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage( "*scratch", (const byte*)data, DEFAULT_SIZE, DEFAULT_SIZE, qfalse, ILF_NONE );
	}

	R_CreateDlightImage();
	R_CreateFogImage();

#ifdef _DEBUG
	tr.imageNotFoundImage = R_LoadDDSImage( "textures/gfx/imgnf.dds", qtrue );
#else
	tr.imageNotFoundImage = tr.defaultImage;
#endif
}


/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings( void ) {
	int		i, j;
	float	g;
	int		inf;
	int		shift;

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	if ( !glConfig.deviceSupportsGamma ) {
		tr.overbrightBits = 0;		// need hardware gamma for overbright
	}

	// never overbright in windowed mode
	if ( !glConfig.isFullscreen ) 
	{
		tr.overbrightBits = 0;
	}

	// allow 2 overbright bits in 24 bit, but only 1 in 16 bit
	if ( glConfig.colorBits > 16 ) {
		if ( tr.overbrightBits > 2 ) {
			tr.overbrightBits = 2;
		}
	} else {
		if ( tr.overbrightBits > 1 ) {
			tr.overbrightBits = 1;
		}
	}
	if ( tr.overbrightBits < 0 ) {
		tr.overbrightBits = 0;
	}

	tr.identityLight = 1.0f / ( 1 << tr.overbrightBits );
	tr.identityLightByte = (int)(tr.identityLight * 0xFF);


	if ( r_intensity->value <= 1 ) {
		ri.Cvar_Set( "r_intensity", "1" );
	}

	if ( r_gamma->value < 0.5f ) {
		ri.Cvar_Set( "r_gamma", "0.5" );
	} else if ( r_gamma->value > 3.0f ) {
		ri.Cvar_Set( "r_gamma", "3.0" );
	}

	g = r_gamma->value;

	shift = tr.overbrightBits;

	for ( i = 0; i < 256; i++ ) {
		if ( g == 1 ) {
			inf = i;
		} else {
			inf = (int)(pow( (float)i / (float)0xFF, 1.0F / g ) * 0xFF + 0.5F );
		}
		inf <<= shift;
		if (inf < 0) {
			inf = 0;
		}
		if (inf > 255) {
			inf = 255;
		}
		s_gammatable[i] = (byte)inf;
	}

	for (i=0 ; i<256 ; i++) {
		j = (int)(i * r_intensity->value);
		if (j > 255) {
			j = 255;
		}
		s_intensitytable[i] = (byte)j;
	}

	if ( glConfig.deviceSupportsGamma )
	{
		GLimp_SetGamma( s_gammatable, s_gammatable, s_gammatable );
	}
}

/*
===============
R_InitImages
===============
*/
void	R_InitImages( void ) {
	Com_Memset(hashTable, 0, sizeof(hashTable));
	// build brightness translation tables
	R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();
}

void R_DeleteTextures( void )
{
	int i;

	for( i = 0; i < tr.numImages; i++ )
		glDeleteTextures( 1, &tr.images[i]->texnum );
	
	tr.numImages = 0;
	Com_Memset( tr.images, 0, sizeof( tr.images ) );

	R_StateSetTexture( 0, GL_TEXTURE1_ARB );
	R_StateSetTexture( 0, GL_TEXTURE0_ARB );
}

/*
============================================================================

SKINS

============================================================================
*/

/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static char *CommaParse( char **data_p ) {
	int c = 0, len;
	char *data;
	static	char	com_token[MAX_TOKEN_CHARS];

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	for( ; ; )
	{
		// skip whitespace
		while( (c = *data) <= ' ') {
			if( !c ) {
				break;
			}
			data++;
		}


		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		for( ; ; )
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = (char)c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = (char)c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32 && c != ',' );

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}


/*
===============
RE_RegisterSkin

===============
*/
qhandle_t Q_EXTERNAL_CALL RE_RegisterSkin( const char *name ) {
	qhandle_t	hSkin;
	skin_t		*skin;
	skinSurface_t	*surf;
	char		*text, *text_p;
	char		*token;
	char		surfName[MAX_QPATH];

	if ( !name || !name[0] ) {
		ri.Printf( PRINT_ALL, "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf( PRINT_ALL, "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}


	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ ) {
		skin = tr.skins[hSkin];
		if ( !Q_stricmp( skin->name, name ) ) {
			if( skin->numSurfaces == 0 ) {
				return 0;		// default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS ) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}
	tr.numSkins++;
	skin = (skin_t*)ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	tr.skins[hSkin] = skin;
	Q_strncpyz( skin->name, name, sizeof( skin->name ) );
	skin->numSurfaces = 0;

	// If not a .skin file, load as a single shader
	if ( strcmp( name + strlen( name ) - 5, ".skin" ) ) {
		skin->numSurfaces = 1;
		skin->surfaces[0] = (skinSurface_t*)ri.Hunk_Alloc( sizeof(skin->surfaces[0]), h_low );
		skin->surfaces[0]->shader = R_FindShader( name, LIGHTMAP_NONE, qtrue );
		return hSkin;
	}

	// load and parse the skin file
    ri.FS_ReadFile( name, (void **)&text );
	if ( !text ) {
		return 0;
	}

	text_p = text;
	while ( text_p && *text_p ) {
		// get surface name
		token = CommaParse( &text_p );
		Q_strncpyz( surfName, token, sizeof( surfName ) );

		if ( !token[0] ) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr( surfName );

		if ( *text_p == ',' ) {
			text_p++;
		}

		if ( strstr( token, "tag_" ) ) {
			continue;
		}
		
		// parse the shader name
		token = CommaParse( &text_p );

		surf = skin->surfaces[ skin->numSurfaces ] = (skinSurface_t*)ri.Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
		Q_strncpyz( surf->name, surfName, sizeof( surf->name ) );
		surf->shader = R_FindShader( token, LIGHTMAP_NONE, qtrue );
		skin->numSurfaces++;
	}

	ri.FS_FreeFile( text );


	// never let a skin have 0 shaders
	if ( skin->numSurfaces == 0 ) {
		return 0;		// use default skin
	}

	return hSkin;
}


/*
===============
R_InitSkins
===============
*/
void R_InitSkins( void )
{
	skin_t		*skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = (skin_t*)ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	Q_strncpyz( skin->name, "<default skin>", sizeof( skin->name )  );
	skin->numSurfaces = 1;
	skin->surfaces[0] = (skinSurface_t*)ri.Hunk_Alloc( sizeof( *skin->surfaces ), h_low );
	skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t	*R_GetSkinByHandle( qhandle_t hSkin ) {
	if ( hSkin < 1 || hSkin >= tr.numSkins ) {
		return tr.skins[0];
	}
	return tr.skins[ hSkin ];
}

/*
===============
R_SkinList_f
===============
*/
void Q_EXTERNAL_CALL R_SkinList_f( void )
{
	int			i, j;
	skin_t		*skin;

	ri.Printf (PRINT_ALL, "------------------\n");

	for ( i = 0 ; i < tr.numSkins ; i++ ) {
		skin = tr.skins[i];

		ri.Printf( PRINT_ALL, "%3i:%s\n", i, skin->name );
		for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
			ri.Printf( PRINT_ALL, "       %s = %s\n", 
				skin->surfaces[j]->name, skin->surfaces[j]->shader->name );
		}
	}
	ri.Printf (PRINT_ALL, "------------------\n");
}

