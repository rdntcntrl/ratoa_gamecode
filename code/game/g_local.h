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

You should have received a copyPl of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// g_local.h -- local definitions for game module

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "g_public.h"
#include "challenges.h"

//==================================================================

// the "gameversion" client command will print this plus compile date
#define	GAMEVERSION	BASEGAME

#define BODY_QUEUE_SIZE		8

#define INFINITE			1000000

#define	FRAMETIME			100					// msec
#define	CARNAGE_REWARD_TIME	3000
#define REWARD_SPRITE_TIME	2000

#define	INTERMISSION_DELAY_TIME	1000
#define	SP_INTERMISSION_DELAY_TIME	5000

//Domination how many seconds between awarded a point (multiplied by two if more than 3 points)
#define DOM_SECSPERPOINT	2000

//limit of the votemaps.cfg file and other custom map files
#define	MAX_MAPS_TEXT		8192

// gentity->flags
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
#define	FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
#define FL_DROPPED_ITEM			0x00001000
#define FL_NO_BOTS				0x00002000	// spawn point not for bot use
#define FL_NO_HUMANS			0x00004000	// spawn point just for bots
#define FL_FORCE_GESTURE		0x00008000	// force gesture on client

// for delagged projectiles
//#define	MISSILE_PRESTEP_MAX_LATENCY 250
#define	DELAG_MAX_BACKTRACK (g_delagMissileMaxLatency.integer + 1000/sv_fps.integer * 2)

#define PLASMA_THINKTIME 10000

#define MUTED_ALWAYS 1
#define MUTED_GAME 2
#define MUTED_INTERMISSION 4
#define MUTED_RENAME 8

#define RULES_FAST 0
#define RULES_SLOW 1

#define ITEMDROP_FLAG 1
#define ITEMDROP_WEAPON 2

#define KILLCMD_GAME	1
#define KILLCMD_WARMUP	2

// movers are things like doors, plats, buttons, etc
typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1
} moverState_t;

#define SP_PODIUM_MODEL		"models/mapobjects/podium/podium4.md3"

//============================================================================

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

struct gentity_s {
	entityState_t	s;				// communicated by server to clients
	entityShared_t	r;				// shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gclient_s	*client;			// NULL if not a client

	qboolean	inuse;

	char		*classname;			// set in QuakeEd
	int			spawnflags;			// set in QuakeEd

	qboolean	neverFree;			// if true, FreeEntity will only unlink
									// bodyque uses this

	int			flags;				// FL_* variables

	char		*model;
	char		*model2;
	int			freetime;			// level.time when the object was freed
	
	int			eventTime;			// events will be cleared EVENT_VALID_MSEC after set
	qboolean	freeAfterEvent;
	qboolean	unlinkAfterEvent;

	qboolean	physicsObject;		// if true, it can be pushed by movers and fall off edges
									// all game items are physicsObjects, 
	float		physicsBounce;		// 1.0 = continuous bounce, 0.0 = no bounce
	int			clipmask;			// brushes with this content value will be collided against
									// when moving.  items and corpses do not collide against
									// players, for instance

	// movers
	moverState_t moverState;
	int			soundPos1;
	int			sound1to2;
	int			sound2to1;
	int			soundPos2;
	int			soundLoop;
	gentity_t	*parent;
	gentity_t	*nextTrain;
	gentity_t	*prevTrain;
	vec3_t		pos1, pos2;

	char		*message;

	int			timestamp;		// body queue sinking, etc

	float		angle;			// set in editor, -1 = up, -2 = down
	char		*target;
	char		*targetname;
	char		*team;
	char		*targetShaderName;
	char		*targetShaderNewName;
	gentity_t	*target_ent;

	float		speed;
	vec3_t		movedir;

	int			nextthink;
	void		(*think)(gentity_t *self);
	void		(*reached)(gentity_t *self);	// movers call this when hitting endpoint
	void		(*blocked)(gentity_t *self, gentity_t *other);
	void		(*touch)(gentity_t *self, gentity_t *other, trace_t *trace);
	void		(*use)(gentity_t *self, gentity_t *other, gentity_t *activator);
	void		(*pain)(gentity_t *self, gentity_t *attacker, int damage);
	void		(*die)(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);

	int			pain_debounce_time;
	int			fly_sound_debounce_time;	// wind tunnel
	int			last_move_time;

	int			health;

	qboolean	takedamage;

	int			damage;
	int			splashDamage;	// quad will increase this without increasing radius
	int			splashRadius;
	int			methodOfDeath;
	int			splashMethodOfDeath;

	int			count;

	int			dropPickupTime; // time at which this item can be picked up by the player who dropped it again
	int			dropClientNum;

	gentity_t	*chain;
	gentity_t	*enemy;
	gentity_t	*activator;
	gentity_t	*teamchain;		// next entity in team
	gentity_t	*teammaster;	// master of the team

	int			kamikazeTime;
	int			kamikazeShockTime;

	int			watertype;
	int			waterlevel;

	int			noise_index;

	// timing variables
	float		wait;
	float		random;

	gitem_t		*item;			// for bonus items

	// for delagged projectiles
	qboolean	needsDelag;
	int		launchTime;
	int		missileRan;

	int		pushed_at;

	qboolean	missileExploded;

	qboolean	missileTeleportCount;

	// for EAWARD_TELEMISSILE_FRAG:
	qboolean	missileTeleported;

	qboolean	teamToken;

	// for ra3compat
	int arenaNum;
};


