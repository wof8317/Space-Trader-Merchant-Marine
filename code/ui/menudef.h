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

#define ITEM_TYPE_TEXT 0					// simple text
#define ITEM_TYPE_BUTTON 1					// button, basically text with a border 
#define ITEM_TYPE_EDITFIELD 4				// editable text, associated with a cvar
#define ITEM_TYPE_LISTBOX 6					// scrollable list  
#define ITEM_TYPE_MODEL 7					// model
#define ITEM_TYPE_OWNERDRAW 8				// owner draw, name specs what it is
#define ITEM_TYPE_NUMERICFIELD 9			// editable text, associated with a cvar
#define ITEM_TYPE_SLIDER 10					// mouse speed, volume, etc.
#define ITEM_TYPE_YESNO 11					// yes no cvar setting
#define ITEM_TYPE_MULTI 12					// multiple list setting, enumerated
#define ITEM_TYPE_BIND 13					// multiple list setting, enumerated
#define ITEM_TYPE_PROGRESS 14

    
#define ITEM_ALIGN_LEFT 0					// left alignment
#define ITEM_ALIGN_CENTER 1					// center alignment
#define ITEM_ALIGN_RIGHT 2					// right alignment
#define ITEM_ALIGN_JUSTIFY 3				// justify alignment
#define TEXT_ALIGN_NOCLIP 0x0080			// don't clip (or wrap) horizontal lines to the rectangle

#define ITEM_TEXTSTYLE_SHADOWED		2		// drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_OUTLINED		4		// drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_BLINK		8		// fast blinking
#define ITEM_TEXTSTYLE_ITALIC		16
#define ITEM_TEXTSTYLE_MULTILINE	32
#define ITEM_TEXTSTYLE_SMALLCAPS	64
                          
#define WINDOW_STYLE_EMPTY 0				// no background
#define WINDOW_STYLE_FILLED 1				// filled with background color
#define WINDOW_STYLE_SHADER   3				// gradient bar based on background color 
#define WINDOW_STYLE_CINEMATIC 5			// cinematic


#define MENU_TRUE 1							// uh.. true
#define MENU_FALSE 0						// and false

#define HUD_VERTICAL						0x00
#define HUD_HORIZONTAL						0x01

					
// owner draw types
// ideally these should be done outside of this file but
// this makes it much easier for the macro expansion to 
// convert them for the designers ( from the .menu files )
                      
#define UI_CROSSHAIR				501
#define UI_KEYBINDSTATUS			502
#define UI_TRAVELSCREEN				503
#define UI_TRAVEL_RECORD_BAR		504

#define UI_SHOPBUY_PRICEGRAPH		505
#define UI_UPGRADE_PRICEGRAPH		506

#define UI_PLAYER_SCOREBOARD		507
#define CG_SNIPER_ZOOM				509

#define UI_PLAYERSELECT_SLOT0_MODEL	528
#define UI_PLAYERSELECT_SLOT1_MODEL	529
#define UI_PLAYERSELECT_SLOT2_MODEL	530
#define UI_PLAYERSELECT_SLOT3_MODEL	531
#define UI_PLAYERSELECT_SLOT4_MODEL	532

#define	UI_PLAYER_MODEL				533		//currently selected character

#define UI_DRAW_PLANET				535

#define UI_CURRENT_MODEL			536		//whatever cvar model is

#define UI_TRAVEL_TIMEREMAININGDESC	540		//something along the lines of "YY/MM/DD
#define UI_TRAVEL_TIMEREMAININGVAL	541		//the actual numbers
#define UI_TRAVEL_BOTTOMBAR_TEXT	543
#define UI_TRAVEL_DISASTERCOMMENT	545

#define	UI_NEWS_TICKER				562

#define UI_RADAR					567
#define UI_BOSSHEALTH				568

#define UI_TRAVEL_DISASTERICON		571

