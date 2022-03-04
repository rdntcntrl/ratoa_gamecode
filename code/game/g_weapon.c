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
// g_weapon.c 
// perform the server side effects of a weapon firing

#include "g_local.h"

static	float	s_quadFactor;
static	vec3_t	forward, right, up;
static	vec3_t	muzzle;

#define NUM_NAILSHOTS 15

/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
	vec3_t v, newv;
	float dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2*dot, dir, newv );

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}


/*
======================================================================

GAUNTLET

======================================================================
*/

void Weapon_Gauntlet( gentity_t *ent ) {

}

/*
===============
CheckGauntletAttack
===============
*/
qboolean CheckGauntletAttack( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			damage;

	// set aiming directions
	AngleVectors (ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePoint ( ent, forward, right, up, muzzle );

	VectorMA (muzzle, 32, forward, end);

	G_DoTimeShiftFor( ent );
		trap_Trace (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);
	G_UndoTimeShiftFor( ent );
	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return qfalse;
	}

	if ( ent->client->noclip ) {
		return qfalse;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// send blood impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
	}

	if ( !traceEnt->takedamage) {
		return qfalse;
	}

	if (ent->client->ps.powerups[PW_QUAD] ) {
		G_AddEvent( ent, EV_POWERUP_QUAD, 0 );
		if (g_quadWhore.integer == 2 && ent->client->pers.quadWhore) {
			s_quadFactor = 0.5;
		} else {
			s_quadFactor = g_quadfactor.value;
		}
	} else {
		s_quadFactor = 1;
	}
	if( ent->client->persistantPowerup && ent->client->persistantPowerup->item && ent->client->persistantPowerup->item->giTag == PW_DOUBLER ) {
		s_quadFactor *= 2;
	}

	if(g_instantgib.integer)
		damage = 500; //High damage in instant gib (normally enough to gib)
	else
		damage = g_gauntDamage.integer * s_quadFactor;
	G_Damage( traceEnt, ent, ent, forward, tr.endpos,
		damage, 0, MOD_GAUNTLET );

	return qtrue;
}


/*
======================================================================

MACHINEGUN

======================================================================
*/

/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating 
into a wall.
======================
*/
//unlagged - attack prediction #3
// moved to q_shared.c
/*
void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int		i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
			v[i] = floor(v[i]);
		} else {
			v[i] = ceil(v[i]);
		}
	}
}
*/
//unlagged - attack prediction #3

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

#define CHAINGUN_SPREAD		600.0
#define MACHINEGUN_SPREAD	200
//#define	MACHINEGUN_DAMAGE	6
#define	MACHINEGUN_DAMAGE	(g_mgDamage.integer)
//#define	MACHINEGUN_TEAM_DAMAGE	5		// wimpier MG in teamplay
#define	MACHINEGUN_TEAM_DAMAGE	(g_mgTeamDamage.integer)		// wimpier MG in teamplay

void Bullet_Fire (gentity_t *ent, float spread, int damage ) {
	trace_t		tr;
	vec3_t		end;
	vec3_t		impactpoint, bouncedir;
	float		r;
	float		u;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			i, passent;

//unlagged - attack prediction #2
	// we have to use something now that the client knows in advance
	int			seed = ent->client->attackTime % 256;
//unlagged - attack prediction #2

	damage *= s_quadFactor;

//unlagged - attack prediction #2
	// this has to match what's on the client
/*
	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * spread * 16;
	r = cos(r) * crandom() * spread * 16;
*/
	r = Q_random(&seed) * M_PI * 2.0f;
	u = sin(r) * Q_crandom(&seed) * spread * 16;
	r = cos(r) * Q_crandom(&seed) * spread * 16;
//unlagged - attack prediction #2

	VectorMA (muzzle, 8192*16, forward, end);
	VectorMA (end, r, right, end);
	VectorMA (end, u, up, end);

	passent = ent->s.number;
	for (i = 0; i < 10; i++) {

//unlagged - backward reconciliation #2
		// backward-reconcile the other clients
		G_DoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

		trap_Trace (&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);
                
//unlagged - backward reconciliation #2
		// put them back
		G_UndoTimeShiftFor( ent );
//unlagged - backward reconciliation #2
                
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			return;
		}

		traceEnt = &g_entities[ tr.entityNum ];

		// snap the endpos to integers, but nudged towards the line
		SnapVectorTowards( tr.endpos, muzzle );

		// send bullet impact
		if ( traceEnt->takedamage && traceEnt->client ) {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
			tent->s.eventParm = traceEnt->s.number;
//unlagged - attack prediction #2
			// we need the client number to determine whether or not to
			// suppress this event
			tent->s.clientNum = ent->s.clientNum;
//unlagged - attack prediction #2
			if( LogAccuracyHit( traceEnt, ent ) ) {
				ent->client->accuracy_hits++;
			}
		} else {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );
			tent->s.eventParm = DirToByte( tr.plane.normal );
//unlagged - attack prediction #2
			// we need the client number to determine whether or not to
			// suppress this event
			tent->s.clientNum = ent->s.clientNum;
//unlagged - attack prediction #2
		}
		tent->s.otherEntityNum = ent->s.number;

		if ( traceEnt->takedamage) {
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					VectorCopy( impactpoint, muzzle );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, muzzle );
					passent = traceEnt->s.number;
				}
				continue;
			}
			else {
                            if(spread == CHAINGUN_SPREAD)
                            {
                                G_Damage( traceEnt, ent, ent, forward, tr.endpos,
					damage, 0, MOD_CHAINGUN);
                                ent->client->accuracy[WP_CHAINGUN][1]++;
                            }
                            else
                            {
				G_Damage( traceEnt, ent, ent, forward, tr.endpos,
					damage, 0, MOD_MACHINEGUN);
                                ent->client->accuracy[WP_MACHINEGUN][1]++;
                            }
			}
		}
		break;
	}
}


