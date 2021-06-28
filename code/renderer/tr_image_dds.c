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

image_t* R_MakeImage( const char *name );

typedef struct
{
	unsigned long		dwColorSpaceLowValue;   // low boundary of color space that is to
												// be treated as Color Key, inclusive
    unsigned long		dwColorSpaceHighValue;  // high boundary of color space that is
												// to be treated as Color Key, inclusive
} DDCOLORKEY_t;

typedef struct
{
    unsigned long		dwCaps;         // capabilities of surface wanted
    unsigned long		dwCaps2;
    unsigned long		dwCaps3;
    union
    {
        unsigned long	dwCaps4;
        unsigned long	dwVolumeDepth;
    } u0;
} DDSCAPS2_t;

typedef struct
{
    unsigned long	dwSize;                 // size of structure
    unsigned long	dwFlags;                // pixel format flags
    unsigned long	dwFourCC;               // (FOURCC code)
    union
    {
        unsigned long	dwRGBBitCount;          // how many bits per pixel
        unsigned long	dwYUVBitCount;          // how many bits per pixel
        unsigned long	dwZBufferBitDepth;      // how many total bits/pixel in z buffer (including any stencil bits)
        unsigned long	dwAlphaBitDepth;        // how many bits for alpha channels
        unsigned long	dwLuminanceBitCount;    // how many bits per pixel
        unsigned long	dwBumpBitCount;         // how many bits per "buxel", total
        unsigned long	dwPrivateFormatBitCount;// Bits per pixel of private driver formats. Only valid in texture
												// format list and if DDPF_D3DFORMAT is set
    } u0;
    union
    {
        unsigned long	dwRBitMask;             // mask for red bit
        unsigned long	dwYBitMask;             // mask for Y bits
        unsigned long	dwStencilBitDepth;      // how many stencil bits (note: dwZBufferBitDepth-dwStencilBitDepth is total Z-only bits)
        unsigned long	dwLuminanceBitMask;     // mask for luminance bits
        unsigned long	dwBumpDuBitMask;        // mask for bump map U delta bits
        unsigned long	dwOperations;           // DDPF_D3DFORMAT Operations
    } u1;
    union
    {
        unsigned long	dwGBitMask;             // mask for green bits
        unsigned long	dwUBitMask;             // mask for U bits
        unsigned long	dwZBitMask;             // mask for Z bits
        unsigned long	dwBumpDvBitMask;        // mask for bump map V delta bits
        struct
        {
            unsigned short	wFlipMSTypes;       // Multisample methods supported via flip for this D3DFORMAT
            unsigned short	wBltMSTypes;        // Multisample methods supported via blt for this D3DFORMAT
        } MultiSampleCaps;

    } u2;
    union
    {
        unsigned long	dwBBitMask;             // mask for blue bits
        unsigned long	dwVBitMask;             // mask for V bits
        unsigned long	dwStencilBitMask;       // mask for stencil bits
        unsigned long	dwBumpLuminanceBitMask; // mask for luminance in bump map
    } u3;
    union
    {
        unsigned long	dwRGBAlphaBitMask;      // mask for alpha channel
        unsigned long	dwYUVAlphaBitMask;      // mask for alpha channel
        unsigned long	dwLuminanceAlphaBitMask;// mask for alpha channel
        unsigned long	dwRGBZBitMask;          // mask for Z channel
        unsigned long	dwYUVZBitMask;          // mask for Z channel
    } u4;
} DDPIXELFORMAT_t;

