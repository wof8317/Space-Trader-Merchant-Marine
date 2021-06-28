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
// bg_misc.c -- both games misc functions, all completely stateless

#include "../qcommon/q_shared.h"
#include "bg_public.h"

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) suspended
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
*/

gitem_t	bg_itemlist[] = 
{
	{
		NULL,
		NULL,
		{ NULL,
		NULL,
		0, 0} ,
/* icon */		NULL,
/* pickup */	NULL,
		0,
		0,
		0,
/* precache */ "",
/* sounds */ "",

		"",
	},	// leave index 0 alone

	//
	// ARMOR
	//

/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_armor_shard", 
		"sound/misc/ar1_pkup.ogg",
		{ "models/powerups/armor/shard.md3", 
		"models/powerups/armor/shard_sphere.md3",
		0, 0} ,
/* icon */		"icons/iconr_shard",
/* pickup */	"Armor Shard",
		5,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ "",

		"radarArmor"
	},

/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_armor_combat", 
		"sound/misc/ar2_pkup.ogg",
        { "models/misc/shieldgenerator.md3",
		0, 0, 0},
/* icon */		"icons/iconr_yellow",
/* pickup */	"Armor",
		50,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ "",

		"radarArmor"
	},

/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_armor_body", 
		"sound/misc/ar2_pkup.ogg",
        { "models/misc/shieldgenerator.md3",
		0, 0, 0},
/* icon */		"icons/iconr_red",
/* pickup */	"Heavy Armor",
		50,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ "",

		"radarArmor"
	},

	//
	// health
	//
/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_small",
		"sound/items/s_health.ogg",
        { "models/powerups/health/small_cross.md3", 
		"models/powerups/health/small_sphere.md3", 
		0, 0 },
/* icon */		"icons/iconh_green",
/* pickup */	"25 Health",
		25,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ "",

		"radarHealth"
	},

/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health",
		"sound/items/n_health.ogg",
        { "models/misc/medkit.md3", 
		0, 0, 0 },
/* icon */		"icons/iconh_yellow",
/* pickup */	"100 Health",
		100,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ "",

		"radarHealth"
	},

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_large",
		"sound/items/l_health.ogg",
        { "models/misc/medkit.md3", 
		0, 0, 0 },
/* icon */		"icons/iconh_red",
/* pickup */	"50 Health",
		50,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ "",

		"radarHealth"
	},

	//
	// WEAPONS 
	//

/*QUAKED weapon_gauntlet (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_gauntlet", 
		"sound/misc/w_pkup.ogg",
        { 0,
		0, 0, 0},
/* icon */		"icons/iconw_gauntlet",
/* pickup */	"Gauntlet",
		0,
		IT_WEAPON,
		WP_GAUNTLET,
/* precache */ "",
/* sounds */ "",

		"radarWeapon"
	},

/*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_shotgun", 
		"sound/misc/w_pkup.ogg",
        { "models/weapons/shotgun/shotgun.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_shotgun",
/* pickup */	"T_Shotgun",
		10,
		IT_WEAPON,
		WP_SHOTGUN,
/* precache */ "",
/* sounds */ "",

		"radarWeapon"
	},

/*QUAKED weapon_pistol (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_pistol", 
		"sound/misc/w_pkup.ogg",
        { "models/weapons/pistol/pistol.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_pistol",
/* pickup */	"T_Machine_Pistol",
		40,
		IT_WEAPON,
		WP_PISTOL,
/* precache */ "",
/* sounds */ "",

		"radarWeapon"
	},

/*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		/* changed the spawn name to disable grenades from spawning */
		"weapon_disablegrenadelauncher",
		"sound/misc/w_pkup.ogg",
        { "models/weapons/grenades/grenades.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_grenade",
/* pickup */	"Grenades",
		4,
		IT_WEAPON,
		WP_GRENADE,
/* precache */ "",
/* sounds */ "sound/weapons/grenade/hgrenb1a.ogg sound/weapons/grenade/hgrenb2a.ogg",

		"radarWeapon"
	},

/*QUAKED weapon_assault (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_assault", 
		"sound/misc/w_pkup.ogg",
        { "models/weapons/assault/assault.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_assault",
/* pickup */	"T_Assault_Rifle",
		50,
		IT_WEAPON,
		WP_ASSAULT,
/* precache */ "",
/* sounds */ "",

		"radarWeapon"
	},

