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

static int voteTime;
//
//	this is called from the server to force a popup on the clients.  the client who called the
//	vote gets the watchvote menu, the rest must vote so they get the castvote menu.
//
void UI_Vote( int voteCmd, const char * cmd )
{
	switch( voteCmd )
	{
	case 2: //vote error
		{
			const char *msg = NULL;

			switch( SWITCHSTRING( cmd ) )
			{
			case CS( 't', 'r', 'a', 'v' ): //travel_*
				switch( SWITCHSTRING( cmd + 7 )  )
				{
				case CS( 'n', 'o', 't', 'r' ): //travel_notready
					msg = "Not everyone is ready to travel.";
					break;

				case CS( 'n', 'o', 't', 'i' ): //travel_notime
					msg = "Not enough time to travel there.";
					break;

				case CS( 'i', 'n', 'v', 'a' ): //travel_invalid
					msg = "Invalid travel.";

				default:
					msg = "Error trying to call travel vote.";
					break;
				}
				break;

			case CS( 'v', 'o', 't', 'e' ): //vote_*
				switch( SWITCHSTRING( cmd + 5 ) )
				{
				case CS( 'i', 'n', 'p', 'r' ): //vote_inprogress
					msg =  "A vote is already in progress.";
					break;

				case CS( 'm', 'a', 'x', 'v' ): //vote_maxvotes
					msg = "You have called the maximum number of votes.";
					break;

				case CS( 's', 'p', 'e', 'c' ): //vote_spectating
					msg = "Not allowed to call a vote as spectator.";
					break;

				case CS( 's', 'o', 'o', 'n' ): //vote_soon
					msg = "Too soon to call a vote, try again in a few seconds."; //ToDo: pull that last param in
					break;

				case CS( 'i', 'n', 'v', 'a' ): //vote_invalidstring
					msg = "Invalid vote string.";
					break;

				case CS( 'n', 'o', 't', 'h' ): //vote_nothub
					msg = "You can only call that kind of vote from a trading zone.";
					break;

				default:
					msg = "Generic voting error.";
					break;
				}
				break;

			default:
				msg = cmd;
				break;
			}

			if( msg )
			{
				trap_Cvar_Set( "ui_errorMessage", msg );
				Menus_OpenByName( "error_popmenu" );
			}
			
		}
		break;
	}
}