/*
======================================================================

BFG

======================================================================
*/

void BFG_Fire ( gentity_t *ent ) {
	gentity_t	*m;

	m = fire_bfg (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	G_ImmediateLaunchMissile(m);

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

SHOTGUN

======================================================================
*/

// DEFAULT_SHOTGUN_SPREAD and DEFAULT_SHOTGUN_COUNT	are in bg_public.h, because
// client predicts same spreads
#define	DEFAULT_SHOTGUN_DAMAGE	10
#define	NEW_SHOTGUN_DAMAGE	9

qboolean ShotgunPellet( vec3_t start, vec3_t end, gentity_t *ent ) {
	trace_t		tr;
	int			damage, i, passent;
	gentity_t	*traceEnt;
	vec3_t		impactpoint, bouncedir;
	vec3_t		tr_start, tr_end;
	qboolean logaccuracyhit = qfalse;

	passent = ent->s.number;
	VectorCopy( start, tr_start );
	VectorCopy( end, tr_end );
	for (i = 0; i < 10; i++) {
		trap_Trace (&tr, tr_start, NULL, NULL, tr_end, passent, MASK_SHOT);
		traceEnt = &g_entities[ tr.entityNum ];

		// send bullet impact
		if (  tr.surfaceFlags & SURF_NOIMPACT ) {
			return qfalse;
		}

		if ( traceEnt->takedamage) {
			damage = (g_newShotgun.integer ? NEW_SHOTGUN_DAMAGE : DEFAULT_SHOTGUN_DAMAGE) * s_quadFactor;
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( tr_start, impactpoint, bouncedir, tr_end );
					VectorCopy( impactpoint, tr_start );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, tr_start );
					passent = traceEnt->s.number;
				}
				continue;
			}
			else {
				if( LogAccuracyHit( traceEnt, ent ) ) {
					logaccuracyhit = qtrue;
				}
				G_Damage( traceEnt, ent, ent, forward, tr.endpos,
					damage, 0, MOD_SHOTGUN);
				if (logaccuracyhit) {
					return qtrue;
				}
			}
		}
		return qfalse;
	}
	return qfalse;
}

// this should match CG_ShotgunPattern
void ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, gentity_t *ent ) {
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;
	//int			oldScore;
	qboolean	hitClient = qfalse;
	qboolean	hitAll = qtrue;

//unlagged - attack prediction #2
	// use this for testing
	//Com_Printf( "Server seed: %d\n", seed );
//unlagged - attack prediction #2

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	//oldScore = ent->client->ps.persistant[PERS_SCORE];

//unlagged - backward reconciliation #2
	// backward-reconcile the other clients
	G_DoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

	// generate the "random" spread pattern
	if (g_newShotgun.integer) {
		for ( i = 0 ; i < 12 ; i++ ) {
			int randomness = 100;
			// creates a pentagon inside a hexagon, with one pellet in the center
			if (i < 5) {
				float t = i*(72.0*M_PI/180.0) + M_PI/5.0;
				r = 300 * 16 * cos(t);
				u = 300 * 16 * sin(t);
			} else if (i < 11) {
				float t = (i-5)*(60.0*M_PI/180.0) + M_PI/6.0;
				r = 600 * 16 * cos(t);
				u = 600 * 16 * sin(t);
			} else {
				r = 0;
				u = 0;
			}
			// add some randomness
			r += Q_crandom( &seed ) * randomness * 16;
			u += Q_crandom( &seed ) * randomness * 16;

			VectorMA( origin, 8192 * 16, forward, end);
			VectorMA (end, r, right, end);
			VectorMA (end, u, up, end);
			if( ShotgunPellet( origin, end, ent ) ) {
				hitClient = qtrue;
				ent->client->consecutive_hits++;
			} else {
				hitAll = qfalse;
			}
		}
	} else {
		for ( i = 0 ; i < DEFAULT_SHOTGUN_COUNT ; i++ ) {
			r = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
			u = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
			VectorMA( origin, 8192 * 16, forward, end);
			VectorMA (end, r, right, end);
			VectorMA (end, u, up, end);
			if( ShotgunPellet( origin, end, ent ) ) {
				hitClient = qtrue;
				ent->client->consecutive_hits++;
			} else {
				hitAll = qfalse;
			}
		}
	}
        if( hitClient ) {
		ent->client->accuracy[WP_SHOTGUN][1]++;
		ent->client->accuracy_hits++;
	}
	if (!hitAll) {
		// make sure we don't get the EAWARD_ACCURACY if we didn't hit all pellets
		ent->client->consecutive_hits = 0;
	} else {
		AwardMessage(ent, EAWARD_FULLSG, ++(ent->client->pers.awardCounts[EAWARD_FULLSG]));
	}

//unlagged - backward reconciliation #2
	// put them back
	G_UndoTimeShiftFor( ent );
//unlagged - backward reconciliation #2
}


