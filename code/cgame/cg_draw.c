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

// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles )
{
//	clipHandle_t	cm;
	clientInfo_t	*ci;
//	float			len;
//	vec3_t			origin;
//	vec3_t			mins, maxs;

	REF_PARAM( headAngles );

	ci = &cgs.clientinfo[ clientNum ];

//Disabled to just draw the icon for the players heads
/*	if ( cg_draw3dIcons.integer ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->headOffset, origin );

		CG_Draw3DModel( x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles );
	} else if ( cg_drawIcons.integer ) {*/
		CG_DrawPic( x, y, w, h, ci->modelIcon );
	//}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker( float y ) {
	int			t;
	float		size;
	vec3_t		angles;
	const char	*info;
	const char	*name;
	int			clientNum;

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if ( !cg.attackerTime ) {
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) {
		return y;
	}

	t = cg.time - cg.attackerTime;
	if ( t > ATTACKER_HEAD_TIME ) {
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;
	CG_DrawHead( 640 - size, y, size, size, clientNum, angles );

	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = Info_ValueForKey(  info, "n" );
	y += size;
//	CG_DrawBigString( 640 - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), y, name, 0.5 );

	return y + BIGCHAR_HEIGHT + 2;
}

/*
===============
CG_DamageView

===============
*/
static void CG_DrawDamageView( void ) {
	float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float left, right, top, bottom, w, h;
	int health = cg.predictedPlayerState.stats[STAT_HEALTH];
	
	if ( health > 40 || health <= 0)
		return;

	color[3] = 1.0f - (float)health/100;
	trap_R_SetColor( color );


	left	= 0.0f;
	right	= 640.0f * cgDC.glconfig.xscale + 2.0f*cgDC.glconfig.xbias;
	top		= 0.0f;
	bottom	= 480.0f * cgDC.glconfig.yscale;

	w		= 40.0f * cgDC.glconfig.xscale;
	h		= 40.0f * cgDC.glconfig.yscale;


	//	left
	trap_R_DrawStretchPic( left,	top,		w,			bottom-top, 0.0f, 0.0f, 1.0f, 1.0f, cgs.media.viewDamagedShader );
	//	right
	trap_R_DrawStretchPic( right-w,	top,		w,			bottom-top, 1.0f, 0.0f, 0.0f, 1.0f, cgs.media.viewDamagedShader );
	//	top
	trap_R_DrawStretchPic( left,	top,		right-left,	h,			0.0f, 0.0f, 1.0f, 1.0f, cgs.media.viewDamagedShader_vert );
	//	bottom
	trap_R_DrawStretchPic( left,	bottom-h,	right-left, h,			1.0f, 1.0f, 0.0f, 0.0f, cgs.media.viewDamagedShader_vert );
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	rectDef_t	r;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, cg.latestSnapshotNum, cgs.serverCommandSequence );

	r.x = 0.0f;
	r.y = y + 2.0f;
	r.w = 630.0f;
	r.h = 24.0f;

	trap_R_RenderText( &r, 0.3f, colorWhite, s, -1, TEXT_ALIGN_RIGHT, TEXT_ALIGN_CENTER, TEXT_STYLE_ITALIC, cgs.media.font, -1, 0 );

	return y + 24.0f;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	16
static float CG_DrawFPS( float y ) {
	char		*s;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	int		fps;
	static	int	previous;
	int		t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "%i fps", fps );

		{
			rectDef_t r;
			r.x = 540.0f;
			r.y = y + 2.0f;
			r.w = 80.0f;
			r.h = 16.0f;

			trap_R_RenderText( &r, 0.3f, colorWhite, s, -1, TEXT_ALIGN_RIGHT, TEXT_ALIGN_CENTER, 3, cgs.media.font, -1, 0 );
		}
	}

	return y + BIGCHAR_HEIGHT + 4;
}

