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
// g_combat.c

#include "g_local.h"
#include "challenges.h"

/*
============
ScorePlum
============
*/
void ScorePlum( gentity_t *ent, vec3_t origin, int score ) {
	gentity_t *plum;

	plum = G_TempEntity( origin, EV_SCOREPLUM );
	// only send this temp entity to a single client
	plum->r.svFlags |= SVF_SINGLECLIENT;
	plum->r.singleClient = ent->s.number;
	//
	plum->s.otherEntityNum = ent->s.number;
	plum->s.time = score;
}

void DamagePlum( gentity_t *ent, gentity_t *target, int mod, int damage ) {
	gentity_t *plum;
	vec3_t origin;

	if (!ent->client || !ent->client->pers.damagePlums) {
		return;
	}

	if (ent->client->pers.damagePlums == 2) {
		switch (mod) {
			case MOD_SHOTGUN:
			case MOD_GAUNTLET:
			case MOD_GRENADE:
			case MOD_GRENADE_SPLASH:
			case MOD_ROCKET:
			case MOD_ROCKET_SPLASH:
			case MOD_PLASMA:
			case MOD_PLASMA_SPLASH:
			case MOD_RAILGUN:
			case MOD_BFG:
			case MOD_BFG_SPLASH:
			case MOD_NAIL:
			case MOD_PROXIMITY_MINE:
			case MOD_KAMIKAZE:
				break;
			default:
				return;
		}
	}

	VectorCopy(target->r.currentOrigin, origin);
	origin[2] += 2 * target->r.maxs[2];

	plum = G_TempEntity( origin, EV_DAMAGEPLUM );
	// only send this temp entity to a single client
	plum->r.svFlags |= SVF_SINGLECLIENT;
	plum->r.singleClient = ent->s.number;
	//
	plum->s.otherEntityNum = ent->s.number;
	plum->s.time = damage;
}

void PushNotify(gentity_t *targ, gentity_t *pusher, int mod, int knockback) {
	gentity_t *ent;
	gclient_t *cl;

	if (!g_pushNotifications.integer ||
			targ == pusher || !OnSameTeam(targ, pusher) || !targ->client) {
		return;
	}

	cl = targ->client;
	if (cl->teamKnockbackTime == level.time && cl->teamKnockbackClientNum == pusher->s.number) {
		cl->teamKnockback += knockback;
	} else if (cl->teamKnockbackClientNum == -1 && cl->teamKnockbackTime + 250 > level.time) {
		// prevent flooding with notifications
		return;
	} else {
		cl->teamKnockbackTime = level.time;
		cl->teamKnockbackClientNum = pusher->s.number;
		cl->teamKnockback = knockback;
	}

	if (cl->teamKnockback < g_pushNotificationsKnockback.integer) {
		return;
	}

	// prevent another notification for a while
	cl->teamKnockbackClientNum = -1;

	ent = G_TempEntity( targ->r.currentOrigin, EV_PUSHNOTIFY );
	ent->s.eventParm = mod;
	ent->s.otherEntityNum = targ->s.number;
	ent->s.otherEntityNum2 = pusher->s.number;
	ent->r.svFlags |= SVF_SINGLECLIENT;
	ent->r.singleClient = targ->s.number;
}

/*
============
AddScore

Adds score to both the client and his team
============
*/
void AddScore( gentity_t *ent, vec3_t origin, int score ) {
        int i;

	if ( !ent->client ) {
		return;
	}
	// no scoring during pre-match warmup
	if ( level.warmupTime ) {
		return;
	}
	// no scoring in elimination warmup
	if (G_IsElimGT() && level.roundNumber != level.roundNumberStarted) {
		return;
	}

        //No scoring during intermission
        if ( level.intermissiontime
#ifdef WITH_MULTITOURNAMENT
			|| G_MtrnIntermissionTime(level.currentGameId)
#endif
			) {
            return;
        }
	// show score plum
        if( level.numNonSpectatorClients<3 && score < 0 && !G_IsTeamGametype()
#ifdef WITH_MULTITOURNAMENT
			&& g_gametype.integer != GT_MULTITOURNAMENT
#endif
			) {
            for ( i = 0 ; i < level.maxclients ; i++ ) {
                if ( level.clients[ i ].pers.connected != CON_CONNECTED )
                    continue; //Client was not connected

                if (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR)
                    continue; //Don't give anything to spectators

                if (g_entities+i == ent)
                    continue; //Don't award dead one

                level.clients[i].ps.persistant[PERS_SCORE] -= score;
                level.clients[i].sess.skillScore -= score;
                ScorePlum(ent, origin, -score);
            }
        }
        else {
            ScorePlum(ent, origin, score);
            //
            ent->client->ps.persistant[PERS_SCORE] += score;
            ent->client->sess.skillScore += score;
            if ( g_gametype.integer == GT_TEAM ) {
                int team = ent->client->ps.persistant[PERS_TEAM];
		AddTeamScore(ent->s.pos.trBase, team, score);
		//level.teamScores[ team ] += score;
		G_LogPrintf("TeamScore: %i %i: Team %d now has %d points\n",
				team, level.teamScores[ team ], team, level.teamScores[ team ] );
            }
        }
        G_LogPrintf("PlayerScore: %i %i: %s now has %d points\n",
		ent->s.number, ent->client->ps.persistant[PERS_SCORE], ent->client->pers.netname, ent->client->ps.persistant[PERS_SCORE] );
	CalculateRanks();
}

