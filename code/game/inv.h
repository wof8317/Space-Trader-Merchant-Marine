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

#define INVENTORY_NONE				0
//armor
#define INVENTORY_ARMOR				1
//weapons
#define INVENTORY_GAUNTLET			4
#define INVENTORY_SHOTGUN			5
#define INVENTORY_PISTOL		6
#define INVENTORY_GRENADE	7
#define INVENTORY_ROCKETLAUNCHER	8
#define INVENTORY_HIGHCAL           9
#define INVENTORY_ASSAULT			11
#define INVENTORY_GRAPPLINGHOOK		14
#define INVENTORY_C4		16
//ammo
#define INVENTORY_SHELLS			21
#define INVENTORY_BULLETS			22
#define INVENTORY_GRENADES			23
#define INVENTORY_CLIPS				24
#define INVENTORY_HIGHCALAMMO		25
#define INVENTORY_ROCKETS			26
#define INVENTORY_MINES				27
//powerups
#define INVENTORY_HEALTH			29
#define INVENTORY_TELEPORTER		30
#define INVENTORY_MEDKIT			31
#define INVENTORY_KAMIKAZE			32
#define INVENTORY_PORTAL			33
#define INVENTORY_INVULNERABILITY	34
#define INVENTORY_QUAD				35
#define INVENTORY_ENVIRONMENTSUIT	36
#define INVENTORY_HASTE				37
#define INVENTORY_INVISIBILITY		38
#define INVENTORY_REGEN				39
#define INVENTORY_FLIGHT			40
#define INVENTORY_SCOUT				41
#define INVENTORY_GUARD				42
#define INVENTORY_DOUBLER			43
#define INVENTORY_AMMOREGEN			44

#define INVENTORY_REDFLAG			45
#define INVENTORY_BLUEFLAG			46
#define INVENTORY_NEUTRALFLAG		47
#define INVENTORY_REDCUBE			48
#define INVENTORY_BLUECUBE			49
//enemy stuff
#define ENEMY_HORIZONTAL_DIST		200
#define ENEMY_HEIGHT				201
#define NUM_VISIBLE_ENEMIES			202
#define NUM_VISIBLE_TEAMMATES		203

//item numbers (make sure they are in sync with bg_itemlist in bg_misc.c)
#define MODELINDEX_ARMORSHARD		1
#define MODELINDEX_ARMORCOMBAT		2
#define MODELINDEX_ARMORBODY		3
#define MODELINDEX_HEALTHSMALL		4
#define MODELINDEX_HEALTH			5
#define MODELINDEX_HEALTHLARGE		6
#define MODELINDEX_HEALTHMEGA		7

#define MODELINDEX_GAUNTLET			8
#define MODELINDEX_SHOTGUN			9
#define MODELINDEX_PISTOL		10
#define MODELINDEX_GRENADELAUNCHER	11
#define MODELINDEX_ROCKETLAUNCHER	12
#define MODELINDEX_GRAPPLINGHOOK	16

#define MODELINDEX_STASH      17
#define MODELINDEX_MONEYBAG   18

#define MODELINDEX_SHELLS			19
#define MODELINDEX_BULLETS			20
#define MODELINDEX_GRENADES			21
#define MODELINDEX_ROCKETS			22

#define MODELINDEX_TELEPORTER		25
#define MODELINDEX_MEDKIT			26
#define MODELINDEX_QUAD				27
#define MODELINDEX_ENVIRONMENTSUIT	28
#define MODELINDEX_HASTE			29
#define MODELINDEX_INVISIBILITY		30
#define MODELINDEX_REGEN			31
#define MODELINDEX_FLIGHT			32

#define MODELINDEX_REDFLAG			33
#define MODELINDEX_BLUEFLAG			34

// mission pack only defines

#define MODELINDEX_KAMIKAZE			35
#define MODELINDEX_PORTAL			36
#define MODELINDEX_INVULNERABILITY	37

#define MODELINDEX_MINES			39

#define MODELINDEX_SCOUT			41
#define MODELINDEX_GUARD			42
#define MODELINDEX_DOUBLER			43
#define MODELINDEX_AMMOREGEN		44

#define MODELINDEX_NEUTRALFLAG		45
#define MODELINDEX_REDCUBE			46
#define MODELINDEX_BLUECUBE			47

//
#define WEAPONINDEX_GAUNTLET			1
#define WEAPONINDEX_HIGHCAL             2
#define WEAPONINDEX_PISTOL			3
#define WEAPONINDEX_ASSAULT     4
#define WEAPONINDEX_SHOTGUN				5
#define WEAPONINDEX_GRENADE	    6
#define WEAPONINDEX_C4    7
#define WEAPONINDEX_GRAPPLING_HOOK		8
#define WEAPONINDEX_ROCKET_LAUNCHER   9
