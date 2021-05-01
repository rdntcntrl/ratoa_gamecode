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

#include "g_local.h"

#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX(x,y) (((x) > (y)) ? (x) : (y))

level_locals_t	level;

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
	int			modificationCount;  // for tracking changes
	qboolean	trackChange;	    // track this variable, and announce if changed
  qboolean teamShader;        // track and if changed, update shader state
} cvarTable_t;

gentity_t		g_entities[MAX_GENTITIES];
gclient_t		g_clients[MAX_CLIENTS];

vmCvar_t	g_gametype;
vmCvar_t	g_dmflags;
vmCvar_t        g_videoflags;
vmCvar_t	g_elimflags;
vmCvar_t	g_voteflags;
vmCvar_t	g_fraglimit;
vmCvar_t	g_timelimit;
vmCvar_t	g_capturelimit;
vmCvar_t	g_overtime;
vmCvar_t	g_friendlyFire;
vmCvar_t	g_friendlyFireReflect;
vmCvar_t	g_friendlyFireReflectFactor;
vmCvar_t	g_password;
vmCvar_t	g_passwordVerifyConnected;
vmCvar_t	g_needpass;
vmCvar_t	g_maxclients;
vmCvar_t	g_maxGameClients;
vmCvar_t	g_dedicated;
vmCvar_t	g_speed;
vmCvar_t	g_spectatorSpeed;
vmCvar_t	g_gravity;
vmCvar_t	g_gravityModifier;
vmCvar_t	g_gravityJumppadFix;
vmCvar_t        g_damageScore;
vmCvar_t        g_damageModifier;
vmCvar_t	g_cheats;
vmCvar_t	g_knockback;
vmCvar_t	g_quadfactor;
vmCvar_t	g_forcerespawn;
vmCvar_t	g_respawntime;
vmCvar_t	g_inactivity;
vmCvar_t	g_debugMove;
vmCvar_t	g_debugDamage;
vmCvar_t	g_debugAlloc;
vmCvar_t	g_weaponRespawn;
vmCvar_t	g_overrideWeaponRespawn;
vmCvar_t	g_weaponTeamRespawn;
vmCvar_t	g_motd;
vmCvar_t        g_motdfile;
vmCvar_t        g_votemaps;
vmCvar_t        g_recommendedMapsFile;
vmCvar_t        g_votecustom;
vmCvar_t	g_synchronousClients;
vmCvar_t	g_warmup;
vmCvar_t	g_doWarmup;
vmCvar_t	g_restarted;
vmCvar_t	g_logIPs;
vmCvar_t	g_logfile;
vmCvar_t	g_logfileSync;
vmCvar_t	g_blood;
vmCvar_t	g_podiumDist;
vmCvar_t	g_podiumDrop;
vmCvar_t	g_allowVote;
vmCvar_t	g_balanceSkillThres;
vmCvar_t	g_balancePlaytime;
vmCvar_t	g_teamAutoJoin;
vmCvar_t	g_teamForceBalance;
vmCvar_t	g_teamForceQueue;
vmCvar_t	g_teamBalance;
vmCvar_t	g_teamBalanceDelay;
vmCvar_t	g_banIPs;
vmCvar_t	g_filterBan;
vmCvar_t	g_smoothClients;
vmCvar_t	pmove_fixed;
vmCvar_t	pmove_msec;
vmCvar_t        pmove_float;
vmCvar_t        g_floatPlayerPosition;
vmCvar_t	g_rankings;
vmCvar_t	g_listEntity;
vmCvar_t	g_obeliskHealth;
vmCvar_t	g_obeliskRegenPeriod;
vmCvar_t	g_obeliskRegenAmount;
vmCvar_t	g_obeliskRespawnDelay;
vmCvar_t	g_cubeTimeout;
#ifdef MISSIONPACK
vmCvar_t	g_redteam;
vmCvar_t	g_blueteam;
vmCvar_t	g_singlePlayer;
#endif
vmCvar_t	g_redclan;
vmCvar_t	g_blueclan;

vmCvar_t	g_treasureTokens;
vmCvar_t	g_treasureHideTime;
vmCvar_t	g_treasureSeekTime;
vmCvar_t	g_treasureRounds;
vmCvar_t	g_treasureTokensDestructible;
vmCvar_t	g_treasureTokenHealth;
vmCvar_t	g_treasureTokenStyle;

vmCvar_t	g_enableDust;
vmCvar_t	g_enableBreath;
vmCvar_t	g_proxMineTimeout;
vmCvar_t	g_music;
vmCvar_t        g_spawnprotect;
//Following for elimination:
vmCvar_t	g_elimination_respawn;
vmCvar_t	g_elimination_respawn_increment;
vmCvar_t	g_elimination_selfdamage;
vmCvar_t	g_elimination_startHealth;
vmCvar_t	g_elimination_startArmor;
vmCvar_t	g_elimination_bfg;
vmCvar_t	g_elimination_grapple;
vmCvar_t	g_elimination_roundtime;
vmCvar_t	g_elimination_warmup;
vmCvar_t	g_elimination_activewarmup;
vmCvar_t        g_elimination_allgametypes;
vmCvar_t        g_elimination_spawn_allgametypes;
vmCvar_t	g_elimination_machinegun;
vmCvar_t	g_elimination_shotgun;
vmCvar_t	g_elimination_grenade;
vmCvar_t	g_elimination_rocket;
vmCvar_t	g_elimination_railgun;
vmCvar_t	g_elimination_lightning;
vmCvar_t	g_elimination_plasmagun;
vmCvar_t	g_elimination_chain;
vmCvar_t	g_elimination_mine;
vmCvar_t	g_elimination_nail;

vmCvar_t        g_elimination_lockspectator;

vmCvar_t	g_swingGrapple;
vmCvar_t	g_grapple;

vmCvar_t	g_rockets;

//dmn_clowns suggestions (with my idea of implementing):
vmCvar_t	g_instantgib;
vmCvar_t	g_vampire;
vmCvar_t	g_vampireMaxHealth;

vmCvar_t	g_midAir;

//Regen
vmCvar_t	g_regen;
int	g_ffa_gt; //Are this a FFA gametype even if gametype is high?
vmCvar_t	g_lms_lives;
vmCvar_t	g_lms_mode;
vmCvar_t	g_elimination_ctf_oneway;
vmCvar_t        g_awardpushing; //The server can decide if players are awarded for pushing people in lave etc.
vmCvar_t        g_persistantpowerups; //Allow missionpack style persistant powerups?

vmCvar_t        g_catchup; //Favors the week players

vmCvar_t         g_autonextmap; //Autochange map
vmCvar_t         g_mappools; //mappools to be used for autochange

vmCvar_t        g_voteNames;
vmCvar_t        g_voteBan;
vmCvar_t        g_voteGametypes;
vmCvar_t        g_voteMinTimelimit;
vmCvar_t        g_voteMaxTimelimit;
vmCvar_t        g_voteMinFraglimit;
vmCvar_t        g_voteMaxFraglimit;
vmCvar_t        g_voteMinCapturelimit;
vmCvar_t        g_voteMaxCapturelimit;
vmCvar_t        g_voteMinBots;
vmCvar_t        g_voteMaxBots;
vmCvar_t        g_maxvotes;
vmCvar_t        g_voteRepeatLimit;

vmCvar_t        g_nextmapVote;
vmCvar_t        g_nextmapVotePlayerNumFilter;
vmCvar_t        g_nextmapVoteCmdEnabled;
vmCvar_t        g_nextmapVoteNumRecommended;
vmCvar_t        g_nextmapVoteNumGametype;
vmCvar_t        g_nextmapVoteTime;

vmCvar_t        g_humanplayers;

//used for voIP
vmCvar_t         g_redTeamClientNumbers;
vmCvar_t         g_blueTeamClientNumbers;

//unlagged - server options
vmCvar_t	g_delagHitscan;
vmCvar_t	g_delagAllowHitsAfterTele;
vmCvar_t	g_truePing;
vmCvar_t	sv_fps;
vmCvar_t        g_lagLightning; //Adds a little lag to the lightninggun to make it less powerfull
//unlagged - server options
vmCvar_t        g_teleMissiles;
vmCvar_t        g_teleMissilesMaxTeleports;
vmCvar_t        g_pushGrenades;
vmCvar_t        g_newShotgun;
vmCvar_t        g_ratPhysics;
vmCvar_t        g_rampJump;
vmCvar_t        g_additiveJump;
vmCvar_t        g_fastSwim;
vmCvar_t        g_lavaDamage;
vmCvar_t        g_slimeDamage;
vmCvar_t        g_allowTimenudge;
vmCvar_t        g_fastSwitch;
vmCvar_t        g_fastWeapons;
vmCvar_t        g_regularFootsteps;
vmCvar_t        g_smoothStairs;
vmCvar_t        g_bobup;
vmCvar_t        g_passThroughInvisWalls;
vmCvar_t        g_ambientSound; 
vmCvar_t        g_rocketSpeed; 
vmCvar_t        g_maxExtrapolatedFrames; 
vmCvar_t        g_delagMissileMaxLatency; 
vmCvar_t        g_delagMissileDebug; 
vmCvar_t        g_delagMissiles; 
vmCvar_t        g_delagMissileLimitVariance;
vmCvar_t        g_delagMissileLimitVarianceMs;
vmCvar_t        g_delagMissileBaseNudge; 
vmCvar_t        g_delagMissileLatencyMode; 
vmCvar_t        g_delagMissileCorrectFrameOffset; 
vmCvar_t        g_delagMissileNudgeOnly; 
vmCvar_t        g_delagMissileImmediateRun; 

vmCvar_t        g_teleporterPrediction; 

//vmCvar_t	g_tournamentMinSpawnDistance;
vmCvar_t	g_tournamentSpawnsystem;
vmCvar_t	g_ffaSpawnsystem;

vmCvar_t	g_ra3compat;
vmCvar_t	g_ra3maxArena;
vmCvar_t	g_ra3forceArena;
vmCvar_t	g_ra3nextForceArena;

vmCvar_t	g_enableGreenArmor;

vmCvar_t	g_readSpawnVarFiles;

vmCvar_t	g_damageThroughWalls;

vmCvar_t	g_pingEqualizer;
vmCvar_t	g_eqpingMax;
vmCvar_t	g_eqpingAuto;
vmCvar_t	g_eqpingAutoConvergeFactor;
vmCvar_t	g_eqpingAutoInterval;
vmCvar_t	g_eqpingSavedPing;
vmCvar_t	g_eqpingAutoTourney;

vmCvar_t        g_autoClans;

vmCvar_t        g_quadWhore;

vmCvar_t        g_killDropsFlag;

vmCvar_t        g_killSafety;
vmCvar_t        g_killDisable;

vmCvar_t        g_startWhenReady;
vmCvar_t        g_autoStartTime;
vmCvar_t        g_countDownHealthArmor;
vmCvar_t        g_powerupGlows;
vmCvar_t        g_screenShake;
vmCvar_t        g_allowForcedModels;
vmCvar_t        g_brightModels;
vmCvar_t        g_brightPlayerShells;
vmCvar_t        g_brightPlayerOutlines;
vmCvar_t        g_friendsWallHack;
vmCvar_t        g_friendsFlagIndicator;
vmCvar_t        g_specShowZoom;
vmCvar_t        g_itemPickup;
vmCvar_t        g_itemDrop;
vmCvar_t        g_usesRatVM;
vmCvar_t        g_usesRatEngine;
vmCvar_t        g_mixedMode;
vmCvar_t        g_broadcastClients;
vmCvar_t        g_useExtendedScores;
vmCvar_t        g_statsboard;
vmCvar_t        g_predictMissiles;
vmCvar_t        g_ratFlags;
vmCvar_t        g_maxBrightShellAlpha;
vmCvar_t        g_allowDuplicateGuid;

vmCvar_t        g_botshandicapped;
vmCvar_t        g_bots_randomcolors;

vmCvar_t        g_pingLocationAllowed;
vmCvar_t        g_pingLocationRadius;
vmCvar_t        g_pingLocationFov;

vmCvar_t        g_tauntAllowed;
vmCvar_t        g_tauntTime;
vmCvar_t        g_tauntAfterDeathTime;

// weapon config
vmCvar_t        g_mgDamage;
vmCvar_t        g_mgTeamDamage;
vmCvar_t        g_railgunDamage;
vmCvar_t        g_lgDamage;

vmCvar_t        g_railJump;

vmCvar_t        g_teamslocked;
vmCvar_t        g_autoTeamsUnlock;
vmCvar_t        g_autoTeamsLock;
vmCvar_t        g_tourneylocked;
vmCvar_t        g_specMuted;
vmCvar_t        g_tournamentMuteSpec;

vmCvar_t        g_timeoutAllowed;
vmCvar_t        g_timeinAllowed;
vmCvar_t        g_timeoutTime;
vmCvar_t        g_timeoutOvertimeStep;

vmCvar_t        g_autoFollowKiller;
vmCvar_t        g_autoFollowSwitchTime;

vmCvar_t        g_shaderremap;
vmCvar_t        g_shaderremap_flag;
vmCvar_t        g_shaderremap_flagreset;
vmCvar_t        g_shaderremap_banner;
vmCvar_t        g_shaderremap_bannerreset;

//KK-OAX
vmCvar_t        g_sprees;
vmCvar_t        g_altExcellent;
vmCvar_t        g_spreeDiv;

//Command/Chat spamming/flooding
vmCvar_t        g_floodMaxDemerits;
vmCvar_t        g_floodMinTime;
vmCvar_t        g_floodLimitUserinfo;

//Admin
vmCvar_t        g_admin;
vmCvar_t        g_adminLog;
vmCvar_t        g_adminParseSay;
vmCvar_t        g_adminNameProtect;
vmCvar_t        g_adminTempBan;
vmCvar_t        g_adminMaxBan;
vmCvar_t        g_specChat;
vmCvar_t        g_publicAdminMessages;

vmCvar_t        g_maxWarnings;
vmCvar_t        g_warningExpire;

vmCvar_t        g_minNameChangePeriod;
vmCvar_t        g_maxNameChanges;

vmCvar_t        g_allowDuplicateNames;

vmCvar_t        g_unnamedPlayersAllowed;
vmCvar_t        g_unnamedRenameAdjlist;
vmCvar_t        g_unnamedRenameNounlist;

vmCvar_t        g_timestamp_startgame;