/*QUAKED weapon_grapplinghook (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_grapplinghook",
		"sound/misc/w_pkup.ogg",
        { "models/weapons2/grapple/grapple.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_grapple",
/* pickup */	"Grappling Hook",
		0,
		IT_WEAPON,
		WP_GRAPPLING_HOOK,
/* precache */ "",
/* sounds */ "",

		"radarWeapon"
	},
/*QUAKED weapon_highcal (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
    {
        "weapon_highcal",
        "sound/misc/w_pkup.ogg",
        { "models/weapons/highcal/highcal.md3", 
        0, 0, 0},
/* icon */      "icons/iconw_highcal",
/* pickup */    "T_High_Cal",
        0,
        IT_WEAPON,
        WP_HIGHCAL,
/* precache */ "",
/* sounds */ "",

		"radarWeapon"
    },
/*QUAKED weapon_c4 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		/* change weapon_disablec4 to weapon_c4 to restore this */
		"weapon_disablec4", 
		"sound/misc/w_pkup.ogg",
        { "models/weapons/c4/c4.md3", 
		0, 0, 0},
/* icon */		"icons/iconw_c4",
/* pickup */	"C4",
		5,
		IT_WEAPON,
		WP_C4,
/* precache */ "",
/* sounds */ "sound/weapons/c4/wstbtick.ogg "
			"sound/weapons/c4/wstbactv.ogg "
			"sound/weapons/c4/wstbimpl.ogg "
			"sound/weapons/c4/wstbimpm.ogg "
			"sound/weapons/c4/wstbimpd.ogg "
			"sound/weapons/c4/wstbactv.ogg",

		"radarWeapon"
	},

/*QUAKED ammo_stash (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
    {
        "ammo_stash",
        "sound/misc/stash_pickup.ogg",
        { "models/misc/stash.md3", 
        0, 0, 0},
/* icon */      "icons/icona_stash",
/* pickup */    "Hidden Stash",
        20,
        IT_STASH,
        0,
/* precache */ "",
/* sounds */ "",

		"radarStash"
    },
    {
        "ammo_moneybag",
        "sound/misc/moneybag_pickup.ogg",
        { "models/misc/moneybag.md3", 
        0, 0, 0},
/* icon */      "icons/icona_stash",
/* pickup */    "Bag 'O Money",
        20,
        IT_MONEYBAG,
        0,
/* precache */ "",
/* sounds */ "",

		"radarStash"
    },

/*QUAKED team_CTF_redflag (1 0 0) (-16 -16 -16) (16 16 16)
Only in CTF games
*/
	{
		"team_CTF_redflag",
		NULL,
        { "models/flags/r_flag.md3",
		0, 0, 0 },
/* icon */		"icons/iconf_red1",
/* pickup */	"Red Flag",
		0,
		IT_TEAM,
		PW_REDFLAG,
/* precache */ "",
/* sounds */ "",

		""
	},

/*QUAKED team_CTF_blueflag (0 0 1) (-16 -16 -16) (16 16 16)
Only in CTF games
*/
	{
		"team_CTF_blueflag",
		NULL,
        { "models/flags/b_flag.md3",
		0, 0, 0 },
/* icon */		"icons/iconf_blu1",
/* pickup */	"Blue Flag",
		0,
		IT_TEAM,
		PW_BLUEFLAG,
/* precache */ "",
/* sounds */ "",

		""
	},


	/*QUAKED team_CTF_neutralflag (0 0 1) (-16 -16 -16) (16 16 16)
Only in One Flag CTF games
*/
	{
		"team_CTF_neutralflag",
		NULL,
        { "models/flags/n_flag.md3",
		0, 0, 0 },
/* icon */		"icons/iconf_neutral1",
/* pickup */	"Neutral Flag",
		0,
		IT_TEAM,
		PW_NEUTRALFLAG,
/* precache */ "",
/* sounds */ "",

		""
	},

	// end of list marker
	{ NULL }
};

int		bg_numItems = sizeof(bg_itemlist) / sizeof(bg_itemlist[0]) - 1;