typedef struct
{
    unsigned long		dwSize;                 // size of the DDSURFACEDESC structure
    unsigned long		dwFlags;                // determines what fields are valid
    unsigned long		dwHeight;               // height of surface to be created
    unsigned long		dwWidth;                // width of input surface
    union
    {
        long			lPitch;                 // distance to start of next line (return value only)
        unsigned long	dwLinearSize;           // Formless late-allocated optimized surface size
    } u0;
    union
    {
        unsigned long	dwBackBufferCount;      // number of back buffers requested
        unsigned long	dwDepth;                // the depth if this is a volume texture 
    } u1;
    union
    {
        unsigned long	dwMipMapCount;          // number of mip-map levels requestde
                                                // dwZBufferBitDepth removed, use ddpfPixelFormat one instead
        unsigned long	dwRefreshRate;          // refresh rate (used when display mode is described)
        unsigned long	dwSrcVBHandle;          // The source used in VB::Optimize
    } u2;
    unsigned long		dwAlphaBitDepth;        // depth of alpha buffer requested
    unsigned long		dwReserved;             // reserved
    void*				lpSurface;              // pointer to the associated surface memory
    union
    {
        DDCOLORKEY_t    ddckCKDestOverlay;      // color key for destination overlay use
        unsigned long	dwEmptyFaceColor;       // Physical color for empty cubemap faces
    } u3;
    DDCOLORKEY_t        ddckCKDestBlt;          // color key for destination blt use
    DDCOLORKEY_t        ddckCKSrcOverlay;       // color key for source overlay use
    DDCOLORKEY_t        ddckCKSrcBlt;           // color key for source blt use
    union
    {
        DDPIXELFORMAT_t ddpfPixelFormat;        // pixel format description of the surface
        unsigned long	dwFVF;                  // vertex format description of vertex buffers
    } u4;
    DDSCAPS2_t          ddsCaps;                // direct draw surface capabilities
    unsigned long		dwTextureStage;         // stage in multitexture cascade
} DDSURFACEDESC2_t;

//
// DDSURFACEDESC2 flags that mark the validity of the struct data
//
#define DDSD_CAPS								0x00000001l			// default
#define DDSD_HEIGHT								0x00000002l			// default
#define DDSD_WIDTH								0x00000004l			// default
#define DDSD_PIXELFORMAT						0x00001000l			// default
#define DDSD_PITCH								0x00000008l			// For uncompressed formats
#define DDSD_MIPMAPCOUNT						0x00020000l
#define DDSD_LINEARSIZE							0x00080000l			// For compressed formats
#define DDSD_DEPTH								0x00800000l			// Volume Textures

//
// DDPIXELFORMAT flags
//
#define DDPF_ALPHAPIXELS						0x00000001l
#define DDPF_FOURCC								0x00000004l			// Compressed formats 
#define DDPF_RGB								0x00000040l			// Uncompressed formats
#define DDPF_ALPHA								0x00000002l
#define DDPF_COMPRESSED							0x00000080l
#define DDPF_LUMINANCE							0x00020000l
#define DDPF_PALETTEINDEXED4					0x00000008l
#define DDPF_PALETTEINDEXEDTO8					0x00000010l
#define DDPF_PALETTEINDEXED8					0x00000020l

//
// DDSCAPS flags
//
#define DDSCAPS_COMPLEX							0x00000008l
#define DDSCAPS_TEXTURE							0x00001000l			// default
#define DDSCAPS_MIPMAP							0x00400000l

#define DDSCAPS2_VOLUME							0x00200000l
#define DDSCAPS2_CUBEMAP						0x00000200L
#define DDSCAPS2_CUBEMAP_POSITIVEX				0x00000400L
#define DDSCAPS2_CUBEMAP_NEGATIVEX				0x00000800L
#define DDSCAPS2_CUBEMAP_POSITIVEY				0x00001000L
#define DDSCAPS2_CUBEMAP_NEGATIVEY				0x00002000L
#define DDSCAPS2_CUBEMAP_POSITIVEZ				0x00004000L
#define DDSCAPS2_CUBEMAP_NEGATIVEZ				0x00008000L

#ifndef MAKEFOURCC

#define MAKEFOURCC(ch0, ch1, ch2, ch3)											\
    ((unsigned long)(char)(ch0) | ((unsigned long)(char)(ch1) << 8) |			\
    ((unsigned long)(char)(ch2) << 16) | ((unsigned long)(char)(ch3) << 24 ))

#endif

#define FOURCC_DDS		MAKEFOURCC( 'D', 'D', 'S', ' ' ) 

//FOURCC codes for DXTn compressed-texture pixel formats
#define FOURCC_DXT1		MAKEFOURCC( 'D', 'X', 'T', '1' )
#define FOURCC_DXT2		MAKEFOURCC( 'D', 'X', 'T', '2' )
#define FOURCC_DXT3		MAKEFOURCC( 'D', 'X', 'T', '3' )
#define FOURCC_DXT4		MAKEFOURCC( 'D', 'X', 'T', '4' )
#define FOURCC_DXT5		MAKEFOURCC( 'D', 'X', 'T', '5' )

#define R_LoadDDSImage_MAX_MIPS 16