// bk001129 - made static to avoid aliasing
static cvarTable_t		gameCvarTable[] = {
	// don't override the cheat state set by the system
	{ &g_cheats, "sv_cheats", "", 0, 0, qfalse },

	// noset vars
	{ NULL, "gamename", GAMEVERSION , CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },
	{ NULL, "gamedate", __DATE__ , CVAR_ROM, 0, qfalse  },
	{ &g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse  },
	{ NULL, "sv_mapname", "", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },

	// latched vars
	{ &g_gametype, "g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, 0, qfalse  },

	{ &g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },
	{ &g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },

	// change anytime vars
	{ &g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
        { &g_videoflags, "videoflags", "7", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
        { &g_elimflags, "elimflags", "0", CVAR_SERVERINFO, 0, qfalse  },
        { &g_voteflags, "voteflags", "0", CVAR_SERVERINFO, 0, qfalse  },
	{ &g_fraglimit, "fraglimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_capturelimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

	{ &g_overtime, "g_overtime", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

	{ &g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, qfalse  },

	{ &g_friendlyFire, "g_friendlyFire", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_friendlyFireReflect, "g_friendlyFireReflect", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_friendlyFireReflectFactor, "g_friendlyFireReflectFactor", "1", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_balanceSkillThres, "g_balanceSkillThres", "0.1", CVAR_ARCHIVE  },
	{ &g_balancePlaytime, "g_balancePlaytime", "120", CVAR_ARCHIVE  },
	{ &g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE  },
	{ &g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE  },
	{ &g_teamForceQueue, "g_teamForceQueue", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_teamBalance, "g_teamBalance", "1", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_teamBalanceDelay, "g_teamBalanceDelay", "30", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_warmup, "g_warmup", "20", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_doWarmup, "g_doWarmup", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },

	{ &g_logIPs, "g_logIPs", "0", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_logfile, "g_log", "games.log", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_logfileSync, "g_logsync", "0", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_password, "g_password", "", CVAR_USERINFO, 0, qfalse  },
	// re-verify if connected clients have the correct password upon map changes, map restarts and so on
	{ &g_passwordVerifyConnected, "g_passwordVerifyConnected", "1", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },

	{ &g_dedicated, "dedicated", "0", 0, 0, qfalse  },

	{ &g_speed, "g_speed", "320", 0, 0, qtrue  },
	{ &g_spectatorSpeed, "g_spectatorSpeed", "400", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_gravity, "g_gravity", "800", 0, 0, qtrue  },
	{ &g_gravityModifier, "g_gravityModifier", "1", 0, 0, qtrue  },
	{ &g_gravityJumppadFix, "g_gravityJumppadFix", "1", CVAR_ARCHIVE, 0, qfalse  },
        { &g_damageScore, "g_damageScore", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_damageModifier, "g_damageModifier", "0", 0, 0, qtrue },
	{ &g_knockback, "g_knockback", "1000", 0, 0, qtrue  },
	{ &g_quadfactor, "g_quadfactor", "3", 0, 0, qtrue  },
	{ &g_weaponRespawn, "g_weaponrespawn", "5", 0, 0, qtrue  },
	{ &g_overrideWeaponRespawn, "g_overrideWeaponRespawn", "0", 0, 0, qtrue  },
	{ &g_weaponTeamRespawn, "g_weaponTeamRespawn", "30", 0, 0, qtrue },
	{ &g_forcerespawn, "g_forcerespawn", "20", 0, 0, qtrue },
        { &g_respawntime, "g_respawntime", "0", CVAR_ARCHIVE, 0, qtrue },
	{ &g_inactivity, "g_inactivity", "0", 0, 0, qtrue },
	{ &g_debugMove, "g_debugMove", "0", 0, 0, qfalse },
	{ &g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse },
	{ &g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse },
	{ &g_motd, "g_motd", "", 0, 0, qfalse },
        { &g_motdfile, "g_motdfile", "motd.cfg", 0, 0, qfalse },
	{ &g_blood, "com_blood", "1", 0, 0, qfalse },

	{ &g_podiumDist, "g_podiumDist", "80", 0, 0, qfalse },
	{ &g_podiumDrop, "g_podiumDrop", "70", 0, 0, qfalse },

        //Votes start:
	{ &g_allowVote, "g_allowVote", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },
        { &g_maxvotes, "g_maxVotes", MAX_VOTE_COUNT, CVAR_ARCHIVE, 0, qfalse },
        { &g_voteRepeatLimit, "g_voteRepeatLimit", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_voteNames, "g_voteNames", "/map_restart/nextmap/map/g_gametype/kick/clientkick/g_doWarmup/timelimit/fraglimit/capturelimit/shuffle/bots/botskill/votenextmap/", CVAR_ARCHIVE, 0, qfalse }, //clientkick g_doWarmup timelimit fraglimit
        { &g_voteBan, "g_voteBan", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_voteGametypes, "g_voteGametypes", "/0/1/3/4/5/6/7/8/9/10/11/12/", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },
        { &g_voteMaxTimelimit, "g_voteMaxTimelimit", "1000", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },
        { &g_voteMinTimelimit, "g_voteMinTimelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },
        { &g_voteMaxFraglimit, "g_voteMaxFraglimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },
        { &g_voteMinFraglimit, "g_voteMinFraglimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },
        { &g_voteMaxCapturelimit, "g_voteMaxCapturelimit", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_voteMinCapturelimit, "g_voteMinCapturelimit", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_voteMaxBots, "g_voteMaxBots", "20", CVAR_ARCHIVE, 0, qfalse },
        { &g_voteMinBots, "g_voteMinBots", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_votemaps, "g_votemapsfile", "votemaps.cfg", 0, 0, qfalse },
        { &g_votecustom, "g_votecustomfile", "votecustom.cfg", 0, 0, qfalse },

        { &g_nextmapVote, "g_nextmapVote", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_nextmapVotePlayerNumFilter, "g_nextmapVotePlayerNumFilter", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_nextmapVoteCmdEnabled, "g_nextmapVoteCmdEnabled", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_nextmapVoteNumRecommended, "g_nextmapVoteNumRecommended", "4", CVAR_ARCHIVE, 0, qfalse },
        { &g_nextmapVoteNumGametype, "g_nextmapVoteNumGametype", "6", CVAR_ARCHIVE, 0, qfalse },
        { &g_nextmapVoteTime, "g_nextmapVoteTime", "10", CVAR_ARCHIVE, 0, qfalse },
        
        { &g_recommendedMapsFile, "g_recommendedMapsFile", "recommendedmaps.cfg", 0, 0, qfalse },

	{ &g_listEntity, "g_listEntity", "0", 0, 0, qfalse },

	{ &g_obeliskHealth, "g_obeliskHealth", "2500", 0, 0, qfalse },
	{ &g_obeliskRegenPeriod, "g_obeliskRegenPeriod", "1", 0, 0, qfalse },
	{ &g_obeliskRegenAmount, "g_obeliskRegenAmount", "15", 0, 0, qfalse },
	{ &g_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", 0, 0, qfalse },

	{ &g_cubeTimeout, "g_cubeTimeout", "30", 0, 0, qfalse },
        #ifdef MISSIONPACK
	{ &g_redteam, "g_redteam", "Stroggs", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO , 0, qtrue, qtrue },
	{ &g_blueteam, "g_blueteam", "Pagans", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO , 0, qtrue, qtrue  },
	{ &g_singlePlayer, "ui_singlePlayerActive", "", 0, 0, qfalse, qfalse  },
        #endif
	{ &g_redclan, "g_redclan", "rat", 0 , 0, qfalse, qtrue },
	{ &g_blueclan, "g_blueclan", "rat", 0, 0, qfalse, qtrue  },

	{ &g_treasureTokens, "g_treasureTokens", "5", CVAR_ARCHIVE, 0, qfalse },
	{ &g_treasureHideTime, "g_treasureHideTime", "180", CVAR_ARCHIVE, 0, qfalse },
	{ &g_treasureSeekTime, "g_treasureSeekTime", "600", CVAR_ARCHIVE, 0, qfalse },
	{ &g_treasureRounds, "g_treasureRounds", "5", CVAR_ARCHIVE, 0, qfalse },
	{ &g_treasureTokenHealth, "g_treasureTokenHealth", "50", CVAR_ARCHIVE, 0, qfalse },
	{ &g_treasureTokensDestructible, "g_treasureTokensDestructible", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_treasureTokenStyle, "g_treasureTokenStyle", "0", CVAR_ARCHIVE, 0, qfalse },

	{ &g_enableDust, "g_enableDust", "0", 0, 0, qtrue, qfalse },
	{ &g_enableBreath, "g_enableBreath", "0", 0, 0, qtrue, qfalse },
	{ &g_proxMineTimeout, "g_proxMineTimeout", "20000", 0, 0, qfalse },

	{ &g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse},
	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qfalse},
	{ &pmove_msec, "pmove_msec", "11", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qfalse},

        { &pmove_float, "pmove_float", "1", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qtrue},

        { &g_floatPlayerPosition, "g_floatPlayerPosition", "1", CVAR_ARCHIVE, 0, qfalse},

//unlagged - server options
	{ &g_delagHitscan, "g_delagHitscan", "1", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qtrue },
	{ &g_delagAllowHitsAfterTele, "g_delagAllowHitsAfterTele", "1", CVAR_ARCHIVE , 0, qfalse },
	{ &g_truePing, "g_truePing", "1", CVAR_ARCHIVE, 0, qtrue },
	// it's CVAR_SYSTEMINFO so the client's sv_fps will be automagically set to its value
	{ &sv_fps, "sv_fps", "20", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qfalse },
        { &g_lagLightning, "g_lagLightning", "0", CVAR_ARCHIVE, 0, qtrue },
//unlagged - server options
        { &g_ambientSound, "g_ambientSound", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_rocketSpeed, "g_rocketSpeed", "1000", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qtrue },
	// TODO: CVAR_ARCHIVE
        { &g_maxExtrapolatedFrames, "g_maxExtrapolatedFrames", "2", 0 , 0, qfalse },

	// Missile Delag
        { &g_delagMissileMaxLatency, "g_delagMissileMaxLatency", "500", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse },
        { &g_delagMissileDebug, "g_delagMissileDebug", "0", 0, 0, qfalse },
        { &g_delagMissiles, "g_delagMissiles", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_delagMissileLimitVariance, "g_delagMissileLimitVariance", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_delagMissileLimitVarianceMs, "g_delagMissileLimitVarianceMs", "25", CVAR_ARCHIVE, 0, qfalse },
        { &g_delagMissileLatencyMode, "g_delagMissileLatencyMode", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_delagMissileCorrectFrameOffset, "g_delagMissileCorrectFrameOffset", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_delagMissileNudgeOnly, "g_delagMissileNudgeOnly", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_delagMissileImmediateRun, "g_delagMissileImmediateRun", "2", CVAR_ARCHIVE, 0, qfalse },

        { &g_predictMissiles, "g_predictMissiles", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_delagMissileBaseNudge, "g_delagMissileBaseNudge", "10", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse },


        { &g_teleporterPrediction, "g_teleporterPrediction", "1", 0, 0, qfalse },

        //{ &g_tournamentMinSpawnDistance, "g_tournamentMinSpawnDistance", "900", CVAR_ARCHIVE, 0, qfalse },
        { &g_tournamentSpawnsystem, "g_tournamentSpawnsystem", "1", CVAR_ARCHIVE, 0, qfalse },

        { &g_ffaSpawnsystem, "g_ffaSpawnsystem", "1", CVAR_ARCHIVE, 0, qfalse },

        { &g_ra3compat, "g_ra3compat", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_ra3maxArena, "g_ra3maxArena", "-1", CVAR_ROM, 0, qfalse },
        { &g_ra3forceArena, "g_ra3forceArena", "-1", 0, 0, qfalse },
        { &g_ra3nextForceArena, "g_ra3nextForceArena", "-1", 0, 0, qfalse },

        { &g_enableGreenArmor, "g_enableGreenArmor", "0", CVAR_ARCHIVE, 0, qfalse },

        { &g_readSpawnVarFiles, "g_readSpawnVarFiles", "0", CVAR_ARCHIVE, 0, qfalse },

        { &g_damageThroughWalls, "g_damageThroughWalls", "0", CVAR_ARCHIVE, 0, qtrue },

        { &g_pingEqualizer, "g_pingEqualizer", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_eqpingMax, "g_eqpingMax", "400", CVAR_ARCHIVE, 0, qfalse },
        { &g_eqpingAuto, "g_eqpingAuto", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_eqpingAutoConvergeFactor, "g_eqpingAutoConvergeFactor", "0.5", 0, 0, qfalse },
        { &g_eqpingAutoInterval, "g_eqpingAutoInterval", "1000", 0, 0, qfalse },
        { &g_eqpingSavedPing, "g_eqpingSavedPing", "0", CVAR_ROM, 0, qfalse },
        { &g_eqpingAutoTourney, "g_eqpingAutoTourney", "0", CVAR_ARCHIVE, 0, qtrue },

        { &g_teleMissiles, "g_teleMissiles", "0", CVAR_ARCHIVE, 0, qtrue },
        { &g_pushGrenades, "g_pushGrenades", "0", CVAR_ARCHIVE, 0, qtrue },
        { &g_teleMissilesMaxTeleports, "g_teleMissilesMaxTeleports", "3", CVAR_ARCHIVE, 0, qfalse },

        { &g_newShotgun, "g_newShotgun", "1", CVAR_ARCHIVE, 0, qtrue },

	{ &g_ratPhysics,   "g_ratPhysics", "0", CVAR_ARCHIVE, 0, qtrue },
	{ &g_rampJump,     "g_rampJump", "0", CVAR_ARCHIVE, 0, qtrue },
	{ &g_additiveJump,     "g_additiveJump", "0", CVAR_ARCHIVE, 0, qtrue },
	{ &g_fastSwim,   "g_fastSwim", "1", CVAR_ARCHIVE, 0, qtrue },
	{ &g_lavaDamage,     "g_lavaDamage", "10", CVAR_ARCHIVE, 0, qfalse },
	{ &g_slimeDamage,     "g_slimeDamage", "4", CVAR_ARCHIVE, 0, qfalse },
	{ &g_fastSwitch,   "g_fastSwitch", "1", CVAR_ARCHIVE, 0, qtrue },
	{ &g_fastWeapons,  "g_fastWeapons", "1", CVAR_ARCHIVE, 0, qtrue },
	{ &g_regularFootsteps,  "g_regularFootsteps", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_smoothStairs,  "g_smoothStairs", "0", CVAR_ARCHIVE, 0, qtrue },

	{ &g_bobup,  "g_bobup", "0", CVAR_ARCHIVE, 0, qfalse },

	// TODO: CVAR_ARCHIVE
	{ &g_passThroughInvisWalls,  "g_passThroughInvisWalls", "0", 0, 0, qtrue },
	{ &g_allowTimenudge,     "g_allowTimenudge", "1", CVAR_ARCHIVE, 0, qfalse },

        { &g_autoClans, "g_autoClans", "0", CVAR_ARCHIVE , 0, qfalse },

        { &g_quadWhore, "g_quadWhore", "0", CVAR_ARCHIVE , 0, qfalse },

        { &g_killDropsFlag, "g_killDropsFlag", "1", CVAR_ARCHIVE , 0, qtrue },

        { &g_killSafety, "g_killSafety", "500", CVAR_ARCHIVE , 0, qfalse },
        { &g_killDisable, "g_killDisable", "0", CVAR_ARCHIVE , 0, qfalse },

        { &g_startWhenReady, "g_startWhenReady", "0", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse },
        { &g_autoStartTime, "g_autoStartTime", "0", CVAR_ARCHIVE, 0, qfalse },

        { &g_countDownHealthArmor, "g_countDownHealthArmor", "1", CVAR_ARCHIVE , 0, qfalse },
	
        { &g_powerupGlows, "g_powerupGlows", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_screenShake, "g_screenShake", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_bobup,  "g_bobup", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_allowForcedModels, "g_allowForcedModels", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_brightModels, "g_brightModels", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_brightPlayerShells, "g_brightPlayerShells", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_brightPlayerOutlines, "g_brightPlayerOutlines", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_friendsWallHack, "g_friendsWallHack", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_friendsFlagIndicator, "g_friendsFlagIndicator", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_specShowZoom, "g_specShowZoom", "1", CVAR_ARCHIVE, 0, qfalse },

        { &g_itemPickup, "g_itemPickup", "0", CVAR_ARCHIVE , 0, qtrue },
        { &g_itemDrop, "g_itemDrop", "0", CVAR_ARCHIVE , 0, qtrue },
        { &g_usesRatVM, "g_usesRatVM", "0", 0, 0, qfalse },
        { &g_usesRatEngine, "g_usesRatEngine", "0", CVAR_ROM | CVAR_INIT, 0, qfalse },
        { &g_mixedMode, "g_mixedMode", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_broadcastClients, "g_broadcastClients", "0", 0, 0, qfalse },
        { &g_useExtendedScores, "g_useExtendedScores", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_statsboard, "g_statsboard", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_ratFlags, "g_ratFlags", "0", CVAR_SERVERINFO, 0, qfalse },
        { &g_maxBrightShellAlpha, "g_maxBrightShellAlpha", "0.5", CVAR_SERVERINFO, 0, qfalse },
        { &g_allowDuplicateGuid, "g_allowDuplicateGuid", "0", 0, 0, qfalse },

        { &g_botshandicapped, "g_botshandicapped", "1", CVAR_ARCHIVE, 0, qfalse },
        { &g_bots_randomcolors, "g_bots_randomcolors", "1", CVAR_ARCHIVE, 0, qfalse },

        { &g_pingLocationAllowed, "g_pingLocationAllowed", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_pingLocationRadius, "g_pingLocationRadius", "300", CVAR_ARCHIVE, 0, qfalse },
        { &g_pingLocationFov, "g_pingLocationFov", "15", CVAR_ARCHIVE, 0, qfalse },

        { &g_tauntAllowed, "g_tauntAllowed", "0", CVAR_ARCHIVE, 0, qfalse },
        { &g_tauntTime, "g_tauntTime", "5000", CVAR_ARCHIVE, 0, qfalse },
        { &g_tauntAfterDeathTime, "g_tauntAfterDeathTime", "1500", CVAR_ARCHIVE, 0, qfalse },

// weapon config
	{ &g_mgDamage,			"g_mgDamage", "6", 0, 0, qtrue },
	{ &g_mgTeamDamage,		"g_mgTeamDamage", "5", 0, 0, qtrue },
	{ &g_railgunDamage,		"g_railgunDamage", "80", 0, 0, qtrue },
	{ &g_lgDamage, 			"g_lgDamage", "7", 0, 0, qtrue },

	{ &g_railJump, 			"g_railJump", "0", CVAR_ARCHIVE, 0, qtrue },

	{ &g_teamslocked, 		"g_teamslocked", "0", 0, 0, qfalse },
	{ &g_autoTeamsUnlock, 		"g_autoTeamsUnlock", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_autoTeamsLock, 		"g_autoTeamsLock", "0", CVAR_ARCHIVE, 0, qtrue },
	{ &g_tourneylocked, 		"g_tourneylocked", "0", 0, 0, qfalse },
	{ &g_specMuted, 		"g_specMuted", "0", 0, 0, qfalse },
	{ &g_tournamentMuteSpec,        "g_tournamentMuteSpec", "0", CVAR_ARCHIVE, 0, qtrue },

	{ &g_timeoutAllowed, 		"g_timeoutAllowed", "0", 0, 0, qfalse },
	{ &g_timeinAllowed, 		"g_timeinAllowed", "1", 0, 0, qfalse },
	{ &g_timeoutTime, 		"g_timeoutTime", "30", 0, 0, qfalse },
	{ &g_timeoutOvertimeStep,	"g_timeoutOvertimeStep", "30", 0, 0, qfalse },

	{ &g_autoFollowKiller,	"g_autoFollowKiller", "0", 0, 0, qfalse },
	{ &g_autoFollowSwitchTime,	"g_autoFollowSwitchTime", "60", 0, 0, qfalse },


	{ &g_shaderremap,		"g_shaderremap", "0", 0, 0, qfalse },
	{ &g_shaderremap_flag,          "g_shaderremap_flag", "1", 0, 0, qfalse },
	{ &g_shaderremap_flagreset,     "g_shaderremap_flagreset", "1", 0, 0, qfalse },
	{ &g_shaderremap_banner,        "g_shaderremap_banner", "1", 0, 0, qfalse },
	{ &g_shaderremap_bannerreset,   "g_shaderremap_bannerreset", "1", 0, 0, qfalse },

	{ &g_rankings, "g_rankings", "0", 0, 0, qfalse},
        { &g_music, "g_music", "", 0, 0, qfalse},
        { &g_spawnprotect, "g_spawnprotect", "500", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue},
	//Now for elimination stuff:
	{ &g_elimination_respawn, "elimination_respawn", "0", CVAR_ARCHIVE, 0, qtrue },
	{ &g_elimination_respawn_increment, "elimination_respawn_increment", "5", CVAR_ARCHIVE, 0, qtrue },
	{ &g_elimination_selfdamage, "elimination_selfdamage", "0", 0, 0, qtrue },
	{ &g_elimination_startHealth, "elimination_startHealth", "200", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_startArmor, "elimination_startArmor", "150", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_bfg, "elimination_bfg", "0", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },
        { &g_elimination_grapple, "elimination_grapple", "0", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_roundtime, "elimination_roundtime", "120", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_warmup, "elimination_warmup", "7", CVAR_ARCHIVE | CVAR_NORESTART , 0, qtrue },
	{ &g_elimination_activewarmup, "elimination_activewarmup", "5", CVAR_ARCHIVE | CVAR_NORESTART , 0, qtrue },
        { &g_elimination_allgametypes, "g_elimination", "0", CVAR_LATCH | CVAR_NORESTART, 0, qfalse },
        { &g_elimination_spawn_allgametypes, "g_elimination_spawn", "0", CVAR_ARCHIVE , 0, qfalse },

	{ &g_elimination_machinegun, "elimination_machinegun", "500", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_shotgun, "elimination_shotgun", "500", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_grenade, "elimination_grenade", "100", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_rocket, "elimination_rocket", "50", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_railgun, "elimination_railgun", "20", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_lightning, "elimination_lightning", "300", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_plasmagun, "elimination_plasmagun", "200", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_chain, "elimination_chain", "0", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_mine, "elimination_mine", "0", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },
	{ &g_elimination_nail, "elimination_nail", "0", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },

	{ &g_elimination_ctf_oneway, "elimination_ctf_oneway", "0", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue },

        { &g_elimination_lockspectator, "elimination_lockspectator", "0", CVAR_NORESTART, 0, qtrue },
        
        { &g_awardpushing, "g_awardpushing", "1", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

        //g_persistantpowerups
        #ifdef MISSIONPACK
        { &g_persistantpowerups, "g_runes", "1", CVAR_LATCH, 0, qfalse },
        #else
        { &g_persistantpowerups, "g_runes", "0", CVAR_LATCH|CVAR_ARCHIVE, 0, qfalse },
        #endif

	{ &g_swingGrapple, "g_swingGrapple", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_grapple, "g_grapple", "0", CVAR_ARCHIVE, 0, qtrue },

	//nexuiz style rocket arena
	{ &g_rockets, "g_rockets", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_NORESTART, 0, qfalse },

	//Instantgib and Vampire thingies
	{ &g_instantgib, "g_instantgib", "0", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
	{ &g_vampire, "g_vampire", "0.0", CVAR_NORESTART, 0, qtrue },
	{ &g_midAir, "g_midAir", "0", CVAR_NORESTART | CVAR_ARCHIVE, 0, qtrue },
	{ &g_regen, "g_regen", "0", CVAR_NORESTART, 0, qtrue },
	{ &g_vampireMaxHealth, "g_vampire_max_health", "500", CVAR_NORESTART, 0, qtrue },
	{ &g_lms_lives, "g_lms_lives", "1", CVAR_NORESTART, 0, qtrue },
	{ &g_lms_mode, "g_lms_mode", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

        { &g_catchup, "g_catchup", "0", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue},

        { &g_autonextmap, "g_autonextmap", "0", CVAR_ARCHIVE | CVAR_NORESTART, 0, qfalse},
        { &g_mappools, "g_mappools", "0\\maps_dm.cfg\\1\\maps_tourney.cfg\\3\\maps_tdm.cfg\\4\\maps_ctf.cfg\\5\\maps_oneflag.cfg\\6\\maps_obelisk.cfg\
\\7\\maps_harvester.cfg\\8\\maps_elimination.cfg\\9\\maps_ctf.cfg\\10\\maps_lms.cfg\\11\\maps_dd.cfg\\12\\maps_dom.cfg\\13\\maps_th.cfg\\", CVAR_ARCHIVE | CVAR_NORESTART, 0, qfalse},
        { &g_humanplayers, "g_humanplayers", "0", CVAR_ROM | CVAR_NORESTART, 0, qfalse },
//used for voIP
        { &g_redTeamClientNumbers, "g_redTeamClientNumbers", "0",CVAR_ROM, 0, qfalse },
        { &g_blueTeamClientNumbers, "g_blueTeamClientNumbers", "0",CVAR_ROM, 0, qfalse },
        
        //KK-OAX
        { &g_sprees, "g_sprees", "sprees.dat", 0, 0, qfalse },
        { &g_altExcellent, "g_altExcellent", "0", CVAR_SERVERINFO, 0, qtrue}, 
        { &g_spreeDiv, "g_spreeDiv", "5", 0, 0, qfalse},
        
        //Used for command/chat flooding
        { &g_floodMaxDemerits, "g_floodMaxDemerits", "5000", CVAR_ARCHIVE, 0, qfalse  },
        { &g_floodMinTime, "g_floodMinTime", "2000", CVAR_ARCHIVE, 0, qfalse  },
        { &g_floodLimitUserinfo, "g_floodLimitUserinfo", "0", CVAR_ARCHIVE, 0, qfalse  },
        
        //Admin
        { &g_admin, "g_admin", "admin.dat", CVAR_ARCHIVE, 0, qfalse  },
        { &g_adminLog, "g_adminLog", "admin.log", CVAR_ARCHIVE, 0, qfalse  },
        { &g_adminParseSay, "g_adminParseSay", "1", CVAR_ARCHIVE, 0, qfalse  },
        { &g_adminNameProtect, "g_adminNameProtect", "1", CVAR_ARCHIVE, 0, qfalse  },
        { &g_adminTempBan, "g_adminTempBan", "2m", CVAR_ARCHIVE, 0, qfalse  },
        { &g_adminMaxBan, "g_adminMaxBan", "2w", CVAR_ARCHIVE, 0, qfalse  },
        
        { &g_specChat, "g_specChat", "1", CVAR_ARCHIVE, 0, qfalse  },
        { &g_publicAdminMessages, "g_publicAdminMessages", "1", CVAR_ARCHIVE, 0, qfalse  },
        
        { &g_maxWarnings, "g_maxWarnings", "3", CVAR_ARCHIVE, 0, qfalse },
	    { &g_warningExpire, "g_warningExpire", "3600", CVAR_ARCHIVE, 0, qfalse },
	    
	    { &g_minNameChangePeriod, "g_minNameChangePeriod", "10", 0, 0, qfalse},
        { &g_maxNameChanges, "g_maxNameChanges", "50", 0, 0, qfalse},

        { &g_allowDuplicateNames, "g_allowDuplicateNames", "0", CVAR_ARCHIVE, 0, qfalse},

        { &g_unnamedPlayersAllowed, "g_unnamedPlayersAllowed", "1", CVAR_ARCHIVE, 0, qfalse},
        { &g_unnamedRenameAdjlist, "g_unnamedRenameAdjlist", "ratname-adjectives.txt", CVAR_ARCHIVE, 0, qfalse},
        { &g_unnamedRenameNounlist, "g_unnamedRenameNounlist", "ratname-nouns.txt", CVAR_ARCHIVE, 0, qfalse},

        { &g_timestamp_startgame, "g_timestamp", "0001-01-01 00:00:00", 0, 0, qfalse}
        
};

// bk001129 - made static to avoid aliasing
static int gameCvarTableSize = sizeof( gameCvarTable ) / sizeof( gameCvarTable[0] );


void G_InitGame( int levelTime, int randomSeed, int restart );
void G_RunFrame( int levelTime );
void G_ShutdownGame( int restart );
void CheckExitRules( void );


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {
	switch ( command ) {
	case GAME_INIT:
		G_InitGame( arg0, arg1, arg2 );
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		return 0;
	case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect( arg0, arg1, arg2 );
	case GAME_CLIENT_THINK:
		ClientThink( arg0 );
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChangedLimited( arg0 );
		return 0;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
		return 0;
	case GAME_CLIENT_BEGIN:
		ClientBegin( arg0 );
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		return 0;
	case GAME_RUN_FRAME:
		G_RunFrame( arg0 );
		return 0;
	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();
	case BOTAI_START_FRAME:
		return BotAIStartFrame( arg0 );
	}

	return -1;
}


void QDECL G_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	Q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	trap_Printf( text );
}

void QDECL G_Error( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	Q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	trap_Error( text );
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for ( i=1, e=g_entities+i ; i < level.num_entities ; i++,e++ ){
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < level.num_entities ; j++,e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}
        G_Printf ("%i teams with %i entities\n", c, c2);
}

void G_RemapTeamShaders( void ) {
	char string[1024];
	char mapname[MAX_QPATH];

	float f = level.time * 0.001;
	ClearRemaps();
#ifdef MISSIONPACK
	Com_sprintf( string, sizeof(string), "team_icon/%s_red", g_redteam.string );
	AddRemap("textures/ctf2/redteam01", string, f); 
	AddRemap("textures/ctf2/redteam02", string, f); 
	Com_sprintf( string, sizeof(string), "team_icon/%s_blue", g_blueteam.string );
	AddRemap("textures/ctf2/blueteam01", string, f); 
	AddRemap("textures/ctf2/blueteam02", string, f); 
#endif

	if (g_shaderremap.integer) {
		qboolean has_banner = qfalse;
		qboolean has_bannerq3 = qfalse;
		trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );

		if (Q_stricmp(mapname, "ps37ctf") == 0
				|| Q_stricmp(mapname, "ps37ctf2") == 0
				|| Q_stricmp(mapname, "ps37ctf-mmp") == 0
				|| Q_stricmp(mapname, "oa_ctf2") == 0
				|| Q_stricmp(mapname, "oa_ctf2old") == 0
				|| Q_stricmp(mapname, "czest3ctf") == 0
				|| Q_stricmp(mapname, "oa_minia") == 0
				|| Q_stricmp(mapname, "oasago1") == 0
				|| Q_stricmp(mapname, "oa_thor") == 0
				|| Q_stricmp(mapname, "wrackdm17") == 0
				) {
			has_banner = qtrue;
		}
		if (Q_stricmp(mapname, "13dream") == 0
				|| Q_stricmp(mapname, "ct3ctf2") == 0
				|| Q_stricmp(mapname, "geit3ctf1") == 0
				|| Q_stricmp(mapname, "hctf3") == 0
				|| Q_stricmp(mapname, "mkbase") == 0
				|| Q_stricmp(mapname, "q3ctfp13") == 0
				|| Q_stricmp(mapname, "q3ctfp22_final") == 0
				|| Q_stricmp(mapname, "q3wcp20") == 0
				|| Q_stricmp(mapname, "rooftopsctf") == 0
				|| Q_stricmp(mapname, "stchctf9a") == 0
				|| Q_stricmp(mapname, "woohaa") == 0) {
			has_bannerq3 = qtrue;
		}
		// RATOA
		if( g_redclan.string[0] ) {
			if (g_shaderremap_flag.integer && (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION)) {
				Com_sprintf( string, sizeof(string), "team_icon/ratoa/%s_redflag", g_redclan.string );
				AddRemap("models/flags/r_flag", string, f); 
			}
			if (g_shaderremap_banner.integer) {
				if (has_banner) {
					Com_sprintf( string, sizeof(string), "team_icon/ratoa/%s_red_banner", g_redclan.string );
					if (Q_stricmp(mapname, "mlctf1beta") == 0) {
						AddRemap("textures/ctf2/red_banner02", string, f); 
					} else {
						AddRemap("textures/clown/red_banner", string, f); 
					}
				}
				if (has_bannerq3) {
					Com_sprintf( string, sizeof(string), "team_icon/ratoa/%s_red_bannerq3", g_redclan.string );
					AddRemap("textures/ctf/ctf_redflag", string, f); 
				}
			}
		}  else {
			if (g_shaderremap_flagreset.integer && (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION)) {
				AddRemap("models/flags/r_flag", "models/flags/r_flag", f); 
			}
			if (g_shaderremap_bannerreset.integer) {
				if (has_banner) {
					if (Q_stricmp(mapname, "mlctf1beta") == 0) {
						AddRemap("textures/ctf2/red_banner02", "textures/ctf2/red_banner02", f); 
					} else {
						AddRemap("textures/clown/red_banner", "textures/clown/red_banner", f); 
					}
				}
				if (has_bannerq3) {
					AddRemap("textures/ctf/ctf_redflag", "textures/ctf_ctf_redflag", f); 
				}
			}
		}
		if( g_blueclan.string[0] ) {
			if (g_shaderremap_flag.integer && (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION)) {
				Com_sprintf( string, sizeof(string), "team_icon/ratoa/%s_blueflag", g_blueclan.string );
				AddRemap("models/flags/b_flag", string, f); 
			}
			if (g_shaderremap_banner.integer) {
				if (has_banner) {
					Com_sprintf( string, sizeof(string), "team_icon/ratoa/%s_blue_banner", g_blueclan.string );
					if (Q_stricmp(mapname, "mlctf1beta") == 0) {
						AddRemap("textures/ctf2/blue_banner02", string, f); 
					} else {
						AddRemap("textures/clown/blue_banner", string, f); 
					}
				}
				if (has_bannerq3) {
					Com_sprintf( string, sizeof(string), "team_icon/ratoa/%s_blue_bannerq3", g_blueclan.string );
					AddRemap("textures/ctf/ctf_blueflag", string, f); 
				}
			}
		}  else {
			if (g_shaderremap_flagreset.integer && (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION)) {
				AddRemap("models/flags/b_flag", "models/flags/b_flag", f); 
			}
			if (g_shaderremap_bannerreset.integer) {
				if (has_banner) {
					if (Q_stricmp(mapname, "mlctf1beta") == 0) {
						AddRemap("textures/ctf2/blue_banner02", "textures/ctf2/blue_banner02", f); 
					} else {
						AddRemap("textures/clown/blue_banner", "textures/clown/blue_banner", f); 
					}
				}
				if (has_bannerq3) {
					AddRemap("textures/ctf/ctf_blueflag", "textures/ctf/ctf_blueflag", f); 
				}
			}
		}
	}
	trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
}

void G_EQPingReset(void) {
	if (!g_usesRatEngine.integer) {
		return;
	}
	trap_RAT_EQPing_Reset();
	trap_Cvar_Set( "sv_eqping", "0" );
	trap_Cvar_Set( "g_eqpingSavedPing", "0");
	if (level.eqPing) {
		trap_SendServerCommand( -1, va("print \"^5EQPing: resetting pings...\n"));
		level.eqPing = 0;
	}
}

int G_MaxPlayingClientPing(void) {
	int i;
	int maxPing = 0;
	gclient_t *cl = NULL;
	for ( i = 0;  i < level.numPlayingClients; i++ ) {
		cl = &level.clients[ level.sortedClients[i] ];
		if (cl->pers.realPing > maxPing) {
			maxPing = cl->pers.realPing;
		}
	}
	return maxPing;
}

void G_EQPingClientReset(gclient_t *client) {
	if (!g_usesRatEngine.integer || !trap_Cvar_VariableIntegerValue("sv_eqping")) {
		return;
	}

	if ( client->pers.connected != CON_CONNECTED || client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_RAT_EQPing_SetDelay(client - g_clients, 0);
		return;
	}
}

void G_EQPingClientSet(gclient_t *client) {
	if (client->pers.realPing < level.eqPing) {
		trap_RAT_EQPing_SetDelay(client - g_clients, level.eqPing - client->pers.realPing);
	} else {
		trap_RAT_EQPing_SetDelay(client - g_clients, 0);
	}

}

// called upon game start
// to re-apply EQPing after warmup
qboolean G_EQPingReapply(void) {
	if (level.warmupTime != 0 || g_eqpingSavedPing.integer <= 0) {
		return qfalse;
	}
	level.eqPing = g_eqpingSavedPing.integer;
	return qtrue;
}

void G_EQPingUpdate(void) {
	int pingMod; 
	int i;
	gclient_t *cl;

	if (!g_usesRatEngine.integer 
			|| !trap_Cvar_VariableIntegerValue("sv_eqping")
			|| level.eqPing == 0
			|| g_eqpingAuto.integer) {
		return;
	}
	for ( i = 0;  i < level.numPlayingClients; i++ ) {
		cl = &level.clients[ level.sortedClients[i] ];
		if (cl->pers.enterTime + 5000 >= level.time) {
			continue;
		}
		pingMod = trap_RAT_EQPing_GetDelay(level.sortedClients[i]);
		if (pingMod <= 0) {
			// FIXME: this may actually increase the ping of the
			// player who previously had the highest ping (upon whom
			// the eqping setting was based). However, this should
			// be acceptable as the increase will be tiny assuming
			// the ping is more or less stable.
			G_EQPingClientSet(cl);
		}
	}
}

void G_EQPingSet(int maxEQPing, qboolean forceMax) {
	int i;
	gclient_t *cl = NULL;

	if (maxEQPing <= 0) {
		G_EQPingReset();
		return;
	}

	if (!g_usesRatEngine.integer) {
		return;
	}
	// g_eqping 500 -> automatically equalize ping to a max of 500 (only for tournament)
	// !eqping 500 -> equalize ping right now, for 1 game only, to a max of 500, w/o setting g_eqping
	if (forceMax) {
		level.eqPing = maxEQPing;
	} else {
		level.eqPing = G_MaxPlayingClientPing();
		if (level.eqPing > maxEQPing) {
			level.eqPing = maxEQPing;
		}
	}
	if (level.warmupTime != 0) {
		// make sure eqping is re-applied once the game actually starts
		trap_Cvar_Set( "g_eqpingSavedPing", va("%i", level.eqPing ));
	}

	trap_Cvar_Set( "sv_eqping", "1" );
	trap_RAT_EQPing_Reset();

	for ( i = 0;  i < level.numPlayingClients; i++ ) {
		cl = &level.clients[ level.sortedClients[i] ];
		G_EQPingClientSet(cl);
	}
	trap_SendServerCommand( -1, va("print \"^5EQPing: equalizing all pings to %ims...\n", level.eqPing));

}

void G_EQPingAutoAdjust(void) {
	int ping;
	int pingMod; 
	float pingdiff; 
	int i;
	gclient_t *cl;

	if (!g_usesRatEngine.integer 
			|| !trap_Cvar_VariableIntegerValue("sv_eqping")
			|| level.eqPing == 0
			|| !g_eqpingAuto.integer
			|| level.eqPingAdjustTime + g_eqpingAutoInterval.integer > level.time) {
		return;
	}

	level.eqPingAdjustTime = level.time;
	for ( i = 0;  i < level.numPlayingClients; i++ ) {
		cl = &level.clients[ level.sortedClients[i] ];
		ping = cl->pers.realPing;
		pingMod = trap_RAT_EQPing_GetDelay(cl - g_clients);
		
		pingdiff = level.eqPing - ping;
		pingdiff = pingdiff * MIN(1.0, MAX(0.0, g_eqpingAutoConvergeFactor.value));

		// this should converge slowly to the desired ping
		// default converge factor is 0.5, it can be increased / decreased for
		// faster/slower convergence
		trap_RAT_EQPing_SetDelay(
				cl - g_clients, 
				(pingdiff < 0 ? floor(pingdiff) : ceil(pingdiff)) + pingMod
				);
	}
}

void G_EQPingAutoTourney(void) {
	qboolean equalize = qfalse;
	gclient_t *c1 = NULL;
	gclient_t *c2 = NULL;
	if (!g_usesRatEngine.integer
			|| g_gametype.integer != GT_TOURNAMENT
			|| !g_eqpingAutoTourney.integer
			|| g_eqpingMax.integer <= 0) {
		return;
	}

	if (level.warmupTime == 0 // game already running
			|| level.numPlayingClients != 2 // not enough players
			|| level.eqPing // already equalized through EQPing
			) {
		return;
	}
	c1 = &level.clients[level.sortedClients[0]];
	c2 = &level.clients[level.sortedClients[1]];
	if (!c1 || !c2) {
		return;
	}
	if (level.warmupTime > 0 
			&& level.time < level.warmupTime
			&& level.warmupTime - level.time <= 10000 ) {
		// warmup already running, equalize now!
		equalize = qtrue;
	} else if (level.warmupTime == -1
			&& c1->pers.enterTime + 5000 <= level.time
			&& c2->pers.enterTime + 5000 <= level.time
		  ) {
		// in \ready phase, both joined at least 5s ago
		equalize = qtrue;
	}
	if (equalize) {

		if (g_entities[level.sortedClients[0]].r.svFlags & SVF_BOT
				|| g_entities[level.sortedClients[1]].r.svFlags & SVF_BOT) {
			return;
		}

		G_EQPingSet(g_eqpingMax.integer, qfalse);

	}


}

void G_PingEqualizerReset(void) {
	fileHandle_t f;
	int len;

	if (!g_pingEqualizer.integer) {
		return;
	}
	if (level.warmupTime == 0 ){
		return;
	}
	Com_Printf("Resetting ping equalizer...\n");
	len = trap_FS_FOpenFile( "pingequalizer.log", &f, FS_WRITE );
	if (len < 0 ) {
		return;
	}
	trap_FS_Write( "\n", 1, f );
	trap_FS_FCloseFile( f );
	level.pingEqualized = qfalse;
	trap_SendServerCommand( -1, va("print \"^5Server: resetting ping equalizer...\n"));
}

void G_PingEqualizerWrite(void) {
	fileHandle_t f;
	int len;
	char *s;
	qboolean equalize = qfalse;
	gclient_t *c1 = NULL;
	gclient_t *c2 = NULL;
	if (!g_pingEqualizer.integer || g_gametype.integer != GT_TOURNAMENT) {
		return;
	}
	if (level.warmupTime == 0 // game already running
			|| level.numPlayingClients != 2 // not enough players
			|| level.pingEqualized // already equalized
			) {
		return;
	}
	c1 = &level.clients[level.sortedClients[0]];
	c2 = &level.clients[level.sortedClients[1]];
	if (!c1 || !c2) {
		return;
	}
	if (level.warmupTime > 0 
			&& level.time < level.warmupTime
			&& level.warmupTime - level.time <= 10000 ) {
		// warmup already running, equalize now!
		equalize = qtrue;
	} else if (level.warmupTime == -1
			&& c1->pers.enterTime + 5000 <= level.time
			&& c2->pers.enterTime + 5000 <= level.time
		  ) {
		// in \ready phase, both joined at least 5s ago
		equalize = qtrue;
	}
	if (equalize) {
		gclient_t *lower = NULL;
		int pingdiff = 0;

		level.pingEqualized = qtrue;

		if (g_entities[level.sortedClients[0]].r.svFlags & SVF_BOT
				|| g_entities[level.sortedClients[1]].r.svFlags & SVF_BOT) {
			return;
		}
		if (c1->pers.realPing > c2->pers.realPing) {
			pingdiff = c1->pers.realPing - c2->pers.realPing;
			lower = c2;
		} else if (c1->pers.realPing < c2->pers.realPing) {
			pingdiff = c2->pers.realPing - c1->pers.realPing;
			lower = c1;
		}
		if (!lower) {
			return;
		}
		Com_Printf("Updating ping equalizer...\n");
		len = trap_FS_FOpenFile( "pingequalizer.log", &f, FS_WRITE );
		if (len < 0 ) {
			return;
		}
		s = va("%s %i\n", lower->pers.ip, pingdiff);
		trap_FS_Write( s, strlen(s), f );
		trap_FS_FCloseFile( f );

		trap_SendServerCommand( -1, va("print \"^5Server: equalizing pings...\n"));
	}
}


void G_UpdateActionCamera(void) {
	int i;
	gclient_t *cl;
	gentity_t *ent;
	int clientNum = level.followauto;

	cl = &level.clients[ clientNum ];
	if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
		if (cl->ps.powerups[PW_REDFLAG] || cl->ps.powerups[PW_BLUEFLAG]) {
			return;
		}
	}
	// if the a flag is taken, follow flag carrier
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t *cl2 = &level.clients[i];
		if ( cl2->pers.connected != CON_CONNECTED 
				|| cl2->sess.sessionTeam == TEAM_SPECTATOR )
			continue;
		if (cl2->ps.powerups[PW_REDFLAG] || cl2->ps.powerups[PW_BLUEFLAG]) {
			level.followauto = i;
			level.followautoTime = level.time;
			return;
		}
	}

	cl = &level.clients[ clientNum ];
	if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
		ent = &g_entities[ clientNum ];
		if (g_autoFollowKiller.integer) {
			if (ent->health <= 0) {
				if (cl->lasthurt_client < level.maxclients) {
					level.followauto = cl->lasthurt_client;
					level.followautoTime = level.time;
					return;
				}
			}
		}
		if (level.followautoTime + g_autoFollowSwitchTime.integer * 1000 > level.time) {
			return;
		}
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t *cl2 = &level.clients[i];
		if ( cl2->pers.connected != CON_CONNECTED || cl2->sess.sessionTeam == TEAM_SPECTATOR )
			continue;
		if (i == clientNum) {
			continue;
		}
		ent = &g_entities[ i ];
		if (ent->health >= 0) {
			level.followauto = i;
			level.followautoTime = level.time;
			return;
		}
	}
}


/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	qboolean remapped = qfalse;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
		if ( cv->vmCvar )
			cv->modificationCount = cv->vmCvar->modificationCount;

		if (cv->teamShader) {
			remapped = qtrue;
		}
	}

	if (remapped) {
		G_RemapTeamShaders();
	}

	// check some things
	if ( g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE ) {
                G_Printf( "g_gametype %i is out of range, defaulting to 0\n", g_gametype.integer );
		trap_Cvar_Set( "g_gametype", "0" );
	}

	//set FFA status for high gametypes:
	if ( g_gametype.integer == GT_LMS ) {
		g_ffa_gt = 1;	//Last Man standig is a FFA gametype
	} else {
		g_ffa_gt = 0;	//If >GT_CTF use bases
	}

	level.warmupModificationCount = g_warmup.modificationCount;
}

void G_UpdateRatFlags( void ) {
	int rflags = 0;

	if (g_itemPickup.integer) {
		rflags |= RAT_EASYPICKUP;
	}

	if (g_powerupGlows.integer) {
		rflags |= RAT_POWERUPGLOWS;
	}

	if (g_screenShake.integer) {
		rflags |= RAT_SCREENSHAKE;
	}

	if (g_predictMissiles.integer && g_delagMissiles.integer) {
		rflags |= RAT_PREDICTMISSILES;
	}

	if (g_fastSwitch.integer) {
		rflags |= RAT_FASTSWITCH;
	}

	if (g_fastWeapons.integer) {
		rflags |= RAT_FASTWEAPONS;
	}

	if (g_ratPhysics.integer == 1) {
		rflags |= RAT_RATPHYSICS;
	}

	if (g_rampJump.integer) {
		rflags |= RAT_RAMPJUMP;
	}


	if (g_allowForcedModels.integer) {
		rflags |= RAT_ALLOWFORCEDMODELS;
	}

	if (g_friendsWallHack.integer) {
		rflags |= RAT_FRIENDSWALLHACK;
	}

	if (g_specShowZoom.integer) {
		rflags |= RAT_SPECSHOWZOOM;
	}

	if (g_brightPlayerShells.integer) {
		rflags |= RAT_BRIGHTSHELL;
	}

	if (g_brightPlayerOutlines.integer) {
		rflags |= RAT_BRIGHTOUTLINE;
	}

	if (g_brightModels.integer) {
		rflags |= RAT_BRIGHTMODEL;
	}

	if (g_newShotgun.integer) {
		rflags |= RAT_NEWSHOTGUN;
	}

	if (g_additiveJump.integer) {
		rflags |= RAT_ADDITIVEJUMP;
	}

	if (!g_allowTimenudge.integer) {
		rflags |= RAT_NOTIMENUDGE;
	}

	if (g_friendsFlagIndicator.integer) {
		rflags |= RAT_FLAGINDICATOR;
	}

	if (g_regularFootsteps.integer) {
		rflags |= RAT_REGULARFOOTSTEPS;
	}

	if (g_smoothStairs.integer) {
		rflags |= RAT_SMOOTHSTAIRS;
	}

	if (g_passThroughInvisWalls.integer) {
		rflags |= RAT_NOINVISWALLS;
	}

	if (!g_bobup.integer) {
		rflags |= RAT_NOBOBUP;
	}
	
	if (g_fastSwim.integer) {
		rflags |= RAT_FASTSWIM;
	}

	if (g_swingGrapple.integer) {
		rflags |= RAT_SWINGGRAPPLE;
	}

	// XXX --> also update code where this is called!

	trap_Cvar_Set("g_ratFlags",va("%i",rflags));
	trap_Cvar_Update( &g_ratFlags );
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	qboolean remapped = qfalse;
	qboolean updateRatFlags = qfalse;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		if ( cv->vmCvar ) {
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount ) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if ( cv->trackChange ) {
					trap_SendServerCommand( -1, va("print \"Server: %s changed to %s\n\"", 
						cv->cvarName, cv->vmCvar->string ) );
				}

                                if ( cv->vmCvar == &g_votecustom )
                                    VoteParseCustomVotes();

                                //Here comes the cvars that must trigger a map_restart
                                if (cv->vmCvar == &g_instantgib || cv->vmCvar == &g_rockets  ||  cv->vmCvar == &g_elimination_allgametypes) {
                                    trap_Cvar_Set("sv_dorestart","1");
                                }
                                
                                if ( cv->vmCvar == &g_voteNames ) {
                                    //Set vote flags
                                    int voteflags=0;
                                    if( allowedVote("map_restart") )
                                        voteflags|=VF_map_restart;

                                    if( allowedVote("map") )
                                        voteflags|=VF_map;

                                    if( allowedVote("clientkick") )
                                        voteflags|=VF_clientkick;

                                    if( allowedVote("shuffle") )
                                        voteflags|=VF_shuffle;

                                    if( allowedVote("nextmap") )
                                        voteflags|=VF_nextmap;

                                    if( allowedVote("g_gametype") )
                                        voteflags|=VF_g_gametype;
                                    
                                    if( allowedVote("g_doWarmup") )
                                        voteflags|=VF_g_doWarmup;

                                    if( allowedVote("timelimit") )
                                        voteflags|=VF_timelimit;

                                    if( allowedVote("fraglimit") )
                                        voteflags|=VF_fraglimit;

                                    if( allowedVote("custom") )
                                        voteflags|=VF_custom;

                                    trap_Cvar_Set("voteflags",va("%i",voteflags));
                                }
      
				if (cv->teamShader) {
					remapped = qtrue;
				}

				if (cv->vmCvar == &g_itemPickup 
						|| cv->vmCvar == &g_powerupGlows
						|| cv->vmCvar == &g_screenShake
						|| cv->vmCvar == &g_predictMissiles
						|| cv->vmCvar == &g_fastSwitch
						|| cv->vmCvar == &g_fastWeapons
						|| cv->vmCvar == &g_ratPhysics
						|| cv->vmCvar == &g_rampJump
						|| cv->vmCvar == &g_allowForcedModels
						|| cv->vmCvar == &g_friendsWallHack
						|| cv->vmCvar == &g_specShowZoom
						|| cv->vmCvar == &g_brightPlayerShells
						|| cv->vmCvar == &g_brightPlayerOutlines
						|| cv->vmCvar == &g_brightModels
						|| cv->vmCvar == &g_newShotgun
						|| cv->vmCvar == &g_additiveJump
						|| cv->vmCvar == &g_friendsFlagIndicator
						|| cv->vmCvar == &g_regularFootsteps
						|| cv->vmCvar == &g_smoothStairs
						|| cv->vmCvar == &g_passThroughInvisWalls
						|| cv->vmCvar == &g_bobup
						|| cv->vmCvar == &g_fastSwim
						|| cv->vmCvar == &g_swingGrapple
						) {
					updateRatFlags = qtrue;
				}
			}
		}
	}

	if (remapped) {
		G_RemapTeamShaders();
	}

	if (updateRatFlags) {
		G_UpdateRatFlags();
	}
}

qboolean G_AutoStartReady( void ) {
	return	(g_autoStartTime.integer > 0 && g_autoStartTime.integer*1000 < (level.time - level.startTime));
}

/*
 Sets the cvar g_timestamp. Return 0 if success or !0 for errors.
 */
int G_UpdateTimestamp( void ) {
    int ret = 0;
    qtime_t timestamp;
    ret = trap_RealTime(&timestamp);
    trap_Cvar_Set("g_timestamp",va("%04i-%02i-%02i %02i:%02i:%02i",
    1900+timestamp.tm_year,1+timestamp.tm_mon, timestamp.tm_mday,
    timestamp.tm_hour,timestamp.tm_min,timestamp.tm_sec));

    return ret;
}

/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, int restart ) {
	int					i;

        
        G_Printf ("------- Game Initialization -------\n");
        G_Printf ("gamename: %s\n", GAMEVERSION);
        G_Printf ("gamedate: %s\n", __DATE__);

	srand( randomSeed );

	G_RegisterCvars();

        G_UpdateTimestamp();
        
        //disable unwanted cvars
        if( g_gametype.integer == GT_SINGLE_PLAYER )
        {
            g_instantgib.integer = 0;
            g_rockets.integer = 0;
            g_vampire.value = 0.0f;
        }

	G_ProcessIPBans();
    
    //KK-OAX Changed to Tremulous's BG_InitMemory
	BG_InitMemory();

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	
	level.time = levelTime;
	level.startTime = levelTime;

	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime

	if ( g_gametype.integer != GT_SINGLE_PLAYER && g_logfile.string[0] ) {
		if ( g_logfileSync.integer ) {
			trap_FS_FOpenFile( g_logfile.string, &level.logFile, FS_APPEND_SYNC );
		} else {
			trap_FS_FOpenFile( g_logfile.string, &level.logFile, FS_APPEND );
		}
		if ( !level.logFile ) {
			G_Printf( "WARNING: Couldn't open logfile: %s\n", g_logfile.string );
		} else {
			char	serverinfo[MAX_INFO_STRING];

			trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

			G_LogPrintf("------------------------------------------------------------\n" );
			G_LogPrintf("InitGame: %s\n", serverinfo );
                        G_LogPrintf("Info: ServerInfo length: %d of %d\n", strlen(serverinfo), MAX_INFO_STRING );
		}
	} else {
		G_Printf( "Not logging to disk.\n" );
	}

        //Parse the custom vote names:
        VoteParseCustomVotes();

	G_InitWorldSession();
    
    //KK-OAX Get Admin Configuration
    G_admin_readconfig( NULL, 0 );
	//Let's Load up any killing sprees/multikills
	G_ReadAltKillSettings( NULL, 0 );

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]) );
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i=0 ; i<level.maxclients ; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

        for ( i=0 ; i<MAX_CLIENTS ; i++ ) {
                g_entities[i].classname = "clientslot";
        }
        
	// let the server system know where the entites are
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ), 
		&level.clients[0].ps, sizeof( level.clients[0] ) );

	// reserve some spots for dead player bodies
	InitBodyQue();

	ClearRegisteredItems();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	G_SetTeleporterDestinations();

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	if( g_gametype.integer >= GT_TEAM && (g_ffa_gt!=1)) {
		G_CheckTeamItems();
	}

	SaveRegisteredItems();

	trap_Cvar_Set("g_usesRatEngine", va("%i", trap_Cvar_VariableIntegerValue( "sv_ratEngine" )));
        
        G_Printf ("-----------------------------------\n");

	if( g_gametype.integer == GT_SINGLE_PLAYER || trap_Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_ModelIndex( SP_PODIUM_MODEL );
	}

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( restart );
	}

	G_RemapTeamShaders();

	//elimination:
	level.roundNumber = 1;
	level.roundNumberStarted = 0;
	level.roundStartTime = level.time+g_elimination_warmup.integer*1000;
	level.roundRespawned = qfalse;
	level.eliminationSides = rand()%2; //0 or 1

	//Challenges:
	level.teamSize = 0;
	level.hadBots = qfalse;

	if(g_gametype.integer == GT_DOUBLE_D)
		Team_SpawnDoubleDominationPoints();

	if(g_gametype.integer == GT_DOMINATION ){
		level.dom_scoreGiven = 0;
		for(i=0;i<MAX_DOMINATION_POINTS;i++)
			level.pointStatusDom[i] = TEAM_NONE;
		level.domination_points_count = 0; //make sure its not too big
	}

        PlayerStoreInit();

        //Set vote flags
        {
            int voteflags=0;
            if( allowedVote("map_restart") )
                voteflags|=VF_map_restart;

            if( allowedVote("map") )
                voteflags|=VF_map;

            if( allowedVote("clientkick") )
                voteflags|=VF_clientkick;

            if( allowedVote("shuffle") )
                voteflags|=VF_shuffle;

            if( allowedVote("nextmap") )
                voteflags|=VF_nextmap;

            if( allowedVote("g_gametype") )
                voteflags|=VF_g_gametype;

            if( allowedVote("g_doWarmup") )
                voteflags|=VF_g_doWarmup;

            if( allowedVote("timelimit") )
                voteflags|=VF_timelimit;

            if( allowedVote("fraglimit") )
                voteflags|=VF_fraglimit;

            if( allowedVote("custom") )
                voteflags|=VF_custom;

            trap_Cvar_Set("voteflags",va("%i",voteflags));
        }


	G_PingEqualizerReset();
	if (!G_EQPingReapply()) {
		G_EQPingReset();
	}

	if (g_teamslocked.integer > 0 ) {
		level.RedTeamLocked = qtrue;
		level.BlueTeamLocked = qtrue;
		level.FFALocked = qtrue;
		trap_Cvar_Set("g_teamslocked",va("%i", g_teamslocked.integer - 1));
	}

	if (!restart && g_ra3forceArena.integer != -1) {
		trap_Cvar_Set("g_ra3forceArena","-1");
	}

	if (g_ra3nextForceArena.integer != -1) {
		// use trap_Cvar_VariableIntegerValue so we get the recently set value already
		if (g_ra3compat.integer &&  trap_Cvar_VariableIntegerValue("g_ra3maxArena")
				&& G_RA3ArenaAllowed(g_ra3nextForceArena.integer)) {
			trap_Cvar_Set("g_ra3forceArena",va("%i", g_ra3nextForceArena.integer));
		}
		trap_Cvar_Set("g_ra3nextForceArena", "-1");
	}

	if (g_autoClans.integer) {
		G_LoadClans();
	}
}