static void CG_DrawWaiting( ) {
	int i, t;
	static int waiting_clients;
	for (i=0,t=0; i < MAX_PLAYERS; i++ )
	{
		if ( cgs.clientinfo[i].infoValid == qtrue && cg_entities[i].currentState.eFlags & EF_CONNECTION )
		{
			t |= (1 << i );
		}
	}
	if ( t != waiting_clients ) {
		trap_Cmd_ExecuteText( EXEC_NOW, va("st_lagging %d", t ) );
		waiting_clients = t;
	}
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {

	char		*s;
	rectDef_t	r;
	int			mins, seconds, tens;
	int			msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );

	r.x = 0.0f;
	r.y = y + 2.0f;
	r.w = 630.0f;
	r.h = 24.0f;

	trap_R_RenderText( &r, 0.3f, colorWhite, s, -1, TEXT_ALIGN_RIGHT, TEXT_ALIGN_CENTER, TEXT_STYLE_ITALIC, cgs.media.font, -1, 0 );


	return y + 24.0f;
}




/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void ) {
	float	y;

	y = 96.0f;

	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if ( cg_drawFPS.integer ) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
	if ( cg_drawAttacker.integer ) {
		y = CG_DrawAttacker( y );
	}

}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	int			seconds;
	static qboolean	show_lag=qfalse;
	//int			w;  // bk010215 - FIXME char message[1024];

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart // bk 0102165 - FIXME
		if (show_lag == qtrue ) {
			show_lag = qfalse;
			trap_Cmd_ExecuteText(EXEC_NOW, "st_lag 0");
		}
		return;
	}
	seconds = ( cmd.serverTime - cg.snap->ps.commandTime )/1000;
	if (seconds >= 60 ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
	}

	if ( show_lag == qfalse && seconds > 5 ) {
		show_lag = qtrue;
		trap_Cmd_ExecuteText(EXEC_NOW, "st_lag 1");
	}


	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer.integer || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	x = 640 - 48;
	y = 480 - 144;

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		//CG_DrawBigString( ax, ay, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}




/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(void) {
	float		w, h;
	qhandle_t	hShader;
	float		f;
	float		x, y;
	float 		scale;
	int			ca;
	vec4_t		hcolor;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else {
		hcolor[0] = cg_crosshairColorR.value;
		hcolor[1] = cg_crosshairColorG.value;
		hcolor[2] = cg_crosshairColorB.value;
		hcolor[3] = cg_crosshairColorA.value;
		trap_R_SetColor( hcolor);
	}


	w = h = cg_crosshairSize.value;
	
	scale = cg.xyspeed / 100;

	//never go below 1 for the scale so we don't make the crosshair disapear
	if (scale < 2) scale = 2;
	if (cg.predictedPlayerState.groundEntityNum != ENTITYNUM_NONE && cg.predictedPlayerState.pm_flags & PMF_DUCKED) scale = 1;

	if ( cg.predictedPlayerState.weapon != WP_SHOTGUN ) {
		w *= scale;
		h *= scale;
	} else {
		w *= 2;
		h *= 2;
	}
	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	x = cg_crosshairX.integer + 320.0f - (w)*0.5f;
	y = cg_crosshairY.integer + 240.0f - (h)*0.5f;
	CG_AdjustFrom640( &x, &y, &w, &h );

	ca = cg_drawCrosshair.integer;
	if (ca < 0)
	{
		ca = 0;
	}

	
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];
	//If (in hub) the last time we had a client under our cursor was less then 250 ms ago, change the crosshair, if in hostage, and the player is on the same team.
	if( (cgs.gametype == GT_HUB || ( cgs.gametype == GT_HOSTAGE && cgs.clientinfo[cg.crosshairClientNum].team == cgs.clientinfo[cg.clientNum].team ) ) && cg.time - cg.crosshairClientTime < 250 && cgs.clientinfo[cg.crosshairClientNum].botSkill > 0 )
	{
		if( cg.crosshairClientDist < CHAT_DISTANCE )
		{
			//	if npc is close enough show the talk icon...
			hShader = cgs.media.crosshairTalk;

			//if ( cg.time == cg.crosshairClientTime && cg.predictedPlayerState.weapon != WP_NONE )
			//	trap_SendClientCommand( "holster" );
		} else {
			return;
		}
	} else if (cgs.gametype == GT_HUB) {
		return;
	}

	//if ( cg.time - cg.crosshairClientTime >=250 && cg.predictedPlayerState.weaponstate == WEAPON_HOLSTERED)
	//	trap_SendClientCommand( "unholster" );

	trap_R_DrawStretchPic( x,y,w,h, 0, 0, 1, 1, hShader );
}