/*
=================
TossClientItems

Toss the weapon and powerups for the killed player
=================
*/
void TossClientItems( gentity_t *self ) {
	gitem_t		*item;
	int			weapon;
	float		angle;
	int			i;
	gentity_t	*drop;

	// drop the weapon if not a gauntlet or machinegun
	weapon = self->s.weapon;

	//Never drop in elimination or last man standing mode!
	if( g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_LMS)
		return;

	// make a special check to see if they are changing to a new
	// weapon that isn't the mg or gauntlet.  Without this, a client
	// can pick up a weapon, be killed, and not drop the weapon because
	// their weapon change hasn't completed yet and they are still holding the MG.
	if ( weapon == WP_MACHINEGUN || weapon == WP_GRAPPLING_HOOK ) {
		if ( self->client->ps.weaponstate == WEAPON_DROPPING ) {
			weapon = self->client->pers.cmd.weapon;
		}
		if ( !( self->client->ps.stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
			weapon = WP_NONE;
		}
	}

	if (g_instantgib.integer || g_rockets.integer || g_gametype.integer == GT_CTF_ELIMINATION || g_elimination_allgametypes.integer){
	//Nothing!	
	}
	else
	if ( weapon > WP_MACHINEGUN && weapon != WP_GRAPPLING_HOOK && 
		self->client->ps.ammo[ weapon ] ) {
		// find the item type for this weapon
		item = BG_FindItemForWeapon( weapon );

		// spawn the item
		Drop_Item( self, item, 0 );
	}

	// drop all the powerups if not in teamplay
	if ( g_gametype.integer != GT_TEAM ) {
		angle = 45;
		for ( i = 1 ; i < PW_NUM_POWERUPS ; i++ ) {
			if ( self->client->ps.powerups[ i ] > level.time ) {
				item = BG_FindItemForPowerup( i );
				if ( !item ) {
					continue;
				}
				drop = Drop_Item( self, item, angle );
				// decide how many seconds it has left
				drop->count = ( self->client->ps.powerups[ i ] - level.time ) / 1000;
				if ( drop->count < 1 ) {
					drop->count = 1;
				}
				angle += 45;
			}
		}
	}
}

void TossClientCoins( gentity_t *self ) {
	gitem_t		*item;
	gentity_t	*drop;
	vec3_t		velocity;
	vec3_t		angles;
	int i;
	int numCoins;
	float speedFactor;

	if (g_coins.integer <= 0) {
		return;
	}

	if (G_IsTeamGametype()) { 
		if (self->client->sess.sessionTeam == TEAM_RED) {
			item = BG_FindItem( "Red Coin" );
		} else {
			item = BG_FindItem( "Blue Coin" );
		}
	} else {
		item = BG_FindItem( "Gold Coin" );
	}


	numCoins = g_coinsDefault.integer
		+ g_coinsFrag.integer * self->client->pers.killstreak;
	speedFactor = 1.0 + (float)self->client->pers.killstreak/20;
	for (i = 0; i < numCoins; ++i) {
		angles[YAW] = (float)(crandom() * 360.0);
		//angles[PITCH] = (float)(crandom() * 30.0 + 45.0);
		angles[PITCH] = 60.0;
		angles[ROLL] = 0;

		AngleVectors( angles, velocity, NULL, NULL );
		//VectorScale( velocity, speedFactor * ((float)(crandom() * 200.0) + 100.0), velocity );
		VectorScale( velocity, speedFactor * ((float)(crandom() * 150.0) + 80.0), velocity );
		//velocity[2] += 200 + crandom() * 300;
		//velocity[2] += 400;
		velocity[2] += 300;

		drop =  LaunchItem( item, self->s.pos.trBase, velocity );
		drop->physicsBounce = 0.6;

	}


}


/*
=================
TossClientCubes
=================
*/
extern gentity_t	*neutralObelisk;

void TossClientCubes( gentity_t *self ) {
	gitem_t		*item;
	gentity_t	*drop;
	vec3_t		velocity;
	vec3_t		angles;
	vec3_t		origin;

	self->client->ps.generic1 = 0;

	// this should never happen but we should never
	// get the server to crash due to skull being spawned in
	if (!G_EntitiesFree()) {
		return;
	}

	if( self->client->sess.sessionTeam == TEAM_RED ) {
		item = BG_FindItem( "Red Cube" );
	}
	else {
		item = BG_FindItem( "Blue Cube" );
	}

	angles[YAW] = (float)(level.time % 360);
	angles[PITCH] = 0;	// always forward
	angles[ROLL] = 0;

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;

	if( neutralObelisk ) {
		VectorCopy( neutralObelisk->s.pos.trBase, origin );
		origin[2] += 44;
	} else {
		VectorClear( origin ) ;
	}

	drop = LaunchItem( item, origin, velocity );

	drop->nextthink = level.time + g_cubeTimeout.integer * 1000;
	drop->think = G_FreeEntity;
	drop->spawnflags = self->client->sess.sessionTeam;
}

/*
=================
TossClientPersistantPowerups
=================
*/
void TossClientPersistantPowerups( gentity_t *ent ) {
	gentity_t	*powerup;

	if( !ent->client ) {
		return;
	}

	if( !ent->client->persistantPowerup ) {
		return;
	}

	powerup = ent->client->persistantPowerup;

	powerup->r.svFlags &= ~SVF_NOCLIENT;
	powerup->s.eFlags &= ~EF_NODRAW;
	powerup->r.contents = CONTENTS_TRIGGER;
	trap_LinkEntity( powerup );

	ent->client->ps.stats[STAT_PERSISTANT_POWERUP] = 0;
	ent->client->persistantPowerup = NULL;
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vec3_t		dir;
	//vec3_t		angles;

	if ( attacker && attacker != self ) {
		VectorSubtract (attacker->s.pos.trBase, self->s.pos.trBase, dir);
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract (inflictor->s.pos.trBase, self->s.pos.trBase, dir);
	} else {
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw ( dir );

	/*angles[YAW] =*/ vectoyaw ( dir );
	//angles[PITCH] = 0; 
	//angles[ROLL] = 0;
}

/*
==================
GibEntity
==================
*/
void GibEntity( gentity_t *self, int killer ) {
	gentity_t *ent;
	int i;

	//if this entity still has kamikaze
	if (self->s.eFlags & EF_KAMIKAZE) {
		// check if there is a kamikaze timer around for this owner
		for (i = 0; i < MAX_GENTITIES; i++) {
			ent = &g_entities[i];
			if (!G_InUse(ent))
				continue;
			if (ent->activator != self)
				continue;
			if (strcmp(ent->classname, "kamikaze timer"))
				continue;
			G_FreeEntity(ent);
			break;
		}
	}
	G_AddEvent( self, EV_GIB_PLAYER, killer );
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
}

#define BUTCHER_GIBCOUNT 1

/*
==================
body_die
==================
*/
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	if ( self->health > GIB_HEALTH ) {
		return;
	}
	if ( !g_blood.integer ) {
		self->health = GIB_HEALTH+1;
		return;
	}

	if (meansOfDeath == MOD_GAUNTLET && attacker && attacker->client) {
		if (++attacker->client->pers.gauntCorpseGibCount >= BUTCHER_GIBCOUNT) {
			attacker->client->pers.gauntCorpseGibCount -= BUTCHER_GIBCOUNT;
			AwardMessage(attacker, EAWARD_BUTCHER, ++(attacker->client->pers.awardCounts[EAWARD_BUTCHER]));
		}
	}

	GibEntity( self, 0 );
}


// these are just for logging, the client prints its own messages
char	*modNames[] = {
	"MOD_UNKNOWN",
	"MOD_SHOTGUN",
	"MOD_GAUNTLET",
	"MOD_MACHINEGUN",
	"MOD_GRENADE",
	"MOD_GRENADE_SPLASH",
	"MOD_ROCKET",
	"MOD_ROCKET_SPLASH",
	"MOD_PLASMA",
	"MOD_PLASMA_SPLASH",
	"MOD_RAILGUN",
	"MOD_LIGHTNING",
	"MOD_BFG",
	"MOD_BFG_SPLASH",
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",
	"MOD_NAIL",
	"MOD_CHAINGUN",
	"MOD_PROXIMITY_MINE",
	"MOD_KAMIKAZE",
	"MOD_JUICED",
	"MOD_GRAPPLE"
};

/*
==================
Kamikaze_DeathActivate
==================
*/
void Kamikaze_DeathActivate( gentity_t *ent ) {
	G_StartKamikaze(ent);
	G_FreeEntity(ent);
}

/*
==================
Kamikaze_DeathTimer
==================
*/
void Kamikaze_DeathTimer( gentity_t *self ) {
	gentity_t *ent;

	ent = G_Spawn();
	ent->classname = "kamikaze timer";
	VectorCopy(self->s.pos.trBase, ent->s.pos.trBase);
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->think = Kamikaze_DeathActivate;
	ent->nextthink = level.time + 5 * 1000;

	ent->activator = self;
}


/*
==================
CheckAlmostCapture
==================
*/
void CheckAlmostCapture( gentity_t *self, gentity_t *attacker ) {
	gentity_t	*ent;
	vec3_t		dir;
	char		*classname;

	// if this player was carrying a flag
	if ( self->client->ps.powerups[PW_REDFLAG] ||
		self->client->ps.powerups[PW_BLUEFLAG] ||
		self->client->ps.powerups[PW_NEUTRALFLAG] ) {
		// get the goal flag this player should have been going for
		if ( g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION) {
			if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
				classname = "team_CTF_blueflag";
			}
			else {
				classname = "team_CTF_redflag";
			}
		}
		else {
			if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
				classname = "team_CTF_redflag";
			}
			else {
				classname = "team_CTF_blueflag";
			}
		}
		ent = NULL;
		do
		{
			ent = G_Find(ent, FOFS(classname), classname);
		} while (ent && (ent->flags & FL_DROPPED_ITEM));
		// if we found the destination flag and it's not picked up
		if (ent && !(ent->r.svFlags & SVF_NOCLIENT) ) {
			// if the player was *very* close
			VectorSubtract( self->client->ps.origin, ent->s.origin, dir );
			if ( VectorLength(dir) < 200 ) {
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				if ( attacker->client ) {
					attacker->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				}
			}
		}
	}
}

/*
==================
CheckAlmostScored
==================
*/
void CheckAlmostScored( gentity_t *self, gentity_t *attacker ) {
	gentity_t	*ent;
	vec3_t		dir;
	char		*classname;

	// if the player was carrying cubes
	if ( self->client->ps.generic1 ) {
		if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
			classname = "team_redobelisk";
		}
		else {
			classname = "team_blueobelisk";
		}
		ent = G_Find(NULL, FOFS(classname), classname);
		// if we found the destination obelisk
		if ( ent ) {
			// if the player was *very* close
			VectorSubtract( self->client->ps.origin, ent->s.origin, dir );
			if ( VectorLength(dir) < 200 ) {
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				if ( attacker->client ) {
					attacker->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				}
			}
		}
	}
}

void rampage_notify(gentity_t *attacker) {
	if (attacker->client->pers.multiKillCount != 3) {
		return;
	}

	attacker->client->pers.awardCounts[EAWARD_FRAGS]++;

	if (!g_usesRatVM.integer && !G_MixedClientHasRatVM(attacker->client)) {
		int soundIndex;
		gentity_t *te;
		soundIndex = G_SoundIndex( "sound/feedback/frags.wav" );
		//G_AddEvent( attacker, EV_GENERAL_SOUND, soundIndex );
		te = G_TempEntity( level.intermission_origin, EV_GLOBAL_SOUND );
		te->s.eventParm = soundIndex;
		te->r.svFlags |= SVF_BROADCAST;
		te->r.svFlags |= SVF_SINGLECLIENT;
		te->r.singleClient = attacker->s.number;

		return;
	}

	AwardMessage(attacker, EAWARD_FRAGS, attacker->client->pers.awardCounts[EAWARD_FRAGS]);
}

void G_CheckKamikazeAward(gentity_t *attacker, int killsBefore, int deathsBefore) {
	if (!attacker || !attacker->client) {
		return;
	}
	if (attacker->client->pers.kills <= killsBefore || attacker->client->pers.deaths <= deathsBefore) {
		return;
	}
	AwardMessage(attacker, EAWARD_KAMIKAZE, ++(attacker->client->pers.awardCounts[EAWARD_KAMIKAZE]));
}

void G_CheckStrongmanAward(gentity_t *attacker, gentity_t *victim) {
	int i;
	int numKilled = 0, numEnemies = 0;
	qboolean reset = qfalse;

	// make sure that if a player dies, he has to start the strongman challenge all over again
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t	*client;

		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if (client->pers.lastKilledByStrongMan == victim->s.number) {
			client->pers.lastKilledByStrongMan = -1;
		}
	}

	if (!attacker || !attacker->client || (attacker == victim)) {
		return;
	}

	victim->client->pers.lastKilledByStrongMan = attacker->s.number;

	for (;;) {
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			gclient_t	*client;

			client = &level.clients[i];
			if ( client->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
				continue;
			}

			if (!reset && client->pers.lastKilledByStrongMan == victim->s.number) {
				// make sure that the victim has to start over again 
				client->pers.lastKilledByStrongMan = -1;
			}

			if (G_IsTeamGametype()) { 
				if (client->sess.sessionTeam == attacker->client->sess.sessionTeam)   {
					continue;
				}
			} else if ( i == attacker->s.number ) {
				client->pers.lastKilledByStrongMan = -1;
				continue;
			}
			
			if (reset) {
				client->pers.lastKilledByStrongMan = -1;
				continue;
			}

			numEnemies++;

			if (client->pers.lastKilledByStrongMan == attacker->s.number) {
				numKilled++;
			} else {
				client->pers.lastKilledByStrongMan = -1;
			}

		}
		if (reset) {
			break;
		}
		if (numKilled == numEnemies && numEnemies >= 3) {
			AwardMessage(attacker, EAWARD_STRONGMAN, ++(attacker->client->pers.awardCounts[EAWARD_STRONGMAN]));
			reset = qtrue;
		}  else {
			break;
		}
	}
}

