#include "g_local.h"

void xfprintf( fileHandle_t f, const char *fmt, ... ) __attribute__ ((format (printf, 2, 3)));

void xfprintf( fileHandle_t f, const char *fmt, ... ) {
	int len;
	va_list	ap;
	char buf[32000];

	va_start (ap, fmt);
	len = Q_vsnprintf (buf, sizeof(buf), fmt, ap);
	va_end (ap);
	if ( len >= sizeof( buf ) ) {
		Com_Error( ERR_FATAL, "xfprintf: overflowed buffer" );
	}
	trap_FS_Write(buf, len, f);
}

void json_writestring(fileHandle_t f, const char *key, const char *val) {
	const char *p;
	xfprintf(f, "\"%s\":\"", key);
	for (p = val; *p; ++p) {
		if (*p == '"') {
			xfprintf(f, "\\\"");
		} else if (isprint(*p)) {
			xfprintf(f, "%c", *p);
		} else if ((unsigned char) *p > 0x7f) {
			xfprintf(f, "\\uFFFD");
		} else {
			xfprintf(f, "\\u00%02x", *p);
		}
	}
	xfprintf(f, "\"");
}

void json_writeint(fileHandle_t f, const char *key, int val) {
	xfprintf(f, "\"%s\":%i", key, val);
}

void json_writefloat(fileHandle_t f, const char *key, float val) {
	xfprintf(f, "\"%s\":%f", key, val);
}

void json_writebool(fileHandle_t f, const char *key, qboolean val) {
	xfprintf(f, "\"%s\":%s", key, val ? "true" : "false");
}

void json_timestamp(fileHandle_t f, const char *key) {
	qtime_t now;
	trap_RealTime(&now);
	json_writestring(f, key, va("%04i-%02i-%02iT%02i:%02i:%02i",
				1900+now.tm_year,
				1+now.tm_mon,
				now.tm_mday,
				now.tm_hour,
				now.tm_min,
				now.tm_sec
				));

}

void G_JSONExportAward(fileHandle_t f, const char *name, int count, qboolean *comma) {
	if (count <= 0) {
		return;
	}
	if (*comma) {
		xfprintf(f, ",");
	}
	*comma = qtrue;
	json_writeint(f, name, count);

}

