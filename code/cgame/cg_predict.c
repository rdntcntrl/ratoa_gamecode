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
// cg_predict.c -- this file generates cg.predictedPlayerState by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"

static	pmove_t		cg_pmove;

static	int			cg_numSolidEntities;
 // MAX_ENTITIES_IN_SNAPSHOT + 1 to make sure predictedPlayerEntity fits in there
static	centity_t	*cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT+1];
static	int			cg_numTriggerEntities;
static	centity_t	*cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void CG_BuildSolidList( void ) {
	int			i;
	centity_t	*cent;
	snapshot_t	*snap;
	entityState_t	*ent;

	cg_numSolidEntities = 0;
	cg_numTriggerEntities = 0;

	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
		snap = cg.nextSnap;
	} else {
		snap = cg.snap;
	}

	if (cg.validPPS 
			&& cg.predictedPlayerState.pm_type == PM_NORMAL 
			&& cg.predictedPlayerEntity.currentState.eType == ET_PLAYER
			&& cg.predictedPlayerEntity.currentState.solid) {
		// eType is ET_INVISIBLE in case we are not in-game (spectating)
		cg_solidEntities[cg_numSolidEntities] = &cg.predictedPlayerEntity;
		cg_numSolidEntities++;
	}
	for ( i = 0 ; i < snap->numEntities ; i++ ) {
		cent = &cg_entities[ snap->entities[ i ].number ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM || ent->eType == ET_PUSH_TRIGGER || ent->eType == ET_TELEPORT_TRIGGER ) {
			cg_triggerEntities[cg_numTriggerEntities] = cent;
			cg_numTriggerEntities++;
			continue;
		}

		if ( cent->nextState.solid ) {
			cg_solidEntities[cg_numSolidEntities] = cent;
			cg_numSolidEntities++;
			continue;
		}
	}
}

void CG_EncodePlayerBBox( pmove_t *pm, entityState_t *ent) {
	int i, j, k;

	ent->solid = 0;
	if (!pm || !pm->ps ||pm->ps->pm_type != PM_NORMAL) {
		return;
	}

	// Encoding logic from trap_LinkEntity
	
	// assume that x/y are equal and symetric
	i = pm->maxs[0];
	if (i<1)
		i = 1;
	if (i>255)
		i = 255;

	// z is not symetric
	j = (-pm->mins[2]);
	if (j<1)
		j = 1;
	if (j>255)
		j = 255;

	// and z maxs can be negative...
	k = (pm->maxs[2]+32);
	if (k<1)
		k = 1;
	if (k>255)
		k = 255;

	ent->solid = (k<<16) | (j<<8) | i;
}

/*
====================
CG_ClipMoveToEntities

====================
*/
static void CG_ClipMoveToEntities ( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
							int skipNumber, int mask, trace_t *tr ) {
	int			i, x, zd, zu;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t 	cmodel;
	vec3_t		bmins, bmaxs;
	vec3_t		origin, angles;
	centity_t	*cent;

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];
		ent = &cent->currentState;

		if ( ent->number == skipNumber ) {
			continue;
		}

		if ( ent->solid == SOLID_BMODEL ) {
			// special value for bmodel
			cmodel = trap_CM_InlineModel( ent->modelindex );
			VectorCopy( cent->lerpAngles, angles );
			BG_EvaluateTrajectory( &cent->currentState.pos, cg.physicsTime, origin );
		} else {
			// encoded bbox
			x = (ent->solid & 255);
			zd = ((ent->solid>>8) & 255);
			zu = ((ent->solid>>16) & 255) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			cmodel = trap_CM_TempBoxModel( bmins, bmaxs );
			VectorCopy( vec3_origin, angles );
			VectorCopy( cent->lerpOrigin, origin );
		}


		trap_CM_TransformedBoxTrace ( &trace, start, end,
			mins, maxs, cmodel,  mask, origin, angles);

		if (trace.allsolid || trace.fraction < tr->fraction) {
			trace.entityNum = ent->number;
			*tr = trace;
		} else if (trace.startsolid) {
			tr->startsolid = qtrue;
		}
		if ( tr->allsolid ) {
			return;
		}
	}
}

/*
================
CG_Trace
================
*/
void	CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, 
					 int skipNumber, int mask ) {
	trace_t	t;

	trap_CM_BoxTrace ( &t, start, end, mins, maxs, 0, mask);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities (start, mins, maxs, end, skipNumber, mask, &t);

	*result = t;
}

