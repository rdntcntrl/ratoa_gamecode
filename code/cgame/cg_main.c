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
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

#ifdef MISSIONPACK
#include "../ui/ui_shared.h"
// display context for new ui stuff
displayContextDef_t cgDC;
#endif

int forceModelModificationCount = -1;
int enemyTeamModelModificationCounts = -1;
int mySoundModificationCount = -1;
int teamSoundModificationCount = -1;
int enemySoundModificationCount = -1;
int forceColorModificationCounts = -1;
int ratStatusbarModificationCount = -1;

static void CG_RegisterNumbers(void);
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void CG_Shutdown( void );

static float CG_Cvar_Get(const char *cvar);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {

	switch ( command ) {
	case CG_INIT:
		CG_Init( arg0, arg1, arg2 );
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame( arg0, arg1, arg2 );
                CG_FairCvars();
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		CG_KeyEvent(arg0, arg1);
		return 0;
	case CG_MOUSE_EVENT:
#ifdef MISSIONPACK
		cgDC.cursorx = cgs.cursorX;
		cgDC.cursory = cgs.cursorY;
#endif
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;
	default:
		CG_Error( "vmMain: unknown command %i", command );
		break;
	}
	return -1;
}


cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];
weaponInfo_t		cg_weapons[MAX_WEAPONS];
itemInfo_t			cg_items[MAX_ITEMS];


vmCvar_t	sv_master1; // so that ioquake3 doesn't override master server
vmCvar_t	cg_railTrailTime;
vmCvar_t	cg_centertime;
vmCvar_t	cg_runpitch;
vmCvar_t	cg_runroll;
vmCvar_t	cg_bobup;
vmCvar_t	cg_bobpitch;
vmCvar_t	cg_bobroll;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
vmCvar_t	cg_gibs;
vmCvar_t	cg_drawTimer;
vmCvar_t	cg_timerPosition;
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_drawSnapshot;
vmCvar_t	cg_draw3dIcons;
vmCvar_t	cg_drawIcons;
vmCvar_t	cg_drawAmmoWarning;
vmCvar_t	cg_drawZoomScope;
vmCvar_t	cg_zoomScopeSize;
vmCvar_t	cg_zoomScopeRGColor;
vmCvar_t	cg_zoomScopeMGColor;
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_drawCrosshairNames;
vmCvar_t	cg_drawRewards;
vmCvar_t	cg_crosshairSize;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_crosshairHit;
vmCvar_t	cg_crosshairHitTime;
vmCvar_t	cg_crosshairHitStyle;
vmCvar_t	cg_crosshairHitColor;
vmCvar_t	cg_crosshairHealth;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_nopredict;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_showmiss;
vmCvar_t	cg_footsteps;
vmCvar_t	cg_addMarks;
vmCvar_t	cg_brassTime;
vmCvar_t	cg_viewsize;
vmCvar_t	cg_drawGun;
vmCvar_t	cg_gun_frame;
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
vmCvar_t	cg_tracerChance;
vmCvar_t	cg_tracerWidth;
vmCvar_t	cg_tracerLength;
vmCvar_t	cg_autoswitch;
vmCvar_t	cg_ignore;
vmCvar_t	cg_simpleItems;
vmCvar_t	cg_fov;
vmCvar_t	cg_zoomFov;
vmCvar_t	cg_zoomFovTmp;
vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_lagometer;
vmCvar_t	cg_drawAttacker;
vmCvar_t	cg_attackerScale;
vmCvar_t	cg_drawPickup;
vmCvar_t	cg_pickupScale;
vmCvar_t	cg_drawSpeed;
vmCvar_t	cg_drawSpeed3D;
vmCvar_t	cg_synchronousClients;
vmCvar_t 	cg_teamChatHeight;
vmCvar_t 	cg_teamChatScaleX;
vmCvar_t 	cg_teamChatScaleY;
vmCvar_t 	cg_stats;
vmCvar_t 	cg_buildScript;
vmCvar_t 	cg_forceModel;
vmCvar_t	cg_paused;
vmCvar_t	cg_blood;
vmCvar_t	cg_predictItems;
vmCvar_t	cg_predictItemsNearPlayers;
vmCvar_t	cg_deferPlayers;
vmCvar_t	cg_drawFollowPosition;
vmCvar_t	cg_drawTeamOverlay;
vmCvar_t	cg_teamOverlayUserinfo;
vmCvar_t	cg_drawFriend;
vmCvar_t	cg_teamChatsOnly;
vmCvar_t	cg_noVoiceChats;
vmCvar_t	cg_noVoiceText;
vmCvar_t	cg_hudFiles;
vmCvar_t 	cg_scorePlum;

vmCvar_t 	cg_ratInitialized;

vmCvar_t 	g_ratPhysics;
vmCvar_t 	g_rampJump;
vmCvar_t 	g_additiveJump;
vmCvar_t 	g_swingGrapple;
vmCvar_t 	g_fastSwim;
vmCvar_t 	g_fastSwitch;
vmCvar_t 	g_fastWeapons;
vmCvar_t 	g_regularFootsteps;
vmCvar_t 	cg_predictTeleport;
vmCvar_t 	cg_predictWeapons;
vmCvar_t 	cg_predictExplosions;
vmCvar_t 	cg_predictPlayerExplosions;
vmCvar_t 	cg_ratPredictMissiles;
vmCvar_t 	cg_delagProjectileTrail;
vmCvar_t 	cg_ratScoreboard;
vmCvar_t 	cg_ratScoreboardAccuracy;
vmCvar_t 	cg_ratStatusbar;
vmCvar_t	cg_ratPlasmaTrail;
vmCvar_t	cg_ratPlasmaTrailAlpha;
vmCvar_t	cg_ratPlasmaTrailStep;
vmCvar_t	cg_ratPlasmaTrailTime;
vmCvar_t	cg_rocketStyle;
vmCvar_t	cg_ratRocketTrail;
vmCvar_t	cg_ratRocketTrailAlpha;
vmCvar_t	cg_ratRocketTrailRadius;
vmCvar_t	cg_ratRocketTrailStep;
vmCvar_t	cg_ratRocketTrailTime;
vmCvar_t	cg_ratRail;
vmCvar_t	cg_ratRailBeefy;
vmCvar_t	cg_ratRailRadius;
vmCvar_t	cg_ratLg;
vmCvar_t	cg_ratLgImpact;
vmCvar_t	cg_consoleStyle;
vmCvar_t 	cg_noBubbleTrail;
vmCvar_t	cg_specShowZoom;
vmCvar_t	cg_zoomToggle;
vmCvar_t	cg_zoomAnim;
vmCvar_t	cg_zoomAnimScale;
vmCvar_t	cg_drawHabarBackground;
vmCvar_t	cg_hudDamageIndicator;
vmCvar_t	cg_hudDamageIndicatorScale;
vmCvar_t	cg_hudDamageIndicatorOffset;
vmCvar_t	cg_hudDamageIndicatorAlpha;
vmCvar_t	cg_emptyIndicator;
vmCvar_t	cg_reloadIndicator;
vmCvar_t	cg_reloadIndicatorY;
vmCvar_t	cg_reloadIndicatorWidth;
vmCvar_t	cg_reloadIndicatorHeight;
vmCvar_t	cg_reloadIndicatorAlpha;
vmCvar_t	cg_crosshairNamesY;
vmCvar_t	cg_crosshairNamesHealth;
vmCvar_t	cg_friendFloatHealth;
vmCvar_t	cg_friendFloatHealthSize;
vmCvar_t	cg_radar;
vmCvar_t	cg_announcer;
vmCvar_t	cg_announcerNewAwards;
vmCvar_t	cg_soundBufferDelay;
vmCvar_t	cg_powerupBlink;
vmCvar_t	cg_quadStyle;
vmCvar_t	cg_quadAlpha;
vmCvar_t	cg_quadHue;
vmCvar_t	cg_drawSpawnpoints;
vmCvar_t	cg_newFont;
vmCvar_t	cg_newConsole;
vmCvar_t	cg_chatTime;
vmCvar_t 	cg_teamChatTime;
vmCvar_t	cg_consoleTime;

vmCvar_t 	cg_teamChatY;

vmCvar_t	cg_fontScale;
vmCvar_t	cg_fontShadow;

vmCvar_t	cg_consoleSizeX;
vmCvar_t	cg_consoleSizeY;
vmCvar_t	cg_chatSizeX;
vmCvar_t	cg_chatSizeY;
vmCvar_t	cg_teamChatSizeX;
vmCvar_t	cg_teamChatSizeY;

vmCvar_t	cg_consoleLines;
vmCvar_t	cg_commonConsoleLines;
vmCvar_t	cg_chatLines;
vmCvar_t	cg_teamChatLines;

vmCvar_t	cg_commonConsole;

vmCvar_t	cg_teamOverlayScale;
vmCvar_t	cg_teamOverlayAutoColor;


vmCvar_t	cg_mySound;
vmCvar_t	cg_teamSound;
vmCvar_t	cg_enemySound;

vmCvar_t	cg_myFootsteps;
vmCvar_t	cg_teamFootsteps;
vmCvar_t	cg_enemyFootsteps;

vmCvar_t	cg_brightShells;
vmCvar_t	cg_brightShellAlpha;
vmCvar_t	cg_brightOutline;

vmCvar_t	cg_enemyModel;
vmCvar_t	cg_teamModel;

vmCvar_t	cg_teamHueBlue;
vmCvar_t	cg_teamHueDefault;
vmCvar_t	cg_teamHueRed;

vmCvar_t	cg_enemyColor;
vmCvar_t	cg_teamColor;
vmCvar_t	cg_enemyHeadColor;
vmCvar_t	cg_teamHeadColor;
vmCvar_t	cg_enemyTorsoColor;
vmCvar_t	cg_teamTorsoColor;
vmCvar_t	cg_enemyLegsColor;
vmCvar_t	cg_teamLegsColor;

vmCvar_t	cg_teamHeadColorAuto;
vmCvar_t	cg_enemyHeadColorAuto;

vmCvar_t	cg_enemyCorpseSaturation;
vmCvar_t	cg_enemyCorpseValue;
vmCvar_t	cg_teamCorpseSaturation;
vmCvar_t	cg_teamCorpseValue;

vmCvar_t	cg_itemFade;
vmCvar_t	cg_itemFadeTime;

vmCvar_t	cg_bobGun;

vmCvar_t	cg_thTokenIndicator;
vmCvar_t	cg_thTokenstyle;

vmCvar_t	cg_timerAlpha;
vmCvar_t	cg_fpsAlpha;
vmCvar_t	cg_speedAlpha;
vmCvar_t	cg_fpsScale;
vmCvar_t	cg_speedScale;
vmCvar_t	cg_timerScale;

vmCvar_t	cg_drawTeamBackground;

//unlagged - smooth clients #2
// this is done server-side now
//vmCvar_t 	cg_smoothClients;
//unlagged - smooth clients #2
vmCvar_t	pmove_fixed;
//vmCvar_t	cg_pmove_fixed;
vmCvar_t	pmove_msec;
vmCvar_t        pmove_float;
vmCvar_t	cg_pmove_msec;
vmCvar_t	cg_cameraMode;
vmCvar_t	cg_cameraOrbit;
vmCvar_t	cg_cameraOrbitDelay;
vmCvar_t	cg_timescaleFadeEnd;
vmCvar_t	cg_timescaleFadeSpeed;
vmCvar_t	cg_timescale;
vmCvar_t	cg_smallFont;
vmCvar_t	cg_bigFont;
vmCvar_t	cg_noTaunt;
vmCvar_t	cg_noProjectileTrail;
vmCvar_t	cg_oldRail;
vmCvar_t	cg_oldRocket;
vmCvar_t	cg_oldMachinegun;
vmCvar_t	cg_leiEnhancement;		// ANOTHER LEILEI LINE!!!
vmCvar_t	cg_leiBrassNoise;		// ANOTHER LEILEI LINE!!!
vmCvar_t	cg_leiGoreNoise;		// ANOTHER LEILEI LINE!!!
vmCvar_t	cg_leiSuperGoreyAwesome;		// ANOTHER LEILEI LINE!!!
vmCvar_t	cg_oldPlasma;
vmCvar_t	cg_trueLightning;
vmCvar_t        cg_music;
vmCvar_t        cg_weaponOrder;


#ifdef MISSIONPACK
vmCvar_t 	cg_redTeamName;
vmCvar_t 	cg_blueTeamName;
vmCvar_t	cg_currentSelectedPlayer;
vmCvar_t	cg_currentSelectedPlayerName;
vmCvar_t	cg_singlePlayer;
vmCvar_t	cg_singlePlayerActive;
vmCvar_t	cg_recordSPDemo;
vmCvar_t	cg_recordSPDemoName;
#endif
vmCvar_t	cg_obeliskRespawnDelay;
vmCvar_t	cg_enableDust;
vmCvar_t	cg_enableBreath;

//unlagged - client options
vmCvar_t	cg_delag;
//vmCvar_t	cg_debugDelag;
//vmCvar_t	cg_drawBBox;
vmCvar_t	cg_cmdTimeNudge;
vmCvar_t	sv_fps;
vmCvar_t	cg_projectileNudge;
vmCvar_t	cg_projectileNudgeAuto;
vmCvar_t	cg_optimizePrediction;
vmCvar_t	cl_timeNudge;
//vmCvar_t	cg_latentSnaps;
//vmCvar_t	cg_latentCmds;
//vmCvar_t	cg_plOut;
//unlagged - client options
vmCvar_t	com_maxfps;
vmCvar_t	con_notifytime;

//elimination addition
vmCvar_t	cg_alwaysWeaponBar;
vmCvar_t	cg_hitsound;
vmCvar_t        cg_voip_teamonly;
vmCvar_t        cg_voteflags;
vmCvar_t        cg_cyclegrapple;
vmCvar_t        cg_vote_custom_commands;

vmCvar_t                cg_autovertex;

vmCvar_t                cg_backupPicmip;
vmCvar_t                cg_backupDrawflat;
vmCvar_t                cg_backupLightmap;

vmCvar_t	cg_fragmsgsize;

vmCvar_t	cg_crosshairPulse;
vmCvar_t	cg_differentCrosshairs;
vmCvar_t	cg_ch1;
vmCvar_t	cg_ch1size;
vmCvar_t	cg_ch2;
vmCvar_t	cg_ch2size;
vmCvar_t	cg_ch3;
vmCvar_t	cg_ch3size;
vmCvar_t	cg_ch4;
vmCvar_t	cg_ch4size;
vmCvar_t	cg_ch5;
vmCvar_t	cg_ch5size;
vmCvar_t	cg_ch6;
vmCvar_t	cg_ch6size;
vmCvar_t	cg_ch7;
vmCvar_t	cg_ch7size;
vmCvar_t	cg_ch8;
vmCvar_t	cg_ch8size;
vmCvar_t	cg_ch9;
vmCvar_t	cg_ch9size;
vmCvar_t	cg_ch10;
vmCvar_t	cg_ch10size;
vmCvar_t	cg_ch11;
vmCvar_t	cg_ch11size;
vmCvar_t	cg_ch12;
vmCvar_t	cg_ch12size;
vmCvar_t	cg_ch13;
vmCvar_t	cg_ch13size;

vmCvar_t	cg_crosshairColorRed;
vmCvar_t	cg_crosshairColorGreen;
vmCvar_t	cg_crosshairColorBlue;

vmCvar_t	cg_weaponBarStyle;
vmCvar_t	cg_chatBeep;
vmCvar_t	cg_teamChatBeep;

vmCvar_t	cg_ui_clientCommand;

#define MASTER_SERVER_NAME "dpmaster.deathmask.net"

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] = { // bk001129
	{ &sv_master1, "sv_master1", MASTER_SERVER_NAME, CVAR_ARCHIVE },
	{ &cg_ignore, "cg_ignore", "0", 0 },	// used for debugging
	{ &cg_autoswitch, "cg_autoswitch", "0", CVAR_ARCHIVE },
	{ &cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE },
	{ &cg_zoomFov, "cg_zoomfov", "30", CVAR_ARCHIVE },
	{ &cg_zoomFovTmp, "cg_zoomfovTmp", "0", 0 },
	{ &cg_fov, "cg_fov", "100", CVAR_ARCHIVE },
	{ &cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE },
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE  },
	{ &cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE  },
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE  },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE  },
	{ &cg_drawTimer, "cg_drawTimer", "1", CVAR_ARCHIVE  },
	{ &cg_timerPosition, "cg_timerPosition", "0", CVAR_ARCHIVE  },
	{ &cg_drawFPS, "cg_drawFPS", "3", CVAR_ARCHIVE  },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE  },
	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE  },
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE  },
	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE  },
	{ &cg_attackerScale, "cg_attackerScale", "0.75", CVAR_ARCHIVE  },
	{ &cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE  },
	{ &cg_drawPickup, "cg_drawPickup", "1", CVAR_ARCHIVE  },
	{ &cg_drawSpeed, "cg_drawSpeed", "1", CVAR_ARCHIVE  },
	{ &cg_drawSpeed3D, "cg_drawSpeed3D", "0", 0  },
	{ &cg_drawZoomScope, "cg_drawZoomScope", "0", CVAR_ARCHIVE | CVAR_LATCH },
	{ &cg_zoomScopeSize, "cg_zoomScopeSize", "1.0", CVAR_ARCHIVE },
	{ &cg_zoomScopeRGColor, "cg_zoomScopeRGColor", "H120 1.0 0.5", CVAR_ARCHIVE },
	{ &cg_zoomScopeMGColor, "cg_zoomScopeMGColor", "H60 1.0 0.5", CVAR_ARCHIVE },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "3", CVAR_ARCHIVE },
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE|CVAR_LATCH },
	{ &cg_crosshairSize, "cg_crosshairSize", "30", CVAR_ARCHIVE },
	{ &cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE },
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE },
	{ &cg_crosshairHit, "cg_crosshairHit", "0", CVAR_ARCHIVE },
	{ &cg_crosshairHitColor, "cg_crosshairHitColor", "H0 1.0 1.0", CVAR_ARCHIVE },
	{ &cg_crosshairHitTime, "cg_crosshairHitTime", "250", CVAR_CHEAT },
	{ &cg_crosshairHitStyle, "cg_crosshairHitStyle", "2", CVAR_ARCHIVE },
	{ &cg_brassTime, "cg_brassTime", "0", CVAR_ARCHIVE },
	{ &cg_simpleItems, "cg_simpleItems", "1", CVAR_ARCHIVE },
	{ &cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE },
	{ &cg_lagometer, "cg_lagometer", "1", CVAR_ARCHIVE },
	{ &cg_railTrailTime, "cg_railTrailTime", "800", CVAR_ARCHIVE  },
	{ &cg_gun_x, "cg_gunX", "0", CVAR_CHEAT },
	{ &cg_gun_y, "cg_gunY", "0", CVAR_CHEAT },
	{ &cg_gun_z, "cg_gunZ", "0", CVAR_CHEAT },
	{ &cg_centertime, "cg_centertime", "3", CVAR_CHEAT },
	{ &cg_runpitch, "cg_runpitch", "0.000", CVAR_ARCHIVE},
	{ &cg_runroll, "cg_runroll", "0.000", CVAR_ARCHIVE },
	//{ &cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
	//{ &cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE },
	{ &cg_bobup , "cg_bobup", "0.005", CVAR_CHEAT },
	{ &cg_bobpitch, "cg_bobpitch", "0.0", CVAR_ARCHIVE },
	{ &cg_bobroll, "cg_bobroll", "0.0", CVAR_ARCHIVE },
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT },
	{ &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT },
	{ &cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT },
	{ &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT },
	{ &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT },
	{ &cg_errorDecay, "cg_errordecay", "100", 0 },
	{ &cg_nopredict, "cg_nopredict", "0", 0 },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT },
	{ &cg_showmiss, "cg_showmiss", "0", 0 },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT },
	{ &cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT },
	{ &cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT },
	{ &cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT },
	{ &cg_thirdPerson, "cg_thirdPerson", "0", 0 },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "8", CVAR_ARCHIVE  },
	{ &cg_teamChatScaleX, "cg_teamChatScaleX", "0.7", CVAR_ARCHIVE  },
	{ &cg_teamChatScaleY, "cg_teamChatScaleY", "1", CVAR_ARCHIVE  },
	{ &cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE  },
	{ &cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE },
	// TODO: CVAR_ARCHIVE
	{ &cg_predictItemsNearPlayers, "cg_predictItemsNearPlayers", "0", 0 },