/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 1e5f, cg.refdef.viewaxis[0], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY );

	if ( trace.entityNum < MAX_CLIENTS ) {
	
		// if the player is in fog, don't show it
		content = trap_CM_PointContents( trace.endpos, 0 );
		if ( !(content & CONTENTS_FOG) ) {

			// notify ui for effects
			if ( cg.crosshairClientNum != trace.entityNum ) {
				trap_Cmd_ExecuteText( EXEC_NOW, va( "st_crosshair %d", trace.entityNum ) );
			}

			// update the fade timer
			cg.crosshairClientNum	= trace.entityNum;
			cg.crosshairClientTime	= cg.time;
			cg.crosshairClientDist	= 1e5f * trace.fraction;
			return;
		}
	}

	// notify ui that there is no client under the crosshair
	if ( cg.crosshairClientTime > 0 && (cg.time - cg.crosshairClientTime) > 250 ) {
		trap_Cmd_ExecuteText( EXEC_NOW, "st_crosshair -1" );
		cg.crosshairClientTime = 0;
		cg.crosshairClientNum = -1;
	}
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	//float		*color;
	//char		*name;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
#if 0
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( color )
	{
		rectDef_t r;
		int botType;
		r.x = 0.0f;
		r.y = 190.0f;
		r.w = 640.0f;
		r.h = 24.0f;
		
		botType = cgs.clientinfo[ cg.crosshairClientNum ].botType;

		/*if ( botType == BOT_GUARD_EASY || botType == BOT_GUARD_MEDIUM || botType == BOT_GUARD_HARD )
			name = "Guard";
		else*/

		name = cgs.clientinfo[ cg.crosshairClientNum ].name;

		color[3] *= 0.5f;
		trap_R_RenderText( &r,
							0.3f,
							color,
							name,
							-1,
							TEXT_ALIGN_CENTER,
							TEXT_ALIGN_CENTER,
							TEXT_STYLE_SHADOWED,
							cgs.media.font,
							-1,
							0 );
	}
#endif
}

extern char* fn( int number, int flags );

static void CG_DrawHint( void ) {

	rectDef_t r;
	char s[ MAX_INFO_STRING ];
	s[ 0 ] = '\0';

	switch ( cgs.gametype )
	{
	case GT_ASSASSINATE:
		{
			r.x = 0.0f;
			r.y = 140.0f;
			r.w = 640.0f;
			r.h = 24.0f;

			if ( gsi.ASSASSINATE.dead_countdown > 0 ) {
				fn_buffer( s, gsi.ASSASSINATE.dead_countdown, FN_TIME );
			}

			if ( gsi.ASSASSINATE.boss_escaping ) {
				trap_SQL_Run( s, sizeof(s), cgs.media.CG_FUGITIVE_ESCAPE, 0,0,0 );
			}

		} break;
	case GT_MOA:
		{
			r.x = 0.0f;
			r.y = 140.0f;
			r.w = 640.0f;
			r.h = 24.0f;

			if ( gsi.ASSASSINATE.dead_countdown > 0 ) {
				fn_buffer( s, gsi.ASSASSINATE.dead_countdown, FN_TIME );
			}
		} break;
	case GT_HUB:
		{
			r.x = 32.0f;
			r.y = 20.0f;
			r.w = 632.0f;
			r.h = 30.0f;
			if ( cg.crosshairClientNum >= MAX_PLAYERS && cg.crosshairClientNum < MAX_CLIENTS && cg.crosshairClientDist < 90.0f && cg.crosshairClientTime + 200 > cg.time )
				trap_SQL_Run( s, sizeof(s), cgs.media.CG_INTERACT_HINT, 0,0,0 );
			else
				trap_SQL_Run( s, sizeof(s), cgs.media.CG_EXPLORE_HINT, 0,0,0 );

		} break;
	}

	if ( s[0] )
	{
		vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };
		color[ 3 ] = 0.75f+0.25f*sinf( ((float)cgDC.realTime) / 140.0f );

		trap_R_RenderText( &r,
							0.35f,
							color,
							s,
							-1,
							TEXT_ALIGN_CENTER,
							TEXT_ALIGN_CENTER,
							TEXT_STYLE_SHADOWED | TEXT_STYLE_ITALIC,
							cgs.media.font,
							-1,
							0 );
	}
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {

	char s[ 255 ];
	rectDef_t r;
	trap_SQL_Run( s, sizeof(s), cgs.media.CG_NOMORECHANCES, 0,0,0 );
	r.x = 0.0f;
	r.y = 448.0f;
	r.w = 640.0f;
	r.h = 24.0f;
	if ( s[0] )
	{
		vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };
		color[ 3 ] = 0.75f+0.25f*sinf( ((float)cgDC.realTime) / 140.0f );

		trap_R_RenderText( &r,
							0.35f,
							color,
							s,
							-1,
							TEXT_ALIGN_CENTER,
							TEXT_ALIGN_CENTER,
							TEXT_STYLE_SHADOWED | TEXT_STYLE_ITALIC,
							cgs.media.font,
							-1,
							0 );
	}
}