/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart ) {
        G_Printf ("==== ShutdownGame ====\n");

	if ( level.logFile ) {
		G_LogPrintf("ShutdownGame:\n" );
		G_LogPrintf("------------------------------------------------------------\n" );
		trap_FS_FCloseFile( level.logFile );
                level.logFile = 0;
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();
	
	//KK-OAX Admin Cleanup
    G_admin_cleanup( );
    G_admin_namelog_cleanup( );

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}
}



//===================================================================

void QDECL Com_Error ( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	G_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

        G_Printf ("%s", text);
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/


static void QueueJoinPlayer(gentity_t *ent, char *team) {
	ent->client->pers.joinedByTeamQueue = level.time;
	SetTeam_Force( ent, team, NULL, qtrue );
	if (level.time - ent->client->pers.queueJoinTime > 2000) {
		// alert player that he was pulled into the game
		trap_SendServerCommand( ent - g_entities, va("qjoin"));
	}
}

/*
 * True when we still expect players from the last round to be connecting, and
 * we don't want spectators / queued players to replace them
 */
qboolean QueueIsConnectingPhase(void) {
	return g_teamForceQueue.integer && level.warmupTime != 0 && level.time < 20000;
}

/*
=============
AddQueuedPlayers

Add a queued players in team games
=============
*/
qboolean AddQueuedPlayers( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine[2];
	gclient_t	*nextInLineBlue;
	gclient_t	*nextInLineRed;
	int		counts[TEAM_NUM_TEAMS];

	if (g_gametype.integer < GT_TEAM || g_ffa_gt == 1) {
		return qfalse;
	}

	if (!g_teamForceQueue.integer) {
		return qfalse;
	}

	if (level.BlueTeamLocked && level.RedTeamLocked) {
		return qfalse;
	}

	if ( g_maxGameClients.integer > 0 && 
			level.numNonSpectatorClients >= g_maxGameClients.integer ) {
		return qfalse;
	}

	// never change during intermission
	if ( level.intermissiontime ) {
		return qfalse;
	}

	nextInLine[0] = NULL;
	nextInLine[1] = NULL;
	nextInLineBlue = NULL;
	nextInLineRed = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD || 
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( client->sess.spectatorGroup == SPECTATORGROUP_AFK ||
				client->sess.spectatorGroup == SPECTATORGROUP_SPEC) {
			continue;
		}

		if (client->sess.spectatorGroup == SPECTATORGROUP_QUEUED) {
			if (!nextInLine[0]) {
				nextInLine[0] = client;
			} else if (!nextInLine[1]) {
				nextInLine[1] = client;
				if (nextInLine[0]->sess.spectatorNum > nextInLine[1]->sess.spectatorNum) {
					// make sure nextInLine[0] is the one who is queued longer
					gclient_t *cl = nextInLine[0]; 
					nextInLine[0] = nextInLine[1];
					nextInLine[1] = cl;
				}
			} else if (nextInLine[0]->sess.spectatorNum < client->sess.spectatorNum) {
				nextInLine[1] = nextInLine[0];
				nextInLine[0] = client;
			} else if (nextInLine[1]->sess.spectatorNum < client->sess.spectatorNum) {
				nextInLine[1] = client;
			}
		} else if (client->sess.spectatorGroup == SPECTATORGROUP_QUEUED_BLUE
				&& (!nextInLineBlue || nextInLineBlue->sess.spectatorNum < client->sess.spectatorNum)) {
			nextInLineBlue = client;
		} else if (client->sess.spectatorGroup == SPECTATORGROUP_QUEUED_RED
				&& (!nextInLineRed || nextInLineRed->sess.spectatorNum < client->sess.spectatorNum)) {
			nextInLineRed = client;
		}
	}

	counts[TEAM_BLUE] = TeamCountExt( -1, TEAM_BLUE, qfalse, QueueIsConnectingPhase());
	counts[TEAM_RED] = TeamCountExt( -1, TEAM_RED, qfalse, QueueIsConnectingPhase());
	if (counts[TEAM_BLUE] < counts[TEAM_RED] && !level.BlueTeamLocked) {
		// one to blue
		if (nextInLine[0] && nextInLineBlue) {
			if (nextInLine[0]->sess.spectatorNum > nextInLineBlue->sess.spectatorNum) {
				QueueJoinPlayer( &g_entities[ nextInLine[0] - level.clients ], "b");
				return qtrue;
			} else {
				QueueJoinPlayer( &g_entities[ nextInLineBlue - level.clients ], "b");
				return qtrue;
			}
		} else if (nextInLine[0]) {
			QueueJoinPlayer( &g_entities[ nextInLine[0] - level.clients ], "b");
			return qtrue;
		} else if (nextInLineBlue) {
			QueueJoinPlayer( &g_entities[ nextInLineBlue - level.clients ], "b");
			return qtrue;
		}
	} else if (counts[TEAM_RED] < counts[TEAM_BLUE] && !level.RedTeamLocked) {
		// one to red
		if (nextInLine[0] && nextInLineRed) {
			if (nextInLine[0]->sess.spectatorNum > nextInLineRed->sess.spectatorNum) {
				QueueJoinPlayer( &g_entities[ nextInLine[0] - level.clients ], "r");
				return qtrue;
			} else {
				QueueJoinPlayer( &g_entities[ nextInLineRed - level.clients ], "r");
				return qtrue;
			}
		} else if (nextInLine[0]) {
			QueueJoinPlayer( &g_entities[ nextInLine[0] - level.clients ], "r");
			return qtrue;
		} else if (nextInLineRed) {
			QueueJoinPlayer( &g_entities[ nextInLineRed - level.clients ], "r");
			return qtrue;
		}
	} else {
		// join a pair
		int freecount = 0;
		for (i = 0; i < 2; ++i) {
			if (nextInLine[i]) {
				++freecount;
			} else {
				break;
			}
		}
		if (!nextInLineBlue && freecount) {
			nextInLineBlue = nextInLine[0];
			freecount--;
			nextInLine[0] = nextInLine[1];
		}
		if (!nextInLineRed && freecount) {
			nextInLineRed = nextInLine[0];
			freecount--;
			nextInLine[0] = nextInLine[1];
		}
		if (!(nextInLineBlue && nextInLineRed)) {
			return qfalse;
		}
		for (i = 0; i < freecount; ++i) {
			if (nextInLineBlue->sess.spectatorNum < nextInLineRed->sess.spectatorNum) {
				if (nextInLine[i]->sess.spectatorNum > nextInLineBlue->sess.spectatorNum) {
					nextInLineBlue = nextInLine[i];
				}
			} else {
				if (nextInLine[i]->sess.spectatorNum > nextInLineRed->sess.spectatorNum) {
					nextInLineRed = nextInLine[i];
				}
			}
		}
		QueueJoinPlayer( &g_entities[ nextInLineBlue - level.clients ], "b");
		QueueJoinPlayer( &g_entities[ nextInLineRed - level.clients ], "r");
		return qtrue;
	}
	return qfalse;

}