#ifdef MISSIONPACK
	{ &cg_deferPlayers, "cg_deferPlayers", "0", CVAR_ARCHIVE },
#else
	{ &cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE },
#endif
	{ &cg_drawFollowPosition, "cg_drawFollowPosition", "1", CVAR_ARCHIVE },

	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "4", CVAR_ARCHIVE },
	{ &cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO },
	{ &cg_stats, "cg_stats", "0", 0 },
	{ &cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE },
	{ &cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE },
	{ &cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE },
	{ &cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE },
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{ &cg_buildScript, "com_buildScript", "0", 0 },	// force loading of all possible data amd error on failures
	{ &cg_paused, "cl_paused", "0", CVAR_ROM },
	{ &cg_blood, "com_blood", "1", CVAR_ARCHIVE },
	{ &cg_alwaysWeaponBar, "cg_alwaysWeaponBar", "1", CVAR_ARCHIVE},	//Elimination
        { &cg_hitsound, "cg_hitsound", "1", CVAR_ARCHIVE},
        { &cg_voip_teamonly, "cg_voipTeamOnly", "1", CVAR_ARCHIVE},
        { &cg_voteflags, "cg_voteflags", "*", CVAR_ROM},
        { &cg_cyclegrapple, "cg_cyclegrapple", "1", CVAR_ARCHIVE},
        { &cg_vote_custom_commands, "cg_vote_custom_commands", "", CVAR_ROM },
	{ &cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO },	// communicated by systeminfo

        { &cg_autovertex, "cg_autovertex", "0", CVAR_ARCHIVE },

        { &cg_backupPicmip, "cg_backupPicmip", "-1", CVAR_ARCHIVE },
        { &cg_backupDrawflat, "cg_backupDrawflat", "-1", CVAR_ARCHIVE },
        { &cg_backupLightmap, "cg_backupLightmap", "-1", CVAR_ARCHIVE },