static ID_INLINE void UnpackRGB565( byte rgb[3], ushort cl )
{
	byte r = (byte)((cl >> 11) & 0x1F);
	byte g = (byte)((cl >> 5) & 0x3F);
	byte b = (byte)(cl & 0x1F);

	rgb[0] = (r << 3) | (r >> 2); //multiply by 8.22 -> 8.25
	rgb[1] = (g << 2) | (g >> 4); //multiply by 4.047 -> 4.0625
	rgb[2] = (b << 3) | (b >> 2); //multiply by 8.22 -> 8.25
}

static void R_DecodeS3TCBlock( byte out[4][4][4], int bx, int by,
	int format, int iw, int ih, const void *image_base )
{
	int x, y;

	int blocksize;
	const byte *block_base, *color_base, *alpha_base;

	ushort c0, c1;
	byte rgba[4][4];

	switch( format )
	{
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		blocksize = 8;
		break;

	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		blocksize = 16;
		break;

	default:
		ri.Printf( PRINT_WARNING, "invalid compressed image format\n" );
		return;
	}

	block_base = (byte*)image_base +
		blocksize * (((iw + 3) / 4) * (by / 4) + bx / 4);

	switch( format )
	{
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		color_base = block_base;
		alpha_base = NULL;
		break;

	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		alpha_base = block_base;
		color_base = block_base + 8;
		break;

	NO_DEFAULT;
	}

	c0 = (ushort)color_base[0] | ((ushort)color_base[1] << 8);
	c1 = (ushort)color_base[2] | ((ushort)color_base[3] << 8);
		
	UnpackRGB565( rgba[0], c0 );
	UnpackRGB565( rgba[1], c1 );

	rgba[0][3] = 0xFF;
	rgba[1][3] = 0xFF;
	rgba[2][3] = 0xFF;
	rgba[3][3] = 0xFF;

	switch( format )
	{
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		if( c0 <= c1 )
			rgba[3][3] = 0;
		//fallthrough

	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		if( c0 <= c1 )
		{
			rgba[2][0] = (byte)((rgba[0][0] + rgba[1][0]) / 2);
			rgba[2][1] = (byte)((rgba[0][1] + rgba[1][1]) / 2);
			rgba[2][2] = (byte)((rgba[0][2] + rgba[1][2]) / 2);

			rgba[3][0] = 0;
			rgba[3][1] = 0;
			rgba[3][2] = 0;

			break;
		}
		//fallthrough

	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		rgba[2][0] = (byte)((rgba[0][0] * 2 + rgba[1][0]) / 3);
		rgba[2][1] = (byte)((rgba[0][1] * 2 + rgba[1][1]) / 3);
		rgba[2][2] = (byte)((rgba[0][2] * 2 + rgba[1][2]) / 3);

		rgba[3][0] = (byte)((rgba[1][0] * 2 + rgba[0][0]) / 3);
		rgba[3][1] = (byte)((rgba[1][1] * 2 + rgba[0][1]) / 3);
		rgba[3][2] = (byte)((rgba[1][2] * 2 + rgba[0][2]) / 3);
		break;

	NO_DEFAULT;
	}

	for( y = 0; y < 4; y++ )
	{
		for( x = 0; x < 4; x++ )
		{
			int idx = (color_base[4 + y] >> (x * 2)) & 0x3;

			out[y][x][0] = rgba[idx][0];
			out[y][x][1] = rgba[idx][1];
			out[y][x][2] = rgba[idx][2];
			out[y][x][3] = rgba[idx][3];
		}
	}

	switch( format )
	{
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		break;

	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		for( y = 0; y < 4; y++ )
		{
			for( x = 0; x < 4; x++ )
			{
				byte b = (alpha_base[(y * 2) + (x / 2)] >> ((x & 1) * 4)) & 0x0F;
				out[y][x][3] = b | (b << 4); //multiply by 17, this one works out exactly
			}
		}
		break;

	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		{
			byte a[8];
			int idxs;

			a[0] = alpha_base[0];
			a[1] = alpha_base[1];

			if( a[0] > a[1] )
			{
				a[2] = (6 * a[0] + 1 * a[1]) / 7;
				a[3] = (5 * a[0] + 2 * a[1]) / 7;
				a[4] = (4 * a[0] + 3 * a[1]) / 7;
				a[5] = (3 * a[0] + 4 * a[1]) / 7;
				a[6] = (2 * a[0] + 5 * a[1]) / 7;
				a[7] = (1 * a[0] + 6 * a[1]) / 7;
			}
			else
			{
				a[2] = (4 * a[0] + 1 * a[1]) / 5;
				a[3] = (3 * a[0] + 2 * a[1]) / 5;
				a[4] = (2 * a[0] + 3 * a[1]) / 5;
				a[5] = (1 * a[0] + 4 * a[1]) / 5;
				a[6] = 0;
				a[7] = 1;
			}

			//do 2 sets of 3 bytes at a time (easier to cope with 3-bit indices crossing byte boundaries)
			idxs = alpha_base[2] | (alpha_base[3] << 8) | (alpha_base[4] << 16);

			for( y = 0; y < 2; y++ )
			{
				for( x = 0; x < 4; x++ )
				{
					int idx = idxs >> 3 * (y * 4 + x) & 0x7;

					out[y][x][3] = a[idx];
				}
			}

			idxs = alpha_base[5] | (alpha_base[6] << 8) | (alpha_base[7] << 16);

			for( y = 0; y < 2; y++ )
			{
				for( x = 0; x < 4; x++ )
				{
					int idx = idxs >> 3 * (y * 4 + x) & 0x7;

					out[y + 2][x][3] = a[idx];
				}
			}
		}
		break;

	NO_DEFAULT;
	}
}

