/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"

#define MAX_AUTOHEADCOLORS 32
static byte head_auto_colors[MAX_AUTOHEADCOLORS][3] = {
	{ 0xFF, 0x00, 0x00 },
	{ 0x00, 0x00, 0xFF },
	{ 0xFF, 0xFF, 0x00 },
	{ 0x00, 0xFF, 0xFF },
	{ 0xFF, 0x00, 0xFF },
	{ 0x7F, 0xFF, 0x00 },
	{ 0x8B, 0x45, 0x13 },
	{ 0xFF, 0xF8, 0xDC },
	{ 0x00, 0x00, 0x00 },
	{ 0x22, 0x8B, 0x22 },
	{ 0x69, 0x69, 0x69 },
	{ 0xFF, 0xA5, 0x00 },
	{ 0x48, 0x3D, 0x8B },
	{ 0x1E, 0x90, 0xFF },
	{ 0x8A, 0x2B, 0xE2 },
	{ 0x80, 0x80, 0x00 },
	{ 0xD8, 0xBF, 0xD8 },
	{ 0x00, 0x8B, 0x8B },
	{ 0xDC, 0x14, 0x3C },
	{ 0x00, 0x00, 0x80 },
	{ 0x9A, 0xCD, 0x32 },
	{ 0x46, 0x82, 0xB4 },
	{ 0x8F, 0xBC, 0x8F },
	{ 0x80, 0x00, 0x80 },
	{ 0x00, 0xFF, 0x7F },
	{ 0xFF, 0x7F, 0x50 },
	{ 0xDB, 0x70, 0x93 },
	{ 0xF0, 0xE6, 0x8C },
	{ 0x90, 0xEE, 0x90 },
	{ 0xFF, 0x14, 0x93 },
	{ 0x7B, 0x68, 0xEE },
	{ 0xEE, 0x82, 0xEE }
};

char	*cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"*death1.wav",
	"*death2.wav",
	"*death3.wav",
	"*jump1.wav",
	"*pain25_1.wav",
	"*pain50_1.wav",
	"*pain75_1.wav",
	"*pain100_1.wav",
	"*falling1.wav",
	"*gasp.wav",
	"*drown.wav",
	"*fall1.wav",
	"*taunt.wav"
};


/*
================
CG_CustomSound

================
*/
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName ) {
	clientInfo_t *ci;
	int			i;
	int myteam;
	clientInfo_t *myself;

	if (cg.snap->ps.pm_flags & PMF_FOLLOW && cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		myteam = cgs.clientinfo[cg.snap->ps.clientNum].team;
		myself = &cgs.clientinfo[cg.snap->ps.clientNum];
	} else {
		myteam = cg.snap->ps.persistant[PERS_TEAM];
		myself = &cgs.clientinfo[cg.clientNum];
	}

	if ( soundName[0] != '*' ) {
		return trap_S_RegisterSound( soundName, qfalse );
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i] ; i++ ) {
		if ( !strcmp( soundName, cg_customSoundNames[i] ) ) {
			if ((cgs.ratFlags & RAT_ALLOWFORCEDMODELS)) {
				if (ci == myself && cgs.mySounds[i]) {
					return cgs.mySounds[i];
				} else if ((myteam != TEAM_FREE && ci->team == myteam) && cgs.teamSounds[i]) {
					return cgs.teamSounds[i];
				} else if (((ci->team != myteam) || (myteam == TEAM_FREE && ci != myself)) && cgs.enemySounds[i]) {
					return cgs.enemySounds[i];
				}
			} 
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
	char		*text_p, *prev;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			skip;
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
		trap_FS_FCloseFile( f );
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
	while ( 1 ) {
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
			} else if ( !Q_stricmp( token, "simple" ) ) {
				ci->footsteps = FOOTSTEP_SIMPLE;
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
		}

		// if it is a number, start parsing animations
		if ( token[0] >= '0' && token[0] <= '9' ) {
			text_p = prev;	// unget the token
			break;
		}
		Com_Printf( "unknown token '%s' is %s\n", token, filename );
	}

	// read information for each frame
	for ( i = 0 ; i < MAX_ANIMATIONS ; i++ ) {

		token = COM_Parse( &text_p );
		if ( !*token ) {
			if( i >= TORSO_GETFLAG && i <= TORSO_NEGATIVE ) {
				animations[i].firstFrame = animations[TORSO_GESTURE].firstFrame;
				animations[i].frameLerp = animations[TORSO_GESTURE].frameLerp;
				animations[i].initialLerp = animations[TORSO_GESTURE].initialLerp;
				animations[i].loopFrames = animations[TORSO_GESTURE].loopFrames;
				animations[i].numFrames = animations[TORSO_GESTURE].numFrames;
				animations[i].reversed = qfalse;
				animations[i].flipflop = qfalse;
				continue;
			}
			break;
		}
		animations[i].firstFrame = atoi( token );
		// leg only frames are adjusted to not count the upper body only frames
		if ( i == LEGS_WALKCR ) {
			skip = animations[LEGS_WALKCR].firstFrame - animations[TORSO_GESTURE].firstFrame;
		}
		if ( i >= LEGS_WALKCR && i<TORSO_GETFLAG) {
			animations[i].firstFrame -= skip;
		}

		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		animations[i].numFrames = atoi( token );

		animations[i].reversed = qfalse;
		animations[i].flipflop = qfalse;
		// if numFrames is negative the animation is reversed
		if (animations[i].numFrames < 0) {
			animations[i].numFrames = -animations[i].numFrames;
			animations[i].reversed = qtrue;
		}

		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		animations[i].loopFrames = atoi( token );

		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		fps = atof( token );
		if ( fps == 0 ) {
			fps = 1;
		}
		animations[i].frameLerp = 1000 / fps;
		animations[i].initialLerp = 1000 / fps;
	}

	if ( i != MAX_ANIMATIONS ) {
		CG_Printf( "Error parsing animation file: %s\n", filename );
		return qfalse;
	}

	// crouch backward animation
	memcpy(&animations[LEGS_BACKCR], &animations[LEGS_WALKCR], sizeof(animation_t));
	animations[LEGS_BACKCR].reversed = qtrue;
	// walk backward animation
	memcpy(&animations[LEGS_BACKWALK], &animations[LEGS_WALK], sizeof(animation_t));
	animations[LEGS_BACKWALK].reversed = qtrue;
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
	animations[TORSO_GETFLAG].flipflop = qtrue;
	animations[TORSO_GUARDBASE].flipflop = qtrue;
	animations[TORSO_PATROL].flipflop = qtrue;
	animations[TORSO_AFFIRMATIVE].flipflop = qtrue;
	animations[TORSO_NEGATIVE].flipflop = qtrue;
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

	len = trap_FS_FOpenFile( filename, NULL, FS_READ );
	if (len>0) {
		return qtrue;
	}
	return qfalse;
}