void weapon_supershotgun_fire (gentity_t *ent) {
	gentity_t		*tent;

	// send shotgun blast
	tent = G_TempEntity( muzzle, EV_SHOTGUN );
	VectorScale( forward, 4096, tent->s.origin2 );
	SnapVector( tent->s.origin2 );
//Sago: This sound like a bad idea...
//unlagged - attack prediction #2
	// this has to be something the client can predict now
	//tent->s.eventParm = rand() & 255;		// seed for spread pattern
	tent->s.eventParm = ent->client->attackTime % 256; // seed for spread pattern
//unlagged - attack prediction #2
	tent->s.otherEntityNum = ent->s.number;

	ShotgunPattern( tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent );
}


/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire (gentity_t *ent) {
	gentity_t	*m;

	// extra vertical velocity
	forward[2] += 0.2f;
	VectorNormalize( forward );

	m = fire_grenade (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	G_ImmediateLaunchMissile(m);

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire (gentity_t *ent) {
	gentity_t	*m;

	m = fire_rocket (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	G_ImmediateLaunchMissile(m);

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

PLASMA GUN

======================================================================
*/

void Weapon_Plasmagun_Fire (gentity_t *ent) {
	gentity_t	*m;

	m = fire_plasma (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	G_ImmediateLaunchMissile(m);

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

/*
======================================================================

RAILGUN

======================================================================
*/


/*
=================
weapon_railgun_fire
=================
*/
#define	MAX_RAIL_HITS	4
void weapon_railgun_fire (gentity_t *ent) {
	vec3_t		end;
	vec3_t impactpoint, bouncedir;
	trace_t		trace;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			damage;
	int			i;
	int			hits;
	int			unlinked;
	int			passent;
	gentity_t	*unlinkedEntities[MAX_RAIL_HITS];

	//damage = 80 * s_quadFactor;
	damage = g_railgunDamage.integer * s_quadFactor;
	if(g_instantgib.integer)
		damage = 800;

	VectorMA (muzzle, 8192, forward, end);

//unlagged - backward reconciliation #2
	// backward-reconcile the other clients
	G_DoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

	// trace only against the solids, so the railgun will go through people
	unlinked = 0;
	hits = 0;
	passent = ent->s.number;
	do {
		trap_Trace (&trace, muzzle, NULL, NULL, end, passent, MASK_SHOT );
		if ( trace.entityNum >= ENTITYNUM_MAX_NORMAL ) {
			if (g_railJump.integer) {
				G_RailJump( trace.endpos, ent );
			}
			break;
		}
		traceEnt = &g_entities[ trace.entityNum ];
		if ( traceEnt->takedamage ) {
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if ( G_InvulnerabilityEffect( traceEnt, forward, trace.endpos, impactpoint, bouncedir ) ) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					// snap the endpos to integers to save net bandwidth, but nudged towards the line
					SnapVectorTowards( trace.endpos, muzzle );
					// send railgun beam effect
					tent = G_TempEntity( trace.endpos, EV_RAILTRAIL );
					// set player number for custom colors on the railtrail
					tent->s.clientNum = ent->s.clientNum;
					VectorCopy( muzzle, tent->s.origin2 );
					// move origin a bit to come closer to the drawn gun muzzle
					VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
					VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );
					tent->s.eventParm = 255;	// don't make the explosion at the end
					//
					VectorCopy( impactpoint, muzzle );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
			}
			else {
				if( LogAccuracyHit( traceEnt, ent ) ) {
					hits++;
				}
				G_Damage (traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_RAILGUN);
			}
		}
		if ( trace.contents & CONTENTS_SOLID ) {
			if (g_railJump.integer) {
				G_RailJump( trace.endpos, ent );
			}
			break;		// we hit something solid enough to stop the beam
		}
		// unlink this entity, so the next trace will go past it
		trap_UnlinkEntity( traceEnt );
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	} while ( unlinked < MAX_RAIL_HITS );

//unlagged - backward reconciliation #2
	// put them back
	G_UndoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

	// link back in any entities we unlinked
	for ( i = 0 ; i < unlinked ; i++ ) {
		trap_LinkEntity( unlinkedEntities[i] );
	}

	// the final trace endpos will be the terminal point of the rail trail

	// snap the endpos to integers to save net bandwidth, but nudged towards the line
	SnapVectorTowards( trace.endpos, muzzle );

	// send railgun beam effect
	tent = G_TempEntity( trace.endpos, EV_RAILTRAIL );

	// set player number for custom colors on the railtrail
	tent->s.clientNum = ent->s.clientNum;

	VectorCopy( muzzle, tent->s.origin2 );
	// move origin a bit to come closer to the drawn gun muzzle
	VectorMA( tent->s.origin2, 4, right, tent->s.origin2 );
	VectorMA( tent->s.origin2, -1, up, tent->s.origin2 );

	// no explosion at end if SURF_NOIMPACT, but still make the trail
	if ( trace.surfaceFlags & SURF_NOIMPACT ) {
		tent->s.eventParm = 255;	// don't make the explosion at the end
	} else {
		tent->s.eventParm = DirToByte( trace.plane.normal );
	}
	tent->s.clientNum = ent->s.clientNum;

	// give the shooter a reward sound if they have made two railgun hits in a row
	if ( hits == 0 ) {
		// complete miss
		ent->client->accurateCount = 0;
	} else {
		// check for "impressive" reward sound
		ent->client->accurateCount += hits;
		if ( ent->client->accurateCount >= 2 ) {
			ent->client->accurateCount -= 2;
			ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
                        G_LogPrintf( "Award: %i %i: %s gained the %s award!\n", ent->client->ps.clientNum, 2, ent->client->pers.netname, "IMPRESSIVE" );
                        if(!level.hadBots) //There has not been any bots
                            ChallengeMessage(ent,AWARD_IMPRESSIVE);
                        // add the sprite over the player's head
			ent->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
			ent->client->ps.eFlags |= EF_AWARD_IMPRESSIVE;
			ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		}
		ent->client->accuracy_hits++;
                ent->client->accuracy[WP_RAILGUN][1]++;
	}

}


/*
======================================================================

GRAPPLING HOOK

======================================================================
*/

void Weapon_GrapplingHook_Fire (gentity_t *ent)
{
	if ( g_offhandGrapple.integer && ent->client->hookhasbeenfired ) {
		AngleVectors (ent->client->ps.viewangles, forward, right, up);
		CalcMuzzlePoint ( ent, forward, right, up, muzzle );
		G_AddEvent( ent, EV_OFFHAND_GRAPPLE, 0 );
	}
	if (!ent->client->fireHeld && !ent->client->hook)
		fire_grapple (ent, muzzle, forward);

	ent->client->fireHeld = qtrue;

	G_ImmediateLaunchMissile(ent);
}

void Weapon_HookFree (gentity_t *ent)
{
	ent->parent->client->hook = NULL;
	ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	G_FreeEntity( ent );
}

void Weapon_HookThink (gentity_t *ent)
{
	if (ent->enemy) {
		vec3_t v, oldorigin;

		VectorCopy(ent->r.currentOrigin, oldorigin);
		v[0] = ent->enemy->r.currentOrigin[0] + (ent->enemy->r.mins[0] + ent->enemy->r.maxs[0]) * 0.5;
		v[1] = ent->enemy->r.currentOrigin[1] + (ent->enemy->r.mins[1] + ent->enemy->r.maxs[1]) * 0.5;
		v[2] = ent->enemy->r.currentOrigin[2] + (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5;
		SnapVectorTowards( v, oldorigin );	// save net bandwidth

		G_SetOrigin( ent, v );
	}

	VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);
}

/*
======================================================================

LIGHTNING GUN

======================================================================
*/

void Weapon_LightningFire( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
	vec3_t impactpoint, bouncedir;
	gentity_t	*traceEnt, *tent;
	int			damage, i, passent;

	damage = g_lgDamage.integer * s_quadFactor;

	passent = ent->s.number;
	for (i = 0; i < 10; i++) {
		VectorMA( muzzle, LIGHTNING_RANGE, forward, end );

//Sago: I'm not sure this should recieve backward reconciliation. It is not a real instant hit weapon, it can normally be dogded
//unlagged - backward reconciliation #2
	// backward-reconcile the other clients
	G_DoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

		trap_Trace( &tr, muzzle, NULL, NULL, end, passent, MASK_SHOT );

//unlagged - backward reconciliation #2
	// put them back
	G_UndoTimeShiftFor( ent );
//unlagged - backward reconciliation #2

		// if not the first trace (the lightning bounced of an invulnerability sphere)
		if (i) {
			// add bounced off lightning bolt temp entity
			// the first lightning bolt is a cgame only visual
			//
			tent = G_TempEntity( muzzle, EV_LIGHTNINGBOLT );
			VectorCopy( tr.endpos, end );
			SnapVector( end );
			VectorCopy( end, tent->s.origin2 );
		}
		if ( tr.entityNum == ENTITYNUM_NONE ) {
			return;
		}

		traceEnt = &g_entities[ tr.entityNum ];

		if ( traceEnt->takedamage) {
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					VectorCopy( impactpoint, muzzle );
					VectorSubtract( end, impactpoint, forward );
					VectorNormalize(forward);
					// the player can hit him/herself with the bounced lightning
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, muzzle );
					passent = traceEnt->s.number;
				}
				continue;
			}
			else {
				if( LogAccuracyHit( traceEnt, ent ) ) {
					ent->client->accuracy_hits++;
					ent->client->accuracy[WP_LIGHTNING][1]++;
				}
				G_Damage( traceEnt, ent, ent, forward, tr.endpos,
					damage, 0, MOD_LIGHTNING);
			}
		}

		if ( traceEnt->takedamage && traceEnt->client ) {
			tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
			tent->s.otherEntityNum = traceEnt->s.number;
			tent->s.eventParm = DirToByte( tr.plane.normal );
			tent->s.weapon = ent->s.weapon;
			tent->s.clientNum = ent->s.clientNum;
		} else if ( !( tr.surfaceFlags & SURF_NOIMPACT ) ) {
			tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
			tent->s.eventParm = DirToByte( tr.plane.normal );
			tent->s.weapon = ent->s.weapon;
			tent->s.clientNum = ent->s.clientNum;
		}

		break;
	}
}

/*
======================================================================

NAILGUN

======================================================================
*/

void Weapon_Nailgun_Fire (gentity_t *ent) {
	gentity_t	*m;
	int			count;
	int seed;

	if (ent->client) {
		seed = ent->client->attackTime;
	} else {
		seed = rand();
	}
	seed %= 256;

	for( count = 0; count < NUM_NAILSHOTS; count++ ) {
		m = fire_nail (ent, muzzle, forward, right, up, &seed );
		m->damage *= s_quadFactor;
		m->splashDamage *= s_quadFactor;

		G_ImmediateLaunchMissile(m);
	}

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


/*
======================================================================

PROXIMITY MINE LAUNCHER

======================================================================
*/

void weapon_proxlauncher_fire (gentity_t *ent) {
	gentity_t	*m;

	// extra vertical velocity
	forward[2] += 0.2f;
	VectorNormalize( forward );

	m = fire_prox (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	G_ImmediateLaunchMissile(m);

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}


//======================================================================


/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if ( !target->takedamage 
			|| target == attacker
			|| !target->client
			|| !attacker->client
			|| target->client->ps.stats[STAT_HEALTH] <= 0
			|| OnSameTeam( target, attacker )) {
		return qfalse;
	}
	return qtrue;
}

void G_CheckAccuracyAward( gentity_t *ent, int old_accuracy_hits) {
	int requiredhits = 20;
	if (ent->client->accuracy_hits - old_accuracy_hits <= 0) {
		ent->client->consecutive_hits = 0;
		return;
	}
	switch (ent->s.weapon) {
		case WP_LIGHTNING:
			ent->client->consecutive_hits++;
			break;
		case WP_MACHINEGUN:
			requiredhits = 15;
			ent->client->consecutive_hits++;
			break;
		case WP_RAILGUN:
			requiredhits = 10;
			ent->client->consecutive_hits++;
			break;
		case WP_CHAINGUN:
			requiredhits = 20;
			ent->client->consecutive_hits++;
			break;
		case WP_SHOTGUN:
			// shotcun hits are already counted in ShotgunPattern()
			break;
		default:
			return;
	}

	if (ent->client->consecutive_hits >= requiredhits) {
		ent->client->consecutive_hits = 0;
		// all hits, give an award
		AwardMessage(ent, EAWARD_ACCURACY, ++(ent->client->pers.awardCounts[EAWARD_ACCURACY]));
	}
}


/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	//SnapVector( muzzlePoint );
}

/*
===============
CalcMuzzlePointOrigin

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePointOrigin ( gentity_t *ent, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	//SnapVector( muzzlePoint );
}



/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent ) {
	int old_accuracy_hits;
	//Make people drop out of follow mode (this should be moved, so people can change betwean players.)
	if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
		StopFollowing( ent );
		return;
	}

	if (ent->client->ps.powerups[PW_QUAD] ) {
		if (g_quadWhore.integer == 2 && ent->client->pers.quadWhore) {
			s_quadFactor = 0.5;
		} else {
			s_quadFactor = g_quadfactor.value;
		}
	} else {
		s_quadFactor = 1;
	}
	if( ent->client->persistantPowerup && ent->client->persistantPowerup->item && ent->client->persistantPowerup->item->giTag == PW_DOUBLER ) {
		s_quadFactor *= 2;
	}

        if (ent->client->spawnprotected)
            ent->client->spawnprotected = qfalse;

	// for EAWARD_ACCURACY
	if (ent->s.weapon != ent->client->consecutive_hits_weapon) {
		ent->client->consecutive_hits_weapon = ent->s.weapon;
		ent->client->consecutive_hits = 0;
	}
	old_accuracy_hits = ent->client->accuracy_hits;

	// track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked
	if( ent->s.weapon != WP_GRAPPLING_HOOK && ent->s.weapon != WP_GAUNTLET ) {
		if( ent->s.weapon == WP_NAILGUN ) {
			ent->client->accuracy_shots += NUM_NAILSHOTS;
                        //ent->client->accuracy[WP_NAILGUN][0]++;
                        ent->client->accuracy[WP_NAILGUN][0] += NUM_NAILSHOTS;
		} else {
			ent->client->accuracy_shots++;
                        ent->client->accuracy[ent->s.weapon][0]++;
		}
	}


	// set aiming directions
	AngleVectors (ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePointOrigin ( ent, ent->client->oldOrigin, forward, right, up, muzzle );

	// fire the specific weapon
	switch( ent->s.weapon ) {
	case WP_GAUNTLET:
		Weapon_Gauntlet( ent );
		break;
	case WP_LIGHTNING:
		Weapon_LightningFire( ent );
		break;
	case WP_SHOTGUN:
		weapon_supershotgun_fire( ent );
		break;
	case WP_MACHINEGUN:
		if ( g_gametype.integer != GT_TEAM ) {
			Bullet_Fire( ent, MACHINEGUN_SPREAD, MACHINEGUN_DAMAGE );
		} else {
			Bullet_Fire( ent, MACHINEGUN_SPREAD, MACHINEGUN_TEAM_DAMAGE );
		}
		break;
	case WP_GRENADE_LAUNCHER:
		weapon_grenadelauncher_fire( ent );
		break;
	case WP_ROCKET_LAUNCHER:
		Weapon_RocketLauncher_Fire( ent );
		break;
	case WP_PLASMAGUN:
		Weapon_Plasmagun_Fire( ent );
		break;
	case WP_RAILGUN:
		weapon_railgun_fire( ent );
		break;
	case WP_BFG:
		BFG_Fire( ent );
		break;
	case WP_GRAPPLING_HOOK:
		Weapon_GrapplingHook_Fire( ent );
		break;
	case WP_NAILGUN:
		Weapon_Nailgun_Fire( ent );
		break;
	case WP_PROX_LAUNCHER:
		weapon_proxlauncher_fire( ent );
		break;
	case WP_CHAINGUN:
		Bullet_Fire( ent, CHAINGUN_SPREAD, MACHINEGUN_DAMAGE );
		break;
	default:
// FIXME		G_Error( "Bad ent->s.weapon" );
		break;
	}

	G_CheckAccuracyAward(ent, old_accuracy_hits);
}


/*
===============
KamikazeRadiusDamage
===============
*/
static void KamikazeRadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius ) {
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;

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

		if (!ent->takedamage) {
			continue;
		}

		// dont hit things we have already hit
		if( ent->kamikazeTime > level.time ) {
			continue;
		}

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

//		if( CanDamage (ent, origin) ) {
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_Damage( ent, NULL, attacker, dir, origin, damage, DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE );
			ent->kamikazeTime = level.time + 3000;
//		}
	}
}