typedef enum {
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum {
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum {
	TEAM_BEGIN,		// Beginning a team game, spawn at base
	TEAM_ACTIVE		// Now actively playing
} playerTeamStateState_t;

typedef struct {
	playerTeamStateState_t	state;

	int			location;

	int			captures;
	int			basedefense;
	int			carrierdefense;
	int			flagrecovery;
	int			fragcarrier;
	int			assists;

	int		lasthurtcarrier;
	int		lastreturnedflag;
	int		flagsince;
	int		lastfraggedcarrier;
	int 		lastFlagFromBase;
} playerTeamState_t;

// the auto following clients don't follow a specific client
// number, but instead follow the first two active players
#define	FOLLOW_ACTIVE1	-1
#define	FOLLOW_ACTIVE2	-2

typedef enum {
	UNNAMEDSTATE_CLEAN = 0,
	UNNAMEDSTATE_ISUNNAMED,
	UNNAMEDSTATE_WASRENAMED
} unnamedRenameState_t;

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct {
	team_t		sessionTeam;
	int			spectatorNum;		// for determining next-in-line to play
	spectatorState_t	spectatorState;
	int			spectatorClient;	// for chasecam and follow mode
	spectatorGroup_t	spectatorGroup;
	int			wins, losses;		// tournament stats
	qboolean	teamLeader;			// true when this client is a team leader
	qboolean    muted;
	unnamedRenameState_t unnamedPlayerState;
} clientSession_t;


//
#define MAX_NETNAME			36
#define	MAX_VOTE_COUNT		"3"

#define NEXTMAPVOTE_NUM_MAPS 6

#define NEXTMAPVOTE_CANVOTE 	1
#define NEXTMAPVOTE_VOTED 	2

#define MAX_MAPS 1024
#define MAX_MAPNAME 32
#define MAPS_PER_PAGE 10
#define MAPS_PER_LARGEPAGE 30
#define MAX_MAPNAME_BUFFER ((MAX_MAPNAME+5)*MAX_MAPS)
#define MAX_MAPNAME_LENGTH 34

#define MAX_CUSTOMNAME  MAX_MAPNAME
#define MAX_CUSTOMCOMMAND 100
#define MAX_CUSTOMDISPLAYNAME 50
#define MAX_CUSTOMDESC  256

typedef struct {
	int pagenumber;
	char mapname[MAPS_PER_LARGEPAGE][MAX_MAPNAME];
} t_mappage;

struct maplist_s {
	int num;
	char mapname[MAX_MAPS][MAX_MAPNAME];
};

//unlagged - true ping
#define NUM_PING_SAMPLES 64
//unlagged - true ping

// client flags
#define CLF_FIRSTCONNECT 1 // client was not carried over from previous level

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct {
	clientConnected_t	connected;	
	int clientFlags;	
	usercmd_t	cmd;				// we would lose angles if not persistant
	qboolean	localClient;		// true if "ip" info key is "localhost"
	qboolean	initialSpawn;		// the first spawn should be at a cool location
	qboolean	predictItemPickup;	// based on cg_predictItems userinfo
	qboolean	pmoveFixed;			//
	char		netname[MAX_NETNAME];
	int			maxHealth;			// for handicapping
	int			enterTime;			// level.time the client entered the game
	playerTeamState_t teamState;	// status in teamplay games
	int			voteCount;			// to prevent people from constantly calling votes
	int			teamVoteCount;		// to prevent people from constantly calling votes
	qboolean	teamInfo;			// send team overlay updates?
	//elimination:
	int		roundReached;			//Only spawn if we are new to this round
	int		livesLeft;			//lives in LMS

//unlagged - client options
	// these correspond with variables in the userinfo string
	int			delag;
//	int			debugDelag;
	int			cmdTimeNudge;

	// if client is completley pure, i.e. runs this cgame and ui qvm
	// this is not completely reliable
	int			pure;

//unlagged - client options
//unlagged - lag simulation #2
/*	int			latentSnaps;
	int			latentCmds;
	int			plOut;
	usercmd_t	cmdqueue[MAX_LATENT_CMDS];
	int			cmdhead;*/
//unlagged - lag simulation #2
//unlagged - true ping
	int			realPing;
	int			pingsamples[NUM_PING_SAMPLES];
	unsigned int		pingsample_counter;
	//int			samplehead;
//unlagged - true ping
//KK-OAX Killing Sprees/Multikills
    int         killstreak;
    int         deathstreak;
    qboolean    onSpree;
    int         multiKillCount;
    
//KK-OAX Admin Stuff        
    char        guid[ 33 ];
    char        ip[ 40 ];
    qboolean    disoriented;
    qboolean    wasdisoriented;
    int         adminLevel;

// flood protection
    int         floodDemerits;
    int         floodTime;
 
//Used To Track Name Changes
    int         nameChangeTime;
    int         nameChanges;
    
    int		dmgTaken, dmgGiven;
    int		kills, deaths;
    int		damage[WP_NUM_WEAPONS];
    int		topweapons[WP_NUM_WEAPONS][2];
    int		handicapforced;		

    int		elimRoundDmgDone;
    int		elimRoundDmgTaken;
    int		elimRoundKills;

    // quad whore detection
    int quadTime;
    int quadNum;
    int quadWhore;

    int	unnamedPlayerRenameTime;
    qboolean forceRename; // set to qtrue while a player is forcefully renamed

    int awardCounts[EAWARD_NUM_AWARDS];

    int nextmapVoteFlags;
    int nextmapVote;

    int th_tokens;

    int tauntTime;

    // for ra3compat
    int arenaNum;

    // for awards, clientno of enemy that last killed us, reset upon strongman award
    int lastKilledByStrongMan;

    // for revenge award, entitynum of enemy that last killed us
    int lastKilledBy;
    // time of last death;
    int lastDeathTime;
    qboolean gotRevenge;

    int gauntCorpseGibCount;

    int	joinedByTeamQueue;
    int	queueJoinTime; // time at which the queue was joined
} clientPersistant_t;

// for twitchrail award
// might need to be increased if a server uses sv_fps > 40
#define VIEWVECTOR_HISTORY 10

#define DAMAGE_HISTORY 16
typedef struct {
	gentity_t *target;
	int mod;
	int damage;
	qboolean died;
	int time;
} damageRecord_t;

//unlagged - backward reconciliation #1
// the size of history we'll keep
#define NUM_CLIENT_HISTORY 17

// everything we need to know to backward reconcile
typedef struct {
	vec3_t		mins, maxs;
	vec3_t		currentOrigin;
	int			leveltime;
} clientHistory_t;
//unlagged - backward reconciliation #1

// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s {
	// ps MUST be the first element, because the server expects it
	playerState_t	ps;				// communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t	pers;
	clientSession_t		sess;

	qboolean	readyToExit;		// wishes to leave the intermission
	qboolean	ready;		// wishes to start the game

	qboolean	noclip;

	//Unlagged - commented out - handled differently
	//int			lastCmdTime;		// level.time of last usercmd_t, for EF_CONNECTION
									// we can't just use pers.lastCommand.time, because
									// of the g_sycronousclients case
	int			buttons;
	int			oldbuttons;
	int			latched_buttons;

	vec3_t		oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int			damage_armor;		// damage absorbed by armor
	int			damage_blood;		// damage taken out of health
	int			damage_knockback;	// impact damage
	vec3_t		damage_from;		// origin for vector calculation
	qboolean	damage_fromWorld;	// if true, don't use the damage_from vector

	// damage taken for EAWARD_IMMORTALITY. Cleared each time the award is issue
	int		dmgTakenImmortality;

	// for EAWARD_RAT, number of consecutive frags with <= 5 HP
	int	ratFragCounter;

	int			accurateCount;		// for "impressive" reward sound

	int			accuracy_shots;		// total number of shots
	int			accuracy_hits;		// total number of hits

	int			consecutive_hits; 		// for EAWARD_ACCURACY	
	int			consecutive_hits_weapon;	// these get reset upon respawn

	//
	int			lastkilled_client;	// last client that this client killed
	int			lasthurt_client;	// last client that damaged this client
	int			lasthurt_mod;		// type of damage the client did

	// timers
	int			respawnTime;		// can respawn when time > this, force after g_forcerespwan
	int			inactivityTime;		// kick players when time > this
	qboolean	inactivityWarning;	// qtrue if the five seoond warning has been given
	qboolean	inactivityLastSuspend; // helps to track inactivity in elimination modes
	int			rewardTime;			// clear the EF_AWARD_IMPRESSIVE, etc when time > this

	int			elimRespawnTime;		// will respawn at this time in GT_ELIMINATION

	int			airOutTime;
	int			lavaDmgTime;

	int			lastKillTime;		// for multiple kill rewards

	qboolean	fireHeld;			// used for hook
	gentity_t	*hook;				// grapple hook if out

	int			switchTeamTime;		// time the player switched teams

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int			timeResidual;

	gentity_t	*persistantPowerup;
	int			portalID;
	int			ammoTimes[WP_NUM_WEAPONS];
        int			invulnerabilityTime;


	char		*areabits;

	qboolean	isEliminated;			//Has been killed in this round

        //New vote system. The votes are saved in the client info, so we know who voted on what and can cancel votes on leave.
        //0=not voted, 1=voted yes, -1=voted no
        int vote;
        
        int lastSentFlying;                             //The last client that sent the player flying
        int lastSentFlyingTime;                         //So we can time out

        int lastGroundTime;                             // the last time the player touched the ground

	// for rocker + rail, lg + rail combo awards
        int lastDmgGivenEntityNum; 
        int lastDmgGivenTime; 
        int lastDmgGivenMOD; 

	// for ambush award
	int lastTeleportTime;

	// for berserker award
	int gauntSpree;

	// for twitchrail award
	vec3_t viewvector_history[VIEWVECTOR_HISTORY];
	int    viewvector_head;
	int    viewvector_historysize;
        

	//unlagged - backward reconciliation #1
	// the serverTime the button was pressed
	// (stored before pmove_fixed changes serverTime)
	int			attackTime;
	// the head of the history queue
	int			historyHead;
	// the history queue
	clientHistory_t	history[NUM_CLIENT_HISTORY];
	// the client's saved position
	clientHistory_t	saved;			// used to restore after time shift
	// an approximation of the actual server time we received this
	// command (not in 50ms increments)
	int			frameOffset;

	// the level.time to which this client was shifted, or 0 if it isn't timeshifted
	int timeshiftTime;
//unlagged - backward reconciliation #1

//unlagged - smooth clients #1
	// the last frame number we got an update from this client
	int			lastUpdateFrame;
//unlagged - smooth clients #1
        qboolean        spawnprotected;

        int			accuracy[WP_NUM_WEAPONS][2];

	int damage_history_head;
	damageRecord_t damage_history[DAMAGE_HISTORY];

	int		timeouts; // number of timeouts called;

	qboolean pingHeld;
};


//
// this structure is cleared as each map is entered
//
#define	MAX_SPAWN_VARS			64
#define	MAX_SPAWN_VARS_CHARS	4096

typedef struct {
	struct gclient_s	*clients;		// [maxclients]

	struct gentity_s	*gentities;
	int			gentitySize;
	int			num_entities;		// current number, <= MAX_GENTITIES

	int			warmupTime;			// restart match at this time

	fileHandle_t	logFile;

	// store latched cvars here that we want to get at often
	int			maxclients;

	int			framenum;
	int			time;					// in msec
	int			previousTime;			// so movers can back up when blocked

	int			realtime;			// real level.time that advances during timeouts

	int			startTime;				// level.time the map was started

	int 			overtimeCount;			// number of times the game was extended by g_overtime seconds

	int			teamScores[TEAM_NUM_TEAMS];
	int			lastTeamLocationTime;		// last time of client team location update

	qboolean	newSession;				// don't use any old session data, because
										// we changed gametype

	qboolean	restarted;				// waiting for a map_restart to fire

	int			numConnectedClients;
	int			numNonSpectatorClients;	// includes connecting clients
	int			numPlayingClients;		// connected, non-spectators
	int			sortedClients[MAX_CLIENTS];		// sorted by score
	int			follow1, follow2;		// clientNums for auto-follow spectators
	int			followauto;			// clientnum for action auto-follow
	int			followautoTime;			

	int			snd_fry;				// sound index for standing in lava

	int			warmupModificationCount;	// for detecting if g_warmup is changed

	// voting state
	char		voteString[MAX_STRING_CHARS];
	char		voteDisplayString[MAX_STRING_CHARS];
	int			voteTime;				// level.time vote was called
	int			voteExecuteTime;		// time the vote is executed
	int			voteYes;
	int			voteNo;
	int			numVotingClients;		// set by CountVotes
        int             voteKickClient;                         // if non-negative the current vote is about this client.
        int             voteKickType;                           // if 1 = ban (execute ban)
        qboolean        voteLightAllowed;       // set if this vote can be passed by light voting (DF_LIGHT_VOTING)
        float        votePassRatio;       // required yes/no ratio to pass the vote (if > 0)
	char		lastFailedVote[MAX_STRING_CHARS]; // old votestring to detect repeated votes
	int		lastFailedVoteTime; 		 
	int		lastFailedVoteCount; 		 

	// team voting state
	char		teamVoteString[2][MAX_STRING_CHARS];
	int			teamVoteTime[2];		// level.time vote was called
	int			teamVoteYes[2];
	int			teamVoteNo[2];
	int			numteamVotingClients[TEAM_NUM_TEAMS];// set by CalculateRanks

	// spawn variables
	qboolean	spawning;				// the G_Spawn*() functions are valid
	int			numSpawnVars;
	char		*spawnVars[MAX_SPAWN_VARS][2];	// key / value pairs
	int			numSpawnVarChars;
	char		spawnVarChars[MAX_SPAWN_VARS_CHARS];

	// intermission state
	int			intermissionQueued;		// intermission was qualified, but
										// wait INTERMISSION_DELAY_TIME before
										// actually going there so the last
										// frag can be watched.  Disable future
										// kills during this delay
	int			intermissiontime;		// time the intermission was started
	char		*changemap;
	qboolean	readyToExit;			// at least one client wants to exit
	int			exitTime;
	vec3_t		intermission_origin;	// also used for spectator spawns
	vec3_t		intermission_angle;

	int		readyMask;

	qboolean	timeout;
	int		timeoutAdd;
	int		timeoutEnd;
	int		timeoutOvertime;
	int		timeoutTotalPausedTime;
	qboolean	timein;

	qboolean	locationLinked;			// target_locations get linked
	gentity_t	*locationHead;			// head of the location list
	int			bodyQueIndex;			// dead bodies
	gentity_t	*bodyQue[BODY_QUEUE_SIZE];
	int			portalSequence;
	//Added for elimination:
	int roundStartTime;		//time the current round was started
	int roundNumber;			//The round number we have reached
	int roundNumberStarted;			//1 less than roundNumber if we are allowed to spawn
	int roundRedPlayers;			//How many players was there at start of round
	int roundBluePlayers;			//used to find winners in a draw.
	qboolean roundRespawned;		//We have respawned for this round!
	int eliminationSides;			//Random, change red/blue bases
	int elimBlueRespawnDelay;		
	int elimRedRespawnDelay;		

	//Added for Double Domination
	//Points get status: TEAM_FREE for not taking, TEAM_RED/TEAM_BLUE for taken and TEAM_NONE for not spawned yet
	int pointStatusA;			//Status of the RED (A) domination point
	int pointStatusB;			//Status of the BLUE (B) doimination point
	int timeTaken;				//Time team started having both points
	//use roundStartTime for telling, then the points respawn

	//Added for standard domination
	int pointStatusDom[MAX_DOMINATION_POINTS]; //Holds the owner of all the points
	int dom_scoreGiven;				//Number of times we have provided scores
	int domination_points_count;
	char domination_points_names[MAX_DOMINATION_POINTS][MAX_DOMINATION_POINTS_NAMES];

	int th_round;
	int th_roundFinished;
	int th_hideTime;
	//qboolean th_hideActive;
	//qboolean th_hideFinished;
	//qboolean th_seekActive;
	treasurehunter_t th_phase;
	int th_seekTime;
	int th_redClientMask;
	int th_blueClientMask;
	int th_specClientMask;
	int th_placedTokensBlue;
	int th_placedTokensRed;
	int th_teamTokensRed;
	int th_teamTokensBlue;
	int th_lastClientTokenUpdate;
	int th_allTokensHiddenTime;

	//Added to keep track of challenges (can only be completed against humanplayers)
	qboolean hadBots;	//There was bots in the level
	int teamSize;		//The highest number of players on the least populated team when there was most players

//unlagged - backward reconciliation #4
	// actual time this server frame started
	int			frameStartTime;
//unlagged - backward reconciliation #4
//KK-OAX Storing upper bounds of spree/multikill arrays 
    int         kSpreeUBound;
    int         dSpreeUBound;
    int         mKillUBound;
//KK-OAX Storing g_spreeDiv to avoid dividing by 0.    
    int         spreeDivisor;

    qboolean    RedTeamLocked;
    qboolean    BlueTeamLocked;
    qboolean    FFALocked;

    qboolean 	autoLocked;
    
    //Obelisk tell
    int healthRedObelisk; //health in percent
    int healthBlueObelisk; //helth in percent
    qboolean MustSendObeliskHealth; //Health has changed

    qboolean	pingEqualized;

    qboolean	tournamentForfeited;

    qboolean	shuffling_teams;
    // restart the game at this time. Separate from warmupTime because this
    // should never be modified by /ready logic and such. It only exists to
    // delay a map_restart after shuffling to avoid a command overflow
    int		restartAt; 

    int		eqPing; // ping to which all pings are equalized
    int		eqPingAdjustTime; // time of last EQPing update

    int		nextMapVoteManual;
    int		nextMapVoteTime;
    int		nextMapVoteExecuted;
    int		nextMapVoteClients;
    int		nextmapVotes[NEXTMAPVOTE_NUM_MAPS];
    char	nextmapVoteMaps[NEXTMAPVOTE_NUM_MAPS][MAX_MAPNAME];

    qboolean arenasLoaded;

    int teamBalanceTime;
     
} level_locals_t;

//KK-OAX These are some Print Shortcuts for KillingSprees and Admin
//KK-Moved to g_admin.h
//Prints to All when using "va()" in conjunction.
//#define AP(x)   trap_SendServerCommand(-1, x) 

//
// g_spawn.c
//
qboolean	G_SpawnString( const char *key, const char *defaultString, char **out );
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean	G_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean	G_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean	G_SpawnVector( const char *key, const char *defaultString, float *out );
void		G_SpawnEntitiesFromString( void );
char *G_NewString( const char *string );

//
// g_cmds.c
//
void Cmd_Score_f (gentity_t *ent);
void StopFollowing( gentity_t *ent );
void BroadcastTeamChange( gclient_t *client, int oldTeam );
void SetTeam( gentity_t *ent, char *s );
void SetTeam_Force( gentity_t *ent, char *s, gentity_t *by, qboolean tryforce );
void Cmd_FollowCycle_f( gentity_t *ent );  //KK-OAX Changed to match definition
char *ConcatArgs( int start );  //KK-OAX This declaration moved from g_svccmds.c
//KK-OAX Added this to make accessible from g_svcmds_ext.c
void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ); 
void G_Timein(void);
void G_TimeinWarning(int levelTime);
void G_Timeout(gentity_t *caller);
void G_TimeinCommand(gentity_t *caller);
void G_TimeoutReminder(gentity_t *ent);
void SendSpectatorGroup( gentity_t *ent );
void SendReadymask(int clientnum);
void G_SendSpawnpoints(gentity_t *ent);
qboolean G_TournamentSpecMuted(void);
void AccMessage( gentity_t *ent );
void AwardMessage(gentity_t *ent, extAward_t award, int count);
qboolean SendNextmapVoteCommand( void );
int G_GametypeBitsCurrent( void );