gentity_t *G_FindPlayerLastJoined(int team) {
	int i;
	int lastEnterTime = -1;
	gentity_t *lastEnterClient = NULL;
	gentity_t *lastClient = NULL;
	int enterTime;

	for( i = 0; i < level.numPlayingClients; i++ ){
		gclient_t *cl = &level.clients[ level.sortedClients[i] ];
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( cl->sess.sessionTeam != team ) {
			continue;
		}

		enterTime = cl->pers.enterTime;
		if ( enterTime > lastEnterTime) {
			lastEnterTime = enterTime;
			lastEnterClient = &g_entities[level.sortedClients[i]];
		}
		lastClient = &g_entities[level.sortedClients[i]];
	}
	if (lastEnterTime - level.startTime < 2000) {
		// every client entered when the game started (within 2s), so
		// it does not make much sense to pick the last client to join
		// instead, pick the client with the lowest score
		return lastClient;
	}
	return lastEnterClient;
}

void G_UnqueuePlayers( void ) {
	int i;
	gclient_t *client;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}

		switch ( client->sess.spectatorGroup ) {
			case SPECTATORGROUP_QUEUED_BLUE:
			case SPECTATORGROUP_QUEUED_RED:
			case SPECTATORGROUP_QUEUED:
				client->sess.spectatorGroup = SPECTATORGROUP_SPEC;
				SendSpectatorGroup(&g_entities[i]);
				break;
			default:
				break;
		}
	}

}

