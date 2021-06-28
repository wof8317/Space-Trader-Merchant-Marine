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

#include "cg_local.h"

// cg_players.c -- handle the media and animation for player entities

char	*cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"*death1.ogg",
	"*death2.ogg",
	"*death3.ogg",
	"*jump1.ogg",
	"*pain25_1.ogg",
	"*pain50_1.ogg",
	"*pain75_1.ogg",
	"*pain100_1.ogg",
	"*falling1.ogg",
	"*gasp.ogg",
	"*drown.ogg",
	"*fall1.ogg",
	"*taunt.ogg"
};


/*
================
CG_CustomSound

================
*/
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName ) {
	clientInfo_t *ci;
	int			i;

	if ( soundName[0] != '*' ) {
		return trap_S_RegisterSound( soundName, qfalse );
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i] ; i++ ) {
		if ( !strcmp( soundName, cg_customSoundNames[i] ) ) {
			return ci->sounds[i];
		}
	}

	CG_Error( "Unknown custom sound: %s", soundName );
	return 0;
}



/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation coutns and rates
models/players/visor/animation.cfg, etc
======================
*/
static qboolean	CG_ParseAnimationFile( const char *filename, clientInfo_t *ci ) {
	const char	*text_p, *prev;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			skip;
	int			anim;
	char		text[20000];
	fileHandle_t	f;
	animation_t *animations;

	animations = ci->animations;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;
	skip = 0;	// quite the compiler warning

	ci->footsteps = FOOTSTEP_NORMAL;
	VectorClear( ci->headOffset );
	ci->gender = GENDER_MALE;
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;

	// read optional parameters
	for( ; ; )
	{
		prev = text_p;	// so we can unget
		token = COM_Parse( &text_p );
		if ( !token ) {
			break;
		}
		if ( !Q_stricmp( token, "footsteps" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}
			if ( !Q_stricmp( token, "default" ) || !Q_stricmp( token, "normal" ) ) {
				ci->footsteps = FOOTSTEP_NORMAL;
			} else if ( !Q_stricmp( token, "boot" ) ) {
				ci->footsteps = FOOTSTEP_BOOT;
			} else if ( !Q_stricmp( token, "flesh" ) ) {
				ci->footsteps = FOOTSTEP_FLESH;
			} else if ( !Q_stricmp( token, "mech" ) ) {
				ci->footsteps = FOOTSTEP_MECH;
			} else if ( !Q_stricmp( token, "energy" ) ) {
				ci->footsteps = FOOTSTEP_ENERGY;
			} else {
				CG_Printf( "Bad footsteps parm in %s: %s\n", filename, token );
			}
			continue;
		} else if ( !Q_stricmp( token, "headoffset" ) ) {
			for ( i = 0 ; i < 3 ; i++ ) {
				token = COM_Parse( &text_p );
				if ( !token ) {
					break;
				}
				ci->headOffset[i] = atof( token );
			}
			continue;
		} else if ( !Q_stricmp( token, "sex" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}
			if ( token[0] == 'f' || token[0] == 'F' ) {
				ci->gender = GENDER_FEMALE;
			} else if ( token[0] == 'n' || token[0] == 'N' ) {
				ci->gender = GENDER_NEUTER;
			} else {
				ci->gender = GENDER_MALE;
			}
			continue;
		} else if ( !Q_stricmp( token, "fixedlegs" ) ) {
			ci->fixedlegs = qtrue;
			continue;
		} else if ( !Q_stricmp( token, "fixedtorso" ) ) {
			ci->fixedtorso = qtrue;
			continue;
		} else if ( (anim = BG_FindAnimationByName( token ) ) >= 0 )
		{
			token = COM_Parse( &text_p );
			if ( !*token ) break;
			animations[anim].firstFrame = atoi( token );
			token = COM_Parse( &text_p );
			if ( !*token ) break;
			animations[anim].numFrames = atoi( token );
			token = COM_Parse( &text_p );
			if ( !*token ) break;
			animations[anim].loopFrames = atoi( token );
			token = COM_Parse( &text_p );
			if ( !*token ) break;
			fps = atoi( token );
			if ( fps == 0 ) {
				fps = 1;
			}
			animations[anim].frameLerp = 1000 / fps;
			animations[anim].initialLerp = 1000 / fps;
			
			animations[anim].reversed = qfalse;
			animations[anim].flipflop = qfalse;
			// if numFrames is negative the animation is reversed
			if (animations[anim].numFrames < 0) {
				animations[anim].numFrames = -animations[anim].numFrames;
				animations[anim].reversed = qtrue;
			}
			if( ci->modelType == MT_MD3 )
			{
				// leg only frames are adjusted to not count the upper body only frames
				if ( anim == LEGS_WALKCR ) {
					skip = animations[LEGS_WALKCR].firstFrame - animations[TORSO_GESTURE].firstFrame;
				}
				if ( anim >= LEGS_WALKCR && anim < TORSO_GETFLAG) {
					animations[anim].firstFrame -= skip;
				}
			}
			continue;
		} else if ( !text_p) {
			break;
		}
	}

	// crouch backward animation
	memcpy(&animations[LEGS_BACKCR], &animations[LEGS_WALKCR], sizeof(animation_t));
	animations[LEGS_BACKCR].reversed = qtrue;
	// walk backward animation
	memcpy(&animations[LEGS_BACKWALK], &animations[LEGS_WALK], sizeof(animation_t));
	animations[LEGS_BACKWALK].reversed = qtrue;
	
	memcpy(&animations[LEGS_BACKWALK_PISTOL], &animations[LEGS_WALK_PISTOL], sizeof(animation_t));
	animations[LEGS_BACKWALK_PISTOL].reversed = qtrue;

	memcpy(&animations[LEGS_BACKCR_PISTOL], &animations[LEGS_WALKCR_PISTOL], sizeof(animation_t));
	animations[LEGS_BACKCR_PISTOL].reversed = qtrue;

	// flag moving fast
	animations[FLAG_RUN].firstFrame = 0;
	animations[FLAG_RUN].numFrames = 16;
	animations[FLAG_RUN].loopFrames = 16;
	animations[FLAG_RUN].frameLerp = 1000 / 15;
	animations[FLAG_RUN].initialLerp = 1000 / 15;
	animations[FLAG_RUN].reversed = qfalse;
	// flag not moving or moving slowly
	animations[FLAG_STAND].firstFrame = 16;
	animations[FLAG_STAND].numFrames = 5;
	animations[FLAG_STAND].loopFrames = 0;
	animations[FLAG_STAND].frameLerp = 1000 / 20;
	animations[FLAG_STAND].initialLerp = 1000 / 20;
	animations[FLAG_STAND].reversed = qfalse;
	// flag speeding up
	animations[FLAG_STAND2RUN].firstFrame = 16;
	animations[FLAG_STAND2RUN].numFrames = 5;
	animations[FLAG_STAND2RUN].loopFrames = 1;
	animations[FLAG_STAND2RUN].frameLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].initialLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].reversed = qtrue;
	//
	// new anims changes
	//
//	animations[TORSO_GETFLAG].flipflop = qtrue;
//	animations[TORSO_GUARDBASE].flipflop = qtrue;
//	animations[TORSO_PATROL].flipflop = qtrue;
//	animations[TORSO_AFFIRMATIVE].flipflop = qtrue;
//	animations[TORSO_NEGATIVE].flipflop = qtrue;
	//
	return qtrue;
}

/*
==========================
CG_FileExists
==========================
*/
static qboolean	CG_FileExists(const char *filename) {
	int len;

	len = trap_FS_FOpenFile( filename, 0, FS_READ );
	if( len > 0 )
	{
		return qtrue;
	}
	return qfalse;
}