static void R_DecodeRGB565Block( byte out[4][4][4], int bx, int by,
	int format, int iw, int ih, const void *image_base )
{
	int x, y;
	const byte *row;
	size_t pitch;

	REF_PARAM( format );

	pitch = iw * 2;
	row = (byte*)image_base + (by * iw + bx) * 2;

	for( y = 0; y < 4; y++ )
	{
		for( x = 0; x < 4; x++ )
		{
			ushort c;

			const byte *px = row + x * 2;

			c = (ushort)px[0] | ((ushort)px[1] << 8);

			UnpackRGB565( out[y][x], c );
			out[y][x][3] = 0xFF;
		}

		row += pitch;		
	}
}

static void R_UploadEncodedImageDirect( GLenum target, int level, GLenum format, GLenum int_fmat, int width, int height, const void *data,
	void (*decoder)( byte out[4][4][4], int bx, int by, int format, int iw, int ih, const void *image_base ) )
{
	int x, y;
	byte block[4][4][4]; //y, x, rgba

	glTexImage2D( target, level, int_fmat, width, height, 0, GL_RGBA, GL_BYTE, NULL ); 

	//decode the data
	for( y = 0; y < height; y += 4 )
	{
		for( x = 0; x < width; x += 4 )
		{
			int uw = width - x;
			int uh = height - y;

			if( uw > 4 ) uw = 4;
			if( uh > 4 ) uh = 4;

			decoder( block, x, y, format, width, height, data );
			glTexSubImage2D( target, level, x, y, uw, uh, GL_RGBA, GL_UNSIGNED_BYTE, block );
		}
	}
}

static void R_UploadCompressedImage2D( image_t *img, GLenum target, int level, GLenum format, int width, int height, int size, const void *data )
{
	GLenum int_fmat;

	if( GLEW_ARB_texture_compression && GLEW_EXT_texture_compression_s3tc )
	{
		glCompressedTexImage2DARB( target, level, format, width, height, 0, size, data );
		return;
	}

	switch( format )
	{
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		int_fmat = GL_RGB;
		break;

	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		int_fmat = GL_RGBA;
		break;

	default:
		ri.Printf( PRINT_WARNING, "invalid compressed image format\n" );
		return;
	}

	if( img->mipmap && (img->uploadWidth * img->uploadHeight) > (256 * 256) )
	{
		//skip high-level mips
		img->uploadWidth /= 2;
		img->uploadHeight /= 2;

		if( img->uploadWidth == 0 )
			img->uploadWidth = 1;

		if( img->uploadHeight == 0 )
			img->uploadHeight = 1;

		return;
	}

	level = 0;

	{
		int x, t;

		if( img->uploadWidth > img->uploadHeight )
		{
			t = img->uploadWidth;
			x = width;
		}
		else
		{
			t = img->uploadHeight;
			x = height;
		}

		while( t > x )
		{
			t /= 2;
			level++;
		}
	}

	R_UploadEncodedImageDirect( target, level, format, int_fmat, width, height, data, R_DecodeS3TCBlock );
}