// KK-OAX Added these in a seperate file to keep g_cmds.c familiar. 
// g_cmds_ext.c
//

int         G_SayArgc( void );
qboolean    G_SayArgv( int n, char *buffer, int bufferLength );
char        *G_SayConcatArgs( int start );
void        G_DecolorString( char *in, char *out, int len );
void        G_MatchOnePlayer( int *plist, int num, char *err, int len );
void        G_SanitiseString( char *in, char *out, int len );
int         G_ClientNumbersFromString( char *s, int *plist, int max );
int         G_FloodLimited( gentity_t *ent );
//void QDECL G_AdminMessage( const char *prefix, const char *fmt, ... ) 
// ^^ Do Not Need to Declare--Just for Documentation of where it is.
void        Cmd_AdminMessage_f( gentity_t *ent );
int         G_ClientNumberFromString( char *s );
qboolean    G_ClientIsLagging( gclient_t *client );
void        SanitizeString( char *in, char *out );

// KK-OAX Added this for common file stuff between Admin and Sprees.
// g_fileops.c
//
void readFile_int( char **cnf, int *v );
void readFile_string( char **cnf, char *s, int size );
void writeFile_int( int v, fileHandle_t f );
void writeFile_string( char *s, fileHandle_t f );

//
// g_items.c
//
void G_CheckTeamItems( void );
void G_RunItem( gentity_t *ent );
void RespawnItem( gentity_t *ent );