/*
==========================
CG_FindClientModelFile
==========================
*/
qboolean	CG_FindClientModelFile( char *filename, int length, clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName, const char *base, const char *ext ) {
	char *team, *charactersFolder;
	int i;

	team = "default";
	charactersFolder = "";
	for( ; ; )
	{
		for ( i = 0; i < 2; i++ ) {
			if ( i == 0 && teamName && *teamName ) {
				//								"models/players/characters/james/stroggs/lower_lily_red.skin"
				Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, team, ext );
			}
			else {
				//								"models/players/characters/james/lower_lily_red.skin"
				Com_sprintf( filename, length, "models/players/%s%s/%s_%s_%s.%s", charactersFolder, modelName, base, skinName, team, ext );
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( cgs.gametype >= GT_TEAM && cgs.forceskins ) {
				if ( i == 0 && teamName && *teamName ) {
					//								"models/players/characters/james/stroggs/lower_red.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, team, ext );
				}
				else {
					//								"models/players/characters/james/lower_red.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", charactersFolder, modelName, base, team, ext );
				}
			}
			else {
				if ( i == 0 && teamName && *teamName ) {
					//								"models/players/characters/james/stroggs/lower_lily.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, ext );
				}
				else {
					//								"models/players/characters/james/lower_lily.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", charactersFolder, modelName, base, skinName, ext );
				}
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( !teamName || !*teamName ) {
				break;
			}
		}
		// if tried the heads folder first
		if ( charactersFolder[0] ) {
			break;
		}
		charactersFolder = "characters/";
	}

	return qfalse;
}

/*
==========================
CG_FindClientHeadFile
==========================
*/
static qboolean	CG_FindClientHeadFile( char *filename, int length, clientInfo_t *ci, const char *teamName, const char *headModelName, const char *headSkinName, const char *base, const char *ext ) {
	char *team, *headsFolder;
	int i;

	if ( cgs.gametype >= GT_TEAM ) {
		switch ( ci->team ) {
			case TEAM_BLUE: {
				team = "blue";
				break;
			}
			default: {
				team = "red";
				break;
			}
		}
	}
	else {
		team = "default";
	}

	if ( headModelName[0] == '*' ) {
		headsFolder = "heads/";
		headModelName++;
	}
	else {
		headsFolder = "";
	}
	for( ; ; )
	{
		for ( i = 0; i < 2; i++ ) {
			if ( i == 0 && teamName && *teamName ) {
				Com_sprintf( filename, length, "models/players/%s%s/%s/%s%s_%s.%s", headsFolder, headModelName, headSkinName, teamName, base, team, ext );
			}
			else {
				Com_sprintf( filename, length, "models/players/%s%s/%s/%s_%s.%s", headsFolder, headModelName, headSkinName, base, team, ext );
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( cgs.gametype >= GT_TEAM && cgs.forceskins ) {
				if ( i == 0 &&  teamName && *teamName ) {
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headModelName, teamName, base, team, ext );
				}
				else {
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headModelName, base, team, ext );
				}
			}
			else {
				if ( i == 0 && teamName && *teamName ) {
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headModelName, teamName, base, headSkinName, ext );
				}
				else {
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headModelName, base, headSkinName, ext );
				}
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( !teamName || !*teamName ) {
				break;
			}
		}
		// if tried the heads folder first
		if ( headsFolder[0] ) {
			break;
		}
		headsFolder = "heads/";
	}

	return qfalse;
}

/*
	CG_RegisterClientSkin

	Must be called *after* ci has had its modelType field filled in!
*/
static qboolean	CG_RegisterClientSkin( clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName ) {
	char filename[MAX_QPATH];

	switch( ci->modelType )
	{
	case MT_X42:
		if( CG_FindClientModelFile( filename, sizeof( filename ), ci, teamName, modelName, skinName, "char", "skin" ) )
			ci->legsSkin = trap_R_RegisterSkin( filename );
		if( ci->legsSkin )
			return qtrue;
		else
			Com_Printf( "Character skin load failure: %s\n", filename );
		break;

	case MT_MD3:
		if( CG_FindClientModelFile( filename, sizeof( filename ), ci, teamName, modelName, skinName, "lower", "skin" ) )
			ci->legsSkin = trap_R_RegisterSkin( filename );
		if( !ci->legsSkin )
			Com_Printf( "Leg skin load failure: %s\n", filename );

		if( CG_FindClientModelFile( filename, sizeof( filename ), ci, teamName, modelName, skinName, "upper", "skin" ) )
			ci->torsoSkin = trap_R_RegisterSkin( filename );
		if( !ci->torsoSkin )
			Com_Printf( "Torso skin load failure: %s\n", filename );

		// if any skins failed to load
		if( ci->legsSkin && ci->torsoSkin && ci->headSkin )
			return qtrue;

		break;

	}
		
	return qfalse;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
static qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName,
	const char *skinName, const char *teamName )
{
	char filename[MAX_QPATH*2];

	if ( cgs.gametype == GT_HUB ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/char_hub.x42", modelName );
		ci->legsModel = trap_R_RegisterModel( filename );
	}
	if( !ci->legsModel )
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/char.x42", modelName );
		ci->legsModel = trap_R_RegisterModel( filename );
	}
	
	if( ci->legsModel )
		ci->modelType = MT_X42;
	else
	{
		ci->modelType = MT_MD3;

		Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", modelName );
		ci->legsModel = trap_R_RegisterModel( filename );
		if( !ci->legsModel )
		{
			Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/lower.md3", modelName );
			ci->legsModel = trap_R_RegisterModel( filename );
			if( !ci->legsModel )
			{
				Com_Printf( "Failed to load model file %s\n", filename );
				return qfalse;
			}
		}

		Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", modelName );
		ci->torsoModel = trap_R_RegisterModel( filename );
		if( !ci->torsoModel )
		{
			Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/upper.md3", modelName );
			ci->torsoModel = trap_R_RegisterModel( filename );

			if( !ci->torsoModel )
			{
				Com_Printf( "Failed to load model file %s\n", filename );
				return qfalse;
			}
		}
	}

	// if any skins failed to load, return failure
	if( !CG_RegisterClientSkin( ci, teamName, modelName, skinName ) )
	{
		Com_Printf( "Failed to load skin file: %s : %s\n", modelName, skinName );
		return qfalse;
	}

	// load the animations
	if ( cgs.gametype == GT_HUB )
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation_hub.cfg", modelName );
		if (!CG_ParseAnimationFile( filename, ci ))
		{
			Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation.cfg", modelName );
		}
	} else {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation.cfg", modelName );
	}
	if ( !CG_ParseAnimationFile( filename, ci ) )
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/animation.cfg", modelName );
		if( !CG_ParseAnimationFile( filename, ci ) )
		{
			Com_Printf( "Failed to load animation file %s\n", filename );
			return qfalse;
		}
	}

	if( CG_FindClientHeadFile( filename, sizeof( filename ), ci, teamName, modelName, skinName, "icon", "skin" ) )
		ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	else if( CG_FindClientHeadFile( filename, sizeof( filename ), ci, teamName, modelName, skinName, "icon", "dds" ) )
		ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	else if( CG_FindClientHeadFile( filename, sizeof( filename ), ci, teamName, modelName, skinName, "icon", "tga" ) )
		ci->modelIcon = trap_R_RegisterShaderNoMip( filename );

	if( !ci->modelIcon )
		return qfalse;
	return qtrue;
}