void CheckTeamBalance( void ) {
	static qboolean queuesClean = qfalse;
	int		counts[TEAM_NUM_TEAMS];
	int balance;
	int largeTeam;
	gentity_t *player;

	if (g_gametype.integer < GT_TEAM || g_ffa_gt == 1) {
		return;
	}

	if (!g_teamForceQueue.integer) {
		if (!queuesClean) {
			// make sure that players don't stay in the queues when
			// the queues aren't active
			G_UnqueuePlayers();
			queuesClean = qtrue;
		}
		return;
	} else {
		queuesClean = qfalse;
	}

	// Make sure the queue is emptied before trying to balance
	if (AddQueuedPlayers()) {
		return;
	}

	if (!g_teamBalance.integer) {
		// only use queues to join, without re-balancing
		return;
	}

	if (level.BlueTeamLocked || level.RedTeamLocked) {
		return ;
	}

	if ( level.intermissiontime 
			|| level.time < level.startTime + 3000) {
		return;
	}

	counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE, qfalse);
	counts[TEAM_RED] = TeamCount( -1, TEAM_RED, qfalse);
	balance = counts[TEAM_BLUE] - counts[TEAM_RED];
	if (abs(balance) == 1 && !(counts[TEAM_BLUE] && counts[TEAM_RED])) {
		// don't try to balance 1 v bot games
		level.teamBalanceTime = 0;
		return;
	}

	if (balance == 0) {
		// is balanced, do nothing
		if (level.teamBalanceTime) {
			trap_SendServerCommand( -1, 
					va("print \"" S_COLOR_CYAN "Server: " S_COLOR_GREEN "Teams fixed!\n"));
		}
		level.teamBalanceTime = 0;
		return;
	} 
	largeTeam = balance < 0 ? TEAM_RED : TEAM_BLUE;
	if (level.teamBalanceTime == 0) {
		if (g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION) {
			// as soon as we're in active warmup
			if(level.roundNumber > level.roundNumberStarted && level.time > level.roundStartTime - 1000 * g_elimination_activewarmup.integer) {
				trap_SendServerCommand( -1, 
						va("cp \"%s" S_COLOR_YELLOW" has more players, balancing now!\n",
							largeTeam == TEAM_RED ? S_COLOR_RED "Red" : S_COLOR_BLUE "Blue"));
				level.teamBalanceTime = level.roundStartTime - 2000;
				if (level.teamBalanceTime - level.time > 4000) {
					level.teamBalanceTime = level.time + 4000;
				}
			} else {
				return;
			}
		} else {
			level.teamBalanceTime = level.time + g_teamBalanceDelay.integer * 1000;
			if (abs(balance) <= 2 && (player = G_FindPlayerLastJoined(largeTeam)) != NULL) {
				// if the imbalance is only 1 or two, announce who is going to be queued/switched
				trap_SendServerCommand( -1, 
						va("print \"" S_COLOR_CYAN "Server: " S_COLOR_WHITE "%s"
							S_COLOR_WHITE " will be %s for balance!\n",
							player->client->pers.netname,
							abs(balance) == 2 ? "switched" : "queued"
							)
						  );
			} 
			trap_SendServerCommand( -1, 
					va("cp \"%s" S_COLOR_YELLOW" has more players, balancing in "
						S_COLOR_RED "%is" S_COLOR_YELLOW"!\n",
						largeTeam == TEAM_RED ? S_COLOR_RED "Red" : S_COLOR_BLUE "Blue",
						g_teamBalanceDelay.integer
					  ));
			return;
		}
	} 
	if (g_gametype.integer != GT_ELIMINATION && g_gametype.integer != GT_CTF_ELIMINATION 
			&&level.teamBalanceTime - level.time == 5000 && g_teamBalanceDelay.integer >= 15) {
		trap_SendServerCommand( -1, 
				va("cp \"" S_COLOR_YELLOW "Balancing in " S_COLOR_RED "5s" S_COLOR_YELLOW"!\n"));
	}
	if (level.teamBalanceTime > level.time) {
		return;
	} else if (level.teamBalanceTime > INT_MIN) {
		trap_SendServerCommand( -1, 
				va("print \"" S_COLOR_CYAN "Server: " S_COLOR_RED "Balancing teams!\n"));
		// make sure we only print this message once:
		level.teamBalanceTime = INT_MIN;
	}


	// only balance one player per frame to try and avoid command overflows
	player = G_FindPlayerLastJoined(largeTeam);
	if (!player) {
		level.teamBalanceTime = 0;
		return;
	}
	// suppress excess messages
	level.shuffling_teams = qtrue;
	if (abs(balance) > 1) {
		SetTeam(player, largeTeam == TEAM_BLUE ? "r" : "b");
	} else {
		// if the imbalance is only 1, the player will be queued so we
		// put him in his own team's queue instead of the other team's
		SetTeam(player, largeTeam == TEAM_BLUE ? "b" : "r");
		if (player->client->sess.sessionTeam == TEAM_SPECTATOR
				&& (player->client->sess.spectatorGroup == SPECTATORGROUP_QUEUED_BLUE
				    || player->client->sess.spectatorGroup == SPECTATORGROUP_QUEUED_RED)) {
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " was queued for balance.\n\"",
						player->client->pers.netname));
		}
	}
	level.shuffling_teams = qfalse;
}


/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
void AddTournamentPlayer( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 2 ) {
		return;
	}

	if (level.numNonSpectatorClients >= 2) {
		return;
	}

	// never change during intermission
	if ( level.intermissiontime ) {
		return;
	}

	nextInLine = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD || 
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( client->sess.spectatorGroup == SPECTATORGROUP_AFK ||
				client->sess.spectatorGroup == SPECTATORGROUP_SPEC) {
			continue;
		}

		if(!nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum) {
			nextInLine = client;
		}
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f" );
}

/*
=======================
AddTournamentQueue
	  	 
Add client to end of tournament queue
=======================
*/
void AddTournamentQueue(gclient_t *client)
{
    int index;
    gclient_t *curclient;
    for(index = 0; index < level.maxclients; index++)
    {
        curclient = &level.clients[index];
        if(curclient->pers.connected != CON_DISCONNECTED)
        {
            if(curclient == client)
		    curclient->sess.spectatorNum = 0;
            else if(curclient->sess.sessionTeam == TEAM_SPECTATOR)
		    curclient->sess.spectatorNum++;
        }
    }
}

/*
=======================
AddTournamentQueueFront
	  	 
Add client to front of tournament queue
=======================
*/
void AddTournamentQueueFront(gclient_t *client)
{
    int index;
    gclient_t *curclient;
    int maxSpectatorNum = 0;
    for(index = 0; index < level.maxclients; index++)
    {
        curclient = &level.clients[index];
        if(curclient->pers.connected != CON_DISCONNECTED)
        {
            if(curclient != client 
			    && curclient->sess.sessionTeam == TEAM_SPECTATOR
			    && curclient->sess.spectatorNum > maxSpectatorNum) {
		    maxSpectatorNum = curclient->sess.spectatorNum;
	    }
	}
    }
    client->sess.spectatorNum = maxSpectatorNum + 1;
}

/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
void RemoveTournamentLoser( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[1];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a queued spectator
	SetTeam( &g_entities[ clientNum ], "q" );
}

/*
=======================
RemoveTournamentWinner
=======================
*/
void RemoveTournamentWinner( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[0];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a queued spectator
	SetTeam( &g_entities[ clientNum ], "q" );
}

/*
=======================
AdjustTournamentScores
=======================
*/
void AdjustTournamentScores( void ) {
	int			clientNum;

	clientNum = level.sortedClients[0];
	if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
		level.clients[ clientNum ].sess.wins++;
		ClientUserinfoChanged( clientNum );
	}

	clientNum = level.sortedClients[1];
	if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
		level.clients[ clientNum ].sess.losses++;
		ClientUserinfoChanged( clientNum );
	}

}

/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b ) {
	gclient_t	*ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if ( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0  ) {
		return -1;
	}

	// then connecting clients
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}

	// afk spectators
	if ( ca->sess.spectatorGroup == SPECTATORGROUP_AFK ) {
		return 1;
	}
	if ( cb->sess.spectatorGroup == SPECTATORGROUP_AFK ) {
		return -1;
	}

	// then spectators
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ca->sess.spectatorNum > cb->sess.spectatorNum ) {
			return -1;
		}
		if ( ca->sess.spectatorNum < cb->sess.spectatorNum ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		return -1;
	}

        //In elimination and CTF elimination, sort dead players last
        //if((g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION)
        //        && level.roundNumber==level.roundNumberStarted && (ca->isEliminated != cb->isEliminated)) {
        //    if( ca->isEliminated )
        //        return 1;
        //    if( cb->isEliminated )
        //        return -1;
        //} // confusing

	// then sort by score
	if ( ca->ps.persistant[PERS_SCORE]
		> cb->ps.persistant[PERS_SCORE] ) {
		return -1;
	}
	if ( ca->ps.persistant[PERS_SCORE]
		< cb->ps.persistant[PERS_SCORE] ) {
		return 1;
	}
	return 0;
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		newScore;
        int             humanplayers;
	gclient_t	*cl;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
        humanplayers = 0; // don't count bots
	for ( i = 0; i < TEAM_NUM_TEAMS; i++ ) {
		level.numteamVotingClients[i] = 0;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

                        //We just set humanplayers to 0 during intermission
                        if ( !level.intermissiontime && level.clients[i].pers.connected == CON_CONNECTED && !(g_entities[i].r.svFlags & SVF_BOT) ) {
                            humanplayers++;
                        }

			if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR ) {
                                level.numNonSpectatorClients++;
				// decide if this should be auto-followed
				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					level.numPlayingClients++;
					if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
						if ( level.clients[i].sess.sessionTeam == TEAM_RED )
							level.numteamVotingClients[0]++;
						else if ( level.clients[i].sess.sessionTeam == TEAM_BLUE )
							level.numteamVotingClients[1]++;
					}
					if ( level.follow1 == -1 ) {
						level.follow1 = i;
					} else if ( level.follow2 == -1 ) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	qsort( level.sortedClients, level.numConnectedClients, 
		sizeof(level.sortedClients[0]), SortRanks );

	// set the rank value for all clients that are connected and not spectators
	if ( g_gametype.integer >= GT_TEAM && g_ffa_gt!=1) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for ( i = 0;  i < level.numConnectedClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			if ( level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 2;
			} else if ( level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 0;
			} else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
	} else {	
		rank = -1;
		score = 0;
		for ( i = 0;  i < level.numPlayingClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			newScore = cl->ps.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[ level.sortedClients[i-1] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if ( g_gametype.integer == GT_SINGLE_PLAYER && level.numPlayingClients == 1 ) {
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if ( g_gametype.integer >= GT_TEAM && g_ffa_gt!=1) {
		trap_SetConfigstring( CS_SCORES1, va("%i", level.teamScores[TEAM_RED] ) );
		trap_SetConfigstring( CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE] ) );
	} else {
		if ( level.numConnectedClients == 0 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", SCORE_NOT_PRESENT) );
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else if ( level.numConnectedClients == 1 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else {
			trap_SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va("%i", level.clients[ level.sortedClients[1] ].ps.persistant[PERS_SCORE] ) );
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission, send the new info to everyone
	if ( level.intermissiontime ) {
		SendScoreboardMessageToAllClients();
	}
        
        if(g_humanplayers.integer != humanplayers) //Presume all spectators are humans!
            trap_Cvar_Set( "g_humanplayers", va("%i", humanplayers) );
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			//DeathmatchScoreboardMessage( g_entities + i, (g_usesRatVM.integer > 0 || G_MixedClientHasRatVM( &level.clients[i])));
			DeathmatchScoreboardMessageAuto( g_entities + i);
			if (g_gametype.integer == GT_ELIMINATION ||
					g_gametype.integer == GT_CTF_ELIMINATION ||
					g_gametype.integer == GT_LMS) {
				EliminationMessage( g_entities + i );
			}
		}
	}
}

/*
========================
SendElimiantionMessageToAllClients

Used to send information important to Elimination
========================
*/
void SendEliminationMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			EliminationMessage( g_entities + i );
		}
	}
}

/*
========================
SendDDtimetakenMessageToAllClients

Do this if a team just started dominating.
========================
*/
void SendDDtimetakenMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DoubleDominationScoreTimeMessage( g_entities + i );
		}
	}
}

/*
========================
SendTreasureHuntMessageToAllClients

Used to send information important to Treasure Hunter
========================
*/
void SendTreasureHuntMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			TreasureHuntMessage( g_entities + i );
		}
	}
}

/*
========================
SendAttackingTeamMessageToAllClients

Used for CTF Elimination oneway
========================
*/
void SendAttackingTeamMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			AttackingTeamMessage( g_entities + i );
		}
	}
}

/*
========================
SendDominationPointsStatusMessageToAllClients

Used for Standard domination
========================
*/
void SendDominationPointsStatusMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DominationPointStatusMessage( g_entities + i );
		}
	}
}
/*
========================
SendYourTeamMessageToTeam

Tell all players on a given team who there allies are. Used for VoIP
========================
*/
void SendYourTeamMessageToTeam( team_t team ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED && level.clients[ i ].sess.sessionTeam == team ) {
			YourTeamMessage( g_entities + i );
		}
	}
}


/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission( gentity_t *ent ) {
	// take out of follow mode if needed
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		StopFollowing( ent );
	}

	if (g_ra3compat.integer && g_ra3maxArena.integer >= 0 
			&& G_RA3ArenaAllowed(ent->client->pers.arenaNum)) {
		FindIntermissionPointArena(ent->client->pers.arenaNum, ent->s.origin, ent->client->ps.viewangles);
		VectorCopy( ent->s.origin, ent->client->ps.origin );
	} else {
		FindIntermissionPoint();
		// move to the spot
		VectorCopy( level.intermission_origin, ent->s.origin );
		VectorCopy( level.intermission_origin, ent->client->ps.origin );
		VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	}
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) );

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint( void ) {
	gentity_t	*ent, *target;
	vec3_t		dir;

	// find the intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		SelectSpawnPoint ( vec3_origin, level.intermission_origin, level.intermission_angle );
	} else {
		VectorCopy (ent->s.origin, level.intermission_origin);
		VectorCopy (ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}

}

void FindIntermissionPointArena( int arenaNum, vec3_t origin, vec3_t angles ) {
	gentity_t	*ent, *target;
	vec3_t		dir;

	ent = NULL;
	// find the intermission spot
	while ((ent = G_Find (ent, FOFS(classname), "info_player_intermission")) != NULL) {
		if (ent->arenaNum == arenaNum) {
			break;
		}
	}
	if (!ent) {
		ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	}
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		SelectSpawnPointArena ( arenaNum, vec3_origin, level.intermission_origin, level.intermission_angle );
	} else {
		VectorCopy (ent->s.origin, origin);
		VectorCopy (ent->s.angles, angles);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, origin, dir );
				vectoangles( dir, angles );
			}
		}
	}

}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void ) {
	int			i;
	gentity_t	*client;

	if ( level.intermissiontime ) {
		return;		// already active
	}

	// if in tournement mode, change the wins / losses
	if ( g_gametype.integer == GT_TOURNAMENT ) {
		AdjustTournamentScores();
	}

	level.intermissiontime = level.time;
	// move all clients to the intermission point
	for (i=0 ; i< level.maxclients ; i++) {
		client = g_entities + i;
		if (!client->inuse)
			continue;
		// respawn if dead
		if (client->health <= 0) {
			ClientRespawn(client);
		}
		MoveClientToIntermission( client );
	}
#ifdef MISSIONPACK
	if (g_singlePlayer.integer) {
		trap_Cvar_Set("ui_singlePlayerActive", "0");
		UpdateTournamentInfo();
	}
#else
	// if single player game
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		UpdateTournamentInfo();
		SpawnModelsOnVictoryPads();
	}
#endif
	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();

}


/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar 

=============
*/
void ExitLevel (void) {
	int		i;
	gclient_t *cl;
	char nextmap[MAX_STRING_CHARS];
	char d1[MAX_STRING_CHARS];
        char	serverinfo[MAX_INFO_STRING];

	//bot interbreeding
	BotInterbreedEndMatch();

	// if we are running a tournement map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if ( g_gametype.integer == GT_TOURNAMENT  ) {
		if ( !level.restarted ) {
			RemoveTournamentLoser();
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			level.changemap = NULL;
			level.intermissiontime = 0;
		}
		return;	
	}

	trap_Cvar_VariableStringBuffer( "nextmap", nextmap, sizeof(nextmap) );
	trap_Cvar_VariableStringBuffer( "d1", d1, sizeof(d1) );
        
        trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

        //Here the game finds the nextmap if g_autonextmap is set
        if(g_autonextmap.integer ) {
            char filename[MAX_FILEPATH];
            fileHandle_t file,mapfile;
            //Look in g_mappools.string for the file to look for maps in
            Q_strncpyz(filename,Info_ValueForKey(g_mappools.string, va("%i",g_gametype.integer)),MAX_FILEPATH);
            //If we found a filename:
            if(filename[0]) {
                //Read the file:
                /*int len =*/ trap_FS_FOpenFile(filename, &file, FS_READ);
                if(!file)
                    trap_FS_FOpenFile(va("%s.org",filename), &file, FS_READ);
                if(file) {
                    char  buffer[4*1024]; // buffer to read file into
                    char mapnames[1024][20]; // Array of mapnames in the map pool
                    char *pointer;
                    int choice, count=0; //The random choice from mapnames and count of mapnames
                    int i;
                    memset(&buffer,0,sizeof(buffer));
                    trap_FS_Read(&buffer,sizeof(buffer)-1,file);
                    pointer = buffer;
                    while ( qtrue ) {
                        Q_strncpyz(mapnames[count],COM_Parse( &pointer ),20);
                        if ( !mapnames[count][0] ) {
                            break;
                        }
                        G_Printf("Mapname in mappool: %s\n",mapnames[count]);
                        count++;
                    }
                    trap_FS_FCloseFile(file);
                    //It is possible that the maps in the file read are flawed, so we try up to ten times:
                    for(i=0;i<10;i++) {
                        choice = (count > 0)? rand()%count : 0;
                        if(Q_strequal(mapnames[choice],Info_ValueForKey(serverinfo,"mapname")))
                            continue;
                        //Now check that the map exists:
                        trap_FS_FOpenFile(va("maps/%s.bsp",mapnames[choice]),&mapfile,FS_READ);
                        if(mapfile) {
                            G_Printf("Picked map number %i - %s\n",choice,mapnames[choice]);
                            Q_strncpyz(nextmap,va("map %s",mapnames[choice]),sizeof(nextmap));
                            trap_Cvar_Set("nextmap",nextmap);
                            trap_FS_FCloseFile(mapfile);
                            break;
                        }
                    }
                }
            }
        }

	if( !Q_stricmp( nextmap, "map_restart 0" ) && Q_stricmp( d1, "" ) ) {
		trap_Cvar_Set( "nextmap", "vstr d2" );
		trap_SendConsoleCommand( EXEC_APPEND, "vstr d1\n" );
	} else {
		trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
	}

	level.changemap = NULL;
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i=0 ; i< g_maxclients.integer ; i++) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}

}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024];
	int			min, tens, sec;

	sec = level.time / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf( string, sizeof(string), "%3i:%i%i ", min, tens, sec );

	va_start( argptr, fmt );
	Q_vsnprintf(string + 7, sizeof(string) - 7, fmt, argptr);
	va_end( argptr );

	if ( g_dedicated.integer ) {
		G_Printf( "%s", string + 7 );
	}

	if ( !level.logFile ) {
		return;
	}

	trap_FS_Write( string, strlen( string ), level.logFile );
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string ) {
	int				i, numSorted;
	gclient_t		*cl;
#ifdef MISSIONPACK
	qboolean won = qtrue;
 #endif
	G_LogPrintf( "Exit: %s\n", string );

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring( CS_INTERMISSION, "1" );

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if ( numSorted > 32 ) {
		numSorted = 32;
	}

	if ( g_gametype.integer >= GT_TEAM && g_ffa_gt!=1) {
		G_LogPrintf( "red:%i  blue:%i\n",
			level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE] );
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( cl->pers.connected == CON_CONNECTING ) {
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		G_LogPrintf( "score: %i  ping: %i  client: %i %s\n", cl->ps.persistant[PERS_SCORE], ping, level.sortedClients[i],	cl->pers.netname );
#ifdef MISSIONPACK
		if (g_singlePlayer.integer && g_gametype.integer == GT_TOURNAMENT) {
			if (g_entities[cl - level.clients].r.svFlags & SVF_BOT && cl->ps.persistant[PERS_RANK] == 0) {
				won = qfalse;
			}
		}
#endif

	}

#ifdef MISSIONPACK
	if (g_singlePlayer.integer) {
		if (g_gametype.integer >= GT_CTF && g_ffa_gt==0) {
			won = level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE];
		}
		trap_SendConsoleCommand( EXEC_APPEND, (won) ? "spWin\n" : "spLose\n" );
	}
#endif


}