void UseHoldableItem( gentity_t *ent );
void PrecacheItem (gitem_t *it);
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle );
gentity_t *Drop_ItemNonRandom( gentity_t *ent, gitem_t *item, float angle );
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity );
void SetRespawn (gentity_t *ent, float delay);
void G_SpawnItem (gentity_t *ent, gitem_t *item);
void FinishSpawningItem( gentity_t *ent );
void Think_Weapon (gentity_t *ent);
int ArmorIndex (gentity_t *ent);
void	Add_Ammo (gentity_t *ent, int weapon, int count);
void Touch_Item (gentity_t *ent, gentity_t *other, trace_t *trace);

void ClearRegisteredItems( void );
void RegisterItem( gitem_t *item );
void SaveRegisteredItems( void );

//
// g_utils.c
//
int G_ModelIndex( char *name );
int		G_SoundIndex( char *name );
void	G_TeamCommand( team_t team, char *cmd );
void	G_KillBox (gentity_t *ent);
gentity_t *G_Find (gentity_t *from, int fieldofs, const char *match);
gentity_t *G_PickTarget (char *targetname);
void	G_UseTargets (gentity_t *ent, gentity_t *activator);
void	G_SetMovedir ( vec3_t angles, vec3_t movedir);

void	G_InitGentity( gentity_t *e );
gentity_t	*G_Spawn (void);
gentity_t *G_TempEntity( vec3_t origin, int event );
void	G_Sound( gentity_t *ent, int channel, int soundIndex );

//KK-OAX For Playing Sounds Globally
void    G_GlobalSound( int soundIndex );

void	    G_FreeEntity( gentity_t *e );
qboolean	G_EntitiesFree( void );

void	G_TouchTriggers (gentity_t *ent);
void	G_TouchSolids (gentity_t *ent);

float	*tv (float x, float y, float z);
char	*vtos( const vec3_t v );

float vectoyaw( const vec3_t vec );

void G_AddPredictableEvent( gentity_t *ent, int event, int eventParm );
void G_AddEvent( gentity_t *ent, int event, int eventParm );
void G_SetOrigin( gentity_t *ent, vec3_t origin );
void AddRemap(const char *oldShader, const char *newShader, float timeOffset);
void ClearRemaps(void);
const char *BuildShaderStateConfig( void );

//
// g_combat.c
//
qboolean CanDamage (gentity_t *targ, vec3_t origin);
void G_Damage (gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod);
qboolean G_RadiusDamage (vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod);
qboolean G_RailJump( vec3_t origin, gentity_t *attacker);
int G_InvulnerabilityEffect( gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir );
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath );
void TossClientItems( gentity_t *self );
void TossClientPersistantPowerups( gentity_t *self );
void TossClientCubes( gentity_t *self );
void G_CheckKamikazeAward(gentity_t *attacker, int killsBefore, int deathsBefore);
void G_StoreViewVectorHistory ( gclient_t *client );

// damage flags
#define DAMAGE_RADIUS				0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR				0x00000002	// armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK			0x00000004	// do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION		0x00000008  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_NO_TEAM_PROTECTION	0x00000010  // armor, shields, invulnerability, and godmode have no effect

//
// g_missile.c
//
void G_RunMissile( gentity_t *ent );
int G_MissilePrestep( gclient_t *client);
void G_MissileRunDelag( gentity_t *ent, int stepmsec);
void G_ImmediateRunMissiles(gentity_t *client);
void G_ImmediateLaunchMissile(gentity_t *ent);
void G_ImmediateRunClientMissiles(gentity_t *client);
void ProximityMine_RemoveAll( void );

gentity_t *fire_blaster (gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_plasma (gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_grenade (gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_rocket (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_bfg (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_nail( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up, int *seed );
gentity_t *fire_prox( gentity_t *self, vec3_t start, vec3_t aimdir );


//
// g_mover.c
//
void G_RunMover( gentity_t *ent );
void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace );

//
// g_trigger.c
//
void trigger_teleporter_touch (gentity_t *self, gentity_t *other, trace_t *trace );
void G_SetTeleporterDestinations(void);


//
// g_misc.c
//
void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles );
void DropPortalSource( gentity_t *ent );
void DropPortalDestination( gentity_t *ent );


//
// g_weapon.c
//
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker );
void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint );
//unlagged - attack prediction #3
// we're making this available to both games
void SnapVectorTowards( vec3_t v, vec3_t to );
//unlagged - attack prediction #3
qboolean CheckGauntletAttack( gentity_t *ent );
void Weapon_HookFree (gentity_t *ent);
void Weapon_HookThink (gentity_t *ent);

void G_PingLocation( gentity_t *ent, locationping_t pingtype );

//unlagged - g_unlagged.c
void G_ResetHistory( gentity_t *ent );
void G_StoreHistory( gentity_t *ent );
void G_TimeShiftAllClients( int time, gentity_t *skip );
void G_UnTimeShiftAllClients( gentity_t *skip );
void G_DoTimeShiftFor( gentity_t *ent );
void G_UndoTimeShiftFor( gentity_t *ent );
void G_UnTimeShiftClient( gentity_t *client );
void G_TimeShiftClient( gentity_t *ent, int time, qboolean debug, gentity_t *debugger );
void G_PredictPlayerMove( gentity_t *ent, float frametime );
//unlagged - g_unlagged.c

//
// g_client.c
//
team_t TeamCount( int ignoreClientNum, int team, qboolean countBots);
team_t TeamCountExt( int ignoreClientNum, int team, qboolean countBots, qboolean countConnecting );
team_t TeamLivingCount( int ignoreClientNum, int team ); //Elimination
team_t TeamHealthCount( int ignoreClientNum, int team ); //Elimination
void RespawnAll(void); //For LMS
void RespawnAllElim(void); //For round elimination
void RespawnDead(qboolean force);
void EliminationRespawnClient(gentity_t *ent);
int RespawnElimZombies(void);
void EnableWeapons(void);
void DisableWeapons(void);
void EndEliminationRound(void);
void LMSpoint(void);
//void wins2score(void);
int TeamLeader( int team );
team_t PickTeam( int ignoreClientNum );
void SetClientViewAngle( gentity_t *ent, vec3_t angle );
gentity_t *SelectSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles );
gentity_t *SelectSpawnPointArena ( int arenaNum,  vec3_t avoidPoint, vec3_t origin, vec3_t angles );
gentity_t *SelectRandomDeathmatchSpawnPointArena( int arenaNum );
gentity_t *SelectFarFromEnemyTeamSpawnpoint ( team_t myteam, vec3_t origin, vec3_t angles);
gentity_t *SelectFarFromEnemyTeamSpawnpointArena ( int arenaNum, team_t myteam, vec3_t origin, vec3_t angles);
void CopyToBodyQue( gentity_t *ent );
void ClientRespawn(gentity_t *ent);
void BeginIntermission (void);
void InitClientPersistant (gclient_t *client);
void InitClientResp (gclient_t *client);
void InitBodyQue (void);
void ClientSpawn( gentity_t *ent );
void player_die (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);
void AddScore( gentity_t *ent, vec3_t origin, int score );
void CalculateRanks( void );
qboolean SpotWouldTelefrag( gentity_t *spot );
void motd_chat (gentity_t *ent);
void G_UpdateTopWeapons(gclient_t *client);
qboolean G_MixedClientHasRatVM(gclient_t *client);
void G_UnnamedPlayerRename(gentity_t *ent);
qboolean G_RA3ArenaAllowed(int arenaNum);
void G_LoadClans(void);

//
// g_svcmds.c
//
qboolean	ConsoleCommand( void );
void G_ProcessIPBans(void);
qboolean G_FilterPacket (char *from);
void VoteNextmap_f ( void );

//KK-OAX Added this to make accessible from g_svcmds_ext.c
gclient_t	*ClientForString( const char *s );

//
// g_weapon.c
//
void FireWeapon( gentity_t *ent );
void G_StartKamikaze( gentity_t *ent );

//
// p_hud.c
//
void MoveClientToIntermission (gentity_t *client);
void G_SetStats (gentity_t *ent);

//
// g_cmds.c
// Also another place /Sago

void DoubleDominationScoreTimeMessage( gentity_t *ent );
void YourTeamMessage( gentity_t *ent);
void AttackingTeamMessage( gentity_t *ent );
void ObeliskHealthMessage( void );
void DeathmatchScoreboardMessage (gentity_t *client, qboolean advanced);
void DeathmatchScoreboardMessageAuto (gentity_t *client);
void EliminationMessage (gentity_t *client);
void RespawnTimeMessage(gentity_t *ent, int time);
void DominationPointNamesMessage (gentity_t *client);
void DominationPointStatusMessage( gentity_t *ent );
void ChallengeMessage( gentity_t *ent, int challengeNumber );
void SendCustomVoteCommands(int clientNum);
void TreasureHuntMessage(gentity_t *ent);

//
// g_pweapon.c
//


