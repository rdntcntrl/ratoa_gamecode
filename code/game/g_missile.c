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

#define MISSILE_PRESTEP_TIME	50

int G_DelagLatency(gclient_t *client) {
	int ping = 0;
	switch (g_delagMissileLatencyMode.integer) {
		case 2:
			ping = client->ps.ping;
			break;
		case 3:
			ping = client->pers.realPing;
			break;
		case 1:
		default:
			ping = level.previousTime + client->frameOffset - client->attackTime;
			if (ping < 0) {
				ping = 0;
			}
			break;
	}
	if (g_delagMissileLimitVariance.integer && g_delagMissileLimitVarianceMs.integer > 0 && g_truePing.integer) {
		int maxping = client->pers.realPing + g_delagMissileLimitVarianceMs.integer;
		int minping = client->pers.realPing - g_delagMissileLimitVarianceMs.integer;
		qboolean limited = qfalse;
		int oldping = ping;

		if (minping < 0) {
			minping = 0;
		}
		if (ping > maxping) {
			ping = maxping;
			limited = qtrue;
		} else if (ping < minping) {
			ping = minping;
			limited = qtrue;
		}
		if (limited && g_delagMissileDebug.integer) {
			Com_Printf("Limited projectile delag ping (c %i): %i -> %i, realPing: %i\n", client->ps.clientNum, oldping, ping, client->pers.realPing);
		}
	}
	return MIN(g_delagMissileMaxLatency.integer, ping);
}

int G_MissileLagTime(gclient_t *client) {
	int offset = 0;

	if (!g_delagMissiles.integer) {
		return MISSILE_PRESTEP_TIME;
	}

	if (g_delagMissileCorrectFrameOffset.integer) {
		offset = level.time - (level.previousTime + client->frameOffset);
		if (offset < 0) {
			offset = 0;
		}
		if (offset > 1000/sv_fps.integer) {
			offset = 1000/sv_fps.integer;
		}
	}
	return offset + G_DelagLatency(client) + g_delagMissileBaseNudge.integer;
}

void G_MissileRunDelag(gentity_t *ent, int stepmsec) {
	int prevTimeSaved;
	int lvlTimeSaved;
	int projectileDelagTime;

	if (g_delagMissileNudgeOnly.integer
			|| level.previousTime <= DELAG_MAX_BACKTRACK
			|| stepmsec <= 0) {
		return;
	}

	// if we see the missile late due to lag & PRESTEP
	// compute the flight since it was launched, 
	// shifting clients back accordingly
	if ( !G_InUse(ent)
			|| ent->freeAfterEvent
			|| ent->s.eType != ET_MISSILE
			|| !ent->needsDelag ) {
		return;
	}

	prevTimeSaved = level.previousTime;
	lvlTimeSaved = level.time;
	projectileDelagTime = level.previousTime - (DELAG_MAX_BACKTRACK/stepmsec) * stepmsec;
	while (projectileDelagTime < prevTimeSaved) {
		if ( !G_InUse(ent) || ent->freeAfterEvent ) {
			// make sure we don't run missile again
			// if it exploded already
			break;
		}
		if (projectileDelagTime >= ent->launchTime) {
			int shiftTime = projectileDelagTime;
			G_TimeShiftAllClients( shiftTime, ent->parent );
			level.time = projectileDelagTime + stepmsec;
			level.previousTime = projectileDelagTime;


			G_RunMissile( ent );

			level.time = lvlTimeSaved;
			level.previousTime = prevTimeSaved;
			G_UnTimeShiftAllClients( ent->parent );
		}
		projectileDelagTime += stepmsec;
	}
	ent->needsDelag = qfalse;
}

void G_ImmediateRunMissile(gentity_t *ent) {
	if (ent->missileRan == 1) {
		// missile was already run immediately after firing
		return;
	}

	if (!g_delagMissileNudgeOnly.integer) {
		int stepmsec = level.time - level.previousTime;
		G_MissileRunDelag(ent, stepmsec);

	}
	if ( !G_InUse(ent) ) {
		return;
	}

	// temporary entities don't think
	if ( ent->freeAfterEvent ) {
		return;
	}

	if ( ent->s.eType == ET_MISSILE ) {
		G_RunMissile( ent );
	}
}