/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString( const char *v, vec3_t color ) {
	int val;

	VectorClear( color );

	val = atoi( v );

	if ( val < 1 || val > 7 ) {
		VectorSet( color, 1, 1, 1 );
		return;
	}

	if ( val & 1 ) {
		color[2] = 1.0f;
	}
	if ( val & 2 ) {
		color[1] = 1.0f;
	}
	if ( val & 4 ) {
		color[0] = 1.0f;
	}
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo( clientInfo_t *ci ) {
	const char	*dir, *fallback;
	int			i, modelloaded;
	const char	*s;
	int			clientNum;
	char		teamname[MAX_QPATH];

	teamname[0] = 0;

	modelloaded = qtrue;
	if ( !CG_RegisterClientModelname( ci, ci->modelName, ci->skinName, teamname ) ) {
		if ( cg_buildScript.integer ) {
			CG_Error( "CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname );
		}

		// fall back to default
		if ( !CG_RegisterClientModelname( ci, DEFAULT_MODEL, "default", teamname ) ) {
			CG_Error( "DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL );
		}
		modelloaded = qfalse;
	}

	ci->newAnims = qfalse;
	if ( ci->torsoModel ) {
		affine_t tag;
		// if the torso model has the "tag_flag"
		if ( trap_R_LerpTag( &tag, ci->torsoModel, 0, 0, 1, "tag_flag" ) ) {
			ci->newAnims = qtrue;
		}
	}

	// sounds
	dir = ci->modelName;
	fallback = DEFAULT_MODEL;

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++ ) {
		s = cg_customSoundNames[i];
		if ( !s ) {
			break;
		}
		ci->sounds[i] = 0;
		// if the model didn't load use the sounds of the default model
		if (modelloaded) {
			ci->sounds[i] = trap_S_RegisterSound( va("sound/player/%s/%s", dir, s + 1), qfalse );
		}
		if ( !ci->sounds[i] ) {
			ci->sounds[i] = trap_S_RegisterSound( va("sound/player/%s/%s", fallback, s + 1), qfalse );
		}
	}
	ci->deferred = qfalse;
	
	// unregister weapons since our arm models may have changed.
	for (i=0; i < MAX_WEAPONS; i++ ) {
		cg_weapons[i].registered = qfalse;
	}

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	clientNum = ci - cgs.clientinfo;
	for ( i = 0 ; i < MAX_GENTITIES ; i++ ) {
		if ( cg_entities[i].currentState.clientNum == clientNum
			&& cg_entities[i].currentState.eType == ET_PLAYER ) {
			CG_ResetPlayerEntity( &cg_entities[i] );
		}
	}
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel( clientInfo_t *from, clientInfo_t *to )
{
	VectorCopy( from->headOffset, to->headOffset );
	to->footsteps = from->footsteps;
	to->gender = from->gender;

	to->modelType = from->modelType;
	to->legsModel = from->legsModel;
	to->legsSkin = from->legsSkin;
	to->torsoModel = from->torsoModel;
	to->torsoSkin = from->torsoSkin;
	to->headModel = from->headModel;
	to->headSkin = from->headSkin;
	to->modelIcon = from->modelIcon;

	to->newAnims = from->newAnims;
	
	memcpy( to->animations, from->animations, sizeof( to->animations ) );
	memcpy( to->sounds, from->sounds, sizeof( to->sounds ) );

}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo( clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}
		if ( match->deferred ) {
			continue;
		}
		if ( !Q_stricmp( ci->modelName, match->modelName )
			&& !Q_stricmp( ci->skinName, match->skinName )
			&& !Q_stricmp( ci->headModelName, match->headModelName )
			&& !Q_stricmp( ci->headSkinName, match->headSkinName ) 
			&& (cgs.gametype < GT_TEAM || ci->team == match->team) ) {
			// this clientinfo is identical, so use it's handles

			ci->deferred = qfalse;

			CG_CopyClientInfoModel( match, ci );

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo( clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	// if someone else is already the same models and skins we
	// can just load the client info
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid || match->deferred ) {
			continue;
		}
		if ( Q_stricmp( ci->skinName, match->skinName ) ||
			 Q_stricmp( ci->modelName, match->modelName ) ||
//			 Q_stricmp( ci->headModelName, match->headModelName ) ||
//			 Q_stricmp( ci->headSkinName, match->headSkinName ) ||
			 (cgs.gametype >= GT_TEAM && ci->team != match->team) ) {
			continue;
		}
		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo( ci );
		return;
	}

	// if we are in teamplay, only grab a model if the skin is correct
	if ( cgs.gametype >= GT_TEAM ) {
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			match = &cgs.clientinfo[ i ];
			if ( !match->infoValid || match->deferred ) {
				continue;
			}
			if ( Q_stricmp( ci->skinName, match->skinName ) ||
				(cgs.gametype >= GT_TEAM && ci->team != match->team) ) {
				continue;
			}
			ci->deferred = qtrue;
			CG_CopyClientInfoModel( match, ci );
			return;
		}
		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo( ci );
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}

		ci->deferred = qtrue;
		CG_CopyClientInfoModel( match, ci );
		return;
	}

	// we should never get here...
	CG_Printf( "CG_SetDeferredClientInfo: no valid clients!\n" );

	CG_LoadClientInfo( ci );
}


/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo( int clientNum ) {
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char	*configstring;
	const char	*v;
	char		*slash;

	ci = &cgs.clientinfo[clientNum];

	configstring = CG_ConfigString( clientNum + CS_PLAYERS );
	if ( !configstring[0] ) {
		memset( ci, 0, sizeof( *ci ) );
		return;		// player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz( newInfo.name, v, sizeof( newInfo.name ) );

	// colors
	v = Info_ValueForKey( configstring, "c1" );
	CG_ColorFromString( v, newInfo.color1 );

	v = Info_ValueForKey( configstring, "c2" );
	CG_ColorFromString( v, newInfo.color2 );

	// bot skill
	v = Info_ValueForKey( configstring, "skill" );
	newInfo.botSkill = atoi( v );

	// handicap
	v = Info_ValueForKey( configstring, "hc" );
	newInfo.handicap = atoi( v );

	// wins
	v = Info_ValueForKey( configstring, "w" );
	newInfo.wins = atoi( v );

	// losses
	v = Info_ValueForKey( configstring, "l" );
	newInfo.losses = atoi( v );

	// team
	v = Info_ValueForKey( configstring, "t" );
	newInfo.team = atoi( v );

	// team task
	v = Info_ValueForKey( configstring, "tt" );
	newInfo.teamTask = atoi(v);

	// team leader
	v = Info_ValueForKey( configstring, "tl" );
	newInfo.teamLeader = atoi(v);

	// model
	v = Info_ValueForKey( configstring, "model" );
	if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		trap_Cvar_VariableStringBuffer( "model", modelStr, sizeof( modelStr ) );
		if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
			skin = "default";
		} else {
			*skin++ = 0;
		}

		Q_strncpyz( newInfo.skinName, skin, sizeof( newInfo.skinName ) );
		Q_strncpyz( newInfo.modelName, modelStr, sizeof( newInfo.modelName ) );

		if ( cgs.gametype >= GT_TEAM ) {
			// keep skin name
			slash = strchr( v, '/' );
			if ( slash ) {
				Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			}
		}
	} else {
		Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );

		slash = strchr( newInfo.modelName, '/' );
		if ( !slash ) {
			// modelName didn not include a skin name
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
		} else {
			Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			Q_strncpyz( newInfo.modelName, newInfo.modelName, slash - newInfo.modelName + 1 );
			memset( slash, 0, sizeof(newInfo.modelName) - (slash - newInfo.modelName) );
		}
	}

	// head model
	v = Info_ValueForKey( configstring, "hmodel" );
	if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		trap_Cvar_VariableStringBuffer( "headmodel", modelStr, sizeof( modelStr ) );
		if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
			skin = "default";
		} else {
			*skin++ = 0;
		}

		Q_strncpyz( newInfo.headSkinName, skin, sizeof( newInfo.headSkinName ) );
		Q_strncpyz( newInfo.headModelName, modelStr, sizeof( newInfo.headModelName ) );

		if ( cgs.gametype >= GT_TEAM ) {
			// keep skin name
			slash = strchr( v, '/' );
			if ( slash ) {
				Q_strncpyz( newInfo.headSkinName, slash + 1, sizeof( newInfo.headSkinName ) );
			}
		}
	} else {
		Q_strncpyz( newInfo.headModelName, v, sizeof( newInfo.headModelName ) );

		slash = strchr( newInfo.headModelName, '/' );
		if ( !slash ) {
			// modelName didn not include a skin name
			Q_strncpyz( newInfo.headSkinName, "default", sizeof( newInfo.headSkinName ) );
		} else {
			Q_strncpyz( newInfo.headSkinName, slash + 1, sizeof( newInfo.headSkinName ) );
			// truncate modelName
			*slash = 0;
		}
	}

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if ( !CG_ScanForExistingClientInfo( &newInfo ) ) {
		qboolean	forceDefer;

		forceDefer = false;

		// if we are defering loads, just have it pick the first valid
		if ( forceDefer || (cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading ) ) {
			// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo( &newInfo );
			// if we are low on memory, leave them with this model
			if ( forceDefer ) {
				CG_Printf( "Memory is low.  Using deferred model.\n" );
				newInfo.deferred = qfalse;
			}
		} else {
			CG_LoadClientInfo( &newInfo );
		}
	}

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci = newInfo;
}