//
// g_main.c
//
void FindIntermissionPoint( void );
void FindIntermissionPointArena( int arenaNum, vec3_t origin, vec3_t angles );
void SetLeader(int team, int client);
void CheckTeamLeader( int team );
void G_RunThink (gentity_t *ent);
void AddTournamentQueue(gclient_t *client);
void AddTournamentQueueFront(gclient_t *client);
qboolean QueueIsConnectingPhase(void);
gentity_t *G_FindPlayerLastJoined(int team);
void CheckTeamBalance( void );
void ExitLevel( void );
void QDECL G_LogPrintf( const char *fmt, ... );
void SendScoreboardMessageToAllClients( void );
void SendEliminationMessageToAllClients( void );
void SendDDtimetakenMessageToAllClients( void );
void SendDominationPointsStatusMessageToAllClients( void );
void SendTreasureHuntMessageToAllClients( void );
void SendYourTeamMessageToTeam( team_t team );
void QDECL G_Printf( const char *fmt, ... );
void QDECL G_Error( const char *fmt, ... ) __attribute__((noreturn));
//KK-OAX Made Accessible for g_admin.c
void LogExit( const char *string ); 
void CheckTeamVote( int team );
void G_PingEqualizerReset(void);
void G_SetRuleset(int ruleset);
void G_LockTeams(void);
void G_UnlockTeams(void);
void G_CheckUnlockTeams(void);
void G_EQPingSet(int maxEQPing, qboolean forceMax);
void G_EQPingClientSet(gclient_t *client);
void G_EQPingClientReset(gclient_t *client);

//
// g_client.c
//
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot );
void ClientUserinfoChanged( int clientNum );
void ClientUserinfoChangedLimited( int clientNum );
void ClientDisconnect( int clientNum );
void ClientBegin( int clientNum );
void ClientCommand( int clientNum );

//
// g_active.c
//
void ClientThink( int clientNum );
void ClientEndFrame( gentity_t *ent );
void G_RunClient( gentity_t *ent );
void ClientInactivityHeartBeat(gclient_t *client);

//
// g_team.c
//
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 );
void Team_CheckDroppedItem( gentity_t *dropped );
qboolean CheckObeliskAttack( gentity_t *obelisk, gentity_t *attacker );
void ShuffleTeams(void);
//KK-OAX Added for Command Handling Changes (r24)
team_t G_TeamFromString( char *str );
void G_SendTeamPlayerCounts(void);
void Token_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);
void Team_TH_TokenDestroyed( gentity_t *ent );
void SetPlayerTokens(int num, qboolean updateOnly);
void Token_Think( gentity_t *token );
gentity_t *SelectElimSpawnPointArena ( team_t team, int teamstate, int arenaNum, vec3_t origin, vec3_t angles );
gentity_t *SelectElimSpawnPoint ( team_t team, int teamstate, vec3_t origin, vec3_t angles );
int G_TeamClientMask(int team);

//KK-OAX Removed these in Code in favor of bg_alloc.c from Tremulous
// g_mem.c
//
//void *G_Alloc( int size );
//void G_InitMemory( void );

//KK-OAX This was moved
// bg_alloc.c
//
void Svcmd_GameMem_f( void );

//
// g_session.c
//
void G_ReadSessionData( gclient_t *client );
void G_InitSessionData( gclient_t *client, char *userinfo, qboolean firstTime,
	       	qboolean levelNewSession, unnamedRenameState_t unnamedPlayerState );

void G_InitWorldSession( void );
void G_WriteSessionData( void );

//
// g_arenas.c
//
void UpdateTournamentInfo( void );
void SpawnModelsOnVictoryPads( void );
void Svcmd_AbortPodium_f( void );

//
// g_bot.c
//
void G_InitBots( qboolean restart );
char *G_GetBotInfoByNumber( int num );
char *G_GetBotInfoByName( const char *name );
void G_CheckBotSpawn( void );
void G_RemoveQueuedBotBegin( int clientNum );
qboolean G_BotConnect( int clientNum, qboolean restart );
void Svcmd_AddBot_f( void );
void Svcmd_BotList_f( void );
void BotInterbreedEndMatch( void );
int G_CountHumanPlayers( int team );
void G_LoadArenas( void );
const char *G_GetArenaInfoByMap( const char *map );
int G_GetArenaNumByMap( const char *map );
int G_GametypeBits( char *string );
int G_GametypeBitsForMap(const char *mapname);
void G_MapMinMaxPlayers(const char *mapname, int *minPlayers, int *maxPlayers);

//
// g_playerstore.c
//

void PlayerStoreInit( void );
void PlayerStore_store(char* guid, playerState_t ps);
void PlayerStore_restore(char* guid, gclient_t *client);

//
// g_vote.c
//
int allowedVote(char *commandStr);
void CheckVote( void );
void CountVotes( void );
void ClientLeaving(int clientNumber);
void G_SendVoteResult(qboolean passed);
void G_ResetRejectedVote(void);
qboolean G_CheckRejectedVote(void);
int VotePrintCustomVotes (gentity_t *ent);


typedef struct {
    char    votename[MAX_CUSTOMNAME]; //Used like "/callvote custom VOTENAME"
    char    displayname[MAX_CUSTOMDISPLAYNAME]; //Displayed during voting
    char    command[MAX_CUSTOMCOMMAND]; //The command executed
    char    description[MAX_CUSTOMDESC]; // Description / help
    qboolean lightvote; // set when light vote (non-majority vote) is allowed for this command
    float passRatio; // yes/no ratio required to pass this vote (or <= 0)
} t_customvote;

extern char custom_vote_info[2048];

extern t_mappage getMappage(int page, qboolean largepage, qboolean recommendedonly);
extern t_mappage getGTMappage(int page, qboolean largepage);
void getCompleteMaplist(qboolean recommenedonly, int gametypebits_filter, int numPlayers, struct maplist_s *out);
extern int allowedMap(char *mapname);
extern int allowedGametype(char *gametypeStr);
extern int allowedTimelimit(int limit);
extern int allowedFraglimit(int limit);
extern int allowedCapturelimit(int limit);
extern int allowedBots(int numbots);
extern int VoteParseCustomVotes( void );
extern t_customvote getCustomVote(char* votecommand);

// ai_main.c
#define MAX_FILEPATH			144

//bot settings
typedef struct bot_settings_s
{
	char characterfile[MAX_FILEPATH];
	float skill;
	char team[MAX_FILEPATH];
} bot_settings_t;

int BotAISetup( int restart );
int BotAIShutdown( int restart );
int BotAILoadMap( int restart );
int BotAISetupClient(int client, struct bot_settings_s *settings, qboolean restart);
int BotAIShutdownClient( int client, qboolean restart );
int BotAIStartFrame( int time );
void BotTestAAS(vec3_t origin);

#include "g_team.h" // teamplay specific stuff


extern	level_locals_t	level;
extern	gentity_t		g_entities[MAX_GENTITIES];

#define	FOFS(x) ((size_t)&(((gentity_t *)0)->x))

//CVARS 
extern	vmCvar_t	g_gametype;
extern	vmCvar_t	g_dedicated;
extern	vmCvar_t	g_cheats;
extern	vmCvar_t	g_maxclients;			// allow this many total, including spectators
extern	vmCvar_t	g_maxGameClients;		// allow this many active
extern	vmCvar_t	g_restarted;