void G_ImmediateLaunchMissile(gentity_t *ent) {
	if (g_delagMissileImmediateRun.integer == 1) {
		G_ImmediateRunMissile(ent);
	} else if (g_delagMissileImmediateRun.integer >= 2) {
		ent->missileRan = -g_delagMissileImmediateRun.integer+1;
	}
}

void G_ImmediateRunClientMissiles(gentity_t *client) {
	gentity_t *ent;
	int i;
	if (g_delagMissileImmediateRun.integer <= 1) {
		return;
	}
	for (i=0 ; i < level.num_entities ; ++i ) {
		ent = &g_entities[i];
		if ( !G_InUse(ent)
				|| ent->freeAfterEvent
				|| ent->s.eType != ET_MISSILE
				|| ent->parent != client) {
			continue;
		}
		if (ent->missileRan < 0) {
			// this missile will be run later
			ent->missileRan++;
			continue;
		}
		G_ImmediateRunMissile(ent);
	}
}

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int	hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	if ( ent->s.eFlags & EF_BOUNCE_HALF ) {
		VectorScale( ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 ) {
			G_SetOrigin( ent, trace->endpos );
                        ent->s.time = level.time / 4;
			return;
		}
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}

/*
================
G_TeleportMissile

================
*/
void G_TeleportMissile( gentity_t *ent, trace_t *trace, gentity_t *portal ) {
	gentity_t		*dest;
	vec3_t 			velocity;
	int			hitTime;
	vec3_t			portalInVec;
	vec3_t			portalInAngles;
	vec3_t			rotationAngles;
	vec3_t			rotationMatrix[3];
	vec_t			length_norm,length_neg_norm;
	vec3_t			tmp;

	dest =  G_PickTarget( portal->target );
	if (!dest) {
		G_Printf ("Couldn't find teleporter destination\n");
		return;
	}

	if (g_usesRatVM.integer) {
		G_TempEntity(ent->r.currentOrigin, EV_MISSILE_TELEPORT );
	}

	// evaluate velocity vector at portal impact
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
        BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );

	// check orientation of normal vector
	VectorAdd(trace->plane.normal, velocity, tmp);
	length_norm = VectorLengthSquared(tmp);

	VectorNegate(trace->plane.normal, portalInVec);
	VectorAdd(portalInVec, velocity, tmp);
	length_neg_norm = VectorLengthSquared(tmp);

	vectoangles(portalInVec, portalInAngles);
	if (length_norm > length_neg_norm) {
		VectorSubtract(dest->s.angles, portalInAngles, rotationAngles);
	} else {
		VectorSubtract(portalInAngles, dest->s.angles, rotationAngles);
	}

	// create rotation matrix
	AngleVectors(rotationAngles, rotationMatrix[0], rotationMatrix[1], rotationMatrix[2]);
        VectorInverse(rotationMatrix[1]);

	// rotate velocity vector
	VectorRotate(velocity, rotationMatrix, ent->s.pos.trDelta);
	SnapVector(ent->s.pos.trDelta);

	// set flag to indicate missile teleport
	ent->s.eFlags ^= EF_TELEPORT_BIT;

	ent->missileTeleported = qtrue;

	// set new origin
	VectorCopy(dest->s.origin, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);

	ent->s.pos.trTime = level.time;

	if (g_usesRatVM.integer) {
		gentity_t *tmp_ent = G_TempEntity(ent->r.currentOrigin, EV_MISSILE_TELEPORT );
		tmp_ent->r.svFlags |= SVF_BROADCAST;
	}
}

/*
================
G_PushGrenade

================
*/
void G_PushGrenade( gentity_t *ent, trace_t *trace, gentity_t *jumppad ) {
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	VectorCopy(jumppad->s.origin2, ent->s.pos.trDelta);
	ent->s.pos.trTime = level.time;
	if (g_usesRatVM.integer) {
		G_AddEvent( ent, EV_JUMP_PAD, 0 );
	}
}