/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers( void ) {
	int		i;
	clientInfo_t	*ci;

	// scan for a deferred player to load
	for ( i = 0, ci = cgs.clientinfo ; i < cgs.maxclients ; i++, ci++ ) {
		if ( ci->infoValid && ci->deferred ) {
			// if we are low on memory, leave it deferred
			if ( trap_MemoryRemaining() < 4000000 ) {
				CG_Printf( "Memory is low.  Using deferred model.\n" );
				ci->deferred = qfalse;
				continue;
			}
			CG_LoadClientInfo( ci );
//			break;
		}
	}
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/


/*
===============
CG_SetLerpFrameAnimation

===============
*/
static void CG_SetLerpFrameAnimation( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	animation_t	*anim;

	lf->animationNumber = newAnimation;

	if ( newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS ) {
		CG_Error( "Bad animation number: %i", newAnimation );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;
	lf->frame = anim->firstFrame;

	if ( cg_debugAnim.integer == 1 ) {
		CG_Printf( "Anim: %i\n", newAnimation );
	}
}
//Weapon Animations
/*
===============
CG_SetWeaponLerpFrameAnimation

===============
*/
static void CG_SetWeaponLerpFrameAnimation( weaponInfo_t *wi, lerpFrame_t *lf, int newAnimation ) {
	animation_t	*anim;

	lf->animationNumber = newAnimation;

	if ( newAnimation < 0 || newAnimation >= MAX_WEAPON_ANIMATIONS ) {
		CG_Error( "Bad weapon animation number: %i", newAnimation );
	}

	anim = &wi->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;
	lf->frame = anim->firstFrame;

	if ( cg_debugAnim.integer == 1 ) {
		CG_Printf( "Weapon Anim: %i\n", newAnimation );
	}
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static qboolean CG_RunLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, float speedScale ) {
	int			f, numFrames;
	animation_t	*anim;

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return qtrue;
	}
	if ( cg_animSpeed.integer < 0 ) {
	    lf->oldFrame = lf->frame = lf->backlerp = cg_animSpeed.integer * -1;
	    return qtrue;
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim ) return qfalse;
		if ( !anim->frameLerp ) {
			return qfalse;		// shouldn't happen
		}
		if ( cg.time < lf->animationTime ) {
			lf->frameTime = lf->animationTime;		// initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		f *= speedScale;		// adjust for haste, etc

		numFrames = anim->numFrames;
		if (anim->flipflop) {
			numFrames *= 2;
		}
		if ( f >= numFrames ) {
			f -= numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				if (anim->loopFrames < 0 ) {
					f += anim->numFrames + anim->loopFrames;
				} else {
					f += anim->numFrames - anim->loopFrames;
				}
			} else {
				return qfalse;
				/*CG_Printf("Starvation: anim set to [%d]\n", starveAnimation);
				CG_SetLerpFrameAnimation(ci, lf, starveAnimation);*/
				/*
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;*/
			}
		}
		if ( anim->reversed ) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		}
		else if (anim->flipflop && f>=anim->numFrames) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - (f%anim->numFrames);
		}
		else {
			lf->frame = anim->firstFrame + f;
		}
		if ( cg.time > lf->frameTime ) {
			lf->frameTime = cg.time;
			if ( cg_debugAnim.integer == 1 ) {
				CG_Printf( "Clamp lf->frameTime\n");
			}
		}
	}

	if ( lf->frameTime > cg.time + 200 ) {
		lf->frameTime = cg.time;
	}

	if ( lf->oldFrameTime > cg.time ) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
	return qtrue;
}