#ifdef MISSIONPACK
	{ &cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO },
	{ &cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO },
	{ &cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE},
	{ &cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE},
	{ &cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{ &cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO},
	{ &cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE},
	{ &cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE},
	{ &cg_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_ARCHIVE},
#endif
	{ &cg_enableDust, "g_enableDust", "0", CVAR_SERVERINFO},
	{ &cg_enableBreath, "g_enableBreath", "0", CVAR_SERVERINFO},
	{ &cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SERVERINFO},

	{ &cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT},
	{ &cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{ &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{ &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{ &cg_timescale, "timescale", "1", 0},
	{ &cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE},

	// RAT ===================
	{ &cg_ratInitialized, "cg_ratInitialized", "0", CVAR_ARCHIVE},

	{ &g_ratPhysics, "g_ratPhysics", "0", CVAR_SYSTEMINFO},
	{ &g_rampJump, "g_rampJump", "0", CVAR_SYSTEMINFO},
	{ &g_additiveJump, "g_additiveJump", "0", CVAR_SYSTEMINFO},
	{ &g_swingGrapple, "g_swingGrapple", "0", CVAR_SYSTEMINFO},
	{ &g_fastSwim, "g_fastSwim", "1", CVAR_SYSTEMINFO},
	{ &g_fastSwitch, "g_fastSwitch", "1", CVAR_SYSTEMINFO},
	{ &g_fastWeapons, "g_fastWeapons", "1", CVAR_SYSTEMINFO},
	{ &g_regularFootsteps, "g_regularFootsteps", "1", CVAR_SYSTEMINFO},

	// TODO: make CVAR_ARCHIVE
	{ &cg_predictTeleport, "cg_predictTeleport", "1", 0},
	{ &cg_predictWeapons, "cg_predictWeapons", "1", 0},
	{ &cg_predictExplosions, "cg_predictExplosions", "1", 0},
	{ &cg_predictPlayerExplosions, "cg_predictPlayerExplosions", "0", 0},

	{ &cg_ratPredictMissiles, "cg_ratPredictMissiles", "1", CVAR_ARCHIVE},
	{ &cg_delagProjectileTrail, "cg_delagProjectileTrail", "1", 0},
	{ &cg_ratScoreboard, "cg_ratScoreboard", "1", CVAR_ARCHIVE},
	{ &cg_ratScoreboardAccuracy, "cg_ratScoreboardAccuracy", "1", 0},
	{ &cg_ratStatusbar, "cg_ratStatusbar", "4", CVAR_ARCHIVE},

	{ &cg_ratPlasmaTrail, "cg_ratPlasmaTrail", "0", CVAR_ARCHIVE},
	{ &cg_ratPlasmaTrailAlpha, "cg_ratPlasmaTrailAlpha", "0.1", CVAR_ARCHIVE},
	{ &cg_ratPlasmaTrailStep, "cg_ratPlasmaTrailStep", "12", CVAR_ARCHIVE},
	{ &cg_ratPlasmaTrailTime, "cg_ratPlasmaTrailTime", "500", CVAR_ARCHIVE},
	//
	{ &cg_rocketStyle, "cg_rocketStyle", "1", CVAR_ARCHIVE | CVAR_LATCH },
	{ &cg_ratRocketTrail, "cg_ratRocketTrail", "1", CVAR_ARCHIVE},
	{ &cg_ratRocketTrailAlpha, "cg_ratRocketTrailAlpha", "0.6", CVAR_ARCHIVE},
	{ &cg_ratRocketTrailRadius, "cg_ratRocketTrailRadius", "6", CVAR_ARCHIVE},
	{ &cg_ratRocketTrailStep, "cg_ratRocketTrailStep", "20", CVAR_ARCHIVE},
	{ &cg_ratRocketTrailTime, "cg_ratRocketTrailTime", "0.5", CVAR_ARCHIVE},
	{ &cg_ratRail, "cg_ratRail", "3", CVAR_ARCHIVE | CVAR_LATCH},
	{ &cg_ratRailBeefy, "cg_ratRailBeefy", "0", CVAR_ARCHIVE},
	{ &cg_ratRailRadius, "cg_ratRailRadius", "0.5", CVAR_ARCHIVE},
	{ &cg_ratLg, "cg_ratLg", "3", CVAR_ARCHIVE|CVAR_LATCH},
	{ &cg_ratLgImpact, "cg_ratLgImpact", "1", CVAR_ARCHIVE},
	{ &cg_consoleStyle, "cg_consoleStyle", "2", CVAR_ARCHIVE},
	{ &cg_noBubbleTrail, "cg_noBubbleTrail", "1", CVAR_ARCHIVE},
	{ &cg_specShowZoom, "cg_specShowZoom", "1", CVAR_ARCHIVE},
	{ &cg_zoomToggle, "cg_zoomToggle", "0", CVAR_ARCHIVE},
	{ &cg_zoomAnim, "cg_zoomAnim", "1", CVAR_ARCHIVE},
	{ &cg_zoomAnimScale, "cg_zoomAnimScale", "2", CVAR_ARCHIVE},
	{ &cg_drawHabarBackground, "cg_drawHabarBackground", "0", CVAR_ARCHIVE},
	{ &cg_hudDamageIndicator, "cg_hudDamageIndicator", "2", CVAR_ARCHIVE|CVAR_LATCH},
	{ &cg_hudDamageIndicatorScale, "cg_hudDamageIndicatorScale", "1.0", CVAR_ARCHIVE},
	{ &cg_hudDamageIndicatorOffset, "cg_hudDamageIndicatorOffset", "0.0", CVAR_ARCHIVE},
	{ &cg_hudDamageIndicatorAlpha, "cg_hudDamageIndicatorAlpha", "1.0", CVAR_ARCHIVE},
	{ &cg_emptyIndicator, "cg_emptyIndicator", "1", CVAR_ARCHIVE},
	{ &cg_reloadIndicator, "cg_reloadIndicator", "0", CVAR_ARCHIVE},
	{ &cg_reloadIndicatorY, "cg_reloadIndicatorY", "340", CVAR_ARCHIVE},
	{ &cg_reloadIndicatorWidth, "cg_reloadIndicatorWidth", "40", CVAR_ARCHIVE},
	{ &cg_reloadIndicatorHeight, "cg_reloadIndicatorHeight", "2", CVAR_ARCHIVE},
	{ &cg_reloadIndicatorAlpha, "cg_reloadIndicatorAlpha", "0.2", CVAR_ARCHIVE},
	{ &cg_crosshairNamesY, "cg_crosshairNamesY", "280", CVAR_ARCHIVE},
	{ &cg_crosshairNamesHealth, "cg_crosshairNamesHealth", "1", CVAR_ARCHIVE},
	{ &cg_friendFloatHealth, "cg_friendFloatHealth", "1", CVAR_ARCHIVE},
	{ &cg_friendFloatHealthSize, "cg_friendFloatHealthSize", "8", CVAR_ARCHIVE},
	{ &cg_radar, "cg_radar", "1", CVAR_ARCHIVE},
	{ &cg_announcer, "cg_announcer", "treb", CVAR_ARCHIVE|CVAR_LATCH},
	{ &cg_announcerNewAwards, "cg_announcerNewAwards", "", CVAR_ARCHIVE|CVAR_LATCH},
	{ &cg_soundBufferDelay, "cg_soundBufferDelay", "750", 0},
	{ &cg_powerupBlink, "cg_powerupBlink", "0", CVAR_ARCHIVE},
	{ &cg_quadStyle, "cg_quadStyle", "0", CVAR_ARCHIVE},
	{ &cg_quadAlpha, "cg_quadAlpha", "1.0", CVAR_ARCHIVE},
	{ &cg_quadHue, "cg_quadHue", "250", CVAR_ARCHIVE},
	{ &cg_drawSpawnpoints, "cg_drawSpawnpoints", "1", CVAR_ARCHIVE},
	{ &cg_teamOverlayScale, "cg_teamOverlayScale", "0.7", CVAR_ARCHIVE},
	{ &cg_teamOverlayAutoColor, "cg_teamOverlayAutoColor", "1", CVAR_ARCHIVE},
	{ &cg_drawTeamBackground, "cg_drawTeamBackground", "0", CVAR_ARCHIVE},
	{ &cg_timerAlpha  ,     "cg_timerAlpha", "1", CVAR_ARCHIVE},
	{ &cg_fpsAlpha    ,     "cg_fpsAlpha", "0.5", CVAR_ARCHIVE},
	{ &cg_speedAlpha  ,     "cg_speedAlpha", "0.5", CVAR_ARCHIVE},
	{ &cg_timerScale ,     "cg_timerScale", "1", CVAR_ARCHIVE},
	{ &cg_fpsScale   ,     "cg_fpsScale", "0.6", CVAR_ARCHIVE},
	{ &cg_speedScale ,     "cg_speedScale", "0.6", CVAR_ARCHIVE},
	{ &cg_pickupScale ,     "cg_pickupScale", "0.75", CVAR_ARCHIVE},

	{ &cg_chatTime ,    "cg_chatTime", "10000", CVAR_ARCHIVE},
	{ &cg_consoleTime , "cg_consoleTime", "10000", CVAR_ARCHIVE},
	{ &cg_teamChatTime, "cg_teamChatTime", "10000", CVAR_ARCHIVE  },

	{ &cg_teamChatY, "cg_teamChatY", "350", CVAR_ARCHIVE  },

	{ &cg_newFont ,     "cg_newFont", "1", CVAR_ARCHIVE},

	{ &cg_newConsole ,  "cg_newConsole", "1", CVAR_ARCHIVE},

	{ &cg_consoleSizeX , "cg_consoleSizeX", "4.5", 0},
	{ &cg_consoleSizeY , "cg_consoleSizeY", "9", 0},
	{ &cg_chatSizeX , "cg_chatSizeX", "5", 0},
	{ &cg_chatSizeY , "cg_chatSizeY", "10", 0},
	{ &cg_teamChatSizeX , "cg_teamChatSizeX", "5", 0},
	{ &cg_teamChatSizeY , "cg_teamChatSizeY", "10", 0},

	{ &cg_commonConsole , "cg_commonConsole", "0", CVAR_ARCHIVE},

	{ &cg_consoleLines , "cg_consoleLines", "3", 0},
	{ &cg_commonConsoleLines , "cg_commonConsoleLines", "6", 0},
	{ &cg_chatLines , "cg_chatLines", "6", 0},
	{ &cg_teamChatLines , "cg_teamChatLines", "6", 0},

	{ &cg_fontScale , "cg_fontScale", "1.0", CVAR_ARCHIVE},
	{ &cg_fontShadow , "cg_fontShadow", "1", CVAR_ARCHIVE},

	{ &cg_mySound ,     "cg_mySound", "", CVAR_ARCHIVE},
	{ &cg_teamSound ,   "cg_teamSound", "", CVAR_ARCHIVE},
	{ &cg_enemySound ,  "cg_enemySound", "penguin", CVAR_ARCHIVE},

	{ &cg_myFootsteps ,     "cg_myFootsteps", "-1", CVAR_ARCHIVE},
	{ &cg_teamFootsteps ,   "cg_teamFootsteps", "-1", CVAR_ARCHIVE},
	{ &cg_enemyFootsteps ,  "cg_enemyFootsteps", "-1", CVAR_ARCHIVE},

	{ &cg_brightShells ,     "cg_brightShells", "1", CVAR_ARCHIVE},
	{ &cg_brightShellAlpha , "cg_brightShellAlpha", "1.0", CVAR_ARCHIVE},
	{ &cg_brightOutline ,     "cg_brightOutline", "1", CVAR_ARCHIVE},

	{ &cg_enemyModel ,     "cg_enemyModel", "smarine/gray", CVAR_ARCHIVE},
	{ &cg_teamModel ,      "cg_teamModel", "sarge/gray", CVAR_ARCHIVE},

	{ &cg_teamHueBlue ,     "cg_teamHueBlue", "210", CVAR_ARCHIVE},
	{ &cg_teamHueDefault ,  "cg_teamHueDefault", "125", CVAR_ARCHIVE},
	{ &cg_teamHueRed ,      "cg_teamHueRed", "0", CVAR_ARCHIVE},

	// either color name ("green", "white"), color index, or 
	// HSV color in the format 'H125 1.0 1.0" (H<H> <S> <V>)
	{ &cg_enemyColor ,     "cg_enemyColor", "", CVAR_ARCHIVE},
	{ &cg_teamColor ,      "cg_teamColor", "", CVAR_ARCHIVE},
	{ &cg_enemyHeadColor ,     "cg_enemyHeadColor", "", CVAR_ARCHIVE},
	{ &cg_teamHeadColor ,      "cg_teamHeadColor", "", CVAR_ARCHIVE},
	{ &cg_enemyTorsoColor ,     "cg_enemyTorsoColor", "", CVAR_ARCHIVE},
	{ &cg_teamTorsoColor ,      "cg_teamTorsoColor", "", CVAR_ARCHIVE},
	{ &cg_enemyLegsColor ,     "cg_enemyLegsColor", "", CVAR_ARCHIVE},
	{ &cg_teamLegsColor ,      "cg_teamLegsColor", "", CVAR_ARCHIVE},

	{ &cg_teamHeadColorAuto ,      "cg_teamHeadColorAuto", "0", CVAR_ARCHIVE},
	{ &cg_enemyHeadColorAuto ,      "cg_enemyHeadColorAuto", "1", CVAR_ARCHIVE},

	{ &cg_enemyCorpseSaturation ,     "cg_enemyCorpseSaturation", "", CVAR_ARCHIVE},
	{ &cg_enemyCorpseValue ,          "cg_enemyCorpseValue", "0.2", CVAR_ARCHIVE},
	{ &cg_teamCorpseSaturation ,      "cg_teamCorpseSaturation", "", CVAR_ARCHIVE},
	{ &cg_teamCorpseValue ,           "cg_teamCorpseValue", "0.2", CVAR_ARCHIVE},

	{ &cg_itemFade ,           "cg_itemFade", "1", CVAR_ARCHIVE},
	{ &cg_itemFadeTime ,           "cg_itemFadeTime", "3000", CVAR_CHEAT},

	{ &cg_bobGun ,           "cg_bobGun", "0", CVAR_ARCHIVE},

	// TREASURE HUNTER:
	{ &cg_thTokenIndicator ,           "cg_thTokenIndicator", "1", CVAR_ARCHIVE},
	{ &cg_thTokenstyle ,           	   "cg_thTokenstyle", "-999", CVAR_ROM},

	// / RAT ===================

//unlagged - smooth clients #2
// this is done server-side now
//	{ &cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE},
//unlagged - smooth clients #2
	{ &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO},
	{ &pmove_msec, "pmove_msec", "11", CVAR_SYSTEMINFO},
        { &pmove_float, "pmove_float", "1", CVAR_SYSTEMINFO},
	{ &cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE},
	{ &cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE},
	{ &cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{ &cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},
	{ &cg_oldRail, "cg_oldRail", "0", CVAR_ARCHIVE},
	{ &cg_oldRocket, "cg_oldRocket", "1", CVAR_ARCHIVE},
	{ &cg_oldMachinegun, "cg_oldMachinegun", "0", CVAR_ARCHIVE|CVAR_LATCH},
	{ &cg_leiEnhancement, "cg_leiEnhancement", "0", CVAR_ARCHIVE},				// LEILEI default off (in case of whiner)
	{ &cg_leiGoreNoise, "cg_leiGoreNoise", "0", CVAR_ARCHIVE},					// LEILEI 
	{ &cg_leiBrassNoise, "cg_leiBrassNoise", "0", CVAR_ARCHIVE},				// LEILEI 
	{ &cg_leiSuperGoreyAwesome, "cg_leiSuperGoreyAwesome", "0", CVAR_ARCHIVE},	// LEILEI 
	{ &cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE},
//unlagged - client options
	{ &cg_delag, "cg_delag", "1", CVAR_ARCHIVE | CVAR_USERINFO },
//	{ &cg_debugDelag, "cg_debugDelag", "0", CVAR_USERINFO | CVAR_CHEAT },
//	{ &cg_drawBBox, "cg_drawBBox", "0", CVAR_CHEAT },
	{ &cg_cmdTimeNudge, "cg_cmdTimeNudge", "0", CVAR_ARCHIVE | CVAR_USERINFO },
	// this will be automagically copied from the server
	{ &sv_fps, "sv_fps", "20", CVAR_SYSTEMINFO | CVAR_ROM },
	{ &cg_projectileNudge, "cg_projectileNudge", "0", CVAR_ARCHIVE },
	{ &cg_projectileNudgeAuto, "cg_projectileNudgeAuto", "0", CVAR_ARCHIVE },
	{ &cg_optimizePrediction, "cg_optimizePrediction", "1", CVAR_ARCHIVE },
	{ &cl_timeNudge, "cl_timeNudge", "0", CVAR_ARCHIVE },
	{ &com_maxfps, "com_maxfps", "125", CVAR_ARCHIVE },
	{ &con_notifytime, "con_notifytime", "3", CVAR_ARCHIVE },
//	{ &cg_latentSnaps, "cg_latentSnaps", "0", CVAR_USERINFO | CVAR_CHEAT },
//	{ &cg_latentCmds, "cg_latentCmds", "0", CVAR_USERINFO | CVAR_CHEAT },
//	{ &cg_plOut, "cg_plOut", "0", CVAR_USERINFO | CVAR_CHEAT },
//unlagged - client options
	{ &cg_trueLightning, "cg_trueLightning", "1.0", CVAR_ARCHIVE},
        { &cg_music, "cg_music", "", CVAR_ARCHIVE},
//	{ &cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE }

	{ &cg_fragmsgsize, "cg_fragmsgsize", "0.6", CVAR_ARCHIVE},
	{ &cg_crosshairPulse, "cg_crosshairPulse", "0", CVAR_ARCHIVE},
	
	{ &cg_differentCrosshairs, "cg_differentCrosshairs", "0", CVAR_ARCHIVE},
	{ &cg_ch1, "cg_ch1", "14", CVAR_ARCHIVE},
	{ &cg_ch1size, "cg_ch1size", "30", CVAR_ARCHIVE},
	{ &cg_ch2, "cg_ch2", "2", CVAR_ARCHIVE},
	{ &cg_ch2size, "cg_ch2size", "30", CVAR_ARCHIVE},
	{ &cg_ch3, "cg_ch3", "8", CVAR_ARCHIVE},
	{ &cg_ch3size, "cg_ch3size", "30", CVAR_ARCHIVE},
	{ &cg_ch4, "cg_ch4", "22", CVAR_ARCHIVE},
	{ &cg_ch4size, "cg_ch4size", "30", CVAR_ARCHIVE},
	{ &cg_ch5, "cg_ch5", "37", CVAR_ARCHIVE},
	{ &cg_ch5size, "cg_ch5size", "30", CVAR_ARCHIVE},
	{ &cg_ch6, "cg_ch6", "7", CVAR_ARCHIVE},
	{ &cg_ch6size, "cg_ch6size", "30", CVAR_ARCHIVE},
	{ &cg_ch7, "cg_ch7", "5", CVAR_ARCHIVE},
	{ &cg_ch7size, "cg_ch7size", "30", CVAR_ARCHIVE},
	{ &cg_ch8, "cg_ch8", "38", CVAR_ARCHIVE},
	{ &cg_ch8size, "cg_ch8size", "30", CVAR_ARCHIVE},
	{ &cg_ch9, "cg_ch9", "24", CVAR_ARCHIVE},
	{ &cg_ch9size, "cg_ch9size", "30", CVAR_ARCHIVE},
	{ &cg_ch10, "cg_ch10", "1", CVAR_ARCHIVE},
	{ &cg_ch10size, "cg_ch10size", "30", CVAR_ARCHIVE},
	{ &cg_ch11, "cg_ch11", "21", CVAR_ARCHIVE},
	{ &cg_ch11size, "cg_ch11size", "30", CVAR_ARCHIVE},
	{ &cg_ch12, "cg_ch12", "23", CVAR_ARCHIVE},
	{ &cg_ch12size, "cg_ch12size", "30", CVAR_ARCHIVE},
	{ &cg_ch13, "cg_ch13", "10", CVAR_ARCHIVE},
	{ &cg_ch13size, "cg_ch13size", "30", CVAR_ARCHIVE},

	{ &cg_crosshairColorRed, "cg_crosshairColorRed", "1.0", CVAR_ARCHIVE},
        { &cg_crosshairColorGreen, "cg_crosshairColorGreen", "1.0", CVAR_ARCHIVE},
        { &cg_crosshairColorBlue, "cg_crosshairColorBlue", "1.0", CVAR_ARCHIVE},

	{ &cg_weaponBarStyle, "cg_weaponBarStyle", "14", CVAR_ARCHIVE},
        //{ &cg_weaponOrder,"cg_weaponOrder", "/1/2/4/3/6/7/8/9/5/", CVAR_ARCHIVE},
        { &cg_weaponOrder,"cg_weaponOrder", "/1/2/4/3/7/6/8/5/13/11/9/", CVAR_ARCHIVE},
        {&cg_chatBeep, "cg_chatBeep", "1", CVAR_ARCHIVE },
        {&cg_teamChatBeep, "cg_teamChatBeep", "1", CVAR_ARCHIVE },

        {&cg_ui_clientCommand, "cg_ui_clientCommand", "", CVAR_ROM }
};

static int  cvarTableSize = sizeof( cvarTable ) / sizeof( cvarTable[0] );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	char		var[MAX_TOKEN_CHARS];

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	forceModelModificationCount = cg_forceModel.modificationCount;
	enemyTeamModelModificationCounts = cg_enemyModel.modificationCount + cg_teamModel.modificationCount;

	trap_Cvar_Register(NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "headmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "team_model", DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "team_headmodel", DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE );
}

void CG_RatRemapShaders(void) {
	switch (cg_consoleStyle.integer) {
		case 1:
			trap_R_RemapShader("console", "ratconsole", 0);
			break;
		case 2:
			if (((float)cgs.glconfig.vidWidth)/((float)cgs.glconfig.vidHeight) > 1.5) {
				// widescreen:
				trap_R_RemapShader("console", "ratconsole_trebrat_wide", 0);
			} else {
				trap_R_RemapShader("console", "ratconsole_trebrat", 0);
			}
			break;
		default:
			break;
	}
}


/*
 * Set good defaults for a number of important engine cvars
 */
void CG_SetEngineCvars( void ) {
 	if ((int)CG_Cvar_Get("snaps") < 40) {
		trap_Cvar_Set( "snaps", "40" );
	}
 	if ((int)CG_Cvar_Get("cl_maxpackets") < 125) {
		trap_Cvar_Set( "cl_maxpackets", "125" );
	}
 	if ((int)CG_Cvar_Get("rate") < 25000) {
		trap_Cvar_Set( "rate", "25000" );
	}
}


#define LATEST_RATINITIALIZED 29

int CG_MigrateOldCrosshair(int old) {
	switch (old) {
		case 0:
			return 0;
		case 1:
		case 2:
		case 3:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
			return 8;
		case 4:
			return 2;
		case 5:
		case 6:
			return 1;
		case 7:
		case 8:
			return 29;
		case 9:
			return 28;
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
			return old-9;
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
			return old-16;
		case 36:
		case 38:
			return 9;
		case 37:
		case 39:
			return 10;
		default:
			break;
	}

	return 1;
}

/*
 *
 * Update really old ratmod configurations
 * This might be removed in the future
 */
void CG_RatOldCfgUpdate(void) {
	if (cg_ratInitialized.integer < 1) {
		if (cg_drawCrosshair.integer < 10) {
			CG_Cvar_SetAndUpdate( "cg_drawCrosshair", "19" );
		}

		CG_Randomcolors_f();

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "1" );
	}

	if (cg_ratInitialized.integer < 2) {
		CG_Cvar_SetAndUpdate( "cg_delag", "1" );
		CG_Cvar_SetAndUpdate( "cg_drawTimer", "1" );
		CG_Cvar_SetAndUpdate( "cg_drawSpeed", "1" );
		CG_Cvar_SetAndUpdate( "cg_drawFPS", "1" );
		CG_Cvar_SetAndUpdate( "cg_drawSpawnpoints", "1" );
		CG_Cvar_SetAndUpdate( "cg_bobpitch", "0.0" );
		CG_Cvar_SetAndUpdate( "cg_bobroll", "0.0" );
		CG_Cvar_SetAndUpdate( "cg_hitsound", "1");

		//CG_Cvar_SetAndUpdate( "cg_forceBrightModels", "2");

		if (cg_drawTeamOverlay.integer <= 0) {
			CG_Cvar_SetAndUpdate( "cg_drawTeamOverlay", "4" );
		}
		if (cg_fragmsgsize.value == 1.0) {
			CG_Cvar_SetAndUpdate( "cg_fragmsgsize", "0.75" );
		}

		// non-cgame settings
		CG_Cvar_SetAndUpdate( "cl_maxpackets", "125" );

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "2" );
	}

	if (cg_ratInitialized.integer < 4) {
		CG_Cvar_SetAndUpdate("cg_alwaysWeaponBar", "1");
		switch (cg_weaponBarStyle.integer) {
			case 2:
			case 3:
			case 4:
				CG_Cvar_SetAndUpdate("cg_weaponBarStyle", "9");
				break;
			default:
				CG_Cvar_SetAndUpdate("cg_weaponBarStyle", "8");
				break;
		}

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "4" );
	}
	
	if (cg_ratInitialized.integer < 5) {
		CG_Cvar_SetAndUpdate("cg_runpitch", "0.0");
		CG_Cvar_SetAndUpdate("cg_runroll", "0.0");

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "5" );
	}

	if (cg_ratInitialized.integer < 6) {
		CG_Cvar_SetAndUpdate("snaps", "40");

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "6" );
	}

	if (cg_ratInitialized.integer < 7) {
		int h;
		float s,v;

		// Update corpse color cvars
		CG_Cvar_SetAndUpdate( "cg_teamCorpseSaturation", "" );
		CG_Cvar_SetAndUpdate( "cg_enemyCorpseSaturation", "" );

		CG_Cvar_SetAndUpdate( "cg_teamCorpseValue", "0.2" );
		CG_Cvar_SetAndUpdate( "cg_enemyCorpseValue", "0.2" );

		// Update default team colors
		h = CG_Cvar_Get("cg_modelHueBlue");
		if (h == 185) {
			CG_Cvar_SetAndUpdate( "cg_teamHueBlue", va("%i", h));
		} else {
			CG_Cvar_SetAndUpdate( "cg_teamHueBlue", "210" );
		}
		h = CG_Cvar_Get("cg_modelHueRed");
		if (h == 10) {
			CG_Cvar_SetAndUpdate( "cg_teamHueRed", va("%i", h));
		} else {
			CG_Cvar_SetAndUpdate( "cg_teamHueRed", "0" );
		}
		CG_Cvar_SetAndUpdate( "cg_teamHueDefault", "125" );

		// update forced colors
		if (CG_Cvar_Get("cg_forceEnemyModelColor")) {
			h = CG_Cvar_Get("cg_forceEnemyModelHue");
			s = CG_Cvar_Get("cg_forceEnemyModelSaturation");
			v = CG_Cvar_Get("cg_forceEnemyModelValue");

			CG_Cvar_SetAndUpdate( "cg_enemyColor", va("H%i %.2f %.2f", h, s, v) );
		} else {
			CG_Cvar_SetAndUpdate( "cg_enemyColor", "" );
		}

		if (CG_Cvar_Get("cg_forceModelColor")) {
			h = CG_Cvar_Get("cg_forceModelHue");
			s = CG_Cvar_Get("cg_forceModelSaturation");
			v = CG_Cvar_Get("cg_forceModelValue");

			CG_Cvar_SetAndUpdate( "cg_teamColor", va("H%i %.2f %.2f", h, s, v) );
		} else {
			CG_Cvar_SetAndUpdate( "cg_teamColor", "" );
		}

		switch ((int)CG_Cvar_Get("cg_autoHeadColors")) {
			case 2:
				CG_Cvar_SetAndUpdate( "cg_teamHeadColorAuto", "0");
				CG_Cvar_SetAndUpdate( "cg_enemyHeadColorAuto", "1");
				break;
			case 3:
				CG_Cvar_SetAndUpdate( "cg_teamHeadColorAuto", "1");
				CG_Cvar_SetAndUpdate( "cg_enemyHeadColorAuto", "0");
				break;
		}
		//trap_SendConsoleCommand("unset cg_autoHeadColors");

		switch ((int)CG_Cvar_Get("cg_forceBrightModels")) {
			case 1:
				CG_Cvar_SetAndUpdate( "cg_teamModel", "sarge/bright");
				CG_Cvar_SetAndUpdate( "cg_enemyModel","sarge/bright");
				break;
			case 3:
				CG_Cvar_SetAndUpdate( "cg_teamModel", "smarine/bright");
				CG_Cvar_SetAndUpdate( "cg_enemyModel","smarine/bright");
				break;
			case 2:
			default:
				CG_Cvar_SetAndUpdate( "cg_enemyModel","smarine/bright");
				CG_Cvar_SetAndUpdate( "cg_teamModel", "sarge/bright");
				break;
		}
		

		// unset old variables
		trap_SendConsoleCommand("unset cg_forceEnemyCorpseColor;"
			       		"unset cg_forceEnemyCorpseSaturation;"
				        "unset cg_forceEnemyCorpseValue;"
				        "unset cg_forceCorpseColor;"
				        "unset cg_forceCorpseSaturation;"
				        "unset cg_forceCorpseValue;"
				        "unset cg_modelHueBlue;"
				        "unset cg_modelHueDefault;"
				        "unset cg_modelHueRed;"
				        "unset cg_forceModelColor;"
				        "unset cg_forceModelHue;"
				        "unset cg_forceModelSaturation;"
				        "unset cg_forceModelValue;"
				        "unset cg_forceEnemyModelColor;"
				        "unset cg_forceEnemyModelHue;"
				        "unset cg_forceEnemyModelSaturation;"
				        "unset cg_forceEnemyModelValue;"
				        "unset cg_autoHeadColors;"
				        "unset cg_forceBrightModels\n"
					);

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "7" );
	}

	if (cg_ratInitialized.integer < 8) {
		trap_SendConsoleCommand("unset cg_corpseSaturation\n");

		CG_Cvar_SetAndUpdate("cg_weaponOrder", "/1/2/4/3/7/6/8/5/13/11/9/");

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "8" );
	}

	if (cg_ratInitialized.integer < 9) {
		if ((int)CG_Cvar_Get("cg_ratStatusbar") == 0) {
			CG_Cvar_SetAndUpdate("cg_ratStatusbar", "1");
		}

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "9" );
	}

	if (cg_ratInitialized.integer < 10) {
		CG_Cvar_SetAndUpdate("cg_projectileNudgeAuto", "0");
		CG_Cvar_SetAndUpdate("cg_predictPlayerExplosions", "0");

		CG_Cvar_SetAndUpdate("cg_ratRocketTrailRadius", "6");
		CG_Cvar_SetAndUpdate("cg_ratRocketTrailAlpha", "0.6");

		trap_SendConsoleCommand("unset cg_ratRocketTrailRadius2; "
			       		"unset cg_ratRocketTrailAlpha2\n");

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "10" );
	}

	if (cg_ratInitialized.integer < 11) {
		if (cg_weaponBarStyle.integer == 8) {
			CG_Cvar_SetAndUpdate("cg_weaponBarStyle", "11");
		} else if (cg_weaponBarStyle.integer == 9) {
			CG_Cvar_SetAndUpdate("cg_weaponBarStyle", "12");
		}

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "11" );
	}

	if (cg_ratInitialized.integer < 12) {
		CG_Cvar_SetAndUpdate("cg_itemFade", "1");

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "12" );
	}

	if (cg_ratInitialized.integer < 13) {
		CG_Cvar_SetAndUpdate("cg_consoleStyle", "2");

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "13" );
	}

	if (cg_ratInitialized.integer < 14) {
		if (cg_weaponBarStyle.integer == 8) {
			CG_Cvar_SetAndUpdate("cg_weaponBarStyle", "11");
		} else if (cg_weaponBarStyle.integer == 9) {
			CG_Cvar_SetAndUpdate("cg_weaponBarStyle", "12");
		}
		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "14" );
	}

	if (cg_ratInitialized.integer < 15) {
		CG_Cvar_SetAndUpdate( "cg_announcer", "treb" );
		CG_Cvar_SetAndUpdate( "cg_announcerNewAwards", "" );

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "15" );
	}

	if (cg_ratInitialized.integer < 16) {
		trap_SendConsoleCommand("unset cg_timerScaleX;"
				        "unset cg_timerScaleY;"
				        "unset cg_fpsScaleX;"
				        "unset cg_fpsScaleY;"
				        "unset cg_speedScaleX;"
				        "unset cg_speedScaleY;"
				        "unset cg_teamOverlayScaleX;"
				        "unset cg_teamOverlayScaleY;"
				        "unset cg_crosshairNamesScaleX;"
				        "unset cg_crosshairNamesScaleY\n"
					);
		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "16" );
	}
	if (cg_ratInitialized.integer < 17) {
		CG_Cvar_ResetToDefault( "cg_hudDamageIndicatorScale" );
		CG_Cvar_ResetToDefault( "cg_hudDamageIndicatorOffset" );
		CG_Cvar_ResetToDefault( "cg_hudDamageIndicatorAlpha" );
		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "17" );
	}
	if (cg_ratInitialized.integer < 18) {
		switch (cg_weaponBarStyle.integer) {
			case 10:
			case 11:
				CG_Cvar_SetAndUpdate("cg_weaponBarStyle", "13");
				break;
		}
		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "18" );
	}
	
	if (cg_ratInitialized.integer < 19) {
		switch (cg_weaponBarStyle.integer) {
			case 1:
			case 5:
			case 6:
			case 7:
			case 8:
			case 10:
			case 11:
			case 13:
				CG_Cvar_SetAndUpdate("cg_weaponBarStyle", "14");
				break;
		}
		CG_Cvar_SetAndUpdate("cg_ratStatusbar", "4");
		CG_Cvar_SetAndUpdate("cg_hudDamageIndicator", "1");
		CG_Cvar_SetAndUpdate("cg_emptyIndicator", "1");
		CG_Cvar_SetAndUpdate("cg_drawFPS", "3");

		CG_Cvar_ResetToDefault( "cg_reloadIndicatorY" );
		CG_Cvar_ResetToDefault( "cg_reloadIndicatorAlpha" );
		CG_Cvar_ResetToDefault( "cg_reloadIndicatorHeight" );

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "19" );
	}

	if (cg_ratInitialized.integer < 20) {
		CG_Cvar_ResetToDefault( "cg_rocketStyle" );
		CG_Cvar_ResetToDefault( "cg_ratRail" );
		CG_Cvar_ResetToDefault( "cg_ratRailRadius" );
		CG_Cvar_ResetToDefault( "cg_railTrailTime" );

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "20" );
	}

	if (cg_ratInitialized.integer < 21) {
		CG_Cvar_ResetToDefault( "cg_zoomScopeRGColor" );
		CG_Cvar_ResetToDefault( "cg_zoomScopeMGColor" );

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "21" );
	}

	if (cg_ratInitialized.integer < 22) {
		int i;
		CG_Cvar_SetAndUpdate("cg_drawCrosshair", va("%i", CG_MigrateOldCrosshair((int)CG_Cvar_Get("cg_drawCrosshair"))));
		
		for (i = WP_GAUNTLET; i < WP_NUM_WEAPONS; ++i) {
			char *cvar = va("cg_ch%i", i);
			CG_Cvar_SetAndUpdate(cvar, va("%i", CG_MigrateOldCrosshair((int)CG_Cvar_Get(cvar))));
			cvar = va("cg_ch%isize", i);
			if ((int)CG_Cvar_Get(cvar) == 24) {
				CG_Cvar_SetAndUpdate(cvar, "30");
			}
		}

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "22" );
	}

	if (cg_ratInitialized.integer < 23) {
		trap_SendConsoleCommand("unset cg_ratPredictMissilesPing;"
				        "unset cg_ratPredictMissilesPingFactor\n"
				);
		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "23" );
	}

	if (cg_ratInitialized.integer < 24) {
		if (strlen(cg_enemySound.string) == 0) {
			CG_Cvar_ResetToDefault( "cg_enemySound" );
		}

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "24" );
	}
	if (cg_ratInitialized.integer < 25) {
		CG_SetEngineCvars();

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "25" );
	}

	if (cg_ratInitialized.integer < 26) {
		CG_Cvar_ResetToDefault( "cg_hudDamageIndicatorScale" );
		CG_Cvar_ResetToDefault( "cg_hudDamageIndicatorOffset" );

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "26" );
	}

	if (cg_ratInitialized.integer < 27) {
		trap_SendConsoleCommand("unset cg_picmipBackup\n");

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "27" );
	}

	if (cg_ratInitialized.integer < 28) {
		if (cg_hudDamageIndicator.integer != 2) {
			CG_Cvar_ResetToDefault( "cg_hudDamageIndicatorScale" );
			CG_Cvar_ResetToDefault( "cg_hudDamageIndicatorOffset" );
			CG_Cvar_ResetToDefault( "cg_hudDamageIndicator" );
		}

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "28" );
	}

	if (cg_ratInitialized.integer < 29) {
		CG_Cvar_ResetToDefault("cg_brightShells");
		CG_Cvar_ResetToDefault("cg_brightShellAlpha");
		if (!Q_stricmp(cg_enemyModel.string, "smarine/bright")) {
			CG_Cvar_ResetToDefault("cg_enemyModel");
		} else if (!Q_stricmp(cg_enemyModel.string, "sarge/bright")) {
			CG_Cvar_SetAndUpdate("cg_enemyModel", "sarge/gray");
		}
		if (!Q_stricmp(cg_teamModel.string, "sarge/bright")) {
			CG_Cvar_ResetToDefault("cg_teamModel");
		} else if (!Q_stricmp(cg_teamModel.string, "smarine/bright")) {
			CG_Cvar_SetAndUpdate("cg_teamModel", "smarine/gray");
		}

		CG_Cvar_SetAndUpdate( "cg_ratInitialized", "29" );
	}
}

