/*
===========================================================================
Copyright (C) 2006 HermitWorks Entertainment Corporation

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

#include "cg_local.h"

static int a;

#if 0
#define MAX_AD_NAME 64
#define MAX_ADS 10
#define MAX_BILLBOARDS 10

typedef struct {
	char 		name[MAX_AD_NAME];
	char 		class[MAX_AD_NAME];
	int			playCount;
	qhandle_t	image;
	qboolean	playing;
	int			startTime;
	int			playTime;
} ad_t;

ad_t ads[MAX_ADS];

typedef struct {
	qhandle_t 	shader;
	ad_t		*ad;
} billboard_t;

typedef struct {
	qhandle_t shader;
	char name[64];
	int billboards;
} baseshader_t;

baseshader_t baseShaders[] = {
	{0, "wide"},
	{0, "square"},
	{0, "tall"},
	{0, "moa"},
};

billboard_t billboards[MAX_BILLBOARDS];

qboolean CG_AdSetImage(int billboard, int ad)
{
	if (billboard > MAX_BILLBOARDS)
		CG_Error("%d is greater then MAX_BILLBOARD", billboard);
	if (ad > MAX_ADS)
		CG_Error("%d is greater then MAX_ADS", ad);
	if (billboards[billboard].shader == 0)
		CG_Error("No shader found for billboard: %d", billboard);
	if (!ads[ad].name[0])
		CG_Error("%d was not a valid ad index", ad);

	billboards[billboard].ad = &ads[ad];
	ads[ad].playCount++;
	ads[ad].startTime = cg.time;
	//Play for 5 seconds
	ads[ad].playTime = 5 * 1000;
	ads[ad].playing = qtrue;

	return trap_R_SetShaderValue( billboards[billboard].shader, "ad", PARAMCLASS_OBJECT, PARAMTYPE_TEXTURE, 0, 0, PARAMORDER_ROWMAJOR, (void *)ads[ad].image );
}

int CG_AdGetRandom(const char *class)
{
	int i, max_ads=0, possible_ads[MAX_ADS];
	for(i=0; i < MAX_ADS; i++)
	{
		if (ads[i].name && ads[i].name[0] && Q_stricmp(ads[i].class, class) == 0 ) 
		{
			possible_ads[max_ads++]=i;
		}
	}
	return possible_ads[rand() % max_ads];
}

//Cycle out any old ads
void CG_AdCycleOldAds()
{
	int i;
	for ( i=0; i < sizeof(billboards) / sizeof(billboard_t); i++ ) {
		if ( billboards[i].shader != 0 && billboards[i].ad->playing && billboards[i].ad->startTime + billboards[i].ad->playTime < cg.time )
		{
			int newAd = CG_AdGetRandom(billboards[i].ad->class);//CG_ST_exec(CG_ST_SWITCH_AD, i);
			CG_AdSetImage(i, newAd );
		}
	}
}

static qboolean CG_AdRegister(const char *name, const char *class)
{
	int cached_image = 0;
	int i;
	for (i=0; i < MAX_ADS; i++)
	{
		//if we already have this ad registered
		if ( Q_stricmp( ads[i].name, name ) == 0 && Q_stricmp( ads[i].class, class ) == 0 )
		{
			return qtrue;
		} else if ( Q_stricmp( ads[i].name, name ) == 0 )
		{
			cached_image = ads[i].image;
		}
		else if (ads[i].image == 0)
		{
			Q_strncpyz( ads[i].name, name, MAX_AD_NAME );
			Q_strncpyz( ads[i].class, class, MAX_AD_NAME );
			if (cached_image == 0)
			{
				ads[i].image = trap_R_RegisterImage(ads[i].name);
			} else {
				ads[i].image = cached_image;
			}
			return qtrue;
		}
	}
	return qfalse;
}

void CG_RegisterAds()
{
	int i;
	int handle;
	pc_token_t token;
	int billboard_count=0;
	
	memset( ads, 0, sizeof(ads));
	memset( billboards, 0, sizeof(billboards));
	handle = trap_PC_LoadSource( va("ads/%s/ads.txt", "en" ) );
	
	for ( i=0; ; i++ )
	{
		char class[MAX_AD_NAME];
		if ( !trap_PC_ReadToken( handle, &token ) )
		{
			break;
		}
		Q_strncpyz( class, token.string, MAX_AD_NAME );
		if ( !trap_PC_ReadToken( handle, &token ) )
		{
			break;
		}
		if ( token.string[0] == '{' )
		{
			for ( ; ; )
			{
				if ( !trap_PC_ReadToken( handle, &token ) )
					break;

				if ( token.string[0] == '}' )
					break;

				if ( token.string[0] == ';' )
					continue;

				if ( CG_AdRegister( token.string, class ) == qfalse)
					break;
			}
		}
	}
	for ( i=0; i < sizeof(baseShaders) / sizeof(baseshader_t); i++ )
	{
		int j;
		baseShaders[i].shader = trap_R_GetBaseShader( va("textures/ads/%s", baseShaders[i].name) );
		baseShaders[i].billboards = trap_R_GetShaderVariations( baseShaders[i].shader );
		//Setup the billboards right off the start with a default value
		for ( j=0; j < baseShaders[i].billboards; j++)
		{
			qhandle_t variant = trap_R_GetShaderVariant( baseShaders[i].shader, j );
			billboards[billboard_count].shader = variant;
			CG_AdSetImage( billboard_count, CG_AdGetRandom(baseShaders[i].name));
			billboard_count++;
		}
	}
	trap_PC_FreeSource( handle );
	CG_Printf( "Loaded %d billboards\n", billboard_count );
}

#endif