static int G_AmbushCalculateTimeshiftTime(int meansOfDeath, gentity_t *attacker, gentity_t *victim) {
	if (victim->client->timeshiftTime) {
		return victim->client->timeshiftTime;
	}

	if (!g_delagHitscan.integer) {
		return level.time;
	}

	// XXX: this is a bit of a hack. Some weapons only timeshift the client
	// during the hit trace and un-shift them before they actually apply
	// the damage, so timeshiftTime might not be set anymore
	// Nevertheless, rewriting tons of damage-related code in order to save
	// that time just for this one award seems excessive
	
	// check if it was a hitscan weapon:
	if (meansOfDeath == MOD_GAUNTLET 
			|| meansOfDeath == MOD_MACHINEGUN
			|| meansOfDeath == MOD_SHOTGUN
			|| meansOfDeath == MOD_LIGHTNING
			|| meansOfDeath == MOD_RAILGUN
			|| meansOfDeath == MOD_CHAINGUN) {
		int timeshiftTime;
		G_DoTimeShiftFor(attacker);
		timeshiftTime = victim->client->timeshiftTime;
		G_UndoTimeShiftFor( attacker );
		return timeshiftTime;
	}
	return level.time;
}

void G_CheckAmbushAward(gentity_t *victim, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath) {
	if (!attacker || !attacker->client || attacker == victim || OnSameTeam(attacker, victim)) {
		return;
	}
	// also makes sure the client wasn't moved back through the portal due to unlag
	if (((victim->client->lastTeleportTime 
					 && victim->client->lastTeleportTime + 800 >= level.time 
					 && attacker->client->lastTeleportTime + 1500 < level.time 
					 && G_AmbushCalculateTimeshiftTime(meansOfDeath, attacker, victim) > victim->client->lastTeleportTime
				)
				|| 
				(victim->client->respawnTime + 1800 >= level.time)
			)
		       	&& (!inflictor || !inflictor->missileTeleported)
			&& meansOfDeath != MOD_TELEFRAG) {
		AwardMessage(attacker, EAWARD_AMBUSH, ++(attacker->client->pers.awardCounts[EAWARD_AMBUSH]));
	}
}

void G_CheckVaporizedAward(gentity_t *attacker, gentity_t *victim) {
	gclient_t *client = attacker->client;
	int i = client->damage_history_head % DAMAGE_HISTORY;
	damageRecord_t *dr;
	int totalPlasmaDamage = 0;
	int awardDamage = 100;
	int awardTime = 800;

	// make it harder if the attacker has haste or quad
	if (attacker->client->ps.powerups[PW_HASTE]) {
		awardTime /= 1.3;
	}
	if (attacker->client->ps.powerups[PW_QUAD] && g_quadfactor.value > 1.0) {
		awardDamage *= g_quadfactor.value;
	}

	do {
		dr = &client->damage_history[i];
		if (dr->damage <= 0) {
			break;
		}

		if (victim != dr->target
				|| (dr->mod != MOD_PLASMA && dr->mod != MOD_PLASMA_SPLASH)
				|| (dr->died && i != (client->damage_history_head % DAMAGE_HISTORY)) // opponent died in between
				) {
			break;
		}
		totalPlasmaDamage += dr->damage;
		if (level.time - dr->time > awardTime) {
			break;
		}
		if (totalPlasmaDamage > awardDamage) {
			// did more than 100 plasma damage to the target within the last 800ms
			// (this was more effective than two perfect consecutive hits with RL)
			AwardMessage(attacker, EAWARD_VAPORIZED, ++(attacker->client->pers.awardCounts[EAWARD_VAPORIZED]));
			break;
		} 
		i = (i + DAMAGE_HISTORY - 1 ) % DAMAGE_HISTORY;
	} while (i != client->damage_history_head);
}

void G_CheckDeathEAwards(gentity_t *victim, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath) {
	if (!attacker || !attacker->client || attacker == victim || OnSameTeam(attacker, victim)) {
		return;
	}

	if (victim->s.powerups & (
			(1 << PW_QUAD) |
			(1 << PW_BATTLESUIT) |
			(1 << PW_HASTE) |
			(1 << PW_INVIS) |
			(1 << PW_REGEN) |
			(1 << PW_FLIGHT))) {
		AwardMessage(attacker, EAWARD_SHOWSTOPPER, ++(attacker->client->pers.awardCounts[EAWARD_SHOWSTOPPER]));
	}


	if (meansOfDeath == MOD_PLASMA || meansOfDeath == MOD_PLASMA_SPLASH) {
		G_CheckVaporizedAward(attacker, victim);
	}

	switch (meansOfDeath) {
		case MOD_TELEFRAG:
			if (!G_IsElimTeamGT() 
					|| level.roundNumber == level.roundNumberStarted) {
				AwardMessage(attacker, EAWARD_TELEFRAG, ++(attacker->client->pers.awardCounts[EAWARD_TELEFRAG]));
			}
			break;
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
		case MOD_PLASMA:
		case MOD_PLASMA_SPLASH:
		case MOD_NAIL:
		case MOD_BFG:
		case MOD_BFG_SPLASH:
			if (attacker->client->ps.pm_type == PM_DEAD 
					&& attacker->client->pers.lastKilledBy >= 0 && attacker->client->pers.lastKilledBy < MAX_CLIENTS
					&& attacker->client->pers.lastKilledBy != attacker->s.number
					&& attacker->client->pers.lastKilledBy == victim->s.number) {
				AwardMessage(attacker, EAWARD_DEADHAND, ++(attacker->client->pers.awardCounts[EAWARD_DEADHAND]));
			}
			if (!inflictor || !inflictor->missileTeleported) {
				break;
			}
			AwardMessage(attacker, EAWARD_TELEMISSILE_FRAG, ++(attacker->client->pers.awardCounts[EAWARD_TELEMISSILE_FRAG]));
			break;
	}

	if (attacker->client->pers.lastDeathTime > 0 
			&& attacker->client->pers.lastKilledBy == victim->s.number
			&& attacker->client->respawnTime > attacker->client->pers.lastDeathTime
			&& attacker->client->respawnTime < level.time
			&& attacker->client->respawnTime + 5 * 1000 > level.time
			// revenge kill has to be the first kill after respawn 
			&& attacker->client->lastkilled_client == -1
			&& !attacker->client->pers.gotRevenge
			) {
		attacker->client->pers.gotRevenge = qtrue;
		AwardMessage(attacker, EAWARD_REVENGE, ++(attacker->client->pers.awardCounts[EAWARD_REVENGE]));

	}

	if (attacker->client->ps.stats[STAT_HEALTH] <= 30) {
		if (++attacker->client->ratFragCounter >= 3) {
			attacker->client->ratFragCounter = 0;
			AwardMessage(attacker, EAWARD_RAT, ++(attacker->client->pers.awardCounts[EAWARD_RAT]));
		}
	} else {
		attacker->client->ratFragCounter = 0;
	}

}

void G_CheckOnePlayerLeft( gentity_t *justdied ) {
	int teamCount = 0;
	if (level.roundNumber != level.roundNumberStarted) {
		return;
	}
	teamCount = TeamLivingCount(-1, justdied->client->sess.sessionTeam);
	if (teamCount == 1) {
		gentity_t *te;
		te = G_TempEntity( justdied->s.pos.trBase, EV_GLOBAL_TEAM_SOUND);
		te->s.eventParm = justdied->client->sess.sessionTeam == TEAM_BLUE ? 
			GTS_ONE_PLAYER_LEFT_BLUE : GTS_ONE_PLAYER_LEFT_RED;
		te->r.svFlags |= SVF_BROADCAST;
	}
}

void G_SetRespawntime( gentity_t *self, int notBefore ) {
	self->client->respawnTime = notBefore;
	if(g_respawntime.integer > 0 && level.time + g_respawntime.integer * 1000 > self->client->respawnTime) {
		self->client->respawnTime = level.time + g_respawntime.integer * 1000;
	}

	//For testing:
	//G_Printf("Respawntime: %i\n",self->client->respawnTime);
	//However during warm up, we should respawn quicker!
	if(G_IsElimGT())
		if(level.time<=level.roundStartTime && level.time>level.roundStartTime-1000*g_elimination_activewarmup.integer) {
			//self->client->respawnTime = level.time + rand()%800;
			self->client->respawnTime = level.time + 400;
		}

	if (g_gametype.integer == GT_ELIMINATION && g_elimination_respawn.integer
			&& level.roundNumber == level.roundNumberStarted) {
		self->client->elimRespawnTime = level.time + g_elimination_respawn.integer * 1000;
		if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
			self->client->elimRespawnTime += level.elimBlueRespawnDelay;
			level.elimBlueRespawnDelay += g_elimination_respawn_increment.integer * 1000;
		} else {
			self->client->elimRespawnTime += level.elimRedRespawnDelay;
			level.elimRedRespawnDelay += g_elimination_respawn_increment.integer * 1000;
		}
		RespawnTimeMessage(self,self->client->elimRespawnTime);
	} else {
		RespawnTimeMessage(self,self->client->respawnTime);
	}
}

static void G_RecordKillForMod(gclient_t *attacker, int mod) {
	int weapon = G_WeaponForMOD(mod);
	attacker->pers.kills_per_weapon[weapon]++;
}