/*
===============
KamikazeShockWave
===============
*/
static void KamikazeShockWave( vec3_t origin, gentity_t *attacker, float damage, float push, float radius ) {
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;

	if ( radius < 1 )
		radius = 1;

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		// dont hit things we have already hit
		if( ent->kamikazeShockTime > level.time ) {
			continue;
		}

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

//		if( CanDamage (ent, origin) ) {
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			dir[2] += 24;
			G_Damage( ent, NULL, attacker, dir, origin, damage, DAMAGE_RADIUS|DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE );
			//
			dir[2] = 0;
			VectorNormalize(dir);
			if ( ent->client ) {
				ent->client->ps.velocity[0] = dir[0] * push;
				ent->client->ps.velocity[1] = dir[1] * push;
				ent->client->ps.velocity[2] = 100;
			}
			ent->kamikazeShockTime = level.time + 3000;
//		}
	}
}

/*
===============
KamikazeDamage
===============
*/
static void KamikazeDamage( gentity_t *self ) {
	int i;
	float t;
	gentity_t *ent;
	vec3_t newangles;

	self->count += 100;

	if (self->count >= KAMI_SHOCKWAVE_STARTTIME) {
		// shockwave push back
		t = self->count - KAMI_SHOCKWAVE_STARTTIME;
		KamikazeShockWave(self->s.pos.trBase, self->activator, 25, 400,	(int) (float) t * KAMI_SHOCKWAVE_MAXRADIUS / (KAMI_SHOCKWAVE_ENDTIME - KAMI_SHOCKWAVE_STARTTIME) );
	}
	//
	if (self->count >= KAMI_EXPLODE_STARTTIME) {
		// do our damage
		t = self->count - KAMI_EXPLODE_STARTTIME;
		KamikazeRadiusDamage( self->s.pos.trBase, self->activator, 400,	(int) (float) t * KAMI_BOOMSPHERE_MAXRADIUS / (KAMI_IMPLODE_STARTTIME - KAMI_EXPLODE_STARTTIME) );
	}

	// either cycle or kill self
	if( self->count >= KAMI_SHOCKWAVE_ENDTIME ) {
		G_FreeEntity( self );
		return;
	}
	self->nextthink = level.time + 100;

	// add earth quake effect
	newangles[0] = crandom() * 2;
	newangles[1] = crandom() * 2;
	newangles[2] = 0;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ent = &g_entities[i];
		if (!G_InUse(ent))
			continue;
		if (!ent->client)
			continue;

		if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE) {
			ent->client->ps.velocity[0] += crandom() * 120;
			ent->client->ps.velocity[1] += crandom() * 120;
			ent->client->ps.velocity[2] = 30 + random() * 25;
		}

		ent->client->ps.delta_angles[0] += ANGLE2SHORT(newangles[0] - self->movedir[0]);
		ent->client->ps.delta_angles[1] += ANGLE2SHORT(newangles[1] - self->movedir[1]);
		ent->client->ps.delta_angles[2] += ANGLE2SHORT(newangles[2] - self->movedir[2]);
	}
	VectorCopy(newangles, self->movedir);
}