/*
 * Make sure defaults are up to date
 */
void CG_RatInitDefaults(void)  {
	if (cg_ratInitialized.integer == 0) {
		CG_SetEngineCvars();
		CG_CvarResetDefaults();
		CG_Cvar_SetAndUpdate( "cg_ratInitialized", va("%i", LATEST_RATINITIALIZED) );
	} else if (cg_ratInitialized.integer < LATEST_RATINITIALIZED) {
		CG_RatOldCfgUpdate();
	}
}

/*																																			
===================
CG_ForceModelChange
===================
*/
void CG_ForceModelChange( void ) {
	int		i;

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0] ) {
			continue;
		}
		CG_NewClientInfo( i );
	}
	CG_LoadDeferredPlayers();
}

void CG_Cvar_PrintUserChanges( qboolean all ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		if ( cv->cvarFlags & (CVAR_ROM 
					| CVAR_USER_CREATED 
					| CVAR_SERVER_CREATED 
					| CVAR_SERVERINFO 
					| CVAR_SYSTEMINFO 
					| CVAR_TEMP)
				) {
			continue;
		}
		if (!all) {
			if (Q_stricmpn(cv->cvarName, "cg_", 3) != 0) {
				// exclude non-cg cvars that might be in the table
				continue;
			}
			if (Q_stricmp(cv->cvarName, "cg_ratInitialized") == 0) {
				// exclude cg_ratInitialized because users should never
				// write that into their manual config files
				continue;
			}
		}
		trap_Cvar_Update( cv->vmCvar );
		if (strcmp(cv->defaultString, cv->vmCvar->string) == 0) {
			continue;
		}
		Com_Printf(S_COLOR_YELLOW "seta " S_COLOR_WHITE "%s " S_COLOR_MAGENTA "\"%s\"\n",
			       	cv->cvarName, cv->vmCvar->string);
	}
}

void CG_Cvar_Update( const char *var_name ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		if (Q_stricmp(cv->cvarName, var_name) == 0) {
			trap_Cvar_Update( cv->vmCvar );
			break;
		}
	}
}

void CG_Cvar_SetAndUpdate( const char *var_name, const char *value ) {
	trap_Cvar_Set( var_name, value );
	CG_Cvar_Update(var_name);
}

void CG_Cvar_ResetToDefault( const char *var_name ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		if (Q_stricmp(var_name, cv->cvarName) != 0) {
			continue;
		}
		trap_Cvar_Set( cv->cvarName, cv->defaultString );
		trap_Cvar_Update( cv->vmCvar );
	}
}

void CG_CvarResetDefaults( void ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Set( cv->cvarName, cv->defaultString );
		trap_Cvar_Update( cv->vmCvar );
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
//unlagged - client options
		// clamp the value between 0 and 999
		// negative values would suck - people could conceivably shoot other
		// players *long* after they had left the area, on purpose
		if ( cv->vmCvar == &cg_cmdTimeNudge ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, 999 );
		}
		// cl_timenudge less than -50 or greater than 50 doesn't actually
		// do anything more than -50 or 50 (actually the numbers are probably
		// closer to -30 and 30, but 50 is nice and round-ish)
		// might as well not feed the myth, eh?
		else if ( cv->vmCvar == &cl_timeNudge ) {
			if (cgs.ratFlags & RAT_NOTIMENUDGE) {
				//if (cv->vmCvar->integer != 0) {
				//	Com_sprintf( cv->vmCvar->string, MAX_CVAR_VALUE_STRING, "0");
				//	trap_Cvar_Set( cv->cvarName, cv->vmCvar->string );
				//}
				if (cv->vmCvar->integer > 0) {
					Com_sprintf( cv->vmCvar->string, MAX_CVAR_VALUE_STRING, "0");
					trap_Cvar_Set( cv->cvarName, cv->vmCvar->string );
				}
			} else {
				CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, -50, 50 );
			}
		}
		// don't let this go too high - no point
		/*else if ( cv->vmCvar == &cg_latentSnaps ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, 10 );
		}*/
		// don't let this get too large
		/*else if ( cv->vmCvar == &cg_latentCmds ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, MAX_LATENT_CMDS - 1 );
		}*/
		// no more than 100% packet loss
		/*else if ( cv->vmCvar == &cg_plOut ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, 100 );
		}*/
//unlagged - client options
                else if ( cv->vmCvar == &cg_errorDecay ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, 250 );
		}
                else if ( cv->vmCvar == &com_maxfps ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, 500 );
		}
                else if ( cv->vmCvar == &sv_fps ) {
			if (cv->vmCvar->integer < 1) {
				Com_sprintf( cv->vmCvar->string, MAX_CVAR_VALUE_STRING, "1");
				trap_Cvar_Set( cv->cvarName, cv->vmCvar->string );
			}
		}
                else if ( cv->vmCvar == &con_notifytime ) {
			if (cg_newConsole.integer ) {
				if (cv->vmCvar->integer != -1) {
					Com_sprintf( cv->vmCvar->string, MAX_CVAR_VALUE_STRING, "-1");
					trap_Cvar_Set( cv->cvarName, cv->vmCvar->string );
				}
			} else if (cv->vmCvar->integer <= 0) {
				Com_sprintf( cv->vmCvar->string, MAX_CVAR_VALUE_STRING, "%s", cv->defaultString);
				trap_Cvar_Set( cv->cvarName, cv->vmCvar->string );
			}
		}
		trap_Cvar_Update( cv->vmCvar );
	}

	// check for modications here
	


	// If team overlay is on, ask for updates from the server.  If its off,
	// let the server know so we don't receive it
	if ( drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount ) {
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;

		if ( cg_drawTeamOverlay.integer > 0 ) {
			trap_Cvar_Set( "teamoverlay", "1" );
		} else {
			trap_Cvar_Set( "teamoverlay", "0" );
		}
	}

	// if force model changed
	if ( forceModelModificationCount != cg_forceModel.modificationCount ) {
		forceModelModificationCount = cg_forceModel.modificationCount;
		CG_ForceModelChange();
	}
	i = cg_enemyModel.modificationCount + cg_teamModel.modificationCount;
	if ( enemyTeamModelModificationCounts != i ) {
		enemyTeamModelModificationCounts = i;
		CG_ForceModelChange();
	}
	if ( mySoundModificationCount != cg_mySound.modificationCount
			|| teamSoundModificationCount != cg_teamSound.modificationCount
			|| enemySoundModificationCount != cg_enemySound.modificationCount) {
		CG_LoadForcedSounds();
		mySoundModificationCount = cg_mySound.modificationCount;
		teamSoundModificationCount = cg_teamSound.modificationCount;
		enemySoundModificationCount = cg_enemySound.modificationCount;
	}

	i = cg_teamHueBlue.modificationCount
		+ cg_teamHueDefault.modificationCount
		+ cg_teamHueRed.modificationCount
		+ cg_enemyColor.modificationCount
		+ cg_teamColor.modificationCount
		+ cg_enemyHeadColor.modificationCount
		+  cg_teamHeadColor.modificationCount
		+ cg_enemyTorsoColor.modificationCount
		+  cg_teamTorsoColor.modificationCount
		+ cg_enemyLegsColor.modificationCount
		+  cg_teamLegsColor.modificationCount
		+ cg_enemyCorpseSaturation.modificationCount
		+ cg_enemyCorpseValue.modificationCount
		+ cg_teamCorpseSaturation.modificationCount
		+ cg_teamCorpseValue.modificationCount;
	if ( forceColorModificationCounts != i) {
		CG_ParseForcedColors();
		forceColorModificationCounts = i;
	}

	if ( ratStatusbarModificationCount != cg_ratStatusbar.modificationCount ) {
		CG_RegisterNumbers();
		ratStatusbarModificationCount = cg_ratStatusbar.modificationCount;
	}
}

int CG_CrosshairPlayer( void ) {
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) ) {
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
	if ( !cg.attackerTime ) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_PrintfChat( qboolean team, const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	if (cg_newConsole.integer) {
		if (team) {
			CG_AddToGenericConsole(text, &cgs.teamChat);
		} else {
			CG_AddToGenericConsole(text, &cgs.chat);
		}
		CG_AddToGenericConsole(text, &cgs.commonConsole);
	}
	trap_Print( text );
}

void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	if (cg_newConsole.integer) {
		CG_AddToGenericConsole(text, &cgs.console);
		CG_AddToGenericConsole(text, &cgs.commonConsole);
	}
	trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Error( text );
}

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	CG_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	CG_Printf ("%s", text);
}

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	gitem_t			*item;
	char			data[MAX_QPATH];
	char			*s, *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if( item->pickup_sound ) {
		trap_S_RegisterSound( item->pickup_sound, qfalse );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string", 
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "wav" )) {
			trap_S_RegisterSound( data, qfalse );
		}
	}
}

qboolean CG_SupportsOggVorbis(void) {
	static int supports_ogg = -1;

	if (supports_ogg == -1) {
		qhandle_t ogg = trap_S_RegisterSound("sound/testoggvorbis.ogg", qtrue);
		if (ogg) {
			supports_ogg = 1;
		} else {
			supports_ogg = 0;
		}
	}

	return (qboolean)supports_ogg;
}