/*
================
CG_PointContents
================
*/
int		CG_PointContents( const vec3_t point, int passEntityNum ) {
	int			i;
	entityState_t	*ent;
	centity_t	*cent;
	clipHandle_t cmodel;
	int			contents;

	contents = trap_CM_PointContents (point, 0);

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];

		ent = &cent->currentState;

		if ( ent->number == passEntityNum ) {
			continue;
		}

		if (ent->solid != SOLID_BMODEL) { // special value for bmodel
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		contents |= trap_CM_TransformedPointContents( point, cmodel, cent->lerpOrigin, cent->lerpAngles );
	}

	return contents;
}


/*
========================
CG_InterpolatePlayerState

Generates cg.predictedPlayerState by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
static void CG_InterpolatePlayerState( qboolean grabAngles ) {
	float			f;
	int				i;
	playerState_t	*out;
	snapshot_t		*prev, *next;

	out = &cg.predictedPlayerState;
	prev = cg.snap;
	next = cg.nextSnap;

	*out = cg.snap->ps;

	// if we are still allowing local input, short circuit the view angles
	if ( grabAngles ) {
		usercmd_t	cmd;
		int			cmdNum;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd( cmdNum, &cmd );

		PM_UpdateViewAngles( out, &cmd );
	}

	// if the next frame is a teleport, we can't lerp to it
	if ( cg.nextFrameTeleport ) {
		return;
	}

	if ( !next || next->serverTime <= prev->serverTime ) {
		return;
	}

	f = (float)( cg.time - prev->serverTime ) / ( next->serverTime - prev->serverTime );

	i = next->ps.bobCycle;
	if ( i < prev->ps.bobCycle ) {
		i += 256;		// handle wraparound
	}
	out->bobCycle = prev->ps.bobCycle + f * ( i - prev->ps.bobCycle );

	for ( i = 0 ; i < 3 ; i++ ) {
		out->origin[i] = prev->ps.origin[i] + f * (next->ps.origin[i] - prev->ps.origin[i] );
		if ( !grabAngles ) {
			out->viewangles[i] = LerpAngle( 
				prev->ps.viewangles[i], next->ps.viewangles[i], f );
		}
		out->velocity[i] = prev->ps.velocity[i] + 
			f * (next->ps.velocity[i] - prev->ps.velocity[i] );
	}

}

qboolean CG_ItemPredictionDangerous(centity_t *item) {
	if (item->currentState.time2 > 0 
			&& cg.time + CG_ReliablePing() + 10 >= item->currentState.time2) {
		// item is about to disappear, so don't predict it
		return qtrue;
	}
	if (!cg_predictItemsNearPlayers.integer) {
		int i;
		centity_t *player;
		for (i = 0; i < MAX_CLIENTS; ++i) {
			if (i == cg.clientNum) {
				continue;
			}
			player = &cg_entities[i];

			if (!player->currentValid || player->currentState.eType != ET_PLAYER) {
				continue;
			}

			//CG_Printf("porigin = %f %f %f, iorigin = %f %f %f \nDistance = %f\n", 
			//		player->lerpOrigin[0],
			//		player->lerpOrigin[1],
			//		player->lerpOrigin[2],
			//		item->currentState.origin[0],
			//		item->currentState.origin[1],
			//		item->currentState.origin[2],
			//		Distance(item->currentState.origin, player->currentState.origin));

			if (Distance(item->currentState.origin, player->lerpOrigin) > 192) {
				continue;
			}

			// TODO: trace maybe?

			// player is close by, don't predict
			return qtrue;

		}
	}
	return qfalse;
}

/*
===================
CG_TouchItem
===================
*/
static void CG_TouchItem( centity_t *cent ) {
	gitem_t		*item;
	//For instantgib
	//qboolean	canBePicked;

	//if(cgs.gametype == GT_ELIMINATION || cgs.gametype == GT_LMS)
	//	return; //No weapon pickup in elimination
	
	// items can only be picked up during the round
	if (BG_IsElimGT(cgs.gametype) && cgs.roundStartTime > cg.time && cg.warmup == 0) {
		return;
	}

	//normally we can
	//canBePicked = qtrue;

	//// TODO: is this even needed? 
	////But in instantgib, rocket arena, and CTF_ELIMINATION we normally can't:
	//if(cgs.nopickup || cgs.gametype == GT_CTF_ELIMINATION)
	//	canBePicked = qfalse;

	if ( !cg_predictItems.integer ) {
		return;
	}
	if ( !BG_PlayerTouchesItem( &cg.predictedPlayerState, &cent->currentState, cg.time, (cgs.ratFlags & RAT_EASYPICKUP) ? 1 : 0) ) {
		return;
	}

	// never pick an item up twice in a prediction
	if ( cent->miscTime == cg.time ) {
		return;
	}

	if ( !BG_CanItemBeGrabbed( cgs.gametype, &cent->currentState, &cg.predictedPlayerState ) ) {
		return;		// can't hold it
	}

	if (cent->currentState.time && cent->currentState.time  > cg.time) {
		// item was recently dropped by player using \drop, prevent
		// pickup (so dropping player doesn't re-pickup it immediately
		return;
	}

	item = &bg_itemlist[ cent->currentState.modelindex ];

	if (item->giType == IT_TEAM) {
		//// Special case for flags.  
		//// We don't predict touching our own flag
		//if( cgs.gametype == GT_1FCTF ) {
		//	if( item->giTag == PW_NEUTRALFLAG ) {
		//		canBePicked = qtrue;
		//	}
		//}
		//	//if( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION || cgs.gametype == GT_HARVESTER ) {
		//	//	if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_RED &&
		//	//		item->giTag == PW_REDFLAG) {
		//	//		return;
		//	//	}
		//	//	if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_BLUE &&
		//	//		item->giTag == PW_BLUEFLAG) {
		//	//		return;
		//	//	}
		//	//Even in instantgib, we can predict our enemy flag
		//	if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_RED &&
		//			item->giTag == PW_BLUEFLAG && (!(cgs.elimflags&EF_ONEWAY) || cgs.attackingTeam == TEAM_RED)) {
		//		canBePicked = qtrue;
		//	}
		//	if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_BLUE &&
		//			item->giTag == PW_REDFLAG && (!(cgs.elimflags&EF_ONEWAY) || cgs.attackingTeam == TEAM_BLUE)) {
		//		canBePicked = qtrue;
		//	}
		//}
		
		if( cgs.gametype == GT_CTF_ELIMINATION ) {
			if (cgs.elimflags & EF_ONEWAY && cg.predictedPlayerState.persistant[PERS_TEAM] != cgs.attackingTeam) {
				return;
			}
		}

		//Currently we don't predict anything in Double Domination because it looks like we take a flag
		if( cgs.gametype == GT_DOUBLE_D ) {
			if(cgs.redflag == TEAM_NONE)
				return; //Can never pick if just one flag is NONE (because then the other is too)
			if(item->giTag == PW_REDFLAG){ //at point A
				//if(cgs.redflag != cg.predictedPlayerState.persistant[PERS_TEAM]) //not already taken
				//	trap_S_StartLocalSound( cgs.media.hitSound , CHAN_ANNOUNCER );
				return;
			}	
			if(item->giTag == PW_BLUEFLAG){ //at point B
				//if(cgs.blueflag != cg.predictedPlayerState.persistant[PERS_TEAM]) //already taken
				//	trap_S_StartLocalSound( cgs.media.hitSound , CHAN_ANNOUNCER );
				return;
			}	
		}
	}

	if (CG_ItemPredictionDangerous(cent)) {
		return;
	}

	// grab it
	//if(canBePicked)
	//{
	
	BG_AddPredictableEventToPlayerstate( EV_ITEM_PICKUP, cent->currentState.modelindex , &cg.predictedPlayerState);

	// remove it from the frame so it won't be drawn
	cent->currentState.eFlags |= EF_NODRAW;
	cent->nextState.eFlags |= EF_NODRAW;

	// don't touch it again this prediction
	cent->miscTime = cg.time;

	// if its a weapon, give them some predicted ammo so the autoswitch will work
	if ( item->giType == IT_WEAPON ) {
		cg.predictedPlayerState.stats[ STAT_WEAPONS ] |= 1 << item->giTag;
		if (cg_predictWeapons.integer && cg.predictedPlayerState.ammo[ item->giTag ] < item->quantity) {
			cg.predictedPlayerState.ammo[ item->giTag ] = item->quantity;
		} else if ( !cg.predictedPlayerState.ammo[ item->giTag ] ) {
			cg.predictedPlayerState.ammo[ item->giTag ] = 1;
		}
	}
	//}
}


