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


stmtInfo_t * sql_format_parse( sqlInfo_t * db, const char ** s )
{
	//	parse select statements
	parseInfo_t	pi = { 0 };
	formatInfo_t * format = sql_alloc( db, sizeof(formatInfo_t) );

	format->type = SQL_FORMAT;

	pi.db		= db;
	pi.flags	= PARSE_STRINGLITERAL;

	format->print = parse_expression( s, &pi );

	ASSERT( pi.rt == STRING );
	ASSERT( pi.more == 0 );

	return (stmtInfo_t*)format;
}


const char * sql_format_work( sqlInfo_t * db, formatInfo_t * format )
{
	return (format)?sql_eval( db, format->print, 0, 0, 0, 0, 0, 0 ).string:"";
}