animationMap_t	bg_animationMap[] = 
{
	{ BOTH_DEATH1, "death1"},
	{ BOTH_DEAD1, "dead1"},
	{ BOTH_DEATH2, "death2"},
	{ BOTH_DEAD2, "dead2"},
	{ BOTH_DEATH3, "death3"},
	{ BOTH_DEAD3, "dead3"},

	{ TORSO_GESTURE, "gesture"},

	{ TORSO_ATTACK, "attack"},
	{ TORSO_ATTACK2, "attack2"},

	{ TORSO_DROP, "drop"},
	{ TORSO_RAISE, "raise"},

	{ TORSO_STAND, "stand"},
	{ TORSO_STAND2, "stand2"},
	
	{ TORSO_MELEE, "melee"},

	{ LEGS_WALKCR, "walk_crouch"},
	{ LEGS_WALKCR_PISTOL, "pistol_crouch_walk"},
	{ LEGS_WALK, "walk"},
	{ LEGS_WALK_PISTOL, "pistol_walk"},
	{ LEGS_RUN, "run"},
	{ LEGS_RUN_PISTOL, "pistol_run"},
	{ LEGS_BACK, "walk_backwards"},
	{ LEGS_BACK_PISTOL, "pistol_run_back"},
	{ LEGS_BACKWALK, "walking_backwards"},
	{ LEGS_BACKWALK_PISTOL, "pistol_walk_back"},
	{ LEGS_SWIM, "swim"},



	{ LEGS_JUMP, "jump"},
	{ LEGS_LAND, "land"},

	{ LEGS_JUMPB, "jump_backwards"},
	{ LEGS_LANDB, "land_backwards"},

	{ LEGS_IDLE, "idle"},
	{ LEGS_IDLECR, "idle_crouch"},
	{ LEGS_IDLECR_PISTOL, "pistol_crouch"},
	{ LEGS_IDLE2, "idle2"},
	{ LEGS_IDLE3, "idle3"},
	{ LEGS_IDLE4, "idle4"},
	{ LEGS_IDLE5, "idle5"},

	{ LEGS_TURN, "turn"},
	
	{ LEGS_COMBATIDLE, "combatidle1"},
	{ LEGS_COMBATPISTOLIDLE, "combatidle2"},

	{ TORSO_GETFLAG, "getflag"},
	{ TORSO_GUARDBASE, "guardbase"},
	{ TORSO_PATROL, "patrol"},
	{ TORSO_FOLLOWME, "followme"},
	{ TORSO_AFFIRMATIVE, "affirmative"},
	{ TORSO_NEGATIVE, "negative"}

};

animationMap_t	bg_weaponAnimationMap[] = 
{
	{ WEAPON_IDLE, "idle" },
	{ WEAPON_FIRE, "fire" },
	{ WEAPON_RELOAD, "reload" },
	{ WEAPON_LOWER, "lower" },
	{ WEAPON_RAISE, "raise" },
	{ WEAPON_OUT_OF_AMMO, "out_of_ammo" },
	{ WEAPON_MODE, "mode" },
	{ WEAPON_FIRE2, "fire_alt" },
	{ WEAPON_LOWER2, "lower_alt" },
	{ WEAPON_RAISE2, "raise_alt" },
	{ WEAPON_IDLE2, "idle_alt" },
	{ WEAPON_GRENADETOSS, "grenade_toss" },
	{ WEAPON_MELEE, "melee" },
	{ WEAPON_QUICKRELOAD, "quick_reload" },
	{ WEAPON_QUICKRELOAD_FINISH, "quick_reload_finish" }

};
/*
==============
BG_FindAnimationByName
==============
*/
int BG_FindAnimationByName(const char *name) {
	int i;
	for (i=0; i < MAX_ANIMATIONS; i++ ) {
		if (Q_stricmp(name, bg_animationMap[i].name) == 0) return bg_animationMap[i].animation;
	}
	return -1;
}
/*
==============
BG_FindAnimationByNumber
==============
*/
char *BG_FindAnimationByNumber(int animation) {
	int i;
	for (i=0; i < MAX_ANIMATIONS; i++) {
		if (bg_animationMap[i].animation == animation) return bg_animationMap[i].name;
	}
	return NULL;
}
/*
==============
BG_FindAnimationByName
==============
*/
int BG_FindWeaponAnimationByName(const char *name) {
	int i;
	for (i=0; i < MAX_WEAPON_ANIMATIONS; i++ ) {
		if (Q_stricmp(name, bg_weaponAnimationMap[i].name) == 0) return bg_weaponAnimationMap[i].animation;
	}
	return -1;
}

/*
==============
BG_FindWeaponAnimationByNumber
==============
*/
char *BG_FindWeaponAnimationByNumber(int animation) {
	int i;
	for (i=0; i < MAX_WEAPON_ANIMATIONS; i++) {
		if (bg_weaponAnimationMap[i].animation == animation) return bg_weaponAnimationMap[i].name;
	}
	return NULL;
}