qboolean CheckNextmapVote( void ) {
	int maxVotes;
	int runnerup;
	int numMaxVotes;
	int numVotes;
	int i,j;
	int mapPick;
	char *map = NULL;
	char nextmap[MAX_STRING_CHARS];

	if (!g_nextmapVote.integer && !level.nextMapVoteManual) {
		return qtrue;
	}

	if (level.nextMapVoteExecuted && level.time > level.nextMapVoteTime + 1000) {
		// vote somehow failed to execute, exit level normally
		Com_Printf("Nextmapvote failed to execute\n");
		return qtrue;
	} else if (level.nextMapVoteTime == 0) {
		// start vote
		Com_Printf("Starting nextmapvote\n");
		return !SendNextmapVoteCommand();
	} 
	
	// check if vote is already decided

	maxVotes = 0;
	numMaxVotes = 0;
	runnerup = 0;
	numVotes = 0;
	for (i = 0; i < NEXTMAPVOTE_NUM_MAPS; ++i) {
		numVotes += level.nextmapVotes[i];
		if (level.nextmapVotes[i] <= 0) {
			continue;
		} else if (level.nextmapVotes[i] > maxVotes) {
			runnerup = maxVotes;
			maxVotes = level.nextmapVotes[i];
			numMaxVotes = 1;
		} else if (level.nextmapVotes[i] == maxVotes) {
			runnerup = maxVotes;
			numMaxVotes++;
		}
	}

	if (level.nextMapVoteTime > level.time
			&& !(maxVotes > runnerup + (level.nextMapVoteClients - numVotes))
			&& level.nextMapVoteClients - numVotes > 0) {
		// vote still in progress
		return qfalse;
	}

	// vote ended, execute result:
	level.nextMapVoteExecuted = 1;

	if (numMaxVotes <= 0) {
		// nobody voted, just go to the next map in rotation
		return qtrue;
	}

	mapPick = rand() % numMaxVotes;
	j = 0;
	for (i = 0; i < NEXTMAPVOTE_NUM_MAPS; ++i) {
		if (level.nextmapVotes[i] == maxVotes) {
			if ( j == mapPick) {
				mapPick = i;
				break;
			}
			j++;
		}
	}
	map = level.nextmapVoteMaps[mapPick];

	// special case for map changes, we want to reset the nextmap setting
	// this allows a player to change maps, but not upset the map rotation
	if(!allowedMap(map)){
		trap_SendServerCommand( -1, "print \"Map is not available.\n\"" );
		return qtrue;
	}

	Com_Printf("NextMapVote: switching to map %s\n", map);
	trap_Cvar_VariableStringBuffer( "nextmap", nextmap, sizeof(nextmap) );
	if (*nextmap) {
		trap_SendConsoleCommand( EXEC_APPEND, va("map \"%s\"; set nextmap \"%s\"\n", map, nextmap ));
	} else {
		trap_SendConsoleCommand( EXEC_APPEND, va("map \"%s\"\n", map));
	}
	return qfalse;
}


/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void ) {
	int			ready, notReady, playerCount;
	int			i;
	gclient_t	*cl;
	int			readyMask;

	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		return;
	}

	if (level.nextMapVoteManual) {
		// nextmapvote was manually started, so don't care if anyone is
		// ready to exit
		if (!CheckNextmapVote()) {
			return;
		}
		ExitLevel();
	}

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask = 0;
        playerCount = 0;
	for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			continue;
		}

                playerCount++;
		if ( cl->readyToExit ) {
			ready++;
			if ( i < 16 ) {
				readyMask |= 1 << i;
			}
		} else {
			notReady++;
		}
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	// never exit in less than five seconds
	if ( level.time < level.intermissiontime + 5000 ) {
		return;
	}

	// only test ready status when there are real players present
	if ( playerCount > 0 ) {
		// if nobody wants to go, clear timer
		if ( !ready ) {
			level.readyToExit = qfalse;
			return;
		}

		// if everyone wants to go, go now
		if ( !notReady ) {
			if (!CheckNextmapVote()) {
				return;
			}
			ExitLevel();
			return;
		}
	}

	// the first person to ready starts the ten second timeout
	if ( !level.readyToExit ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if ( level.time < level.exitTime + 10000 ) {
		return;
	}

	if (!CheckNextmapVote()) {
		return;
	}
	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void ) {
	int		a, b;

	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}
        
        //Sago: In Elimination and Oneway Flag Capture teams must win by two points.
        if ( g_gametype.integer == GT_ELIMINATION || 
                (g_gametype.integer == GT_CTF_ELIMINATION && g_elimination_ctf_oneway.integer)) {
            return (level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] /*|| 
                    level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE]+1 ||
                    level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE]-1*/);
        }
	
	if ( g_gametype.integer >= GT_TEAM && g_ffa_gt!=1) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules( void ) {
 	int			i;
	gclient_t	*cl;
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit ();
		return;
	} else {
            //sago: Find the reason for this to be neccesary.
            for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
                }
                cl->ps.stats[STAT_CLIENTS_READY] = 0;
            }
        }

	if ( level.intermissionQueued ) {
#ifdef MISSIONPACK
		int time = (g_singlePlayer.integer) ? SP_INTERMISSION_DELAY_TIME : INTERMISSION_DELAY_TIME;
		if ( level.time - level.intermissionQueued >= time ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
#else
		if ( level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
#endif
		return;
	}

	// check for sudden death
	if ( ScoreIsTied() && g_overtime.integer <= 0) {
		// always wait for sudden death
		return;
	}

	if ( g_timelimit.integer > 0 && !level.warmupTime ) {
		//if ( (level.time - level.startTime)/60000 >= g_timelimit.integer ) {
		if ( (level.time - level.startTime) >= g_timelimit.integer * 60000 + level.timeoutOvertime + g_overtime.integer * 60*1000 *  level.overtimeCount) {
			if (ScoreIsTied() && g_overtime.integer > 0) {
				level.overtimeCount++;
				int newtimelimit = g_timelimit.integer * 60 + level.overtimeCount * g_overtime.integer * 60;
				trap_SendServerCommand( -1, va("print \"" S_COLOR_YELLOW "%i minute%s" S_COLOR_CYAN " of overtime added. " 
						S_COLOR_CYAN "Game ends at " S_COLOR_YELLOW "%i:%02i\n\"", 
						g_overtime.integer,
						g_overtime.integer == 1 ? "" : "s",
						newtimelimit/60,
						newtimelimit - (newtimelimit/60)*60
						));
				trap_SendServerCommand( -1, va("cp \"" S_COLOR_YELLOW "%i minute%s" S_COLOR_CYAN " of overtime added\n" 
						S_COLOR_CYAN "Game ends at " S_COLOR_YELLOW "%i:%02i\n\"", 
						g_overtime.integer,
						g_overtime.integer == 1 ? "" : "s",
						newtimelimit/60,
						newtimelimit - (newtimelimit/60)*60
						));
			} else {
				trap_SendServerCommand( -1, "print \"Timelimit hit.\n\"");
				LogExit( "Timelimit hit." );
			}
			return;
		}
	}

	if (g_gametype.integer == GT_TOURNAMENT && level.tournamentForfeited) {
		LogExit("Match ended due to forfeit!\n");
		return;
	}

	if ( level.numPlayingClients < 2 ) {
		return;
	}

	if ( (g_gametype.integer < GT_CTF || g_ffa_gt>0 ) && g_fraglimit.integer ) {
		if ( level.teamScores[TEAM_RED] >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, "print \"Red hit the fraglimit.\n\"" );
			LogExit( "Fraglimit hit." );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, "print \"Blue hit the fraglimit.\n\"" );
			LogExit( "Fraglimit hit." );
			return;
		}

		for ( i=0 ; i< g_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->sess.sessionTeam != TEAM_FREE ) {
				continue;
			}

			if ( cl->ps.persistant[PERS_SCORE] >= g_fraglimit.integer ) {
				LogExit( "Fraglimit hit." );
				trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " hit the fraglimit.\n\"",
					cl->pers.netname ) );
				return;
			}
		}
	}

	if ( (g_gametype.integer >= GT_CTF && g_ffa_gt<1) && g_capturelimit.integer ) {

		if ( level.teamScores[TEAM_RED] >= g_capturelimit.integer ) {
			trap_SendServerCommand( -1, "print \"Red hit the capturelimit.\n\"" );
			LogExit( "Capturelimit hit." );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_capturelimit.integer ) {
			trap_SendServerCommand( -1, "print \"Blue hit the capturelimit.\n\"" );
			LogExit( "Capturelimit hit." );
			return;
		}
	}

	if ( (g_gametype.integer == GT_TREASURE_HUNTER ) ) {
		if (level.th_round == level.th_roundFinished && 
				level.th_roundFinished >= (g_treasureRounds.integer ? g_treasureRounds.integer : 1)) {
			trap_SendServerCommand( -1, "print \"Reached the round limit.\n\"" );
			LogExit( "Roundlimit hit." );
			return;
		}
	}
}

void ResetElimRoundStats(void) {
	int i;
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t *client = &level.clients[i];
		client->pers.elimRoundDmgDone = 0;
		client->pers.elimRoundDmgTaken = 0;
		client->pers.elimRoundKills = 0;
		client->pers.lastKilledByStrongMan = -1;
	}
}

//LMS - Last man Stading functions:
void StartLMSRound(void) {
	int		countsLiving;
	countsLiving = TeamLivingCount( -1, TEAM_FREE );
	if(countsLiving<2) {
		trap_SendServerCommand( -1, "print \"Not enough players to start the round\n\"");
		level.roundNumberStarted = level.roundNumber-1;
		level.roundStartTime = level.time+1000*g_elimination_warmup.integer;
		return;
	}

	//If we are enough to start a round:
	level.roundNumberStarted = level.roundNumber; //Set numbers

        G_LogPrintf( "LMS: %i %i %i: Round %i has started!\n", level.roundNumber, -1, 0, level.roundNumber );
        
	SendEliminationMessageToAllClients();
	EnableWeapons();

	ResetElimRoundStats();
}

void PrintElimRoundStats(void) {
	gclient_t *client;
	int maxKills = -1;
	int maxKillsClient = -1;
	int maxDG = -1;
	int maxDGClient = -1;
	int minDT = INT_MAX;
	int minDTClient = -1;
	int i;

	if (!g_usesRatVM.integer) {
		return;
	}

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];

		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		if (client->pers.elimRoundKills > maxKills 
				|| (client->pers.elimRoundKills == maxKills && maxKillsClient != -1 && client->pers.elimRoundDmgDone > level.clients[maxKillsClient].pers.elimRoundDmgDone)) {
			maxKills = client->pers.elimRoundKills;
			maxKillsClient = i;
		}

		if (client->pers.elimRoundDmgDone > maxDG) {
			maxDG = client->pers.elimRoundDmgDone;
			maxDGClient = i;
		}

		if (client->pers.elimRoundDmgTaken < minDT) {
			minDT = client->pers.elimRoundDmgTaken;
			minDTClient = i;
		}

	}

	if (maxKillsClient != -1) {
		trap_SendServerCommand(-1, va("print \"Most kills: %s " S_COLOR_RED "%i\n\"", 
					level.clients[maxKillsClient].pers.netname,
					maxKills));
	}
	if (minDTClient != -1 && g_gametype.integer != GT_LMS) {
		trap_SendServerCommand(-1, va("print\"Least Damage Taken: %s " S_COLOR_RED "%i\n\"", 
					level.clients[minDTClient].pers.netname,
					minDT));
	}
	if (maxDGClient != -1) {
		trap_SendServerCommand(-1, va("print\"Most Damage Given: %s " S_COLOR_RED "%i\n\"", 
					level.clients[maxDGClient].pers.netname,
					maxDG));
	}
}


//the elimination start function
void StartEliminationRound(void) {
	int		countsLiving[TEAM_NUM_TEAMS];
	countsLiving[TEAM_BLUE] = TeamLivingCount( -1, TEAM_BLUE );
	countsLiving[TEAM_RED] = TeamLivingCount( -1, TEAM_RED );
	if((countsLiving[TEAM_BLUE]==0) || (countsLiving[TEAM_RED]==0))
	{
		trap_SendServerCommand( -1, "print \"Not enough players to start the round\n\"");
		level.roundNumberStarted = level.roundNumber-1;
		level.roundRespawned = qfalse;
		//Remember that one of the teams is empty!
		level.roundRedPlayers = countsLiving[TEAM_RED];
		level.roundBluePlayers = countsLiving[TEAM_BLUE];
		level.roundStartTime = level.time+1000*g_elimination_warmup.integer;
		return;
	}
	
	//If we are enough to start a round:
	level.roundNumberStarted = level.roundNumber; //Set numbers
	level.roundRedPlayers = countsLiving[TEAM_RED];
	level.roundBluePlayers = countsLiving[TEAM_BLUE];
	if(g_gametype.integer == GT_CTF_ELIMINATION) {
		Team_ReturnFlag( TEAM_RED );
		Team_ReturnFlag( TEAM_BLUE );
	}
	level.elimBlueRespawnDelay = 0;
	level.elimRedRespawnDelay = 0;
        if(g_gametype.integer == GT_ELIMINATION) {
            G_LogPrintf( "ELIMINATION: %i %i %i: Round %i has started!\n", level.roundNumber, -1, 0, level.roundNumber );
        } else if(g_gametype.integer == GT_CTF_ELIMINATION) {
            G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: Round %i has started!\n", level.roundNumber, -1, -1, 4, level.roundNumber );
        }
	SendEliminationMessageToAllClients();
	if(g_elimination_ctf_oneway.integer)
		SendAttackingTeamMessageToAllClients(); //Ensure that evaryone know who should attack.
	EnableWeapons();
	G_SendTeamPlayerCounts();

	ResetElimRoundStats();
}

//things to do at end of round:
void EndEliminationRound(void)
{
	PrintElimRoundStats();
	DisableWeapons();
	level.roundNumber++;
	level.roundStartTime = level.time+1000*g_elimination_warmup.integer;
	SendEliminationMessageToAllClients();
        CalculateRanks();
	level.roundRespawned = qfalse;
	if(g_elimination_ctf_oneway.integer)
		SendAttackingTeamMessageToAllClients();
	G_SendTeamPlayerCounts();
}

//Things to do if we don't want to move the roundNumber
void RestartEliminationRound(void) {
	DisableWeapons();
	level.roundNumberStarted = level.roundNumber-1;
	level.roundStartTime = level.time+1000*g_elimination_warmup.integer;
        if(!level.intermissiontime)
            SendEliminationMessageToAllClients();
	level.roundRespawned = qfalse;
	if(g_elimination_ctf_oneway.integer)
		SendAttackingTeamMessageToAllClients();
}

//Things to do during match warmup
void WarmupEliminationRound(void) {
	EnableWeapons();
	level.roundNumberStarted = level.roundNumber-1;
	level.roundStartTime = level.time+1000*g_elimination_warmup.integer;
	SendEliminationMessageToAllClients();
	level.roundRespawned = qfalse;
	if(g_elimination_ctf_oneway.integer)
		SendAttackingTeamMessageToAllClients();
}

/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/

/*
CheckDoubleDomination
*/

void CheckDoubleDomination( void ) {
	if ( level.numPlayingClients < 1 ) {
		return;
	}

	if ( level.warmupTime != 0) {
            if( ((level.pointStatusA == TEAM_BLUE && level.pointStatusB == TEAM_BLUE) ||
                 (level.pointStatusA == TEAM_RED && level.pointStatusB == TEAM_RED)) &&
                    level.timeTaken + 10*1000 <= level.time ) {
                        Team_RemoveDoubleDominationPoints();
                        level.roundStartTime = level.time + 10*1000;
                        SendScoreboardMessageToAllClients();
            }
            return;
        }

	if(g_gametype.integer != GT_DOUBLE_D)
		return;

	//Don't score if we are in intermission. Both points might have been taken when we went into intermission
	if(level.intermissiontime)
		return;

	if(level.pointStatusA == TEAM_RED && level.pointStatusB == TEAM_RED && level.timeTaken + 10*1000 <= level.time) {
		//Red scores
		trap_SendServerCommand( -1, "print \"Red team scores!\n\"");
		AddTeamScore(level.intermission_origin,TEAM_RED,1);
                G_LogPrintf( "DD: %i %i %i: %s scores!\n", -1, TEAM_RED, 2, TeamName(TEAM_RED) );
		Team_ForceGesture(TEAM_RED);
		Team_DD_bonusAtPoints(TEAM_RED);
		Team_RemoveDoubleDominationPoints();
		//We start again in 10 seconds:
		level.roundStartTime = level.time + 10*1000;
		SendScoreboardMessageToAllClients();
		CalculateRanks();
	}

	if(level.pointStatusA == TEAM_BLUE && level.pointStatusB == TEAM_BLUE && level.timeTaken + 10*1000 <= level.time) {
		//Blue scores
		trap_SendServerCommand( -1, "print \"Blue team scores!\n\"");
		AddTeamScore(level.intermission_origin,TEAM_BLUE,1);
                G_LogPrintf( "DD: %i %i %i: %s scores!\n", -1, TEAM_BLUE, 2, TeamName(TEAM_BLUE) );
		Team_ForceGesture(TEAM_BLUE);
		Team_DD_bonusAtPoints(TEAM_BLUE);
		Team_RemoveDoubleDominationPoints();
		//We start again in 10 seconds:
		level.roundStartTime = level.time + 10*1000;
		SendScoreboardMessageToAllClients();
		CalculateRanks();
	}

	if((level.pointStatusA == TEAM_NONE || level.pointStatusB == TEAM_NONE) && level.time>level.roundStartTime) {
		trap_SendServerCommand( -1, "print \"A new round has started\n\"");
		Team_SpawnDoubleDominationPoints();
		SendScoreboardMessageToAllClients();
	}
}

/*
CheckLMS
*/

void CheckLMS(void) {
	int mode;
	mode = g_lms_mode.integer;
	if ( level.numPlayingClients < 1 ) {
		return;
	}

	

	//We don't want to do anything in intermission
	if(level.intermissiontime) {
		if(level.roundRespawned) {
			RestartEliminationRound();
		}
		level.roundStartTime = level.time; //so that a player might join at any time to fix the bots+no humans+autojoin bug
		return;
	}

	if(g_gametype.integer == GT_LMS)
	{
		int		countsLiving[TEAM_NUM_TEAMS];
		//trap_SendServerCommand( -1, "print \"This is LMS!\n\"");
		countsLiving[TEAM_FREE] = TeamLivingCount( -1, TEAM_FREE );
		if(countsLiving[TEAM_FREE]==1 && level.roundNumber==level.roundNumberStarted)
		{
			if(mode <=1 )
			LMSpoint();
			trap_SendServerCommand( -1, "print \"We have a winner!\n\"");
			EndEliminationRound();
			Team_ForceGesture(TEAM_FREE);
		}

		if(countsLiving[TEAM_FREE]==0 && level.roundNumber==level.roundNumberStarted)
		{
			trap_SendServerCommand( -1, "print \"All death... how sad\n\"");
			EndEliminationRound();
		}

		if((g_elimination_roundtime.integer) && (level.roundNumber==level.roundNumberStarted)&&(g_lms_mode.integer == 1 || g_lms_mode.integer==3)&&(level.time>=level.roundStartTime+1000*g_elimination_roundtime.integer))
		{
			trap_SendServerCommand( -1, "print \"Time up - Overtime disabled\n\"");
			if(mode <=1 )
			LMSpoint();
			EndEliminationRound();
		}

		//This might be better placed another place:
		if(g_elimination_activewarmup.integer<2)
			g_elimination_activewarmup.integer=2; //We need at least 2 seconds to spawn all players
		if(g_elimination_activewarmup.integer >= g_elimination_warmup.integer) //This must not be true
			g_elimination_warmup.integer = g_elimination_activewarmup.integer+1; //Increase warmup

		//Force respawn
		if(level.roundNumber != level.roundNumberStarted && level.time>level.roundStartTime-1000*g_elimination_activewarmup.integer && !level.roundRespawned)
		{
			level.roundRespawned = qtrue;
			RespawnAll();
			DisableWeapons();
			SendEliminationMessageToAllClients();
		}

		if(level.time<=level.roundStartTime && level.time>level.roundStartTime-1000*g_elimination_activewarmup.integer)
		{
			RespawnDead(qfalse);
			//DisableWeapons();
		}

		if(level.roundNumber == level.roundNumberStarted)
		{
			EnableWeapons();
		}

		if((level.roundNumber>level.roundNumberStarted)&&(level.time>=level.roundStartTime)) {
			RespawnDead(qtrue);
			StartLMSRound();
		}
	
		if(level.time+1000*g_elimination_warmup.integer-500>level.roundStartTime && level.numPlayingClients < 2)
		{
			RespawnDead(qfalse); //Allow player to run around anyway
			WarmupEliminationRound(); //Start over
			return;
		}

		if(level.warmupTime != 0) {
			if(level.time+1000*g_elimination_warmup.integer-500>level.roundStartTime)
			{
				RespawnDead(qfalse);
				WarmupEliminationRound();
			}
		}

	}
}