/*
===============
G_StartKamikaze
===============
*/
void G_StartKamikaze( gentity_t *ent ) {
	gentity_t	*explosion;
	gentity_t	*te;
	vec3_t		snapped;

	// start up the explosion logic
	explosion = G_Spawn();

	explosion->s.eType = ET_EVENTS + EV_KAMIKAZE;
	explosion->eventTime = level.time;

	if ( ent->client ) {
		VectorCopy( ent->s.pos.trBase, snapped );
	}
	else {
		VectorCopy( ent->activator->s.pos.trBase, snapped );
	}
	SnapVector( snapped );		// save network bandwidth
	G_SetOrigin( explosion, snapped );

	explosion->classname = "kamikaze";
	explosion->s.pos.trType = TR_STATIONARY;

	explosion->kamikazeTime = level.time;

	explosion->think = KamikazeDamage;
	explosion->nextthink = level.time + 100;
	explosion->count = 0;
	VectorClear(explosion->movedir);

	trap_LinkEntity( explosion );

	if (ent->client) {
		//
		explosion->activator = ent;
		//
		ent->s.eFlags &= ~EF_KAMIKAZE;
		// nuke the guy that used it
		G_Damage( ent, ent, ent, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_KAMIKAZE );
	}
	else {
		if ( !strcmp(ent->activator->classname, "bodyque") ) {
			explosion->activator = &g_entities[ent->activator->r.ownerNum];
		}
		else {
			explosion->activator = ent->activator;
		}
	}

	// play global sound at all clients
	te = G_TempEntity(snapped, EV_GLOBAL_TEAM_SOUND );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = GTS_KAMIKAZE;
}