static void R_UploadImage2D( image_t *img, GLenum target, int level, GLenum int_fmat,
	int width, int height, GLenum format, GLenum type, const void *data )
{
#ifdef Q3_BIG_ENDIAN
	// OSX 10.3.9 has problems with GL_UNSIGNED_SHORT_5_6_5 which gives us the multicolored lightmaps
	if( type == GL_UNSIGNED_SHORT_5_6_5 )
#else
	if( type == GL_UNSIGNED_SHORT_5_6_5 && !GLEW_VERSION_1_2 )
#endif
	{
		R_UploadEncodedImageDirect( target, level, format, int_fmat, width, height, data, R_DecodeRGB565Block );
		return;
	}
	glTexImage2D( target, level, int_fmat, width, height, 0, format, type, data ); 
}

image_t* R_LoadDDSImageData( void *pImageData, const char *name, qboolean mipmap )
{
	image_t *ret = NULL;

	byte *buff;
	
	DDSURFACEDESC2_t *ddsd;			//used to get at the dds header in the read buffer

	//based on texture type:
    //	cube		width != 0, height == 0, depth == 0
	//	2D			width != 0, height != 0, depth == 0
	//	volume		width != 0, height != 0, depth != 0
	int width, height, depth;

	//mip count and pointers to image data for each mip
	//level, idx 0 = top level last pointer does not start
	//a mip level, it's just there to mark off the end of
	//the final mip data segment (thus the odd + 1)
	//
	//for cube textures we only find the offsets into the
	//first face of the cube, subsequent faces will use the
	//same offsets, just shifted over
	int mipLevels;
	byte *mipOffsets[R_LoadDDSImage_MAX_MIPS + 1];

	qboolean usingAlpha;

	qboolean compressed;
	GLuint format = 0;
	GLuint internal_format = 0;
	GLenum type = GL_UNSIGNED_BYTE;

	//comes from R_CreateImage
	if( tr.numImages == MAX_DRAWIMAGES )
	{
		ri.Printf( PRINT_WARNING, "R_LoadDDSImage: MAX_DRAWIMAGES hit\n" );
		return NULL;
	}

	buff = (byte*)pImageData;

	if( strncmp( (const char*)buff, "DDS ", 4 ) != 0 )
	{
		ri.Printf( PRINT_WARNING, "R_LoadDDSImage: invalid dds header \"%s\"\n", name );
		goto ret_error;
	}

	ddsd = (DDSURFACEDESC2_t*)(buff + 4);
	
	//Byte Swapping for the DDS headers.
	//beware: we ignore some of the shorts.
	#ifdef Q3_BIG_ENDIAN
	{
		int i;
		long *field;
		for (i=0; i < sizeof(DDSURFACEDESC2_t)/sizeof(long); i++) {
			field = (long *)ddsd;
			field[i] = LittleLong(field[i]);
		}
		
	}
	
	#endif

	if( ddsd->dwSize != sizeof( DDSURFACEDESC2_t ) ||
		ddsd->u4.ddpfPixelFormat.dwSize != sizeof( DDPIXELFORMAT_t ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadDDSImage: invalid dds header \"%s\"\n", name );
		goto ret_error;
	}

	usingAlpha = (ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) ? qtrue : qfalse;
	mipLevels = ((ddsd->dwFlags & DDSD_MIPMAPCOUNT) && (ddsd->u2.dwMipMapCount > 1)) ? ddsd->u2.dwMipMapCount : 1;

	if( mipLevels > R_LoadDDSImage_MAX_MIPS )
	{
		ri.Printf( PRINT_WARNING, "R_LoadDDSImage: dds image has too many mip levels \"%s\"\n", name );
		goto ret_error;
	}

	compressed = (ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) ? qtrue : qfalse;

	//either a cube or a volume
	if( ddsd->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP )
	{
		//cube texture

		if( ddsd->dwWidth != ddsd->dwHeight )
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: invalid dds image \"%s\"\n", name );
			goto ret_error;
		}

		width = ddsd->dwWidth;
		height = 0;
		depth = 0;

		if( width & (width - 1) )
		{
			//cubes must be a power of two
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: cube images must be power of two \"%s\"\n", name );
			goto ret_error;
		}
	}
	else if( (ddsd->ddsCaps.dwCaps2 & DDSCAPS2_VOLUME) && (ddsd->dwFlags & DDSD_DEPTH) )
	{
		//3D texture

		width = ddsd->dwWidth;
		height = ddsd->dwHeight;
		depth = ddsd->u1.dwDepth;

		if( width & (width - 1) || height & (height - 1) || depth & (depth - 1) )
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: volume images must be power of two \"%s\"\n", name );
			goto ret_error;
		}
	}
	else
	{
		//2D texture

		width = ddsd->dwWidth;
		height = ddsd->dwHeight;
		depth = 0;

		//these are allowed to be non power of two, will be dealt with later on
		//except for compressed images!
		if( compressed && (width & (width - 1) || height & (height - 1)) )
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: compressed texture images must be power of two \"%s\"\n", name );
			goto ret_error;
		}
	}

	if( compressed )
	{
		int blockSize;

		if( depth != 0 )
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: compressed volume textures are not supported \"%s\"\n", name );
			goto ret_error;
		}

		//compressed FOURCC formats
		switch( ddsd->u4.ddpfPixelFormat.dwFourCC )
		{
		case FOURCC_DXT1:
			blockSize = 8;
			usingAlpha = qtrue;
			format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			break;

		case FOURCC_DXT3:
			blockSize = 16;
			usingAlpha = qtrue;
			format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			break;

		case FOURCC_DXT5:
			blockSize = 16;
			usingAlpha = qtrue;
			format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;
            
		default:
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: unsupported FOURCC \"%s\", \"%s\"\n",
				&ddsd->u4.ddpfPixelFormat.dwFourCC, name );
			goto ret_error;
		}

		//get mip offsets
		if( format )
		{
			int w		= width;
			int h		= height;
			int offset	= 0;
			int i;

			if( h == 0 ) h = w; //cube map

			for( i = 0; (i < mipLevels) && (w || h); i++ )
			{
				int qw, qh;

				mipOffsets[ i ] = buff + 4 + sizeof( DDSURFACEDESC2_t ) + offset;

				if( w == 0 ) w = 1;
				if( h == 0 ) h = 1;

				//size formula comes from DX docs (August 2005 SDK reference)
				qw = w >> 2; if( qw == 0 ) qw = 1;
				qh = h >> 2; if( qh == 0 ) qh = 1;

				offset += qw * qh * blockSize;

				w >>= 1;
				h >>= 1;
			}

			//put in the trailing offset
			mipOffsets[ i ] = buff + 4 + sizeof( DDSURFACEDESC2_t ) + offset;
		}
		else
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: error reading DDS image \"%s\"\n", name );
			goto ret_error;
		}

		internal_format = format;
	}
	else
	{
		//uncompressed format
		if( ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_RGB )
		{
			switch( ddsd->u4.ddpfPixelFormat.u0.dwRGBBitCount )
			{
			case 32:
				internal_format = usingAlpha ? GL_RGBA8 : GL_RGB8;
				format = GL_BGRA_EXT;
				break;

			case 24:
				internal_format = GL_RGB8;
				format = GL_BGR_EXT;
				break;

			case 16:
				if( usingAlpha )
				{
					//must be A1R5G5B5
					ri.Printf( PRINT_WARNING, "R_LoadDDSImage: unsupported format, 5551 \"%s\"\n", name );
					goto ret_error;
				}
				else
				{
					//find out if it's X1R5G5B5 or R5G6B5

					internal_format = GL_RGB5;
					format = GL_RGB;

					if( ddsd->u4.ddpfPixelFormat.u2.dwGBitMask == 0x7E0 )
						type = GL_UNSIGNED_SHORT_5_6_5;
					else
					{
						ri.Printf( PRINT_WARNING, "R_LoadDDSImage: unsupported format, 5551 \"%s\"\n", name );
						goto ret_error;
					}
				}
				break;

			default:
				ri.Printf( PRINT_WARNING, "R_LoadDDSImage: unsupported RGB bit depth \"%s\"\n", name );
				goto ret_error;
			}
		}
		else if( ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_LUMINANCE )
		{
			internal_format = usingAlpha ? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE8;
			format = usingAlpha ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
		}
		else if( ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_ALPHA )
		{
			internal_format = GL_ALPHA8;
			format = GL_ALPHA;
		}
		else if( ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4 )
		{
			internal_format = usingAlpha ? GL_RGB5_A1 : GL_RGB5;
			format = GL_COLOR_INDEX4_EXT;
		}
		else if( ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8 )
		{
			internal_format = usingAlpha ? GL_RGBA : GL_RGB;
			format = GL_COLOR_INDEX8_EXT;
		}
		else
		{  
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: unsupported DDS image type \"%s\"\n", name );
			goto ret_error;
		}

		if ( format )
		{
			int offset = 0;
			int w = width;
			int h = height;
			int d = depth;
			int i;

			if( h == 0 ) h = w; //cube map

			for( i = 0; (i < mipLevels) && (w || h || d); i++ )
			{
				mipOffsets[ i ] = buff + 4 + sizeof( DDSURFACEDESC2_t ) + offset;

				if( w == 0 ) w = 1;
				if( h == 0 ) h = 1;
				if( d == 0 ) d = 1;
				
				offset += (w * h * d * (ddsd->u4.ddpfPixelFormat.u0.dwRGBBitCount / 8));

				w >>= 1;
				h >>= 1;
				d >>= 1;
			}

			//put in the trailing offset
			mipOffsets[ i ] = buff + 4 + sizeof( DDSURFACEDESC2_t ) + offset;
		}
		else
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: Unexpected error reading DDS image \"%s\"\n", name );
			goto ret_error;
		}
	}

	//we now have a full description of our image set up
	//if we fail after this point we still want our image_t
	//record in the hash so that we don't go trying to read
	//the file again - been printing an error and returning
	//NULL up to this point...
	ret = R_MakeImage( name );

	ret->uploadWidth = ret->width = width;
	ret->uploadHeight = ret->height = height;

	ret->internalFormat = internal_format;
	ret->hasAlpha = usingAlpha;

	ret->mipmap = (mipmap && (mipLevels > 1)) ? qtrue : qfalse;
	    	
	if( !ret->mipmap )
		mipLevels = 1;

	if( depth )
	{
		//volume texture - must be power of 2

		int w = width;
		int h = height;
		int d = depth;

		int i;

		if( !GLEW_EXT_texture3D && !GLEW_VERSION_1_2 )
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: Tried to load 3D texture but missing hardware support \"%s\"\n", name );
			goto ret_error;
		}

		ret->depth = depth;
		ret->uploadDepth = depth;

		ret->texTarget = GL_TEXTURE_3D_EXT;
		ret->addrMode = TAM_Normalized;

		R_StateSetTexture( ret, GL_TEXTURE0_ARB );

		if( mipmap && mipLevels == 1 )
			glTexParameteri( ret->texTarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );

		for( i = 0; i < mipLevels; i++ )
		{
			if( GLEW_EXT_texture3D )
			{
				glTexImage3DEXT( GL_TEXTURE_3D_EXT, i, internal_format, w, h, d,
					0, format, type, mipOffsets[ i ] );
			}
#if GL_VERSION_1_2
			else
			{
				glTexImage3D( GL_TEXTURE_3D, i, internal_format, w, h, d,
					0, format, type, mipOffsets[ i ] );
			}
#endif
		    
			w >>= 1; if( w == 0 ) w = 1;
			h >>= 1; if( h == 0 ) h = 1;
			d >>= 1; if( d == 0 ) d = 1;
		}

		R_StateSetTexture( 0, GL_TEXTURE0_ARB );
	}
	else if( !height )
	{
		//cube texture - must be power of 2

		int w;
		int i;
		size_t shift = mipOffsets[ mipLevels ] - mipOffsets[ 0 ];
	
		if( !GLEW_ARB_texture_cube_map && !GLEW_EXT_texture_cube_map && !GLEW_VERSION_1_2 )
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: Tried to load Cube texture but missing hardware support \"%s\"\n", name );
			goto ret_error;
		}

		ret->texTarget = GL_TEXTURE_CUBE_MAP_ARB;
		ret->addrMode = TAM_Normalized;
       		                          
		R_StateSetTexture( ret, GL_TEXTURE0_ARB );

		//macros so this doesn't get disgustingly huge