extern	vmCvar_t	g_dmflags;
extern	vmCvar_t	g_videoflags;
extern	vmCvar_t	g_elimflags;
extern	vmCvar_t	g_voteflags;
extern	vmCvar_t	g_fraglimit;
extern	vmCvar_t	g_timelimit;
extern	vmCvar_t	g_capturelimit;
extern	vmCvar_t	g_overtime;
extern	vmCvar_t	g_friendlyFire;
extern	vmCvar_t	g_password;
extern	vmCvar_t	g_passwordVerifyConnected;
extern	vmCvar_t	g_needpass;
extern	vmCvar_t	g_gravity;
extern	vmCvar_t	g_gravityModifier;
extern	vmCvar_t	g_gravityJumppadFix;
extern  vmCvar_t        g_damageScore;
extern  vmCvar_t        g_damageModifier;
extern	vmCvar_t	g_speed;
extern	vmCvar_t	g_spectatorSpeed;
extern	vmCvar_t	g_knockback;
extern	vmCvar_t	g_quadfactor;
extern	vmCvar_t	g_forcerespawn;
extern	vmCvar_t	g_respawntime;
extern	vmCvar_t	g_inactivity;
extern	vmCvar_t	g_debugMove;
extern	vmCvar_t	g_debugAlloc;
extern	vmCvar_t	g_debugDamage;
extern	vmCvar_t	g_weaponRespawn;
extern	vmCvar_t	g_overrideWeaponRespawn;
extern	vmCvar_t	g_weaponTeamRespawn;
extern	vmCvar_t	g_synchronousClients;
extern	vmCvar_t	g_motd;
extern	vmCvar_t	g_motdfile;
extern  vmCvar_t        g_votemaps;
extern  vmCvar_t        g_recommendedMapsFile;
extern  vmCvar_t        g_votecustom;
extern  vmCvar_t        g_nextmapVote;
extern  vmCvar_t        g_nextmapVotePlayerNumFilter;
extern  vmCvar_t        g_nextmapVoteCmdEnabled;
extern  vmCvar_t        g_nextmapVoteNumRecommended;
extern  vmCvar_t        g_nextmapVoteNumGametype;
extern  vmCvar_t        g_nextmapVoteTime;
extern	vmCvar_t	g_warmup;
extern	vmCvar_t	g_doWarmup;
extern	vmCvar_t	g_logIPs;
extern	vmCvar_t	g_blood;
extern	vmCvar_t	g_allowVote;
extern	vmCvar_t	g_teamAutoJoin;
extern	vmCvar_t	g_teamForceBalance;
extern	vmCvar_t	g_teamForceQueue;
extern	vmCvar_t	g_teamBalance;
extern	vmCvar_t	g_teamBalanceDelay;
extern	vmCvar_t	g_banIPs;
extern	vmCvar_t	g_filterBan;
extern	vmCvar_t	g_obeliskHealth;
extern	vmCvar_t	g_obeliskRegenPeriod;
extern	vmCvar_t	g_obeliskRegenAmount;
extern	vmCvar_t	g_obeliskRespawnDelay;
extern	vmCvar_t	g_cubeTimeout;
extern	vmCvar_t	g_smoothClients;
extern	vmCvar_t	pmove_fixed;
extern	vmCvar_t	pmove_msec;
extern	vmCvar_t	pmove_float;
extern	vmCvar_t	g_floatPlayerPosition;
extern	vmCvar_t	g_rankings;
#ifdef MISSIONPACK
extern	vmCvar_t	g_singlePlayer;
extern	vmCvar_t	g_redteam;
extern	vmCvar_t	g_blueteam;
#endif
extern	vmCvar_t	g_redclan;
extern	vmCvar_t	g_blueclan;

extern vmCvar_t		g_treasureTokens;
extern vmCvar_t		g_treasureHideTime;
extern vmCvar_t		g_treasureSeekTime;
extern vmCvar_t		g_treasureRounds;
extern vmCvar_t		g_treasureTokensDestructible;
extern vmCvar_t		g_treasureTokenHealth;
extern vmCvar_t		g_treasureTokenStyle;

extern	vmCvar_t	g_enableDust;
extern	vmCvar_t	g_enableBreath;
extern	vmCvar_t	g_proxMineTimeout;
extern	vmCvar_t	g_music;
extern  vmCvar_t        g_spawnprotect;

//elimination:
extern	vmCvar_t	g_elimination_respawn;
extern	vmCvar_t	g_elimination_respawn_increment;
extern	vmCvar_t	g_elimination_selfdamage;
extern	vmCvar_t	g_elimination_startHealth;
extern	vmCvar_t	g_elimination_startArmor;
extern	vmCvar_t	g_elimination_bfg;
extern	vmCvar_t	g_elimination_grapple;
extern	vmCvar_t	g_elimination_roundtime;
extern	vmCvar_t	g_elimination_warmup;
extern	vmCvar_t	g_elimination_activewarmup;
extern  vmCvar_t        g_elimination_allgametypes;
extern  vmCvar_t        g_elimination_spawn_allgametypes;
extern	vmCvar_t	g_elimination_machinegun;
extern	vmCvar_t	g_elimination_shotgun;
extern	vmCvar_t	g_elimination_grenade;
extern	vmCvar_t	g_elimination_rocket;
extern	vmCvar_t	g_elimination_railgun;
extern	vmCvar_t	g_elimination_lightning;
extern	vmCvar_t	g_elimination_plasmagun;
extern	vmCvar_t	g_elimination_chain;
extern	vmCvar_t	g_elimination_mine;
extern	vmCvar_t	g_elimination_nail;

//If lockspectator: 0=no limit, 1 = cannot follow enemy, 2 = must follow friend
extern  vmCvar_t        g_elimination_lockspectator;

extern vmCvar_t		g_grapple;
extern vmCvar_t		g_swingGrapple;

extern vmCvar_t		g_rockets;

//new in elimination Beta2
extern vmCvar_t		g_instantgib;
extern vmCvar_t		g_vampire;
extern vmCvar_t		g_vampireMaxHealth;
extern vmCvar_t		g_midAir;
//new in elimination Beta3
extern vmCvar_t		g_regen;
//Free for all gametype
extern int		g_ffa_gt; //0 = TEAM GAME, 1 = FFA, 2 = TEAM GAME without bases

extern vmCvar_t		g_lms_lives;

extern vmCvar_t		g_lms_mode; //How do we score: 0 = One Survivor get a point, 1 = same but without overtime, 2 = one point for each player killed (+overtime), 3 = same without overtime

extern vmCvar_t		g_elimination_ctf_oneway;	//Only attack in one direction (level.eliminationSides+level.roundNumber)%2 == 0 red attacks

extern vmCvar_t         g_awardpushing; //The server can decide if players are awarded for pushing people in lave etc.

extern vmCvar_t         g_persistantpowerups;

extern vmCvar_t        g_catchup; //Favors the week players

extern vmCvar_t         g_autonextmap; //Autochange map
extern vmCvar_t         g_mappools; //mappools to be used for autochange

extern vmCvar_t        g_voteNames;
extern vmCvar_t        g_voteBan;
extern vmCvar_t        g_voteGametypes;
extern vmCvar_t        g_voteMinTimelimit;
extern vmCvar_t        g_voteMaxTimelimit;
extern vmCvar_t        g_voteMinFraglimit;
extern vmCvar_t        g_voteMaxFraglimit;
extern vmCvar_t        g_voteMinCapturelimit;
extern vmCvar_t        g_voteMaxCapturelimit;
extern vmCvar_t        g_voteMinBots;
extern vmCvar_t        g_voteMaxBots;
extern vmCvar_t        g_maxvotes;
extern vmCvar_t        g_voteRepeatLimit;

extern vmCvar_t        g_humanplayers;

//used for voIP
extern vmCvar_t         g_redTeamClientNumbers;
extern vmCvar_t         g_blueTeamClientNumbers;

//unlagged - server options
// some new server-side variables
extern	vmCvar_t	g_delagHitscan;
extern	vmCvar_t	g_delagAllowHitsAfterTele;
extern	vmCvar_t	g_truePing;
// this is for convenience - using "sv_fps.integer" is nice :)
extern	vmCvar_t	sv_fps;
extern  vmCvar_t        g_lagLightning;
//unlagged - server options
extern  vmCvar_t        g_teleMissiles;
extern  vmCvar_t        g_teleMissilesMaxTeleports;
extern  vmCvar_t        g_pushGrenades;
extern  vmCvar_t        g_newShotgun;
extern  vmCvar_t        g_ratPhysics;
extern  vmCvar_t        g_rampJump;
extern  vmCvar_t        g_additiveJump;
extern  vmCvar_t        g_fastSwim;
extern  vmCvar_t        g_lavaDamage;
extern  vmCvar_t        g_slimeDamage;
extern  vmCvar_t        g_allowTimenudge;
extern  vmCvar_t        g_fastSwitch;
extern  vmCvar_t        g_fastWeapons;
extern  vmCvar_t        g_regularFootsteps;
extern  vmCvar_t        g_passThroughInvisWalls;
extern  vmCvar_t        g_ambientSound;
extern  vmCvar_t        g_rocketSpeed;
extern  vmCvar_t        g_maxExtrapolatedFrames;
extern  vmCvar_t        g_delagMissileMaxLatency;
extern  vmCvar_t        g_delagMissileDebug;
extern  vmCvar_t        g_delagMissiles;
extern  vmCvar_t        g_delagMissileLimitVariance;
extern  vmCvar_t        g_delagMissileLimitVarianceMs;
extern  vmCvar_t        g_delagMissileBaseNudge;
extern  vmCvar_t        g_delagMissileNudgeOnly;
extern  vmCvar_t        g_delagMissileLatencyMode;
extern  vmCvar_t        g_delagMissileCorrectFrameOffset;
extern  vmCvar_t        g_delagMissileImmediateRun;

extern  vmCvar_t        g_teleporterPrediction;

//extern  vmCvar_t	g_tournamentMinSpawnDistance;
extern  vmCvar_t	g_tournamentSpawnsystem;
extern  vmCvar_t	g_ffaSpawnsystem;

extern  vmCvar_t	g_ra3compat;
extern  vmCvar_t	g_ra3maxArena;
extern  vmCvar_t	g_ra3forceArena;
extern  vmCvar_t	g_ra3nextForceArena;

extern  vmCvar_t	g_enableGreenArmor;

extern  vmCvar_t	g_readSpawnVarFiles;

extern vmCvar_t		g_damageThroughWalls;

