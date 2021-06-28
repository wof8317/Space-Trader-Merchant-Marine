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
#include "tr_state-local.h"

stateTrack_t track;
stateCounts_t count;

void R_StateInit( void )
{
	int i;

	memset( &track, 0, sizeof( track ) );
	memset( &count, 0, sizeof( count ) );

	R_StateSetupStates();
	R_StateSetupTextureStates();

	//start with the GL defaults set
	for( i = 0; i < TSN_MaxStandardStates; i++ )
		if( track.s[i].reset )
			track.s[i].reset( track.s + i );
	track.numCustom = 0;

	R_StateSetTextureModeCvar( r_textureMode->string );
	R_StateSetTextureMinLodCvar( r_textureLod->integer );
}

void R_StateKill( void )
{
	track.numCustom = 0;
	memset( track.s, 0, sizeof( track.s ) ); //sanity
}

trackedState_t* R_StateRegisterCustomTrackedState( stateReset_t resetFn )
{
	trackedState_t *s;

	if( TSN_FirstCustomState + track.numCustom == TSN_MaxCustomStates )
		return NULL; //can't create a new state

	s = track.s + (TSN_FirstCustomState + track.numCustom);
	memset( s, 0, sizeof( *s ) );
	s->name = (TSN_FirstCustomState + track.numCustom);
	s->reset = resetFn;

	track.numCustom++;

	return s;
}

countedState_t* R_StateRegisterCustomCountedState( uint startCount )
{
	countedState_t *s;

	if( CSN_FirstCustomState + count.numCustom == CSN_MaxCustomStates )
		return NULL; //out of space

	s = count.s + (CSN_FirstCustomState + count.numCustom);
	memset( s, 0, sizeof( *s ) );
	s->name = (CSN_FirstCustomState + count.numCustom);
	s->count = startCount;

	count.numCustom++;

	return s;
}

uint R_StateGetCount( countedStateName_t state )
{
	return count.s[(uint)state].count;
}

void R_StateCountIncrement( countedStateName_t state )
{
	count.s[(uint)state].count++;
}

/*
	This bit is the state set counter code. It attaches a unique
	index to each group of related state sets and can revert all
	state sets up to and including a given group in one go. This
	makes for very efficient state sets as only states that have
	been altered in one pass but not also set in the next are to
	be reset.
*/

static struct
{
	/*
		Nothing mysterious here. Just an array of uints.
		
		0 and ~0 are reserved. 0 (STATE_DEFAULT) is used when
		the corresponding state is set set to its default value.
		~0 (STATE_RESET) means the corresponding state must be
		reset next time a group restore is made, regardless of
		the restore type.
	*/

	stateGroup_t	current;
	stateGroup_t	groups[TSN_MaxStates];
} sg;

#define STATE_DEFAULT 0
#define STATE_RESET ((stateGroup_t)~0)

/*
	Starts a new state group and puts the state
	manager in record mode.
*/
stateGroup_t R_StateBeginGroup( void )
{
	/*
		The counter is reset each frame. If it manages
		to wrap then we have some fairly serious problems
		to deal with already (like, say, a render speed of
		roughly a day or two per frame).
	*/
	sg.current++;

	return sg.current;
}

void R_StateJoinGroup( trackedStateName_t state )
{
	ASSERT( (uint)state < TSN_MaxStates );

	sg.groups[state] = sg.current;
}

void R_StateForceReset( trackedStateName_t state )
{
	ASSERT( (uint)state < TSN_MaxStates );

	sg.groups[state] = STATE_RESET;
}

qboolean R_StateIsCurrent( trackedStateName_t state )
{
	ASSERT( (uint)state < TSN_MaxStates );

	return sg.groups[state] == sg.current;
}

/*
	Restores any render sets set during a particular group.
*/
void R_StateRestoreGroupStates( stateGroup_t group )
{
	int i;
	int numStates = TSN_FirstCustomState + track.numCustom;

	GLimp_LogComment( 4, "BEGIN R_StateRestoreGroupStates( %i )", group );

	for( i = 0; i < numStates; i++ )
		if( sg.groups[i] == group || sg.groups[i] == STATE_RESET )
		{
			if( track.s[i].reset )
				track.s[i].reset( track.s + i );
			sg.groups[i] = STATE_DEFAULT;
		}

	GLimp_LogComment( 4, "END R_StateRestoreGroupStates( %i )", group );
}

void R_StateRestorePriorGroupStates( stateGroup_t group )
{
	int i;
	int numStates = TSN_FirstCustomState + track.numCustom;

	GLimp_LogComment( 4, "BEGIN R_StateRestorePriorGroupStates( %i )", group );

	for( i = 0; i < numStates; i++ )
		if( sg.groups[i] && (sg.groups[i] < group || sg.groups[i] == STATE_RESET) )
		{
			if( track.s[i].reset )
				track.s[i].reset( track.s + i );
			sg.groups[i] = STATE_DEFAULT;
		}

	GLimp_LogComment( 4, "END R_StateRestorePriorGroupStates( %i )", group );
}

void R_StateRestoreSubsequentGroupStates( stateGroup_t group )
{
	int i;
	int numStates = TSN_FirstCustomState + track.numCustom;

	GLimp_LogComment( 4, "BEGIN R_StateRestoreSubsequentGroupStates( %i )", group );

	for( i = 0; i < numStates; i++ )
		if( sg.groups[i] > group || sg.groups[i] == STATE_RESET )
		{
			if( track.s[i].reset )
				track.s[i].reset( track.s + i );
			sg.groups[i] = STATE_DEFAULT;
		}

	GLimp_LogComment( 4, "END R_StateRestoreSubsequentGroupStates( %i )", group );
}

void R_StateRestoreAllStates( void )
{
	int i;
	int numStates = TSN_FirstCustomState + track.numCustom;

	GLimp_LogComment( 4, "BEGIN R_StateRestoreAllStates( )" );

	for( i = 0; i < numStates; i++ )
		if( sg.groups[i] != STATE_DEFAULT && track.s[i].reset )
			track.s[i].reset( track.s + i );

	memset( &sg, 0, sizeof( sg ) );

	GLimp_LogComment( 4, "END R_StateRestoreAllStates( )" );
}

void R_StateRestoreState( trackedStateName_t state )
{
	ASSERT( (uint)state < TSN_MaxStates );

	if( sg.groups[state] != STATE_DEFAULT )
	{
		if( track.s[state].reset )
			track.s[state].reset( track.s + state );
		sg.groups[state] = STATE_DEFAULT;
	}
}

void R_StateForceRestoreState( trackedStateName_t state )
{
	ASSERT( (uint)state < TSN_MaxStates );

	if( track.s[state].reset )
		track.s[state].reset( track.s + state );
	sg.groups[state] = STATE_DEFAULT;
}

void R_StateRestoreTextureStates( void )
{
	int i;
	for( i = TSN_TextureFirst; i <= TSN_TextureLast; i++ )
		if( sg.groups[i] != STATE_DEFAULT )
		{
			if( track.s[i].reset )
				track.s[i].reset( track.s + i );
			sg.groups[i] = STATE_DEFAULT;
		}
}

void R_StateForceRestoreTextureStates( void )
{
	int i;
	for( i = TSN_TextureFirst; i <= TSN_TextureLast; i++ )
	{
		if( track.s[i].reset )
			track.s[i].reset( track.s + i );
		sg.groups[i] = STATE_DEFAULT;
	}
}