/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
	vec3_t		origin;
	int ownerKills = 0, ownerDeaths = 0;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

	ent->freeAfterEvent = qtrue;

	if (ent->parent && ent->parent->client) {
		ownerKills = ent->parent->client->pers.kills;
		ownerDeaths = ent->parent->client->pers.deaths;
	}

	// splash damage
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( ent->r.currentOrigin, ent, ent->parent, ent->splashDamage, ent->splashRadius, ent
			, ent->splashMethodOfDeath ) ) {
			g_entities[ent->r.ownerNum].client->accuracy_hits++;
                        g_entities[ent->r.ownerNum].client->accuracy[ent->s.weapon][1]++;
		}
	}

	trap_LinkEntity( ent );

	G_CheckKamikazeAward(ent->parent, ownerKills, ownerDeaths);
}

/*
================
ProximityMine_Explode
================
*/
static void ProximityMine_Explode( gentity_t *mine ) {
	G_ExplodeMissile( mine );
	// if the prox mine has a trigger free it
	if (mine->activator) {
		G_FreeEntity(mine->activator);
		mine->activator = NULL;
	}
}

/*
================
ProximityMine_Die
================
*/
static void ProximityMine_Die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + 1;
}

/*
================
ProximityMine_Trigger
================
*/
void ProximityMine_Trigger( gentity_t *trigger, gentity_t *other, trace_t *trace ) {
	vec3_t		v;
	gentity_t	*mine;

	if( !other->client ) {
		return;
	}

	// trigger is a cube, do a distance test now to act as if it's a sphere
	VectorSubtract( trigger->s.pos.trBase, other->s.pos.trBase, v );
	if( VectorLength( v ) > trigger->parent->splashRadius ) {
		return;
	}


	if (G_IsTeamGametype()) {
		// don't trigger same team mines
		if (trigger->parent->s.generic1 == other->client->sess.sessionTeam) {
			return;
		}
	}

	// ok, now check for ability to damage so we don't get triggered thru walls, closed doors, etc...
	if( !CanDamage( other, trigger->s.pos.trBase ) ) {
		return;
	}

	// trigger the mine!
	mine = trigger->parent;
	mine->s.loopSound = 0;
	G_AddEvent( mine, EV_PROXIMITY_MINE_TRIGGER, 0 );
	mine->nextthink = level.time + 500;

	G_FreeEntity( trigger );
}

/*
================
ProximityMine_Activate
================
*/
static void ProximityMine_Activate( gentity_t *ent ) {
	gentity_t	*trigger;
	float		r;
        vec3_t          v1;
        gentity_t       *flag;
        char            *c;
        qboolean        nearFlag = qfalse;

        // find the flag
        switch (ent->s.generic1) {
        case TEAM_RED:
                c = "team_CTF_redflag";
                break;
        case TEAM_BLUE:
                c = "team_CTF_blueflag";
                break;
        default:
            c = NULL;
        }

        if(c) {
            flag = NULL;
            while ((flag = G_Find (flag, FOFS(classname), c)) != NULL) {
                    if (!(flag->flags & FL_DROPPED_ITEM))
                            break;
            }

            if(flag) {
                VectorSubtract(ent->r.currentOrigin,flag->r.currentOrigin , v1);
                if(VectorLength(v1) < 500)
                    nearFlag = qtrue;
            }
        }

	ent->think = ProximityMine_Explode;
        if( nearFlag)
            ent->nextthink = level.time + g_proxMineTimeout.integer/15;
        else
            ent->nextthink = level.time + g_proxMineTimeout.integer;

	ent->takedamage = qtrue;
	ent->health = 1;
	ent->die = ProximityMine_Die;

	ent->s.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav" );

	// build the proximity trigger
	trigger = G_Spawn ();

	trigger->classname = "proxmine_trigger";

	r = ent->splashRadius;
	VectorSet( trigger->r.mins, -r, -r, -r );
	VectorSet( trigger->r.maxs, r, r, r );

	G_SetOrigin( trigger, ent->s.pos.trBase );

	trigger->parent = ent;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->touch = ProximityMine_Trigger;

	trap_LinkEntity (trigger);

	// set pointer to trigger so the entity can be freed when the mine explodes
	ent->activator = trigger;
}