extern  vmCvar_t	g_pingEqualizer;
extern  vmCvar_t	g_eqpingMax;
extern  vmCvar_t	g_eqpingAuto;
extern  vmCvar_t	g_eqpingSavedPing;
extern  vmCvar_t	g_eqpingAutoTourney;

extern  vmCvar_t        g_autoClans;

extern  vmCvar_t        g_quadWhore;

extern  vmCvar_t        g_killDropsFlag;

extern  vmCvar_t        g_killSafety;
extern  vmCvar_t        g_killDisable;

extern  vmCvar_t        g_startWhenReady;
extern  vmCvar_t        g_autoStartTime;
extern  vmCvar_t        g_countDownHealthArmor;
extern  vmCvar_t        g_powerupGlows;
extern  vmCvar_t        g_screenShake;
extern  vmCvar_t        g_bobup;
extern  vmCvar_t        g_allowForcedModels;
extern  vmCvar_t        g_brightModels;
extern  vmCvar_t        g_brightPlayerShells;
extern  vmCvar_t        g_brightPlayerOutline;
extern  vmCvar_t        g_friendsWallHack;
extern  vmCvar_t        g_friendsFlagIndicator;
extern  vmCvar_t        g_specShowZoom;

extern  vmCvar_t        g_itemPickup;
extern  vmCvar_t        g_itemDrop;
extern  vmCvar_t        g_usesRatVM;
extern  vmCvar_t        g_usesRatEngine;
extern  vmCvar_t        g_mixedMode;
extern  vmCvar_t        g_broadcastClients;
extern  vmCvar_t        g_useExtendedScores;
extern  vmCvar_t        g_statsboard;
extern  vmCvar_t        g_predictMissiles;
extern  vmCvar_t        g_ratFlags;
extern  vmCvar_t        g_allowDuplicateGuid;

extern  vmCvar_t        g_botshandicapped;
extern  vmCvar_t        g_bots_randomcolors;

extern  vmCvar_t        g_pingLocationAllowed;
extern  vmCvar_t        g_pingLocationRadius;

extern  vmCvar_t        g_tauntAllowed;
extern  vmCvar_t        g_tauntTime;
extern  vmCvar_t        g_tauntAfterDeathTime;

// weapon config
extern vmCvar_t        g_weaponChangeTime_Dropping;
extern vmCvar_t        g_weaponChangeTime_Raising;
extern vmCvar_t        g_weaponReloadTime_Shotgun;
extern vmCvar_t        g_weaponReloadTime_Lg;
extern vmCvar_t        g_weaponReloadTime_Railgun;
extern vmCvar_t        g_mgDamage;
extern vmCvar_t        g_mgTeamDamage;
extern vmCvar_t        g_railgunDamage;
extern vmCvar_t        g_lgDamage;

extern vmCvar_t        g_railJump;

extern vmCvar_t	       g_teamslocked;
extern vmCvar_t	       g_autoTeamsUnlock;
extern vmCvar_t	       g_autoTeamsLock;
extern vmCvar_t	       g_tourneylocked;
extern vmCvar_t	       g_specMuted;
extern vmCvar_t	       g_tournamentMuteSpec;

extern vmCvar_t	       g_timeoutAllowed;
extern vmCvar_t	       g_timeinAllowed;
extern vmCvar_t	       g_timeoutTime;
extern vmCvar_t	       g_timeoutOvertimeStep;

extern vmCvar_t        g_autoFollowKiller;
extern vmCvar_t        g_autoFollowSwitchTime;

extern vmCvar_t        g_shaderremap;
extern vmCvar_t        g_shaderremap_flag;
extern vmCvar_t        g_shaderremap_flagreset;
extern vmCvar_t        g_shaderremap_banner;
extern vmCvar_t        g_shaderremap_bannerreset;

//KK-OAX Killing Sprees
extern  vmCvar_t    g_sprees; //Used for specifiying the config file
extern  vmCvar_t    g_altExcellent; //Turns on Multikills instead of Excellent
extern  vmCvar_t    g_spreeDiv; // Interval of a "streak" that form the spree triggers
//KK-OAX Command/Chat Flooding/Spamming
extern  vmCvar_t    g_floodMaxDemerits;
extern  vmCvar_t    g_floodMinTime;
extern  vmCvar_t    g_floodLimitUserinfo;
//KK-OAX Admin
extern  vmCvar_t    g_admin;
extern  vmCvar_t    g_adminLog;
extern  vmCvar_t    g_adminParseSay;
extern  vmCvar_t    g_adminNameProtect;
extern  vmCvar_t    g_adminTempBan;
extern  vmCvar_t    g_adminMaxBan;
//KK-OAX Admin-Like
extern  vmCvar_t    g_specChat;
extern  vmCvar_t    g_publicAdminMessages;

extern  vmCvar_t    g_maxWarnings;
extern  vmCvar_t    g_warningExpire;

extern  vmCvar_t    g_minNameChangePeriod;
extern  vmCvar_t    g_maxNameChanges;

extern  vmCvar_t    g_allowDuplicateNames;

extern  vmCvar_t    g_unnamedPlayersAllowed;
extern  vmCvar_t    g_unnamedRenameAdjlist;
extern  vmCvar_t    g_unnamedRenameNounlist;


void	trap_Printf( const char *fmt );
void	trap_Error( const char *fmt ) __attribute__((noreturn));
int		trap_Milliseconds( void );
int     trap_RealTime( qtime_t *qtime );
int		trap_Argc( void );
void	trap_Argv( int n, char *buffer, int bufferLength );
void	trap_Args( char *buffer, int bufferLength );
int		trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void	trap_FS_Read( void *buffer, int len, fileHandle_t f );
void	trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void	trap_FS_FCloseFile( fileHandle_t f );
int		trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
int		trap_FS_Seek( fileHandle_t f, long offset, int origin ); // fsOrigin_t
void	trap_SendConsoleCommand( int exec_when, const char *text );
void	trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags );
void	trap_Cvar_Update( vmCvar_t *cvar );
void	trap_Cvar_Set( const char *var_name, const char *value );
int		trap_Cvar_VariableIntegerValue( const char *var_name );
float	trap_Cvar_VariableValue( const char *var_name );
void	trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void	trap_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *gameClients, int sizeofGameClient );
void	trap_DropClient( int clientNum, const char *reason );
void	trap_SendServerCommand( int clientNum, const char *text );
void	trap_SetConfigstring( int num, const char *string );
void	trap_GetConfigstring( int num, char *buffer, int bufferSize );
void	trap_GetUserinfo( int num, char *buffer, int bufferSize );
void	trap_SetUserinfo( int num, const char *buffer );
void	trap_GetServerinfo( char *buffer, int bufferSize );
void	trap_SetBrushModel( gentity_t *ent, const char *name );
void	trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
int		trap_PointContents( const vec3_t point, int passEntityNum );
qboolean trap_InPVS( const vec3_t p1, const vec3_t p2 );
qboolean trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 );
void	trap_AdjustAreaPortalState( gentity_t *ent, qboolean open );
qboolean trap_AreasConnected( int area1, int area2 );
void	trap_LinkEntity( gentity_t *ent );
void	trap_UnlinkEntity( gentity_t *ent );
int		trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount );
qboolean trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
int		trap_BotAllocateClient( void );
void	trap_BotFreeClient( int clientNum );
void	trap_GetUsercmd( int clientNum, usercmd_t *cmd );
qboolean	trap_GetEntityToken( char *buffer, int bufferSize );

int		trap_DebugPolygonCreate(int color, int numPoints, vec3_t *points);
void	trap_DebugPolygonDelete(int id);

int		trap_BotLibSetup( void );
int		trap_BotLibShutdown( void );
int		trap_BotLibVarSet(char *var_name, char *value);
int		trap_BotLibVarGet(char *var_name, char *value, int size);
int		trap_BotLibDefine(char *string);
int		trap_BotLibStartFrame(float time);
int		trap_BotLibLoadMap(const char *mapname);
int		trap_BotLibUpdateEntity(int ent, void /* struct bot_updateentity_s */ *bue);
int		trap_BotLibTest(int parm0, char *parm1, vec3_t parm2, vec3_t parm3);

int		trap_BotGetSnapshotEntity( int clientNum, int sequence );
int		trap_BotGetServerCommand(int clientNum, char *message, int size);
void	trap_BotUserCommand(int client, usercmd_t *ucmd);

int		trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas);
int		trap_AAS_AreaInfo( int areanum, void /* struct aas_areainfo_s */ *info );
void	trap_AAS_EntityInfo(int entnum, void /* struct aas_entityinfo_s */ *info);

int		trap_AAS_Initialized(void);
void	trap_AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs);
float	trap_AAS_Time(void);

int		trap_AAS_PointAreaNum(vec3_t point);
int		trap_AAS_PointReachabilityAreaIndex(vec3_t point);
int		trap_AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);