/*
=============
CheckElimination
=============
*/
void CheckElimination(void) {
	if ( level.numPlayingClients < 1 ) {
		if( (g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION) &&
			( level.time+1000*g_elimination_warmup.integer-500>level.roundStartTime ))
			RestartEliminationRound(); //For spectators
		return;
	}	

	//We don't want to do anything in itnermission
	if(level.intermissiontime) {
		if(level.roundRespawned)
			RestartEliminationRound();
		level.roundStartTime = level.time+1000*g_elimination_warmup.integer;
		return;
	}	

	if(g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION)
	{
		int		counts[TEAM_NUM_TEAMS];
		int		countsLiving[TEAM_NUM_TEAMS];
		int		countsHealth[TEAM_NUM_TEAMS];
		counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE, qtrue );
		counts[TEAM_RED] = TeamCount( -1, TEAM_RED, qtrue );

		countsLiving[TEAM_BLUE] = TeamLivingCount( -1, TEAM_BLUE );
		countsLiving[TEAM_RED] = TeamLivingCount( -1, TEAM_RED );

		countsHealth[TEAM_BLUE] = TeamHealthCount( -1, TEAM_BLUE );
		countsHealth[TEAM_RED] = TeamHealthCount( -1, TEAM_RED );

		if(level.roundBluePlayers != 0 && level.roundRedPlayers != 0  //Cannot score if one of the team never got any living players
				&& level.roundNumber == level.roundNumberStarted) {
			if (countsLiving[TEAM_BLUE] == 0 && countsLiving[TEAM_RED] == 0) {
				//Draw
				if(g_gametype.integer == GT_ELIMINATION) {
					G_LogPrintf( "ELIMINATION: %i %i %i: Round %i ended in a draw!\n", level.roundNumber, -1, 4, level.roundNumber );
				} else {
					G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: Round %i ended in a draw!\n", level.roundNumber, -1, -1, 9, level.roundNumber );
				}
				trap_SendServerCommand( -1, "print \"Round ended in a draw!\n\"");
				EndEliminationRound();
			} else if (countsLiving[TEAM_BLUE] == 0) {
				//Blue team has been eliminated!
				trap_SendServerCommand( -1, "print \"Blue Team eliminated!\n\"");
				AddTeamScore(level.intermission_origin,TEAM_RED,1);
				if(g_gametype.integer == GT_ELIMINATION) {
					G_LogPrintf( "ELIMINATION: %i %i %i: %s wins round %i by eleminating the enemy team!\n", level.roundNumber, TEAM_RED, 1, TeamName(TEAM_RED), level.roundNumber );
				} else {
					G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i by eleminating the enemy team!\n", level.roundNumber, -1, TEAM_RED, 6, TeamName(TEAM_RED), level.roundNumber );
				}
				EndEliminationRound();
				Team_ForceGesture(TEAM_RED);
			} else if (countsLiving[TEAM_RED] == 0) {
				//Red team eliminated!
				trap_SendServerCommand( -1, "print \"Red Team eliminated!\n\"");
				AddTeamScore(level.intermission_origin,TEAM_BLUE,1);
				if(g_gametype.integer == GT_ELIMINATION) {
					G_LogPrintf( "ELIMINATION: %i %i %i: %s wins round %i by eleminating the enemy team!\n", level.roundNumber, TEAM_BLUE, 1, TeamName(TEAM_BLUE), level.roundNumber );
				} else {
					G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i by eleminating the enemy team!\n", level.roundNumber, -1, TEAM_BLUE, 6, TeamName(TEAM_BLUE), level.roundNumber );
				}
				EndEliminationRound();
				Team_ForceGesture(TEAM_BLUE);
			}
		}

		//Time up
		if((level.roundNumber==level.roundNumberStarted)&&(g_elimination_roundtime.integer)&&(level.time>=level.roundStartTime+1000*g_elimination_roundtime.integer))
		{
			trap_SendServerCommand( -1, "print \"No teams eliminated.\n\"");

			if(level.roundBluePlayers != 0 && level.roundRedPlayers != 0) {//We don't want to divide by zero. (should not be possible)
				if(g_gametype.integer == GT_CTF_ELIMINATION && g_elimination_ctf_oneway.integer) {
					//One way CTF, make defensice team the winner.
					if ( (level.eliminationSides+level.roundNumber)%2 == 0 ) { //Red was attacking
						trap_SendServerCommand( -1, "print \"Blue team defended the base\n\"");
						AddTeamScore(level.intermission_origin,TEAM_BLUE,1);
                                                G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i by defending the flag!\n", level.roundNumber, -1, TEAM_BLUE, 5, TeamName(TEAM_BLUE), level.roundNumber );
					}
					else {
						trap_SendServerCommand( -1, "print \"Red team defended the base\n\"");
						AddTeamScore(level.intermission_origin,TEAM_RED,1);
                                                G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i by defending the flag!\n", level.roundNumber, -1, TEAM_RED, 5, TeamName(TEAM_RED), level.roundNumber );
					}
				}
				else if(((double)countsLiving[TEAM_RED])/((double)level.roundRedPlayers)>((double)countsLiving[TEAM_BLUE])/((double)level.roundBluePlayers))
				{
					//Red team has higher procentage survivors
					trap_SendServerCommand( -1, "print \"Red team has most survivers!\n\"");
					AddTeamScore(level.intermission_origin,TEAM_RED,1);
                                        if(g_gametype.integer == GT_ELIMINATION) {
                                            G_LogPrintf( "ELIMINATION: %i %i %i: %s wins round %i due to more survivors!\n", level.roundNumber, TEAM_RED, 2, TeamName(TEAM_RED), level.roundNumber );
                                        } else {
                                            G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i due to more survivors!\n", level.roundNumber, -1, TEAM_RED, 7, TeamName(TEAM_RED), level.roundNumber );
                                        }
				}
				else if(((double)countsLiving[TEAM_RED])/((double)level.roundRedPlayers)<((double)countsLiving[TEAM_BLUE])/((double)level.roundBluePlayers))
				{
					//Blue team has higher procentage survivors
					trap_SendServerCommand( -1, "print \"Blue team has most survivers!\n\"");
					AddTeamScore(level.intermission_origin,TEAM_BLUE,1);	
                                        if(g_gametype.integer == GT_ELIMINATION) {
                                            G_LogPrintf( "ELIMINATION: %i %i %i: %s wins round %i due to more survivors!\n", level.roundNumber, TEAM_BLUE, 2, TeamName(TEAM_BLUE), level.roundNumber );
                                        } else {
                                            G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i due to more survivors!\n", level.roundNumber, -1, TEAM_BLUE, 7, TeamName(TEAM_BLUE), level.roundNumber );
                                        }
				}
				else if(countsHealth[TEAM_RED]>countsHealth[TEAM_BLUE])
				{
					//Red team has more health
					trap_SendServerCommand( -1, "print \"Red team has more health left!\n\"");
					AddTeamScore(level.intermission_origin,TEAM_RED,1);
                                        if(g_gametype.integer == GT_ELIMINATION) {
                                            G_LogPrintf( "ELIMINATION: %i %i %i: %s wins round %i due to more health left!\n", level.roundNumber, TEAM_RED, 3, TeamName(TEAM_RED), level.roundNumber );
                                        } else {
                                            G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i due to more health left!\n", level.roundNumber, -1, TEAM_RED, 8, TeamName(TEAM_RED), level.roundNumber );
                                        }
				}
				else if(countsHealth[TEAM_RED]<countsHealth[TEAM_BLUE])
				{
					//Blue team has more health
					trap_SendServerCommand( -1, "print \"Blue team has more health left!\n\"");
					AddTeamScore(level.intermission_origin,TEAM_BLUE,1);
                                        if(g_gametype.integer == GT_ELIMINATION) {
                                            G_LogPrintf( "ELIMINATION: %i %i %i: %s wins round %i due to more health left!\n", level.roundNumber, TEAM_BLUE, 3, TeamName(TEAM_BLUE), level.roundNumber );
                                        } else {
                                            G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i due to more health left!\n", level.roundNumber, -1, TEAM_BLUE, 8, TeamName(TEAM_BLUE), level.roundNumber );
                                        }
				}
			}
                        //Draw
                        if(g_gametype.integer == GT_ELIMINATION) {
                            G_LogPrintf( "ELIMINATION: %i %i %i: Round %i ended in a draw!\n", level.roundNumber, -1, 4, level.roundNumber );
                        } else {
                            G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: Round %i ended in a draw!\n", level.roundNumber, -1, -1, 9, level.roundNumber );
                        }
			trap_SendServerCommand( -1, "print \"Round ended in a draw!\n\"");
			EndEliminationRound();
		}

		if (g_elimination_respawn.integer && 
				(level.roundNumber==level.roundNumberStarted) 
				&& (((g_elimination_roundtime.integer) && (level.time < level.roundStartTime+1000*g_elimination_roundtime.integer)) || !g_elimination_roundtime.integer)) {

			if (RespawnElimZombies()) {
				G_SendTeamPlayerCounts();
			}
		}

		//This might be better placed another place:
		if(g_elimination_activewarmup.integer<1)
			g_elimination_activewarmup.integer=1; //We need at least 1 second to spawn all players
		if(g_elimination_activewarmup.integer >= g_elimination_warmup.integer) //This must not be true
			g_elimination_warmup.integer = g_elimination_activewarmup.integer+1; //Increase warmup

		//Force respawn
		if(level.roundNumber!=level.roundNumberStarted && level.time>level.roundStartTime-1000*g_elimination_activewarmup.integer && !level.roundRespawned)
		{
			level.roundRespawned = qtrue;
			RespawnAllElim();
			SendEliminationMessageToAllClients();
		}

		if(level.time<=level.roundStartTime && level.time>level.roundStartTime-1000*g_elimination_activewarmup.integer)
		{
			RespawnDead(qfalse);
		}
			

		if((level.roundNumber>level.roundNumberStarted)&&(level.time>=level.roundStartTime)) {
			RespawnDead(qtrue);
			StartEliminationRound();
		}
	
		if(level.time+1000*g_elimination_warmup.integer-500>level.roundStartTime)
		if(counts[TEAM_BLUE]<1 || counts[TEAM_RED]<1)
		{
			RespawnDead(qfalse); //Allow players to run around anyway
			WarmupEliminationRound(); //Start over
			return;
		}

		if(level.warmupTime != 0) {
			if(level.time+1000*g_elimination_warmup.integer-500>level.roundStartTime)
			{
				RespawnDead(qfalse);
				WarmupEliminationRound();
			}
		}
	}
}

/*
=============
CheckDomination
=============
*/
void CheckDomination(void) {
	int i;
        int scoreFactor = 1;

	if ( (level.numPlayingClients < 1) || (g_gametype.integer != GT_DOMINATION) ) {
		return;
	}

	//Do nothing if warmup
	if(level.warmupTime != 0)
		return; 

	//Don't score if we are in intermission. Just plain stupid
	if(level.intermissiontime)
		return;

	//Sago: I use if instead of while, since if the server stops for more than 2 seconds people should not be allowed to score anyway
	if(level.domination_points_count>3)
            scoreFactor = 2; //score more slowly if there are many points
        if(level.time>=level.dom_scoreGiven*DOM_SECSPERPOINT*scoreFactor) {
		for(i=0;i<level.domination_points_count;i++) {
			if ( level.pointStatusDom[i] == TEAM_RED )
				AddTeamScore(level.intermission_origin,TEAM_RED,1);
			if ( level.pointStatusDom[i] == TEAM_BLUE )
				AddTeamScore(level.intermission_origin,TEAM_BLUE,1);
                        G_LogPrintf( "DOM: %i %i %i %i: %s holds point %s for 1 point!\n",
                                    -1,i,1,level.pointStatusDom[i],
                                    TeamName(level.pointStatusDom[i]),level.domination_points_names[i]);
		}
		level.dom_scoreGiven++;
		while(level.time>level.dom_scoreGiven*DOM_SECSPERPOINT*scoreFactor)
			level.dom_scoreGiven++;
		CalculateRanks();
	}
}

qboolean ScheduleTreasureHunterRound( void ) {
	if (level.th_round >= (g_treasureRounds.integer ? g_treasureRounds.integer : 1) 
			&& !ScoreIsTied()) {
		level.th_hideTime = 0;
		level.th_seekTime = 0;
		level.th_phase = TH_INIT;
		level.th_teamTokensRed = 0;
		level.th_teamTokensBlue = 0;
		return qfalse;
	}

	level.th_round++;

	level.th_hideTime = level.time + 1000*5;
	//level.th_seekTime = level.th_hideTime + 1000*5 + g_treasureHideTime.integer * 1000;
	level.th_seekTime = 0;
	level.th_phase = TH_INIT;
	level.th_teamTokensRed = 0;
	level.th_teamTokensBlue = 0;

	return qtrue;
}

void UpdateToken(gentity_t *token, qboolean vulnerable, int setHealth) {
	if (setHealth) {
		token->health = setHealth;
	}
	if (vulnerable) {
		// used for pickup prediction
		token->s.generic1 = 1;
	} else {
		// used for pickup prediction
		token->s.generic1 = 0;
	}
}

void UpdateTreasureEntityVisiblity(qboolean hiddenFromEnemy) {
	gentity_t	*ent;
	int i;

	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		int team = TEAM_SPECTATOR;
		if (!ent->inuse) {
			continue;
		}

		switch (ent->s.eType) {
			case ET_PLAYER:
				if (!ent->client) {
					continue;
				}
				team = ent->client->sess.sessionTeam;
				break;
			case ET_MISSILE:
				if (!ent->parent || !ent->parent->client) {
					continue;
				}
				team = ent->parent->client->sess.sessionTeam;
				break;
			default:
				continue;
				break;
		}
		if (!hiddenFromEnemy || team == TEAM_SPECTATOR) {
			ent->r.svFlags &= ~SVF_CLIENTMASK;
			ent->r.singleClient = 0;
		} else if (team == TEAM_BLUE) {
			ent->r.svFlags |= SVF_CLIENTMASK;
			ent->r.singleClient = level.th_blueClientMask | level.th_specClientMask;
		} else if (team == TEAM_RED) {
			ent->r.svFlags |= SVF_CLIENTMASK;
			ent->r.singleClient = level.th_redClientMask | level.th_specClientMask;
		}
	}
}

void UpdateTreasureVisibility( qboolean hiddenFromEnemy, int setHealth) {
	gentity_t	*token;

	token = NULL;
	while ((token = G_Find (token, FOFS(classname), "item_redcube")) != NULL) {
		token->r.singleClient = hiddenFromEnemy ? level.th_redClientMask : level.th_redClientMask | level.th_blueClientMask;
		if (hiddenFromEnemy) {
			token->r.svFlags |= SVF_BROADCAST;
		} else {
			token->r.svFlags &= ~SVF_BROADCAST;
		}
		UpdateToken(token, !hiddenFromEnemy, setHealth);

	}
	token = NULL;
	while ((token = G_Find (token, FOFS(classname), "item_bluecube")) != NULL) {
		token->r.singleClient = hiddenFromEnemy ? level.th_blueClientMask : level.th_blueClientMask | level.th_redClientMask;
		if (hiddenFromEnemy) {
			token->r.svFlags |= SVF_BROADCAST;
		} else {
			token->r.svFlags &= ~SVF_BROADCAST;
		}
		UpdateToken(token, !hiddenFromEnemy, setHealth);
	}
}

int CountTreasures(int team) {
	gentity_t	*token;
	int count = 0;

	token = NULL;
	while ((token = G_Find (token, FOFS(classname), team == TEAM_RED ? "item_redcube" : "item_bluecube")) != NULL) {
		count++;
	}
	return count;
}

int CountPlayerTokens(int team) {
	int i;
	gentity_t *ent;
	int count = 0;

	for( i=0;i < level.numPlayingClients; i++ ) {
		ent = &g_entities[level.sortedClients[i]];

		if (!ent->inuse) {
			continue;
		}
		if (ent->client->pers.th_tokens <= 0) {
			continue;
		}

		if (ent->client->sess.sessionTeam == team) {
			count += ent->client->pers.th_tokens;
		}
	}

	return count;
}