void CG_GetAnnouncer(const char *announcerCfg, char *outAnnouncer, int announcersz, char *outformat, int formatsz) {
	const char *p;
	char *p2;
	int len;
	fileHandle_t f;
	char buf[8];

	Q_strncpyz(outAnnouncer, "", announcersz);
	Q_strncpyz(outformat, "wav", formatsz);

	if (strlen(announcerCfg) == 0) {
		return;
	}

	for (p = announcerCfg; *p != '\0'; ++p) {
		if (!isalnum(*p)) {
			return;
		}
	}
	
	len = trap_FS_FOpenFile(va("scripts/announcer_%s.formatcfg", announcerCfg), &f, FS_READ);
	if (!f) {
		return;
	}
	if (!len || len >= sizeof(buf)-1) {
		trap_FS_FCloseFile(f);
	}
	trap_FS_Read(buf, sizeof(buf)-1, f);
	buf[len] = '\0';
	trap_FS_FCloseFile(f);

	for (p2 = buf; *p2 != '\0'; ++p2) {
		if (*p2 == '\n') {
			*p2 = '\0';
			continue;
		}
		if (!isalnum(*p2)) {
			return;
		}
	}

	if (strlen(buf) + 1 > formatsz || strlen(announcerCfg) + 2 > announcersz) {
		return;
	}

	if (Q_stricmp(buf, "ogg") == 0) {
		if (!CG_SupportsOggVorbis()) {
			// use default announcer if there is ogg support
			CG_Printf(S_COLOR_RED "ERROR: Unable to load announcer \"%s\"" S_COLOR_RED ": engine lacks Ogg Vorbis support!\n",
					announcerCfg);
			return;
		}
	} else if (Q_stricmp(buf, "wav") != 0) {
		// if it's something other than ogg or wav, use the default announcer
		return;
	}

	Q_strncpyz(outformat, buf, formatsz);
	Q_strncpyz(outAnnouncer, va("%s/", announcerCfg), announcersz);
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int		i;
	char	items[MAX_ITEMS+1];
	char	name[MAX_QPATH];
	const char	*soundName;
 	char announcer[32];
	char format[16];

	// voice commands
#ifdef MISSIONPACK
	CG_LoadVoiceChats();
#endif

	CG_GetAnnouncer(cg_announcer.string, announcer, sizeof(announcer),
			format, sizeof(format));

	cgs.media.oneMinuteSound = trap_S_RegisterSound( va("sound/%sfeedback/1_minute.%s", announcer, format), qtrue );
	cgs.media.fiveMinuteSound = trap_S_RegisterSound( va("sound/%sfeedback/5_minute.%s", announcer, format), qtrue );
	cgs.media.suddenDeathSound = trap_S_RegisterSound( va("sound/%sfeedback/sudden_death.%s", announcer, format), qtrue );
	cgs.media.oneFragSound = trap_S_RegisterSound( va("sound/%sfeedback/1_frag.%s", announcer, format), qtrue );
	cgs.media.twoFragSound = trap_S_RegisterSound( va("sound/%sfeedback/2_frags.%s", announcer, format), qtrue );
	cgs.media.threeFragSound = trap_S_RegisterSound( va("sound/%sfeedback/3_frags.%s", announcer, format), qtrue );
	cgs.media.count3Sound = trap_S_RegisterSound( va("sound/%sfeedback/three.%s", announcer, format), qtrue );
	cgs.media.count2Sound = trap_S_RegisterSound( va("sound/%sfeedback/two.%s", announcer, format), qtrue );
	cgs.media.count1Sound = trap_S_RegisterSound( va("sound/%sfeedback/one.%s", announcer, format), qtrue );
	cgs.media.countFightSound = trap_S_RegisterSound( va("sound/%sfeedback/fight.%s", announcer, format), qtrue );
	cgs.media.countPrepareSound = trap_S_RegisterSound( va("sound/%sfeedback/prepare.%s", announcer, format), qtrue );
#ifdef MISSIONPACK
	cgs.media.countPrepareTeamSound = trap_S_RegisterSound( "sound/feedback/prepare_team.wav", qtrue );
#endif

	// N_G: Another condition that makes no sense to me, see for
	// yourself if you really meant this
	// Sago: Makes perfect sense: Load team game stuff if the gametype is a teamgame and not an exception (like GT_LMS)
	if ( ( ( cgs.gametype >= GT_TEAM ) && ( cgs.ffa_gt != 1 ) ) ||
		cg_buildScript.integer ) {

		cgs.media.captureAwardSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
		cgs.media.redLeadsSound = trap_S_RegisterSound( va("sound/%sfeedback/redleads.%s", announcer, format), qtrue );
		cgs.media.blueLeadsSound = trap_S_RegisterSound( va("sound/%sfeedback/blueleads.%s", announcer, format), qtrue );
		cgs.media.teamsTiedSound = trap_S_RegisterSound( va("sound/%sfeedback/teamstied.%s", announcer, format), qtrue );
		cgs.media.hitTeamSound = trap_S_RegisterSound( "sound/feedback/hit_teammate.wav", qtrue );

		cgs.media.redScoredSound = trap_S_RegisterSound( va("sound/%steamplay/voc_red_scores.%s", announcer, format), qtrue );
		cgs.media.blueScoredSound = trap_S_RegisterSound( va("sound/%steamplay/voc_blue_scores.%s", announcer, format), qtrue );

		cgs.media.captureYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
		cgs.media.captureOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_opponent.wav", qtrue );

		cgs.media.returnYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_yourteam.wav", qtrue );
		cgs.media.returnOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );

		cgs.media.takenYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_yourteam.wav", qtrue );
		cgs.media.takenOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_opponent.wav", qtrue );

		cgs.media.flagDroppedSound = trap_S_RegisterSound( "sound/teamplay/flag_dropped.wav", qtrue );

		if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION|| cg_buildScript.integer ) {
			cgs.media.redFlagReturnedSound = trap_S_RegisterSound( va("sound/%steamplay/voc_red_returned.%s", announcer, format), qtrue );
			cgs.media.blueFlagReturnedSound = trap_S_RegisterSound( va("sound/%steamplay/voc_blue_returned.%s", announcer, format), qtrue );
			cgs.media.enemyTookYourFlagSound = trap_S_RegisterSound( va("sound/%steamplay/voc_enemy_flag.%s", announcer, format), qtrue );
			cgs.media.yourTeamTookEnemyFlagSound = trap_S_RegisterSound( va("sound/%steamplay/voc_team_flag.%s", announcer, format), qtrue );
		}

		if ( cgs.gametype == GT_ELIMINATION ) {
			cgs.media.oneLeftSound = trap_S_RegisterSound( "sound/treb/ratmod/extermination/rat_race.ogg", qtrue );
			cgs.media.oneFriendLeftSound = trap_S_RegisterSound( "sound/treb/ratmod/extermination/last_man.ogg", qtrue );
			cgs.media.oneEnemyLeftSound = trap_S_RegisterSound( "sound/treb/ratmod/extermination/the_chase_is_on.ogg", qtrue );
			cgs.media.elimPlayerRespawnSound = trap_S_RegisterSound( "sound/treb/ratmod/extermination/spawn.ogg", qtrue );
		}

		if ( cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {
			// FIXME: get a replacement for this sound ?
			cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );
			cgs.media.yourTeamTookTheFlagSound = trap_S_RegisterSound( va("sound/%steamplay/voc_team_1flag.%s", announcer, format), qtrue );
			cgs.media.enemyTookTheFlagSound = trap_S_RegisterSound( va("sound/%steamplay/voc_enemy_1flag.%s", announcer, format), qtrue );
		}

		if ( cgs.gametype == GT_1FCTF || cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION ||cg_buildScript.integer ) {
			cgs.media.youHaveFlagSound = trap_S_RegisterSound( va("sound/%steamplay/voc_you_flag.%s", announcer, format), qtrue );
			cgs.media.holyShitSound = trap_S_RegisterSound(va("sound/%sfeedback/voc_holyshit.%s", announcer, format), qtrue);
		}

                if ( cgs.gametype == GT_OBELISK || cg_buildScript.integer ) {
			cgs.media.yourBaseIsUnderAttackSound = trap_S_RegisterSound( va("sound/%steamplay/voc_base_attack.%s", announcer, format), qtrue );
		}
	}

	cgs.media.tracerSound = trap_S_RegisterSound( "sound/weapons/machinegun/buletby1.wav", qfalse );
	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav", qfalse );
	cgs.media.wearOffSound = trap_S_RegisterSound( "sound/items/wearoff.wav", qfalse );
	cgs.media.useNothingSound = trap_S_RegisterSound( "sound/items/use_nothing.wav", qfalse );
	cgs.media.gibSound = trap_S_RegisterSound( "sound/player/gibsplt1.wav", qfalse );
	cgs.media.gibBounce1Sound = trap_S_RegisterSound( "sound/player/gibimp1.wav", qfalse );
	cgs.media.gibBounce2Sound = trap_S_RegisterSound( "sound/player/gibimp2.wav", qfalse );
	cgs.media.gibBounce3Sound = trap_S_RegisterSound( "sound/player/gibimp3.wav", qfalse );


	// LEILEI

	cgs.media.lspl1Sound = trap_S_RegisterSound( "sound/le/splat1.wav", qfalse );
	cgs.media.lspl2Sound = trap_S_RegisterSound( "sound/le/splat2.wav", qfalse );
	cgs.media.lspl3Sound = trap_S_RegisterSound( "sound/le/splat3.wav", qfalse );

	cgs.media.lbul1Sound = trap_S_RegisterSound( "sound/le/bullet1.wav", qfalse );
	cgs.media.lbul2Sound = trap_S_RegisterSound( "sound/le/bullet2.wav", qfalse );
	cgs.media.lbul3Sound = trap_S_RegisterSound( "sound/le/bullet3.wav", qfalse );

	cgs.media.lshl1Sound = trap_S_RegisterSound( "sound/le/shell1.wav", qfalse );
	cgs.media.lshl2Sound = trap_S_RegisterSound( "sound/le/shell2.wav", qfalse );
	cgs.media.lshl3Sound = trap_S_RegisterSound( "sound/le/shell3.wav", qfalse );

	cgs.media.useInvulnerabilitySound = trap_S_RegisterSound( "sound/items/invul_activate.wav", qfalse );
	cgs.media.invulnerabilityImpactSound1 = trap_S_RegisterSound( "sound/items/invul_impact_01.wav", qfalse );
	cgs.media.invulnerabilityImpactSound2 = trap_S_RegisterSound( "sound/items/invul_impact_02.wav", qfalse );
	cgs.media.invulnerabilityImpactSound3 = trap_S_RegisterSound( "sound/items/invul_impact_03.wav", qfalse );
	cgs.media.invulnerabilityJuicedSound = trap_S_RegisterSound( "sound/items/invul_juiced.wav", qfalse );

	//cgs.media.ammoregenSound = trap_S_RegisterSound("sound/items/cl_ammoregen.wav", qfalse);
	cgs.media.doublerSound = trap_S_RegisterSound("sound/items/cl_doubler.wav", qfalse);
	cgs.media.guardSound = trap_S_RegisterSound("sound/items/cl_guard.wav", qfalse);
	cgs.media.scoutSound = trap_S_RegisterSound("sound/items/cl_scout.wav", qfalse);
        cgs.media.obeliskHitSound1 = trap_S_RegisterSound( "sound/items/obelisk_hit_01.wav", qfalse );
	cgs.media.obeliskHitSound2 = trap_S_RegisterSound( "sound/items/obelisk_hit_02.wav", qfalse );
	cgs.media.obeliskHitSound3 = trap_S_RegisterSound( "sound/items/obelisk_hit_03.wav", qfalse );
	cgs.media.obeliskRespawnSound = trap_S_RegisterSound( "sound/items/obelisk_respawn.wav", qfalse );

	cgs.media.teleShotSound = trap_S_RegisterSound( "sound/world/teleshot.wav", qfalse );

	cgs.media.teleInSound = trap_S_RegisterSound( "sound/world/telein.wav", qfalse );
	cgs.media.teleOutSound = trap_S_RegisterSound( "sound/world/teleout.wav", qfalse );
	cgs.media.respawnSound = trap_S_RegisterSound( "sound/items/respawn1.wav", qfalse );

	cgs.media.noAmmoSound = trap_S_RegisterSound( "sound/weapons/noammo.wav", qfalse );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/player/talk.wav", qfalse );
	cgs.media.teamTalkSound = trap_S_RegisterSound( "sound/player/teamtalk.wav", qfalse );
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav", qfalse);

        switch(cg_hitsound.integer) {
            
            case 0:
            default:
            cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit_old.wav", qfalse );
            cgs.media.hitSound0 = trap_S_RegisterSound( "sound/feedback/hit0.wav", qfalse );
            cgs.media.hitSound1 = trap_S_RegisterSound( "sound/feedback/hit1.wav", qfalse );
            cgs.media.hitSound2 = trap_S_RegisterSound( "sound/feedback/hit2.wav", qfalse );
            cgs.media.hitSound3 = trap_S_RegisterSound( "sound/feedback/hit3.wav", qfalse );
            cgs.media.hitSound4 = trap_S_RegisterSound( "sound/feedback/hit4.wav", qfalse );
        };

#ifdef MISSIONPACK
	cgs.media.hitSoundHighArmor = trap_S_RegisterSound( "sound/feedback/hithi.wav", qfalse );
	cgs.media.hitSoundLowArmor = trap_S_RegisterSound( "sound/feedback/hitlo.wav", qfalse );
#endif

	cgs.media.accuracySound = trap_S_RegisterSound( va("sound/%sfeedback/accuracy.%s", announcer, format), qtrue );
	cgs.media.fragsSound = trap_S_RegisterSound( va("sound/%sfeedback/frags.%s", announcer, format), qtrue );
	cgs.media.impressiveSound = trap_S_RegisterSound( va("sound/%sfeedback/impressive.%s", announcer, format), qtrue );
	cgs.media.excellentSound = trap_S_RegisterSound( va("sound/%sfeedback/excellent.%s", announcer, format), qtrue );
	cgs.media.deniedSound = trap_S_RegisterSound( va("sound/%sfeedback/denied.%s", announcer, format), qtrue );
	cgs.media.humiliationSound = trap_S_RegisterSound( va("sound/%sfeedback/humiliation.%s", announcer, format), qtrue );
	cgs.media.assistSound = trap_S_RegisterSound( va("sound/%sfeedback/assist.%s", announcer, format), qtrue );
	cgs.media.defendSound = trap_S_RegisterSound( va("sound/%sfeedback/defense.%s", announcer, format), qtrue );
	cgs.media.perfectSound = trap_S_RegisterSound( va("sound/%sfeedback/perfect.%s", announcer, format), qtrue );
#ifdef MISSIONPACK
	cgs.media.firstImpressiveSound = trap_S_RegisterSound( "sound/feedback/first_impressive.wav", qtrue );
	cgs.media.firstExcellentSound = trap_S_RegisterSound( "sound/feedback/first_excellent.wav", qtrue );
	cgs.media.firstHumiliationSound = trap_S_RegisterSound( "sound/feedback/first_gauntlet.wav", qtrue );
#endif


	cgs.media.takenLeadSound = trap_S_RegisterSound( va("sound/%sfeedback/takenlead.%s", announcer, format), qtrue);
	cgs.media.tiedLeadSound = trap_S_RegisterSound( va("sound/%sfeedback/tiedlead.%s", announcer, format), qtrue);
	cgs.media.lostLeadSound = trap_S_RegisterSound( va("sound/%sfeedback/lostlead.%s", announcer, format), qtrue);

//#ifdef MISSIONPACK
	cgs.media.voteNow = trap_S_RegisterSound( va("sound/%sfeedback/vote_now.%s", announcer, format), qtrue);
	cgs.media.votePassed = trap_S_RegisterSound( va("sound/%sfeedback/vote_passed.%s", announcer, format), qtrue);
	cgs.media.voteFailed = trap_S_RegisterSound( va("sound/%sfeedback/vote_failed.%s", announcer, format), qtrue);
//#endif


	cgs.media.eaward_sounds[EAWARD_FRAGS] = trap_S_RegisterSound( va("sound/%sfeedback/frags.%s", announcer, format), qtrue);
	cgs.media.eaward_sounds[EAWARD_ACCURACY] = trap_S_RegisterSound( va("sound/%sfeedback/accuracy.%s", announcer, format), qtrue);

	if (strlen(cg_announcerNewAwards.string) > 0) {
		CG_GetAnnouncer(cg_announcerNewAwards.string, announcer, sizeof(announcer),
				format, sizeof(format));
	}
	if (strlen(announcer) > 0) {
		cgs.media.eaward_sounds[EAWARD_TELEFRAG] = trap_S_RegisterSound( va("sound/%sratmod/medals/telefrag.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_TELEMISSILE_FRAG] = trap_S_RegisterSound( va("sound/%sratmod/medals/interdimensional.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_ROCKETSNIPER] = trap_S_RegisterSound( va("sound/%sratmod/medals/rocketsniper.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_FULLSG] = trap_S_RegisterSound( va("sound/%sratmod/medals/pointblank.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_IMMORTALITY] = trap_S_RegisterSound( va("sound/%sratmod/medals/immortal.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_AIRROCKET] = trap_S_RegisterSound( va("sound/%sratmod/medals/airrocket.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_AIRGRENADE] = trap_S_RegisterSound( va("sound/%sratmod/medals/airnade.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_ROCKETRAIL] = trap_S_RegisterSound( va("sound/%sratmod/medals/combokill.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_LGRAIL] = trap_S_RegisterSound( va("sound/%sratmod/medals/combokill.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_RAILTWO] = trap_S_RegisterSound( va("sound/%sratmod/medals/railmaster.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_DEADHAND] = trap_S_RegisterSound( va("sound/%sratmod/medals/deadhand.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_TWITCHRAIL] = trap_S_RegisterSound( va("sound/%sratmod/medals/aimbot.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_SHOWSTOPPER] = trap_S_RegisterSound( va("sound/%sratmod/medals/showstopper.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_AMBUSH] = trap_S_RegisterSound( va("sound/%sratmod/medals/ambush.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_KAMIKAZE] = trap_S_RegisterSound( va("sound/%sratmod/medals/kamikaze_medal.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_STRONGMAN] = trap_S_RegisterSound( va("sound/%sratmod/medals/strongman.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_HERO] = trap_S_RegisterSound( va("sound/%sratmod/medals/hero.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_RAT] = trap_S_RegisterSound( va("sound/%sratmod/medals/rat.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_BUTCHER] = trap_S_RegisterSound( va("sound/%sratmod/medals/butcher.%s", announcer, format), qtrue);

		cgs.media.eaward_sounds[EAWARD_KILLINGSPREE] = trap_S_RegisterSound( va("sound/%sratmod/medals/killing_spree.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_RAMPAGE] = trap_S_RegisterSound( va("sound/%sratmod/medals/rampage.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_MASSACRE] = trap_S_RegisterSound( va("sound/%sratmod/medals/masacre.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_UNSTOPPABLE] = trap_S_RegisterSound( va("sound/%sratmod/medals/unstoppable.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_GRIMREAPER] = trap_S_RegisterSound( va("sound/%sratmod/medals/grim_reaper.%s", announcer, format), qtrue);

		cgs.media.eaward_sounds[EAWARD_BERSERKER] = trap_S_RegisterSound( va("sound/%sratmod/medals/berserker.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_REVENGE] = trap_S_RegisterSound( va("sound/%sratmod/medals/revenge.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_VAPORIZED] = trap_S_RegisterSound( va("sound/%sratmod/medals/vaporized.%s", announcer, format), qtrue);
	} else {
		cgs.media.eaward_sounds[EAWARD_TELEFRAG] = cgs.media.humiliationSound;
		cgs.media.eaward_sounds[EAWARD_TELEMISSILE_FRAG] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_ROCKETSNIPER] = cgs.media.accuracySound;
		cgs.media.eaward_sounds[EAWARD_FULLSG] = cgs.media.accuracySound;
		cgs.media.eaward_sounds[EAWARD_IMMORTALITY] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_AIRROCKET] = cgs.media.accuracySound;
		cgs.media.eaward_sounds[EAWARD_AIRGRENADE] = cgs.media.accuracySound;
		cgs.media.eaward_sounds[EAWARD_ROCKETRAIL] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_LGRAIL] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_RAILTWO] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_DEADHAND] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_TWITCHRAIL] = cgs.media.accuracySound;
		cgs.media.eaward_sounds[EAWARD_SHOWSTOPPER] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_AMBUSH] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_KAMIKAZE] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_STRONGMAN] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_HERO] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_RAT] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_BUTCHER] = cgs.media.perfectSound;

		cgs.media.eaward_sounds[EAWARD_KILLINGSPREE] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_RAMPAGE] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_MASSACRE] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_UNSTOPPABLE] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_GRIMREAPER] = cgs.media.holyShitSound;

		cgs.media.eaward_sounds[EAWARD_BERSERKER] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_REVENGE] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_VAPORIZED] = cgs.media.perfectSound;
	}

	cgs.media.watrInSound = trap_S_RegisterSound( "sound/player/watr_in.wav", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/watr_out.wav", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound( "sound/player/watr_un.wav", qfalse);

	cgs.media.jumpPadSound = trap_S_RegisterSound ("sound/world/jumppad.wav", qfalse );

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/mech%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/energy%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/splash%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/clank%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound (name, qfalse);
	}

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
//		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_RegisterItemSounds( i );
//		}
	}

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS+i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' ) {
			continue;	// custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound( soundName, qfalse );
	}

	// FIXME: only needed with item
	cgs.media.flightSound = trap_S_RegisterSound( "sound/items/flight.wav", qfalse );
	cgs.media.medkitSound = trap_S_RegisterSound ("sound/items/use_medkit.wav", qfalse);
	cgs.media.quadSound = trap_S_RegisterSound("sound/items/damage3.wav", qfalse);
	cgs.media.sfx_ric1 = trap_S_RegisterSound ("sound/weapons/machinegun/ric1.wav", qfalse);
	cgs.media.sfx_ric2 = trap_S_RegisterSound ("sound/weapons/machinegun/ric2.wav", qfalse);
	cgs.media.sfx_ric3 = trap_S_RegisterSound ("sound/weapons/machinegun/ric3.wav", qfalse);
	cgs.media.sfx_railg = trap_S_RegisterSound ("sound/weapons/railgun/railgf1a.wav", qfalse);
	cgs.media.sfx_rockexp = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
	cgs.media.sfx_plasmaexp = trap_S_RegisterSound ("sound/weapons/plasma/plasmx1a.wav", qfalse);
	cgs.media.sfx_proxexp = trap_S_RegisterSound( "sound/weapons/proxmine/wstbexpl.wav" , qfalse);
	//cgs.media.sfx_nghit = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpd.wav" , qfalse);
	cgs.media.sfx_nghitflesh = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpl.wav" , qfalse);
	cgs.media.sfx_nghitmetal = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpm.wav", qfalse );
	//cgs.media.sfx_chghit = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpd.wav", qfalse );
	//cgs.media.sfx_chghitflesh = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpl.wav", qfalse );
	//cgs.media.sfx_chghitmetal = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpm.wav", qfalse );
	cgs.media.weaponHoverSound = trap_S_RegisterSound( "sound/weapons/weapon_hover.wav", qfalse );
	cgs.media.kamikazeExplodeSound = trap_S_RegisterSound( "sound/items/kam_explode.wav", qfalse );
	cgs.media.kamikazeImplodeSound = trap_S_RegisterSound( "sound/items/kam_implode.wav", qfalse );
	cgs.media.kamikazeFarSound = trap_S_RegisterSound( "sound/items/kam_explode_far.wav", qfalse );
	cgs.media.winnerSound = trap_S_RegisterSound( "sound/feedback/voc_youwin.wav", qfalse );
	cgs.media.loserSound = trap_S_RegisterSound( "sound/feedback/voc_youlose.wav", qfalse );
	//cgs.media.youSuckSound = trap_S_RegisterSound( "sound/misc/yousuck.wav", qfalse );

	cgs.media.wstbimplSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpl.wav", qfalse);
	cgs.media.wstbimpmSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpm.wav", qfalse);
	cgs.media.wstbimpdSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpd.wav", qfalse);
	cgs.media.wstbactvSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbactv.wav", qfalse);

	cgs.media.regenSound = trap_S_RegisterSound("sound/items/regen.wav", qfalse);
	cgs.media.protectSound = trap_S_RegisterSound("sound/items/protect3.wav", qfalse);
	cgs.media.n_healthSound = trap_S_RegisterSound("sound/items/n_health.wav", qfalse );
	cgs.media.hgrenb1aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb1a.wav", qfalse);
	cgs.media.hgrenb2aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb2a.wav", qfalse);

	cgs.media.announceQuad = trap_S_RegisterSound("sound/treb/ratmod/powerups/quad_damage.ogg", qtrue);
	cgs.media.announceBattlesuit = trap_S_RegisterSound("sound/treb/ratmod/powerups/battlesuit.ogg", qtrue);
	cgs.media.announceHaste = trap_S_RegisterSound("sound/treb/ratmod/powerups/haste.ogg", qtrue);
	cgs.media.announceInvis = trap_S_RegisterSound("sound/treb/ratmod/powerups/invisibility.ogg", qtrue);
	cgs.media.announceRegen = trap_S_RegisterSound("sound/treb/ratmod/powerups/regeneration.ogg", qtrue);
	cgs.media.announceFlight = trap_S_RegisterSound("sound/treb/ratmod/powerups/flight.ogg", qtrue);