/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int animationNumber ) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation( ci, lf, animationNumber );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}
static int CG_WeaponAnimationPriorities(weapAnimNumber_t currentAnimation, weapAnimNumber_t newAnimation)
{
	if ( newAnimation == WEAPON_FIRE ) return newAnimation;
	if ( currentAnimation == WEAPON_QUICKRELOAD && cg.predictedPlayerState.stats[STAT_AMMO] == 12 ) {
		return WEAPON_QUICKRELOAD_FINISH;
	}

	if ( (currentAnimation == WEAPON_RELOAD) && cg.predictedPlayerState.weaponTime > 0 && !(newAnimation == WEAPON_MELEE || newAnimation == WEAPON_LOWER) )
		return currentAnimation;
	if ( currentAnimation == WEAPON_QUICKRELOAD_FINISH && newAnimation == WEAPON_QUICKRELOAD ) return currentAnimation;
	return newAnimation;
}
static int CG_BlendWeaponAnimation( weapAnimNumber_t newAnimation, weapAnimNumber_t oldAnimation, lerpFrame_t *weapon, lerpFrame_t *weapon2)
{
	// see if the animation sequence is switching
	if ( newAnimation != oldAnimation || !weapon->animation ) {
		//CG_Printf("[NOTICE]Blending from %d to %d\n", oldAnimation, newAnimation);
		if (weapon->animation)
		{
			Com_Memcpy(weapon2, weapon, sizeof(lerpFrame_t) );
			CG_SetWeaponLerpFrameAnimation( &cg_weapons[cg.predictedPlayerState.weapon], weapon, newAnimation );
		} else {
			//transition from null animation to first animation.
			CG_SetWeaponLerpFrameAnimation( &cg_weapons[cg.predictedPlayerState.weapon], weapon, newAnimation );
			Com_Memcpy(weapon2, weapon, sizeof(lerpFrame_t) );
		}
		// adjust the lerp frames, but dont return a transition time
		// to avoid blending from a pistol to a shotgun on weapon changes.
		if ( newAnimation == WEAPON_RAISE || newAnimation == WEAPON_FIRE || oldAnimation == WEAPON_FIRE ) return 0;

		return ANIM_TRANSITION_TIME;

	}
	return 0;
}
/*
===============
CG_WeaponAnimation
===============
*/
void CG_WeaponAnimation( centity_t *cent, refEntity_t *weapon, refEntity_t *weapon2 ) {
	clientInfo_t	*ci;
	int				clientNum;
	weapAnimNumber_t	newAnimation;
	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer ) {
		weapon->oldframe = weapon->frame = 0;
		return;
	}

	ci = &cgs.clientinfo[ clientNum ];
	
	newAnimation = CG_WeaponAnimationPriorities( cent->pe.weapon.animationNumber, cent->currentState.weaponAnim );
	if (cent->pe.weaponTransTime <= 0 )
		cent->pe.weaponTransTime = CG_BlendWeaponAnimation( newAnimation, cent->pe.weapon.animationNumber, &cent->pe.weapon, &cent->pe.weapon2  );

	if ( CG_RunLerpFrame( ci, &cent->pe.weapon, 1.0f ) == qfalse ) {
		cent->pe.weaponTransTime = CG_BlendWeaponAnimation( cent->pe.weapon.animationNumber, cent->pe.weapon.animationNumber, &cent->pe.weapon, &cent->pe.weapon2  );
		CG_RunLerpFrame( ci, &cent->pe.weapon, 1.0f );
	}
	weapon->oldframe = cent->pe.weapon.oldFrame;
	weapon->frame = cent->pe.weapon.frame;
	weapon->backlerp = cent->pe.weapon.backlerp;

	if ( cent->pe.weaponTransTime > 0 )
	{
		CG_RunLerpFrame( ci, &cent->pe.weapon2, 1.0f );
		weapon2->oldframe = cent->pe.weapon2.oldFrame;
		weapon2->frame = cent->pe.weapon2.frame;
		weapon2->backlerp = cent->pe.weapon2.backlerp;
	}
}
static int CG_AnimationPriorities(centity_t *cent, animNumber_t currentAnimation, animNumber_t newAnimation, qboolean legs)
{
	if ( cg.snap->ps.pm_type == PM_INTERMISSION && cgs.gametype == GT_HUB) return LEGS_IDLE;
	
	if ( !legs && cent->currentState.weaponAnim == WEAPON_MELEE && !(newAnimation >= BOTH_DEATH1 && newAnimation <= BOTH_DEAD3) ) 
	{
		return TORSO_MELEE;
	}
	if ( cent->pe.legs.yawing && newAnimation != LEGS_WALK && newAnimation != LEGS_RUN && ( (currentAnimation >= LEGS_IDLE && currentAnimation <= LEGS_IDLE4) || currentAnimation == LEGS_TURN ) && cgs.gametype == GT_HUB ) {
		return LEGS_TURN;
	}
	
	if ( currentAnimation >= LEGS_IDLE2 && currentAnimation <= LEGS_IDLE4 && newAnimation == LEGS_IDLE )
		return currentAnimation;
		
	if (legs && ( currentAnimation == LEGS_JUMP || currentAnimation == LEGS_JUMPB ) && (newAnimation == TORSO_ATTACK || newAnimation == TORSO_ATTACK2 ) ) return currentAnimation;

	//sync animation time on the torso to that of the legs.  Removes horrible torso sway
	if (!legs && currentAnimation >= LEGS_WALKCR && currentAnimation <= LEGS_TURN )
	{
		cent->pe.torso.animationTime = cent->pe.legs.animationTime;
		cent->pe.torso2.animationTime = cent->pe.legs2.animationTime;
	}
	/*if ( ( currentAnimation == TORSO_ATTACK || currentAnimation == TORSO_ATTACK2) && !legs) 
		return currentAnimation;*/
	if ( newAnimation == LEGS_IDLE && cgs.gametype != GT_HUB ) return currentAnimation;
	
	if ( newAnimation == TORSO_RAISE && currentAnimation != TORSO_DROP ) return currentAnimation;
	
	return newAnimation;
}
static int CG_BlendPlayerAnimation( clientInfo_t *ci, animNumber_t newAnimation, animNumber_t oldAnimation, lerpFrame_t *current, lerpFrame_t *new)
{
	// see if the animation sequence is switching
	if ( newAnimation != oldAnimation || !current->animation) {
		//CG_Printf("[NOTICE]Blending from %s to %s\n", BG_FindAnimationByNumber(oldAnimation), BG_FindAnimationByNumber(newAnimation));
		if (current->animation)
		{
			Com_Memcpy(new, current, sizeof(lerpFrame_t) );
			CG_SetLerpFrameAnimation( ci, current, newAnimation );
		} else {
			//transition from null animation to first animation.
			CG_SetLerpFrameAnimation( ci, current, newAnimation );
			Com_Memcpy(new, current, sizeof(lerpFrame_t) );
		}
		return ANIM_TRANSITION_TIME;

	}
	return 0;
}

/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation( centity_t *cent, refEntity_t *legs, refEntity_t *legs2, refEntity_t *torso, refEntity_t *torso2 ) {
	clientInfo_t	*ci;
	int				clientNum;
	float			speedScale;
	animNumber_t	newAnimation;

	speedScale = 1.0f;
	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer ) {
		legs->oldframe = legs->frame = torso->frame = torso->oldframe = 0;
		return;
	}

	ci = &cgs.clientinfo[ clientNum ];

	newAnimation = CG_AnimationPriorities(cent, cent->pe.legs.animationNumber, cent->currentState.legsAnim, qtrue );
	if ( cent->pe.legsTransTime <= 0 )
		cent->pe.legsTransTime = CG_BlendPlayerAnimation( ci, newAnimation, cent->pe.legs.animationNumber, &cent->pe.legs, &cent->pe.legs2 );

	if (CG_RunLerpFrame( ci, &cent->pe.legs, speedScale ) == qfalse)
	{
		if ( !(cent->pe.legs.animationNumber >= BOTH_DEATH1 && cent->pe.legs.animationNumber <= BOTH_DEAD3) ) {
			int default_anim;
			if (cgs.gametype == GT_HUB) {
				default_anim = LEGS_IDLE;
			} else {
				if ( cent->currentState.weapon == WP_PISTOL || cent->currentState.weapon == WP_HIGHCAL ) {
					default_anim = LEGS_COMBATPISTOLIDLE;
				} else {
					default_anim = LEGS_COMBATIDLE;
				}
			}
			if ( cent->pe.legsTransTime <= 0 )
			{
				cent->pe.legsTransTime = CG_BlendPlayerAnimation( ci, default_anim, cent->pe.legs.animationNumber, &cent->pe.legs, &cent->pe.legs2  );
				CG_RunLerpFrame( ci, &cent->pe.legs, 1.0f );	
			}
		}
	}
	legs->oldframe = cent->pe.legs.oldFrame;
	legs->frame = cent->pe.legs.frame;
	legs->backlerp = cent->pe.legs.backlerp;
	if ( cent->pe.legsTransTime > 0 )
	{
		CG_RunLerpFrame( ci, &cent->pe.legs2, speedScale );
		legs2->oldframe = cent->pe.legs2.oldFrame;
		legs2->frame = cent->pe.legs2.frame;
		legs2->backlerp = cent->pe.legs2.backlerp;
		//if (cg_debugAnim.integer == 2 ) CG_Printf("Legs");
	}
	

	newAnimation = CG_AnimationPriorities(cent, cent->pe.torso.animationNumber, cent->currentState.torsoAnim, qfalse );
	if ( cent->pe.torsoTransTime <= 0 )
		cent->pe.torsoTransTime = CG_BlendPlayerAnimation( ci, newAnimation, cent->pe.torso.animationNumber, &cent->pe.torso, &cent->pe.torso2 );

	if (CG_RunLerpFrame( ci, &cent->pe.torso, speedScale ) == qfalse)
	{
		if ( !(cent->pe.torso.animationNumber >= BOTH_DEATH1 && cent->pe.torso.animationNumber <= BOTH_DEAD3) ) {
			int default_anim;
			if (cgs.gametype == GT_HUB) {
				default_anim = LEGS_IDLE;
			} else {
				if ( cent->currentState.weapon == WP_PISTOL || cent->currentState.weapon == WP_HIGHCAL ) {
					default_anim = LEGS_COMBATPISTOLIDLE;
				} else {
					default_anim = LEGS_COMBATIDLE;
				}
			}
			if ( cent->pe.torsoTransTime <= 0 )
			{
				cent->pe.torsoTransTime = CG_BlendPlayerAnimation( ci, default_anim, cent->pe.torso.animationNumber, &cent->pe.torso, &cent->pe.torso2 );
				CG_RunLerpFrame( ci, &cent->pe.torso, 1.0f );	
			}
		}
	}

	torso->oldframe = cent->pe.torso.oldFrame;
	torso->frame = cent->pe.torso.frame;
	torso->backlerp = cent->pe.torso.backlerp;
	if ( cent->pe.torsoTransTime > 0 )
	{
		CG_RunLerpFrame( ci, &cent->pe.torso2, speedScale );
		torso2->oldframe = cent->pe.torso2.oldFrame;
		torso2->frame = cent->pe.torso2.frame;
		torso2->backlerp = cent->pe.torso2.backlerp;
	}
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
static void CG_SwingAngles( float destination, float swingTolerance, float clampTolerance,
					float speed, float *angle, qboolean *swinging ) {
	float	swing;
	float	move;
	float	scale;

	if ( !*swinging ) {
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );
		if ( swing > swingTolerance || swing < -swingTolerance ) {
			*swinging = qtrue;
		}
	}

	if ( !*swinging ) {
		return;
	}
	
	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract( destination, *angle );
	scale = fabs( swing );
	if ( scale < swingTolerance * 0.5 ) {
		scale = 0.5;
	} else if ( scale < swingTolerance ) {
		scale = 1.0;
	} else {
		scale = 2.0;
	}

	// swing towards the destination angle
	if ( swing >= 0 ) {
		move = cg.frametime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	} else if ( swing < 0 ) {
		move = cg.frametime * scale * -speed;
		if ( move <= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );
	if ( swing > clampTolerance ) {
		*angle = AngleMod( destination - (clampTolerance - 1) );
	} else if ( swing < -clampTolerance ) {
		*angle = AngleMod( destination + (clampTolerance - 1) );
	}
}