/*
================
ProximityMine_ExplodeOnPlayer
================
*/
static void ProximityMine_ExplodeOnPlayer( gentity_t *mine ) {
	gentity_t	*player;

	player = mine->enemy;
	player->client->ps.eFlags &= ~EF_TICKING;

	if ( player->client->invulnerabilityTime > level.time ) {
		G_Damage( player, mine->parent, mine->parent, vec3_origin, mine->s.origin, 1000, DAMAGE_NO_KNOCKBACK, MOD_JUICED );
		player->client->invulnerabilityTime = 0;
		G_TempEntity( player->client->ps.origin, EV_JUICED );
	}
	else {
		G_SetOrigin( mine, player->s.pos.trBase );
		// make sure the explosion gets to the client
		mine->r.svFlags &= ~SVF_NOCLIENT;
		mine->splashMethodOfDeath = MOD_PROXIMITY_MINE;
		G_ExplodeMissile( mine );
	}
}

/*
================
ProximityMine_Player
================
*/
static void ProximityMine_Player( gentity_t *mine, gentity_t *player ) {
	if( mine->s.eFlags & EF_NODRAW || !player->client) {
		return;
	}

	G_AddEvent( mine, EV_PROXIMITY_MINE_STICK, 0 );

	if( player->s.eFlags & EF_TICKING ) {
		player->activator->splashDamage += mine->splashDamage;
		player->activator->splashRadius *= 1.50;
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}

	player->client->ps.eFlags |= EF_TICKING;
	player->activator = mine;

	mine->s.eFlags |= EF_NODRAW;
	mine->r.svFlags |= SVF_NOCLIENT;
	mine->s.pos.trType = TR_LINEAR;
	VectorClear( mine->s.pos.trDelta );

	mine->enemy = player;
	mine->think = ProximityMine_ExplodeOnPlayer;
	if ( player->client->invulnerabilityTime > level.time ) {
		mine->nextthink = level.time + 2 * 1000;
	}
	else {
		mine->nextthink = level.time + 10 * 1000;
	}
}

/*
 *=================
 *ProximityMine_RemoveAll
 *=================
 */

void ProximityMine_RemoveAll() {
    gentity_t	*mine;
    
    mine = NULL;
    
    while ((mine = G_Find (mine, FOFS(classname), "prox mine")) != NULL) {
        mine->think = ProximityMine_Explode;
	mine->nextthink = level.time + 1;
    }
}