static qboolean CG_DrawScoreboard() {
	static qboolean firstTime = qtrue;
	float fade, *fadeColor;

	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// should never happen in Team Arena
	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}
	
	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}
	//no scoreboard if we are in the hub gametype
	if (cgs.gametype == GT_HUB) return qfalse;

	if ( cg.showScores || 
		cg.predictedPlayerState.pm_type == PM_DEAD || 
		cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			firstTime = qtrue;
			return qfalse;
		}
		fade = *fadeColor;
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}


/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
}

//==================================================================================
/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D( void )
{
	int ui_active =  trap_Key_GetCatcher() & KEYCATCH_UI;

	if( !cg.snap )
		return;

	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}
	
	if ( cg_cubemap.integer != 0 ) { //no 2d when drawing cubemaps
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {

		if ( !ui_active )
		{
			CG_DrawSpectator();
			CG_DrawCrosshairNames();
			CG_DrawCrosshair();
		}

	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {

			if ( cg_drawStatus.integer )
			{
				//if ( !ui_active )
				//	Menu_PaintAll();
			}
     
            //Never draw the out of ammo warnings any more
			//CG_DrawAmmoWarning();

			if ( !ui_active )
			{
				CG_DrawCrosshairNames();
				CG_DrawCrosshair();
				CG_DrawWeaponSelect();
				CG_DrawHint();
				if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
					CG_DrawSpectator();
				}
			}
		}
	}

	CG_DrawLagometer();

	if (!cg_paused.integer) {
		CG_DrawUpperRight();
		CG_DrawWaiting();
	}

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW )
	{
		CG_DrawWarmup();
	}
	
	CG_DrawDamageView();

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	float		separation;
	vec3_t		baseOrg;

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		//CG_DrawInformation();
		return;
	}

	switch( stereoView )
	{
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		CG_Error( "CG_DrawActive: Undefined stereoView" );
	}


	// clear around the rendered view if sized down
	//CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( cg.refdef.vieworg, baseOrg );
	if ( separation != 0 ) {
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg );
	}
	switch ( cg_cubemap.integer )
	{
	case 1: //up
		cg.refdefViewAngles[PITCH]-=90;
		break;
	case 2: //down
		cg.refdefViewAngles[PITCH]+=90;
		break;
	case 3: //left
		cg.refdefViewAngles[YAW]+=90;
		break;
	case 4: //right
		cg.refdefViewAngles[YAW]-=90;
		break;
	case 5: //forward
		break;
	case 6: //back
		cg.refdefViewAngles[YAW]-=180;
		break;
	}
	if ( cg_cubemap.integer != 0 )
	{
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
	}
	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// restore original viewpoint if running stereo
	if ( separation != 0 ) {
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}
}

void CG_DrawOverlays( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback )
{
	REF_PARAM( serverTime );
	REF_PARAM( stereoView );
	REF_PARAM( demoPlayback );

	// draw status bar and other floating elements
 	CG_Draw2D();

	// fade in screen
	if ( 0 )
	{
		int t = cg.time - cgs.levelStartTime;
		if ( t < 3000 )
		{
			float c[4];
			float f;
			c[ 0 ] = c[ 1 ] = c[ 2] = 0.0f;
			f = (float)t / 3000.0f;
			c[ 3 ] = 1.0f - (f*f);
			trap_R_SetColor( c );
			trap_R_DrawStretchPic( 0.0f, 0.0f, 640.0f * cgDC.glconfig.xscale, 480.0f * cgDC.glconfig.yscale, 0.0f, 0.0f, 1.0f, 1.0f, cgs.media.whiteShader );
		}
	}
}