/*
=============
CheckTreasureHunter
=============
*/
void CheckTreasureHunter(void) {
	int i;
	int tokens_red;
	int tokens_blue;
	qboolean needsUpdate = qfalse;

	if (g_gametype.integer != GT_TREASURE_HUNTER) {
		return;
	}

	if (level.intermissiontime
			|| level.warmupTime != 0) {
		return;
	}

	// update client masks for the tokens
	level.th_blueClientMask = 0;
	level.th_redClientMask = 0;
	level.th_specClientMask = 0;
	for( i=0;i < level.numPlayingClients; i++ ) {
		int cNum = level.sortedClients[i];
		gentity_t *ent = &g_entities[cNum];
		if (!ent->inuse) {
			continue;
		}

		if (cNum >= 32) {
			continue;
		}

		if (ent->client->sess.sessionTeam == TEAM_BLUE) {
			level.th_blueClientMask |= (1 << cNum);
		} else if (ent->client->sess.sessionTeam == TEAM_RED) {
			level.th_redClientMask |= (1 << cNum);
		} else {
			level.th_specClientMask |= (1 << cNum);
		}
	}

	// set the token visiblity for each phase
	if (level.th_phase == TH_HIDE) {
		UpdateTreasureVisibility(qtrue, 0);
	} else if (level.th_phase == TH_SEEK) {
		UpdateTreasureVisibility(qfalse, 0);
	} else if (level.time >= level.th_hideTime) {
		UpdateTreasureVisibility(qtrue, 0);
	}

	// hide players / missiles during hiding phase
	if (level.th_phase == TH_HIDE) {
		UpdateTreasureEntityVisiblity(qtrue);
	} else {
		UpdateTreasureEntityVisiblity(qfalse);
	}


	CalculateRanks();

	if (TeamCount(-1, TEAM_BLUE, qfalse) == 0 
			|| TeamCount(-1, TEAM_RED, qfalse) == 0) {
		return;
	}
	if (!level.th_round) {
		level.th_roundFinished = 0;
	}

	if (!level.th_hideTime && !level.th_seekTime) {
		 if (!ScheduleTreasureHunterRound()) {
			 return;
		 }
		 SendTreasureHuntMessageToAllClients();
	}

	tokens_red = CountTreasures(TEAM_RED);
	tokens_blue = CountTreasures(TEAM_BLUE);
	if (level.th_placedTokensRed != tokens_red ||
			level.th_placedTokensBlue != tokens_blue) {
		level.th_placedTokensRed = tokens_red;
		level.th_placedTokensBlue = tokens_blue;
		needsUpdate = qtrue;
	}

	// TODO: remaining token indicator

	if (level.th_phase == TH_INIT
			&& level.time >= level.th_hideTime) {
		char str[256] = "";
		if (g_treasureHideTime.integer > 0) {
			int min = (level.th_hideTime + g_treasureHideTime.integer * 1000 - level.startTime)/(60*1000);
			int s = (level.th_hideTime + g_treasureHideTime.integer * 1000 - level.startTime)/1000 - min * 60;
			Com_sprintf(str, sizeof(str), "\nHiding phase lasts until %i:%02i", min, s);
		}
		level.th_phase = TH_HIDE;
		trap_SendServerCommand( -1, va("cp \"Hide your tokens!\nUse \\placeToken."
					"%s\n\"", str));
		trap_SendServerCommand( -1, va("print \"" S_COLOR_CYAN "Hide your tokens using \\placeToken."
					"%s\n\"", str));
		// enables placeToken
		// give players their tokens
		SetPlayerTokens((g_treasureTokens.integer <= 0) ? 1 : g_treasureTokens.integer, qfalse);

		needsUpdate = qtrue;

	} else if (level.th_phase == TH_HIDE 
			&& g_treasureHideTime.integer > 0
			&& level.time == level.th_hideTime + g_treasureHideTime.integer * 1000 - 10000)  {
		trap_SendServerCommand( -1, va("cp \"10s left to hide!\n"));
		// TODO: what if tokens get destroyed (e.g. by lava)
		// TODO: make sure it advances if everything is hidden
	} else if (level.th_phase == TH_HIDE) {
		int leftover_tokens_red = CountPlayerTokens(TEAM_RED);
		int leftover_tokens_blue = CountPlayerTokens(TEAM_BLUE);
		int teamTokenDiff = 0;
		char *s = NULL;

		teamTokenDiff = (level.th_placedTokensRed + level.th_teamTokensRed +leftover_tokens_red) -
		       (level.th_placedTokensBlue + level.th_teamTokensBlue + leftover_tokens_blue);	

		if (teamTokenDiff < 0) {
			level.th_teamTokensRed -= teamTokenDiff;
			// update generic1 only:
			SetPlayerTokens(0, qtrue);
		} else if (teamTokenDiff > 0) {
			level.th_teamTokensBlue += teamTokenDiff;
			// update generic1 only:
			SetPlayerTokens(0, qtrue);
		}
		
		if (level.th_teamTokensRed > 0 && level.th_teamTokensBlue > 0) {
			// if both teams have team tokens, take any excess
			// tokens away to prevent continously inflating the
			// team token numbers as players join/leave
			if (level.th_teamTokensRed > level.th_teamTokensBlue) { 
				level.th_teamTokensRed -= level.th_teamTokensBlue;
				level.th_teamTokensBlue = 0;
			} else {
				level.th_teamTokensBlue -= level.th_teamTokensRed;
				level.th_teamTokensRed = 0;
			}
			// update generic1 only:
			SetPlayerTokens(0, qtrue);
		}

		leftover_tokens_red += level.th_teamTokensRed;
		leftover_tokens_blue += level.th_teamTokensBlue;

		if (g_treasureHideTime.integer > 0 && level.time >= level.th_hideTime + g_treasureHideTime.integer * 1000)  {
			s = "Time is up!";
		} else if (leftover_tokens_red + leftover_tokens_blue == 0)  {
			// wait 5 seconds to allow the player to remove the token again if it's misplaced
			if (level.th_allTokensHiddenTime == 0) {
				level.th_allTokensHiddenTime = level.time;
			} else if (level.time >=  level.th_allTokensHiddenTime + 5000) {
				s = "All tokens hidden!";
			} 
		} else {
			level.th_allTokensHiddenTime = 0;
		}

		if (s) {
			// schedule seeking in 5s
			level.th_seekTime = level.time + 1000*5;
			level.th_phase = TH_INTER;

			trap_SendServerCommand( -1, va("cp \"%s\nHiding phase finished!\nPrepare to seek!\"", s));
			// disables placeToken
			// give leftovers to other team

			if (leftover_tokens_red) {
				trap_SendServerCommand( -1, va("print \"Blue gets %i leftover tokens from red!\n\"", leftover_tokens_red));
				AddTeamScore(level.intermission_origin, TEAM_BLUE, leftover_tokens_red);
			}
			if (leftover_tokens_blue) {
				trap_SendServerCommand( -1, va("print \"Red gets %i leftover tokens from blue!\n\"", leftover_tokens_blue));
				AddTeamScore(level.intermission_origin, TEAM_RED, leftover_tokens_blue);
			}
			needsUpdate = qtrue;
		}
	} else if (level.th_phase == TH_INTER 
			&& level.time >= level.th_seekTime) {
		char str[256] = "";
		if (g_treasureSeekTime.integer > 0) {
			int min = (level.th_seekTime + g_treasureSeekTime.integer * 1000 - level.startTime)/(60*1000);
			int s = (level.th_seekTime + g_treasureSeekTime.integer * 1000 - level.startTime)/1000 - min * 60;
			Com_sprintf(str, sizeof(str), "\nSeeking phase lasts until %i:%02i", min, s);
		}
		level.th_phase = TH_SEEK;
		UpdateTreasureVisibility(qfalse, g_treasureTokenHealth.integer);
		trap_SendServerCommand( -1, va("cp \"Find your opponent's tokens!"
					"%s\n\"", str));
		trap_SendServerCommand( -1, va("print \"" S_COLOR_CYAN "Find your opponent's tokens!"
					"%s\n\"", str));
		// make enemy tokens visible
		// enables pickup of enemy tokens
		SendTreasureHuntMessageToAllClients();
		needsUpdate = qfalse;
	} else if (level.th_phase == TH_SEEK) {
		gentity_t	*token;

		if (tokens_red == 0 
				|| tokens_blue == 0
				|| (g_treasureSeekTime.integer > 0 
					&& level.time >= level.th_seekTime + g_treasureSeekTime.integer * 1000
					&& !ScoreIsTied() // for overtime
					)) { 
			char *s = NULL;

			if (tokens_red + tokens_blue == 0) {
				s = "No tokens left!";
			} else if (tokens_red == 0) {
				s = "Blue found all tokens!";
			} else if (tokens_blue == 0) {
				s = "Red found all tokens!";
			} else {
				s = "Time is up!";
			}
			level.th_phase = TH_INIT;
			trap_SendServerCommand( -1, va("cp \"%s\nSeeking phase finished!\n\"", s));

			// finish, clear tokens from map!
			token = NULL;
			while ((token = G_Find (token, FOFS(classname), "item_redcube")) != NULL) {
				G_FreeEntity(token);
			}
			token = NULL;
			while ((token = G_Find (token, FOFS(classname), "item_bluecube")) != NULL) {
				G_FreeEntity(token);
			}

			level.th_roundFinished = level.th_round;

			// schedule next round
			ScheduleTreasureHunterRound();
			needsUpdate = qtrue;
		}

	}

	if (needsUpdate) {
		SendTreasureHuntMessageToAllClients();
	}
}

/*
=============
CheckTournament

Once a frame, check for changes in tournement player state
=============
*/
void CheckTournament( void ) {
	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	if ( level.numPlayingClients == 0 ) {
		return;
	}

	if ( g_gametype.integer == GT_TOURNAMENT ) {

		if (!level.warmupTime && level.numPlayingClients < 2) {
			level.tournamentForfeited = qtrue;
			return;
		}

		// pull in a spectator if needed
		if ( level.numPlayingClients < 2 ) {
			AddTournamentPlayer();
			if (level.eqPing && g_eqpingAutoTourney.integer) {
				G_EQPingReset();
			}
			if (level.pingEqualized) {
				G_PingEqualizerReset();
			}
		}

		// if we don't have two players, go back to "waiting for players"
		if ( level.numPlayingClients != 2 ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return;
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			if ( level.numPlayingClients == 2 ) {
				if ( ( g_startWhenReady.integer && 
				       ( g_entities[level.sortedClients[0]].client->ready || ( g_entities[level.sortedClients[0]].r.svFlags & SVF_BOT ) ) && 
				       ( g_entities[level.sortedClients[1]].client->ready || ( g_entities[level.sortedClients[1]].r.svFlags & SVF_BOT ) ) 
				      ) || !g_startWhenReady.integer || !g_doWarmup.integer || G_AutoStartReady()) {
					// fudge by -1 to account for extra delays
					if ( g_warmup.integer > 1 ) {
						level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
					} else {
						level.warmupTime = 0;
					}

					trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				}
			}
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	} else if ( g_gametype.integer != GT_SINGLE_PLAYER && level.warmupTime != 0 ) {
		int		counts[TEAM_NUM_TEAMS];
		qboolean	notEnough = qfalse;
		int i;
		int clientsReady = 0;
		int clientsReadyRed = 0;
		int clientsReadyBlue = 0;

		memset(counts, 0, sizeof(counts));
		if ( g_gametype.integer > GT_TEAM && !g_ffa_gt ) {
			counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE, qtrue);
			counts[TEAM_RED] = TeamCount( -1, TEAM_RED, qtrue);

			if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1) {
				notEnough = qtrue;
			}
		} else if ( level.numPlayingClients < 2 ) {
			notEnough = qtrue;
		}

		if( g_startWhenReady.integer ){
			for( i = 0; i < level.numPlayingClients; i++ ){
				if( ( g_entities[level.sortedClients[i]].client->ready || g_entities[level.sortedClients[i]].r.svFlags & SVF_BOT ) && g_entities[level.sortedClients[i]].inuse ) {
					clientsReady++;
					switch (g_entities[level.sortedClients[i]].client->sess.sessionTeam) {
						case TEAM_RED:
							clientsReadyRed++;
							break;
						case TEAM_BLUE:
							clientsReadyBlue++;
							break;
						default:
							break;
					}
				}
			}
		}

		if ( g_doWarmup.integer && g_startWhenReady.integer == 1 
				&& ( clientsReady < level.numPlayingClients/2 + 1 )
				&& !G_AutoStartReady()) {
			notEnough = qtrue;
		} else if ( g_doWarmup.integer && g_startWhenReady.integer == 2 
				&& ( clientsReady < level.numPlayingClients )
				&& !G_AutoStartReady()) {
			notEnough = qtrue;
		} else if ( g_doWarmup.integer && g_startWhenReady.integer == 3 
				&& !G_AutoStartReady()) {
			if (g_gametype.integer >= GT_TEAM && !g_ffa_gt) {
				if ( clientsReadyRed < counts[TEAM_RED]/2 + 1  || 
						clientsReadyBlue < counts[TEAM_BLUE]/2 + 1) {
					notEnough = qtrue;
				}
			} else if (( clientsReady < level.numPlayingClients/2 + 1 )) {
				notEnough = qtrue;

			}
		}

		if ( notEnough ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return; // still waiting for team members
		} 

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			// fudge by -1 to account for extra delays
			level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
			trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );

			if (g_autoTeamsLock.integer 
					&& g_autoTeamsUnlock.integer
					&& g_startWhenReady.integer
					&& g_warmup.integer
					&& (g_gametype.integer >= GT_TEAM && g_ffa_gt != 1)
					&& G_CountHumanPlayers(TEAM_RED) > 0 
					&& G_CountHumanPlayers(TEAM_BLUE) > 0) {
				
				trap_SendServerCommand( -1, va("print \"^5Server: Automatically locking teams!\n"));
				G_LockTeams();
				level.autoLocked = qtrue;
			}

			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	}
}




/*
==================
PrintTeam
==================
*/
void PrintTeam(int team, char *message) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		trap_SendServerCommand( i, message );
	}
}

/*
==================
SetLeader
==================
*/
void SetLeader(int team, int client) {
	int i;

	if ( level.clients[client].pers.connected == CON_DISCONNECTED ) {
		PrintTeam(team, va("print \"%s is not connected\n\"", level.clients[client].pers.netname) );
		return;
	}
	if (level.clients[client].sess.sessionTeam != team) {
		PrintTeam(team, va("print \"%s is not on the team anymore\n\"", level.clients[client].pers.netname) );
		return;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader) {
			level.clients[i].sess.teamLeader = qfalse;
			ClientUserinfoChanged(i);
		}
	}
	level.clients[client].sess.teamLeader = qtrue;
	ClientUserinfoChanged( client );
	PrintTeam(team, va("print \"%s is the new team leader\n\"", level.clients[client].pers.netname) );
}

/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader( int team ) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader)
			break;
	}
	if (i >= level.maxclients) {
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			if (!(g_entities[i].r.svFlags & SVF_BOT)) {
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			level.clients[i].sess.teamLeader = qtrue;
			break;
		}
	}
}

/*
==================
CheckTeamVote
==================
*/
void CheckTeamVote( int team ) {
	int cs_offset;

	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		return;
	}
	if ( level.time - level.teamVoteTime[cs_offset] >= VOTE_TIME ) {
		trap_SendServerCommand( -1, "print \"Team vote failed.\n\"" );
	} else {
		if ( level.teamVoteYes[cs_offset] > level.numteamVotingClients[cs_offset]/2 ) {
			// execute the command, then remove the vote
			trap_SendServerCommand( -1, "print \"Team vote passed.\n\"" );
			//
			if ( !Q_strncmp( "leader", level.teamVoteString[cs_offset], 6) ) {
				//set the team leader
				SetLeader(team, atoi(level.teamVoteString[cs_offset] + 7));
			}
			else {
				trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.teamVoteString[cs_offset] ) );
			}
		} else if ( level.teamVoteNo[cs_offset] >= level.numteamVotingClients[cs_offset]/2 ) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, "print \"Team vote failed.\n\"" );
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.teamVoteTime[cs_offset] = 0;
	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, "" );

}


/*
==================
CheckCvars
==================
*/
void CheckCvars( void ) {
	static int lastMod = -1;

	if ( g_password.modificationCount != lastMod ) {
		lastMod = g_password.modificationCount;
		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
			trap_Cvar_Set( "g_needpass", "1" );
		} else {
			trap_Cvar_Set( "g_needpass", "0" );
		}
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink (gentity_t *ent) {
	float	thinktime;

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		return;
	}
	if (thinktime > level.time) {
		return;
	}
	
	ent->nextthink = 0;
	if (!ent->think) {
		G_Error ( "NULL ent->think");
	}
	ent->think (ent);
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame( int levelTime ) {
	int			i;
	gentity_t	*ent;
	int			msec;

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {

		// this is used to delay a map_restart after shuffling, to prevent a command overflow
		if (level.restartAt && level.restartAt <= levelTime) {
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
		}

		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.realtime = levelTime;
	

	if (level.timeout && levelTime >= level.timeoutEnd) {
		G_Timein();
	}

	if (!level.timeout)
		level.time = levelTime;
	level.realtime = levelTime;

	msec = level.time - level.previousTime;

	// get any cvar changes
	G_UpdateCvars();

	G_UpdateRatFlags();

	G_UpdateActionCamera();

	if (level.timeout) {
		G_TimeinWarning(levelTime);
		CheckVote();
		return;
	}

        if( (g_gametype.integer==GT_ELIMINATION || g_gametype.integer==GT_CTF_ELIMINATION) && !(g_elimflags.integer & EF_NO_FREESPEC) && g_elimination_lockspectator.integer>1)
            trap_Cvar_Set("elimflags",va("%i",g_elimflags.integer|EF_NO_FREESPEC));
        else
        if( (g_elimflags.integer & EF_NO_FREESPEC) && g_elimination_lockspectator.integer<2)
            trap_Cvar_Set("elimflags",va("%i",g_elimflags.integer&(~EF_NO_FREESPEC) ) );

        if( g_elimination_ctf_oneway.integer && !(g_elimflags.integer & EF_ONEWAY) ) {
            trap_Cvar_Set("elimflags",va("%i",g_elimflags.integer|EF_ONEWAY ) );
            //If the server admin has enabled it midgame imidiantly braodcast attacking team
            SendAttackingTeamMessageToAllClients();
        }
        else
        if( !g_elimination_ctf_oneway.integer && (g_elimflags.integer & EF_ONEWAY) ) {
            trap_Cvar_Set("elimflags",va("%i",g_elimflags.integer&(~EF_ONEWAY) ) );
        }

	//
	// go through all allocated objects
	//
	//start = trap_Milliseconds();
	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}

		// clear events that are too old
		if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if ( ent->client ) {
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
				}
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				G_FreeEntity( ent );
				continue;
			} else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity( ent );
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( !ent->r.linked && ent->neverFree ) {
			continue;
		}

//unlagged - backward reconciliation #2
		// we'll run missiles separately to save CPU in backward reconciliation
/*
		if ( ent->s.eType == ET_MISSILE ) {
			G_RunMissile( ent );
			continue;
		}
*/
//unlagged - backward reconciliation #2

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
			G_RunItem( ent );
			continue;
		}

		if ( ent->s.eType == ET_MOVER ) {
			G_RunMover( ent );
			continue;
		}

		if ( i < MAX_CLIENTS ) {
			G_RunClient( ent );
			continue;
		}

		G_RunThink( ent );
	}

	for (i=0 ; i < level.num_entities ; ++i ) {
		ent = &g_entities[i];
		G_MissileRunDelag(ent, msec);
	}

//unlagged - backward reconciliation #2
	// NOW run the missiles, with all players backward-reconciled
	// to the positions they were in exactly 50ms ago, at the end
	// of the last server frame
	G_TimeShiftAllClients( level.previousTime, NULL );

	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		if ( !ent->inuse ) {
			continue;
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( ent->s.eType == ET_MISSILE ) {
			G_RunMissile( ent );
		}
	}

	G_UnTimeShiftAllClients( NULL );
//unlagged - backward reconciliation #2

//end = trap_Milliseconds();

//start = trap_Milliseconds();
	// perform final fixups on the players
	ent = &g_entities[0];
	for (i=0 ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse ) {
			ClientEndFrame( ent );
		}
	}
//end = trap_Milliseconds();

	// see if it is time to do a tournement restart
	CheckTournament();

	//Check Elimination state
	CheckElimination();
	CheckLMS();

	//Check Double Domination
	CheckDoubleDomination();

	CheckDomination();

	//Sago: I just need to think why I placed this here... they should only spawn once
	if(g_gametype.integer == GT_DOMINATION)
		Team_Dom_SpawnPoints();

	CheckTreasureHunter();

	CheckTeamBalance();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// check team votes
	CheckTeamVote( TEAM_RED );
	CheckTeamVote( TEAM_BLUE );

	// for tracking changes
	CheckCvars();

	if (g_listEntity.integer) {
		for (i = 0; i < MAX_GENTITIES; i++) {
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}
		trap_Cvar_Set("g_listEntity", "0");
	}

	G_PingEqualizerWrite();
	G_EQPingAutoTourney();
	G_EQPingUpdate();
	G_EQPingAutoAdjust();

	G_CheckUnlockTeams();

//unlagged - backward reconciliation #4
	// record the time at the end of this frame - it should be about
	// the time the next frame begins - when the server starts
	// accepting commands from connected clients
	level.frameStartTime = trap_Milliseconds();
//unlagged - backward reconciliation #4
}


void G_SetRuleset(int ruleset) {
	if (ruleset == RULES_FAST) {
		trap_Cvar_Set("g_fastSwitch", "1");
		trap_Cvar_Set("g_fastWeapons", "1");
		trap_Cvar_Set("g_additiveJump", "1");
		trap_Cvar_Set("g_ratPhysics", "0");
		trap_Cvar_Set("g_rampJump", "0");
		trap_Cvar_Set("g_itemPickup", "1");
		trap_Cvar_Set("g_screenShake", "0");
		trap_Cvar_Set("g_teleMissiles", "1");
		trap_Cvar_Set("g_pushGrenades", "1");
	} else if (ruleset == RULES_SLOW) {
		trap_Cvar_Set("g_fastSwitch", "0");
		trap_Cvar_Set("g_fastWeapons", "1");
		trap_Cvar_Set("g_additiveJump", "0");
		trap_Cvar_Set("g_ratPhysics", "0");
		trap_Cvar_Set("g_rampJump", "0");
		trap_Cvar_Set("g_itemPickup", "1");
		trap_Cvar_Set("g_screenShake", "0");
		trap_Cvar_Set("g_teleMissiles", "1");
		trap_Cvar_Set("g_pushGrenades", "1");
	}
}

void G_LockTeams(void) {
	level.RedTeamLocked = qtrue;
	level.BlueTeamLocked = qtrue;
	level.FFALocked = qtrue;
	if (level.warmupTime != 0) {
		// during warmup, make sure teams stay locked when the game starts
		trap_Cvar_Set("g_teamslocked", "1");
	} else {
		// game already started, don't lock next game
		trap_Cvar_Set("g_teamslocked", "0");
	}
	trap_SendServerCommand( -1, va("print \"^5Server: teams locked!\n"));
}

void G_UnlockTeams(void) {
	level.RedTeamLocked = qfalse;
	level.BlueTeamLocked = qfalse;
	level.FFALocked = qfalse;
	trap_Cvar_Set("g_teamslocked", "0");
	trap_SendServerCommand( -1, va("print \"^5Server: teams unlocked!\n"));
	level.autoLocked = qfalse;
}

void G_CheckUnlockTeams(void) {
	qboolean unlock = qfalse;

	if (!g_autoTeamsUnlock.integer) {
		return;
	}

	if (level.warmupTime == 0 && level.time - level.startTime < 15000) {
		return;
	}

	if (!(level.RedTeamLocked || level.BlueTeamLocked || level.FFALocked)) {
		return;
	}

	if (level.autoLocked && level.warmupTime < 0) {
		// teams were automatically locked, but warmup re-started for some reason, so unlock
		G_UnlockTeams();
		return;
	} 

	if (g_gametype.integer >= GT_TEAM && g_ffa_gt != 1) {
		if (G_CountHumanPlayers(TEAM_RED) == 0 
				|| G_CountHumanPlayers(TEAM_BLUE) == 0) {
			unlock = qtrue;
		}
	} else {
		if (G_CountHumanPlayers(TEAM_FREE) == 0) {
			unlock = qtrue;
		}
	}
	if (unlock) {
		trap_SendServerCommand( -1, va("print \"^5Server: unlocking teams due to lack of human players!\n"));
		G_UnlockTeams();
	}
}