/*
 * Similar to CanDamage(), but more accurately checks visibility
 */
qboolean G_IsVisible (gentity_t *targ, vec3_t origin) {
	vec3_t	dest;
	trace_t	tr;
	vec3_t	midpoint;
	float xd, yd, zd;

	xd = (targ->r.absmax[0] - targ->r.absmin[0])/2.0;
	yd = (targ->r.absmax[1] - targ->r.absmin[1])/2.0;
	zd = (targ->r.absmax[2] - targ->r.absmin[2])/2.0;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd (targ->r.absmin, targ->r.absmax, midpoint);
	VectorScale (midpoint, 0.5, midpoint);

	VectorCopy (midpoint, dest);
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] += xd;
	dest[1] += yd;
	dest[2] += zd;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] -= xd;
	dest[1] += yd;
	dest[2] += zd;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] += xd;
	dest[1] -= yd;
	dest[2] += zd;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] -= xd;
	dest[1] -= yd;
	dest[2] += zd;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] += xd;
	dest[1] += yd;
	dest[2] -= zd;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] -= xd;
	dest[1] += yd;
	dest[2] -= zd;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] += xd;
	dest[1] -= yd;
	dest[2] -= zd;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] -= xd;
	dest[1] -= yd;
	dest[2] -= zd;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;


	return qfalse;
}