/*
================
G_MissileImpact
================
*/
void G_MissileImpact( gentity_t *ent, trace_t *trace ) {
	gentity_t		*other;
	qboolean		hitClient = qfalse;
	vec3_t			forward, impactpoint, bouncedir;
	int				eFlags;
	other = &g_entities[trace->entityNum];

	//if (other->s.eType == ET_ITEM 
	//		|| other->s.eType == ET_MISSILE) {
	//	ent->target_ent = other;
	//	return;
	//} else if (other->s.eType == ET_PLAYER && ent->r.ownerNum == other->s.number) {
	//	ent->target_ent = other;
	//	return;
	//} else {
	//	ent->target_ent = NULL;
	//}

	//// check if missile hit portal
	//if (other->s.eType == ET_TELEPORT_TRIGGER) {
	//	if (g_teleMissiles.integer == 1 && other->target) {
	//		G_TeleportMissile( ent, trace, other );
	//	}
	//	return;
	//}

	//// check if grenade hit jumppad
	//if (other->s.eType == ET_PUSH_TRIGGER) {
	//       if (g_pushGrenades.integer == 1
	//		       && other->target 
	//		       && strcmp(ent->classname, "grenade") == 0) {
	//	       G_PushGrenade( ent, trace, other );
	//	       return;
	//       }
	//       ent->target_ent = other;
	//       return;
	//}

	//if (other->r.contents == CONTENTS_TRIGGER) {
	//	// check for equality, r.contents seems to not contain other FLAGS?
	//	ent->target_ent = other;
	//	return;
	//}
	
	// check for bounce
	if ( !other->takedamage &&
		( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) ) {
		G_BounceMissile( ent, trace );
		G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		return;
	}


	if ( other->takedamage ) {
		if ( ent->s.weapon != WP_PROX_LAUNCHER ) {
			if ( other->client && other->client->invulnerabilityTime > level.time ) {
              
				//
				VectorCopy( ent->s.pos.trDelta, forward );
				VectorNormalize( forward );
				if (G_InvulnerabilityEffect( other, forward, ent->s.pos.trBase, impactpoint, bouncedir )) {
					VectorCopy( bouncedir, trace->plane.normal );
					eFlags = ent->s.eFlags & EF_BOUNCE_HALF;
					ent->s.eFlags &= ~EF_BOUNCE_HALF;
					G_BounceMissile( ent, trace );
					ent->s.eFlags |= eFlags;
				}
				ent->target_ent = other;
				return;
			}
		}
	}
	// impact damage
	if (other->takedamage) {
		// FIXME: wrong damage direction?
		if ( ent->damage ) {
			vec3_t	velocity;

			if( LogAccuracyHit( other, &g_entities[ent->r.ownerNum] ) ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
                                g_entities[ent->r.ownerNum].client->accuracy[ent->s.weapon][1]++;
			}
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			if ( VectorLength( velocity ) == 0 ) {
				velocity[2] = 1;	// stepped on a grenade
			}
			G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
				ent->s.origin, ent->damage, 
				0, ent->methodOfDeath);
		}
	}

	if( ent->s.weapon == WP_PROX_LAUNCHER ) {
		if( ent->s.pos.trType != TR_GRAVITY ) {
			return;
		}

		// if it's a player, stick it on to them (flag them and remove this entity)
		if( other->s.eType == ET_PLAYER && other->health > 0 && !G_IsFrozenPlayerRemnant(other) ) {
			ProximityMine_Player( ent, other );
			return;
		}

		SnapVectorTowards( trace->endpos, ent->s.pos.trBase );
		G_SetOrigin( ent, trace->endpos );
		ent->s.pos.trType = TR_STATIONARY;
		VectorClear( ent->s.pos.trDelta );

		G_AddEvent( ent, EV_PROXIMITY_MINE_STICK, trace->surfaceFlags );

		ent->think = ProximityMine_Activate;
		ent->nextthink = level.time + 2000;

		vectoangles( trace->plane.normal, ent->s.angles );
		ent->s.angles[0] += 90;

		// link the prox mine to the other entity
		ent->enemy = other;
		ent->die = ProximityMine_Die;
		VectorCopy(trace->plane.normal, ent->movedir);
		VectorSet(ent->r.mins, -4, -4, -4);
		VectorSet(ent->r.maxs, 4, 4, 4);
		trap_LinkEntity(ent);

		return;
	}

	if (!strcmp(ent->classname, "hook")) {
		gentity_t *nent;
		vec3_t v;

		nent = G_Spawn();
		if ( other->takedamage && other->client ) {

			G_AddEvent( nent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
			nent->s.otherEntityNum = other->s.number;

			ent->enemy = other;

			v[0] = other->r.currentOrigin[0] + (other->r.mins[0] + other->r.maxs[0]) * 0.5;
			v[1] = other->r.currentOrigin[1] + (other->r.mins[1] + other->r.maxs[1]) * 0.5;
			v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5;

			SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth
		} else {
			VectorCopy(trace->endpos, v);
			G_AddEvent( nent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
			ent->enemy = NULL;
		}

		SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth

		nent->freeAfterEvent = qtrue;
		// change over to a normal entity right at the point of impact
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GRAPPLE;

		G_SetOrigin( ent, v );
		G_SetOrigin( nent, v );

		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;

		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);

		trap_LinkEntity( ent );
		trap_LinkEntity( nent );

		return;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && other->client ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}

	ent->freeAfterEvent = qtrue;

	SnapVectorTowards( trace->endpos, ent->s.pos.trBase );	// save net bandwidth

	if (g_usesRatVM.integer) {
		ent->missileExploded = qtrue;
		// set time of explosion so client knows not to render missile anymore
		ent->s.time2 = level.time > 0 ? level.time : 1;
		VectorCopy( trace->endpos, ent->s.origin2 );
		// save owner of missile
		ent->s.otherEntityNum2 = ent->r.ownerNum;
	} else {
		// change over to a normal entity right at the point of impact
		ent->s.eType = ET_GENERAL;
		G_SetOrigin( ent, trace->endpos );
	}


	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( trace->endpos, ent, ent->parent, ent->splashDamage, ent->splashRadius, 
			other, ent->splashMethodOfDeath ) ) {
			if( !hitClient ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
                                g_entities[ent->r.ownerNum].client->accuracy[ent->s.weapon][1]++;
			}
		}
	}

	trap_LinkEntity( ent );
}