void G_JSONExportPlayer(fileHandle_t f, gclient_t *cl) {
	qboolean comma;
	gentity_t *ent = g_entities + (cl - level.clients);
	int i;

	xfprintf(f, "{");
	json_writestring(f, "name", cl->pers.netname);
	xfprintf(f, ",");
	json_writeint(f, "team", cl->sess.sessionTeam);
	xfprintf(f, ",");
	json_writeint(f, "score", cl->ps.persistant[PERS_SCORE]);
	xfprintf(f, ",");
	json_writebool(f, "isbot", ent->r.svFlags & SVF_BOT);
	xfprintf(f, ",");
	json_writeint(f, "playtime", (level.time - cl->pers.enterTime)/1000);
	xfprintf(f, ",");
	json_writeint(f, "kills", cl->pers.kills);
	xfprintf(f, ",");
	json_writeint(f, "deaths", cl->pers.deaths);
	xfprintf(f, ",");
	json_writeint(f, "damage_given", cl->pers.dmgGiven);
	xfprintf(f, ",");
	json_writeint(f, "damage_taken", cl->pers.dmgTaken);
	xfprintf(f, ",");
	json_writeint(f, "captures", cl->ps.persistant[PERS_CAPTURES]);
	xfprintf(f, ",");
	json_writeint(f, "defends", cl->ps.persistant[PERS_DEFEND_COUNT]);
	xfprintf(f, ",");
	json_writeint(f, "assists", cl->ps.persistant[PERS_ASSIST_COUNT]);
	xfprintf(f, ",");
	json_writeint(f, "flag_recoveries", cl->pers.teamState.flagrecovery);

	xfprintf(f, ",");
	xfprintf(f, "\"weapons\":{");
	comma = qfalse;
	for (i = WP_GAUNTLET; i < WP_NUM_WEAPONS; ++i) {
		if (cl->accuracy[i][0] <= 0) {
			continue;
		}
		if (comma) {
			xfprintf(f, ",");
		}
		comma = qtrue;
		xfprintf(f, "\"%i\": {", i);
		json_writeint(f, "shots", cl->accuracy[i][0]);
		xfprintf(f, ",");
		json_writeint(f, "hits", cl->accuracy[i][1]);
		xfprintf(f, ",");
		json_writeint(f, "damage", cl->pers.damage[i]);
		xfprintf(f, "}");
	}
	xfprintf(f, "}");

	xfprintf(f, ",");
	xfprintf(f, "\"awards\":{");
	comma = qfalse;
	G_JSONExportAward(f, "impressive", cl->ps.persistant[PERS_IMPRESSIVE_COUNT], &comma);
	G_JSONExportAward(f, "excellent", cl->ps.persistant[PERS_EXCELLENT_COUNT], &comma);
	G_JSONExportAward(f, "humiliation", cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], &comma);
	G_JSONExportAward(f, "defend", cl->ps.persistant[PERS_DEFEND_COUNT], &comma);
	G_JSONExportAward(f, "assist", cl->ps.persistant[PERS_ASSIST_COUNT], &comma);
	G_JSONExportAward(f, "capture", cl->ps.persistant[PERS_CAPTURES], &comma);
	G_JSONExportAward(f, "frags", cl->pers.awardCounts[EAWARD_FRAGS], &comma);
	G_JSONExportAward(f, "accuracy", cl->pers.awardCounts[EAWARD_ACCURACY], &comma);
	G_JSONExportAward(f, "telefrag", cl->pers.awardCounts[EAWARD_TELEFRAG], &comma);
	G_JSONExportAward(f, "telemissile_frag", cl->pers.awardCounts[EAWARD_TELEMISSILE_FRAG], &comma);
	G_JSONExportAward(f, "bullseye", cl->pers.awardCounts[EAWARD_ROCKETSNIPER], &comma);
	G_JSONExportAward(f, "fullsg", cl->pers.awardCounts[EAWARD_FULLSG], &comma);
	G_JSONExportAward(f, "immortality", cl->pers.awardCounts[EAWARD_IMMORTALITY], &comma);
	G_JSONExportAward(f, "airrocket", cl->pers.awardCounts[EAWARD_AIRROCKET], &comma);
	G_JSONExportAward(f, "airgrenade", cl->pers.awardCounts[EAWARD_AIRGRENADE], &comma);
	G_JSONExportAward(f, "rocketrail", cl->pers.awardCounts[EAWARD_ROCKETRAIL], &comma);
	G_JSONExportAward(f, "lgrail", cl->pers.awardCounts[EAWARD_LGRAIL], &comma);
	G_JSONExportAward(f, "railtwo", cl->pers.awardCounts[EAWARD_RAILTWO], &comma);
	G_JSONExportAward(f, "deadhand", cl->pers.awardCounts[EAWARD_DEADHAND], &comma);
	G_JSONExportAward(f, "showstopper", cl->pers.awardCounts[EAWARD_SHOWSTOPPER], &comma);
	G_JSONExportAward(f, "ambush", cl->pers.awardCounts[EAWARD_AMBUSH], &comma);
	G_JSONExportAward(f, "kamikaze", cl->pers.awardCounts[EAWARD_KAMIKAZE], &comma);
	G_JSONExportAward(f, "strongman", cl->pers.awardCounts[EAWARD_STRONGMAN], &comma);
	G_JSONExportAward(f, "hero", cl->pers.awardCounts[EAWARD_HERO], &comma);
	G_JSONExportAward(f, "butcher", cl->pers.awardCounts[EAWARD_BUTCHER], &comma);
	G_JSONExportAward(f, "killingspree", cl->pers.awardCounts[EAWARD_KILLINGSPREE], &comma);
	G_JSONExportAward(f, "rampage", cl->pers.awardCounts[EAWARD_RAMPAGE], &comma);
	G_JSONExportAward(f, "massacre", cl->pers.awardCounts[EAWARD_MASSACRE], &comma);
	G_JSONExportAward(f, "unstoppable", cl->pers.awardCounts[EAWARD_UNSTOPPABLE], &comma);
	G_JSONExportAward(f, "grimreaper", cl->pers.awardCounts[EAWARD_GRIMREAPER], &comma);
	G_JSONExportAward(f, "revenge", cl->pers.awardCounts[EAWARD_REVENGE], &comma);
	G_JSONExportAward(f, "berserker", cl->pers.awardCounts[EAWARD_BERSERKER], &comma);
	G_JSONExportAward(f, "vaporized", cl->pers.awardCounts[EAWARD_VAPORIZED], &comma);
	G_JSONExportAward(f, "twitchrail", cl->pers.awardCounts[EAWARD_TWITCHRAIL], &comma);
	G_JSONExportAward(f, "rat", cl->pers.awardCounts[EAWARD_RAT], &comma);
	G_JSONExportAward(f, "thawbuddy", cl->pers.awardCounts[EAWARD_THAWBUDDY], &comma);
	xfprintf(f, "}");

	xfprintf(f, ",");
	xfprintf(f, "\"items\":{");
	comma = qfalse;
	for (i = 0; i < MIN(bg_numItems, MAX_BG_ITEM_STATS); ++i) {
		gitem_t *item = &bg_itemlist[i];
		if (!item->classname) {
			continue;
		}
		if (cl->pers.items_collected[i] <= 0) {
			continue;
		}
		if (comma) {
			xfprintf(f, ",");
		}
		comma = qtrue;
		json_writeint(f, item->classname, cl->pers.items_collected[i]);
	}
	xfprintf(f, "}");

	xfprintf(f, "}");
}