float G_PingCheckFOVEntity( gentity_t *ent, vec3_t muzzle, vec3_t normDir ) {
	vec3_t targetDir;

	VectorSubtract(ent->r.currentOrigin, muzzle, targetDir);
	VectorNormalize(targetDir);
	return DotProduct(targetDir, normDir);
}

powerup_t G_PingEntityPowerup( gentity_t *pingPlayer, gentity_t *ent ) {
	if (ent->client 
			&& ent->client->sess.sessionTeam != pingPlayer->client->sess.sessionTeam
			&& ent->client->ps.pm_type != PM_DEAD
		  )  {
		if (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION) {
			if (ent->client->ps.powerups[PW_REDFLAG]) {
				return PW_REDFLAG;
			} else if (ent->client->ps.powerups[PW_BLUEFLAG]) {
				return PW_BLUEFLAG;
			}
		} else if (g_gametype.integer == GT_1FCTF) {
			if (ent->client->ps.powerups[PW_NEUTRALFLAG]) {
				return PW_NEUTRALFLAG;
			}
		}
	} else if (ent->s.eType == ET_ITEM
			&& ent->item
			&& ent->item->giType == IT_TEAM
			&& !(ent->s.eFlags & EF_NODRAW)
			&& (((g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION)
					&& (ent->item->giTag == PW_REDFLAG || ent->item->giTag == PW_BLUEFLAG))
				|| (g_gametype.integer == GT_1FCTF && ent->item->giTag == PW_NEUTRALFLAG)
			   )) {
		return ent->item->giTag;
	}  

	return PW_NONE;
}

locationping_t G_PingFindEnemiesInFOV( gentity_t *pingPlayer, vec3_t muzzle, vec3_t normDir, vec3_t out ) {
	float minDotVal;
	float maxDotVal;
	float dv;
	float blueFlagDv = 0;
	float redFlagDv = 0;
	gentity_t *target = NULL;
	gentity_t *blueFlagTarget = NULL;
	gentity_t *redFlagTarget = NULL;
	gentity_t *neutralFlagTarget = NULL;
	int i;
	gentity_t *ent;
	locationping_t pingType = LOCPING_PING;

	if (g_pingLocationFov.value <= 0) {
		return pingType;
	}

	minDotVal = cos(g_pingLocationFov.value/2.0 * M_PI/180.0);
	maxDotVal = minDotVal;

	G_DoTimeShiftFor( pingPlayer );

	for ( i = 0; i < level.numPlayingClients; ++i) {
		ent = g_entities + level.sortedClients[i];
		if (ent->client->sess.sessionTeam == pingPlayer->client->sess.sessionTeam
					|| ent->client->ps.pm_type == PM_DEAD
					|| ent->client->isEliminated
					) {
			continue;
		}
		dv = G_PingCheckFOVEntity(ent, muzzle, normDir);
		if (dv <= minDotVal || !G_IsVisible(ent, muzzle)) {
			continue;
		}
		switch (G_PingEntityPowerup(pingPlayer, ent)) {
			case PW_REDFLAG:
				redFlagTarget = ent;
				redFlagDv = dv;
				break;
			case PW_BLUEFLAG:
				blueFlagTarget = ent;
				blueFlagDv = dv;
				break;
			case PW_NEUTRALFLAG:
				neutralFlagTarget = ent;
				break;
			default:
				break;
		}
		if (dv <= maxDotVal) {
			continue;
		}
		target = ent;
		maxDotVal = dv;
	}


	if (((g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION) 
				&& (Team_GetFlagStatus(TEAM_RED) == FLAG_DROPPED || Team_GetFlagStatus(TEAM_BLUE) == FLAG_DROPPED))
			|| (g_gametype.integer == GT_1FCTF && Team_GetFlagStatus(TEAM_FREE) == FLAG_DROPPED)) {
		// check for dropped flags
		powerup_t powerup;

		for ( i = MAX_CLIENTS; i < level.num_entities; ++i) {
			ent = g_entities + i;
			if (!G_InUse(ent) ) {
				continue;
			}
			powerup = G_PingEntityPowerup(pingPlayer, ent);
			if (powerup == PW_NONE) {
				continue;
			}
			dv = G_PingCheckFOVEntity(ent, muzzle, normDir);
			if (dv <= minDotVal || !G_IsVisible(ent, muzzle)) {
				continue;
			}
			switch (powerup) {
				case PW_REDFLAG:
					redFlagTarget = ent;
					redFlagDv = dv;
					break;
				case PW_BLUEFLAG:
					blueFlagTarget = ent;
					blueFlagDv = dv;
					break;
				case PW_NEUTRALFLAG:
					neutralFlagTarget = ent;
					break;
				default:
					break;
			}
		}
	}

	
	if (neutralFlagTarget) {
		target = neutralFlagTarget;
		pingType = LOCPING_NEUTRALFLAG;
	} else if (blueFlagTarget && redFlagTarget) {
		if (blueFlagDv > redFlagDv) {
			target = blueFlagTarget;
			pingType = LOCPING_BLUEFLAG;
		} else {
			target = redFlagTarget;
			pingType = LOCPING_REDFLAG;
		}
	} else if (blueFlagTarget) {
		target = blueFlagTarget;
		pingType = LOCPING_BLUEFLAG;
	} else if (redFlagTarget) {
		target = redFlagTarget;
		pingType = LOCPING_REDFLAG;
	} else if (target) {
		pingType = LOCPING_ENEMY;
	}

	if (target != NULL) {
		VectorCopy(target->r.currentOrigin, out);
		out[2] += target->r.mins[2];
	}

	G_UndoTimeShiftFor( pingPlayer );

	return pingType;
}