#define BG_TOOLTIP					1007
#define BG_GLOBALSTATE				1019


//	owner text
#define UI_PLAYERSELECT_SLOT0_BTN	2518
#define UI_PLAYERSELECT_SLOT1_BTN	2519
#define UI_PLAYERSELECT_SLOT2_BTN	2520
#define UI_PLAYERSELECT_SLOT3_BTN	2521
#define UI_PLAYERSELECT_SLOT4_BTN	2522

#define UI_PLAYERSELECT_SLOT0_NAME	2523
#define UI_PLAYERSELECT_SLOT1_NAME	2524
#define UI_PLAYERSELECT_SLOT2_NAME	2525
#define UI_PLAYERSELECT_SLOT3_NAME	2526
#define UI_PLAYERSELECT_SLOT4_NAME	2527
#define UI_PLAYER_NAME				2534




#define VOICECHAT_GETFLAG			"getflag"				// command someone to get the flag
#define VOICECHAT_OFFENSE			"offense"				// command someone to go on offense
#define VOICECHAT_DEFEND			"defend"				// command someone to go on defense
#define VOICECHAT_DEFENDFLAG		"defendflag"			// command someone to defend the flag
#define VOICECHAT_PATROL			"patrol"				// command someone to go on patrol (roam)
#define VOICECHAT_CAMP				"camp"					// command someone to camp (we don't have sounds for this one)
#define VOICECHAT_FOLLOWME			"followme"				// command someone to follow you
#define VOICECHAT_RETURNFLAG		"returnflag"			// command someone to return our flag
#define VOICECHAT_FOLLOWFLAGCARRIER	"followflagcarrier"		// command someone to follow the flag carrier
#define VOICECHAT_YES				"yes"					// yes, affirmative, etc.
#define VOICECHAT_NO				"no"					// no, negative, etc.
#define VOICECHAT_ONGETFLAG			"ongetflag"				// I'm getting the flag
#define VOICECHAT_ONOFFENSE			"onoffense"				// I'm on offense
#define VOICECHAT_ONDEFENSE			"ondefense"				// I'm on defense
#define VOICECHAT_ONPATROL			"onpatrol"				// I'm on patrol (roaming)
#define VOICECHAT_ONCAMPING			"oncamp"				// I'm camping somewhere
#define VOICECHAT_ONFOLLOW			"onfollow"				// I'm following
#define VOICECHAT_ONFOLLOWCARRIER	"onfollowcarrier"		// I'm following the flag carrier
#define VOICECHAT_ONRETURNFLAG		"onreturnflag"			// I'm returning our flag
#define VOICECHAT_INPOSITION		"inposition"			// I'm in position
#define VOICECHAT_IHAVEFLAG			"ihaveflag"				// I have the flag
#define VOICECHAT_BASEATTACK		"baseattack"			// the base is under attack
#define VOICECHAT_ENEMYHASFLAG		"enemyhasflag"			// the enemy has our flag (CTF)
#define VOICECHAT_STARTLEADER		"startleader"			// I'm the leader
#define VOICECHAT_STOPLEADER		"stopleader"			// I resign leadership
#define VOICECHAT_TRASH				"trash"					// lots of trash talk
#define VOICECHAT_WHOISLEADER		"whoisleader"			// who is the team leader
#define VOICECHAT_WANTONDEFENSE		"wantondefense"			// I want to be on defense
#define VOICECHAT_WANTONOFFENSE		"wantonoffense"			// I want to be on offense
#define VOICECHAT_KILLINSULT		"kill_insult"			// I just killed you
#define VOICECHAT_TAUNT				"taunt"					// I want to taunt you
#define VOICECHAT_DEATHINSULT		"death_insult"			// you just killed me
#define VOICECHAT_KILLGAUNTLET		"kill_gauntlet"			// I just killed you with the gauntlet
#define VOICECHAT_PRAISE			"praise"				// you did something good