/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t	*BG_FindItemForHoldable( holdable_t pw ) {
	int		i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	Com_Error( ERR_DROP, "HoldableItem not found" );

	return NULL;
}


/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t	*BG_FindItemForWeapon( weapon_t weapon ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++) {
		if ( it->giType == IT_WEAPON && it->giTag == weapon ) {
			return it;
		}
	}

	Com_Error( ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

/*
===============
BG_FindItem

===============
*/
gitem_t	*BG_FindItem( const char *pickupName ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->pickup_name, pickupName ) )
			return it;
	}

	return NULL;
}

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t		origin;

	BG_EvaluateTrajectory( &item->pos, atTime, origin );

	// we are ignoring ducked differences here
	if ( ps->origin[0] - origin[0] > 44
		|| ps->origin[0] - origin[0] < -50
		|| ps->origin[1] - origin[1] > 36
		|| ps->origin[1] - origin[1] < -36
		|| ps->origin[2] - origin[2] > 36
		|| ps->origin[2] - origin[2] < -36 ) {
		return qfalse;
	}

	return qtrue;
}



/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
qboolean BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps ) {
	gitem_t	*item;

	if ( ent->modelindex < 1 || ent->modelindex >= bg_numItems ) {
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
	}

	item = &bg_itemlist[ent->modelindex];

	switch( item->giType ) {
	case IT_WEAPON:
		if ( (!(ps->stats[ STAT_WEAPONS ] & ( 1 << item->giTag )) || ps->ammo[item->giTag] == 0 ))
			return qtrue;	// weapons are always picked up once
		else
			return qfalse;
	case IT_AMMO:
		if ( ps->ammo[ item->giTag ] >= 200 ) {
			return qfalse;		// can't hold any more
		}
		return qtrue;

	case IT_ARMOR:
		if ( ps->shields >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
			return qfalse;
		}
		return qtrue;

	case IT_HEALTH:
		if ( item->quantity == 5 ) {
			if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
				return qfalse;
			}
			return qtrue;
		}

		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;

	case IT_TEAM: // team items, such as flags
		if( gametype == GT_1FCTF ) {
			// neutral flag can always be picked up
			if( item->giTag == PW_NEUTRALFLAG ) {
				return qtrue;
			}
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if (item->giTag == PW_BLUEFLAG ) {
					return qtrue;
				}
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if (item->giTag == PW_REDFLAG ) {
					return qtrue;
				}
			}
		}

		if( gametype == GT_CTF ) {
			// ent->modelindex2 is non-zero on items if they are dropped
			// we need to know this because we can pick up our dropped flag (and return it)
			// but we can't pick up our flag at base
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if (item->giTag == PW_BLUEFLAG ||
					(item->giTag == PW_REDFLAG && ent->modelindex2) ||
					(item->giTag == PW_REDFLAG ) )
					return qtrue;
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if (item->giTag == PW_REDFLAG ||
					(item->giTag == PW_BLUEFLAG && ent->modelindex2) ||
					(item->giTag == PW_BLUEFLAG ) )
					return qtrue;
			}
		}

		return qfalse;

	case IT_HOLDABLE:
		// can only hold one item at a time
		if ( ps->stats[STAT_HOLDABLE_ITEM] ) {
			return qfalse;
		}
		return qtrue;
    case IT_STASH:
		return qtrue;
    case IT_MONEYBAG:
		return qtrue;

    case IT_BAD:
            Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD" );
        default:
#ifndef Q3_VM
#ifdef _DEBUG // bk0001204
          Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n", item->giType );
#endif
#endif
         break;
	}

	return qfalse;
}

//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result ) {
	float		deltaTime;
	float		phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001f;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_LINEAR_BOB:
		deltaTime = ( atTime - tr->trTime ) * 0.001f;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		phase = sinf( atTime / 100.0f );
		result[2] += phase * 5.0f;
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = sinf( deltaTime * M_PI * 2.0f );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001f;	// milliseconds to seconds
		if ( deltaTime < 0.0f ) {
			deltaTime = 0.0f;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001f;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5f * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result ) {
	float	deltaTime;
	float	phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
	case TR_LINEAR_BOB:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = cosf( deltaTime * M_PI * 2.0f );	// derivative of sin = cos
		phase *= 0.5f;
		VectorScale( tr->trDelta, phase, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001f;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= DEFAULT_GRAVITY * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
		break;
	}
}

