/*
===========================================================================
Copyright (C) 2007 HermitWorks Entertainment Corporation

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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#include "sql.h"

#ifdef _DEBUG
#define SQL_STR_MAGIC 0xDEADBEEF
#endif

#define SQL_STR_ALLOC_PAGE_SIZE 4096

typedef struct sql_str
{
#ifdef _DEBUG
	unsigned int	magic;
#endif

	struct sql_str	*next;
	char			str[1]; //inline char array
} sql_str;

static sql_str* sql_str_getrec( const char *str )
{
	sql_str *rec = (sql_str*)(str - offsetof( sql_str, str ));

#ifdef _DEBUG
	ASSERT( rec->magic == SQL_STR_MAGIC );
#endif

	return rec;
}

struct sql_strings
{
	int				alloc_tag;
	unsigned int	hash_size;

	char			*alloc_page;
	size_t			alloc_page_left;
	
#ifdef _DEBUG
	int				num_pools;
	int				bytes_used;
	int				num_strings;
	int				num_already_interned;
	int				bytes_saved;
	int				num_collisions;
#endif

	sql_str			*hash_entries[1]; //inline array
};

static unsigned long int sql_str_hash( const char *str )
{
	int c;
	unsigned long int ret = 5381;

	for( c = *str; c; c = *++str )
		ret = ((ret << 5) + ret) + c;

	return ret;
}

static unsigned long int sql_str_hash_n( const char *str, unsigned int n )
{
	int c;
	unsigned long int ret = 5381;

	for( c = *str; c && n--; c = *++str )
		ret = ((ret << 5) + ret) + c;

	return ret;
}

//initializes the string pool, set hash_size to an adequately sized prime for efficiency
//(should be at least a third of the number of strings you expect to intern at once)
sql_strings* sql_str_init_strings( unsigned int hash_size, int alloc_tag )
{
	sql_strings *ret;

	size_t cb = (size_t)&((sql_strings*)0)->hash_entries[hash_size];
	ret = Z_TagMalloc( cb, alloc_tag );
	
	memset( ret, 0, cb );
	ret->alloc_tag = alloc_tag;
	ret->hash_size = hash_size;

	ret->alloc_page = Z_TagMalloc( SQL_STR_ALLOC_PAGE_SIZE, alloc_tag );
	ret->alloc_page_left = SQL_STR_ALLOC_PAGE_SIZE;

#ifdef _DEBUG
	ret->num_pools = 1;
#endif

	return ret;
}

//returns a pointer to a pointer to the sql_str for the string,
//if the string doesn't exist *ret == NULL and is where the string should be inserted
static sql_str** sql_str_find( sql_strings *strs, const char *str )
{
	sql_str **s;
	unsigned long int h = sql_str_hash( str ) % strs->hash_size;
	
	s = strs->hash_entries + h;
	while( *s )
	{
		if( (*s)->str == str || strcmp( (*s)->str, str ) == 0 )
			break;

		s = &(*s)->next;
	}

	return s;
}

static sql_str** sql_str_find_n( sql_strings *strs, const char *str, unsigned int n )
{
	sql_str **s;
	unsigned long int h = sql_str_hash_n( str, n ) % strs->hash_size;
	
	s = strs->hash_entries + h;
	while( *s )
	{
		if( (*s)->str == str || ((*s)->str[n] == 0 && strncmp( (*s)->str, str, n ) == 0) )
			break;

		s = &(*s)->next;
	}

	return s;
}

//alloc a sql_str* for an n-char string (n does *not* include the null terminator)
static sql_str* sql_str_alloc_str( sql_strings *strs, const char *str, unsigned int n )
{
	sql_str *ret;

	size_t cb = (size_t)&((sql_str*)0)->str[n + 1];
	cb = cb + ((sizeof( void* ) - (cb & (sizeof( void* ) - 1))) & (sizeof( void* ) - 1));

	assert( cb < SQL_STR_ALLOC_PAGE_SIZE ); //holy crap that's a big string...

	if( cb > strs->alloc_page_left )
	{
		//start a new page

#ifdef _DEBUG
		strs->num_pools++;
#endif

		strs->alloc_page = Z_TagMalloc( SQL_STR_ALLOC_PAGE_SIZE, strs->alloc_tag );
		strs->alloc_page_left = SQL_STR_ALLOC_PAGE_SIZE;
	}

	ret = (sql_str*)strs->alloc_page;
	strs->alloc_page += cb;
	strs->alloc_page_left -= cb;

	ret->next = NULL;
	memcpy( ret->str, str, n );
	ret->str[n] = 0;

#ifdef _DEBUG
	ret->magic = SQL_STR_MAGIC;
	assert( strlen( ret->str ) == n );
	assert( strncmp( ret->str, str, n ) == 0 );

	strs->bytes_used += cb;
#endif

	return ret;
}

//add a string to the sql interned strings table - if it's already there it gets addref'd
const char*	sql_str_intern( sql_strings *strs, const char *str )
{
	sql_str **s = sql_str_find( strs, str );

	if( *s )
	{
#ifdef _DEBUG
		strs->num_already_interned++;
		strs->bytes_saved += strlen( str );
#endif

		return (*s)->str;
	}
	else
	{
		sql_str *ns = sql_str_alloc_str( strs, str, strlen( str ) );

#ifdef _DEBUG
		strs->num_strings++;
		if( s < strs->hash_entries || s >= strs->hash_entries + strs->hash_size )
			strs->num_collisions++;
#endif

		*s = ns;
		return ns->str;
	}
}

//same as sql_str_intern, only stop reading str at n chars
const char*	sql_str_intern_n( sql_strings *strs, const char *str, unsigned int n )
{
	sql_str **s = sql_str_find_n( strs, str, n );

	if( *s )
	{
#ifdef _DEBUG
		strs->num_already_interned++;
		strs->bytes_saved += n;
#endif

		return (*s)->str;
	}
	else
	{
		sql_str *ns = sql_str_alloc_str( strs, str, n );

#ifdef _DEBUG
		strs->num_strings++;
		if( s < strs->hash_entries || s >= strs->hash_entries + strs->hash_size )
			strs->num_collisions++;
#endif

		*s = ns;
		return ns->str;
	}
}

//if a string has been interned, return the interned pointer else return null - string is NOT addref'd
const char*	sql_str_is_interned( sql_strings *strs, const char *str )
{
	sql_str **s = sql_str_find( strs, str );
	return (*s) ? (*s)->str : NULL;
}

//same as sql_str_is_interned, only stop reading str at n chars
const char*	sql_str_is_interned_n( sql_strings *strs, const char *str, unsigned int n  )
{
	sql_str **s = sql_str_find_n( strs, str, n );
	return (*s) ? (*s)->str : NULL;
}