/*
==========================
CG_FindClientModelFile
==========================
*/
static qboolean	CG_FindClientModelFile( char *filename, int length, clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName, const char *base, const char *ext ) {
	char *team, *charactersFolder;
	int i;

	if (CG_IsTeamGametype()) {
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
	charactersFolder = "";
	while(1) {
		for ( i = 0; i < 2; i++ ) {
			if ( i == 0 && teamName && *teamName ) {
				//								"models/players/characters/sergei/stroggs/lower_lily_red.skin"
				Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, team, ext );
			}
			else {
				//								"models/players/characters/sergei/lower_lily_red.skin"
				Com_sprintf( filename, length, "models/players/%s%s/%s_%s_%s.%s", charactersFolder, modelName, base, skinName, team, ext );
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if (CG_IsTeamGametype()) {
				const char *tmpTeam = (ci->forcedModel) ? skinName : team;
				if ( i == 0 && teamName && *teamName ) {
					//								"models/players/characters/sergei/stroggs/lower_red.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, tmpTeam, ext );
				}
				else {
					//								"models/players/characters/sergei/lower_red.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", charactersFolder, modelName, base, tmpTeam, ext );
				}
			}
			else {
				if ( i == 0 && teamName && *teamName ) {
					//								"models/players/characters/sergei/stroggs/lower_lily.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, ext );
				}
				else {
					//								"models/players/characters/sergei/lower_lily.skin"
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

	if (CG_IsTeamGametype()) {
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
	while(1) {
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
			if (CG_IsTeamGametype()) {
				const char *tmpTeam = (ci->forcedModel) ? headSkinName : team;
				if ( i == 0 &&  teamName && *teamName ) {
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headModelName, teamName, base, tmpTeam, ext );
				}
				else {
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headModelName, base, tmpTeam, ext );
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
==========================
CG_RegisterClientSkin
==========================
*/
static qboolean	CG_RegisterClientSkin( clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName ) {
	char filename[MAX_QPATH];

	/*
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/%slower_%s.skin", modelName, teamName, skinName );
	ci->legsSkin = trap_R_RegisterSkin( filename );
	if (!ci->legsSkin) {
		Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/%slower_%s.skin", modelName, teamName, skinName );
		ci->legsSkin = trap_R_RegisterSkin( filename );
		if (!ci->legsSkin) {
			Com_Printf( "Leg skin load failure: %s\n", filename );
		}
	}


	Com_sprintf( filename, sizeof( filename ), "models/players/%s/%supper_%s.skin", modelName, teamName, skinName );
	ci->torsoSkin = trap_R_RegisterSkin( filename );
	if (!ci->torsoSkin) {
		Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/%supper_%s.skin", modelName, teamName, skinName );
		ci->torsoSkin = trap_R_RegisterSkin( filename );
		if (!ci->torsoSkin) {
			Com_Printf( "Torso skin load failure: %s\n", filename );
		}
	}
	*/
	if ( CG_FindClientModelFile( filename, sizeof(filename), ci, teamName, modelName, skinName, "lower", "skin" ) ) {
		ci->legsSkin = trap_R_RegisterSkin( filename );
	}
	if (!ci->legsSkin) {
		Com_Printf( "Leg skin load failure: %s\n", filename );
	}

	if ( CG_FindClientModelFile( filename, sizeof(filename), ci, teamName, modelName, skinName, "upper", "skin" ) ) {
		ci->torsoSkin = trap_R_RegisterSkin( filename );
	}
	if (!ci->torsoSkin) {
		Com_Printf( "Torso skin load failure: %s\n", filename );
	}

	if ( CG_FindClientHeadFile( filename, sizeof(filename), ci, teamName, headModelName, headSkinName, "head", "skin" ) ) {
		ci->headSkin = trap_R_RegisterSkin( filename );
	}
	if (!ci->headSkin) {
		Com_Printf( "Head skin load failure: %s\n", filename );
	}

	// if any skins failed to load
	if ( !ci->legsSkin || !ci->torsoSkin || !ci->headSkin ) {
		return qfalse;
	}
	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
static qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName, const char *teamName ) {
	char	filename[MAX_QPATH*2];
	const char		*headName;
	char newTeamName[MAX_QPATH*2];

	if ( headModelName[0] == '\0' ) {
		headName = modelName;
	}
	else {
		headName = headModelName;
	}
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", modelName );
	ci->legsModel = trap_R_RegisterModel( filename );
	if ( !ci->legsModel ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/lower.md3", modelName );
		ci->legsModel = trap_R_RegisterModel( filename );
		if ( !ci->legsModel ) {
			Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", modelName );
	ci->torsoModel = trap_R_RegisterModel( filename );
	if ( !ci->torsoModel ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/upper.md3", modelName );
		ci->torsoModel = trap_R_RegisterModel( filename );
		if ( !ci->torsoModel ) {
			Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}
	}

	if( headName[0] == '*' ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/%s.md3", &headModelName[1], &headModelName[1] );
	}
	else {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/head.md3", headName );
	}
	ci->headModel = trap_R_RegisterModel( filename );
	// if the head model could not be found and we didn't load from the heads folder try to load from there
	if ( !ci->headModel && headName[0] != '*' ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/%s.md3", headModelName, headModelName );
		ci->headModel = trap_R_RegisterModel( filename );
	}
	if ( !ci->headModel ) {
		Com_Printf( "Failed to load model file %s\n", filename );
		return qfalse;
	}

	// if any skins failed to load, return failure
	if ( !CG_RegisterClientSkin( ci, teamName, modelName, skinName, headName, headSkinName ) ) {
		if ( teamName && *teamName) {
			Com_Printf( "Failed to load skin file: %s : %s : %s, %s : %s\n", teamName, modelName, skinName, headName, headSkinName );
			if( ci->team == TEAM_BLUE ) {
				Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_BLUETEAM_NAME);
			}
			else {
				Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_REDTEAM_NAME);
			}
			if ( !CG_RegisterClientSkin( ci, newTeamName, modelName, skinName, headName, headSkinName ) ) {
				Com_Printf( "Failed to load skin file: %s : %s : %s, %s : %s\n", newTeamName, modelName, skinName, headName, headSkinName );
				return qfalse;
			}
		} else {
			Com_Printf( "Failed to load skin file: %s : %s, %s : %s\n", modelName, skinName, headName, headSkinName );
			return qfalse;
		}
	}

	// load the animations
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation.cfg", modelName );
	if ( !CG_ParseAnimationFile( filename, ci ) ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/animation.cfg", modelName );
		if ( !CG_ParseAnimationFile( filename, ci ) ) {
			Com_Printf( "Failed to load animation file %s\n", filename );
			return qfalse;
		}
	}

	if ( CG_FindClientHeadFile( filename, sizeof(filename), ci, teamName, headName, headSkinName, "icon", "skin" ) ) {
		ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	}
	else if ( CG_FindClientHeadFile( filename, sizeof(filename), ci, teamName, headName, headSkinName, "icon", "tga" ) ) {
		ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	}

	if ( !ci->modelIcon ) {
		return qfalse;
	}

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

	if (v[0] && (v[0] == 'H' || v[0] == 'h')) {
		float hcolor[4];
		val = atoi(v+1);
		if (val < 0 || val >= 360) {
			val = 0;
		} 
		Q_HSV2RGB((float)val, 1.0, 1.0, hcolor);
		color[0] = hcolor[0];
		color[1] = hcolor[1];
		color[2] = hcolor[2];
		return;
	}

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

void CG_LoadForcedSounds(void) {
	char mySoundModel[MAX_QPATH];
	char teamSoundModel[MAX_QPATH];
	char enemySoundModel[MAX_QPATH];
	char *s;
	int i;

	trap_Cvar_VariableStringBuffer( "cg_mySound", mySoundModel, sizeof( mySoundModel ) );
	trap_Cvar_VariableStringBuffer( "cg_teamSound", teamSoundModel, sizeof( teamSoundModel ) );
	trap_Cvar_VariableStringBuffer( "cg_enemySound", enemySoundModel, sizeof( enemySoundModel ) );

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++ ) {
		s = cg_customSoundNames[i];
		if ( !s ) {
			break;
		}

		cgs.mySounds[i] = 0;
		cgs.teamSounds[i] = 0;
		cgs.enemySounds[i] = 0;

		if (mySoundModel[0]) {
			cgs.mySounds[i] = trap_S_RegisterSound( va("sound/player/%s/%s", mySoundModel, s + 1), qfalse );
		}
		if (teamSoundModel[0]) {
			cgs.teamSounds[i] = trap_S_RegisterSound( va("sound/player/%s/%s", teamSoundModel, s + 1), qfalse );
		}
		if (enemySoundModel[0]) {
			cgs.enemySounds[i] = trap_S_RegisterSound( va("sound/player/%s/%s", enemySoundModel, s + 1), qfalse );
		}
	}


}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo( int clientNum, clientInfo_t *ci ) {
	const char	*dir, *fallback;
	int			i, modelloaded;
	const char	*s;
	char		teamname[MAX_QPATH];

	fallback = DEFAULT_MODEL;;

	teamname[0] = 0;
#ifdef MISSIONPACK
	if(CG_IsTeamGametype()) {
		if( ci->team == TEAM_BLUE ) {
			Q_strncpyz(teamname, cg_blueTeamName.string, sizeof(teamname) );
		} else {
			Q_strncpyz(teamname, cg_redTeamName.string, sizeof(teamname) );
		}
	}
	if( teamname[0] ) {
		strcat( teamname, "/" );
	}
#endif
	modelloaded = qtrue;
	if ( !CG_RegisterClientModelname( ci, ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname ) ) {
		if ( cg_buildScript.integer ) {
			CG_Error( "CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname );
		}

		if (ci->forcedModel && ci->forcedBrightModel) {
			Q_strncpyz(teamname, "/", sizeof(teamname));
			if ( !CG_RegisterClientModelname( ci, DEFAULT_FORCED_MODEL, DEFAULT_FORCED_BRIGHT_SKIN, DEFAULT_FORCED_MODEL, DEFAULT_FORCED_BRIGHT_SKIN, teamname ) ) {
				CG_Error( "DEFAULT_FORCED_MODEL / DEFAULT_FORCED_BRIGHT_SKIN (%s/%s) failed to register", DEFAULT_FORCED_MODEL, DEFAULT_FORCED_BRIGHT_SKIN );
			}
			fallback = DEFAULT_FORCED_MODEL;;
		} else if (ci->forcedModel) {
			Q_strncpyz(teamname, "/", sizeof(teamname));
			if ( !CG_RegisterClientModelname( ci, DEFAULT_FORCED_MODEL, "default", DEFAULT_FORCED_MODEL, "default", teamname ) ) {
				CG_Error( "DEFAULT_FORCED_MODEL (%s) failed to register", DEFAULT_FORCED_MODEL);
			}
			fallback = DEFAULT_FORCED_MODEL;;
		}
		// fall back to default team name
	       	else if(CG_IsTeamGametype()) {
			// keep skin name
			if( ci->team == TEAM_BLUE ) {
				Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME, sizeof(teamname) );
			} else {
				Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME, sizeof(teamname) );
			}
			if ( !CG_RegisterClientModelname( ci, DEFAULT_TEAM_MODEL, ci->skinName, DEFAULT_TEAM_HEAD, ci->skinName, teamname ) ) {
				CG_Error( "DEFAULT_TEAM_MODEL / skin (%s/%s) failed to register", DEFAULT_TEAM_MODEL, ci->skinName );
			}
			fallback = DEFAULT_TEAM_MODEL;
		} else {
			if ( !CG_RegisterClientModelname( ci, DEFAULT_MODEL, "default", DEFAULT_MODEL, "default", teamname ) ) {
				CG_Error( "DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL );
			}
		}
		modelloaded = qfalse;
	}

	ci->newAnims = qfalse;
	if ( ci->torsoModel ) {
		orientation_t tag;
		// if the torso model has the "tag_flag"
		if ( trap_R_LerpTag( &tag, ci->torsoModel, 0, 0, 1, "tag_flag" ) ) {
			ci->newAnims = qtrue;
		}
	}

	// sounds
	dir = ci->modelName;

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

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
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
static void CG_CopyClientInfoModel( clientInfo_t *from, clientInfo_t *to ) {
	VectorCopy( from->headOffset, to->headOffset );
	to->footsteps = from->footsteps;
	to->gender = from->gender;

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
			&& !Q_stricmp( ci->blueTeam, match->blueTeam ) 
			&& !Q_stricmp( ci->redTeam, match->redTeam )
			&& (!CG_IsTeamGametype()|| ci->team == match->team) ) {
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
static void CG_SetDeferredClientInfo( int clientNum, clientInfo_t *ci ) {
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
			 (CG_IsTeamGametype() && ci->team != match->team) ) {
			continue;
		}
		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo( clientNum, ci );
		return;
	}

	// if we are in teamplay, only grab a model if the skin is correct
	if (CG_IsTeamGametype()) {
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			match = &cgs.clientinfo[ i ];
			if ( !match->infoValid || match->deferred ) {
				continue;
			}
			//if ( Q_stricmp( ci->skinName, match->skinName ) ||
			//	(CG_IsTeamGametype() && ci->team != match->team) ) {
			//	continue;
			//}
			if ( ci->team != match->team )  {
				continue;
			}
			ci->deferred = qtrue;
			CG_CopyClientInfoModel( match, ci );
			return;
		}

		if (ci->team != TEAM_SPECTATOR) {
			// load the full model, because we don't ever want to show
			// an improper team skin.  This will cause a hitch for the first
			// player, when the second enters.  Combat shouldn't be going on
			// yet, so it shouldn't matter
			CG_LoadClientInfo( clientNum, ci );
			return;
		}
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

	CG_LoadClientInfo( clientNum, ci );
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
	const char	*local_config;
	int 	local_team;
	qboolean enemy = qfalse;

	ci = &cgs.clientinfo[clientNum];

	configstring = CG_ConfigString( clientNum + CS_PLAYERS );
	if ( !configstring[0] ) {
		memset( ci, 0, sizeof( *ci ) );
		return;		// player just left
	}

	local_config = CG_ConfigString(cg.clientNum + CS_PLAYERS);
	v = Info_ValueForKey(local_config, "t");
	local_team = atoi(v);

	// build into a temp buffer so the defer checks can use
	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );

	newInfo.forcedModel = qfalse;
	newInfo.forcedBrightModel = qfalse;

#ifdef WITH_MULTITOURNAMENT
	// gameId for GT_MULTITOURNAMENT
	v = Info_ValueForKey( configstring, "g" );
	newInfo.gameId = atoi(v);
	if (newInfo.gameId < 0 || newInfo.gameId >= MULTITRN_MAX_GAMES) {
		newInfo.gameId = 0;
	}
#endif

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz( newInfo.name, v, sizeof( newInfo.name ) );

	// colors
	v = Info_ValueForKey( configstring, "c1" );
	CG_ColorFromString( v, newInfo.color1 );

	v = Info_ValueForKey( configstring, "c2" );
	CG_ColorFromString( v, newInfo.color2 );

	v = Info_ValueForKey( configstring, "pc" );
	newInfo.playerColorIndex = abs(atoi( v )) % MAX_AUTOHEADCOLORS;
	

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

	v = Info_ValueForKey( configstring, "g_redteam" );
	Q_strncpyz(newInfo.redTeam, v, MAX_TEAMNAME);

	v = Info_ValueForKey( configstring, "g_blueteam" );
	Q_strncpyz(newInfo.blueTeam, v, MAX_TEAMNAME);

	// model
	v = Info_ValueForKey( configstring, "model" );

	if (CG_IsTeamGametype()) {
		if (local_team != newInfo.team)
			enemy = 1;
		else
			enemy = 0;
	} else {
		if (cg.clientNum == clientNum) {
			enemy = 0;
		} else {
			enemy = 1;
		}
	}

	if (cgs.ratFlags & RAT_ALLOWFORCEDMODELS && 
			(  (!enemy && cg_teamModel.string[0]) ||
			   ( enemy && cg_enemyModel.string[0])
			)) {
		if (enemy) {
			Q_strncpyz( newInfo.modelName, cg_enemyModel.string, sizeof( newInfo.modelName ) );
		} else if (cg.clientNum == clientNum) {
			Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );
		} else {
			Q_strncpyz( newInfo.modelName, cg_teamModel.string, sizeof( newInfo.modelName ) );
		}

		slash = strchr( newInfo.modelName, '/' );
		if ( !slash ) {
			// modelName didn not include a skin name
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
		} else {
			Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			// truncate modelName
			*slash = 0;
		}
		// replace "pm" with "bright" models for compatibility with
		// configs from other mods
		if (strcmp(newInfo.skinName, "pm") == 0) {
			Q_strncpyz( newInfo.skinName, "bright", sizeof( newInfo.skinName ) );
		}

		if (!(cgs.ratFlags & RAT_BRIGHTMODEL) && Q_stristr(newInfo.skinName, "bright") != NULL) {
			// use default skin (or red/blue) if bright skin is not available
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
		}

		newInfo.forcedBrightModel = (strcmp(newInfo.skinName, "bright") == 0);
		newInfo.forcedModel = qtrue;
	} else if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		if(CG_IsTeamGametype()) {
			Q_strncpyz( newInfo.modelName, DEFAULT_TEAM_MODEL, sizeof( newInfo.modelName ) );
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
		} else {
			trap_Cvar_VariableStringBuffer( "model", modelStr, sizeof( modelStr ) );
			if (!(cgs.ratFlags & RAT_BRIGHTMODEL && cgs.ratFlags & RAT_ALLOWFORCEDMODELS) && Q_stristr(modelStr, "bright") != NULL) {
				Q_strncpyz(modelStr, "smarine/orange", sizeof(modelStr));
			}
			if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
				skin = "default";
			} else {
				*skin++ = 0;
			}


			Q_strncpyz( newInfo.skinName, skin, sizeof( newInfo.skinName ) );
			Q_strncpyz( newInfo.modelName, modelStr, sizeof( newInfo.modelName ) );
		}

		if (CG_IsTeamGametype()) {
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
			// truncate modelName
			*slash = 0;
		}
	}

	// head model
	v = Info_ValueForKey( configstring, "hmodel" );
	if (cgs.ratFlags & RAT_ALLOWFORCEDMODELS && 
			(  (!enemy && cg_teamModel.string[0]) ||
			   ( enemy && cg_enemyModel.string[0])
			)) {
		if (enemy) {
			Q_strncpyz( newInfo.headModelName, cg_enemyModel.string, sizeof( newInfo.headModelName ) );
		} else if (cg.clientNum == clientNum) {
			Q_strncpyz( newInfo.headModelName, v, sizeof( newInfo.headModelName ) );
		} else {
			Q_strncpyz( newInfo.headModelName, cg_teamModel.string, sizeof( newInfo.headModelName ) );
		}

		slash = strchr( newInfo.headModelName, '/' );
		if ( !slash ) {
			// headModelName didn not include a skin name
			Q_strncpyz( newInfo.headSkinName, "default", sizeof( newInfo.headSkinName ) );
		} else {
			Q_strncpyz( newInfo.headSkinName, slash + 1, sizeof( newInfo.headSkinName ) );
			// truncate headModelName
			*slash = 0;
		}
		// replace "pm" with "bright" models for compatibility with
		// configs from other mods
		if (strcmp(newInfo.headSkinName, "pm") == 0) {
			Q_strncpyz( newInfo.headSkinName, "bright", sizeof( newInfo.headSkinName ) );
		}

		if (!(cgs.ratFlags & RAT_BRIGHTMODEL) && Q_stristr(newInfo.headSkinName, "bright") != NULL) {
			// use default skin (or red/blue) if bright skin is not available
			Q_strncpyz( newInfo.headSkinName, "default", sizeof( newInfo.headSkinName ) );
		}

		newInfo.forcedBrightModel = (strcmp(newInfo.skinName, "bright") == 0);
		newInfo.forcedModel = qtrue;
	} else if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		if(CG_IsTeamGametype()) {
			Q_strncpyz( newInfo.headModelName, DEFAULT_TEAM_MODEL, sizeof( newInfo.headModelName ) );
			Q_strncpyz( newInfo.headSkinName, "default", sizeof( newInfo.headSkinName ) );
		} else {
			trap_Cvar_VariableStringBuffer( "headmodel", modelStr, sizeof( modelStr ) );
			if (!(cgs.ratFlags & RAT_BRIGHTMODEL && cgs.ratFlags & RAT_ALLOWFORCEDMODELS) && Q_stristr(modelStr, "bright") != NULL) {
				Q_strncpyz(modelStr, "smarine/orange", sizeof(modelStr));
			}
			if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
				skin = "default";
			} else {
				*skin++ = 0;
			}

			Q_strncpyz( newInfo.headSkinName, skin, sizeof( newInfo.headSkinName ) );
			Q_strncpyz( newInfo.headModelName, modelStr, sizeof( newInfo.headModelName ) );
		}

		if (CG_IsTeamGametype()) {
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

		forceDefer = trap_MemoryRemaining() < 4000000;

		// if we are defering loads, just have it pick the first valid
		if ( forceDefer || (cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading ) ) {
			// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo( clientNum, &newInfo );
			// if we are low on memory, leave them with this model
			if ( forceDefer ) {
				CG_Printf( "Memory is low. Using deferred model.\n" );
				newInfo.deferred = qfalse;
			}
		} else {
			CG_LoadClientInfo( clientNum, &newInfo );
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
				CG_Printf( "Memory is low. Using deferred model.\n" );
				ci->deferred = qfalse;
				continue;
			}
			CG_LoadClientInfo( i, ci );
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

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	animation_t	*anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS ) {
		CG_Error( "Bad animation number: %i", newAnimation );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if ( cg_debugAnim.integer ) {
		CG_Printf( "Anim: %i\n", newAnimation );
	}
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale ) {
	int			f, numFrames;
	animation_t	*anim;

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation ) {
		CG_SetLerpFrameAnimation( ci, lf, newAnimation );
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim->frameLerp ) {
			return;		// shouldn't happen
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
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
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
			if ( cg_debugAnim.integer ) {
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


/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
						int *torsoOld, int *torso, float *torsoBackLerp ) {
	clientInfo_t	*ci;
	int				clientNum;
	float			speedScale;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer ) {
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	if ( cent->currentState.powerups & ( 1 << PW_HASTE ) ) {
		speedScale = 1.5;
	} else {
		speedScale = 1;
	}

	ci = &cgs.clientinfo[ clientNum ];

	// do the shuffle turn frames locally
	if ( cent->pe.legs.yawing && ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLE ) {
		CG_RunLerpFrame( ci, &cent->pe.legs, LEGS_TURN, speedScale );
	} else {
		CG_RunLerpFrame( ci, &cent->pe.legs, cent->currentState.legsAnim, speedScale );
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	CG_RunLerpFrame( ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale );

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
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
static void CG_PlayerAngles( centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3] ) {
	vec3_t		legsAngles, torsoAngles, headAngles;
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
	if ( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != LEGS_IDLE 
		|| ((cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND 
		&& (cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND2)) {
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

		speed *= 0.05f;

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
CG_HasteTrail
===============
*/
static void CG_HasteTrail( centity_t *cent ) {
	localEntity_t	*smoke;
	vec3_t			origin;
	int				anim;

	if ( cent->trailTime > cg.time ) {
		return;
	}
	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
	if ( anim != LEGS_RUN && anim != LEGS_BACK ) {
		return;
	}

	cent->trailTime += 100;
	if ( cent->trailTime < cg.time ) {
		cent->trailTime = cg.time;
	}

	VectorCopy( cent->lerpOrigin, origin );
	origin[2] -= 16;

	smoke = CG_SmokePuff( origin, vec3_origin, 
				  8, 
				  1, 1, 1, 1,
				  500, 
				  cg.time,
				  0,
				  0,
				  cgs.media.hastePuffShader );

	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}

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
	contents = CG_PointContents( head->origin, 0 );
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
	vec3_t end, vel;
	trace_t tr;

	if (!cg_enableDust.integer)
		return;

	if ( cent->dustTrailTime > cg.time ) {
		return;
	}

	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
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
	CG_SmokePuff( end, vel,
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
CG_PlayerFlag
===============
*/
static void CG_PlayerFlag( centity_t *cent, qhandle_t hSkin, refEntity_t *torso ) {
	clientInfo_t	*ci;
	refEntity_t	pole;
	refEntity_t	flag;
	vec3_t		angles, dir;
	int			legsAnim, flagAnim, updateangles;
	float		angle, d;

	// show the flag pole model
	memset( &pole, 0, sizeof(pole) );
	pole.hModel = cgs.media.flagPoleModel;
	VectorCopy( torso->lightingOrigin, pole.lightingOrigin );
	pole.shadowPlane = torso->shadowPlane;
	pole.renderfx = torso->renderfx;
	CG_PositionEntityOnTag( &pole, torso, torso->hModel, "tag_flag" );
	trap_R_AddRefEntityToScene( &pole );

	// show the flag model
	memset( &flag, 0, sizeof(flag) );
	flag.hModel = cgs.media.flagFlapModel;
	flag.customSkin = hSkin;
	VectorCopy( torso->lightingOrigin, flag.lightingOrigin );
	flag.shadowPlane = torso->shadowPlane;
	flag.renderfx = torso->renderfx;

	VectorClear(angles);

	updateangles = qfalse;
	legsAnim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if( legsAnim == LEGS_IDLE || legsAnim == LEGS_IDLECR ) {
		flagAnim = FLAG_STAND;
	} else if ( legsAnim == LEGS_WALK || legsAnim == LEGS_WALKCR ) {
		flagAnim = FLAG_STAND;
		updateangles = qtrue;
	} else {
		flagAnim = FLAG_RUN;
		updateangles = qtrue;
	}

	if ( updateangles ) {

		VectorCopy( cent->currentState.pos.trDelta, dir );
		// add gravity
		dir[2] += 100;
		VectorNormalize( dir );
		d = DotProduct(pole.axis[2], dir);
		// if there is anough movement orthogonal to the flag pole
		if (fabs(d) < 0.9) {
			//
			d = DotProduct(pole.axis[0], dir);
			if (d > 1.0f) {
				d = 1.0f;
			}
			else if (d < -1.0f) {
				d = -1.0f;
			}
			angle = acos(d);

			d = DotProduct(pole.axis[1], dir);
			if (d < 0) {
				angles[YAW] = 360 - angle * 180 / M_PI;
			}
			else {
				angles[YAW] = angle * 180 / M_PI;
			}
			if (angles[YAW] < 0)
				angles[YAW] += 360;
			if (angles[YAW] > 360)
				angles[YAW] -= 360;

			//vectoangles( cent->currentState.pos.trDelta, tmpangles );
			//angles[YAW] = tmpangles[YAW] + 45 - cent->pe.torso.yawAngle;
			// change the yaw angle
			CG_SwingAngles( angles[YAW], 25, 90, 0.15f, &cent->pe.flag.yawAngle, &cent->pe.flag.yawing );
		}

		/*
		d = DotProduct(pole.axis[2], dir);
		angle = Q_acos(d);

		d = DotProduct(pole.axis[1], dir);
		if (d < 0) {
			angle = 360 - angle * 180 / M_PI;
		}
		else {
			angle = angle * 180 / M_PI;
		}
		if (angle > 340 && angle < 20) {
			flagAnim = FLAG_RUNUP;
		}
		if (angle > 160 && angle < 200) {
			flagAnim = FLAG_RUNDOWN;
		}
		*/
	}

	// set the yaw angle
	angles[YAW] = cent->pe.flag.yawAngle;
	// lerp the flag animation frames
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	CG_RunLerpFrame( ci, &cent->pe.flag, flagAnim, 1 );
	flag.oldframe = cent->pe.flag.oldFrame;
	flag.frame = cent->pe.flag.frame;
	flag.backlerp = cent->pe.flag.backlerp;

	AnglesToAxis( angles, flag.axis );
	CG_PositionRotatedEntityOnTag( &flag, &pole, pole.hModel, "tag_flag" );

	trap_R_AddRefEntityToScene( &flag );
}

/*
===============
CG_PlayerTokens
===============
*/
static void CG_PlayerTokens( centity_t *cent, int renderfx ) {
	int			tokens, i, j;
	float		angle;
	refEntity_t	ent;
	vec3_t		dir, origin;
	skulltrail_t *trail;
	trail = &cg.skulltrails[cent->currentState.number];
	tokens = cent->currentState.generic1;
	if ( !tokens ) {
		trail->numpositions = 0;
		return;
	}

	if ( tokens > MAX_SKULLTRAIL ) {
		tokens = MAX_SKULLTRAIL;
	}

	// add skulls if there are more than last time
	for (i = 0; i < tokens - trail->numpositions; i++) {
		for (j = trail->numpositions; j > 0; j--) {
			VectorCopy(trail->positions[j-1], trail->positions[j]);
		}
		VectorCopy(cent->lerpOrigin, trail->positions[0]);
	}
	trail->numpositions = tokens;

	// move all the skulls along the trail
	VectorCopy(cent->lerpOrigin, origin);
	for (i = 0; i < trail->numpositions; i++) {
		VectorSubtract(trail->positions[i], origin, dir);
		if (VectorNormalize(dir) > 30) {
			VectorMA(origin, 30, dir, trail->positions[i]);
		}
		VectorCopy(trail->positions[i], origin);
	}

	memset( &ent, 0, sizeof( ent ) );
	if( cgs.clientinfo[ cent->currentState.clientNum ].team == TEAM_BLUE ) {
		ent.hModel = cgs.media.redCubeModel;
	} else {
		ent.hModel = cgs.media.blueCubeModel;
	}
	ent.renderfx = renderfx;

	VectorCopy(cent->lerpOrigin, origin);
	for (i = 0; i < trail->numpositions; i++) {
		VectorSubtract(origin, trail->positions[i], ent.axis[0]);
		ent.axis[0][2] = 0;
		VectorNormalize(ent.axis[0]);
		VectorSet(ent.axis[2], 0, 0, 1);
		CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);

		VectorCopy(trail->positions[i], ent.origin);
		angle = (((cg.time + 500 * MAX_SKULLTRAIL - 500 * i) / 16) & 255) * (M_PI * 2) / 255;
		ent.origin[2] += sin(angle) * 10;
		trap_R_AddRefEntityToScene( &ent );
		VectorCopy(trail->positions[i], origin);
	}
}


/*
===============
CG_PlayerPowerups
===============
*/
static void CG_PlayerPowerups( centity_t *cent, refEntity_t *torso ) {
	int		powerups;
	clientInfo_t	*ci;

	powerups = cent->currentState.powerups;
	if ( !powerups ) {
		return;
	}

	// quad gives a dlight
	if ( powerups & ( 1 << PW_QUAD ) ) {
		if (cgs.ratFlags & RAT_POWERUPGLOWS) {
			trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1 );
		}
	}

	// flight plays a looped sound
	if ( powerups & ( 1 << PW_FLIGHT ) ) {
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.flightSound );
	}

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	// redflag
	if ( powerups & ( 1 << PW_REDFLAG ) ) {
		if (ci->newAnims) {
			CG_PlayerFlag( cent, cgs.media.redFlagFlapSkin, torso );
		}
		else {
			CG_TrailItem( cent, cgs.media.redFlagModel );
		}
		if (cgs.ratFlags & RAT_POWERUPGLOWS) {
			trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0, 0.2f, 0.2f );
		}
	}

	// blueflag
	if ( powerups & ( 1 << PW_BLUEFLAG ) ) {
		if (ci->newAnims){
			CG_PlayerFlag( cent, cgs.media.blueFlagFlapSkin, torso );
		}
		else {
			CG_TrailItem( cent, cgs.media.blueFlagModel );
		}
		if (cgs.ratFlags & RAT_POWERUPGLOWS) {
			trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1.0 );
		}
	}

	// neutralflag
	if ( powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		if (ci->newAnims) {
			CG_PlayerFlag( cent, cgs.media.neutralFlagFlapSkin, torso );
		}
		else {
			CG_TrailItem( cent, cgs.media.neutralFlagModel );
		}
		if (cgs.ratFlags & RAT_POWERUPGLOWS) {
			trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0, 1.0, 1.0 );
		}
	}

	// haste leaves smoke trails
	if ( powerups & ( 1 << PW_HASTE ) ) {
		CG_HasteTrail( cent );
	}
}

/*
===================
CG_PlayerFloatHealth
===================
*/


void CG_PlayerFloatHealth( centity_t *cent, qboolean armor ) {
	refEntity_t	re;
	vec3_t		origin, delta, dir, vec, up = {0, 0, 1};
	float		len;
	int			i, score, digits[10], numdigits;
	int rf;
	//int xoffset = armor ? - 1 - 3 * HEALTHNUMBER_SIZE : 1 + 3 * HEALTHNUMBER_SIZE;
	int yoffset = armor ? 0 : cg_friendFloatHealthSize.value*1.10;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	if (armor ) {
		score = cgs.clientinfo[cent->currentState.number].armor;
	} else {
		score = cgs.clientinfo[cent->currentState.number].health;
	}

	if (score >= 999) {
		score = 999;
	} else if (score < 0) {
		score = 0;
	}

	memset( &re, 0, sizeof( re ) );
	if (armor) {
		re.shaderRGBA[0] = 0xff;
		re.shaderRGBA[1] = 0;
	} else {
		re.shaderRGBA[0] = 0;
		re.shaderRGBA[1] = 0xff;
	}

	re.shaderRGBA[2] = 0;
	re.shaderRGBA[3] = 0xff;

	re.reType = RT_SPRITE;
	re.renderfx = rf;
	re.radius = cg_friendFloatHealthSize.value/2;

	VectorCopy(cent->lerpOrigin, origin);
	origin[2] += 40 + yoffset;

	VectorSubtract(cg.refdef.vieworg, origin, dir);
	CrossProduct(dir, up, vec);
	VectorNormalize(vec);

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < 20 ) {
		return;
	}

	for (numdigits = 0; !(numdigits && !score); numdigits++) {
		digits[numdigits] = score % 10;
		score = score / 10;
	}

	for (i = 0; i < numdigits; i++) {
		VectorMA(origin, 
			(float) (((float) numdigits / 2) - i) * cg_friendFloatHealthSize.value,
			vec,
			re.origin);
		re.customShader = cgs.media.numberShaders[digits[numdigits-1-i]];
		trap_R_AddRefEntityToScene( &re );
	}
}



/*
   ===============
   CG_PlayerFloatSprite

   Float a sprite over the player's head
   ===============
*/
static void CG_PlayerFloatSprite( centity_t *cent, qhandle_t shader ) {
	int				rf;
	refEntity_t		ent;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );
}


static qhandle_t CG_GetPlayerSpriteShader(centity_t *cent) {
	qhandle_t *shaderarr = cgs.media.friendColorShaders;
	int totalhp;

	if (CG_IsFrozenPlayer(cent)) {
		return cgs.media.friendFrozenShader;
	}

	if (cgs.ratFlags & RAT_FRIENDSWALLHACK) {
		shaderarr = cgs.media.friendThroughWallColorShaders;
	}

	if (cgs.clientinfo[cent->currentState.number].infoValid) {
		totalhp = CG_GetTotalHitPoints(cgs.clientinfo[cent->currentState.number].health,
				cgs.clientinfo[cent->currentState.number].armor);
	} else {
		totalhp = 100;
	}

	// CROSSREF CG_DrawHealthBar()
	if (totalhp > 300) {
		return shaderarr[0];
	} else if (totalhp > 200) {
		return shaderarr[1];
	} else if (totalhp > 150) {
		return shaderarr[2];
	} else if (totalhp > 100) {
		return shaderarr[3];
	} else if (totalhp > 80) {
		return shaderarr[4];
	} else {
		return shaderarr[5];
	}
}

static qboolean CG_FriendVisible(centity_t *cent) {
	vec3_t start;
	trace_t trace;

	VectorCopy( cg.refdef.vieworg, start );
	CG_Trace(&trace, start, vec3_origin, vec3_origin, cent->lerpOrigin, 
			cg.snap->ps.clientNum,
			CONTENTS_SOLID );

	if (trace.fraction == 1.0) {
		return qtrue;
	}
	return qfalse;

	//VectorCopy( cg.refdef.vieworg, start );
	//CG_Trace(&trace, start, vec3_origin, vec3_origin, cent->lerpOrigin, 
	//		cg.snap->ps.clientNum,
	//		CONTENTS_SOLID |CONTENTS_BODY );

	//if (trace.entityNum <= MAX_CLIENTS && trace.entityNum == cent->currentState.clientNum) {
	//	return qtrue;
	//}
	//return qfalse;
}

static void CG_FriendFlagIndicator(centity_t *cent) {
	int powerups = cent->currentState.powerups;
	qhandle_t shader = 0;
	refEntity_t ent;
	int rf;

	if (powerups & ( 1 << PW_REDFLAG)) {
		shader = cgs.media.friendFlagShaderRed;
	} else if (powerups & ( 1 << PW_BLUEFLAG)) {
		shader = cgs.media.friendFlagShaderBlue;
	} else if (powerups & (1 << PW_NEUTRALFLAG)) {
		shader = cgs.media.friendFlagShaderNeutral;
	} else {
		return;
	}

	//if (CG_FriendVisible(cent)) {
	//	return;
	//}


	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 0;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 32;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );

}

static void CG_FriendHudMarker( centity_t *cent ) {
	int team;
	float distance;
	float hfov_x;
	float size;

	team = cgs.clientinfo[ cent->currentState.clientNum ].team;
	if ( !CG_IsTeamGametype()
		       || !cg_friendHudMarker.integer
		       || !(cgs.ratFlags & RAT_FRIENDSWALLHACK)
		       || cg.snap->ps.persistant[PERS_TEAM] != team 
		       || (cent->currentState.eFlags & EF_DEAD 
			       && !CG_IsFrozenPlayer(cent))
		       || cent->currentState.number == cg.snap->ps.clientNum
		       || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}
	distance = Distance( cent->lerpOrigin, cg.predictedPlayerState.origin);
	if (cg_friendHudMarkerMaxDist.integer > 0 && distance > cg_friendHudMarkerMaxDist.value) {
		return;
	}
	distance = MAX(distance, 1.0);
 	hfov_x = cg.refdef.fov_x * M_PI/360;
	size = 1.0/tan(hfov_x) * 150/distance;
	size = MIN(cg_friendHudMarkerMaxScale.value, size);
	size = MAX(cg_friendHudMarkerMinScale.value, size);
	CG_HudBorderMarker ( cent->lerpOrigin,
		       	1.0,
			cg_friendHudMarkerSize.value * size,
			CG_GetPlayerSpriteShader(cent),
			270);
}

/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites( centity_t *cent ) {
	int		team;
	qboolean 	drawFriendInfo = qfalse;

	team = cgs.clientinfo[ cent->currentState.clientNum ].team;
	if ( CG_IsTeamGametype() &&
		cg.snap->ps.persistant[PERS_TEAM] == team &&
		(!(cent->currentState.eFlags & EF_DEAD) 
		 || CG_IsFrozenPlayer(cent)) && 
		cg_drawFriend.integer) {

		drawFriendInfo = qtrue;

		if (CG_IsFrozenPlayer(cent)) {
			CG_PlayerFloatSprite( cent, CG_GetPlayerSpriteShader(cent));
			return;
		}

		if (cgs.ratFlags & (RAT_FRIENDSWALLHACK | RAT_FLAGINDICATOR)) {
				if (!CG_FriendVisible(cent)) { 
					if (cgs.ratFlags & RAT_FLAGINDICATOR) {
						CG_FriendFlagIndicator(cent);
					}
					if (cgs.ratFlags & RAT_FRIENDSWALLHACK) {
						CG_PlayerFloatSprite( cent, CG_GetPlayerSpriteShader(cent));
					}
					return;
				}
		}
	}

	if ( cent->currentState.eFlags & EF_CONNECTION ) {
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader );
		return;
	}

	if ( cent->currentState.eFlags & EF_TALK ) {
		CG_PlayerFloatSprite( cent, cgs.media.balloonShader );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_IMPRESSIVE ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalImpressive );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_EXCELLENT ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalExcellent );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_GAUNTLET ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalGauntlet );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_DEFEND ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalDefend );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_ASSIST ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalAssist );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_CAP ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalCapture );
		return;
	}

	if (drawFriendInfo) {
		if (cgs.ratFlags & RAT_FRIENDSWALLHACK || cg_friendFloatHealth.integer != 2) {
			CG_PlayerFloatSprite( cent, CG_GetPlayerSpriteShader(cent));
		} else {
			CG_PlayerFloatHealth( cent, qfalse );
			CG_PlayerFloatHealth( cent, qtrue );
		}
		return;
	}

	//team = cgs.clientinfo[ cent->currentState.clientNum ].team;
	//if ( !(cent->currentState.eFlags & EF_DEAD) && 
	//	cg.snap->ps.persistant[PERS_TEAM] == team &&
	//	CG_IsTeamGametype()) {
	//	if (cg_drawFriend.integer) {
	//		if (cgs.ratFlags & RAT_FRIENDSWALLHACK || cg_friendFloatHealth.integer != 2) {
	//			CG_PlayerFloatSprite( cent, CG_GetPlayerSpriteShader(cent));
	//		} else {
	//			CG_PlayerFloatHealth( cent, qfalse );
	//			CG_PlayerFloatHealth( cent, qtrue );
	//		}
	//	}
	//	return;
	//}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define	SHADOW_DISTANCE		128