#define TELEMISSILE_MAX_TRIGGERS 32
/*
================
G_RunMissile
================
*/
void G_RunMissile( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	trace_t		tr2;
	gentity_t	*stuckIn;
	int			passent;
	gentity_t	*unlinkedEntities[TELEMISSILE_MAX_TRIGGERS];
	int		unlinked = 0;
	int		i;
	int		telepushed = 0;
	int ownerKills = 0, ownerDeaths = 0;
	gentity_t *owner = ent->parent;

	ent->missileRan = 1;

	if (ent->missileExploded) {
		return;
	}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// if this missile bounced off an invulnerability sphere
	if ( ent->target_ent ) {
		passent = ent->target_ent->s.number;
	}
	// prox mines that left the owner bbox will attach to anything, even the owner
	else if (ent->s.weapon == WP_PROX_LAUNCHER && ent->count) {
		passent = ENTITYNUM_NONE;
	}
	else {
		// ignore interactions with the missile owner
		passent = ent->r.ownerNum;
	}

	if (ent->parent && ent->parent->client) {
		ownerKills = ent->parent->client->pers.kills;
		ownerDeaths = ent->parent->client->pers.deaths;
	}

	if (g_teleMissiles.integer == 1 || g_pushGrenades.integer == 1) {
		do {
			// trace a line from the previous position to the current position
			trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask);
			trap_Trace( &tr2, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask | CONTENTS_TRIGGER );

			if (tr2.fraction == 1) {
				break;
			}

			if (tr2.entityNum == tr.entityNum) {
				break;
			}

			stuckIn = &g_entities[tr2.entityNum];
			if (stuckIn->s.eType == ET_TELEPORT_TRIGGER) {
				if (g_teleMissiles.integer == 1 
						&& stuckIn->target
						&& (!g_teleMissilesMaxTeleports.integer || ent->missileTeleportCount < g_teleMissilesMaxTeleports.integer)) {
					G_TeleportMissile( ent, &tr2, stuckIn );
					ent->missileTeleportCount++;
					// make sure we don't impact
					tr.fraction = 1;
					telepushed = 1;
					break;
				}
			} else if (stuckIn->s.eType == ET_PUSH_TRIGGER) {
				if (g_pushGrenades.integer == 1 
						&& stuckIn->target
						&& strcmp(ent->classname, "grenade") == 0
						&& ent->pushed_at + 200 <= level.time) {
					G_PushGrenade( ent, &tr2, stuckIn );
					//ent->target_ent = stuckIn;
					ent->pushed_at = level.time;
					// make sure we don't impact
					tr.fraction = 1;
					telepushed = 1;
					break;
				}
			}
			// other trigger, unlink entity so next trace won't
			// catch this
			trap_UnlinkEntity( stuckIn );
			unlinkedEntities[unlinked++] = stuckIn;

		} while (unlinked < TELEMISSILE_MAX_TRIGGERS) ;
		// link entities again
		for ( i = 0 ; i < unlinked ; ++i ) {
			trap_LinkEntity( unlinkedEntities[i] );
		}
	} else {
		// trace a line from the previous position to the current position
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask );
	}
	if ( tr.startsolid || tr.allsolid ) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask );
		tr.fraction = 0;
	}
	else if (!telepushed) {
		VectorCopy( tr.endpos, ent->r.currentOrigin );
	}
	trap_LinkEntity( ent );

	if ( tr.fraction != 1 ) {
		// never explode or bounce on sky
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent) {
				ent->parent->client->hook = NULL;
			}
			G_FreeEntity( ent );
			return;
		}
		G_MissileImpact( ent, &tr );
		if ( ent->s.eType != ET_MISSILE || ent->missileExploded) {
			G_CheckKamikazeAward(owner, ownerKills, ownerDeaths);
			return;		// exploded
		}
	}
	// if the prox mine wasn't yet outside the player body
	if (ent->s.weapon == WP_PROX_LAUNCHER && !ent->count) {
		// check if the prox mine is outside the owner bbox
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ENTITYNUM_NONE, ent->clipmask );
		if (!tr.startsolid || tr.entityNum != ent->r.ownerNum) {
			ent->count = 1;
		}
	}
	// check think function after bouncing
	G_RunThink( ent );
}