/*
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch( centity_t *cent, vec3_t torsoAngles ) {
	int		t;
	float	f;

	t = cg.time - cent->pe.painTime;
	if ( t >= PAIN_TWITCH_TIME ) {
		return;
	}

	f = 1.0 - (float)t / PAIN_TWITCH_TIME;

	if ( cent->pe.painDirection ) {
		torsoAngles[ROLL] += 20 * f;
	} else {
		torsoAngles[ROLL] -= 20 * f;
	}
}


/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void CG_PlayerAngles( centity_t *cent,
	vec3_t legsAngles, vec3_t legs[3],
	vec3_t torsoAngles, vec3_t torso[3],
	vec3_t headAngles, vec3_t head[3] )
{
	float		dest;
	static	int	movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 };
	vec3_t		velocity;
	float		speed;
	int			dir, clientNum;
	clientInfo_t	*ci;

	VectorCopy( cent->lerpAngles, headAngles );
	headAngles[YAW] = AngleMod( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( cent->currentState.legsAnim  != LEGS_IDLE 
		|| cent->currentState.torsoAnim != TORSO_STAND  ) {
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue;	// always center
		cent->pe.torso.pitching = qtrue;	// always center
		cent->pe.legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	if ( cent->currentState.eFlags & EF_DEAD ) {
		// don't let dead bodies twitch
		dir = 0;
	} else {
		dir = cent->currentState.angles2[YAW];
		if ( dir < 0 || dir > 7 ) {
			CG_Error( "Bad player movement angle" );
		}
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[ dir ];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[ dir ];

	// torso
	CG_SwingAngles( torsoAngles[YAW], 25, 90, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
	CG_SwingAngles( legsAngles[YAW], 40, 90, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );

	torsoAngles[YAW] = cent->pe.torso.yawAngle;
	legsAngles[YAW] = cent->pe.legs.yawAngle;


	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if ( headAngles[PITCH] > 180 ) {
		dest = (-360 + headAngles[PITCH]) * 0.75f;
	} else {
		dest = headAngles[PITCH] * 0.75f;
	}
	CG_SwingAngles( dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
	torsoAngles[PITCH] = cent->pe.torso.pitchAngle;

	//
	clientNum = cent->currentState.clientNum;
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		ci = &cgs.clientinfo[ clientNum ];
		if ( ci->fixedtorso ) {
			torsoAngles[PITCH] = 0.0f;
		}
	}

	// --------- roll -------------


	// lean towards the direction of travel
	VectorCopy( cent->currentState.pos.trDelta, velocity );
	speed = VectorNormalize( velocity );
	if ( speed ) {
		vec3_t	axis[3];
		float	side;

		speed *= 0.02f;

		AnglesToAxis( legsAngles, axis );
		side = speed * DotProduct( velocity, axis[1] );
		legsAngles[ROLL] -= side;

		side = speed * DotProduct( velocity, axis[0] );
		legsAngles[PITCH] += side;
	}

	//
	clientNum = cent->currentState.clientNum;
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		ci = &cgs.clientinfo[ clientNum ];
		if ( ci->fixedlegs ) {
			legsAngles[YAW] = torsoAngles[YAW];
			legsAngles[PITCH] = 0.0f;
			legsAngles[ROLL] = 0.0f;
		}
	}

	// pain twitch
	CG_AddPainTwitch( cent, torsoAngles );

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
}


//==========================================================================

/*
===============
CG_BreathPuffs
===============
*/
static void CG_BreathPuffs( centity_t *cent, refEntity_t *head) {
	clientInfo_t *ci;
	vec3_t up, origin;
	int contents;

	ci = &cgs.clientinfo[ cent->currentState.number ];

	if (!cg_enableBreath.integer) {
		return;
	}
	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson) {
		return;
	}
	if ( cent->currentState.eFlags & EF_DEAD ) {
		return;
	}
	contents = trap_CM_PointContents( head->origin, 0 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}
	if ( ci->breathPuffTime > cg.time ) {
		return;
	}

	VectorSet( up, 0, 0, 8 );
	VectorMA(head->origin, 8, head->axis[0], origin);
	VectorMA(origin, -4, head->axis[2], origin);
	CG_SmokePuff( origin, up, 16, 1, 1, 1, 0.66f, 1500, cg.time, cg.time + 400, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
	ci->breathPuffTime = cg.time + 2000;
}

/*
===============
CG_DustTrail
===============
*/
static void CG_DustTrail( centity_t *cent ) {
	int				anim;
	localEntity_t	*dust;
	vec3_t end, vel;
	trace_t tr;

	if (!cg_enableDust.integer)
		return;

	if ( cent->dustTrailTime > cg.time ) {
		return;
	}

	anim = cent->pe.legs.animationNumber;
	if ( anim != LEGS_LANDB && anim != LEGS_LAND ) {
		return;
	}

	cent->dustTrailTime += 40;
	if ( cent->dustTrailTime < cg.time ) {
		cent->dustTrailTime = cg.time;
	}

	VectorCopy(cent->currentState.pos.trBase, end);
	end[2] -= 64;
	CG_Trace( &tr, cent->currentState.pos.trBase, NULL, NULL, end, cent->currentState.number, MASK_PLAYERSOLID );

	if ( !(tr.surfaceFlags & SURF_DUST) )
		return;

	VectorCopy( cent->currentState.pos.trBase, end );
	end[2] -= 16;

	VectorSet(vel, 0, 0, -30);
	dust = CG_SmokePuff( end, vel,
				  24,
				  .8f, .8f, 0.7f, 0.33f,
				  500,
				  cg.time,
				  0,
				  0,
				  cgs.media.dustPuffShader );
}