#ifdef MISSIONPACK
	trap_S_RegisterSound("sound/player/sergei/death1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sergei/death2.wav", qfalse );
	trap_S_RegisterSound("sound/player/sergei/death3.wav", qfalse );
	trap_S_RegisterSound("sound/player/sergei/jump1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sergei/pain25_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sergei/pain75_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sergei/pain100_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sergei/falling1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sergei/gasp.wav", qfalse );
	trap_S_RegisterSound("sound/player/sergei/drown.wav", qfalse );
	trap_S_RegisterSound("sound/player/sergei/fall1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sergei/taunt.wav", qfalse );

	trap_S_RegisterSound("sound/player/kyonshi/death1.wav", qfalse );
	trap_S_RegisterSound("sound/player/kyonshi/death2.wav", qfalse );
	trap_S_RegisterSound("sound/player/kyonshi/death3.wav", qfalse );
	trap_S_RegisterSound("sound/player/kyonshi/jump1.wav", qfalse );
	trap_S_RegisterSound("sound/player/kyonshi/pain25_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/kyonshi/pain75_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/kyonshi/pain100_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/kyonshi/falling1.wav", qfalse );
	trap_S_RegisterSound("sound/player/kyonshi/gasp.wav", qfalse );
	trap_S_RegisterSound("sound/player/kyonshi/drown.wav", qfalse );
	trap_S_RegisterSound("sound/player/kyonshi/fall1.wav", qfalse );
	trap_S_RegisterSound("sound/player/kyonshi/taunt.wav", qfalse );
#endif

	CG_LoadTaunts();

}


//===================================================================================


static void CG_RegisterNumbers(void) {
	int i;
	static char		*sb_nums[11] = {
		"gfx/2d/numbers%s/zero_32b",
		"gfx/2d/numbers%s/one_32b",
		"gfx/2d/numbers%s/two_32b",
		"gfx/2d/numbers%s/three_32b",
		"gfx/2d/numbers%s/four_32b",
		"gfx/2d/numbers%s/five_32b",
		"gfx/2d/numbers%s/six_32b",
		"gfx/2d/numbers%s/seven_32b",
		"gfx/2d/numbers%s/eight_32b",
		"gfx/2d/numbers%s/nine_32b",
		"gfx/2d/numbers%s/minus_32b",
	};
	for ( i=0 ; i<11 ; i++) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader( 
				va(sb_nums[i],
					(cg_ratStatusbar.integer >= 4 && cg_ratStatusbar.integer <= 5) ? "_trebfuture" : ""
				  )
			       	);
	}
}

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	int			i;
	char		items[MAX_ITEMS+1];

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_LoadingString( cgs.mapname );

	trap_R_LoadWorldMap( cgs.mapname );

	// precache status bar pics
	CG_LoadingString( "game media" );

	CG_RegisterNumbers();

	cgs.media.botSkillShaders[0] = trap_R_RegisterShader( "menu/art/skill1.tga" );
	cgs.media.botSkillShaders[1] = trap_R_RegisterShader( "menu/art/skill2.tga" );
	cgs.media.botSkillShaders[2] = trap_R_RegisterShader( "menu/art/skill3.tga" );
	cgs.media.botSkillShaders[3] = trap_R_RegisterShader( "menu/art/skill4.tga" );
	cgs.media.botSkillShaders[4] = trap_R_RegisterShader( "menu/art/skill5.tga" );

	cgs.media.ratSmallIcon = trap_R_RegisterShaderNoMip( "gfx/2d/rat_icon.tga" );

	cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer.tga" );

	cgs.media.scoreboardName = trap_R_RegisterShaderNoMip( "menu/tab/name.tga" );
	cgs.media.scoreboardPing = trap_R_RegisterShaderNoMip( "menu/tab/ping.tga" );
	cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip( "menu/tab/score.tga" );
	cgs.media.scoreboardTime = trap_R_RegisterShaderNoMip( "menu/tab/time.tga" );

	cgs.media.smokePuffShader = trap_R_RegisterShader( "smokePuff" );
	cgs.media.smokePuffRageProShader = trap_R_RegisterShader( "smokePuffRagePro" );
	cgs.media.plasmaTrailShader = trap_R_RegisterShader( "plasmaTrail" );
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader( "shotgunSmokePuff" );
	cgs.media.nailPuffShader = trap_R_RegisterShader( "nailtrail2" );
	cgs.media.blueProxMine = trap_R_RegisterModel( "models/weaphits/proxmineb.md3" );
	cgs.media.plasmaBallShader = trap_R_RegisterShader( "sprites/plasma1" );
	cgs.media.bloodTrailShader = trap_R_RegisterShader( "bloodTrail" );
	//cgs.media.lagometerShader = trap_R_RegisterShader("lagometer" );
	cgs.media.lagometerShader = trap_R_RegisterShader("gfx/2d/lag.tga" );
	cgs.media.connectionShader = trap_R_RegisterShader( "ratdisconnected" );
	//cgs.media.connectionShader = trap_R_RegisterShader( "gfx/2d/net.tga" );

	cgs.media.waterBubbleShader = trap_R_RegisterShader( "waterBubble" );

	cgs.media.tracerShader = trap_R_RegisterShader( "gfx/misc/tracer" );
	cgs.media.selectShader = trap_R_RegisterShader( "gfx/2d/select" );

	for (i = 0; i < NUM_CROSSHAIRS; i++ ) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshairs/crosshair%d", (i+1)) );
		cgs.media.crosshairOutlineShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshairs/crosshair%d_outline", (i+1)) );
 	}

	cgs.media.backTileShader = trap_R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.noammoShader = trap_R_RegisterShader( "icons/noammo" );

	// powerup shaders
	cgs.media.quadShader = trap_R_RegisterShader("powerups/ratQuad" );
	cgs.media.quadShaderBase = trap_R_RegisterShader("powerups/ratQuadGreyAlpha" );
	cgs.media.quadShaderSpots = trap_R_RegisterShader("powerups/ratQuadSpots" );
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon" );
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/ratBattleSuit" );
	cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon" );
	cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility" );
	cgs.media.regenShader = trap_R_RegisterShader("powerups/ratRegen" );
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff" );

	cgs.media.spawnPointShader = trap_R_RegisterShader("spawnPoint" );

	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION|| cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER || cgs.gametype == GT_TREASURE_HUNTER || cg_buildScript.integer ) {
		cgs.media.redCubeModel = trap_R_RegisterModel( "models/powerups/orb/r_orb.md3" );
		cgs.media.blueCubeModel = trap_R_RegisterModel( "models/powerups/orb/b_orb.md3" );
		cgs.media.redCubeIcon = trap_R_RegisterShader( "icons/skull_red" );
		cgs.media.blueCubeIcon = trap_R_RegisterShader( "icons/skull_blue" );
	}

	if (cgs.gametype == GT_TREASURE_HUNTER) {
		//cgs.media.thEnemyToken = trap_R_RegisterModel( "models/powerups/overload_base.md3" );
		cgs.th_oldTokenStyle = -1000;
		cgs.th_tokenStyle = -999;
		//cgs.media.thTokenTeamShader = trap_R_RegisterShader( "models/powerups/treasure/thTokenTeam" );
		//cgs.media.thTokenRedShader = trap_R_RegisterShader( "models/powerups/treasure/thTokenRed" );
		//cgs.media.thTokenBlueShader = trap_R_RegisterShader( "models/powerups/treasure/thTokenBlue" );
		cgs.media.thTokenBlueIShader = trap_R_RegisterShaderNoMip("sprites/thTokenIndicatorBlue.tga");
		cgs.media.thTokenRedIShader = trap_R_RegisterShaderNoMip("sprites/thTokenIndicatorRed.tga");
		cgs.media.thTokenBlueISolidShader= trap_R_RegisterShaderNoMip("sprites/thTokenIndicatorBlueSolid.tga");
		cgs.media.thTokenRedISolidShader= trap_R_RegisterShaderNoMip("sprites/thTokenIndicatorRedSolid.tga");
	}
        
        if( ( cgs.gametype >= GT_TEAM ) && ( cgs.ffa_gt != 1 ) ) {
                cgs.media.redOverlay = trap_R_RegisterShader( "playeroverlays/playerSuit1_Red");
                cgs.media.blueOverlay = trap_R_RegisterShader( "playeroverlays/playerSuit1_Blue");
        } else {
                cgs.media.neutralOverlay = trap_R_RegisterShader( "playeroverlays/playerSuit1_Neutral");
        }

	cgs.media.brightShell = trap_R_RegisterShader( "playerBrightShell");
	cgs.media.brightShellFlat = trap_R_RegisterShader( "playerBrightShellFlat");

	//cgs.media.brightOutline = trap_R_RegisterShader( "playerBrightOutline10");
	cgs.media.brightOutline = trap_R_RegisterShader( "playerBrightOutline08");
	cgs.media.brightOutlineSmall = trap_R_RegisterShader( "playerBrightOutline05");
	cgs.media.brightOutlineOpaque = trap_R_RegisterShader( "playerBrightOutlineOp10");