int		trap_AAS_PointContents(vec3_t point);
int		trap_AAS_NextBSPEntity(int ent);
int		trap_AAS_ValueForBSPEpairKey(int ent, char *key, char *value, int size);
int		trap_AAS_VectorForBSPEpairKey(int ent, char *key, vec3_t v);
int		trap_AAS_FloatForBSPEpairKey(int ent, char *key, float *value);
int		trap_AAS_IntForBSPEpairKey(int ent, char *key, int *value);

int		trap_AAS_AreaReachability(int areanum);

int		trap_AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags);
int		trap_AAS_EnableRoutingArea( int areanum, int enable );
int		trap_AAS_PredictRoute(void /*struct aas_predictroute_s*/ *route, int areanum, vec3_t origin,
							int goalareanum, int travelflags, int maxareas, int maxtime,
							int stopevent, int stopcontents, int stoptfl, int stopareanum);

int		trap_AAS_AlternativeRouteGoals(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
										void /*struct aas_altroutegoal_s*/ *altroutegoals, int maxaltroutegoals,
										int type);
int		trap_AAS_Swimming(vec3_t origin);
int		trap_AAS_PredictClientMovement(void /* aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize);


void	trap_EA_Say(int client, char *str);
void	trap_EA_SayTeam(int client, char *str);
void	trap_EA_Command(int client, char *command);

void	trap_EA_Action(int client, int action);
void	trap_EA_Gesture(int client);
void	trap_EA_Talk(int client);
void	trap_EA_Attack(int client);
void	trap_EA_Use(int client);
void	trap_EA_Respawn(int client);
void	trap_EA_Crouch(int client);
void	trap_EA_MoveUp(int client);
void	trap_EA_MoveDown(int client);
void	trap_EA_MoveForward(int client);
void	trap_EA_MoveBack(int client);
void	trap_EA_MoveLeft(int client);
void	trap_EA_MoveRight(int client);
void	trap_EA_SelectWeapon(int client, int weapon);
void	trap_EA_Jump(int client);
void	trap_EA_DelayedJump(int client);
void	trap_EA_Move(int client, vec3_t dir, float speed);
void	trap_EA_View(int client, vec3_t viewangles);

void	trap_EA_EndRegular(int client, float thinktime);
void	trap_EA_GetInput(int client, float thinktime, void /* struct bot_input_s */ *input);
void	trap_EA_ResetInput(int client);


int		trap_BotLoadCharacter(char *charfile, float skill);
void	trap_BotFreeCharacter(int character);
float	trap_Characteristic_Float(int character, int index);
float	trap_Characteristic_BFloat(int character, int index, float min, float max);
int		trap_Characteristic_Integer(int character, int index);
int		trap_Characteristic_BInteger(int character, int index, int min, int max);
void	trap_Characteristic_String(int character, int index, char *buf, int size);

int		trap_BotAllocChatState(void);
void	trap_BotFreeChatState(int handle);
void	trap_BotQueueConsoleMessage(int chatstate, int type, char *message);
void	trap_BotRemoveConsoleMessage(int chatstate, int handle);
int		trap_BotNextConsoleMessage(int chatstate, void /* struct bot_consolemessage_s */ *cm);
int		trap_BotNumConsoleMessages(int chatstate);
void	trap_BotInitialChat(int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
int		trap_BotNumInitialChats(int chatstate, char *type);
int		trap_BotReplyChat(int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
int		trap_BotChatLength(int chatstate);
void	trap_BotEnterChat(int chatstate, int client, int sendto);
void	trap_BotGetChatMessage(int chatstate, char *buf, int size);
int		trap_StringContains(char *str1, char *str2, int casesensitive);
int		trap_BotFindMatch(char *str, void /* struct bot_match_s */ *match, unsigned long int context);
void	trap_BotMatchVariable(void /* struct bot_match_s */ *match, int variable, char *buf, int size);
void	trap_UnifyWhiteSpaces(char *string);
void	trap_BotReplaceSynonyms(char *string, unsigned long int context);
int		trap_BotLoadChatFile(int chatstate, char *chatfile, char *chatname);
void	trap_BotSetChatGender(int chatstate, int gender);
void	trap_BotSetChatName(int chatstate, char *name, int client);
void	trap_BotResetGoalState(int goalstate);
void	trap_BotRemoveFromAvoidGoals(int goalstate, int number);
void	trap_BotResetAvoidGoals(int goalstate);
void	trap_BotPushGoal(int goalstate, void /* struct bot_goal_s */ *goal);
void	trap_BotPopGoal(int goalstate);
void	trap_BotEmptyGoalStack(int goalstate);
void	trap_BotDumpAvoidGoals(int goalstate);
void	trap_BotDumpGoalStack(int goalstate);
void	trap_BotGoalName(int number, char *name, int size);
int		trap_BotGetTopGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int		trap_BotGetSecondGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int		trap_BotChooseLTGItem(int goalstate, vec3_t origin, int *inventory, int travelflags);
int		trap_BotChooseNBGItem(int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime);
int		trap_BotTouchingGoal(vec3_t origin, void /* struct bot_goal_s */ *goal);
int		trap_BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal);
int		trap_BotGetNextCampSpotGoal(int num, void /* struct bot_goal_s */ *goal);
int		trap_BotGetMapLocationGoal(char *name, void /* struct bot_goal_s */ *goal);
int		trap_BotGetLevelItemGoal(int index, char *classname, void /* struct bot_goal_s */ *goal);
float	trap_BotAvoidGoalTime(int goalstate, int number);
void	trap_BotSetAvoidGoalTime(int goalstate, int number, float avoidtime);
void	trap_BotInitLevelItems(void);
void	trap_BotUpdateEntityItems(void);
int		trap_BotLoadItemWeights(int goalstate, char *filename);
void	trap_BotFreeItemWeights(int goalstate);
void	trap_BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child);
void	trap_BotSaveGoalFuzzyLogic(int goalstate, char *filename);
void	trap_BotMutateGoalFuzzyLogic(int goalstate, float range);
int		trap_BotAllocGoalState(int state);
void	trap_BotFreeGoalState(int handle);

void	trap_BotResetMoveState(int movestate);
void	trap_BotMoveToGoal(void /* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags);
int		trap_BotMoveInDirection(int movestate, vec3_t dir, float speed, int type);
void	trap_BotResetAvoidReach(int movestate);
void	trap_BotResetLastAvoidReach(int movestate);
int		trap_BotReachabilityArea(vec3_t origin, int testground);
int		trap_BotMovementViewTarget(int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target);
int		trap_BotPredictVisiblePosition(vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags, vec3_t target);
int		trap_BotAllocMoveState(void);
void	trap_BotFreeMoveState(int handle);
void	trap_BotInitMoveState(int handle, void /* struct bot_initmove_s */ *initmove);
void	trap_BotAddAvoidSpot(int movestate, vec3_t origin, float radius, int type);

int		trap_BotChooseBestFightWeapon(int weaponstate, int *inventory);
void	trap_BotGetWeaponInfo(int weaponstate, int weapon, void /* struct weaponinfo_s */ *weaponinfo);
int		trap_BotLoadWeaponWeights(int weaponstate, char *filename);
int		trap_BotAllocWeaponState(void);
void	trap_BotFreeWeaponState(int weaponstate);
void	trap_BotResetWeaponState(int weaponstate);

int		trap_GeneticParentsAndChildSelection(int numranks, float *ranks, int *parent1, int *parent2, int *child);

void	trap_SnapVector( float *v );
void	trap_RAT_EQPing_Reset( void );
void	trap_RAT_EQPing_SetDelay( int clientnum, int msec );
int	trap_RAT_EQPing_GetDelay( int clientnum );

//KK-OAX
//These enable the simplified command handling. 

#define CMD_CHEAT           0x0001
#define CMD_CHEAT_TEAM      0x0002 // is a cheat when used on a team
#define CMD_MESSAGE         0x0004 // sends message to others (skip when muted)
#define CMD_TEAM            0x0008 // must be on a team
#define CMD_NOTEAM          0x0010 // must not be on a team
#define CMD_RED             0x0020 // must be on the red team (useless right now)
#define CMD_BLUE            0x0040 // must be on the blue team (useless right now)
#define CMD_LIVING          0x0080
#define CMD_INTERMISSION    0x0100 // valid during intermission
#define CMD_FLOODLIMITED    0x0200 // flood limit this command


typedef struct
{
    char    *cmdName;
    int     cmdFlags;
    void    ( *cmdHandler )( gentity_t *ent );
} commands_t;

//
// g_svcmds_ext.c
// These were added to a seperate file to keep g_svcmds.c navigable. 
void Svcmd_Status_f( void );
void Svcmd_TeamMessage_f( void );
void Svcmd_CenterPrint_f( void );
void Svcmd_BannerPrint_f( void );
void Svcmd_EjectClient_f( void );
void Svcmd_DumpUser_f( void );
void Svcmd_Chat_f( void );
void Svcmd_Ruleset_f( void );
void Svcmd_ListIP_f( void );
void Svcmd_MessageWrapper( void );

#include "g_killspree.h"
#include "g_admin.h"