void G_ApplyMissileNudge (gentity_t *self, gentity_t *bolt) {
	if (!self->client) {
		return;
	}
	bolt->s.pos.trTime -= G_MissileLagTime(self->client);
	bolt->needsDelag = qtrue;
	bolt->launchTime = bolt->s.pos.trTime;
	
	if (G_IsElimGT() && level.time > level.roundStartTime - 1000*g_elimination_activewarmup.integer) {
		if (bolt->launchTime < level.roundStartTime) {
			int prestep = 0;
			if (g_delagMissiles.integer) {
				prestep = g_delagMissileBaseNudge.integer;
			} else {
				prestep = MISSILE_PRESTEP_TIME;
			}
			if (bolt->launchTime < level.roundStartTime-prestep) {
				bolt->s.pos.trTime = level.roundStartTime-prestep;
				bolt->launchTime = level.roundStartTime-prestep;
			}

		}
	}
}

//=============================================================================

/*
=================
fire_plasma

=================
*/
gentity_t *fire_plasma (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "plasma";
	bolt->nextthink = level.time + PLASMA_THINKTIME;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PLASMAGUN;
	bolt->r.ownerNum = self->s.number;
//unlagged - projectile nudge
	// we'll need this for nudging projectiles later
	bolt->s.otherEntityNum = self->s.number;
//unlagged - projectile nudge
	bolt->parent = self;
	bolt->damage = 20;
	bolt->splashDamage = 15;
	bolt->splashRadius = 20;
	bolt->methodOfDeath = MOD_PLASMA;
	bolt->splashMethodOfDeath = MOD_PLASMA_SPLASH;
	bolt->clipmask = MASK_SHOT;
	//if (g_teleMissiles.integer == 1) {
	//	bolt->clipmask |= CONTENTS_TRIGGER;
	//}
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	//bolt->s.pos.trTime = level.time;
	G_ApplyMissileNudge(self, bolt);
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, PLASMA_VELOCITY, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}	

//=============================================================================