//For Double Domination:
	if ( cgs.gametype == GT_DOUBLE_D ) {
		cgs.media.ddPointSkinA[TEAM_RED] = trap_R_RegisterShaderNoMip( "icons/icona_red" );
                cgs.media.ddPointSkinA[TEAM_BLUE] = trap_R_RegisterShaderNoMip( "icons/icona_blue" );
                cgs.media.ddPointSkinA[TEAM_FREE] = trap_R_RegisterShaderNoMip( "icons/icona_white" );
                cgs.media.ddPointSkinA[TEAM_NONE] = trap_R_RegisterShaderNoMip( "icons/noammo" );
                
                cgs.media.ddPointSkinB[TEAM_RED] = trap_R_RegisterShaderNoMip( "icons/iconb_red" );
                cgs.media.ddPointSkinB[TEAM_BLUE] = trap_R_RegisterShaderNoMip( "icons/iconb_blue" );
                cgs.media.ddPointSkinB[TEAM_FREE] = trap_R_RegisterShaderNoMip( "icons/iconb_white" );
                cgs.media.ddPointSkinB[TEAM_NONE] = trap_R_RegisterShaderNoMip( "icons/noammo" );
	}

	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION || cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER || cg_buildScript.integer ) {
		cgs.media.redFlagModel = trap_R_RegisterModel( "models/flags/r_flag.md3" );
		cgs.media.blueFlagModel = trap_R_RegisterModel( "models/flags/b_flag.md3" );
                cgs.media.neutralFlagModel = trap_R_RegisterModel( "models/flags/n_flag.md3" );
		cgs.media.redFlagShader[0] = trap_R_RegisterShader( "icons/iconf_red1" );
		cgs.media.redFlagShader[1] = trap_R_RegisterShader( "icons/iconf_red2" );
		cgs.media.redFlagShader[2] = trap_R_RegisterShader( "icons/iconf_red3" );
		cgs.media.blueFlagShader[0] = trap_R_RegisterShader( "icons/iconf_blu1" );
		cgs.media.blueFlagShader[1] = trap_R_RegisterShader( "icons/iconf_blu2" );
		cgs.media.blueFlagShader[2] = trap_R_RegisterShader( "icons/iconf_blu3" );
		cgs.media.flagPoleModel = trap_R_RegisterModel( "models/flag2/flagpole.md3" );
		cgs.media.flagFlapModel = trap_R_RegisterModel( "models/flag2/flagflap3.md3" );

		cgs.media.redFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/red.skin" );
		cgs.media.blueFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/blue.skin" );
		cgs.media.neutralFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/white.skin" );

		cgs.media.redFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/red_base.md3" );
		cgs.media.blueFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/blue_base.md3" );
		cgs.media.neutralFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/ntrl_base.md3" );
	}

	if ( cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {
		cgs.media.neutralFlagModel = trap_R_RegisterModel( "models/flags/n_flag.md3" );
		cgs.media.flagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral1" );
		cgs.media.flagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.flagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.flagShader[3] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral3" );
	}

	if ( cgs.gametype == GT_OBELISK || cg_buildScript.integer ) {
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");
		cgs.media.overloadBaseModel = trap_R_RegisterModel( "models/powerups/overload_base.md3" );
		cgs.media.overloadTargetModel = trap_R_RegisterModel( "models/powerups/overload_target.md3" );
		cgs.media.overloadLightsModel = trap_R_RegisterModel( "models/powerups/overload_lights.md3" );
		cgs.media.overloadEnergyModel = trap_R_RegisterModel( "models/powerups/overload_energy.md3" );
	}

	if ( cgs.gametype == GT_HARVESTER || cgs.gametype == GT_TREASURE_HUNTER || cg_buildScript.integer ) {
		cgs.media.harvesterModel = trap_R_RegisterModel( "models/powerups/harvester/harvester.md3" );
		cgs.media.harvesterRedSkin = trap_R_RegisterSkin( "models/powerups/harvester/red.skin" );
		cgs.media.harvesterBlueSkin = trap_R_RegisterSkin( "models/powerups/harvester/blue.skin" );
		cgs.media.harvesterNeutralModel = trap_R_RegisterModel( "models/powerups/obelisk/obelisk.md3" );
	}

	//cgs.media.redKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikred" );
	cgs.media.dustPuffShader = trap_R_RegisterShader("hasteSmokePuff" );

	if ( ( ( cgs.gametype >= GT_TEAM ) && ( cgs.ffa_gt != 1 ) ) ||
		cg_buildScript.integer ) {

		//cgs.media.friendShader = trap_R_RegisterShader( "sprites/foe" );
		//cgs.media.friendShaderThroughWalls = trap_R_RegisterShader( "sprites/friendthroughwall" );

		cgs.media.friendColorShaders[0] = trap_R_RegisterShader("sprites/friendBlue");
		cgs.media.friendColorShaders[1] = trap_R_RegisterShader("sprites/friendCyan");
		cgs.media.friendColorShaders[2] = trap_R_RegisterShader("sprites/friendGreen");
		cgs.media.friendColorShaders[3] = trap_R_RegisterShader("sprites/friendYellow");
		cgs.media.friendColorShaders[4] = trap_R_RegisterShader("sprites/friendOrange");
		cgs.media.friendColorShaders[5] = trap_R_RegisterShader("sprites/friendRed");

		cgs.media.friendThroughWallColorShaders[0] = trap_R_RegisterShaderNoMip("sprites/friendIBlue.tga");
		cgs.media.friendThroughWallColorShaders[1] = trap_R_RegisterShaderNoMip("sprites/friendICyan.tga");
		cgs.media.friendThroughWallColorShaders[2] = trap_R_RegisterShaderNoMip("sprites/friendIGreen.tga");
		cgs.media.friendThroughWallColorShaders[3] = trap_R_RegisterShaderNoMip("sprites/friendIYellow.tga");
		cgs.media.friendThroughWallColorShaders[4] = trap_R_RegisterShaderNoMip("sprites/friendIOrange.tga");
		cgs.media.friendThroughWallColorShaders[5] = trap_R_RegisterShaderNoMip("sprites/friendIRed.tga");

		cgs.media.friendFlagShaderNeutral = trap_R_RegisterShaderNoMip("sprites/flagINeutral.tga");
		cgs.media.friendFlagShaderRed = trap_R_RegisterShaderNoMip("sprites/flagIRed.tga");
		cgs.media.friendFlagShaderBlue = trap_R_RegisterShaderNoMip("sprites/flagIBlue.tga");

		cgs.media.radarShader = trap_R_RegisterShader("radar");
		cgs.media.radarDotShader = trap_R_RegisterShader("radardot");

		cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag" );
		//cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" ); - moved outside, used by accuracy
		//cgs.media.blueKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikblu" );
	}


	cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" );


	cgs.media.armorModel = trap_R_RegisterModel( "models/powerups/armor/armor_yel.md3" );
	cgs.media.armorIcon  = trap_R_RegisterShaderNoMip( "icons/iconr_yellow" );
	cgs.media.healthIcon  = trap_R_RegisterShaderNoMip( "icons/iconh_yellow" );

	cgs.media.armorIconBlue  = trap_R_RegisterShaderNoMip( "armorIconBlue" );
	cgs.media.healthIconBlue  = trap_R_RegisterShaderNoMip( "healthIconBlue" );
	cgs.media.armorIconRed  = trap_R_RegisterShaderNoMip( "armorIconRed" );
	cgs.media.healthIconRed  = trap_R_RegisterShaderNoMip( "healthIconRed" );

	cgs.media.machinegunBrassModel = trap_R_RegisterModel( "models/weapons2/shells/m_shell.md3" );
	cgs.media.shotgunBrassModel = trap_R_RegisterModel( "models/weapons2/shells/s_shell.md3" );

	cgs.media.gibAbdomen = trap_R_RegisterModel( "models/gibs/abdomen.md3" );
	cgs.media.gibArm = trap_R_RegisterModel( "models/gibs/arm.md3" );
	cgs.media.gibChest = trap_R_RegisterModel( "models/gibs/chest.md3" );
	cgs.media.gibFist = trap_R_RegisterModel( "models/gibs/fist.md3" );
	cgs.media.gibFoot = trap_R_RegisterModel( "models/gibs/foot.md3" );
	cgs.media.gibForearm = trap_R_RegisterModel( "models/gibs/forearm.md3" );
	cgs.media.gibIntestine = trap_R_RegisterModel( "models/gibs/intestine.md3" );
	cgs.media.gibLeg = trap_R_RegisterModel( "models/gibs/leg.md3" );
	cgs.media.gibSkull = trap_R_RegisterModel( "models/gibs/skull.md3" );
	cgs.media.gibBrain = trap_R_RegisterModel( "models/gibs/brain.md3" );

	cgs.media.smoke2 = trap_R_RegisterModel( "models/weapons2/shells/s_shell.md3" );

	cgs.media.balloonShader = trap_R_RegisterShader( "sprites/typing" );

	cgs.media.bloodExplosionShader = trap_R_RegisterShader( "bloodExplosion" );

	cgs.media.bulletFlashModel = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.ringFlashModel = trap_R_RegisterModel("models/weaphits/ring02.md3");
	cgs.media.dishFlashModel = trap_R_RegisterModel("models/weaphits/boom01.md3");
#ifdef MISSIONPACK
	cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/powerups/pop.md3" );
#else
	cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/misc/telep.md3" );
	cgs.media.teleportEffectShader = trap_R_RegisterShader( "teleportEffect" );
#endif
	cgs.media.kamikazeEffectModel = trap_R_RegisterModel( "models/weaphits/kamboom2.md3" );
	cgs.media.kamikazeShockWave = trap_R_RegisterModel( "models/weaphits/kamwave.md3" );
	cgs.media.kamikazeHeadModel = trap_R_RegisterModel( "models/powerups/kamikazi.md3" );
	//cgs.media.kamikazeHeadTrail = trap_R_RegisterModel( "models/powerups/trailtest.md3" );
	cgs.media.guardPowerupModel = trap_R_RegisterModel( "models/powerups/guard_player.md3" );
	cgs.media.scoutPowerupModel = trap_R_RegisterModel( "models/powerups/scout_player.md3" );
	cgs.media.doublerPowerupModel = trap_R_RegisterModel( "models/powerups/doubler_player.md3" );
	cgs.media.ammoRegenPowerupModel = trap_R_RegisterModel( "models/powerups/ammo_player.md3" );
	cgs.media.invulnerabilityImpactModel = trap_R_RegisterModel( "models/powerups/shield/impact.md3" );
	cgs.media.invulnerabilityJuicedModel = trap_R_RegisterModel( "models/powerups/shield/juicer.md3" );
	cgs.media.medkitUsageModel = trap_R_RegisterModel( "models/powerups/medkit_use.md3" );
	cgs.media.heartShader = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/selectedhealth.tga" );


	cgs.media.invulnerabilityPowerupModel = trap_R_RegisterModel( "models/powerups/shield/shield.md3" );
	cgs.media.medalImpressive = trap_R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent = trap_R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalGauntlet = trap_R_RegisterShaderNoMip( "medal_gauntlet" );
	cgs.media.medalDefend = trap_R_RegisterShaderNoMip( "medal_defend" );
	cgs.media.medalAssist = trap_R_RegisterShaderNoMip( "medal_assist" );
	cgs.media.medalCapture = trap_R_RegisterShaderNoMip( "medal_capture" );

	cgs.media.eaward_medals[EAWARD_FRAGS] = trap_R_RegisterShaderNoMip( "medal_frags" );
	cgs.media.eaward_medals[EAWARD_ACCURACY] = trap_R_RegisterShaderNoMip( "medal_accuracy" );
	cgs.media.eaward_medals[EAWARD_TELEFRAG] = trap_R_RegisterShaderNoMip( "medal_telefrag" );
	cgs.media.eaward_medals[EAWARD_TELEMISSILE_FRAG] = trap_R_RegisterShaderNoMip( "medal_telemissilefrag" );
	cgs.media.eaward_medals[EAWARD_ROCKETSNIPER] = trap_R_RegisterShaderNoMip( "medal_rocketsniper" );
	cgs.media.eaward_medals[EAWARD_FULLSG] = trap_R_RegisterShaderNoMip( "medal_fullsg" );
	cgs.media.eaward_medals[EAWARD_IMMORTALITY] = trap_R_RegisterShaderNoMip( "medal_immortality" );
	cgs.media.eaward_medals[EAWARD_AIRROCKET] = trap_R_RegisterShaderNoMip( "medal_airrocket" );
	cgs.media.eaward_medals[EAWARD_AIRGRENADE] = trap_R_RegisterShaderNoMip( "medal_airgrenade" );
	cgs.media.eaward_medals[EAWARD_ROCKETRAIL] = trap_R_RegisterShaderNoMip( "medal_rocketrail" );
	cgs.media.eaward_medals[EAWARD_LGRAIL] = trap_R_RegisterShaderNoMip( "medal_lgrail" );
	cgs.media.eaward_medals[EAWARD_RAILTWO] = trap_R_RegisterShaderNoMip( "medal_railtwo" );
	cgs.media.eaward_medals[EAWARD_DEADHAND] = trap_R_RegisterShaderNoMip( "medal_grave" );
	cgs.media.eaward_medals[EAWARD_TWITCHRAIL] = trap_R_RegisterShaderNoMip( "medal_twitchrail" );
	cgs.media.eaward_medals[EAWARD_SHOWSTOPPER] = trap_R_RegisterShaderNoMip( "medal_showstopper" );
	cgs.media.eaward_medals[EAWARD_AMBUSH] = trap_R_RegisterShaderNoMip( "medal_ambush" );
	cgs.media.eaward_medals[EAWARD_KAMIKAZE] = trap_R_RegisterShaderNoMip( "medal_kamikaze" );
	cgs.media.eaward_medals[EAWARD_STRONGMAN] = trap_R_RegisterShaderNoMip( "medal_strongman" );
	cgs.media.eaward_medals[EAWARD_HERO] = trap_R_RegisterShaderNoMip( "medal_hero" );
	cgs.media.eaward_medals[EAWARD_BUTCHER] = trap_R_RegisterShaderNoMip( "medal_butcher" );
	cgs.media.eaward_medals[EAWARD_KILLINGSPREE] = trap_R_RegisterShaderNoMip( "medal_killingspree" );
	cgs.media.eaward_medals[EAWARD_RAMPAGE] = trap_R_RegisterShaderNoMip( "medal_rampage" );
	cgs.media.eaward_medals[EAWARD_MASSACRE] = trap_R_RegisterShaderNoMip( "medal_massacre" );
	cgs.media.eaward_medals[EAWARD_UNSTOPPABLE] = trap_R_RegisterShaderNoMip( "medal_unstoppable" );
	cgs.media.eaward_medals[EAWARD_GRIMREAPER] = trap_R_RegisterShaderNoMip( "medal_grimreaper" );
	cgs.media.eaward_medals[EAWARD_REVENGE] = trap_R_RegisterShaderNoMip( "medal_revenge" );
	cgs.media.eaward_medals[EAWARD_BERSERKER] = trap_R_RegisterShaderNoMip( "medal_berserker" );
	cgs.media.eaward_medals[EAWARD_VAPORIZED] = trap_R_RegisterShaderNoMip( "medal_vaporized" );

	switch (cg_ratStatusbar.integer) {
		case 3:
			CG_Ratstatusbar3RegisterShaders();
			break;
		case 4:
		case 5:
			CG_Ratstatusbar4RegisterShaders();
			break;
	}

	cgs.media.weaponSelectShaderTech = trap_R_RegisterShaderNoMip("weapselectTech");
	cgs.media.weaponSelectShaderTechBorder = trap_R_RegisterShaderNoMip("weapselectTechBorder");

	cgs.media.weaponSelectShaderCircle = trap_R_RegisterShaderNoMip("weapselectTechCircle");
	cgs.media.weaponSelectShaderCircleGlow = trap_R_RegisterShaderNoMip("weapselectTechCircleGlow");
	cgs.media.noammoCircleShader = trap_R_RegisterShaderNoMip("noammoCircle");

	cgs.media.powerupFrameShader = trap_R_RegisterShader("powerupFrame");
	cgs.media.bottomFPSShaderDecor = trap_R_RegisterShader("bottomFPSDecorDecor");
	cgs.media.bottomFPSShaderColor = trap_R_RegisterShader("bottomFPSDecorColor");

	switch (cg_hudDamageIndicator.integer) {
		case 0:
			break;
		case 2:
			cgs.media.damageIndicatorCenter = trap_R_RegisterShaderNoMip("damageIndicator2");
			break;
		case 3:
			cgs.media.viewBloodShader = trap_R_RegisterShader( "viewBloodBlend" );
		default:
			cgs.media.damageIndicatorBottom = trap_R_RegisterShaderNoMip("damageIndicatorBottom");
			cgs.media.damageIndicatorTop = trap_R_RegisterShaderNoMip("damageIndicatorTop");
			cgs.media.damageIndicatorTop = trap_R_RegisterShaderNoMip("damageIndicatorTop");
			cgs.media.damageIndicatorRight = trap_R_RegisterShaderNoMip("damageIndicatorRight");
			cgs.media.damageIndicatorLeft = trap_R_RegisterShaderNoMip("damageIndicatorLeft");
			break;
	}

	if (cg_drawZoomScope.integer) {
		cgs.media.zoomScopeMGShader = trap_R_RegisterShader("zoomScopeMG");
		cgs.media.zoomScopeRGShader = trap_R_RegisterShader("zoomScopeRG");
	}

	// LEILEI SHADERS
	cgs.media.lsmkShader1 = trap_R_RegisterShader("leismoke1" );
	cgs.media.lsmkShader2 = trap_R_RegisterShader("leismoke2" );
	cgs.media.lsmkShader3 = trap_R_RegisterShader("leismoke3" );
	cgs.media.lsmkShader4 = trap_R_RegisterShader("leismoke4" );

	cgs.media.lsplShader = trap_R_RegisterShader("leisplash" );
	cgs.media.lspkShader1 = trap_R_RegisterShader("leispark" );
	cgs.media.lspkShader2 = trap_R_RegisterShader("leispark2" );
	cgs.media.lbumShader1 = trap_R_RegisterShader("leiboom1" );
	cgs.media.lfblShader1 = trap_R_RegisterShader("leifball" );

	cgs.media.lbldShader1 = trap_R_RegisterShader("leiblood1" );	
	cgs.media.lbldShader2 = trap_R_RegisterShader("leiblood2" );	// this is a mark, by the way

	// New Bullet Marks
	cgs.media.lmarkmetal1 = trap_R_RegisterShader("leimetalmark1" );	
	cgs.media.lmarkmetal2 = trap_R_RegisterShader("leimetalmark2" );	
	cgs.media.lmarkmetal3 = trap_R_RegisterShader("leimetalmark3" );	
	cgs.media.lmarkmetal4 = trap_R_RegisterShader("leimetalmark4" );	
	cgs.media.lmarkbullet1 = trap_R_RegisterShader("leibulletmark1" );	
	cgs.media.lmarkbullet2 = trap_R_RegisterShader("leibulletmark2" );	
	cgs.media.lmarkbullet3 = trap_R_RegisterShader("leibulletmark3" );	
	cgs.media.lmarkbullet4 = trap_R_RegisterShader("leibulletmark4" );	


	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
		}
	}

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader = trap_R_RegisterShader( "gfx/damage/burn_med_mrk" );
	cgs.media.holeMarkShader = trap_R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.energyMarkShader = trap_R_RegisterShader( "gfx/damage/plasma_mrk" );
	cgs.media.shadowMarkShader = trap_R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader = trap_R_RegisterShader( "wake" );
	cgs.media.bloodMarkShader = trap_R_RegisterShader( "bloodMark" );

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_MODELS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel( modelName );
	}

#ifdef MISSIONPACK
	// new stuff
	cgs.media.patrolShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/patrol.tga");
	cgs.media.assaultShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/assault.tga");
	cgs.media.campShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/camp.tga");
	cgs.media.followShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/follow.tga");
	cgs.media.defendShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/defend.tga");
	cgs.media.teamLeaderShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/team_leader.tga");
	cgs.media.retrieveShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/retrieve.tga");
	cgs.media.escortShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/escort.tga");
        cgs.media.deathShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/death.tga");

	cgs.media.cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor = trap_R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	cgs.media.selectCursor = trap_R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	cgs.media.flagShaders[0] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_in_base.tga");
	cgs.media.flagShaders[1] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_capture.tga");
	cgs.media.flagShaders[2] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_missing.tga");

	trap_R_RegisterModel( "models/players/sergei/lower.md3" );
	trap_R_RegisterModel( "models/players/sergei/upper.md3" );
	trap_R_RegisterModel( "models/players/sergei/head.md3" );

	trap_R_RegisterModel( "models/players/kyonshi/lower.md3" );
	trap_R_RegisterModel( "models/players/kyonshi/upper.md3" );
	trap_R_RegisterModel( "models/players/kyonshi/head.md3" );

#endif
	CG_ClearParticles ();
/*
	for (i=1; i<MAX_PARTICLES_AREAS; i++)
	{
		{
			int rval;

			rval = CG_NewParticleArea ( CS_PARTICLES + i);
			if (!rval)
				break;
		}
	}
*/
}



