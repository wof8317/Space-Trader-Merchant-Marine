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
//
// q_shared.c -- stateless support routines that are included in each code dll
#include "q_shared.h"


int SWITCHSTRING( const char* s )
{
	int hash;
	int c;
	
	c = s[0];
	if ( !c )
		return 0;
	hash = c;

	c = s[1];
	if ( c ) {
		hash = hash | (c<<8);

		c = s[2];
		if ( c ) {
			hash = hash | (c<<16);

			c = s[3];
			if ( c ) {
				hash = hash | (c<<24);
			}
		}
	}

	return hash&~0x20202020;
}

float Com_Clamp( float min, float max, float value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}


/*
============
COM_SkipPath
============
*/
char *COM_SkipPath (char *pathname)
{
	char	*last;
	
	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension( const char *in, char *out, int destsize ) {
	int             length;

	Q_strncpyz(out, in, destsize);

	length = strlen(out)-1;
	while (length > 0 && out[length] != '.')
	{
		length--;
		if (out[length] == '/')
			return;		// no extension
	}
	if (length)
		out[length] = 0;
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension (char *path, int maxSize, const char *extension ) {
	char	oldPath[MAX_QPATH];
	char    *src;

//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path) {
		if ( *src == '.' ) {
			return;                 // it has an extension
		}
		src--;
	}

	Q_strncpyz( oldPath, path, sizeof( oldPath ) );
	Com_sprintf( path, maxSize, "%s%s", oldPath, extension );
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/
/*
// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
static short	(*_BigShort) (short l);
static short	(*_LittleShort) (short l);
static int		(*_BigLong) (int l);
static int		(*_LittleLong) (int l);
static qint64	(*_BigLong64) (qint64 l);
static qint64	(*_LittleLong64) (qint64 l);
static float	(*_BigFloat) (const float *l);
static float	(*_LittleFloat) (const float *l);

short	BigShort(short l){return _BigShort(l);}
short	LittleShort(short l) {return _LittleShort(l);}
int		BigLong (int l) {return _BigLong(l);}
int		LittleLong (int l) {return _LittleLong(l);}
qint64 	BigLong64 (qint64 l) {return _BigLong64(l);}
qint64 	LittleLong64 (qint64 l) {return _LittleLong64(l);}
float	BigFloat (const float *l) {return _BigFloat(l);}
float	LittleFloat (const float *l) {return _LittleFloat(l);}
*/

short ShortSwap( short l )
{
	byte    b1,b2;

	b1 = (byte)(l & 255);
	b2 = (byte)((l >> 8) & 255);

	return (b1 << 8) | b2;
}

short ShortNoSwap( short l )
{
	return l;
}

int LongSwap( int l )
{
	byte b1, b2, b3, b4;

	b1 = (byte)(l & 255);
	b2 = (byte)((l>>8) & 255);
	b3 = (byte)((l>>16) & 255);
	b4 = (byte)((l>>24) & 255);

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int	LongNoSwap (int l)
{
	return l;
}

qint64 Long64Swap (qint64 ll)
{
	qint64	result;

	result.b0 = ll.b7;
	result.b1 = ll.b6;
	result.b2 = ll.b5;
	result.b3 = ll.b4;
	result.b4 = ll.b3;
	result.b5 = ll.b2;
	result.b6 = ll.b1;
	result.b7 = ll.b0;

	return result;
}

qint64 Long64NoSwap (qint64 ll)
{
	return ll;
}

typedef union {
    float	f;
    unsigned int i;
} _FloatByteUnion;

float FloatSwap (const float *f) {
	_FloatByteUnion out;

	out.f = *f;
	out.i = LongSwap(out.i);

	return out.f;
}

float FloatNoSwap (const float *f)
{
	return *f;
}

/*
================
Swap_Init
================
*/
/*
void Swap_Init (void)
{
	byte	swaptest[2] = {1,0};

// set the byte swapping variables in a portable manner	
	if ( *(short *)swaptest == 1)
	{
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_BigLong64 = Long64Swap;
		_LittleLong64 = Long64NoSwap;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
	}
	else
	{
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongSwap;
		_BigLong64 = Long64NoSwap;
		_LittleLong64 = Long64Swap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
	}

}
*/

/*
============================================================================

PARSING

============================================================================
*/

static	int		com_lines;

int COM_GetCurrentParseLine( void )
{
	return com_lines;
}

char *COM_Parse( const char **data_p )
{
	return COM_ParseExt( data_p, qtrue );
}

/*
==============
COM_Parse

Parse a token out of a string
Will never return NULL, just empty strings

If "allowLineBreaks" is qtrue then an empty
string will be returned if the next token is
a newline.
==============
*/
static const char *SkipWhitespace( const char *data, qboolean *hasNewLines ) {
	int c;

	while( (c = *data) <= ' ') {
		if( !c ) {
			return NULL;
		}
		if( c == '\n' ) {
			com_lines++;
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}

int COM_Compress( char *data_p ) {
	char *in, *out;
	int c;
	qboolean newline = qfalse, whitespace = qfalse;

	in = out = data_p;
	if (in) {
		while ((c = *in) != 0) {
			// skip double slash comments
			if ( c == '/' && in[1] == '/' ) {
				while (*in && *in != '\n') {
					in++;
				}
			// skip /* */ comments
			} else if ( c == '/' && in[1] == '*' ) {
				while ( *in && ( *in != '*' || in[1] != '/' ) ) 
					in++;
				if ( *in ) 
					in += 2;
                        // record when we hit a newline
                        } else if ( c == '\n' || c == '\r' ) {
                            newline = qtrue;
                            in++;
                        // record when we hit whitespace
                        } else if ( c == ' ' || c == '\t') {
                            whitespace = qtrue;
                            in++;
                        // an actual token
			} else {
                            // if we have a pending newline, emit it (and it counts as whitespace)
                            if (newline) {
                                *out++ = '\n';
                                newline = qfalse;
                                whitespace = qfalse;
                            } if (whitespace) {
                                *out++ = ' ';
                                whitespace = qfalse;
                            }
                            
                            // copy quoted strings unmolested
                            if (c == '"') {
                                    *out++ = (char)c;
                                    in++;
                                    for( ; ; )
									{
                                        c = *in;
                                        if (c && c != '"') {
                                            *out++ = (char)c;
                                            in++;
                                        } else {
                                            break;
                                        }
                                    }
                                    if (c == '"') {
                                        *out++ = (char)c;
                                        in++;
                                    }
                            }
							else
							{
                                *out = (char)c;
                                out++;
                                in++;
                            }
			}
		}
	}
	*out = 0;
	return out - data_p;
}

char *COM_ParseExt( const char **data_p, qboolean allowLineBreaks )
{
	static	char	com_token_buffer[MAX_TOKEN_CHARS];
	static	char *	com_token = com_token_buffer;

	int c = 0, len;
	int size;
	qboolean hasNewLines = qfalse;
	const char *data;
	char * r;

	goto start;

start_over:
	if ( len == sizeof(com_token_buffer) ) {
		com_token[ len-1 ] = '\0';
		goto done;
	}
	com_token = com_token_buffer;

start:
	size = sizeof(com_token_buffer)-(com_token-com_token_buffer)-1;
	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data )
		goto done;

	for( ; ; )
	{
		// skip whitespace
		data = SkipWhitespace( data, &hasNewLines );
		if( !data )
			goto done;

		if( hasNewLines && !allowLineBreaks )
			goto done;

		c = *data;

		if( c == '/' && data[1] == '/' )
		{
			// skip double slash comments

			data += 2;

			while( *data && *data != '\n' )
				data++;
		}
		else if( c == '/' && data[1] == '*' ) 
		{
			// skip /* */ comments

			data += 2;
			while( *data && (*data != '*' || data[1] != '/') )
				data++;
			
			if( *data ) 
				data += 2;
		}
		else
		{
			break;
		}
	}

	// handle quoted strings
	if( c == '\"' || c == '\'' )
	{
		int quote = c;
		data++;
		for( ; ; )
		{
			c = *data++;
			if( c == quote || !c )
				goto done;

			com_token[len] = (char)c;
			len++;

			if ( len >= size )
				goto start_over;
		}
	}

	// parse a regular word
	do
	{
		com_token[len] = (char)c;
		len++;

		if ( len >= size )
			goto start_over;

		data++;
		c = *data;

		if( c == '\n' )
			com_lines++;
	} while( c > 32 );

done:
	com_token[len] = 0;

	*data_p = (char*)data;
	r = com_token;

	com_token += (len+1);

	return r;
}


#if 0
// no longer used
/*
===============
COM_ParseInfos
===============
*/
int COM_ParseInfos( char *buf, int max, char infos[][MAX_INFO_STRING] ) {
	char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];

	count = 0;

	while ( 1 ) {
		token = COM_Parse( &buf );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		infos[count][0] = 0;
		while ( 1 ) {
			token = COM_ParseExt( &buf, qtrue );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );
			if ( !token[0] ) {
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( infos[count], key, token );
		}
		count++;
	}

	return count;
}
#endif


/*
==================
COM_MatchToken
==================
*/
void COM_MatchToken( const char **buf_p, char *match ) {
	char	*token;

	token = COM_Parse( buf_p );
	if ( strcmp( token, match ) ) {
		Com_Error( ERR_DROP, "MatchToken: %s != %s", token, match );
	}
}


/*
=================
SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
void SkipBracedSection (const char **program) {
	char			*token;
	int				depth;

	depth = 0;
	do {
		token = COM_ParseExt( program, qtrue );
		if( token[1] == 0 ) {
			if( token[0] == '{' ) {
				depth++;
			}
			else if( token[0] == '}' ) {
				depth--;
			}
		}
	} while( depth && *program );
}

/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine ( const char **data ) {
	const char	*p;
	int		c;

	p = *data;
	while ( (c = *p++) != 0 ) {
		if ( c == '\n' ) {
			com_lines++;
			break;
		}
	}

	*data = p;
}


void Parse1DMatrix (const char **buf_p, int x, float *m) {
	char	*token;
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < x ; i++) {
		token = COM_Parse(buf_p);
		m[i] = fatof(token);
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse2DMatrix (const char **buf_p, int y, int x, float *m) {
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < y ; i++) {
		Parse1DMatrix (buf_p, x, m + i * x);
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse3DMatrix (const char **buf_p, int z, int y, int x, float *m) {
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < z ; i++) {
		Parse2DMatrix (buf_p, y, x, m + i * x*y);
	}

	COM_MatchToken( buf_p, ")" );
}


/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_isprint( int c )
{
	if ( c >= 0x20 && c <= 0x7E )
		return ( 1 );
	return ( 0 );
}

int Q_islower( int c )
{
	if (c >= 'a' && c <= 'z')
		return ( 1 );
	return ( 0 );
}

int Q_isupper( int c )
{
	if (c >= 'A' && c <= 'Z')
		return ( 1 );
	return ( 0 );
}

int Q_isalpha( int c )
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return ( 1 );
	return ( 0 );
}

char* Q_strrchr( const char* string, int c )
{
	char cc = (char)c;
	char *s;
	char *sp=(char *)0;

	s = (char*)string;

	while (*s)
	{
		if (*s == cc)
			sp = s;
		s++;
	}
	if (cc == 0)
		sp = s;

	return sp;
}

/*
=============
Q_strncpyz
 
Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz( char *dest, const char *src, int destsize ) {
  // bk001129 - also NULL dest
  if ( !dest ) {
    Com_Error( ERR_FATAL, "Q_strncpyz: NULL dest" );
  }
	if ( !src ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
	}
	if ( destsize < 1 ) {
		Com_Error(ERR_FATAL,"Q_strncpyz: destsize < 1" ); 
	}

	strncpy( dest, src, destsize-1 );
  dest[destsize-1] = 0;
}
                 
int Q_stricmpn (const char *s1, const char *s2, int n) {
	int		c1, c2;

	// bk001129 - moved in 1.17 fix not in id codebase
        if ( s1 == NULL ) {
           if ( s2 == NULL )
             return 0;
           else
             return -1;
        }
        else if ( s2==NULL )
          return 1;


	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}
		
		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);
	
	return 0;		// strings are equal
}

int Q_strncmp (const char *s1, const char *s2, int n) {
	int		c1, c2;
	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}
		
		if (c1 != c2) {
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);
	
	return 0;		// strings are equal
}

int Q_stricmp (const char *s1, const char *s2) {
	return (s1 && s2) ? Q_stricmpn (s1, s2, 99999) : -1;
}


char *Q_strlwr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = (char)tolower(*s);
		s++;
	}
    return s1;
}

char *Q_strupr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = (char)toupper(*s);
		s++;
	}
    return s1;
}

char * Q_strstr( const char * str, int size, const char * search )
{
	int	i,j;
	for ( i=0,j=0; i<size; i++ )
	{
		if ( str[ i ] == 13 )
			continue;

		if ( str[ i ] == search[ j ] )
			j++;
		else
		{
			i -= j;
			j = 0;
		}

		if ( search[ j ] == '\0' )
			return (char*)(str + i - j + 1);
	}

	return 0;
}

char * Q_stristr( const char * str, int size, const char * search )
{
	int	i,j;
	for ( i=0,j=0; i<size; i++ )
	{
		if ( str[ i ] == 13 )
			continue;

		if ( toupper(str[ i ]) == toupper(search[ j ]) )
			j++;
		else
		{
			i -= j;
			j = 0;
		}

		if ( search[ j ] == '\0' )
			return (char*)(str + i - j + 1);
	}

	return 0;
}


// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, int size, const char *src ) {
	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
	}
	Q_strncpyz( dest + l1, src, size - l1 );
}

void Q_StrListCat( char *dst, size_t dstLen, const char *item )
{
	if( dst[0] )
		Q_strcat( dst, dstLen, ", " );

	Q_strcat( dst, dstLen, item );
}

int Q_PrintStrlen( const char *string ) {
	int			len;
	const char	*p;

	if( !string ) {
		return 0;
	}

	len = 0;
	p = string;
	while( *p ) {
		if( Q_IsColorString( p ) ) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}


char *Q_CleanStr( char *string ) {
	char*	d;
	char*	s;
	int		c;

	s = string;
	d = string;
	while ((c = *s) != 0 ) {
		if ( Q_IsColorString( s ) ) {
			s++;
		}		
		else if ( c >= 0x20 && c <= 0x7E ) {
			*d++ = (char)c;
		}
		s++;
	}
	*d = '\0';

	return string;
}


int QDECL Com_sprintf( char *dest, int size, const char *fmt, ...) {
	int len;
	va_list		argptr;
	va_start (argptr,fmt);
	len = vsnprintf( dest, size, fmt, argptr );
	va_end (argptr);
	return len;
}

char * Q_strcpy_ringbuffer( char * buffer, int size, char * first, char * last, const char * s ) {

	int i;

	if ( !first ) {
		first = buffer + size;
	} else {
		while ( *first ) first++;
		first++;
	}

	if ( !last ) {
		last = buffer;
	} else {
		while ( *last ) last++;
		last++;
	}

	if ( last == first ) {
		first = buffer + ((last-buffer)-1)%size;
	}

	for ( i=0; ; i++ ) {

		if ( last + i >= buffer + size ) {
			last = buffer;
			i = 0;
		}

		if ( last + i == first ) {
			return 0;
		}

		last[ i ] = s[ i ];

		if ( s[ i ] == '\0' )
			break;
	}

	return last;
}



/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char	* QDECL va( char *format, ... ) {

	static char	buffer[ 2048 ];	// in case va is called by nested functions
	static char	*buf = buffer;

	va_list		argptr;
	int			len;
	int			max = sizeof(buffer) - (buf-buffer);
	char *		ret;

	va_start (argptr, format);

	for ( ;; ) {
		len = vsnprintf( buf, max, format,argptr);

		if ( len >= 0 && len < max ){
			break;
		}

		if ( max == sizeof(buffer) ) {
			Com_Error( ERR_FATAL, "va buffer overflow" );
			break;
		}

		//	start again at beginning of buffer
		buf = buffer;
		max = sizeof(buffer);
	}

	va_end (argptr);

	ret = buf;
	buf += len+1;

	return ret;
}

/*
============
Com_TruncateLongString

Assumes buffer is atleast TRUNCATE_LENGTH big
============
*/
void Com_TruncateLongString( char *buffer, const char *s )
{
	int length = strlen( s );

	if( length <= TRUNCATE_LENGTH )
		Q_strncpyz( buffer, s, TRUNCATE_LENGTH );
	else
	{
		Q_strncpyz( buffer, s, ( TRUNCATE_LENGTH / 2 ) - 3 );
		Q_strcat( buffer, TRUNCATE_LENGTH, " ... " );
		Q_strcat( buffer, TRUNCATE_LENGTH, s + length - ( TRUNCATE_LENGTH / 2 ) + 3 );
	}
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
char *Info_ValueForKey( const char *s, const char *key ) {
	char			pkey[BIG_INFO_KEY];
	static	char	buffer[ BIG_INFO_VALUE * 2 ];	// use two buffers so compares work without stomping on each other
	static	char*	value = buffer;
	char	*o;
	
	if ( !s || !key ) {
		return "";
	}

	if ( value-buffer >= BIG_INFO_STRING ) {
		value = buffer;
	}

	if (*s == '\\')
		s++;
	for( ; ; )
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o++ = 0;

		if (!Q_stricmp (key, pkey) )
		{
			char * r = value;
			value = o;
			return r;
		}

		if (!*s)
			break;
		s++;
	}

	return "";
}


/*
===================
Info_NextPair

Used to itterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair( const char **head, char *key, char *value ) {
	char	*o;
	const char	*s;

	s = *head;

	if ( *s == '\\' ) {
		s++;
	}
	key[0] = 0;
	value[0] = 0;

	o = key;
	while ( *s != '\\' ) {
		if ( !*s ) {
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	o = value;
	while ( *s != '\\' && *s ) {
		*o++ = *s++;
	}
	*o = 0;

	*head = s;
}


/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey( char *s, const char *key ) {
	char	*start;
	char	pkey[MAX_INFO_KEY];
	char	value[MAX_INFO_VALUE];
	char	*o;

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey: oversize infostring" );
	}

	if (strchr (key, '\\')) {
		return;
	}

	for( ; ; )
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			strcpy (start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}

/*
===================
Info_RemoveKey_Big
===================
*/
void Info_RemoveKey_Big( char *s, const char *key ) {
	char	*start;
	char	pkey[BIG_INFO_KEY];
	char	value[BIG_INFO_VALUE];
	char	*o;

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey_Big: oversize infostring" );
	}

	if (strchr (key, '\\')) {
		return;
	}

	for( ; ; )
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			strcpy (start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}




/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate( const char *s ) {
	if ( strchr( s, '\"' ) ) {
		return qfalse;
	}
	if ( strchr( s, ';' ) ) {
		return qfalse;
	}
	return qtrue;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey( char *s, const char *key, const char *value ) {
	char	newi[MAX_INFO_STRING];

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	if (strchr (key, '\\') || strchr (value, '\\'))
	{
		Com_Error( ERR_FATAL, "Can't use keys or values with a \\\n");
		return;
	}

	if (strchr (key, ';') || strchr (value, ';'))
	{
		Com_Error( ERR_FATAL, "Can't use keys or values with a semicolon\n");
		return;
	}

	if (strchr (key, '\"') || strchr (value, '\"'))
	{
		Com_Error( ERR_FATAL, "Can't use keys or values with a \"\n");
		return;
	}

	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) >= MAX_INFO_STRING)
	{
		Com_Error( ERR_FATAL, "Info string length exceeded\n");
		return;
	}

	strcat (newi, s);
	strcpy (s, newi);
}

/*
==================
Info_SetValueForKey_Big

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey_Big( char *s, const char *key, const char *value ) {
	char	newi[BIG_INFO_STRING];

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	if (strchr (key, '\\') || strchr (value, '\\'))
	{
		Com_Error( ERR_FATAL, "Can't use keys or values with a \\\n");
		return;
	}

	if (strchr (key, ';') || strchr (value, ';'))
	{
		Com_Error( ERR_FATAL, "Can't use keys or values with a semicolon\n");
		return;
	}

	if (strchr (key, '\"') || strchr (value, '\"'))
	{
		Com_Error( ERR_FATAL, "Can't use keys or values with a \"\n");
		return;
	}

	Info_RemoveKey_Big (s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) >= BIG_INFO_STRING)
	{
		Com_Error( ERR_FATAL, "BIG Info string length exceeded\n");
		return;
	}

	strcat (s, newi);
}




//====================================================================

/*
==================
Com_CharIsOneOfCharset
==================
*/
static qboolean Com_CharIsOneOfCharset( char c, char *set )
{
	size_t i;
	size_t len = strlen( set );

	for( i = 0; i < len; i++ )
	{
		if( set[ i ] == c )
			return qtrue;
	}

	return qfalse;
}

/*
==================
Com_SkipCharset
==================
*/
char *Com_SkipCharset( char *s, char *sep )
{
	char	*p = s;

	while( p )
	{
		if( Com_CharIsOneOfCharset( *p, sep ) )
			p++;
		else
			break;
	}

	return p;
}

/*
==================
Com_SkipTokens
==================
*/
char *Com_SkipTokens( char *s, int numTokens, char *sep )
{
	int		sepCount = 0;
	char	*p = s;

	while( sepCount < numTokens )
	{
		if( Com_CharIsOneOfCharset( *p++, sep ) )
		{
			sepCount++;
			while( Com_CharIsOneOfCharset( *p, sep ) )
				p++;
		}
		else if( *p == '\0' )
			break;
	}

	if( sepCount == numTokens )
		return p;
	else
		return s;
}

float QDECL fatof( const char *string )
{
	float	sign;
	float	value;
	int		c;


	// skip whitespace
	while ( *string <= ' ' ) {
		if ( !*string ) {
			return 0;
		}
		string++;
	}

	// check sign
	switch ( *string ) {
	case '+':
		string++;
		sign = 1.0f;
		break;
	case '-':
		string++;
		sign = -1.0f;
		break;
	default:
		sign = 1.0f;
		break;
	}

	// read digits
	value = 0.0f;
	c = string[0];
	if ( c != '.' ) {
		for( ; ; )
		{
			c = *string++;
			if ( c < '0' || c > '9' ) {
				break;
			}
			c -= '0';
			value = value * 10.0f + c;
		}
	} else {
		string++;
	}

	// check for decimal point
	if ( c == '.' ) {
		float fraction;

		fraction = 0.1f;
		for( ; ; )
		{
			c = *string++;
			if ( c < '0' || c > '9' ) {
				break;
			}
			c -= '0';
			value += c * fraction;
			fraction *= 0.1f;
		}

	}

	// not handling 10e10 notation...

	return value * sign;
}

int fn_buffer( char * buffer, int number, int flags ) {

	int digits[ 16 ];
	int i,c = 0,n = 0;
	int negative = number < 0;
	bool hasFormatting = false;

	if ( (flags&FN_DISABLED) )
	{
		buffer[ c++ ] = '^';
		buffer[ c++ ] = '8';
		hasFormatting = true;

	} else if ( (negative && (flags&FN_CURRENCY)) || (flags&FN_BAD) )	// make red
	{
		buffer[ c++ ] = '^';
		buffer[ c++ ] = '1';
		hasFormatting = true;

	} else if ( flags&FN_GOOD )
	{
		buffer[ c++ ] = '^';
		buffer[ c++ ] = '2';
		hasFormatting = true;
	}


	if ( negative )
	{
		number = -number;
		buffer[ c++ ] = '-';
	}

	if ( (flags&FN_CURRENCY) )
		buffer[ c++ ] = '$';

	if ( number == 0 )
	{
		buffer[ c++ ] = '0';
	}

	if ( (flags&FN_TIME) )
	{
		int		hours	= number / 3600;
		int		minutes	= (number % 3600) / 60;
		int		seconds	= number % 60;

		// hours
		for ( n=0; hours>0; n++ )
		{
			digits[ n ] = hours%10;
			hours /= 10;
		}
		if ( n > 0 )
		{
			for ( i=n-1; i>=0; i-- )
				buffer[ c++ ] = '0'+digits[i];
			buffer[ c++ ] = ':';
		}

		//	minutes
		digits[ 0 ] = digits[ 1 ] = 0;
		for ( n=0; minutes>0; n++ )
		{
			digits[ n ] = minutes%10;
			minutes /= 10;
		}

		if ( n > 0 )
		{
			for ( i=1; i>=0; i-- )
				buffer[ c++ ] = '0'+digits[i];
		}

		buffer[ c++ ] = ':';
		//	seconds
		digits[ 0 ] = digits[ 1 ] = 0;
		for ( n=0; seconds>0; n++ )
		{
			digits[ n ] = seconds%10;
			seconds /= 10;
		}
		for ( i=1; i>=0; i-- )
			buffer[ c++ ] = '0'+digits[i];


	} else
	{
		while ( number > 0 )
		{
			digits[ n++ ] = number%10;
			number /= 10;
		}
		digits[ n ] = 0;

		if ( (flags&FN_SHORT) && n>5 )
		{
			for ( i=n-1; i>=n-3; i-- )
			{
				buffer[ c++ ] = '0'+digits[i];

				if ( i > n-3 && (i % 3)==0 )
					buffer[ c++ ] = '.';
			}
			switch( n )
			{
			case 6: buffer[ c++ ] = 'k'; break;
			case 7:
			case 8:
			case 9: buffer[ c++ ] = 'M'; break;
			case 10:
			case 11:
			case 12: buffer[ c++ ] = 'B'; break;
			}
		} else
		{
			for ( i=n-1; i>=0; i-- )
			{
				buffer[ c++ ] = '0'+digits[i];

				if ( i > 0 && (i % 3)==0 && !(flags&FN_PLAIN) )
					buffer[ c++ ] = ',';
			}

			if ( flags&FN_RANK  )
			{
				int d = ( digits[1] == 1 )?4:digits[0];
				switch( d )
				{
					case 1:	buffer[ c++ ] = 's';
							buffer[ c++ ] = 't'; break;
					case 2:	buffer[ c++ ] = 'n';
							buffer[ c++ ] = 'd'; break;
					case 3: buffer[ c++ ] = 'r';
							buffer[ c++ ] = 'd'; break;
					default:
							buffer[ c++ ] = 't';
							buffer[ c++ ] = 'h'; break;
				}
			}
		}
	}

	if ( flags&FN_PERCENT ) {
		buffer[ c++ ] = '%';
	}

	if( hasFormatting )
	{
		buffer[c++] = '^';
		buffer[c++] = '-';
	}

	buffer[ c++ ] = '\0';

	return c;
}

char* fn( int number, int flags )
{
	static char buff[ 4 ][ 64 ];	// boo
	static int	buffIndex = 0;
	char * buffer;

	// this hackery is meant to allow this to be called a few times 
	// over and not stomp all over each other.
	buffIndex = (buffIndex+1)%4;
	buffer = &buff[ buffIndex ][0];

	fn_buffer( buffer, number, flags );
	return buffer;
}

bool Q_CleanPlayerName( const char * in, char *out, int outSize )
{
	int len, i;
	int numColorChanges;

	numColorChanges = 0;

	for( len = 0, i = 0; len < outSize - 1; i++ )
	{
		if( !in[i] )
			break;

		if( Q_IsColorString( in + i ) )
		{
			if( in[i + 1] == COLOR_BLACK )
			{
				//don't allow black
				i++;
				continue;
			}

			if( len >= outSize - 2 )
				//not enough room
				break;

			numColorChanges++;

			out[len++] = Q_COLOR_ESCAPE;
			out[len++] = in[i + 1];
			i++;

			continue;
		}

		if( in[i] == ' ' )
		{
			if( !len )
				//no leading spaces
				continue;
			else if( out[len - 1] == ' ' )
				//don't allow more than once consecutive space
				continue;
		}

		out[len++] = in[i];
	}

	for( ; ; )
	{
		if( len >= 1 && out[len - 1] == ' ' )
		{
			//don't allow any trailing spaces
			len -= 1;
		}
		if( len >= 2 && Q_IsColorString( out + len - 2 ) )
		{
			//don't allow any trailing color strings
			len -= 2;
			numColorChanges--;
		}
		else
			break;
	}

	if( numColorChanges )
	{
		//must append a return to default color at the end of the string

		if( len < outSize - 2 )
		{
			//good to append, no adjustments
		}
		else
		{
			if( len < outSize - 1 )
			{
				//we know that the last thing in the string isn't a color
				//sequence so we can freely truncate the last character
				len -= 1;
			}
			else
			{
				//we have to overwrite the last two characters in the string

				if( Q_IsColorString( out + len - 3 ) )
					//second last char is the end of a color string,
					//replace the whole color string to avoid "^^-"
					len -= 3;
				else
					//just chop two characters
					len -= 2;
			}
		}

		//append the "^-" to restore the default color
		out[len++] = Q_COLOR_ESCAPE;
		out[len++] = COLOR_DEFAULT;
	}
	
	out[len] = 0;

	if( !len )
		//don't allow empty player names
		Q_strncpyz( out, "UnnamedPlayer", outSize );

	return strcmp( in, out ) != 0;
}