qboolean CG_MissileTouchedPortal(const vec3_t start, const vec3_t end) {
	int			i;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t cmodel;
	centity_t	*cent;

	for ( i = 0 ; i < cg_numTriggerEntities ; i++ ) {
		cent = cg_triggerEntities[ i ];
		ent = &cent->currentState;

		if ( ent->solid != SOLID_BMODEL ) {
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		trap_CM_BoxTrace( &trace, start, end,
			NULL, NULL, cmodel, -1 );

		//if ( !trace.startsolid ) {
		//	continue;
		//}
		if (trace.fraction == 1.0) {
			continue;
		}

		if ( ent->eType == ET_TELEPORT_TRIGGER ) {
			return qtrue;
		}
	}
	return qfalse;
}

/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
=========================
*/
static void CG_TouchTriggerPrediction( void ) {
	int			i;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t cmodel;
	centity_t	*cent;
	qboolean	spectator;

	// dead clients don't activate triggers
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	spectator = ( cg.predictedPlayerState.pm_type == PM_SPECTATOR );

	if ( cg.predictedPlayerState.pm_type != PM_NORMAL && !spectator ) {
		return;
	}

	for ( i = 0 ; i < cg_numTriggerEntities ; i++ ) {
		cent = cg_triggerEntities[ i ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM && !spectator ) {
			CG_TouchItem( cent );
			continue;
		}

		if ( ent->solid != SOLID_BMODEL ) {
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		trap_CM_BoxTrace( &trace, cg.predictedPlayerState.origin, cg.predictedPlayerState.origin, 
			cg_pmove.mins, cg_pmove.maxs, cmodel, -1 );

		if ( !trace.startsolid ) {
			continue;
		}

		if ( ent->eType == ET_TELEPORT_TRIGGER ) {
			if (cg_predictTeleport.integer && ent->generic1 && !cg.predictedTeleports) {
				qboolean noAngles;

				// only predict 1 teleport until we get confirmation from the server
				cg.predictedTeleports++;
						
				// teleporter has unique destination
				VectorCopy( ent->origin2, cg.predictedPlayerState.origin );

				cg.predictedPlayerState.origin[2] += 1;
				noAngles = (ent->angles2[0] > 999999.0);
				if (!noAngles) {
					int			i;
					// spit the player out
					AngleVectors( ent->angles2, cg.predictedPlayerState.velocity, NULL, NULL );
					VectorScale( cg.predictedPlayerState.velocity, 400, cg.predictedPlayerState.velocity );
					cg.predictedPlayerState.pm_time = 160;
					cg.predictedPlayerState.pm_flags |= PMF_TIME_KNOCKBACK;

					switch (cgs.movement) {
					case MOVEMENT_CPM_CPMA:
					case MOVEMENT_CPM_DEFRAG:
						break;
					default:
						// reset rampjump
						cg.predictedPlayerState.stats[STAT_JUMPTIME] = 0;
					}
					// Reset crouch slide.
					cg.predictedPlayerState.stats[STAT_EXTFLAGS] &= EXTFL_SLIDING;
					cg.predictedPlayerState.stats[STAT_SLIDETIMEOUT] = 0;

					// set the delta angle
					for (i=0 ; i<3 ; i++) {
						int		cmdAngle;

						cmdAngle = ANGLE2SHORT(ent->angles2[i]);
						cg.predictedPlayerState.delta_angles[i] = cmdAngle - cg_pmove.cmd.angles[i];
					}
					VectorCopy (ent->angles2, cg.predictedPlayerState.viewangles );
				}
			} else {
				cg.hyperspace = qtrue;
			}
			//cg.hyperspace = qtrue;
		} else if ( ent->eType == ET_PUSH_TRIGGER ) {
			BG_TouchJumpPad( &cg.predictedPlayerState, ent );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( cg.predictedPlayerState.jumppad_frame != cg.predictedPlayerState.pmove_framecount ) {
		cg.predictedPlayerState.jumppad_frame = 0;
		cg.predictedPlayerState.jumppad_ent = 0;
	}
}

//unlagged - optimized prediction
#define ABS(x) ((x) < 0 ? (-(x)) : (x))

static int IsUnacceptableError( playerState_t *ps, playerState_t *pps ) {
	vec3_t delta;
	int i;

	if ( pps->pm_type != ps->pm_type ||
			pps->pm_flags != ps->pm_flags ||
			pps->pm_time != ps->pm_time ) {
		return 1;
	}

	VectorSubtract( pps->origin, ps->origin, delta );
	if ( VectorLengthSquared( delta ) > 0.1f * 0.1f ) {
		if ( cg_showmiss.integer ) {
			CG_Printf("delta: %.2f  ", VectorLength(delta) );
		}
		return 2;
	}

	VectorSubtract( pps->velocity, ps->velocity, delta );
	if ( VectorLengthSquared( delta ) > 0.1f * 0.1f ) {
		if ( cg_showmiss.integer ) {
			CG_Printf("delta: %.2f  ", VectorLength(delta) );
		}
		return 3;
	}

	if ( pps->weaponTime != ps->weaponTime ||
			pps->gravity != ps->gravity ||
			pps->speed != ps->speed ||
			pps->delta_angles[0] != ps->delta_angles[0] ||
			pps->delta_angles[1] != ps->delta_angles[1] ||
			pps->delta_angles[2] != ps->delta_angles[2] || 
			pps->groundEntityNum != ps->groundEntityNum ) {
		return 4;
	}

	if ( pps->legsTimer != ps->legsTimer ||
			pps->legsAnim != ps->legsAnim ||
			pps->torsoTimer != ps->torsoTimer ||
			pps->torsoAnim != ps->torsoAnim ||
			pps->movementDir != ps->movementDir ) {
		return 5;
	}

	VectorSubtract( pps->grapplePoint, ps->grapplePoint, delta );
	if ( VectorLengthSquared( delta ) > 0.1f * 0.1f ) {
		return 6;
	}

	if ( pps->eFlags != ps->eFlags ) {
		return 7;
	}

	if ( pps->eventSequence != ps->eventSequence ) {
		return 8;
	}

	for ( i = 0; i < MAX_PS_EVENTS; i++ ) {
		if ( pps->events[i] != ps->events[i] ||
				pps->eventParms[i] != ps->eventParms[i] ) {
			return 9;
		}
	}

	if ( pps->externalEvent != ps->externalEvent ||
			pps->externalEventParm != ps->externalEventParm ||
			pps->externalEventTime != ps->externalEventTime ) {
		return 10;
	}

	if ( pps->clientNum != ps->clientNum ||
			pps->weapon != ps->weapon ||
			pps->weaponstate != ps->weaponstate ) {
		return 11;
	}

	if ( ABS(pps->viewangles[0] - ps->viewangles[0]) > 1.0f ||
			ABS(pps->viewangles[1] - ps->viewangles[1]) > 1.0f ||
			ABS(pps->viewangles[2] - ps->viewangles[2]) > 1.0f ) {
		return 12;
	}

	if ( pps->viewheight != ps->viewheight ) {
		return 13;
	}

	if ( pps->damageEvent != ps->damageEvent ||
			pps->damageYaw != ps->damageYaw ||
			pps->damagePitch != ps->damagePitch ||
			pps->damageCount != ps->damageCount ) {
		return 14;
	}

	for ( i = 0; i < MAX_STATS; i++ ) {
		if ( pps->stats[i] != ps->stats[i] ) {
			return 15;
		}
	}

	for ( i = 0; i < MAX_PERSISTANT; i++ ) {
		if ( pps->persistant[i] != ps->persistant[i] ) {
			return 16;
		}
	}

	for ( i = 0; i < MAX_POWERUPS; i++ ) {
		if ( pps->powerups[i] != ps->powerups[i] ) {
			return 17;
		}
	}

	for ( i = 0; i < MAX_WEAPONS; i++ ) {
		if ( pps->ammo[i] != ps->ammo[i] ) {
			return 18;
		}
	}

	if ( pps->generic1 != ps->generic1 ||
			pps->loopSound != ps->loopSound ||
			pps->jumppad_ent != ps->jumppad_ent ) {
		return 19;
	}

	return 0;
}
//unlagged - optimized prediction

/*
=================
CG_PredictPlayerState

Generates cg.predictedPlayerState for the current cg.time
cg.predictedPlayerState is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmd over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that on an internet connection, quite a few pmoves may be issued
each frame.

OPTIMIZE: don't re-simulate unless the newly arrived snapshot playerState_t
differs from the predicted one.  Would require saving all intermediate
playerState_t during prediction.

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/
void CG_PredictPlayerState( void ) {
	int			cmdNum, current;
	playerState_t	oldPlayerState;
	qboolean	moved;
	usercmd_t	oldestCmd;
	usercmd_t	latestCmd;
//unlagged - optimized prediction
	int stateIndex = 0, predictCmd = 0; //Sago: added initializing
	int numPredicted = 0, numPlayedBack = 0; // debug code
	int prevMsec;
//unlagged - optimized prediction

	cg.hyperspace = qfalse;	// will be set if touching a trigger_teleport
	cg.predictedTeleports = 0;

	// if this is the first frame we must guarantee
	// predictedPlayerState is valid even if there is some
	// other error condition
	if ( !cg.validPPS ) {
		cg.validPPS = qtrue;
		cg.predictedPlayerState = cg.snap->ps;
	}


	// demo playback just copies the moves
	if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		CG_InterpolatePlayerState( qfalse );
		CG_EncodePlayerBBox(NULL, &cg.predictedPlayerEntity.currentState);
		return;
	}

	// non-predicting local movement will grab the latest angles
	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_InterpolatePlayerState( qtrue );
		CG_EncodePlayerBBox(NULL, &cg.predictedPlayerEntity.currentState);
		return;
	}

	// prepare for pmove
	cg_pmove.ps = &cg.predictedPlayerState;
	cg_pmove.trace = CG_Trace;
	cg_pmove.pointcontents = CG_PointContents;
	if ( cg_pmove.ps->pm_type == PM_DEAD ) {
		cg_pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else {
		cg_pmove.tracemask = MASK_PLAYERSOLID;
	}
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		cg_pmove.tracemask &= ~CONTENTS_BODY;	// spectators can fly through bodies
	}
	if (cgs.ratFlags & RAT_NOINVISWALLS) {
		cg_pmove.tracemask &= ~CONTENTS_PLAYERCLIP;
	}
	cg_pmove.noFootsteps = ( cgs.dmflags & DF_NO_FOOTSTEPS ) > 0;

	// save the state before the pmove so we can detect transitions
	oldPlayerState = cg.predictedPlayerState;

	current = trap_GetCurrentCmdNumber();

	// if we don't have the commands right after the snapshot, we
	// can't accurately predict a current position, so just freeze at
	// the last good position we had
	cmdNum = current - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &oldestCmd );
	if ( oldestCmd.serverTime > cg.snap->ps.commandTime 
		&& oldestCmd.serverTime < cg.time ) {	// special check for map_restart
		if ( cg_showmiss.integer ) {
			CG_Printf ("exceeded PACKET_BACKUP on commands\n");
		}
		return;
	}

	// get the latest command so we can know which commands are from previous map_restarts
	trap_GetUserCmd( current, &latestCmd );

	// get the most recent information we have, even if
	// the server time is beyond our current cg.time,
	// because predicted player positions are going to 
	// be ahead of everything else anyway
	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
		cg.predictedPlayerState = cg.nextSnap->ps;
		cg.physicsTime = cg.nextSnap->serverTime;
	} else {
		cg.predictedPlayerState = cg.snap->ps;
		cg.physicsTime = cg.snap->serverTime;
	}

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
	}

	cg_pmove.pmove_fixed = pmove_fixed.integer;// | cg_pmove_fixed.integer;
	cg_pmove.pmove_msec = pmove_msec.integer;
        cg_pmove.pmove_float = pmove_float.integer;
        cg_pmove.pmove_flags = cgs.dmflags;
        cg_pmove.pmove_ratflags = cgs.ratFlags;
        cg_pmove.pmove_movement = cgs.movement;

//unlagged - optimized prediction
	// Like the comments described above, a player's state is entirely
	// re-predicted from the last valid snapshot every client frame, which
	// can be really, really, really slow.  Every old command has to be
	// run again.  For every client frame that is *not* directly after a
	// snapshot, this is unnecessary, since we have no new information.
	// For those, we'll play back the predictions from the last frame and
	// predict only the newest commands.  Essentially, we'll be doing
	// an incremental predict instead of a full predict.
	//
	// If we have a new snapshot, we can compare its player state's command
	// time to the command times in the queue to find a match.  If we find
	// a matching state, and the predicted version has not deviated, we can
	// use the predicted state as a base - and also do an incremental predict.
	//
	// With this method, we get incremental predicts on every client frame
	// except a frame following a new snapshot in which there was a prediction
	// error.  This yeilds anywhere from a 15% to 40% performance increase,
	// depending on how much of a bottleneck the CPU is.

	if ( cg_optimizePrediction.integer ) {
		if ( cg.nextFrameTeleport || cg.thisFrameTeleport ) {
			// do a full predict
			cg.lastPredictedCommand = 0;
			cg.stateTail = cg.stateHead;
			predictCmd = current - CMD_BACKUP + 1;
		}
		// cg.physicsTime is the current snapshot's serverTime
		// if it's the same as the last one
		else if ( cg.physicsTime == cg.lastServerTime ) {
			// we have no new information, so do an incremental predict
			predictCmd = cg.lastPredictedCommand + 1;
		}
		else {
			// we have a new snapshot

			int i;
			qboolean error = qtrue;

			// loop through the saved states queue
			for ( i = cg.stateHead; i != cg.stateTail; i = (i + 1) % NUM_SAVED_STATES ) {
				// if we find a predicted state whose commandTime matches the snapshot player state's commandTime
				if ( cg.savedPmoveStates[i].commandTime == cg.predictedPlayerState.commandTime ) {
					// make sure the state differences are acceptable
					int errorcode = IsUnacceptableError( &cg.predictedPlayerState, &cg.savedPmoveStates[i] );

					// too much change?
					if ( errorcode ) {
						if ( cg_showmiss.integer ) {
							CG_Printf("errorcode %d at %d\n", errorcode, cg.time);
						}
						// yeah, so do a full predict
						break;
					}

					// this one is almost exact, so we'll copy it in as the starting point
					*cg_pmove.ps = cg.savedPmoveStates[i];
					// advance the head
					cg.stateHead = (i + 1) % NUM_SAVED_STATES;

					// set the next command to predict
					predictCmd = cg.lastPredictedCommand + 1;

					// a saved state matched, so flag it
					error = qfalse;
					break;
				}
			}

			// if no saved states matched
			if ( error ) {
				// do a full predict
				cg.lastPredictedCommand = 0;
				cg.stateTail = cg.stateHead;
				predictCmd = current - CMD_BACKUP + 1;
			}
		}

		// keep track of the server time of the last snapshot so we
		// know when we're starting from a new one in future calls
		cg.lastServerTime = cg.physicsTime;
		stateIndex = cg.stateHead;
	}
//unlagged - optimized prediction

	// run cmds
	moved = qfalse;
	prevMsec = -1;
	for ( cmdNum = current - CMD_BACKUP + 1 ; cmdNum <= current ; cmdNum++ ) {
		// get the command
		trap_GetUserCmd( cmdNum, &cg_pmove.cmd );

		if ( cg_pmove.pmove_fixed ) {
			PM_UpdateViewAngles( cg_pmove.ps, &cg_pmove.cmd );
		}

		// save previous time
		if (cmdNum < current) {
			prevMsec = cg_pmove.cmd.serverTime;
		}

		// don't do anything if the time is before the snapshot player time
		if ( cg_pmove.cmd.serverTime <= cg.predictedPlayerState.commandTime ) {
			continue;
		}

		// don't do anything if the command was from a previous map_restart
		if ( cg_pmove.cmd.serverTime > latestCmd.serverTime ) {
			continue;
		}

		// check for a prediction error from last frame
		// on a lan, this will often be the exact value
		// from the snapshot, but on a wan we will have
		// to predict several commands to get to the point
		// we want to compare
		if ( cg.predictedPlayerState.commandTime == oldPlayerState.commandTime ) {
			vec3_t	delta;
			float	len;

			if ( cg.thisFrameTeleport ) {
				// a teleport will not cause an error decay
				VectorClear( cg.predictedError );
				if ( cg_showmiss.integer ) {
					CG_Printf( "PredictionTeleport\n" );
				}
				cg.thisFrameTeleport = qfalse;
			} else {
				vec3_t	adjusted;
				CG_AdjustPositionForMover( cg.predictedPlayerState.origin, 
					cg.predictedPlayerState.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted );

				if ( cg_showmiss.integer ) {
					if (!VectorCompare( oldPlayerState.origin, adjusted )) {
						CG_Printf("prediction error\n");
					}
				}
				VectorSubtract( oldPlayerState.origin, adjusted, delta );
				len = VectorLength( delta );
				if ( len > 0.1 ) {
					if ( cg_showmiss.integer ) {
						CG_Printf("Prediction miss: %f\n", len);
					}
					if ( cg_errorDecay.integer ) {
						int		t;
						float	f;

						t = cg.time - cg.predictedErrorTime;
						f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
						if ( f < 0 ) {
							f = 0;
						}
						if ( f > 0 && cg_showmiss.integer ) {
							CG_Printf("Double prediction decay: %f\n", f);
						}
						VectorScale( cg.predictedError, f, cg.predictedError );
					} else {
						VectorClear( cg.predictedError );
					}
					VectorAdd( delta, cg.predictedError, cg.predictedError );
					cg.predictedErrorTime = cg.oldTime;
				}
			}
		}

		// store time delta between commands (used for projectile delag)
		if (prevMsec != -1 && latestCmd.serverTime - prevMsec > 0) {
			cg.cmdMsecDelta = latestCmd.serverTime - prevMsec;
		} else {
			cg.cmdMsecDelta = 0;
		}

		// don't predict gauntlet firing, which is only supposed to happen
		// when it actually inflicts damage
		cg_pmove.gauntletHit = qfalse;

		if ( cg_pmove.pmove_fixed ) {
			cg_pmove.cmd.serverTime = ((cg_pmove.cmd.serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		}

//unlagged - optimized prediction
		if ( cg_optimizePrediction.integer ) {
			// if we need to predict this command, or we've run out of space in the saved states queue
			if ( cmdNum >= predictCmd || (stateIndex + 1) % NUM_SAVED_STATES == cg.stateHead ) {
				// run the Pmove
				Pmove (&cg_pmove);

				numPredicted++; // debug code

				// record the last predicted command
				cg.lastPredictedCommand = cmdNum;

				// if we haven't run out of space in the saved states queue
				if ( (stateIndex + 1) % NUM_SAVED_STATES != cg.stateHead ) {
					// save the state for the false case (of cmdNum >= predictCmd)
					// in later calls to this function
					cg.savedPmoveStates[stateIndex] = *cg_pmove.ps;
					stateIndex = (stateIndex + 1) % NUM_SAVED_STATES;
					cg.stateTail = stateIndex;
				}
			}
			else {
				numPlayedBack++; // debug code

				if ( cg_showmiss.integer && 
						cg.savedPmoveStates[stateIndex].commandTime != cg_pmove.cmd.serverTime) {
					// this should ONLY happen just after changing the value of pmove_fixed
					CG_Printf( "saved state miss\n" );
				}

				// play back the command from the saved states
				*cg_pmove.ps = cg.savedPmoveStates[stateIndex];

				// go to the next element in the saved states array
				stateIndex = (stateIndex + 1) % NUM_SAVED_STATES;
			}
		}
		else {
			// run the Pmove
			Pmove (&cg_pmove);

			numPredicted++; // debug code
		}
//unlagged - optimized prediction

		moved = qtrue;

		// add push trigger movement effects
		CG_TouchTriggerPrediction();

		// check for predictable events that changed from previous predictions
		if (cg_checkChangedEvents.integer) {
			if (!cg_optimizePrediction.integer || cmdNum == cg.lastPredictedCommand) {
				CG_CheckChangedPredictableEvents(&cg.predictedPlayerState);
			}
		}
	}

//unlagged - optimized prediction
	// do a /condump after a few seconds of this
	//CG_Printf("cg.time: %d, numPredicted: %d, numPlayedBack: %d\n", cg.time, numPredicted, numPlayedBack); // debug code
	// if everything is working right, numPredicted should be 1 more than 98%
	// of the time, meaning only ONE predicted move was done in the frame
	// you should see other values for numPredicted after IsUnacceptableError
	// returns nonzero, and that's it
//unlagged - optimized prediction

	if ( cg_showmiss.integer > 1 ) {
		CG_Printf( "[%i : %i] ", cg_pmove.cmd.serverTime, cg.time );
	}

	if ( !moved ) {
		CG_EncodePlayerBBox(&cg_pmove, &cg.predictedPlayerEntity.currentState);
		if ( cg_showmiss.integer ) {
			CG_Printf( "not moved\n" );
		}
		return;
	}

	// adjust for the movement of the groundentity
	CG_AdjustPositionForMover( cg.predictedPlayerState.origin, 
		cg.predictedPlayerState.groundEntityNum, 
		cg.physicsTime, cg.time, cg.predictedPlayerState.origin );

	if ( cg_showmiss.integer ) {
		if (cg.predictedPlayerState.eventSequence > oldPlayerState.eventSequence + MAX_PS_EVENTS) {
			CG_Printf("WARNING: dropped event\n");
		}
	}

	// fire events and other transition triggered things
	CG_TransitionPlayerState( &cg.predictedPlayerState, &oldPlayerState );

	if ( (cg_checkChangedEvents.integer || cg_showmiss.integer) &&
			cg.eventSequence > cg.predictedPlayerState.eventSequence) {
		if (cg_showmiss.integer) {
			CG_Printf("WARNING: double event\n");
		}
		cg.eventSequence = cg.predictedPlayerState.eventSequence;
	}

	CG_EncodePlayerBBox(&cg_pmove, &cg.predictedPlayerEntity.currentState);
}


