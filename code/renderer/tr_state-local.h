#ifndef __tr_state_local_h_
#define __tr_state_local_h_

typedef struct stateTrack_s
{
	int				numCustom;
	trackedState_t	s[TSN_MaxStates];
} stateTrack_t;

typedef struct stateCounts_s
{
	int				numCustom;
	countedState_t	s[CSN_MaxStates];
} stateCounts_t;

extern stateTrack_t track;
extern stateCounts_t count;

void R_StateSetupStates( void );
void R_StateSetupTextureStates( void );

#define R_StateSetDefault R_StateRestoreState
#define R_StateChanged R_StateJoinGroup

#endif