static qboolean CG_PlayerShadow( centity_t *cent, float *shadowPlane ) {
	vec3_t		end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
	trace_t		trace;
	float		alpha;

	*shadowPlane = 0;

	if ( cg_shadows.integer == 0 ) {
		return qfalse;
	}

	// no shadows when invisible
	if ( cent->currentState.powerups & ( 1 << PW_INVIS ) ) {
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction == 1.0 || trace.startsolid || trace.allsolid ) {
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1;

	if ( cg_shadows.integer != 1 ) {	// no mark for stencil or projection shadows
		return qtrue;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// bk0101022 - hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f ) 

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, 
		cent->pe.legs.yawAngle, alpha,alpha,alpha,1, qfalse, 24, qtrue );

	return qtrue;
}


/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash( centity_t *cent ) {
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
	contents = CG_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, start );
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = CG_PointContents( start, 0 );
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

byte CG_GetBrightShellAlpha(void) {
	return (byte)0xff * MAX(MIN(cg_brightShellAlpha.value, cgs.maxBrightshellAlpha), 0.1);
}

byte CG_GetBrightOutlineAlpha(void) {
	return 0xff;
}


/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team, qboolean isMissile,
	       	clientInfo_t *ci, int orderIndicator, qboolean useBlendBrightshell ) {

	if ( state->powerups & ( 1 << PW_INVIS ) ) {
            if( (cgs.dmflags & DF_INVIS) == 0) {
		ent->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene( ent );
            }
	} else {
		/*
		if ( state->eFlags & EF_KAMIKAZE ) {
			if (team == TEAM_BLUE)
				ent->customShader = cgs.media.blueKamikazeShader;
			else
				ent->customShader = cgs.media.redKamikazeShader;
			trap_R_AddRefEntityToScene( ent );
		}
		else {*/
			trap_R_AddRefEntityToScene( ent );
		//}
		
		if(!isMissile && ( ci && !ci->forcedBrightModel )) {
			byte alpha_save = ent->shaderRGBA[3];
			       	//&& !(state->eFlags & EF_DEAD)  ) {
			if ((cgs.ratFlags & RAT_BRIGHTSHELL) && cg_brightShells.integer) {
				ent->shaderRGBA[3] = CG_GetBrightShellAlpha();

				if (cg_brightShells.integer == 1) {
					// useBlendBrightshell is usually true
					// for head models, to make darker auto
					// head colors work
					ent->customShader = useBlendBrightshell ?
					       	cgs.media.brightShellBlend : cgs.media.brightShell;
					trap_R_AddRefEntityToScene( ent );
				} else if (cg_brightShells.integer == 2) {
					ent->customShader = cgs.media.brightShellFlat;
					trap_R_AddRefEntityToScene( ent );
				} else if (cg_brightShells.integer == 3) {
					ent->customShader = cgs.media.brightShellBlend;
					trap_R_AddRefEntityToScene( ent );
				} else {
					ent->customShader = cgs.media.brightShellFlat;
					trap_R_AddRefEntityToScene( ent );
					ent->shaderRGBA[3] = 0xff;
					ent->customShader = useBlendBrightshell ?
					       	cgs.media.brightOutlineBlend : cgs.media.brightOutline;
					trap_R_AddRefEntityToScene( ent );
				}
			} else if ((cgs.ratFlags & RAT_BRIGHTOUTLINE) && cg_brightOutline.integer) {
				if (cg_brightOutline.integer == 1 || cg_brightOutline.integer == 2) {
					ent->customShader = cgs.media.brightShellFlat;
					ent->shaderRGBA[3] = (byte)(0xff * 0.125);
					trap_R_AddRefEntityToScene( ent );

					ent->customShader = (useBlendBrightshell || cg_brightOutline.integer == 2) ?
					       	cgs.media.brightOutlineBlend : cgs.media.brightOutline;
				} else {
					ent->customShader = cgs.media.brightOutlineOpaque;
				}
				ent->shaderRGBA[3] = CG_GetBrightOutlineAlpha();
				trap_R_AddRefEntityToScene( ent );
			}
			ent->shaderRGBA[3] = alpha_save;
		}

		if (CG_IsFrozenPlayerState(state)) {
			byte alpha_save = ent->shaderRGBA[3];
			float thawfrac = 1.0 - (float)(state->generic1 >> 1)/0x7f;
			if (state->generic1 & 1) {
				ent->customShader = cgs.media.thawingShader;
			} else {
				ent->customShader = cgs.media.frozenShader;
			}
			// never make the ice shell entirely transparent
			ent->shaderRGBA[3] = (byte)85 + 170*thawfrac;
			//ent->shaderRGBA[3] = (byte)0xff*1.0;
			trap_R_AddRefEntityToScene( ent );
			ent->shaderRGBA[3] = alpha_save;
		}

		
		if(!isMissile && (cgs.dmflags & DF_PLAYER_OVERLAY) && !(state->eFlags & EF_DEAD)  ) {
		    switch(team) {
			case TEAM_RED:
			    ent->customShader = cgs.media.redOverlay;
			    trap_R_AddRefEntityToScene( ent );
			    break;
			case TEAM_BLUE:
			    ent->customShader = cgs.media.blueOverlay;
			    trap_R_AddRefEntityToScene( ent );
			    break;
			default:
			    ent->customShader = cgs.media.neutralOverlay;
			    trap_R_AddRefEntityToScene( ent );
		    }
		}
                        
		if ( state->powerups & ( 1 << PW_QUAD ) )
		{
			byte color_bak[4] = {0,0,0,0};
			float color[4];
			//if (team == TEAM_RED)
			//	ent->customShader = cgs.media.redQuadShader;
			//else
			//	ent->customShader = cgs.media.quadShader;
			memcpy(color_bak, ent->shaderRGBA, sizeof(color_bak));

			if (cg_quadStyle.integer >= 1 && cg_quadStyle.integer <= 2) {
				int hue;
				ent->customShader = cgs.media.quadShaderSpots;
				trap_R_AddRefEntityToScene( ent );

				ent->customShader = cgs.media.quadShaderBase;

				if (cg_quadStyle.integer == 2) {
					// hue range 200-285
					hue = (cg.time / 3) % 170;
					if (hue < 85) {
						hue = 200 + hue;
					} else {
						hue = 285 - (hue % 85);
					}
				} else {
					hue = (cg.time / 3) % 360;
				}
				
				Q_HSV2RGB(hue,1.0,1.0, color);
				CG_FloatColorToRGBA(color, ent->shaderRGBA);
				ent->shaderRGBA[3] = (byte)0xff * MAX(MIN(cg_quadAlpha.value, 1.0), 0.0);
			} else {
				int hue;
				ent->customShader = cgs.media.quadShaderSpots;
				trap_R_AddRefEntityToScene( ent );

				ent->customShader = cgs.media.quadShaderBase;

				hue = cg_quadHue.integer;
				
				Q_HSV2RGB(hue,1.0,1.0, color);
				CG_FloatColorToRGBA(color, ent->shaderRGBA);
				ent->shaderRGBA[3] = (byte)0xff * MAX(MIN(cg_quadAlpha.value, 1.0), 0.0);
			}

			if (cg_powerupBlink.integer && orderIndicator) {
				switch  (cg_powerupBlink.integer) {
					case 1:
					case 2:
						if ((cg.time / 100) % 12 == orderIndicator-1) {
							trap_R_AddRefEntityToScene( ent );
						}
						break;
					case 3:
					default:
						if ((cg.time / 100) % 9 == 0) {
							trap_R_AddRefEntityToScene( ent );
						}
						break;

				}
			} else {
				trap_R_AddRefEntityToScene(ent);
			}

			memcpy(ent->shaderRGBA, color_bak, sizeof(color_bak));
		}
		if ( state->powerups & ( 1 << PW_REGEN ) ) {
			ent->customShader = cgs.media.regenShader;
			if (cg_powerupBlink.integer && orderIndicator) {
				switch (cg_powerupBlink.integer) {
					case 1:
						if ((cg.time / 100) % 12 == orderIndicator-1+4) {
							trap_R_AddRefEntityToScene( ent );
						}
						break;
					case 2:
						if (((cg.time / 100) % 12 ) >= 4 && ((cg.time / 100 ) % 12) <= 6 ) {
							trap_R_AddRefEntityToScene( ent );
						}
						break;
					case 3:
					default:
						if (((cg.time / 100) % 9 ) == 3 ) {
							trap_R_AddRefEntityToScene( ent );
						}
						break;

				}
			} else {
				if (((cg.time / 100) % 10 ) == 1 ) {
					trap_R_AddRefEntityToScene( ent );
				}
			}
		}
		if ( state->powerups & ( 1 << PW_BATTLESUIT ) ) {
			ent->customShader = cgs.media.battleSuitShader;
			if (cg_powerupBlink.integer && orderIndicator) {
				switch (cg_powerupBlink.integer) {
					case 1:
					case 2:
						if (((cg.time / 100) % 12) == orderIndicator-1+8) {
							trap_R_AddRefEntityToScene( ent );
						}
						break;
					case 3:
					default:
						if (((cg.time / 100) % 9) == 6) {
							trap_R_AddRefEntityToScene( ent );
						}
						break;
				}
			} else {
				trap_R_AddRefEntityToScene( ent );
			}
		}

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

void CG_FloatColorToRGBA(float *color, byte *out) {
	out[0] = color[0]*0xff;
	out[1] = color[1]*0xff;
	out[2] = color[2]*0xff;
	out[3] = color[3]*0xff;
}

void CG_PlayerColorFromString(char *str, float *h, float *s, float *v) {
	char *p;

	*h = 125.0;
	*s = 1.0;
	*v = 1.0;
	if (!str[0]) {
		*s = 0.0;
		*v = 0.0;
		return;
	}
	if (isalpha(str[0])) {
		switch (toupper(str[0])) {
			case 'H':
				// HSV color
				p = &str[1];
				*h = atof(p);
				if ((p = strchr(p, ' ')) != NULL) {
					*s = atof(p);
					if ((p = strchr(p+1, ' ')) != NULL) {
						*v = atof(p);
					}
				}
				*h = MAX(MIN(*h, 360.0), 0.0);
				*s = MAX(MIN(*s, 1.0), 0.0);
				*v = MAX(MIN(*v, 1.0), 0.0);
				break;
			case 'R':
				*h = 0.0;
				break;
			case 'G':
				*h = 125.0;
				break;
			case 'Y':
				*h = 60.0;
				break;
			case 'B':
				*h = 230.0;
				break;
			case 'C':
				*h = 180.0;
				break;
			case 'P':
				*h = 300.0;
				break;
			case 'W':
				*h = 180.0;
				*s = 0.0;
				break;
			case 'O':
				*h = 20.0;
				break;

		}
	} else {
		char cs[3];
		// this is just so we don't have to replicate the color index
		// checks here and cause an implicit dependency
		cs[0] = Q_COLOR_ESCAPE;
		cs[1] = str[0];
		cs[2] = '\0';
		p = cs;
		if (Q_IsColorString(p)) {
			Q_RGB2HSV(g_color_table[ColorIndex(*(p+1))], h, s, v);
		}
	}

}

#define RGBA_SIZE (4*sizeof(byte))
void CG_PlayerGetColors(clientInfo_t *ci, qboolean isDead, int bodyPart, byte *outColor) {
	clientInfo_t *player = &cgs.clientinfo[cg.clientNum];
	int myteam = player->team;

	if (bodyPart < 0 || bodyPart > MCIDX_LEGS) {
		CG_Error( "CG_PlayerGetColors: Invalid body part!\n");
	}

	if (!(cgs.ratFlags & RAT_BRIGHTSHELL && cg_brightShells.integer) 
			&& !(cgs.ratFlags & RAT_BRIGHTOUTLINE && cg_brightOutline.integer) 
			&& (!ci->forcedBrightModel)) {
		float color[4];
		color[0] = color[1] = color[2] = color[3] = 1.0;
		CG_FloatColorToRGBA(color, outColor);
		return;
	}

	if ((!(ci->forcedBrightModel)
			&& ((cg_brightShells.integer || cg_brightOutline.integer ) && cgs.ratFlags & (RAT_BRIGHTSHELL | RAT_BRIGHTOUTLINE))
		       ) && (cgs.gametype == GT_FFA && !(cgs.ratFlags & RAT_ALLOWFORCEDMODELS))) {
		float color[4];
		color[0] = ci->color2[0];
		color[1] = ci->color2[1];
		color[2] = ci->color2[2];
		if (isDead) {
			color[0] *= 0.3;
			color[1] *= 0.3;
			color[2] *= 0.3;
		}
		CG_FloatColorToRGBA(color, outColor);
		return;
	}

	if (myteam == TEAM_SPECTATOR && CG_IsTeamGametype()) {
		switch (ci->team) {
			case TEAM_BLUE:
				memcpy(outColor, cgs.modelRGBA[bodyPart][MODELCOLOR_BLUE], RGBA_SIZE);
				break;
			case TEAM_RED:
				memcpy(outColor, cgs.modelRGBA[bodyPart][MODELCOLOR_RED], RGBA_SIZE);
				break;
			default:
				memcpy(outColor, cgs.modelRGBA[bodyPart][MODELCOLOR_DEFAULT], RGBA_SIZE);
				break;
		}
		return;
	}

	if ((myteam == TEAM_FREE && player != ci)
			|| (myteam != ci->team)) {
		// is enemy
		if (isDead) {
			memcpy(outColor, cgs.corpseRGBA[bodyPart][MODELCOLOR_ENEMY], RGBA_SIZE);
		} else {
			memcpy(outColor, cgs.modelRGBA[bodyPart][MODELCOLOR_ENEMY], RGBA_SIZE);
		}
		return;
	} 
	// teammate/self
	if (isDead) {
		memcpy(outColor, cgs.corpseRGBA[bodyPart][MODELCOLOR_TEAM], RGBA_SIZE);
	} else {
		memcpy(outColor, cgs.modelRGBA[bodyPart][MODELCOLOR_TEAM], RGBA_SIZE);
	}

}


qboolean CG_AllowColoredProjectiles(void) {
	return (cgs.ratFlags & RAT_ALLOWFORCEDMODELS && cgs.ratFlags & (RAT_BRIGHTSHELL | RAT_BRIGHTOUTLINE));
}

void CG_ProjectileColor(team_t team, byte *outColor) {
	clientInfo_t *player = &cgs.clientinfo[cg.clientNum];
	int myteam = player->team;
	int bodyPart = MCIDX_TORSO;
	int coloridx = MODELCOLOR_DEFAULT;

	if (myteam == TEAM_SPECTATOR) {
		switch (team) {
			case TEAM_BLUE:
				coloridx = MODELCOLOR_BLUE;
				break;
			case TEAM_RED:
				coloridx = MODELCOLOR_RED;
				break;
			default:
				break;
		}
	} else if (team == myteam) {
		coloridx = MODELCOLOR_TEAM;
	} else {
		coloridx = MODELCOLOR_ENEMY;
	}

	memcpy(outColor, cgs.modelRGBA[bodyPart][coloridx], RGBA_SIZE);
}

int CG_CountPlayers(team_t team) {
	int i;
	int count = 0;
	clientInfo_t *ci;
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		ci = &cgs.clientinfo[ i ];
		if ( !ci->infoValid ) {
			continue;
		}
		if ( ci->deferred ) {
			continue;
		}
		
		if (ci->team == team) {
			count++;
		}
	}
	return count;
}

void CG_ParseForcedColors( void ) {
	clientInfo_t *player = &cgs.clientinfo[cg.clientNum];
	int myteam = player->team;
	float h,s,v;
	float color[4];
	int bodyPart;

	color[0] = color[1] = color[2] = color[3] = 1.0;


	memset(cgs.modelRGBA, 255, sizeof(cgs.modelRGBA));
	memset(cgs.corpseRGBA, 255, sizeof(cgs.corpseRGBA));

	s = v = 1.0;

	for (bodyPart = MCIDX_HEAD; bodyPart < MCIDX_NUM; ++bodyPart) {
		h = cg_teamHueDefault.value;
		Q_HSV2RGB(h,s,v, color);
		CG_FloatColorToRGBA(color, cgs.modelRGBA[bodyPart][MODELCOLOR_DEFAULT]);
		if (cg_enemyCorpseSaturation.string[0]) {
			s = cg_enemyCorpseSaturation.value;
		}
		if (cg_enemyCorpseValue.string[0]) {
			v = cg_enemyCorpseValue.value;
		}
		Q_HSV2RGB(h,s,v, color);
		CG_FloatColorToRGBA(color, cgs.corpseRGBA[bodyPart][MODELCOLOR_DEFAULT]);
	}

	if (myteam == TEAM_SPECTATOR && CG_IsTeamGametype()) {
		// When spectating team modes, don't apply forced colors to
		// avoid confusion
		
		for (bodyPart = MCIDX_HEAD; bodyPart < MCIDX_NUM; ++bodyPart) {
			s = v = 1.0;

			h = cg_teamHueBlue.value;
			Q_HSV2RGB(h,s,v, color);
			CG_FloatColorToRGBA(color, cgs.modelRGBA[bodyPart][MODELCOLOR_BLUE]);
			if (cg_enemyCorpseSaturation.string[0]) {
				s = cg_enemyCorpseSaturation.value;
			}
			if (cg_enemyCorpseValue.string[0]) {
				v = cg_enemyCorpseValue.value;
			}
			Q_HSV2RGB(h,s,v, color);
			CG_FloatColorToRGBA(color, cgs.corpseRGBA[bodyPart][MODELCOLOR_BLUE]);

			s = v = 1.0;

			h = cg_teamHueRed.value;
			Q_HSV2RGB(h,s,v, color);
			CG_FloatColorToRGBA(color, cgs.modelRGBA[bodyPart][MODELCOLOR_RED]);
			if (cg_enemyCorpseSaturation.string[0]) {
				s = cg_enemyCorpseSaturation.value;
			}
			if (cg_enemyCorpseValue.string[0]) {
				v = cg_enemyCorpseValue.value;
			}
			Q_HSV2RGB(h,s,v, color);
			CG_FloatColorToRGBA(color, cgs.corpseRGBA[bodyPart][MODELCOLOR_RED]);
		}

		return;
	}

	for (bodyPart = MCIDX_HEAD; bodyPart < MCIDX_NUM; ++bodyPart) {
		s = v = 1.0;
		// team color:
		if (bodyPart == MCIDX_HEAD && cg_teamHeadColor.string[0]) {
			CG_PlayerColorFromString(cg_teamHeadColor.string, &h, &s, &v);
		} else if (bodyPart == MCIDX_TORSO && cg_teamTorsoColor.string[0]) {
			CG_PlayerColorFromString(cg_teamTorsoColor.string, &h, &s, &v);
		} else if (bodyPart == MCIDX_LEGS && cg_teamLegsColor.string[0]) {
			CG_PlayerColorFromString(cg_teamLegsColor.string, &h, &s, &v);
		} else if (cg_teamColor.string[0]) {
			CG_PlayerColorFromString(cg_teamColor.string, &h, &s, &v);
		} else if (myteam == TEAM_BLUE) {
			h = cg_teamHueBlue.value;
		} else if (myteam == TEAM_RED) {
			h = cg_teamHueRed.value;
		} else {
			h = cg_teamHueDefault.value;
		}
		Q_HSV2RGB(h,s,v, color);
		CG_FloatColorToRGBA(color, cgs.modelRGBA[bodyPart][MODELCOLOR_TEAM]);
		if (cg_teamCorpseSaturation.string[0]) {
			s = cg_teamCorpseSaturation.value;
		}
		if (cg_teamCorpseValue.string[0]) {
			v = cg_teamCorpseValue.value;
		}
		Q_HSV2RGB(h,s,v, color);
		CG_FloatColorToRGBA(color, cgs.corpseRGBA[bodyPart][MODELCOLOR_TEAM]);

		s = v = 1.0;
		// enemy color:
		if (bodyPart == MCIDX_HEAD && cg_enemyHeadColor.string[0]) {
			CG_PlayerColorFromString(cg_enemyHeadColor.string, &h, &s, &v);
		} else if (bodyPart == MCIDX_TORSO && cg_enemyTorsoColor.string[0]) {
			CG_PlayerColorFromString(cg_enemyTorsoColor.string, &h, &s, &v);
		} else if (bodyPart == MCIDX_LEGS && cg_enemyLegsColor.string[0]) {
			CG_PlayerColorFromString(cg_enemyLegsColor.string, &h, &s, &v);
		} else if (cg_enemyColor.string[0]) {
			CG_PlayerColorFromString(cg_enemyColor.string, &h, &s, &v);
		} else if (myteam == TEAM_BLUE) {
			h = cg_teamHueRed.value;
		} else if (myteam == TEAM_RED) {
			h = cg_teamHueBlue.value;
		} else {
			h = cg_teamHueDefault.value;
		}
		Q_HSV2RGB(h,s,v, color);
		CG_FloatColorToRGBA(color, cgs.modelRGBA[bodyPart][MODELCOLOR_ENEMY]);
		if (cg_enemyCorpseSaturation.string[0]) {
			s = cg_enemyCorpseSaturation.value;
		}
		if (cg_enemyCorpseValue.string[0]) {
			v = cg_enemyCorpseValue.value;
		}
		Q_HSV2RGB(h,s,v, color);
		CG_FloatColorToRGBA(color, cgs.corpseRGBA[bodyPart][MODELCOLOR_ENEMY]);
	}

}


void CG_PlayerAutoHeadColor(clientInfo_t *ci, byte *outColor) {
	int idx = abs(ci->playerColorIndex) % MAX_AUTOHEADCOLORS;
	outColor[0] = head_auto_colors[idx][0];
	outColor[1] = head_auto_colors[idx][1];
	outColor[2] = head_auto_colors[idx][2];
	outColor[3] = 0xff;
}

/*
void CG_PlayerAutoHeadColor(clientInfo_t *ci, byte *outColor) {
	float h,s,v;
	float color[4];
	int team_count = CG_CountPlayers(ci->team);
	int team_idx = 0;
	clientInfo_t *ci2;
	int i;

	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		ci2 = &cgs.clientinfo[ i ];
		if ( !ci2->infoValid ) {
			continue;
		}
		if ( ci2->deferred ) {
			continue;
		}

		if (ci2->team == ci->team) {
			team_idx++;
			if (ci == ci2) {
				break;
			}
		}
	}
	s = v = 1.0;
	h = team_idx * 360.0/team_count;
	Q_HSV2RGB(h,s,v, color);
	color[3] = 1.0;
	CG_FloatColorToRGBA(color, outColor);
}
*/

/*
===============
CG_Player
===============
*/
void CG_Player( centity_t *cent ) {
	clientInfo_t	*ci;
	refEntity_t		legs;
	refEntity_t		torso;
	refEntity_t		head;
	int				clientNum;
	int				renderfx;
	qboolean		shadow;
	float			shadowPlane;
	refEntity_t		skull;
	refEntity_t		powerup;
	int				t;
	float			c;
	float			angle;
	vec3_t			dir, angles;
	qboolean autoHeadColors = qfalse;
	qboolean useDeadColors;

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity");
	}
	ci = &cgs.clientinfo[ clientNum ];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !ci->infoValid ) {
		return;
	}

	// get the player model information
	renderfx = 0;
	if ( cent->currentState.number == cg.snap->ps.clientNum) {
		if (!cg.renderingThirdPerson) {
			renderfx = RF_THIRD_PERSON;			// only draw in mirrors
		} else {
			if (cg_cameraMode.integer) {
				return;
			}
		}
	}

	// hide enemies during Treasure Hunter Hiding phase
	if (cgs.gametype == GT_TREASURE_HUNTER && !CG_THPlayerVisible(cent)) {
		return;
	}


	memset( &legs, 0, sizeof(legs) );
	memset( &torso, 0, sizeof(torso) );
	memset( &head, 0, sizeof(head) );

	useDeadColors = (cent->currentState.eFlags & EF_DEAD && !CG_IsFrozenPlayer(cent)) ? qtrue : qfalse;
	CG_PlayerGetColors(ci, useDeadColors, MCIDX_TORSO, torso.shaderRGBA);
	CG_PlayerGetColors(ci, useDeadColors, MCIDX_LEGS, legs.shaderRGBA);
	if ((ci->forcedBrightModel || (cgs.ratFlags & (RAT_BRIGHTSHELL | RAT_BRIGHTOUTLINE) 
					&& (cg_brightShells.integer || cg_brightOutline.integer) 
					&& (cgs.gametype != GT_FFA || cgs.ratFlags & RAT_ALLOWFORCEDMODELS)))
			&& ci->team != TEAM_SPECTATOR &&
			( (cg_teamHeadColorAuto.integer && ci->team == cg.snap->ps.persistant[PERS_TEAM])
			  || (cg_enemyHeadColorAuto.integer && ci->team != cg.snap->ps.persistant[PERS_TEAM])
			)) {
		CG_PlayerAutoHeadColor(ci, head.shaderRGBA);
		autoHeadColors = qtrue;
	} else {
		CG_PlayerGetColors(ci, useDeadColors, MCIDX_HEAD, head.shaderRGBA);
	}

	// get the rotation information
	CG_PlayerAngles( cent, legs.axis, torso.axis, head.axis );
	
	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
		 &torso.oldframe, &torso.frame, &torso.backlerp );

	// add the talk baloon or disconnect icon
	CG_PlayerSprites( cent );

	// draw hud markers for friendly players
	CG_FriendHudMarker( cent );

	// add the shadow
	shadow = CG_PlayerShadow( cent, &shadowPlane );

	// add a water splash if partially in and out of water
	CG_PlayerSplash( cent );

	if ( cg_shadows.integer == 3 && shadow ) {
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all
	if( cgs.gametype == GT_HARVESTER ) {
		CG_PlayerTokens( cent, renderfx );
	}
	//
	// add the legs
	//
	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;

	VectorCopy( cent->lerpOrigin, legs.origin );

	VectorCopy( cent->lerpOrigin, legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy (legs.origin, legs.oldorigin);	// don't positionally lerp at all

	CG_AddRefEntityWithPowerups( &legs, &cent->currentState, ci->team, qfalse, ci, 3, qfalse );

	// if the model failed, allow the default nullmodel to be displayed
	if (!legs.hModel) {
		return;
	}

	//
	// add the torso
	//
	torso.hModel = ci->torsoModel;
	if (!torso.hModel) {
		return;
	}

	torso.customSkin = ci->torsoSkin;

	VectorCopy( cent->lerpOrigin, torso.lightingOrigin );

	CG_PositionRotatedEntityOnTag( &torso, &legs, ci->legsModel, "tag_torso");

	torso.shadowPlane = shadowPlane;
	torso.renderfx = renderfx;

	CG_AddRefEntityWithPowerups( &torso, &cent->currentState, ci->team, qfalse, ci, 2, qfalse );

	if ( cent->currentState.eFlags & EF_KAMIKAZE ) {

		memset( &skull, 0, sizeof(skull) );

		VectorCopy( cent->lerpOrigin, skull.lightingOrigin );
		skull.shadowPlane = shadowPlane;
		skull.renderfx = renderfx;

		if ( cent->currentState.eFlags & EF_DEAD ) {
			// one skull bobbing above the dead body
			angle = ((cg.time / 7) & 255) * (M_PI * 2) / 255;
			if (angle > M_PI * 2)
				angle -= (float)M_PI * 2;
			dir[0] = sin(angle) * 20;
			dir[1] = cos(angle) * 20;
			angle = ((cg.time / 4) & 255) * (M_PI * 2) / 255;
			dir[2] = 15 + sin(angle) * 8;
			VectorAdd(torso.origin, dir, skull.origin);
			
			dir[2] = 0;
			VectorCopy(dir, skull.axis[1]);
			VectorNormalize(skull.axis[1]);
			VectorSet(skull.axis[2], 0, 0, 1);
			CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);

			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene( &skull );
			//skull.hModel = cgs.media.kamikazeHeadTrail;
			//trap_R_AddRefEntityToScene( &skull );
		}
		else {
			// three skulls spinning around the player
			angle = ((cg.time / 4) & 255) * (M_PI * 2) / 255;
			dir[0] = cos(angle) * 20;
			dir[1] = sin(angle) * 20;
			dir[2] = cos(angle) * 20;
			VectorAdd(torso.origin, dir, skull.origin);

			angles[0] = sin(angle) * 30;
			angles[1] = (angle * 180 / M_PI) + 90;
			if (angles[1] > 360)
				angles[1] -= 360;
			angles[2] = 0;
			AnglesToAxis( angles, skull.axis );

			/*
			dir[2] = 0;
			VectorInverse(dir);
			VectorCopy(dir, skull.axis[1]);
			VectorNormalize(skull.axis[1]);
			VectorSet(skull.axis[2], 0, 0, 1);
			CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);
			*/

			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene( &skull );
			// flip the trail because this skull is spinning in the other direction
			VectorInverse(skull.axis[1]);
			//skull.hModel = cgs.media.kamikazeHeadTrail;
			//trap_R_AddRefEntityToScene( &skull );

			angle = ((cg.time / 4) & 255) * (M_PI * 2) / 255 + M_PI;
			if (angle > M_PI * 2)
				angle -= (float)M_PI * 2;
			dir[0] = sin(angle) * 20;
			dir[1] = cos(angle) * 20;
			dir[2] = cos(angle) * 20;
			VectorAdd(torso.origin, dir, skull.origin);

			angles[0] = cos(angle - 0.5 * M_PI) * 30;
			angles[1] = 360 - (angle * 180 / M_PI);
			if (angles[1] > 360)
				angles[1] -= 360;
			angles[2] = 0;
			AnglesToAxis( angles, skull.axis );

			/*
			dir[2] = 0;
			VectorCopy(dir, skull.axis[1]);
			VectorNormalize(skull.axis[1]);
			VectorSet(skull.axis[2], 0, 0, 1);
			CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);
			*/

			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene( &skull );
			//skull.hModel = cgs.media.kamikazeHeadTrail;
			//trap_R_AddRefEntityToScene( &skull );

			angle = ((cg.time / 3) & 255) * (M_PI * 2) / 255 + 0.5 * M_PI;
			if (angle > M_PI * 2)
				angle -= (float)M_PI * 2;
			dir[0] = sin(angle) * 20;
			dir[1] = cos(angle) * 20;
			dir[2] = 0;
			VectorAdd(torso.origin, dir, skull.origin);
			
			VectorCopy(dir, skull.axis[1]);
			VectorNormalize(skull.axis[1]);
			VectorSet(skull.axis[2], 0, 0, 1);
			CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);

			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene( &skull );
			//skull.hModel = cgs.media.kamikazeHeadTrail;
			//trap_R_AddRefEntityToScene( &skull );
		}
	}

	if ( cent->currentState.powerups & ( 1 << PW_GUARD ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.guardPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		powerup.customShader = 0;
		trap_R_AddRefEntityToScene( &powerup );
	}
	if ( cent->currentState.powerups & ( 1 << PW_SCOUT ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.scoutPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		powerup.customShader = 0;
		trap_R_AddRefEntityToScene( &powerup );
	}
	if ( cent->currentState.powerups & ( 1 << PW_DOUBLER ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.doublerPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		powerup.customShader = 0;
		trap_R_AddRefEntityToScene( &powerup );
	}
	if ( cent->currentState.powerups & ( 1 << PW_AMMOREGEN ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.ammoRegenPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		powerup.customShader = 0;
		trap_R_AddRefEntityToScene( &powerup );
	}
	if ( cent->currentState.powerups & ( 1 << PW_INVULNERABILITY ) ) {
		if ( !ci->invulnerabilityStartTime ) {
			ci->invulnerabilityStartTime = cg.time;
		}
		ci->invulnerabilityStopTime = cg.time;
	}
	else {
		ci->invulnerabilityStartTime = 0;
	}
	if ( (cent->currentState.powerups & ( 1 << PW_INVULNERABILITY ) ) ||
		cg.time - ci->invulnerabilityStopTime < 250 ) {

		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.invulnerabilityPowerupModel;
		powerup.customSkin = 0;
		powerup.customShader = 0;
		// always draw
		powerup.renderfx &= ~RF_THIRD_PERSON;
		VectorCopy(cent->lerpOrigin, powerup.origin);

		if ( cg.time - ci->invulnerabilityStartTime < 250 ) {
			c = (float) (cg.time - ci->invulnerabilityStartTime) / 250;
		}
		else if (cg.time - ci->invulnerabilityStopTime < 250 ) {
			c = (float) (250 - (cg.time - ci->invulnerabilityStopTime)) / 250;
		}
		else {
			c = 1;
		}
		VectorSet( powerup.axis[0], c, 0, 0 );
		VectorSet( powerup.axis[1], 0, c, 0 );
		VectorSet( powerup.axis[2], 0, 0, c );
		trap_R_AddRefEntityToScene( &powerup );
	}

	t = cg.time - ci->medkitUsageTime;
	if ( cent->currentState.number < MAX_CLIENTS // don't show the medkit animation for dead bodies
			&& ci->medkitUsageTime && t < 500 ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.medkitUsageModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		powerup.customShader = 0;
		//// always draw
		//powerup.renderfx &= ~RF_THIRD_PERSON;
		VectorClear(angles);
		AnglesToAxis(angles, powerup.axis);
		VectorCopy(cent->lerpOrigin, powerup.origin);
		powerup.origin[2] += -24 + (float) t * 80 / 500;
		if ( t > 400 ) {
			c = (float) (t - 400) * 0xff / 100.0;
			powerup.shaderRGBA[0] = 0xff - c;
			powerup.shaderRGBA[1] = 0xff - c;
			powerup.shaderRGBA[2] = 0xff - c;
			powerup.shaderRGBA[3] = 0xff - c;
		}
		else {
			powerup.shaderRGBA[0] = 0xff;
			powerup.shaderRGBA[1] = 0xff;
			powerup.shaderRGBA[2] = 0xff;
			powerup.shaderRGBA[3] = 0xff;
		}
		trap_R_AddRefEntityToScene( &powerup );
	}

	//
	// add the head
	//
	head.hModel = ci->headModel;
	if (!head.hModel) {
		return;
	}
	head.customSkin = ci->headSkin;

	VectorCopy( cent->lerpOrigin, head.lightingOrigin );

	CG_PositionRotatedEntityOnTag( &head, &torso, ci->torsoModel, "tag_head");

	head.shadowPlane = shadowPlane;
	head.renderfx = renderfx;

	CG_AddRefEntityWithPowerups( &head, &cent->currentState, ci->team, qfalse, ci, 1, autoHeadColors );

	CG_BreathPuffs(cent, &head);

	CG_DustTrail(cent);

	//
	// add the gun / barrel / flash
	//
	CG_AddPlayerWeapon( &torso, NULL, cent, ci->team );

	// add powerups floating behind the player
	CG_PlayerPowerups( cent, &torso );
	CG_AddBoundingBox( cent );
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

	memset( &cent->pe.torso, 0, sizeof( cent->pe.legs ) );
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;

	cent->pe.crouchSlideSndCounter = rand() % CROUCHSLIDE_SOUNDS;

	if ( cg_debugPosition.integer ) {
		CG_Printf("%i ResetPlayerEntity yaw=%i\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}
}

qboolean CG_IsFrozenPlayerState( entityState_t *state ) {
	return ((cgs.ratFlags & RAT_FREEZETAG) && (state->eFlags & EF_DEAD));
}

qboolean CG_IsFrozenPlayer( centity_t *cent ) {
	return CG_IsFrozenPlayerState(&cent->currentState);
}

/*
===============
CG_GetTotalHitPoints

calculates the total points of damage that can be sustained at a specific
health / armor level
===============
*/
int CG_GetTotalHitPoints(int health, int armor) {
	int count, max;


	if (health <= 0) {
		return 0;
	}

	count = armor;
	max = health * ARMOR_PROTECTION / ( 1.0 - ARMOR_PROTECTION );
	if ( max < count ) {
		count = max;
	}
	health += count;

	return health;
}

qboolean CG_THPlayerVisible(centity_t *cent) {
	int clientNum;
	clientInfo_t *ci;
	clientInfo_t *player = &cgs.clientinfo[cg.clientNum];
	int myteam = player->team;

	if (cgs.th_phase != TH_HIDE && cgs.th_phase != TH_INTER) {
		return qtrue;
	}

	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity");
	}
	ci = &cgs.clientinfo[ clientNum ];

	if (myteam != TEAM_SPECTATOR && myteam != ci->team) {
		return qfalse;
	}

	return qtrue;
}