/*																																			
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString(void) {
	int i;
	cg.spectatorList[0] = 0;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}
	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen) {
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
}


/*																																			
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients( void ) {
	int		i;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum);

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		if (cg.clientNum == i) {
			continue;
		}

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0]) {
			continue;
		}
		CG_LoadingClient( i );
		CG_NewClientInfo( i );
	}
	CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void ) {
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	if ( *cg_music.string && Q_stricmp( cg_music.string, "none" ) ) {
		s = (char *)cg_music.string;
	} else {
		s = (char *)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
        }
}
#ifdef MISSIONPACK
char *CG_GetMenuBuffer(const char *filename) {
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return NULL;
	}
	if ( len >= MAX_MENUFILE ) {
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", filename, len, MAX_MENUFILE ) );
		trap_FS_FCloseFile( f );
		return NULL;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	return buf;
}

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
qboolean CG_Asset_Parse(int handle) {
	pc_token_t token;
	const char *tempStr;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}
    
	while ( 1 ) {
		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.textFont);
			continue;
		}

		// smallFont
		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.smallFont);
			continue;
		}

		// font
		if (Q_stricmp(token.string, "bigfont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.bigFont);
			continue;
		}

		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) {
			if (!PC_String_Parse(handle, &cgDC.Assets.cursorStr)) {
				return qfalse;
			}
			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &cgDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &cgDC.Assets.shadowColor)) {
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			continue;
		}
	}
	return qfalse; // bk001204 - why not?
}

void CG_ParseMenu(const char *menuFile) {
	pc_token_t token;
	int handle;

	handle = trap_PC_LoadSource(menuFile);
	if (!handle)
		handle = trap_PC_LoadSource("ui/testhud.menu");
	if (!handle)
		return;

	while ( 1 ) {
		if (!trap_PC_ReadToken( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (CG_Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}


		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle);
		}
	}
	trap_PC_FreeSource(handle);
}

qboolean CG_Load_Menu(char **p) {
	char *token;

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		token = COM_ParseExt(p, qtrue);
    
		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		CG_ParseMenu(token); 
	}
	return qfalse;
}



void CG_LoadMenus(const char *menuFile) {
	char	*token;
	char *p;
	int	len, start;
	fileHandle_t	f;
	static char buf[MAX_MENUDEFFILE];

	start = trap_Milliseconds();

	len = trap_FS_FOpenFile( menuFile, &f, FS_READ );
	if ( !f ) {
		trap_Error( va( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile ) );
		len = trap_FS_FOpenFile( "ui/hud.txt", &f, FS_READ );
		if (!f) {
			trap_Error( va( S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!\n") );
		}
	}

	if ( len >= MAX_MENUDEFFILE ) {
		trap_Error( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", menuFile, len, MAX_MENUDEFFILE ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );
	
	COM_Compress(buf);

	Menu_Reset();

	p = buf;

	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}') {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( Q_stricmp( token, "}" ) == 0 ) {
			break;
		}

		if (Q_stricmp(token, "loadmenu") == 0) {
			if (CG_Load_Menu(&p)) {
				continue;
			} else {
				break;
			}
		}
	}

	Com_Printf("UI menu load time = %d milli seconds\n", trap_Milliseconds() - start);

}



static qboolean CG_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
	return qfalse;
}


static int CG_FeederCount(float feederID) {
	int i, count;
	count = 0;
	if (feederID == FEEDER_REDTEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_RED) {
				count++;
			}
		}
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_BLUE) {
				count++;
			}
		}
	} else if (feederID == FEEDER_SCOREBOARD) {
		return cg.numScores;
	}
	return count;
}


void CG_SetScoreSelection(void *p) {
	menuDef_t *menu = (menuDef_t*)p;
	playerState_t *ps = &cg.snap->ps;
	int i, red, blue;
	red = blue = 0;
	for (i = 0; i < cg.numScores; i++) {
		if (cg.scores[i].team == TEAM_RED) {
			red++;
		} else if (cg.scores[i].team == TEAM_BLUE) {
			blue++;
		}
		if (ps->clientNum == cg.scores[i].client) {
			cg.selectedScore = i;
		}
	}

	if (menu == NULL) {
		// just interested in setting the selected score
		return;
	}

	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1) {
		int feeder = FEEDER_REDTEAM_LIST;
		i = red;
		if (cg.scores[cg.selectedScore].team == TEAM_BLUE) {
			feeder = FEEDER_BLUETEAM_LIST;
			i = blue;
		}
		Menu_SetFeederSelection(menu, feeder, i, NULL);
	} else {
		Menu_SetFeederSelection(menu, FEEDER_SCOREBOARD, cg.selectedScore, NULL);
	}
}

// FIXME: might need to cache this info
static clientInfo_t * CG_InfoFromScoreIndex(int index, int team, int *scoreIndex) {
	int i, count;
	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1) {
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (count == index) {
					*scoreIndex = i;
					return &cgs.clientinfo[cg.scores[i].client];
				}
				count++;
			}
		}
	}
	*scoreIndex = index;
	return &cgs.clientinfo[ cg.scores[index].client ];
}

static const char *CG_FeederItemText(float feederID, int index, int column, qhandle_t *handle) {
	gitem_t *item;
	int scoreIndex = 0;
	clientInfo_t *info = NULL;
	int team = -1;
	score_t *sp = NULL;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	if (info && info->infoValid) {
		switch (column) {
			case 0:
				if ( info->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
					item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
					*handle = cg_items[ ITEM_INDEX(item) ].icon;
				} else if ( info->powerups & ( 1 << PW_REDFLAG ) ) {
					item = BG_FindItemForPowerup( PW_REDFLAG );
					*handle = cg_items[ ITEM_INDEX(item) ].icon;
				} else if ( info->powerups & ( 1 << PW_BLUEFLAG ) ) {
					item = BG_FindItemForPowerup( PW_BLUEFLAG );
					*handle = cg_items[ ITEM_INDEX(item) ].icon;
				} else {
					if ( info->botSkill > 0 && info->botSkill <= 5 ) {
						*handle = cgs.media.botSkillShaders[ info->botSkill - 1 ];
					} else if ( info->handicap < 100 ) {
					return va("%i", info->handicap );
					}
				}
			break;
			case 1:
				if (team == -1) {
					return "";
				} else if (info->isDead) {
                                        *handle = cgs.media.deathShader;
                                } else {
					*handle = CG_StatusHandle(info->teamTask);
				}
		  break;
			case 2:
				if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
					return "Ready";
				}
				if (team == -1) {
					if (cgs.gametype == GT_TOURNAMENT) {
						return va("%i/%i", info->wins, info->losses);
					} else if (info->infoValid && info->team == TEAM_SPECTATOR ) {
						return "Spectator";
					} else {
						return "";
					}
				} else {
					if (info->teamLeader) {
						return "Leader";
					}
				}
			break;
			case 3:
				return info->name;
			break;
			case 4:
				return va("%i", info->score);
			break;
			case 5:
				return va("%4i", sp->time);
			break;
			case 6:
				if ( sp->ping == -1 ) {
					return "connecting";
				} 
				return va("%4i", sp->ping);
			break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage(float feederID, int index) {
	return 0;
}

static void CG_FeederSelection(float feederID, int index) {
	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1) {
		int i, count;
		int team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_RED : TEAM_BLUE;
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (index == count) {
					cg.selectedScore = i;
				}
				count++;
			}
		}
	} else {
		cg.selectedScore = index;
	}
}
#endif

static float CG_Cvar_Get(const char *cvar) {
	char buff[128];
	memset(buff, 0, sizeof(buff));
	trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return atof(buff);
}

#ifdef MISSIONPACK
void CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style) {
	CG_Text_Paint(x, y, scale, color, text, 0, limit, style);
}

static int CG_OwnerDrawWidth(int ownerDraw, float scale) {
	switch (ownerDraw) {
	  case CG_GAME_TYPE:
			return CG_Text_Width(CG_GameTypeString(), scale, 0);
	  case CG_GAME_STATUS:
			return CG_Text_Width(CG_GetGameStatusText(), scale, 0);
			break;
	  case CG_KILLER:
			return CG_Text_Width(CG_GetKillerText(), scale, 0);
			break;
	  case CG_RED_NAME:
			return CG_Text_Width(cg_redTeamName.string, scale, 0);
			break;
	  case CG_BLUE_NAME:
			return CG_Text_Width(cg_blueTeamName.string, scale, 0);
			break;


	}
	return 0;
}

static int CG_PlayCinematic(const char *name, float x, float y, float w, float h) {
  return trap_CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

static void CG_StopCinematic(int handle) {
  trap_CIN_StopCinematic(handle);
}

static void CG_DrawCinematic(int handle, float x, float y, float w, float h) {
  trap_CIN_SetExtents(handle, x, y, w, h);
  trap_CIN_DrawCinematic(handle);
}

static void CG_RunCinematicFrame(int handle) {
  trap_CIN_RunCinematic(handle);
}

/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu( void ) {
	char buff[1024];
	const char *hudSet;

	cgDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	cgDC.setColor = &trap_R_SetColor;
	cgDC.drawHandlePic = &CG_DrawPic;
	cgDC.drawStretchPic = &trap_R_DrawStretchPic;
	cgDC.drawText = &CG_Text_Paint;
	cgDC.textWidth = &CG_Text_Width;
	cgDC.textHeight = &CG_Text_Height;
	cgDC.registerModel = &trap_R_RegisterModel;
	cgDC.modelBounds = &trap_R_ModelBounds;
	cgDC.fillRect = &CG_FillRect;
	cgDC.drawRect = &CG_DrawRect;   
	cgDC.drawSides = &CG_DrawSides;
	cgDC.drawTopBottom = &CG_DrawTopBottom;
	cgDC.clearScene = &trap_R_ClearScene;
	cgDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	cgDC.renderScene = &trap_R_RenderScene;
	cgDC.registerFont = &trap_R_RegisterFont;
	cgDC.ownerDrawItem = &CG_OwnerDraw;
	cgDC.getValue = &CG_GetValue;
	cgDC.ownerDrawVisible = &CG_OwnerDrawVisible;
	cgDC.runScript = &CG_RunMenuScript;
	cgDC.getTeamColor = &CG_GetTeamColor;
	cgDC.setCVar = trap_Cvar_Set;
	cgDC.getCVarString = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue = CG_Cvar_Get;
	cgDC.drawTextWithCursor = &CG_Text_PaintWithCursor;
	//cgDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	//cgDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount = &CG_FeederCount;
	cgDC.feederItemImage = &CG_FeederItemImage;
	cgDC.feederItemText = &CG_FeederItemText;
	cgDC.feederSelection = &CG_FeederSelection;
	//cgDC.setBinding = &trap_Key_SetBinding;
	//cgDC.getBindingBuf = &trap_Key_GetBindingBuf;
	//cgDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	//cgDC.executeText = &trap_Cmd_ExecuteText;
	cgDC.Error = &Com_Error; 
	cgDC.Print = &Com_Printf; 
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	//cgDC.Pause = &CG_Pause;
	cgDC.registerSound = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic = &CG_PlayCinematic;
	cgDC.stopCinematic = &CG_StopCinematic;
	cgDC.drawCinematic = &CG_DrawCinematic;
	cgDC.runCinematicFrame = &CG_RunCinematicFrame;
	
	Init_Display(&cgDC);

	Menu_Reset();
	
	trap_Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
	hudSet = buff;
	if (hudSet[0] == '\0') {
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus(hudSet);
}

void CG_AssetCache( void ) {
	//if (Assets.textFont == NULL) {
	//  trap_R_RegisterFont("fonts/arial.ttf", 72, &Assets.textFont);
	//}
	//Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	cgDC.Assets.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
	cgDC.Assets.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	cgDC.Assets.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	cgDC.Assets.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	cgDC.Assets.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	cgDC.Assets.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	cgDC.Assets.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	cgDC.Assets.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
	cgDC.Assets.scrollBar = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	cgDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	cgDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	cgDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	cgDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	cgDC.Assets.sliderBar = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	cgDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
}
#endif
/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum ) {
	const char	*s;

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof(cg_entities) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );
	memset( cg_items, 0, sizeof(cg_items) );

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader		= trap_R_RegisterShader( "gfx/2d/bigchars" );
	cgs.media.charsetShader16	= trap_R_RegisterShader( "gfx/2d/bigchars16" );
	cgs.media.charsetShader32	= trap_R_RegisterShader( "gfx/2d/bigchars32" );
	cgs.media.charsetShader64	= trap_R_RegisterShader( "gfx/2d/bigchars64" );

	cgs.media.whiteShader		= trap_R_RegisterShader( "white" );
	cgs.media.charsetProp		= trap_R_RegisterShaderNoMip( "menu/art/font1_prop.tga" );
	cgs.media.charsetPropGlow	= trap_R_RegisterShaderNoMip( "menu/art/font1_prop_glo.tga" );
	cgs.media.charsetPropB		= trap_R_RegisterShaderNoMip( "menu/art/font2_prop.tga" );

	CG_RegisterCvars();

	CG_RatInitDefaults();


	CG_InitConsoleCommands();

	cg.weaponSelect = WP_MACHINEGUN;

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if ( strcmp( s, GAME_VERSION ) ) {
		CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
	}

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );
    
	CG_ParseServerinfo();

	// load the new map
	CG_LoadingString( "collision map" );

	trap_CM_LoadMap( cgs.mapname );

#ifdef MISSIONPACK
	String_Init();
#endif

	cg.loading = qtrue;		// force players to load instead of defer

	CG_LoadingString( "sounds" );

	CG_RegisterSounds();

	CG_LoadingString( "graphics" );

	CG_RegisterGraphics();

	CG_LoadingString( "clients" );

	CG_RegisterClients();		// if low on memory, some clients will be deferred

#ifdef MISSIONPACK
	CG_AssetCache();
	CG_LoadHudMenu();      // load new hud stuff
#endif

	cg.loading = qfalse;	// future players will be deferred

	CG_InitPMissilles();
	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString( "" );

#ifdef MISSIONPACK
	CG_InitTeamChat();
#endif

	CG_ShaderStateChanged();

	//Init challenge system
	challenges_init();

	addChallenge(GENERAL_TEST);

	trap_S_ClearLoopingSounds( qtrue );

	CG_LoadForcedSounds();
	CG_ParseForcedColors();

	CG_RatRemapShaders();
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
	challenges_save();
}


/*
==================
CG_EventHandling
==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/
#ifndef MISSIONPACK
void CG_EventHandling(int type) {
}



void CG_KeyEvent(int key, qboolean down) {
}

void CG_MouseEvent(int x, int y) {
}
#endif

//unlagged - attack prediction #3
// moved from g_weapon.c
/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating 
into a wall.
======================
*/
void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int		i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
			v[i] = (int)v[i];
		} else {
			v[i] = (int)v[i] + 1;
		}
	}
}
//unlagged - attack prediction #3

static qboolean do_vid_restart = qfalse;

void CG_FairCvars() {
    qboolean vid_restart_required = qfalse;
    char rendererinfos[128];

    if(cgs.gametype == GT_SINGLE_PLAYER) {
        trap_Cvar_VariableStringBuffer("r_vertexlight",rendererinfos,sizeof(rendererinfos) );
        if(cg_autovertex.integer && atoi( rendererinfos ) == 0 ) {
            trap_Cvar_Set("r_vertexlight","1");
            vid_restart_required = qtrue;
        }
        return; //Don't do anything in single player
    }

    if(cgs.videoflags & VF_LOCK_CVARS_BASIC) {
        //Lock basic cvars.
        trap_Cvar_VariableStringBuffer("r_subdivisions",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 80 ) {
            trap_Cvar_Set("r_subdivisions","80");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("cg_shadows",rendererinfos,sizeof(rendererinfos) );
        if (atoi( rendererinfos )!=0 && atoi( rendererinfos )!=1 ) {
            trap_Cvar_Set("cg_shadows","1");
        }
    }

    if(cgs.videoflags & VF_LOCK_CVARS_EXTENDED) {
        //Lock extended cvars.
        trap_Cvar_VariableStringBuffer("r_subdivisions",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 20 ) {
            trap_Cvar_Set("r_subdivisions","20");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("r_picmip",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 3 ) {
            trap_Cvar_Set("r_picmip","3");
            vid_restart_required = qtrue;
        } else if(atoi( rendererinfos ) < 0 ) {
            trap_Cvar_Set("r_picmip","0");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("r_intensity",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 2 ) {
            trap_Cvar_Set("r_intensity","2");
            vid_restart_required = qtrue;
        } else if(atoi( rendererinfos ) < 0 ) {
            trap_Cvar_Set("r_intensity","0");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("r_mapoverbrightbits",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 2 ) {
            trap_Cvar_Set("r_mapoverbrightbits","2");
            vid_restart_required = qtrue;
        } else if(atoi( rendererinfos ) < 0 ) {
            trap_Cvar_Set("r_mapoverbrightbits","0");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("r_overbrightbits",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 2 ) {
            trap_Cvar_Set("r_overbrightbits","2");
            vid_restart_required = qtrue;
        } else if(atoi( rendererinfos ) < 0 ) {
            trap_Cvar_Set("r_overbrightbits","0");
            vid_restart_required = qtrue;
        }
    } 

    if(cgs.videoflags & VF_LOCK_VERTEX) {
        trap_Cvar_VariableStringBuffer("r_vertexlight",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) != 0 ) {
            trap_Cvar_Set("r_vertexlight","0");
            vid_restart_required = qtrue;
        }
    } else if(cg_autovertex.integer){
        trap_Cvar_VariableStringBuffer("r_vertexlight",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) == 0 ) {
            trap_Cvar_Set("r_vertexlight","1");
            vid_restart_required = qtrue;
        }
    }

    if(cgs.videoflags & VF_LOCK_PICMIP) {
	    int value = 0;

	    trap_Cvar_VariableStringBuffer("r_picmip",rendererinfos,sizeof(rendererinfos) );
	    value = atoi(rendererinfos);
	    if(value != 0) {
		    trap_Cvar_Set("r_picmip","0");
		    // store picmip value
		    trap_Cvar_Set("cg_backupPicmip",va("%i", value));
		    vid_restart_required = qtrue;
	    }

	    trap_Cvar_VariableStringBuffer("r_drawFlat",rendererinfos,sizeof(rendererinfos) );
	    value = atoi(rendererinfos);
	    if(value != 0) {
		    trap_Cvar_Set("r_drawFlat","0");
		    // store picmip value
		    trap_Cvar_Set("cg_backupDrawflat",va("%i", value));
		    vid_restart_required = qtrue;
	    }

	    trap_Cvar_VariableStringBuffer("r_lightmap",rendererinfos,sizeof(rendererinfos) );
	    value = atoi(rendererinfos);
	    if(value != 0) {
		    trap_Cvar_Set("r_lightmap","0");
		    // store picmip value
		    trap_Cvar_Set("cg_backupLightmap",va("%i", value));
		    vid_restart_required = qtrue;
	    }
    } else {
	    if (cg_backupPicmip.integer > 0) {
		    // restore old value the user set for r_picmip before lock was enabled
		    trap_Cvar_Set("r_picmip",va("%i", cg_backupPicmip.integer));
		    trap_Cvar_Set("cg_backupPicmip","-1");
		    vid_restart_required = qtrue;
	    }
	    if (cg_backupDrawflat.integer > 0) {
		    // restore old value the user set for r_picmip before lock was enabled
		    trap_Cvar_Set("r_drawFlat",va("%i", cg_backupDrawflat.integer));
		    trap_Cvar_Set("cg_backupDrawflat","-1");
		    vid_restart_required = qtrue;
	    }
	    if (cg_backupLightmap.integer > 0) {
		    // restore old value the user set for r_picmip before lock was enabled
		    trap_Cvar_Set("r_lightmap",va("%i", cg_backupLightmap.integer));
		    trap_Cvar_Set("cg_backupLightmap","-1");
		    vid_restart_required = qtrue;
	    }
    }

    if(vid_restart_required && do_vid_restart)
        trap_SendConsoleCommand("vid_restart\n");

    do_vid_restart = qtrue;
}