/*
=================
fire_grenade
=================
*/
gentity_t *fire_grenade (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 2500;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_GRENADE_LAUNCHER;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
//unlagged - projectile nudge
	// we'll need this for nudging projectiles later
	bolt->s.otherEntityNum = self->s.number;
//unlagged - projectile nudge

	if (self && self->client) {
		//FIXME: we prolly wanna abuse another field
		bolt->s.generic1 = self->client->sess.sessionTeam;
	} else {
		// in case this gets launched from a shooter_grenade
		bolt->s.generic1 = TEAM_FREE;
	}

	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;
	//if (g_teleMissiles.integer == 1 || g_pushGrenades.integer == 1) {
	//	bolt->clipmask |= CONTENTS_TRIGGER;
	//}
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time;
	//bolt->s.pos.trTime = level.time;
	G_ApplyMissileNudge(self, bolt);
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, GRENADE_VELOCITY, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================


/*
=================
fire_bfg
=================
*/
gentity_t *fire_bfg (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "bfg";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BFG;
	bolt->r.ownerNum = self->s.number;
//unlagged - projectile nudge
	// we'll need this for nudging projectiles later
	bolt->s.otherEntityNum = self->s.number;
//unlagged - projectile nudge
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_BFG;
	bolt->splashMethodOfDeath = MOD_BFG_SPLASH;
	bolt->clipmask = MASK_SHOT;
	//if (g_teleMissiles.integer == 1) {
	//	bolt->clipmask |= CONTENTS_TRIGGER;
	//}
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	//bolt->s.pos.trTime = level.time;
	G_ApplyMissileNudge(self, bolt);
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, BFG_VELOCITY, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================


/*
=================
fire_rocket
=================
*/
gentity_t *fire_rocket (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 15000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_ROCKET_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
//unlagged - projectile nudge
	// we'll need this for nudging projectiles later
	bolt->s.otherEntityNum = self->s.number;
//unlagged - projectile nudge
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	//if (g_teleMissiles.integer == 1) {
	//	bolt->clipmask |= CONTENTS_TRIGGER;
	//}
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	//bolt->s.pos.trTime = level.time;
	G_ApplyMissileNudge(self, bolt);
	VectorCopy( start, bolt->s.pos.trBase );
	//VectorScale( dir, 900, bolt->s.pos.trDelta );
	//VectorScale( dir, 1000, bolt->s.pos.trDelta );
	VectorScale( dir, g_rocketSpeed.integer, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

/*
=================
fire_grapple
=================
*/
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*hook;
	VectorNormalize (dir);

	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 20000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_GRAPPLING_HOOK;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_GRAPPLE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = NULL;

//unlagged - grapple
	// we might want this later
	hook->s.otherEntityNum = self->s.number;

	hook->s.pos.trTime = level.time;
	G_ApplyMissileNudge(self, hook);

//unlagged - grapple

	hook->s.pos.trType = TR_LINEAR;
//unlagged - grapple
	//hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
//unlagged - grapple
	hook->s.otherEntityNum = self->s.number; // use to match beam in client
	VectorCopy( start, hook->s.pos.trBase );
	VectorScale( dir, g_swingGrapple.integer ? 2000 : 800, hook->s.pos.trDelta );
	SnapVector( hook->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, hook->r.currentOrigin);

	self->client->hook = hook;

	return hook;
}

/*
=================
fire_nail
=================
*/

gentity_t *fire_nail( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up, int *seed ) {
	gentity_t	*bolt;
	vec3_t		dir;
	vec3_t		end;
	float		r, u, scale;

	bolt = G_Spawn();
	bolt->classname = "nail";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_NAILGUN;
	bolt->r.ownerNum = self->s.number;
//unlagged - projectile nudge
	// we'll need this for nudging projectiles later
	bolt->s.otherEntityNum = self->s.number;
//unlagged - projectile nudge
	bolt->parent = self;
	bolt->damage = 20;
	bolt->methodOfDeath = MOD_NAIL;
	bolt->clipmask = MASK_SHOT;
	//if (g_teleMissiles.integer == 1) {
	//	bolt->clipmask |= CONTENTS_TRIGGER;
	//}
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	G_ApplyMissileNudge(self, bolt);
	VectorCopy( start, bolt->s.pos.trBase );

	//r = random() * M_PI * 2.0f;
	//u = sin(r) * crandom() * NAILGUN_SPREAD * 16;
	//r = cos(r) * crandom() * NAILGUN_SPREAD * 16;
	r = Q_random(seed) * M_PI * 2.0f;
	u = sin(r) * Q_crandom(seed) * NAILGUN_SPREAD * 16;
	r = cos(r) * Q_crandom(seed) * NAILGUN_SPREAD * 16;
	VectorMA( start, 8192 * 16, forward, end);
	VectorMA (end, r, right, end);
	VectorMA (end, u, up, end);
	VectorSubtract( end, start, dir );
	VectorNormalize( dir );

	//scale = 555 + random() * 1800;
	scale = NAIL_BASE_VELOCITY + Q_random(seed) * NAIL_RND_VELOCITY;
	VectorScale( dir, scale, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );

	VectorCopy( start, bolt->r.currentOrigin );

	return bolt;
}	


/*
=================
fire_prox
=================
*/
gentity_t *fire_prox( gentity_t *self, vec3_t start, vec3_t dir ) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "prox mine";
	bolt->nextthink = level.time + 3000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PROX_LAUNCHER;
	bolt->s.eFlags = 0;
	bolt->r.ownerNum = self->s.number;
//unlagged - projectile nudge
	// we'll need this for nudging projectiles later
	bolt->s.otherEntityNum = self->s.number;
//unlagged - projectile nudge
	bolt->parent = self;
	bolt->damage = 0;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_PROXIMITY_MINE;
	bolt->splashMethodOfDeath = MOD_PROXIMITY_MINE;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;
	// count is used to check if the prox mine left the player bbox
	// if count == 1 then the prox mine left the player bbox and can attack to it
	bolt->count = 0;

	//FIXME: we prolly wanna abuse another field
	bolt->s.generic1 = self->client->sess.sessionTeam;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time;
	//bolt->s.pos.trTime = level.time;
	G_ApplyMissileNudge(self, bolt);
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, PROXMINE_VELOCITY, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}