/*
===============
CG_TrailItem
===============
*/
static void CG_TrailItem( centity_t *cent, qhandle_t hModel ) {
	refEntity_t		ent;
	vec3_t			angles;
	vec3_t			axis[3];

	VectorCopy( cent->lerpAngles, angles );
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis( angles, axis );

	memset( &ent, 0, sizeof( ent ) );
	VectorMA( cent->lerpOrigin, -16, axis[0], ent.origin );
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis( angles, ent.axis );

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene( &ent );
}


/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite( centity_t *cent, qhandle_t shader )
{
	clientInfo_t *ci = cgs.clientinfo + cent->currentState.clientNum;
	float dist;

	refEntity_t ent;
	memset( &ent, 0, sizeof( ent ) );

	if( ci->modelType == MT_X42 && ci->hasEye )
	{
		//VectorAdd( cent->lerpOrigin, ci->eye.origin, ent.origin );
		VectorCopy( cent->lerpOrigin, ent.origin );
		ent.origin[2] += ci->eye.origin[2];
		ent.origin[2] += -6.0f;
	}
	else
	{
		VectorCopy( cent->lerpOrigin, ent.origin );
		ent.origin[2] += 52.0f;
	}
	
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	
	ent.radius = 8.0F + sinf( 2 * M_PI * 0.001F * cg.time );
	
	if( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson )
		ent.renderfx = RF_THIRD_PERSON;		// only show in mirrors
	
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;

	dist = Vec3_Dist( cg.refdef.vieworg, ent.origin );

	dist *= 1.0F / 425.0F;

	if( dist > 1.0f )
		dist = 1.0f;
	
	dist *= dist;

	dist = 1.0f - dist;

	ent.shaderRGBA[3] = (byte)(0xFF * dist);

	if ( dist > 1e-5 ) {
		trap_R_AddRefEntityToScene( &ent );
	}
}



/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites( centity_t *cent )
{
	if( cent->currentState.eFlags & EF_CONNECTION )
	{
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader );
		return;
	}

	if( cent->currentState.eFlags & EF_TALK && !cent->currentState.eFlags & EF_DEAD )
	{
		CG_PlayerFloatSprite( cent, cgs.media.balloonShader );
		return;
	}

	if( cent->currentState.eFlags & EF_AWARD_CAP )
	{
		CG_PlayerFloatSprite( cent, cgs.media.medalCapture );
		return;
	}

	if( !(cent->currentState.eFlags & EF_DEAD) && 
		cg.snap->ps.persistant[PERS_TEAM] == cgs.clientinfo[ cent->currentState.clientNum ].team &&
		cgs.gametype >= GT_TEAM && cgs.gametype != GT_HUB )
	{
		return;
	}

	if( cent->currentState.clientNum < MAX_PLAYERS )
		return;
	
	if ( cg_spacetrader.integer ) {
		if (sql( "SELECT npcs_sprites.npc[ clients.client[ ?1 ].id ].sprite;", "i", cent->currentState.clientNum ) )
		{
			int i = sqlint(0);
			sqldone();

			if ( i>= 0 ) {
				CG_PlayerFloatSprite( cent, cgs.media.npc_sprites[ i ] );
			}
		}
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define	SHADOW_DISTANCE		128.0f
static qboolean CG_PlayerShadow( centity_t *cent, float *shadowPlane ) {
	vec3_t		end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
	trace_t		trace;
	float		alpha;

	*shadowPlane = 0;

	if ( cg_shadows.integer == 0 ) {
		return qfalse;
	}
	if ( cg_cubemap.integer != 0 ) {
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction == 1.0f || trace.startsolid || trace.allsolid ) {
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1;

	// fade the shadow out with height
	alpha = 1.0f - trace.fraction;

	// bk0101022 - hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f ) 

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, 
		cent->pe.legs.yawAngle, alpha,alpha,alpha,1.0f, qfalse, 24.0f, qtrue );

	return qtrue;
}


/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
// Scott: remove player splashes
/*static void CG_PlayerSplash( centity_t *cent ) {
	vec3_t		start, end;
	trace_t		trace;
	int			contents;
	polyVert_t	verts[4];

	if ( !cg_shadows.integer ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, end );
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = trap_CM_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, start );
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = trap_CM_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	if ( trace.fraction == 1.0 ) {
		return;
	}

	// create a mark polygon
	VectorCopy( trace.endpos, verts[0].xyz );
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[1].xyz );
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[2].xyz );
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[3].xyz );
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.wakeMarkShader, 4, verts );
}
*/


/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team )
{

	REF_PARAM( state );
	REF_PARAM( team );

	trap_R_AddRefEntityToScene( ent );
	if ( CG_ST_exec( CG_ST_SHIELD, ent, state ) )
	{
		trap_R_AddRefEntityToScene( ent );
	}
}