locationping_t G_PingFindEnemies( gentity_t *pingPlayer, vec3_t muzzle, vec3_t origin, float radius ) {
	float		dist;
	gentity_t	*ent;
	int		entityList[MAX_GENTITIES];
	int		numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	int		i, e;
	powerup_t powerup = PW_NONE;
	qboolean foundEnemy = qfalse;

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	G_DoTimeShiftFor( pingPlayer );

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];
		powerup = G_PingEntityPowerup(pingPlayer, ent);

		if (powerup == PW_NONE 
				&& (!ent->client
					|| ent->client->sess.sessionTeam == pingPlayer->client->sess.sessionTeam
					|| ent->client->ps.pm_type == PM_DEAD)) {
			continue;
		}


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

		if( G_IsVisible (ent, muzzle)) {
			foundEnemy = qtrue;
			if ((g_gametype.integer != GT_CTF && g_gametype.integer != GT_CTF_ELIMINATION && g_gametype.integer != GT_1FCTF) || powerup != PW_NONE) {
				break;
			}
			// make sure we continue in CTF modes so we are sure to find the flag(s)
		}
	}

	G_UndoTimeShiftFor( pingPlayer );

	if (foundEnemy) {
		switch (powerup) {
			case PW_REDFLAG:
				return LOCPING_REDFLAG;
			case PW_BLUEFLAG:
				return LOCPING_BLUEFLAG;
			case PW_NEUTRALFLAG:
				return LOCPING_NEUTRALFLAG;
			default:
				return LOCPING_ENEMY;
		}
	}
	return LOCPING_PING;
}

void G_PingLocation( gentity_t *ent, locationping_t pingtype ) {
	vec3_t forward, right, up, muzzle, end;
	trace_t trace;
	gentity_t *ping;

	if (ent->client->ps.pm_type == PM_DEAD) {
		if (pingtype == LOCPING_PING) {
			pingtype = G_PingFindEnemies(ent, ent->r.currentOrigin, ent->r.currentOrigin, g_pingLocationRadius.integer);
			switch (pingtype) {
				case LOCPING_REDFLAG:
				case LOCPING_BLUEFLAG:
				case LOCPING_NEUTRALFLAG:
					break;
				default:
					pingtype = LOCPING_DEAD;
			}
		}
		ping = G_TempEntity(ent->r.currentOrigin, EV_PING_LOCATION);
	} else {
		vec3_t pingOrigin = { 0, 0, 0 };
		AngleVectors (ent->client->ps.viewangles, forward, right, up);
		CalcMuzzlePointOrigin ( ent, ent->client->oldOrigin, forward, right, up, muzzle );

		if (pingtype == LOCPING_PING) {
			pingtype = G_PingFindEnemiesInFOV(ent, muzzle, forward, pingOrigin);
		}

		if (pingtype != LOCPING_PING && pingtype != LOCPING_WARN) {
			// found enemy / flag within FOV, ping that
			ping = G_TempEntity(pingOrigin, EV_PING_LOCATION);
		} else {
			// trace until we hit something, ping that
			VectorMA(muzzle, 16258, forward, end);
			trap_Trace(&trace, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);
			if (trace.entityNum == ENTITYNUM_NONE || trace.entityNum > ENTITYNUM_MAX_NORMAL) {
				return;
			}
			ping = G_TempEntity(trace.endpos, EV_PING_LOCATION);
		}

	}


	// tell clients who pinged
	ping->s.otherEntityNum = ent->s.number;

	if (pingtype == LOCPING_PING) {
		ping->s.generic1 = G_PingFindEnemies(ent, muzzle, trace.endpos, g_pingLocationRadius.integer);
	} else {
		ping->s.generic1 = pingtype;
	}

	ping->r.svFlags |= (SVF_CLIENTMASK | SVF_BROADCAST);
	ping->r.singleClient = G_TeamClientMask(ent->client->sess.sessionTeam);

	Com_Printf("Ping by client %i, type = %i\n", ent->s.number, ping->s.generic1);

	trap_LinkEntity( ping );
}