/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	gentity_t	*ent;
	int			anim;
	int			contents;
	int			killer;
	int			i,counter2;
	char		*killerName, *obit;

	if ( self->client->ps.pm_type == PM_DEAD ) {
		return;
	}

	if ( level.intermissiontime
#ifdef WITH_MULTITOURNAMENT
		       	|| G_MtrnIntermissionTime(level.currentGameId)
#endif
			) {
		return;
	}

	if (self->client) {
		if (G_IsElimTeamGT()) {
			if (level.roundNumber == level.roundNumberStarted) {
				self->client->pers.deaths += 1;
				self->client->pers.elimRoundDeaths += 1;
			}
		} else {
			self->client->pers.deaths += 1;
		}
	}

	// needs to be checked before we untimeshift the victim
    	G_CheckAmbushAward(self, inflictor, attacker, meansOfDeath);

//unlagged - backward reconciliation #2
	// make sure the body shows up in the client's current position
	G_UnTimeShiftClient( self );
//unlagged - backward reconciliation #2

	// has to run before streak logic
	TossClientCoins( self );

	//KK-OAX Here is where we run the streak logic. 
	G_RunStreakLogic( attacker, self );
	
	// check for an almost capture
	CheckAlmostCapture( self, attacker );
	// check for a player that almost brought in cubes
	CheckAlmostScored( self, attacker );

	if (self->client && self->client->hook) {
		Weapon_HookFree(self->client->hook);
	}
	if ((self->client->ps.eFlags & EF_TICKING) && self->activator) {
		self->client->ps.eFlags &= ~EF_TICKING;
		self->activator->think = G_FreeEntity;
		self->activator->nextthink = level.time;
	}

	self->client->ps.pm_type = PM_DEAD;

	if ( attacker ) {
		killer = attacker->s.number;
		if ( attacker->client ) {
			killerName = attacker->client->pers.netname;
			if (self != attacker) {
				if (G_IsElimGT()) {
					attacker->client->pers.elimRoundKills += 1;
					if (level.roundNumber == level.roundNumberStarted) {
						attacker->client->pers.kills += 1;
						G_RecordKillForMod(attacker->client, meansOfDeath);
					}
				} else {
					attacker->client->pers.kills += 1;
					G_RecordKillForMod(attacker->client, meansOfDeath);
				}
			}
		} else {
			killerName = "<non-client>";
		}
	} else {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( killer < 0 || killer >= MAX_CLIENTS ) {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( meansOfDeath < 0 || meansOfDeath >= sizeof( modNames ) / sizeof( modNames[0] ) ) {
		obit = "<bad obituary>";
	} else {
		obit = modNames[meansOfDeath];
	}

	G_LogPrintf("Kill: %i %i %i: %s killed %s by %s\n", 
		killer, self->s.number, meansOfDeath, killerName, 
		self->client->pers.netname, obit );

	// broadcast the death event to everyone
	ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
        //Sago: Hmmm... generic? Can I transmit anything I like? Like if it is a team kill? Let's try
        ent->s.generic1 = OnSameTeam (self, attacker);
        if( !(G_IsElimTeamGT() && level.time < level.roundStartTime) )
            ent->r.svFlags = SVF_BROADCAST;	// send to everyone (if not an elimination gametype during active warmup)
        else
            ent->r.svFlags = SVF_NOCLIENT;

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;

	self->client->pers.lastDeathTime = level.time;
	self->client->pers.lastKilledBy = killer;
	self->client->pers.gotRevenge = qfalse;
    
	G_CheckStrongmanAward(attacker, self);
    	G_CheckDeathEAwards(self, inflictor, attacker, meansOfDeath);

	if (attacker && attacker->client) {
		attacker->client->lastkilled_client = self->s.number;

		if ( attacker == self || OnSameTeam (self, attacker ) ) {
			if(g_gametype.integer != GT_TREASURE_HUNTER && g_gametype.integer!=GT_LMS && !(G_IsElimTeamGT() && level.time < level.roundStartTime))
                                AddScore( attacker, self->r.currentOrigin, -1 );
		} else {
			if( meansOfDeath == MOD_GAUNTLET ) {
				
				// Attack gets a challenge complete:
				if(!(attacker->r.svFlags & SVF_BOT) && !(self->r.svFlags & SVF_BOT))
					ChallengeMessage(attacker,WEAPON_GAUNTLET_KILLS);
		
				// play humiliation on player
				attacker->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
                                G_LogPrintf( "Award: %i %i: %s gained the %s award!\n", attacker->client->ps.clientNum, 0, attacker->client->pers.netname, "GAUNTLET" );

				// add the sprite over the player's head
				attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				attacker->client->ps.eFlags |= EF_AWARD_GAUNTLET;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

				// also play humiliation on target
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_GAUNTLETREWARD;

				if (attacker->client->ps.powerups[PW_INVIS]) {
					AwardMessage(attacker, EAWARD_AMBUSH, ++(attacker->client->pers.awardCounts[EAWARD_AMBUSH]));
				}
				if ((++(attacker->client->gauntSpree)) % 3 == 0 ) {
					AwardMessage(attacker, EAWARD_BERSERKER, ++(attacker->client->pers.awardCounts[EAWARD_BERSERKER]));
				}
			} else {
				attacker->client->gauntSpree = 0;
			}

			if(g_gametype.integer!=GT_LMS && g_gametype.integer != GT_TREASURE_HUNTER) {
				AddScore( attacker, self->r.currentOrigin, 1 );
			}


                        //If neither attacker or taget is bots and not the same
                        if(!(attacker->r.svFlags & SVF_BOT) && !(self->r.svFlags & SVF_BOT) && self!=attacker)
                        {
                            switch(meansOfDeath)
                            {
                                case MOD_GAUNTLET:
                                    ChallengeMessage(attacker,WEAPON_GAUNTLET_KILLS);
                                    break;
                                case MOD_MACHINEGUN:
                                    ChallengeMessage(attacker,WEAPON_MACHINEGUN_KILLS);
                                    break;
                                case MOD_SHOTGUN:
                                    ChallengeMessage(attacker,WEAPON_SHOTGUN_KILLS);
                                    break;
                                case MOD_GRENADE:
                                case MOD_GRENADE_SPLASH:
                                    ChallengeMessage(attacker,WEAPON_GRANADE_KILLS);
                                    break;
                                case MOD_ROCKET:
                                case MOD_ROCKET_SPLASH:
                                    ChallengeMessage(attacker,WEAPON_ROCKET_KILLS);
                                    break;
                                case MOD_LIGHTNING:
                                    ChallengeMessage(attacker,WEAPON_LIGHTNING_KILLS);
                                    break;
                                case MOD_PLASMA:
                                case MOD_PLASMA_SPLASH:
                                    ChallengeMessage(attacker,WEAPON_PLASMA_KILLS);
                                    break;
                                case MOD_RAILGUN:
                                    if(g_instantgib.integer)
                                        ChallengeMessage(attacker,WEAPON_INSTANT_RAIL_KILLS);
                                    else
                                        ChallengeMessage(attacker,WEAPON_RAIL_KILLS);
                                    break;
                                case MOD_BFG:
                                case MOD_BFG_SPLASH:
                                    ChallengeMessage(attacker,WEAPON_BFG_KILLS);
                                    break;
                                case MOD_NAIL:
                                    ChallengeMessage(attacker,WEAPON_NAILGUN_KILLS);
                                    break;
                                case MOD_CHAINGUN:
                                    ChallengeMessage(attacker,WEAPON_CHAINGUN_KILLS);
                                    break;
                                case MOD_PROXIMITY_MINE:
                                    ChallengeMessage(attacker,WEAPON_MINE_KILLS);
                                    break;
                                case MOD_GRAPPLE:
                                    ChallengeMessage(attacker,WEAPON_GRAPPLE_KILLS);
                                    break;
                                case MOD_LAVA:
                                case MOD_SLIME:
                                case MOD_TRIGGER_HURT:
                                case MOD_FALLING:
                                    ChallengeMessage(attacker,WEAPON_PUSH_KILLS);
                                    break;
                                case MOD_CRUSH:
                                    ChallengeMessage(attacker,WEAPON_CRUSH_KILLS);
                                    break;
                                case MOD_TELEFRAG:
                                    ChallengeMessage(attacker,WEAPON_TELEFRAG_KILLS);
                                    break;
                            };
                            ChallengeMessage(attacker,GENERAL_TOTALKILLS);
                            ChallengeMessage(self,GENERAL_TOTALDEATHS);

                            //Lets count number of powerups:
                            i = 0;
                            counter2 = 0;

                            if(attacker->client->ps.powerups[PW_QUAD]) {
                                ChallengeMessage(attacker,POWERUP_QUAD_KILL);
                                counter2++;
                            }
                            if(self->client->ps.powerups[PW_QUAD]) {
                                ChallengeMessage(attacker,POWERUP_COUNTER_QUAD);
                                i++;
                            }

                            if(attacker->client->ps.powerups[PW_HASTE]) {
                                ChallengeMessage(attacker,POWERUP_SPEED_KILL);
                                counter2++;
                            }
                            if(self->client->ps.powerups[PW_HASTE]) {
                                ChallengeMessage(attacker,POWERUP_COUNTER_SPEED);
                                i++;
                            }

                            if(attacker->client->ps.powerups[PW_INVIS]) {
                                ChallengeMessage(attacker,POWERUP_INVIS_KILL);
                                counter2++;
                            }
                            if(self->client->ps.powerups[PW_INVIS]) {
                                ChallengeMessage(attacker,POWERUP_COUNTER_INVIS);
                                i++;
                            }

                            if(attacker->client->ps.powerups[PW_FLIGHT]) {
                                ChallengeMessage(attacker,POWERUP_FLIGHT_KILL);
                                counter2++;
                            }
                            if(self->client->ps.powerups[PW_FLIGHT]) {
                                ChallengeMessage(attacker,POWERUP_COUNTER_FLIGHT);
                                i++;
                            }

                            if(self->client->ps.powerups[PW_BATTLESUIT]) {
                                ChallengeMessage(attacker,POWERUP_COUNTER_ENVIR);
                                i++;
                            }

                            if(self->client->ps.powerups[PW_REGEN]) {
                                ChallengeMessage(attacker,POWERUP_COUNTER_REGEN);
                                i++;
                            }

                            if(i>1) //The target had more than one powerup
                                ChallengeMessage(attacker,POWERUP_COUNTER_MULTI);
                            if(counter2>1) //The attacker has more than one powerup
                                ChallengeMessage(attacker,POWERUP_MULTI_KILL);
                        }

			// check for two kills in a short amount of time
			// if this is close enough to the last kill, give a reward sound
			if ( level.time - attacker->client->lastKillTime < CARNAGE_REWARD_TIME ) {
				// KK-OAX
				// Check if Multikills are enabled
				if( g_altExcellent.integer ) {
				    attacker->client->pers.multiKillCount++;
				    G_checkForMultiKill( attacker );
				} // play excellent on player
				attacker->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
                                G_LogPrintf( "Award: %i %i: %s gained the %s award!\n", attacker->client->ps.clientNum, 1, attacker->client->pers.netname, "EXCELLENT" );
                                if(!level.hadBots) //There has not been any bots
                                    ChallengeMessage(attacker,AWARD_EXCELLENT);
				// add the sprite over the player's head
				attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				attacker->client->ps.eFlags |= EF_AWARD_EXCELLENT;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
				attacker->client->pers.multiKillCount++;
				rampage_notify(attacker);
			} else { 
			    //KK-OAX Clear multikill count
			    //Must be 1 so the correct number of kills are displayed to the clients. 
			    attacker->client->pers.multiKillCount = 1;
			}
			attacker->client->lastKillTime = level.time;
		}
	} else {
		if(g_gametype.integer != GT_TREASURE_HUNTER && g_gametype.integer!=GT_LMS && !(G_IsElimTeamGT() && level.time < level.roundStartTime))
                    //if(self->client->ps.persistant[PERS_SCORE]>0 || level.numNonSpectatorClients<3) //Cannot get negative scores by suicide
			AddScore( self, self->r.currentOrigin, -1 );
	}

	// Add team bonuses
	Team_FragBonuses(self, inflictor, attacker);

	// if I committed suicide, the flag does not fall, it returns.
	if (!g_killDropsFlag.integer && meansOfDeath == MOD_SUICIDE) {
		if ( self->client->ps.powerups[PW_NEUTRALFLAG] ) {		// only happens in One Flag CTF
			Team_ReturnFlag( TEAM_FREE );
			self->client->ps.powerups[PW_NEUTRALFLAG] = 0;
		}
		else if ( self->client->ps.powerups[PW_REDFLAG] ) {		// only happens in standard CTF
			Team_ReturnFlag( TEAM_RED );
			self->client->ps.powerups[PW_REDFLAG] = 0;
		}
		else if ( self->client->ps.powerups[PW_BLUEFLAG] ) {	// only happens in standard CTF
			Team_ReturnFlag( TEAM_BLUE );
			self->client->ps.powerups[PW_BLUEFLAG] = 0;
		}
	}
        TossClientPersistantPowerups( self );
        if( g_gametype.integer == GT_HARVESTER ) {
                TossClientCubes( self );
        }
	// if client is in a nodrop area, don't drop anything (but return CTF flags!)
	TossClientItems( self );
//#endif

	Cmd_Score_f( self );		// show scores
	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t	*client;

		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		if ( client->sess.spectatorClient == self->s.number ) {
			Cmd_Score_f( g_entities + i );
		}
	}

	self->client->frozen = FROZEN_NOT;
	if (g_freeze.integer) {
		self->client->frozen = FROZEN_DIED;
		self->client->freezetag_thawedBy = -1;
		self->client->freezetag_thawed = 0.0;
		
		if (meansOfDeath != MOD_LAVA
				&& meansOfDeath != MOD_SLIME
				&& meansOfDeath != MOD_TRIGGER_HURT
				&& meansOfDeath != MOD_CRUSH
				&& meansOfDeath != MOD_UNKNOWN
				&& meansOfDeath != MOD_WATER
				&& meansOfDeath != MOD_TARGET_LASER
				&& meansOfDeath != MOD_TELEFRAG
				&& meansOfDeath != MOD_JUICED
		   ) {
			self->client->frozen = FROZEN_ONMAP;
			G_CreateFrozenPlayer(self);
			if (self->frozenPlayer) {
				G_AddEvent(self->frozenPlayer, EV_FREEZE, killer );
			}
			trap_UnlinkEntity(self);
		}
		if (self->health > GIB_HEALTH || self->client->ps.stats[STAT_HEALTH] > GIB_HEALTH) {
			// make sure no body is left behind
			self->health = GIB_HEALTH;
			self->client->ps.stats[STAT_HEALTH] = GIB_HEALTH;
		}
	}

	self->takedamage = qtrue;	// can still be gibbed

	self->s.weapon = WP_NONE;
	self->s.powerups = 0;
	self->r.contents = CONTENTS_CORPSE;

	if (self->client->frozen == FROZEN_ONMAP) {
		self->client->ps.stats[STAT_DEAD_YAW] = self->client->ps.viewangles[YAW];
		self->client->ps.viewangles[PITCH] = 0;
		self->client->ps.viewangles[ROLL] = 0;
		VectorCopy( self->client->ps.viewangles, self->s.angles );
	} else {
		self->s.angles[0] = 0;
		self->s.angles[2] = 0;
		LookAtKiller (self, inflictor, attacker);

		self->r.maxs[2] = -8;
		VectorCopy( self->s.angles, self->client->ps.viewangles );
	}


	self->s.loopSound = 0;


	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	G_SetRespawntime(self, level.time + 1700);
	
	// remove powerups
	memset( self->client->ps.powerups, 0, sizeof(self->client->ps.powerups) );

	if (self->client->frozen != FROZEN_ONMAP) {
		// never gib in a nodrop
		contents = trap_PointContents( self->r.currentOrigin, -1 );

		if ( (self->health <= GIB_HEALTH && !(contents & CONTENTS_NODROP) && g_blood.integer) || meansOfDeath == MOD_SUICIDE
				|| self->client->frozen) {
			// gib death
			GibEntity( self, killer );
		} else {
			// normal death
			static int i;

			switch ( i ) {
				case 0:
					anim = BOTH_DEATH1;
					break;
				case 1:
					anim = BOTH_DEATH2;
					break;
				case 2:
				default:
					anim = BOTH_DEATH3;
					break;
			}

			// for the no-blood option, we need to prevent the health
			// from going to gib level
			if ( self->health <= GIB_HEALTH ) {
				self->health = GIB_HEALTH+1;
			}

			self->client->ps.legsAnim = 
				( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
			self->client->ps.torsoAnim = 
				( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

			G_AddEvent( self, EV_DEATH1 + i, killer );

			// the body can still be gibbed
			self->die = body_die;

			// globally cycle through the different death animations
			i = ( i + 1 ) % 3;

			if (self->s.eFlags & EF_KAMIKAZE) {
				Kamikaze_DeathTimer( self );
			}
		}
		trap_LinkEntity (self);
	} else {
		// created a frozen player copy, so make the real player entity invisible
		self->takedamage = qfalse;
		self->s.eType = ET_INVISIBLE;
		self->r.contents = 0;
		//self->r.contents = CONTENTS_BODY;
		// prevent other code from showing this player again
		self->health = GIB_HEALTH;
		
		//self->takedamage = qtrue;
		//self->s.eFlags = EF_DEAD;
		//self->s.eType = ET_FROZENPLAYER;
		//self->r.contents = CONTENTS_BODY;
		//self->client->ps.eFlags = EF_DEAD;
		//self->health = 0;
		//if (self->client->ps.pm_flags & PMF_DUCKED) {
		//	self->client->ps.legsAnim = 
		//		( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | LEGS_IDLECR;
		//	self->client->ps.torsoAnim = 
		//		( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | TORSO_STAND;
		//} else {
		//	self->client->ps.legsAnim = 
		//		( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | LEGS_IDLE;
		//	self->client->ps.torsoAnim = 
		//		( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | TORSO_STAND;
		//}
	}


	if (G_IsElimTeamGT()) {
		G_SendTeamPlayerCounts();
	}

	if (g_gametype.integer == GT_ELIMINATION && g_elimination_respawn.integer > 0) {
		G_CheckOnePlayerLeft(self);
	}

}


/*
================
CheckArmor
================
*/
int CheckArmor (gentity_t *ent, int damage, int dflags)
{
	gclient_t	*client;
	int			save;
	int			count;

	if (!damage)
		return 0;

	client = ent->client;

	if (!client)
		return 0;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	// armor
	count = client->ps.stats[STAT_ARMOR];
	save = ceil( damage * ARMOR_PROTECTION );
	if (save >= count)
		save = count;

	if (!save)
		return 0;

	client->ps.stats[STAT_ARMOR] -= save;

	return save;
}

/*
================
RaySphereIntersections
================
*/
int RaySphereIntersections( vec3_t origin, float radius, vec3_t point, vec3_t dir, vec3_t intersections[2] ) {
	float b, c, d, t;

	//	| origin - (point + t * dir) | = radius
	//	a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	//	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	//	c = (point[0] - origin[0])^2 + (point[1] - origin[1])^2 + (point[2] - origin[2])^2 - radius^2;

	// normalize dir so a = 1
	VectorNormalize(dir);
	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	c = (point[0] - origin[0]) * (point[0] - origin[0]) +
		(point[1] - origin[1]) * (point[1] - origin[1]) +
		(point[2] - origin[2]) * (point[2] - origin[2]) -
		radius * radius;

	d = b * b - 4 * c;
	if (d > 0) {
		t = (- b + sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[0]);
		t = (- b - sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[1]);
		return 2;
	}
	else if (d == 0) {
		t = (- b ) / 2;
		VectorMA(point, t, dir, intersections[0]);
		return 1;
	}
	return 0;
}

/*
================
G_InvulnerabilityEffect
================
*/
int G_InvulnerabilityEffect( gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir ) {
	gentity_t	*impact;
	vec3_t		intersections[2], vec;
	int			n;

	if ( !targ->client ) {
		return qfalse;
	}
	VectorCopy(dir, vec);
	VectorInverse(vec);
	// sphere model radius = 42 units
	n = RaySphereIntersections( targ->client->ps.origin, 42, point, vec, intersections);
	if (n > 0) {
		impact = G_TempEntity( targ->client->ps.origin, EV_INVUL_IMPACT );
		VectorSubtract(intersections[0], targ->client->ps.origin, vec);
		vectoangles(vec, impact->s.angles);
		impact->s.angles[0] += 90;
		if (impact->s.angles[0] > 360)
			impact->s.angles[0] -= 360;
		if ( impactpoint ) {
			VectorCopy( intersections[0], impactpoint );
		}
		if ( bouncedir ) {
			VectorCopy( vec, bouncedir );
			VectorNormalize( bouncedir );
		}
		return qtrue;
	}
	else {
		return qfalse;
	}
}

/*
catchup_damage
*/
static int catchup_damage(int damage, int attacker_points, int target_points) {
    int newdamage;
    if(g_catchup.integer <= 0 )
        return damage;
    //Reduce damage
    if(attacker_points<=target_points+5)
        return damage; //Never reduce damage if only 5 points ahead.

    newdamage=damage-((attacker_points-target_points-5) * (g_catchup.integer*damage))/100;
    if(newdamage<damage/2)
        return damage/2;
    return newdamage;
}

qboolean G_TreasureHuntDamage( gentity_t *targ, gentity_t *attacker, int mod ) {
	if (!attacker || !attacker->client) {
		return qtrue;
	}
	if (targ->client && !OnSameTeam(targ, attacker)) {
		// don't do any damage to enemy players
		return qfalse;
	} else if (targ->s.eType == ET_ITEM 
			&& targ->item 
			&& targ->item->giType == IT_TEAM) {
		if (g_treasureTokensDestructible.integer 
				&& targ->spawnflags != attacker->client->sess.sessionTeam
				&& level.th_phase == TH_SEEK) {
			// only allow enemies to attack cubes during seek time
			return qtrue;
		} else if (targ->spawnflags == attacker->client->sess.sessionTeam
				&& level.th_phase == TH_HIDE
				&& targ->parent == attacker
				&& (mod == MOD_MACHINEGUN || mod == MOD_SHOTGUN || mod == MOD_GAUNTLET 
						|| mod == MOD_RAILGUN || mod == MOD_LIGHTNING || mod == MOD_CHAINGUN)) {
			// players can destroy their own tokens during hiding phase, but only with hitscan weapons
			return qtrue;
		} else {
			return qfalse;
		}
	}
	return qtrue;
}

void G_CheckRocketSniper(gentity_t *victim, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath) {
	if (meansOfDeath != MOD_ROCKET 
			|| !victim 
			|| !victim->client 
			|| victim->client->ps.pm_type == PM_DEAD
			|| !attacker 
			|| !attacker->client 
			|| !inflictor
			|| OnSameTeam(attacker, victim)) {
		return;
	}

	if (Distance(victim->r.currentOrigin, inflictor->s.pos.trBase) < 800.0) {
		return;
	}
	
	AwardMessage(attacker, EAWARD_ROCKETSNIPER, ++(attacker->client->pers.awardCounts[EAWARD_ROCKETSNIPER]));
}

void G_CheckAirrocket(gentity_t *victim, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath) {
	extAward_t award;
	if ((meansOfDeath != MOD_ROCKET && meansOfDeath != MOD_GRENADE)
			|| !victim 
			|| !victim->client 
			|| victim->client->ps.pm_type == PM_DEAD
			|| !attacker 
			|| !attacker->client 
			|| !inflictor
			|| OnSameTeam(attacker, victim)) {
		return;
	}

	// make sure that victim was actually in the air for longer than just a normal jump
	if (victim->client->lastGroundTime != 0 && level.time - victim->client->lastGroundTime < 800) {
		trace_t	tr;
		vec3_t below = {0, 0, -140};

		if (level.time - victim->client->lastGroundTime < 200) {
			return;
		}

		// if the victim has not been airborne for long, check if he at
		// least is at a significant height above ground
		VectorAdd(below, victim->r.currentOrigin, below);
		trap_Trace ( &tr, victim->r.currentOrigin, vec3_origin, vec3_origin, below, victim->s.number, MASK_SOLID|MASK_WATER);
		if (tr.fraction < 1.0) {
			return;
		}
	}

	// don't count point-blank hits
	if (Distance(victim->r.currentOrigin, inflictor->s.pos.trBase) < (meansOfDeath == MOD_ROCKET ? 140 : 100)) {
		return;
	}

	award = meansOfDeath == MOD_ROCKET ? EAWARD_AIRROCKET : EAWARD_AIRGRENADE;
	AwardMessage(attacker, award, ++(attacker->client->pers.awardCounts[award]));
}

void G_CheckRailtwo(gentity_t *victim, gentity_t *attacker, int meansOfDeath, int lastDmgGivenTime, int lastDmgGivenMOD, int lastDmgGivenEntityNum) {
	if (meansOfDeath != MOD_RAILGUN
			|| !victim 
			|| !victim->client 
			|| victim->client->ps.pm_type == PM_DEAD
			|| !attacker 
			|| !attacker->client 
			|| attacker == victim // to exclude railjump
			|| OnSameTeam(attacker, victim)) {
		return;
	}

	if (lastDmgGivenMOD != MOD_RAILGUN 
			|| lastDmgGivenTime != level.time
			|| lastDmgGivenEntityNum == attacker->s.number // to exclude railjump
			) {
		return;
	}

	AwardMessage(attacker, EAWARD_RAILTWO, ++(attacker->client->pers.awardCounts[EAWARD_RAILTWO]));
}

//#define ROCKETRAIL_TIME 1350
#define ROCKETRAIL_TIME 1550
#define LGRAIL_TIME 600
void G_CheckComboAwards(gentity_t *victim, gentity_t *attacker, int mod, int lastDmgGivenEntityNum, int lastDmgGivenTime, int lastDmgGivenMOD) {
	int elapsed;
	if (mod != MOD_RAILGUN
			|| lastDmgGivenTime == 0
			|| !victim 
			|| !victim->client 
			|| victim->client->ps.pm_type == PM_DEAD
			|| !attacker 
			|| !attacker->client 
			|| OnSameTeam(attacker, victim)
			|| victim->s.number != lastDmgGivenEntityNum) {
		return;
	}

	elapsed = level.time - lastDmgGivenTime;

	if (lastDmgGivenMOD == MOD_ROCKET || lastDmgGivenMOD == MOD_ROCKET_SPLASH) {
		if (elapsed > ROCKETRAIL_TIME || victim->client->lastGroundTime + elapsed > level.time) {
			return;
		}
		AwardMessage(attacker, EAWARD_ROCKETRAIL, ++(attacker->client->pers.awardCounts[EAWARD_ROCKETRAIL]));
	} else if (lastDmgGivenMOD == MOD_LIGHTNING) {
		if (level.time - lastDmgGivenTime > LGRAIL_TIME) {
			return;
		}
		AwardMessage(attacker, EAWARD_LGRAIL, ++(attacker->client->pers.awardCounts[EAWARD_LGRAIL]));
	}

}

void G_StoreViewVectorHistory ( gclient_t *client ) {
	AngleVectors (client->ps.viewangles, client->viewvector_history[client->viewvector_head], NULL, NULL);
	client->viewvector_head = (client->viewvector_head + 1) % VIEWVECTOR_HISTORY;
	if (++(client->viewvector_historysize) > VIEWVECTOR_HISTORY) {
		client->viewvector_historysize = VIEWVECTOR_HISTORY;
	}
}

#define TWITCHRAIL_TIME 135
#define TWITCHRAIL_ANGLE 55.0
#define TWITCHRAIL_DISTANCE 100.0
void G_CheckTwitchRail(gentity_t *attacker, gentity_t *victim, int mod) {
	int idx;
	int i;
	int sz;
	vec3_t forward;
	gclient_t *cl;
	double angle;

	if (mod != MOD_RAILGUN
			|| !victim 
			|| !victim->client 
			|| victim->client->ps.pm_type == PM_DEAD
			|| !attacker 
			|| !attacker->client 
			|| OnSameTeam(attacker, victim)) {
		return;
	}

	cl = attacker->client;

	if (cl->lastTeleportTime + TWITCHRAIL_TIME >= level.time) {
		return;
	}
	
	if (Distance(victim->r.currentOrigin, attacker->r.currentOrigin) < TWITCHRAIL_DISTANCE) {
		return;
	}

	sz = cl->viewvector_historysize;
	if ((TWITCHRAIL_TIME*sv_fps.integer)/1000 < sz) {
		sz = (TWITCHRAIL_TIME*sv_fps.integer)/1000;
	}

	AngleVectors (cl->ps.viewangles, forward, NULL, NULL);

	idx = (cl->viewvector_head + VIEWVECTOR_HISTORY - sz) % VIEWVECTOR_HISTORY;
	for (i = 0; i < sz; ++i ) {
		angle = acos(DotProduct(forward, cl->viewvector_history[idx]));
		//Com_Printf("angle %i = %f\n", idx, (float) angle * 180.0/M_PI);
		if (angle * 180.0/M_PI > TWITCHRAIL_ANGLE) {
			AwardMessage(attacker, EAWARD_TWITCHRAIL, ++(attacker->client->pers.awardCounts[EAWARD_TWITCHRAIL]));
			return;
		}
		idx = (idx + 1) % VIEWVECTOR_HISTORY;
	}
}



#define IMMORTALITY_DAMAGE 800
void G_CheckImmortality(gentity_t *ent) {
	if (G_IsElimGT()) {
		// immortality award doesn't make sense in those gametypes
		return;
	}
	if (ent->client->dmgTakenImmortality < IMMORTALITY_DAMAGE) {
		return;
	}
	ent->client->dmgTakenImmortality -= IMMORTALITY_DAMAGE;
	AwardMessage(ent, EAWARD_IMMORTALITY, ++(ent->client->pers.awardCounts[EAWARD_IMMORTALITY]));
}

int G_WeaponForMOD(int mod) {
	switch (mod) {
		case MOD_GAUNTLET:
			return WP_GAUNTLET;
		case MOD_MACHINEGUN:
			return WP_MACHINEGUN;
		case MOD_SHOTGUN:
			return WP_SHOTGUN;
		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
			return WP_GRENADE_LAUNCHER;
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			return WP_ROCKET_LAUNCHER;
		case MOD_PLASMA:
		case MOD_PLASMA_SPLASH:
			return WP_PLASMAGUN;
		case MOD_LIGHTNING:
			return WP_LIGHTNING;
		case MOD_RAILGUN:
			return WP_RAILGUN;
		case MOD_BFG:
		case MOD_BFG_SPLASH:
			return WP_BFG;
		case MOD_GRAPPLE:
			return WP_GRAPPLING_HOOK;
		case MOD_NAIL:
			return WP_NAILGUN;
		case MOD_PROXIMITY_MINE:
			return WP_PROX_LAUNCHER;
		case MOD_CHAINGUN:
			return WP_CHAINGUN;
	}
	return WP_NONE;
}

/*
 * Award score points for damage
 */
void G_CheckDamageScore(gentity_t *attacker, gentity_t *victim, int giveDmg) {
	int diff;

	if (!G_IsTeamGametype() || g_gametype.integer == GT_TEAM) {
		return;
	}
	
	if (g_damageScore.integer <= 0) {
		return;
	}

	// Award 1 point for each g_damageScore.integer damage
	diff = (attacker->client->pers.dmgGiven + giveDmg)/(g_damageScore.integer) 
		- attacker->client->pers.dmgGiven/(g_damageScore.integer);
	if (!diff) {
		return;
	}
	AddScore( attacker, victim->r.currentOrigin, diff );

}

void G_RecordDamageGivenHistory(gentity_t *attacker, int mod, int damage, gentity_t *target) {
	gclient_t *client;
	damageRecord_t *dr;
	if (!target
			|| !target->client 
			|| target->client->ps.pm_type == PM_DEAD
			|| !attacker 
			|| !attacker->client 
			|| OnSameTeam(attacker, target)
			|| damage <= 0) {
		return;
	}

	client = attacker->client;
	client->damage_history_head = (client->damage_history_head + 1 ) % DAMAGE_HISTORY;
	dr = &client->damage_history[client->damage_history_head];
	dr->target = target;
	dr->mod = mod;
	dr->damage = damage;
	dr->died = (target->health <= 0);
	dr->time = level.time;
}

/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/

void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			   vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	gclient_t	*client;
	int			take;
	//int			save;
	int			asave;
	int			knockback;
	int			max;
        
	vec3_t		bouncedir, impactpoint;

	if (!targ->takedamage) {
		return;
	}

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued ) {
		return;
	}

#ifdef WITH_MULTITOURNAMENT
	if (G_MtrnIntermissionQueued(level.currentGameId)) {
		return;
	}
#endif

	if (G_IsElimTeamGT()) {
		// avoid doing damage w/ gaunt during elimination warmup
		if( (( level.time < level.roundStartTime ) && ( level.time>level.roundStartTime-1000*g_elimination_activewarmup.integer ))
				&& mod != MOD_FALLING && mod != MOD_SUICIDE && mod != MOD_TELEFRAG && mod != MOD_SUICIDE) {
			return;
		}
	} else if (g_gametype.integer == GT_TREASURE_HUNTER) {
		if (!G_TreasureHuntDamage(targ, attacker, mod)) {
			return;
		}
	}

	if ( targ->client && mod != MOD_JUICED) {
		if ( targ->client->invulnerabilityTime > level.time) {
			if ( dir && point ) {
				G_InvulnerabilityEffect( targ, dir, point, impactpoint, bouncedir );
			}
			return;
		}
	}
        //Sago: This was moved up
        client = targ->client;

	if (g_freeze.integer) { 
		if (G_IsFrozenPlayerRemnant(targ)) {
			G_FrozenPlayerDamage(targ->frozenPlayer, targ, attacker, inflictor, dir, damage, mod);
			return;
		}
	}

        
        //Sago: See if the client was sent flying
        //Check if damage is by somebody who is not a player!
        if( (!attacker || attacker->s.eType != ET_PLAYER) && client && client->lastSentFlying>-1 && ( mod==MOD_FALLING || mod==MOD_LAVA || mod==MOD_SLIME || mod==MOD_TRIGGER_HURT || mod==MOD_SUICIDE) )  {
            if( client->lastSentFlyingTime+5000<level.time) {
                client->lastSentFlying = -1; //More than 5 seconds, not a kill!
            } else {
                //G_Printf("LastSentFlying %i\n",client->lastSentFlying);
                attacker = &g_entities[client->lastSentFlying];
            }
        }
        
	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER ) {
		if ( targ->use && targ->moverState == MOVER_POS1 ) {
			targ->use( targ, inflictor, attacker );
		}
		return;
	}
	if( g_gametype.integer == GT_OBELISK && CheckObeliskAttack( targ, attacker ) ) {
		return;
	}
	// reduce damage by the attacker's handicap value
	// unless they are rocket jumping
	if ( attacker->client && attacker != targ ) {
		max = attacker->client->ps.stats[STAT_MAX_HEALTH];
		if( bg_itemlist[attacker->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
			max /= 2;
		}
		damage = damage * max / 100;
	}

        //Sago: I have moved this up
	//client = targ->client;
	
	if ( client ) {
		if ( client->noclip ) {
			return;
		}
	}

	if ( !dir ) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize(dir);
	}

	knockback = damage;
	if ( knockback > 200 ) {
		knockback = 200;
	}
	if ( targ->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client ) {
		vec3_t	kvel;
		float	mass;

		mass = 200;

		VectorScale (dir, g_knockback.value * (float)knockback / mass, kvel);
		VectorAdd (targ->client->ps.velocity, kvel, targ->client->ps.velocity);

		//Com_Printf("knockback = %d\n", knockback);

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if ( !targ->client->ps.pm_time ) {
			int		t;

			t = knockback * 2;
			if ( t < 50 ) {
				t = 50;
			}
			if ( t > 200 ) {
				t = 200;
			}
			targ->client->ps.pm_time = t;
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
                //Remeber the last person to hurt the player
                if( !g_awardpushing.integer || targ==attacker || OnSameTeam (targ, attacker)) {
                    targ->client->lastSentFlying = -1;
                } else {
	/*if ( pm->waterlevel <= 1 ) {
		if ( pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK) ) {
			// if getting knocked back, no friction
			if ( ! (pm->ps->pm_flags & PMF_TIME_KNOCKBACK) ) {
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				drop += control*pm_friction*pml.frametime;
			}
		}
	}*/
                    targ->client->lastSentFlying = attacker->s.number;
                    targ->client->lastSentFlyingTime = level.time;
                }

		PushNotify(targ, attacker, mod, knockback);
	}

	// check for completely getting out of the damage
	if ( !(dflags & DAMAGE_NO_PROTECTION) ) {

		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
		if ( mod != MOD_JUICED && mod != MOD_CRUSH && targ != attacker && !(dflags & DAMAGE_NO_TEAM_PROTECTION) && OnSameTeam (targ, attacker)  ) {
			if ( !g_friendlyFire.integer ) {
				return;
			}

			if ( g_friendlyFireReflect.integer && attacker) {
				G_Damage(attacker, NULL, attacker, vec3_origin, vec3_origin,
					       	damage * g_friendlyFireReflectFactor.value,
					       	dflags | DAMAGE_NO_SELF_PROTECTION, mod );
				return;
			}
		}
		if (mod == MOD_PROXIMITY_MINE) {
			if (inflictor && inflictor->parent && OnSameTeam(targ, inflictor->parent)) {
				return;
			}
			if (targ == attacker) {
				return;
			}
		}

		// check for godmode
		if ( targ->flags & FL_GODMODE ) {
			return;
		}

                if(targ->client && targ->client->spawnprotected) {
                   if(level.time>targ->client->respawnTime+g_spawnprotect.integer)
                       targ->client->spawnprotected = qfalse;
                   else
                       if( (mod > MOD_UNKNOWN && mod < MOD_WATER) || mod == MOD_TELEFRAG || mod>MOD_TRIGGER_HURT)
                       return;
                }
	}

	if (client && targ != attacker) {
		if (g_midAir.integer && client->ps.groundEntityNum != ENTITYNUM_NONE) {
			return;
		}
	}


	// battlesuit protects from all radius damage (but takes knockback)
	// and protects 50% against all damage
	if ( client && client->ps.powerups[PW_BATTLESUIT] ) {
		G_AddEvent( targ, EV_POWERUP_BATTLESUIT, 0 );
		if ( ( dflags & DAMAGE_RADIUS ) || ( mod == MOD_FALLING ) ) {
			return;
		}
		damage *= 0.5;
	}

	// add to the attacker's hit counter (if the target isn't a general entity like a prox mine)
	if ( attacker->client && client
			&& (targ != attacker || (dflags & DAMAGE_NO_SELF_PROTECTION))
			&& targ->health > 0
			&& targ->s.eType != ET_MISSILE
			&& targ->s.eType != ET_GENERAL) {
		if ( OnSameTeam( targ, attacker ) || targ == attacker ) {
			attacker->client->ps.persistant[PERS_HITS]--;
		} else {
			attacker->client->ps.persistant[PERS_HITS]++;
			attacker->client->ps.persistant[PERS_DAMAGE_DONE] += damage;
		}
		attacker->client->ps.persistant[PERS_ATTACKEE_ARMOR] = (targ->health<<8)|(client->ps.stats[STAT_ARMOR]);
	}

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	if ( targ == attacker && !(dflags & DAMAGE_NO_SELF_PROTECTION)) {
		damage *= 0.5;
	}

        if(targ && targ->client && attacker->client )
            damage = catchup_damage(damage, attacker->client->ps.persistant[PERS_SCORE], targ->client->ps.persistant[PERS_SCORE]);
        
        if(g_damageModifier.value > 0.01) {
            damage *= g_damageModifier.value;
        }

	if ( damage < 1 ) {
		damage = 1;
	}
        
        if(targ == attacker && (g_dmflags.integer & DF_NO_SELF_DAMAGE) && !(dflags & DAMAGE_NO_SELF_PROTECTION))
            damage = 0;

	if ((G_IsElimGT() || g_elimination_allgametypes.integer)
				&& g_elimination_selfdamage.integer<1 && ( targ == attacker ||  mod == MOD_FALLING ) && !(dflags & DAMAGE_NO_SELF_PROTECTION)) {
		damage = 0;
	}


//So people can be telefragged!
	if (G_IsElimGT() && level.time < level.roundStartTime && ((mod == MOD_LAVA) || (mod == MOD_SLIME)) ) {
		damage = 1000;
	}


	take = damage;
	//save = 0;

	// save some from armor
	asave = CheckArmor (targ, take, dflags);
	take -= asave;

	if ( g_debugDamage.integer ) {
		G_Printf( "%i: client:%i health:%i damage:%i armor:%i\n", level.time, targ->s.number,
			targ->health, take, asave );
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( client ) {
		if ( attacker ) {
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
                } else if(client->lastSentFlying) {
                        client->ps.persistant[PERS_ATTACKER] = client->lastSentFlying;
                } else {
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		if ( dir ) {
			VectorCopy ( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		} else {
			VectorCopy ( targ->r.currentOrigin, client->damage_from );
			client->damage_fromWorld = qtrue;
		}
	}

	// See if it's the player hurting the emeny flag carrier
	if( g_gametype.integer == GT_CTF || g_gametype.integer == GT_1FCTF || g_gametype.integer == GT_CTF_ELIMINATION) {
		Team_CheckHurtCarrier(targ, attacker);
	}

	if (targ->client) {
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;
	}

	//If vampire is enabled, gain health but not from self or teammate, cannot steal more than targ has
	if( g_vampire.value>0.0 && (targ != attacker) && take > 0 && 
                !(OnSameTeam(targ, attacker)) && attacker->health > 0 && targ->health > 0 )
	{
		if(take<targ->health)
			attacker->health += (int)(((float)take)*g_vampire.value);
		else
			attacker->health += (int)(((float)targ->health)*g_vampire.value);
		if(attacker->health>g_vampireMaxHealth.integer)
			attacker->health = g_vampireMaxHealth.integer;
	}

	if ( g_damagePlums.integer && damage > 0 && targ->client && targ != attacker && targ->health > 0) {
		if (mod == MOD_SHOTGUN) {
			targ->client->shotgunDamagePlumDmg += damage;
		} else {
			DamagePlum(attacker, targ, mod, damage);
		}
	}

	// do the damage
	if (take) {
		int dmgTaken = (take >= 0 ? take : 0) + (asave >= 0 ? asave : 0);
		int lastDmgGivenEntityNum = -1;
		int lastDmgGivenTime = 0;
		int lastDmgGivenMOD = MOD_UNKNOWN;

		if (attacker->client) {
			lastDmgGivenEntityNum = attacker->client->lastDmgGivenEntityNum;
			lastDmgGivenTime = attacker->client->lastDmgGivenTime;
			lastDmgGivenMOD = attacker->client->lastDmgGivenMOD;
		}

		targ->health = targ->health - take;
		if ( targ->client ) {
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
		}

		G_CheckRocketSniper(targ, inflictor, attacker, mod);
		G_CheckAirrocket(targ, inflictor, attacker, mod);
		G_CheckRailtwo(targ, attacker, mod, lastDmgGivenTime, lastDmgGivenMOD, lastDmgGivenEntityNum);
		G_CheckTwitchRail(attacker, targ, mod);

		// stats
		if (targ->health < 0) {
			dmgTaken += targ->health;
			if (dmgTaken < 0) {
				dmgTaken = 0;
			}
		}
		if (targ->client && targ->client->ps.pm_type != PM_DEAD) {
			if (G_IsElimGT()) {
				targ->client->pers.elimRoundDmgTaken += dmgTaken;
				if (level.roundNumber == level.roundNumberStarted) {
					targ->client->pers.dmgTaken += dmgTaken;
				}
			} else {
				targ->client->pers.dmgTaken += dmgTaken;
				if (attacker->client && targ != attacker && !OnSameTeam(targ, attacker)) {
					// you only get the immortality award for damage done by an enemy player
					targ->client->dmgTakenImmortality += dmgTaken;
				}
			}
		}
		if (targ != attacker) {
			if (attacker && attacker->client && targ->client 
					&& targ->client->ps.pm_type != PM_DEAD && !OnSameTeam(targ, attacker) ) {
				int weapon = G_WeaponForMOD(mod);
				if (G_IsElimGT()) {
					attacker->client->pers.elimRoundDmgDone += dmgTaken;
					if (level.roundNumber == level.roundNumberStarted) {
						G_CheckDamageScore(attacker, targ, dmgTaken);
						attacker->client->pers.dmgGiven += dmgTaken;
						attacker->client->pers.damage[weapon] += dmgTaken;
						G_RecordDamageGivenHistory(attacker, mod, damage, targ);
					}
				} else {
					G_CheckDamageScore(attacker, targ, dmgTaken);
					attacker->client->pers.dmgGiven += dmgTaken;
					attacker->client->pers.damage[weapon] += dmgTaken;
					G_RecordDamageGivenHistory(attacker, mod, damage, targ);
				}
				attacker->client->lastDmgGivenEntityNum = targ->s.number;
				attacker->client->lastDmgGivenTime = level.time;
				attacker->client->lastDmgGivenMOD = mod;
			}
		}
		///
			
		if ( targ->health <= 0 ) {
			G_CheckComboAwards(targ, attacker, mod, lastDmgGivenEntityNum, lastDmgGivenTime, lastDmgGivenMOD);
			if ( client )
				targ->flags |= FL_NO_KNOCKBACK;

			if (targ->health < -999)
				targ->health = -999;
                        
			targ->enemy = attacker;
			targ->die (targ, inflictor, attacker, take, mod);
			return;
		} else if ( targ->pain ) {
			targ->pain (targ, attacker, take);
		}

		if (targ->client && targ->client->ps.pm_type != PM_DEAD && dmgTaken)  {
			// player took damage but survived, check for immortality award
			G_CheckImmortality(targ);
		}
	}

	
}


/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage (gentity_t *targ, vec3_t origin) {
	vec3_t	dest;
	trace_t	tr;
	vec3_t	midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd (targ->r.absmin, targ->r.absmax, midpoint);
	VectorScale (midpoint, 0.5, midpoint);

	VectorCopy (midpoint, dest);
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	// this should probably check in the plane of projection, 
	// rather than in world coordinate, and also include Z
	VectorCopy (midpoint, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;


	return qfalse;
}

#define RAILJUMP_TIME 800
qboolean G_RailJump( vec3_t origin, gentity_t *attacker) {
	float		points, dist;
	float damage = 100;
	float radius = 120;
	vec3_t		v;
	vec3_t		dir;
	int i;

	// find the distance from the edge of the bounding box
	for ( i = 0 ; i < 3 ; i++ ) {
		if ( origin[i] < attacker->r.absmin[i] ) {
			v[i] = attacker->r.absmin[i] - origin[i];
		} else if ( origin[i] > attacker->r.absmax[i] ) {
			v[i] = origin[i] - attacker->r.absmax[i];
		} else {
			v[i] = 0;
		}
	}

	dist = VectorLength( v );
	if ( dist >= radius ) {
		return qfalse;
	}

	points = damage * ( 1.0 - dist / radius );

	if( CanDamage (attacker, origin) || g_damageThroughWalls.integer ) {
		VectorSubtract (attacker->r.currentOrigin, origin, dir);
		// push the center of mass higher than the origin so players
		// get knocked into the air more
		dir[2] += 24;
		G_Damage (attacker, attacker, attacker, dir, origin, (int)points, DAMAGE_RADIUS, MOD_RAILGUN);
		if (attacker->client && attacker->client->ps.weaponTime > RAILJUMP_TIME) {
			attacker->client->ps.weaponTime = RAILJUMP_TIME;
		}
		return qtrue;
	}
	return qfalse;
}


/*
============
G_RadiusDamage
============
*/
qboolean G_RadiusDamage ( vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius,
					 gentity_t *ignore, int mod) {
	float		points, dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;
	qboolean	hitClient = qfalse;

	if ( radius < 1 ) {
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if( CanDamage (ent, origin) || g_damageThroughWalls.integer ) {
			if( LogAccuracyHit( ent, attacker ) ) {
				hitClient = qtrue;
			}
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_Damage (ent, inflictor, attacker, dir, origin, (int)points, DAMAGE_RADIUS, mod);
		}
	}

	return hitClient;
}