/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts( vec3_t normal, int numVerts, polyVert_t *verts )
{
	int				i, j;
	float			incoming;
	vec3_t			ambientLight;
	vec3_t			lightDir;
	vec3_t			directedLight;

	trap_R_LightForPoint( verts[0].xyz, ambientLight, directedLight, lightDir );

	for (i = 0; i < numVerts; i++) {
		incoming = DotProduct (normal, lightDir);
		if ( incoming <= 0 ) {
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		} 
		j = ( ambientLight[0] + incoming * directedLight[0] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[0] = j;

		j = ( ambientLight[1] + incoming * directedLight[1] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[1] = j;

		j = ( ambientLight[2] + incoming * directedLight[2] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}
/*
===============
CG_Player
===============
*/
void CG_Player( centity_t *cent )
{
	clientInfo_t	*ci;
	refEntity_t		legs;
	refEntity_t		torso;
	refEntity_t		head;
	refEntity_t		legs2;
	refEntity_t		torso2;
	refEntity_t		head2;
	int				clientNum;
	int				renderfx;
	qboolean		shadow;
	float			shadowPlane;
	vec3_t			legsAngles, torsoAngles, headAngles;
	
	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if( clientNum < 0 || clientNum >= MAX_CLIENTS )
		CG_Error( "Bad clientNum on player entity");
	
	ci = &cgs.clientinfo[ clientNum ];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if( !ci->infoValid )
		return;

	// get the player model information
	renderfx = 0;
	if( cent->currentState.number == cg.snap->ps.clientNum)
	{
		if( !cg.renderingThirdPerson )
		{
			renderfx = RF_THIRD_PERSON;			// only draw in mirrors
		}
		else
		{
			if( cg_cameraMode.integer )
				return;
		}
	}


	memset( &legs, 0, sizeof( legs ) );
	memset( &torso, 0, sizeof( torso ) );
	memset( &head, 0, sizeof( head ) );
	memset( &legs2, 0, sizeof( legs2 ) );
	memset( &torso2, 0, sizeof( torso2 ) );
	memset( &head2, 0, sizeof( head2 ) );

	{
		//ToDo: set the player color to something meaningful!
		byte playercolor[4] = { 0xFF, 0, 0, 0xFF };

		*(uint*)legs.shaderRGBA = *(uint*)playercolor;
		*(uint*)torso.shaderRGBA = *(uint*)playercolor;
		*(uint*)head.shaderRGBA = *(uint*)playercolor;
	}

	// get the rotation information
	CG_PlayerAngles( cent, legsAngles, legs.axis, torsoAngles, torso.axis, headAngles, head.axis );
	
	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation( cent, &legs, &legs2, &torso, &torso2 );

#ifdef _DEBUG
	{
		char buff[32];
		float frame;

		trap_Cvar_VariableStringBuffer( "cg_frameHack", buff, sizeof( buff ) );
		frame = atof( buff );

		if( frame > 0 )
		{
			legs.oldframe = frame - 1;
			legs.frame = frame - 1;
			legs.backlerp = 0;

			torso.oldframe = frame - 1;
			torso.frame = frame - 1;
			torso.backlerp = 0;
		}
	}
#endif

	// add the talk baloon or disconnect icon
	CG_PlayerSprites( cent );

	// add the shadow
	shadow = CG_PlayerShadow( cent, &shadowPlane );

	// add a water splash if partially in and out of water
	// no player splashes any more.
	//CG_PlayerSplash( cent );
	
	renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all
	//
	// add the legs
	//
	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;

	VectorCopy( cent->lerpOrigin, legs.origin );

	VectorCopy( cent->lerpOrigin, legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy( legs.origin, legs.oldorigin ); //don't positionally lerp at all

	if( ci->modelType == MT_X42 )
	{
		animGroupFrame_t groups[2];
		boneOffset_t offsets[2];
		animGroupTransition_t transitions[2];
		
		animGroupFrame_t groups2[2];
		
		groups[0].animGroup = 0;
		groups[0].frame0 = legs.oldframe;
		groups[0].frame1 = legs.frame;
		groups[0].interp = 1.0F - legs.backlerp;
		groups[0].wrapFrames = legs.renderfx & RF_WRAP_FRAMES;

		groups[1].animGroup = 1;
		groups[1].frame0 = torso.oldframe;
		groups[1].frame1 = torso.frame;
		groups[1].interp = 1.0F - torso.backlerp;
		groups[1].wrapFrames = torso.renderfx & RF_WRAP_FRAMES;
		
		//blend groups
		if (cent->pe.legsTransTime > 0 )
		{
			groups2[0].animGroup = 0;
			groups2[0].frame0 = legs2.oldframe;
			groups2[0].frame1 = legs2.frame;
			groups2[0].interp = 1.0F - legs2.backlerp;
			groups2[0].wrapFrames = legs2.renderfx & RF_WRAP_FRAMES;
		} else {
			groups2[0].animGroup = 0;
			groups2[0].frame0 = legs.oldframe;
			groups2[0].frame1 = legs.frame;
			groups2[0].interp = 1.0F - legs.backlerp;
			groups2[0].wrapFrames = legs.renderfx & RF_WRAP_FRAMES;
		}
		//blend groups
		if (cent->pe.torsoTransTime > 0 )
		{
			groups2[1].animGroup = 1;
			groups2[1].frame0 = torso2.oldframe;
			groups2[1].frame1 = torso2.frame;
			groups2[1].interp = 1.0F - torso2.backlerp;
			groups2[1].wrapFrames = torso2.renderfx & RF_WRAP_FRAMES;
		} else {
			groups2[1].animGroup = 1;
			groups2[1].frame0 = torso.oldframe;
			groups2[1].frame1 = torso.frame;
			groups2[1].interp = 1.0F - torso.backlerp;
			groups2[1].wrapFrames = torso.renderfx & RF_WRAP_FRAMES;
		}

		memset( offsets, 0, sizeof( offsets ) );

		Q_strncpyz( offsets[0].boneName, "spine_base", sizeof( offsets[0].boneName ) );

		Quat_FromEulerAngles( offsets[0].rot, torsoAngles );
		
		Q_strncpyz( offsets[1].boneName, "neck_base", sizeof( offsets[1].boneName ) );
		Quat_FromEulerAngles( offsets[1].rot, headAngles );

			
		if ( cent->pe.legsTransTime > 0 || cent->pe.torsoTransTime > 0 )
		{
			if ( cent->pe.legsTransTime > 0 ) {
				transitions[0].animGroup = 0;
				transitions[0].interp = (float)(ANIM_TRANSITION_TIME - cent->pe.legsTransTime) / ANIM_TRANSITION_TIME;
				cent->pe.legsTransTime  -= cg.frametime;
			} else {
				transitions[0].animGroup = 0;
				transitions[0].interp = 1.0f;
			}
			if ( cent->pe.torsoTransTime > 0 ) {
				transitions[1].animGroup = 1;
				transitions[1].interp = (float)(ANIM_TRANSITION_TIME - cent->pe.torsoTransTime) / ANIM_TRANSITION_TIME;
				cent->pe.torsoTransTime -= cg.frametime;
			} else {
				transitions[1].animGroup = 1;
				transitions[1].interp = 1.0f;
			}
			/*if (transitions[0].interp >= 1.0f) transitions[0].interp = 1.0f;
			if (transitions[1].interp >= 1.0f) transitions[1].interp = 1.0f;
			if (transitions[0].interp <= 0.0f) transitions[0].interp = 0.0f;
			if (transitions[1].interp <= 0.0f) transitions[1].interp = 0.0f;*/

			//CG_Printf("[NOTICE]Torso: [%f] [%d,%d] [%d->%d]\n", transitions[1].interp, cent->pe.torsoTransTime, cg.frametime, groups2[1].frame0, groups[1].frame0);
			legs.poseData = trap_R_BuildPose2( legs.hModel, groups2, offsets, groups, offsets, transitions );

		} else {
			legs.poseData = trap_R_BuildPose( legs.hModel, groups, lengthof( groups ), offsets, lengthof( offsets ) );
		}

		ci->hasEye = trap_R_LerpTagFromPose( &ci->eye, legs.hModel, legs.poseData, "tag_eye" );

		//x42 characters have their feet at zero,
		//but MD3s are set up on this odd offset thing: adjust
		legs.origin[2] -= 24;
	}

	CG_AddRefEntityWithPowerups( &legs, &cent->currentState, ci->team );

	// if the model failed, allow the default nullmodel to be displayed
	if( !legs.hModel )
		return;

	if( ci->modelType == MT_MD3 )
	{
		//add the torso
		torso.hModel = ci->torsoModel;
		if( !torso.hModel )
			return;

		torso.customSkin = ci->torsoSkin;

		VectorCopy( cent->lerpOrigin, torso.lightingOrigin );

		CG_PositionRotatedEntityOnTag( &torso, ci->torsoModel, &legs, ci->legsModel, "tag_torso" );

		torso.shadowPlane = shadowPlane;
		torso.renderfx = renderfx;

		CG_AddRefEntityWithPowerups( &torso, &cent->currentState, ci->team );

		// add the head
		head.hModel = ci->headModel;
		if( !head.hModel )
			return;

		head.customSkin = ci->headSkin;

		VectorCopy( cent->lerpOrigin, head.lightingOrigin );

		CG_PositionRotatedEntityOnTag( &head, ci->headModel, &torso, ci->torsoModel, "tag_head" );

		head.shadowPlane = shadowPlane;
		head.renderfx = renderfx;

		CG_AddRefEntityWithPowerups( &head, &cent->currentState, ci->team );
	}

	CG_BreathPuffs( cent, &head );

	CG_DustTrail( cent );

	//
	// add the gun / barrel / flash
	//
	if ( ci->modelType == MT_MD3 )
	{
		CG_AddPlayerWeapon( &torso, NULL, cent, ci->team );
	} else {
		CG_AddPlayerWeapon( &legs, NULL, cent, ci->team );
	}

	// add powerups floating behind the player
	//CG_PlayerPowerups( cent, &torso );
}


//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity( centity_t *cent ) {
	cent->errorTime = -99999;		// guarantee no error decay added
	cent->extrapolated = qfalse;	

	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, cent->currentState.legsAnim );
	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, cent->currentState.torsoAnim );

	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

	VectorCopy( cent->lerpOrigin, cent->rawOrigin );
	VectorCopy( cent->lerpAngles, cent->rawAngles );

	memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
	cent->pe.legs.yawAngle = cent->rawAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;
	cent->pe.legs.animationNumber = LEGS_IDLE;

	memset( &cent->pe.torso, 0, sizeof( cent->pe.legs ) );
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;
	cent->pe.torso.animationNumber = LEGS_IDLE;

	if ( cg_debugPosition.integer ) {
		CG_Printf("%i ResetPlayerEntity yaw=%i\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}
}