#define	loadCubeFace( glTarget )													\
		w = width;																	\
																					\
		for( i = 0; i < mipLevels; i++ )											\
		{																			\
			if( compressed )														\
			{																		\
				GLsizei size = mipOffsets[ i + 1 ] - mipOffsets[ i ];				\
				R_UploadCompressedImage2D( ret, glTarget, i, format, w, w,			\
					size, mipOffsets[ i ] );										\
			}																		\
			else																	\
			{																		\
				R_UploadImage2D( ret, glTarget, i, internal_format, w, w,			\
					internal_format, type, mipOffsets[ i ] );						\
			}																		\
																					\
			w >>= 1; if( w == 0 ) w = 1;											\
		}

#define shiftMipOffsets()															\
		for( i = 0; i <= mipLevels; i++ )											\
			mipOffsets[ i ] += shift

		if( mipmap && mipLevels == 1 )
			glTexParameteri( ret->texTarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );

		//the faces are stored in the order +x, -x, +y, -y, +z, -z
		//but there may be missing faces in the sequence which we cannot upload
		if( ddsd->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX )
		{
			loadCubeFace( GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB ); shiftMipOffsets();
		}

		if( ddsd->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEX )
		{
			loadCubeFace( GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB ); shiftMipOffsets();
		}

		if( ddsd->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEY )
		{
			loadCubeFace( GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB ); shiftMipOffsets();
		}

		if( ddsd->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEY )
		{
			loadCubeFace( GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB ); shiftMipOffsets();
		}

		if( ddsd->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEZ )
		{
			loadCubeFace( GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB ); shiftMipOffsets();
		}

		if( ddsd->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ )
		{
			loadCubeFace( GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB ); shiftMipOffsets();
		}

		R_StateSetDefaultTextureModesUntracked( ret->texTarget, ret->mipmap, GL_TEXTURE0_ARB );

		R_StateSetTexture( 0, GL_TEXTURE0_ARB );
	}
	else
	{
		//planar texture

		int w = width;
		int h = height;
		
		int i;

		GLuint texnum;

		if( w & (w - 1) || h & (h - 1) )
		{
			//non-pow2: check extensions

			if( GLEW_ARB_texture_non_power_of_two )
			{
				ret->texTarget = GL_TEXTURE_2D;
				ret->addrMode = TAM_Normalized;
			}
			else
			{
				ret->texTarget = R_StateGetRectTarget();
				ret->addrMode = TAM_Dimensions;
			}
		}
		else
		{
			ret->texTarget		= GL_TEXTURE_2D;
			ret->addrMode		= TAM_Normalized;
		}
		
		texnum = ret->texnum;

		R_StateSetTexture( ret, GL_TEXTURE0_ARB );

		if( mipmap && mipLevels == 1 )
			glTexParameteri( ret->texTarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );

		for( i = 0; i < mipLevels; i++ )
		{
			if( compressed )
			{
				GLsizei size = mipOffsets[ i + 1 ] - mipOffsets[ i ];
				R_UploadCompressedImage2D( ret, ret->texTarget, i, internal_format, w, h, size, mipOffsets[ i ] );
			}
			else
			{
				R_UploadImage2D( ret, ret->texTarget, i, internal_format, w, h,
					format, type, mipOffsets[ i ] );
			}

			w >>= 1; if( w == 0 ) w = 1;
			h >>= 1; if( h == 0 ) h = 1;
		}

		R_StateSetDefaultTextureModesUntracked( ret->texTarget, ret->mipmap, GL_TEXTURE0_ARB );

		R_StateSetTexture( 0, GL_TEXTURE0_ARB );
	}

ret_error:
	return ret;
}

void Q_EXTERNAL_CALL RE_PreloadDDSImage( void *pImageData, const char *name )
{
	image_t *img;
	
	img = R_LoadDDSImageData( pImageData, name, qtrue );
}

image_t* R_LoadDDSImage( const char *name, qboolean mipmap )
{
	image_t *ret;
	byte *buff;
	int len;
	
	//comes from R_CreateImage
	if( tr.numImages == MAX_DRAWIMAGES )
	{
		ri.Printf( PRINT_WARNING, "R_LoadDDSImage: MAX_DRAWIMAGES hit\n" );
		return NULL;
	}

	len = ri.FS_ReadFile( name, (void**)&buff );
	if( !buff )
		return 0;

	ret = R_LoadDDSImageData( buff, name, mipmap );

	ri.FS_FreeFile( buff );

	return ret;
}