char *eventnames[] = {
	"EV_NONE",

	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",

	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",

	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",

	"EV_JUMP_PAD",			// boing sound at origin", jump sound on player

	"EV_JUMP",
	"EV_WATER_TOUCH",	// foot touches
	"EV_WATER_LEAVE",	// foot leaves
	"EV_WATER_UNDER",	// head touches
	"EV_WATER_CLEAR",	// head leaves

	"EV_ITEM_PICKUP",			// normal item pickups are predictable
	"EV_GLOBAL_ITEM_PICKUP",	// powerup / team sounds are broadcast to everyone

	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_FIRE_GRENADE",
	"EV_FIRE_MELEE",
	"EV_RELOAD_WEAPON",

	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",

	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GRENADE_BOUNCE",		// eventParm will be the soundindex

	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",		// no attenuation
	"EV_GLOBAL_TEAM_SOUND",

	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_RAILTRAIL",
	"EV_SHOTGUN",
	"EV_BULLET",				// otherEntity is the shooter

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_REGEN",

	"EV_GIB_PLAYER",			// gib a previously living player
	"EV_SCOREPLUM",			// score plum

	"EV_C4_STICK",
	"EV_C4_TRIGGER",
	"EV_KAMIKAZE",			// kamikaze explodes
	"EV_OBELISKEXPLODE",		// obelisk explodes
	"EV_INVUL_IMPACT",		// invulnerability sphere impact
	"EV_JUICED",				// invulnerability juiced effect
	"EV_LIGHTNINGBOLT",		// lightning bolt bounced of invulnerability sphere

	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT"

};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void	trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if ( atof(buf) != 0 ) {
			Com_Printf("event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_TouchJumpPad
========================
*/
void BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad ) {
	vec3_t	angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if ( ps->pm_type != PM_NORMAL ) {
		return;
	}

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if ( ps->jumppad_ent != jumppad->number ) {

		vectoangles( jumppad->origin2, angles);
		p = fabsf( AngleNormalize180( angles[PITCH] ) );
		if( p < 45.0f ) {
			effectNum = 0;
		} else {
			effectNum = 1;
		}
		BG_AddPredictableEventToPlayerstate( EV_JUMP_PAD, effectNum, ps );
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy( jumppad->origin2, ps->velocity );
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_GIBBED] == 1 ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}
	// set the trDelta for flag direction
	VectorCopy( ps->velocity, s->pos.trDelta );

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->weaponAnim = ps->weaponAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->shields = ps->shields;;

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}
	// set the trDelta for flag direction and linear prediction
	VectorCopy( ps->velocity, s->pos.trDelta );
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->shields = ps->shields;

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}


bottype_t BG_GetBotType( const char * type ) {

	switch ( SWITCHSTRING( type ) )
	{
	case CS('g','u','a','r'):
		//guards have the guardtype in the botname field
		switch( SWITCHSTRING( type+5 ) )
		{
		case CS('e','a','s','y'):
			return BOT_GUARD_EASY;
		case CS('m','e','d','i'):
			return BOT_GUARD_MEDIUM;
		case CS('h','a','r','d'):
			return BOT_GUARD_HARD;
		}
		return BOT_GUARD_MEDIUM;
	case CS('i','n','f','o'):	return BOT_INFORMANT;	// "informant"
	case CS('m','e','r','c'):	return BOT_MERCHANT;	// "merchant"
	case CS('h','o','s','t'):	return BOT_HOSTAGE;		// "hostage"
	case CS('b','o','s','s'):	return BOT_BOSS;		// "boss"
	case CS('t','r','a','d'):	return BOT_TRADER;		// "trader"
	case CS('p','l','a','y'):	return BOT_PLAYER;
	default: return BOT_NORMAL;
	}
}

qboolean BG_GameInProgress( globalState_t * gs ) {
	return (gs->INPLAY == 1) || (gs->TURN > 1);
}


int		Q_rrand( int low, int high ) {
	return ((high-low)==0)?0:low + abs( Q_rand() ) % ( high-low );
}

float	Q_random() {
	return ( Q_rand() & 0xffff ) * (1.0f/(float)0x10000);
}

float	Q_crandom( int *seed ) {
	float v;
	*seed = (69069 * *seed + 1);
	v = (*seed & 0x7fff ) / ((float)0x7fff);
	return 2.0 * v - 0.5;
}