void G_JSONExport(fileHandle_t f, const char *exitreason, int gameId) {
	char mapname[MAX_QPATH];
	char svname[MAX_INFO_VALUE];
	int i;
	qboolean comma;
	gclient_t *cl;

	xfprintf(f, "{");
	trap_Cvar_VariableStringBuffer( "sv_hostname", svname, sizeof( svname ) );
	json_writestring(f, "servername", svname);
	xfprintf(f, ",");
	json_timestamp(f, "time");
	xfprintf(f, ",");
	json_writeint(f, "gametype", g_gametype.integer);
#ifdef WITH_MULTITOURNAMENT
	if (g_gametype.integer == GT_MULTITOURNAMENT) {
		xfprintf(f, ",");
		json_writeint(f, "gameId", gameId);
	}
#endif
	xfprintf(f, ",");
	if (G_IsTeamGametype()) {
		json_writebool(f, "team_gt", qtrue);
		xfprintf(f, ",");
		json_writeint(f, "score_red", level.teamScores[TEAM_RED]);
		xfprintf(f, ",");
		json_writeint(f, "score_blue", level.teamScores[TEAM_BLUE]);
	} else {
		json_writebool(f, "team_gt", qfalse);
	}
	xfprintf(f, ",");
	json_writeint(f, "fraglimit", g_fraglimit.integer);
	xfprintf(f, ",");
	json_writeint(f, "capturelimit", g_capturelimit.integer);
	xfprintf(f, ",");
	json_writeint(f, "timelimit", g_timelimit.integer);

	xfprintf(f, ",");
	json_writestring(f, "exit_reason", exitreason);

	xfprintf(f, ",");
	trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
	json_writestring(f, "map", mapname);

	xfprintf(f, ",");
	xfprintf(f, "\"players\":[");

	comma = qfalse;
	for (i=0 ; i < level.numConnectedClients ; ++i) {
		cl = &level.clients[level.sortedClients[i]];

		if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( cl->pers.connected == CON_CONNECTING ) {
			continue;
		}

#ifdef WITH_MULTITOURNAMENT
		if (g_gametype.integer == GT_MULTITOURNAMENT && cl->sess.gameId != gameId) {
			continue;
		}
#endif

		if (comma) {
			xfprintf(f, ",");
		}
		G_JSONExportPlayer(f, cl);
		comma = qtrue;

	}
	xfprintf(f, "]");

	xfprintf(f, "}");
}

#ifdef WITH_MULTITOURNAMENT
qboolean G_ShouldRecordMtrnGame(int gameId) {
	int i;
	multiTrnGame_t *game;

	if (!G_ValidGameId(gameId)) {
		return qfalse;
	}
       
	game = &level.multiTrnGames[gameId];

	if (!(game->gameFlags & MTRN_GFL_RUNNING)) {
		return qfalse;
	}
	if (!(game->gameFlags & MTRN_GFL_STARTEDATBEGINNING)) {
		return qfalse;
	}
	if (game->numPlayers != 2) {
		return qfalse;
	}

	for (i = 0; i < game->numPlayers; ++i) {
		gentity_t *player;
		if (game->clients[i] < 0 || game->clients[i] >= MAX_CLIENTS) {
			return qfalse;
		}
		player = &g_entities[game->clients[i]];
		if (player->r.svFlags & SVF_BOT) {
			return qfalse;
		}
	}
	return qtrue;
}
#endif // WITH_MULTITOURNAMENT

void G_WriteStatsJSON(const char *exitreason, int game_id) {
	char fname[MAX_OSPATH];
	qtime_t now;
	fileHandle_t f;
	int len;
	int humans = 0;

	if (!g_exportStats.integer) {
		return;
	}

	if (G_IsTeamGametype()) {
		humans = G_CountHumanPlayers(TEAM_RED) + G_CountHumanPlayers(TEAM_BLUE);
#ifdef WITH_MULTITOURNAMENT
	} else if (g_gametype.integer == GT_MULTITOURNAMENT) {
		if (!G_ShouldRecordMtrnGame(game_id)) {
			return;
		}
		humans = 2;
#endif
	} else {
		humans = G_CountHumanPlayers(TEAM_FREE);
	}
	if (humans == 0) {
		return;
	}

	trap_RealTime(&now);
	Com_sprintf(fname, sizeof(fname), "%04i-%02i-%02i-%02i-%02i-%02i_%i.json",
				1900+now.tm_year,
				1+now.tm_mon,
			       	now.tm_mday,
				now.tm_hour,
				now.tm_min,
				now.tm_sec,
				game_id
				);
	len = trap_FS_FOpenFile( va("stats/%s", fname), &f, FS_WRITE );
	if (len < 0) {
		Com_Printf(S_COLOR_YELLOW "WARNING: failed to write stats file\n");
		return;
	}
	if (len > 0) {
		Com_Printf(S_COLOR_YELLOW "WARNING: failed to write stats file: file exists\n");
		trap_FS_FCloseFile(f);
		return;
	}

	G_JSONExport(f, exitreason, game_id);

	trap_FS_FCloseFile(f);
	len = trap_FS_FOpenFile( va("stats/%s.done", fname), &f, FS_WRITE );
	if (len >= 0) {
		trap_FS_FCloseFile(f);
	}

}
