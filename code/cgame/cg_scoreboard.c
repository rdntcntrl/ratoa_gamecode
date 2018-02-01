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
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"


#define	SCOREBOARD_X		(0)

#define SB_HEADER			86
#define SB_TOP				(SB_HEADER+32)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR		420

#define SB_NORMAL_HEIGHT	40
#define SB_INTER_HEIGHT		16 // interleaved height

#define SB_MAXCLIENTS_NORMAL  ((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER   ((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

// Used when interleaved



#define SB_LEFT_BOTICON_X	(SCOREBOARD_X+0)
#define SB_LEFT_HEAD_X		(SCOREBOARD_X+32)
#define SB_RIGHT_BOTICON_X	(SCOREBOARD_X+64)
#define SB_RIGHT_HEAD_X		(SCOREBOARD_X+96)
// Normal
#define SB_BOTICON_X		(SCOREBOARD_X+32)
#define SB_HEAD_X			(SCOREBOARD_X+64)

#define SB_SCORELINE_X		112

#define SB_RATING_WIDTH	    (6 * BIGCHAR_WIDTH) // width 6
#define SB_SCORE_X			(SB_SCORELINE_X + BIGCHAR_WIDTH) // width 6
#define SB_RATING_X			(SB_SCORELINE_X + 6 * BIGCHAR_WIDTH) // width 6
#define SB_PING_X			(SB_SCORELINE_X + 12 * BIGCHAR_WIDTH + 8) // width 5
#define SB_TIME_X			(SB_SCORELINE_X + 17 * BIGCHAR_WIDTH + 8) // width 5
#define SB_NAME_X			(SB_SCORELINE_X + 22 * BIGCHAR_WIDTH) // width 15



// RAT Scoreboard ==============================
#define RATSCOREBOARD_X  (0)

//#define RATSB_HEADER   86
#define RATSB_TOP    (RATSB_HEADER+10)

// Where the status bar starts, so we don't overwrite it
#define RATSB_STATUSBAR  420

#define RATSB_NORMAL_HEIGHT 40
#define RATSB_INTER_HEIGHT  16 // interleaved height

#define RATSB_MAXCLIENTS_INTER   ((RATSB_STATUSBAR - RATSB_TOP) / RATSB_INTER_HEIGHT - 1)

#define RATSB_BOTICON_X  (RATSCOREBOARD_X+10)
#define RATSB_HEAD_X   (RATSB_BOTICON_X+32)

#define RATSB_SCORELINE_X  64

#define RATSB_RATING_WIDTH     (0)
#define RATSB_WINS_X           (RATSB_SCORELINE_X +     SCORESMALLCHAR_WIDTH)
#define RATSB_WL_X             (RATSB_WINS_X      + RATSB_WINS_WIDTH      + 0 * SCORESMALLCHAR_WIDTH)
#define RATSB_LOSSES_X         (RATSB_WL_X        + RATSB_WL_WIDTH        + 0 * SCORESMALLCHAR_WIDTH)
#define RATSB_SCORE_X          (RATSB_LOSSES_X    + RATSB_LOSSES_WIDTH    + 0 * SCORESMALLCHAR_WIDTH)
#define RATSB_TIME_X           (RATSB_SCORE_X     + RATSB_SCORE_WIDTH     + 1 * SCORESMALLCHAR_WIDTH)
#define RATSB_CNUM_X           (RATSB_TIME_X      + RATSB_TIME_WIDTH      + 1 * SCORESMALLCHAR_WIDTH)
#define RATSB_NAME_X           (RATSB_CNUM_X      + RATSB_CNUM_WIDTH      + 1 * SCORESMALLCHAR_WIDTH)
#define RATSB_KD_X	       (RATSB_NAME_X      + RATSB_NAME_WIDTH      + 1 * SCORESMALLCHAR_WIDTH)
#define RATSB_DT_X	       (RATSB_KD_X        + RATSB_KD_WIDTH	  + 1 * SCORESMALLCHAR_WIDTH)

#define RATSB_ACCURACY_X       (RATSB_DT_X        + RATSB_DT_WIDTH        + 1 * SCORESMALLCHAR_WIDTH)
#define RATSB_PING_X           (RATSB_ACCURACY_X  + RATSB_ACCURACY_WIDTH  + 1 * SCORESMALLCHAR_WIDTH)

//#define RATSB_ACCURACY_X       (RATSB_DT_X        + RATSB_DT_WIDTH        + 1 * SCORESMALLCHAR_WIDTH)
//#define RATSB_PING_X           (RATSB_ACCURACY_X  + RATSB_ACCURACY_WIDTH  + 1 * SCORESMALLCHAR_WIDTH)

#define RATSB_NAME_LENGTH	(25)

#define RATSB_WINS_WIDTH       (2 * SCORESMALLCHAR_WIDTH)
#define RATSB_WL_WIDTH         (1 * SCORESMALLCHAR_WIDTH)
#define RATSB_LOSSES_WIDTH     (2 * SCORESMALLCHAR_WIDTH)
#define RATSB_SCORE_WIDTH      (MAX(4*SCORECHAR_WIDTH,5*SCORESMALLCHAR_WIDTH))
#define RATSB_TIME_WIDTH       (3 * SCORESMALLCHAR_WIDTH)
#define RATSB_CNUM_WIDTH       (2 * SCORESMALLCHAR_WIDTH)
#define RATSB_NAME_WIDTH       (RATSB_NAME_LENGTH * SCORECHAR_WIDTH)
#define RATSB_KD_WIDTH         (7 * SCORETINYCHAR_WIDTH)
#define RATSB_DT_WIDTH         (9 * SCORETINYCHAR_WIDTH)
#define RATSB_ACCURACY_WIDTH   (4 * SCORETINYCHAR_WIDTH)
#define RATSB_PING_WIDTH       (3 * SCORESMALLCHAR_WIDTH)

#define RATSB_WINS_CENTER      (RATSB_WINS_X + RATSB_WINS_WIDTH/2)
#define RATSB_WL_CENTER        (RATSB_WL_X + RATSB_WL_WIDTH/2)
#define RATSB_LOSSES_CENTER    (RATSB_LOSSES_X + RATSB_LOSSES_WIDTH/2)
#define RATSB_SCORE_CENTER     (RATSB_SCORE_X + RATSB_SCORE_WIDTH/2)
#define RATSB_TIME_CENTER      (RATSB_TIME_X + RATSB_TIME_WIDTH/2)
#define RATSB_CNUM_CENTER      (RATSB_CNUM_X + RATSB_CNUM_WIDTH/2)
#define RATSB_KD_CENTER        (RATSB_KD_X + RATSB_KD_WIDTH/2)
#define RATSB_DT_CENTER        (RATSB_DT_X + RATSB_DT_WIDTH/2)
#define RATSB_ACCURACY_CENTER  (RATSB_ACCURACY_X + RATSB_ACCURACY_WIDTH/2)
#define RATSB_PING_CENTER      (RATSB_PING_X + RATSB_PING_WIDTH/2)

// The new and improved score board
//
// In cases where the number of clients is high, the score board heads are interleaved
// here's the layout

//
//	0   32   80  112  144   240  320  400   <-- pixel position
//  bot head bot head score ping time name
//  
//  wins/losses are drawn on bot icon now

static qboolean localClient; // true if local client has been displayed


/*
=================
CG_RatioColor
=================
 */
static void CG_RatioColor(float a, float b, vec4_t color) {
	float s = a + b;
	color[3] = 1.0;
	color[2] = 0;

	if (s == 0.0) {
		color[0] = color[1] = 1.0;
		return;
	}
	color[0] = MIN(2.0*b/s, 1.0);
	color[1] = MIN(2.0*a/s, 1.0);

}

/*
=================
CG_PingColor
=================
 */
static void CG_PingColor(int ping, vec4_t color) {
	float ratio;
	color[3] = 1.0;
	color[2] = 0;
	if (ping >= 200) {
		color[0] = 1.0;
		color[1] = 0;
		return;
	}
	ratio = (float)ping/200.0;
	color[0] = MIN(2.0*ratio, 1.0);
	color[1] = MIN(2.0*(1.0-ratio), 1.0);

}


/*
=================
CG_RatDrawScoreboard
=================
 */
static void CG_RatDrawClientScore(int y, score_t *score, float *color, float fade, qboolean largeFormat) {
	char string[1024];
	vec3_t headAngles;
	clientInfo_t *ci;
	centity_t *cent;
	int iconx, headx;
	float tcolor[4] = { 1.0, 1.0, 1.0, 1.0 };
	int ysmall = y + (SCORECHAR_HEIGHT - SCORESMALLCHAR_HEIGHT);
	int ytiny = y + (SCORECHAR_HEIGHT - SCORETINYCHAR_HEIGHT);

	if (score->client < 0 || score->client >= cgs.maxclients) {
		Com_Printf("Bad score->client: %i\n", score->client);
		return;
	}

	ci = &cgs.clientinfo[score->client];
	cent = &cg_entities[score->client];

	iconx = RATSB_BOTICON_X + (RATSB_RATING_WIDTH / 2);
	headx = RATSB_HEAD_X + (RATSB_RATING_WIDTH / 2);

	// draw the handicap or bot skill marker (unless player has flag)
	if (ci->powerups & (1 << PW_NEUTRALFLAG)) {
		if (largeFormat) {
			CG_DrawFlagModel(iconx, y - (32 - SCORECHAR_HEIGHT) / 2, 32, 32, TEAM_FREE, qfalse);
		} else {
			CG_DrawFlagModel(iconx, y, 16, 16, TEAM_FREE, qfalse);
		}
	} else if (ci->powerups & (1 << PW_REDFLAG)) {
		if (largeFormat) {
			CG_DrawFlagModel(iconx, y - (32 - SCORECHAR_HEIGHT) / 2, 32, 32, TEAM_RED, qfalse);
		} else {
			CG_DrawFlagModel(iconx, y, 16, 16, TEAM_RED, qfalse);
		}
	} else if (ci->powerups & (1 << PW_BLUEFLAG)) {
		if (largeFormat) {
			CG_DrawFlagModel(iconx, y - (32 - SCORECHAR_HEIGHT) / 2, 32, 32, TEAM_BLUE, qfalse);
		} else {
			CG_DrawFlagModel(iconx, y, 16, 16, TEAM_BLUE, qfalse);
		}
	} else {
		if (ci->botSkill > 0 && ci->botSkill <= 5) {
			if (cg_drawIcons.integer) {
				if (largeFormat) {
					CG_DrawPic(iconx, y - (32 - SCORECHAR_HEIGHT) / 2, 32, 32, cgs.media.botSkillShaders[ ci->botSkill - 1 ]);
				} else {
					CG_DrawPic(iconx, y, 16, 16, cgs.media.botSkillShaders[ ci->botSkill - 1 ]);
				}
			}
		} else if (ci->handicap < 100) {
			Com_sprintf(string, sizeof ( string), "%i", ci->handicap);
			//CG_DrawSmallScoreStringColor(iconx, ysmall, string, color);
			tcolor[0] = tcolor[1] = tcolor[2] = 0.75;
			//CG_DrawSmallScoreStringColor(iconx, ysmall, string, tcolor);
			CG_DrawTinyScoreStringColor(iconx, ytiny, string, tcolor);
		}
		if ( ci->team != TEAM_SPECTATOR && cent &&  cent->currentState.eFlags & EF_TALK ) {
			if (largeFormat) {
				CG_DrawPic(iconx, y - (32 - SCORECHAR_HEIGHT) / 2, 32, 32, cgs.media.balloonShader);
			} else {
				CG_DrawPic(iconx, y, 16, 16, cgs.media.balloonShader);
			}
		}
	}

	// draw the face
	VectorClear(headAngles);
	headAngles[YAW] = 180;
	if (largeFormat) {
		CG_DrawHead(headx, y - (ICON_SIZE - SCORECHAR_HEIGHT) / 2, ICON_SIZE, ICON_SIZE,
				score->client, headAngles);
	} else {
		CG_DrawHead(headx, y, 16, 16, score->client, headAngles);
	}


	// highlight your position
	if (score->client == cg.snap->ps.clientNum) {
		float hcolor[4];
		int rank;

		localClient = qtrue;

		if ((cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) ||
				((cgs.gametype >= GT_TEAM) &&
				(cgs.ffa_gt != 1))) {
			// Sago: I think this means that it doesn't matter if two players are tied in team game - only team score counts
			rank = -1;
		} else {
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		}
		if (rank == 0) {
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;
		} else if (rank == 1) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;
		} else if (rank == 2) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;
		} else {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0.7f;
		}

		hcolor[3] = fade * 0.7;
		CG_FillRect(RATSB_SCORELINE_X + SCORECHAR_WIDTH + (RATSB_RATING_WIDTH / 2), y,
				640 - RATSB_SCORELINE_X - SCORECHAR_WIDTH, SCORECHAR_HEIGHT + 1, hcolor);
	}
	// draw the score line ===========
	tcolor[3] = fade;
	tcolor[0] = tcolor[1] = tcolor[2] = 1.0;
	if (score->ping == -1) {
		Com_sprintf(string, sizeof (string), "connecting ");
		CG_DrawSmallScoreStringColor(RATSB_WINS_X, ysmall, string, tcolor);
	} else if (ci->team == TEAM_SPECTATOR) {
		const char *team_s = NULL;
		tcolor[0] = tcolor[1] = tcolor[2] = 1.0;
		switch (score->spectatorGroup) {
			case SPECTATORGROUP_NOTREADY:
				team_s = "NOTREADY";
				break;
			case SPECTATORGROUP_AFK:
				team_s = "AFK";
				break;
			default:
				team_s = "SPECT";
				break;
		}
		Com_sprintf(string, sizeof (string), "%9s", team_s);
		CG_DrawSmallScoreStringColor(
				RATSB_SCORE_X+RATSB_SCORE_WIDTH - CG_DrawStrlen(string) * SCORESMALLCHAR_WIDTH,
			       	ysmall, string, tcolor);
	} else {
		tcolor[0] = tcolor[2] = 0.5;
		tcolor[1] = 1.0;
		if (cgs.gametype == GT_TOURNAMENT) {
			Com_sprintf(string, sizeof (string), "%2i", ci->wins);
		} else if (cgs.gametype == GT_CTF) {
			Com_sprintf(string, sizeof (string), "%2i", score->captures);
		} else {
			Com_sprintf(string, sizeof (string), "  ");
		}
		CG_DrawSmallScoreStringColor(RATSB_WINS_X, ysmall, string, tcolor);

		tcolor[0] = tcolor[1] = tcolor[2] = 1.0;
		if (cgs.gametype == GT_TOURNAMENT 
				|| cgs.gametype == GT_CTF) {
			Com_sprintf(string, sizeof (string), "/");
		} else {
			Com_sprintf(string, sizeof (string), " ");
		}
		CG_DrawSmallScoreStringColor(RATSB_WL_X, ysmall, string, tcolor);

		if (cgs.gametype == GT_TOURNAMENT) {
			tcolor[1] = tcolor[2] = 0.5;
			tcolor[2] = 1.0;
			Com_sprintf(string, sizeof (string), "%-2i", ci->losses);
		} else if (cgs.gametype == GT_CTF) {
			tcolor[0] = tcolor[2] = 0.25;
			tcolor[1] = 0.75;
			Com_sprintf(string, sizeof (string), "%-2i", score->flagrecovery);
		} else {
			Com_sprintf(string, sizeof (string), "  ");
		}
		CG_DrawSmallScoreStringColor(RATSB_LOSSES_X, ysmall, string, tcolor);

		tcolor[0] = tcolor[1] = 1.0;
		tcolor[2] = 0;
		Com_sprintf(string, sizeof (string), "%4i", score->score);
		CG_DrawScoreStringColor(RATSB_SCORE_X, y, string, tcolor);
	}

	tcolor[0] = tcolor[1] = tcolor[2] = 1.0;
	Com_sprintf(string, sizeof (string), "%3i", score->time);
	CG_DrawSmallScoreStringColor(RATSB_TIME_X, ysmall, string, tcolor);

	tcolor[0] = 0;
	tcolor[1] = 0.45;
	tcolor[2] = 1.0;
	Com_sprintf(string, sizeof (string), "%2i", score->client);
	CG_DrawSmallScoreStringColor(RATSB_CNUM_X, ysmall, string, tcolor);

	tcolor[0] = tcolor[1] = tcolor[2] = 1.0;
	Com_sprintf(string, sizeof (string), "%s", ci->name);
	CG_DrawScoreString(RATSB_NAME_X, y, string, fade, RATSB_NAME_LENGTH);

	CG_RatioColor(score->kills, score->deaths, tcolor);
	Com_sprintf(string, sizeof (string), "%3i/%-3i", score->kills, score->deaths);
	CG_DrawTinyScoreStringColor(RATSB_KD_X, ytiny, string, tcolor);

	CG_RatioColor(score->dmgGiven, score->dmgTaken, tcolor);
	Com_sprintf(string, sizeof (string), "%2.1f/%-2.1f",
			(double)score->dmgGiven/1000.0, (double)score->dmgTaken/1000.0);
	CG_DrawTinyScoreStringColor(RATSB_DT_X, ytiny, string, tcolor);

	tcolor[0] = tcolor[1] = tcolor[2] = 0.80;
	Com_sprintf(string, sizeof (string), "%3i%%", score->accuracy);
	CG_DrawTinyScoreStringColor(RATSB_ACCURACY_X, ysmall, string, tcolor);

	//tcolor[0] = tcolor[1] = tcolor[2] = 1.0;
	CG_PingColor(score->ping, tcolor);
	Com_sprintf(string, sizeof (string), "%3i", score->ping);
	CG_DrawSmallScoreStringColor(RATSB_PING_X, ysmall, string, tcolor);

	
	//if (score->ping == -1) {
	//	Com_sprintf(string, sizeof (string),
	//			"       connecting    ^5%2i  %s", score->client, ci->name);
	//} else if (ci->team == TEAM_SPECTATOR) {
	//	Com_sprintf(string, sizeof (string),
	//			"       SPECT %3i ^2%3i ^5%2i  %s", score->ping, score->time, score->client, ci->name);
	//} else {
	//	Com_sprintf(string, sizeof (string),
	//			"^3%2i^7/^3%-2i^7 %5i %4i ^2%3i ^5%2i  %s %i", ci->wins, ci->losses, score->score, score->ping, score->time, score->client, ci->name, score->accuracy);
	//}

	//CG_DrawScoreString(RATSB_SCORELINE_X + (RATSB_RATING_WIDTH / 2), y, string, fade);
	// ===================

	// add the "ready" marker for intermission exiting
	if (cg.snap->ps.stats[ STAT_CLIENTS_READY ] & (1 << score->client)) {
		CG_DrawScoreStringColor(iconx, y, "READY", color);
	} else
		if (cgs.gametype == GT_LMS) {
		//CG_DrawScoreStringColor(iconx - 50, y, va("*%i*", ci->isDead), color);
		CG_DrawScoreStringColor(iconx, y, va("*%i*", ci->isDead), color);
	} else if (ci->isDead) {
		//CG_DrawScoreStringColor(iconx - 60, y, "DEAD", color);
		CG_DrawScoreStringColor(iconx, y, "DEAD", color);
	}

	if( cg.warmup < 0 && ci->team != TEAM_SPECTATOR && cgs.startWhenReady ){
		if( cg.readyMask & ( 1 << score->client ) ){
			color[0] = 0;
			color[1] = 1;
			color[2] = 0;
			CG_DrawScoreStringColor(iconx, y, "READY", color);
		}
	}
}

/*
=================
CG_RatTeamScoreboard
=================
 */
static int CG_RatTeamScoreboard(int y, team_t team, float fade, int maxClients, int lineHeight, qboolean countOnly) {
	int i;
	score_t *score;
	float color[4];
	int count;
	clientInfo_t *ci;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;

	count = 0;
	for (i = 0; i < cg.numScores && count < maxClients; i++) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if (team != ci->team) {
			continue;
		}

		if (!countOnly) {
			CG_RatDrawClientScore(y + lineHeight * count, score, color, fade, lineHeight == RATSB_NORMAL_HEIGHT);
		}

		count++;
	}

	return count;
}


/*
=================
CG_DrawRatboard

Draw the normal in-game scoreboard
=================
 */
qboolean CG_DrawRatScoreboard(void) {
	int x, y, w, i, n1, n2, len;
	float fade;
	float *fadeColor;
	vec4_t color;
	char *s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;

	// don't draw amuthing if the menu or console is up
	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if (cg.warmup && !cg.showScores) {
		return qfalse;
	}

	if (cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD ||
			cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor(cg.scoreFadeTime, FADE_TIME);

		if (!fadeColor) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}

	if ( cg.scoresRequestTime + 1000 < cg.time ) {
		// the scores are more than 1s out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );
	}


	// fragged by ... line
	if (cg.killerName[0]) {
		s = va("%sFragged by%s %s",
			       	S_COLOR_YELLOW, S_COLOR_WHITE, cg.killerName);
		w = CG_DrawStrlen(s) * SCORECHAR_WIDTH;
		x = (SCREEN_WIDTH - w) / 2;
		y = RATSB_HEADER-56;
		CG_DrawScoreString(x, y, s, fade, 0);
	}

	// current rank
	if (cgs.gametype < GT_TEAM || cgs.ffa_gt == 1) {
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
			s = va("%s place with %i",
					CG_PlaceString(cg.snap->ps.persistant[PERS_RANK] + 1),
					cg.snap->ps.persistant[PERS_SCORE]);
			w = CG_DrawStrlen(s) * SCORECHAR_WIDTH;
			x = (SCREEN_WIDTH - w) / 2;
			y = RATSB_HEADER - 36;
			CG_DrawScoreString(x, y, s, fade, 0);
		}

		if (cgs.gametype != GT_TOURNAMENT) {
			s = va("%i players", CG_CountPlayers(TEAM_FREE));
			w = CG_DrawStrlen(s) * SCORESMALLCHAR_WIDTH;
			x = (SCREEN_WIDTH - w) / 2;
			y = RATSB_HEADER - 16;
			CG_DrawSmallScoreString(x, y, s, 0.6);
		}
	} else {
		int redCount = CG_CountPlayers(TEAM_RED);
		int blueCount = CG_CountPlayers(TEAM_BLUE);
		if (cg.teamScores[0] == cg.teamScores[1]) {
			s = va("Teams are tied at %i", cg.teamScores[0]);
		} else if (cg.teamScores[0] >= cg.teamScores[1]) {
			s = va("^1Red^7 leads ^1%i^7 to ^4%i", cg.teamScores[0], cg.teamScores[1]);
		} else {
			s = va("^4Blue^7 leads ^4%i^7 to ^1%i", cg.teamScores[1], cg.teamScores[0]);
		}

		w = CG_DrawStrlen(s) * SCORECHAR_WIDTH;
		x = (SCREEN_WIDTH - w) / 2;
		y = RATSB_HEADER - 36;
		CG_DrawScoreString(x, y, s, fade, 0);

		if (cg.teamScores[0] >= cg.teamScores[1]) {
			s = va("%ivs%i", redCount, blueCount);
		} else {
			s = va("%ivs%i", blueCount, redCount);
		}
		w = CG_DrawStrlen(s) * SCORESMALLCHAR_WIDTH;
		x = (SCREEN_WIDTH - w) / 2;
		y = RATSB_HEADER - 16;
		CG_DrawSmallScoreString(x, y, s, 0.6);
	}

	// draw gametype
	if ( cgs.gametype == GT_FFA ) {
		s = "Free For All";
	} else if ( cgs.gametype == GT_TOURNAMENT ) {
		s = "Tournament";
	} else if ( cgs.gametype == GT_TEAM ) {
		s = "Team Deathmatch";
	} else if ( cgs.gametype == GT_CTF ) {
		s = "Capture the Flag";
	} else if ( cgs.gametype == GT_ELIMINATION ) {
		s = "Elimination";
	} else if ( cgs.gametype == GT_CTF_ELIMINATION ) {
		s = "CTF Elimination";
	} else if ( cgs.gametype == GT_LMS ) {
		s = "Last Man Standing";
	} else if ( cgs.gametype == GT_DOUBLE_D ) {
		s = "Double Domination";
	} else if ( cgs.gametype == GT_1FCTF ) {
		s = "One Flag CTF";
	} else if ( cgs.gametype == GT_OBELISK ) {
		s = "Overload";
	} else if ( cgs.gametype == GT_HARVESTER ) {
		s = "Harvester";
          } else if ( cgs.gametype == GT_DOMINATION ) {
		s = "Domination";
	} else {
		s = "";
	}

	len = CG_DrawStrlen(s);
	if (len > 20) {
		len = 20;
	}
	w = len * SCORETINYCHAR_WIDTH;
	x = (RATSB_PING_X+RATSB_PING_WIDTH - w);
	y = RATSB_HEADER - 26 - SCORETINYCHAR_HEIGHT;
	memcpy(color, colorGreen, sizeof(color));
	color[3] = fade;
	CG_DrawTinyScoreStringColor(x, y, s, color);

	// draw map name
	s = va("%s", cgs.mapbasename);
	len = CG_DrawStrlen(s);
	if (len > 20) {
		len = 20;
	}
	w = len * SCORETINYCHAR_WIDTH;
	x = (RATSB_PING_X+RATSB_PING_WIDTH - w);
	y = RATSB_HEADER - 26;
	memcpy(color, colorCyan, sizeof(color));
	color[3] = fade;
	CG_DrawTinyScoreStringColor(x, y, s, color);

	// scoreboard
	y = RATSB_HEADER;

	if (cgs.gametype == GT_TOURNAMENT) {
		CG_DrawTinyScoreString(RATSB_WL_CENTER-1.5*SCORETINYCHAR_WIDTH, y, "W/L", fade);
	} else if (cgs.gametype == GT_CTF) {
		CG_DrawTinyScoreString(RATSB_WL_CENTER-1.5*SCORETINYCHAR_WIDTH, y, "C/R", fade);
	}
	CG_DrawTinyScoreString(RATSB_SCORE_CENTER - 2.5*SCORETINYCHAR_WIDTH, y, "Score", fade);
	CG_DrawTinyScoreString(RATSB_TIME_CENTER - 2 * SCORETINYCHAR_WIDTH, y, "Time", fade);
	CG_DrawTinyScoreString(RATSB_CNUM_CENTER - SCORETINYCHAR_WIDTH, y, "CN", fade);
	CG_DrawTinyScoreString(RATSB_NAME_X, y, "Name", fade);
	CG_DrawTinyScoreString(RATSB_KD_CENTER - 1.5 * SCORETINYCHAR_WIDTH, y, "K/D", fade);
	CG_DrawTinyScoreString(RATSB_DT_CENTER - 2.5 * SCORETINYCHAR_WIDTH, y, "DG/DT", fade);
	CG_DrawTinyScoreString(RATSB_ACCURACY_CENTER - 1.5 * SCORETINYCHAR_WIDTH, y, "Acc", fade);
	//CG_DrawTinyScoreString(RATSB_ACCURACY_CENTER - 1.5 * SCORETINYCHAR_WIDTH, y, "Acc", fade);
	CG_DrawTinyScoreString(RATSB_PING_CENTER - 2 * SCORETINYCHAR_WIDTH, y, "Ping", fade);

	y = RATSB_TOP;

	maxClients = RATSB_MAXCLIENTS_INTER;
	lineHeight = RATSB_INTER_HEIGHT;
	topBorderSize = 8;
	bottomBorderSize = 16;

	localClient = qfalse;

	if (cgs.gametype >= GT_TEAM && cgs.ffa_gt != 1) {
		//
		// teamplay scoreboard
		//
		y += lineHeight / 2;


		if (cg.teamScores[0] >= cg.teamScores[1]) {
			n1 = CG_RatTeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight, qtrue);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED);
			n1 = CG_RatTeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight, qfalse);
			y += (n1 * lineHeight) + SCORECHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_RatTeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight, qtrue);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE);
			n2 = CG_RatTeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight, qfalse);
			y += (n2 * lineHeight) + SCORECHAR_HEIGHT;
			maxClients -= n2;
		} else {
			n1 = CG_RatTeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight, qtrue);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE);
			n1 = CG_RatTeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight, qfalse);
			y += (n1 * lineHeight) + SCORECHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_RatTeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight, qtrue);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED);
			n2 = CG_RatTeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight, qfalse);
			y += (n2 * lineHeight) + SCORECHAR_HEIGHT;
			maxClients -= n2;
		}
		n1 = CG_RatTeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients, lineHeight, qfalse);
		y += (n1 * lineHeight) + SCORECHAR_HEIGHT;

	} else {
		//
		// free for all scoreboard
		//
		n1 = CG_RatTeamScoreboard(y, TEAM_FREE, fade, maxClients, lineHeight, qfalse);
		y += (n1 * lineHeight) + SCORECHAR_HEIGHT;
		n2 = CG_RatTeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight, qfalse);
		y += (n2 * lineHeight) + SCORECHAR_HEIGHT;
	}

	if (!localClient) {
		// draw local client at the bottom
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].client == cg.snap->ps.clientNum) {
				CG_RatDrawClientScore(y, &cg.scores[i], fadeColor, fade, lineHeight == RATSB_NORMAL_HEIGHT);
				break;
			}
		}
	}

	// load any models that have been deferred
	if (++cg.deferredPlayerLoading > 10) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
}

							 /*
=================
CG_DrawScoreboard
=================
*/
static void CG_DrawClientScore( int y, score_t *score, float *color, float fade, qboolean largeFormat ) {
	char	string[1024];
	vec3_t	headAngles;
	clientInfo_t	*ci;
	int iconx, headx;

	if ( score->client < 0 || score->client >= cgs.maxclients ) {
		Com_Printf( "Bad score->client: %i\n", score->client );
		return;
	}
	
	ci = &cgs.clientinfo[score->client];

	iconx = SB_BOTICON_X + (SB_RATING_WIDTH / 2);
	headx = SB_HEAD_X + (SB_RATING_WIDTH / 2);

	// draw the handicap or bot skill marker (unless player has flag)
	if ( ci->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_FREE, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 16, 16, TEAM_FREE, qfalse );
		}
	} else if ( ci->powerups & ( 1 << PW_REDFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_RED, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 16, 16, TEAM_RED, qfalse );
		}
	} else if ( ci->powerups & ( 1 << PW_BLUEFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_BLUE, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 16, 16, TEAM_BLUE, qfalse );
		}
	} else {
		if ( ci->botSkill > 0 && ci->botSkill <= 5 ) {
			if ( cg_drawIcons.integer ) {
				if( largeFormat ) {
					CG_DrawPic( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, cgs.media.botSkillShaders[ ci->botSkill - 1 ] );
				}
				else {
					CG_DrawPic( iconx, y, 16, 16, cgs.media.botSkillShaders[ ci->botSkill - 1 ] );
				}
			}
		} else if ( ci->handicap < 100 ) {
			Com_sprintf( string, sizeof( string ), "%i", ci->handicap );
			if ( cgs.gametype == GT_TOURNAMENT )
				CG_DrawSmallStringColor( iconx, y - SMALLCHAR_HEIGHT/2, string, color );
			else
				CG_DrawSmallStringColor( iconx, y, string, color );
		}

		// draw the wins / losses
		if ( cgs.gametype == GT_TOURNAMENT ) {
			Com_sprintf( string, sizeof( string ), "%i/%i", ci->wins, ci->losses );
			if( ci->handicap < 100 && !ci->botSkill ) {
				CG_DrawSmallStringColor( iconx, y + SMALLCHAR_HEIGHT/2, string, color );
			}
			else {
				CG_DrawSmallStringColor( iconx, y, string, color );
			}
		}
	}

	// draw the face
	VectorClear( headAngles );
	headAngles[YAW] = 180;
	if( largeFormat ) {
		CG_DrawHead( headx, y - ( ICON_SIZE - BIGCHAR_HEIGHT ) / 2, ICON_SIZE, ICON_SIZE, 
			score->client, headAngles );
	}
	else {
		CG_DrawHead( headx, y, 16, 16, score->client, headAngles );
	}

#ifdef MISSIONPACK
	// draw the team task
	if ( ci->teamTask != TEAMTASK_NONE ) {
                if (ci->isDead) {
                    CG_DrawPic( headx + 48, y, 16, 16, cgs.media.deathShader );
                }
                else if ( ci->teamTask == TEAMTASK_OFFENSE ) {
			CG_DrawPic( headx + 48, y, 16, 16, cgs.media.assaultShader );
		}
		else if ( ci->teamTask == TEAMTASK_DEFENSE ) {
			CG_DrawPic( headx + 48, y, 16, 16, cgs.media.defendShader );
		}
	}
#endif
	// draw the score line
	if ( score->ping == -1 ) {
		Com_sprintf(string, sizeof(string),
			" connecting    %s", ci->name);
	} else if ( ci->team == TEAM_SPECTATOR ) {
		Com_sprintf(string, sizeof(string),
			" SPECT %3i %4i %s", score->ping, score->time, ci->name);
	} else {
		/*if(cgs.gametype == GT_LMS)
			Com_sprintf(string, sizeof(string),
				"%5i %4i %4i %s *%i*", score->score, score->ping, score->time, ci->name, ci->isDead);
		else*/
		/*if(ci->isDead)
			Com_sprintf(string, sizeof(string),
				"%5i %4i %4i %s *DEAD*", score->score, score->ping, score->time, ci->name);
		else*/
			Com_sprintf(string, sizeof(string),
				"%5i %4i %4i %s", score->score, score->ping, score->time, ci->name);
	}

	// highlight your position
	if ( score->client == cg.snap->ps.clientNum ) {
		float	hcolor[4];
		int		rank;

		localClient = qtrue;

		if ( ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) ||
			( ( cgs.gametype >= GT_TEAM ) &&
			( cgs.ffa_gt != 1 ) ) ) {
			// Sago: I think this means that it doesn't matter if two players are tied in team game - only team score counts
			rank = -1;
		} else {
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		}
		if ( rank == 0 ) {
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;
		} else if ( rank == 1 ) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;
		} else if ( rank == 2 ) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;
		} else {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0.7f;
		}

		hcolor[3] = fade * 0.7;
		CG_FillRect( SB_SCORELINE_X + BIGCHAR_WIDTH + (SB_RATING_WIDTH / 2), y, 
			640 - SB_SCORELINE_X - BIGCHAR_WIDTH, BIGCHAR_HEIGHT+1, hcolor );
	}

	CG_DrawBigString( SB_SCORELINE_X + (SB_RATING_WIDTH / 2), y, string, fade );

	// add the "ready" marker for intermission exiting
	if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << score->client ) ) {
		CG_DrawBigStringColor( iconx, y, "READY", color );
	} else
        if(cgs.gametype == GT_LMS) {
            CG_DrawBigStringColor( iconx-50, y, va("*%i*",ci->isDead), color );
        } else
        if(ci->isDead) {
            CG_DrawBigStringColor( iconx-60, y, "DEAD", color );
        }
}

/*
=================
CG_TeamScoreboard
=================
*/
static int CG_TeamScoreboard( int y, team_t team, float fade, int maxClients, int lineHeight ) {
	int		i;
	score_t	*score;
	float	color[4];
	int		count;
	clientInfo_t	*ci;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;

	count = 0;
	for ( i = 0 ; i < cg.numScores && count < maxClients ; i++ ) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if ( team != ci->team ) {
			continue;
		}

		CG_DrawClientScore( y + lineHeight * count, score, color, fade, lineHeight == SB_NORMAL_HEIGHT );

		count++;
	}

	return count;
}

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawOldScoreboard( void ) {
	int		x, y, w, i, n1, n2;
	float	fade;
	float	*fadeColor;
	char	*s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;

	// don't draw amuthing if the menu or console is up
	if ( cg_paused.integer ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	if ( cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD ||
		 cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );
		
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}


	// fragged by ... line
	if ( cg.killerName[0] ) {
		s = va("Fragged by %s", cg.killerName );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 40;
		CG_DrawBigString( x, y, s, fade );
	}

	// current rank
	if ( cgs.gametype < GT_TEAM || cgs.ffa_gt == 1) {
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			s = va("%s place with %i",
				CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),
				cg.snap->ps.persistant[PERS_SCORE] );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
			x = ( SCREEN_WIDTH - w ) / 2;
			y = 60;
			CG_DrawBigString( x, y, s, fade );
		}
	} else {
		if ( cg.teamScores[0] == cg.teamScores[1] ) {
			s = va("Teams are tied at %i", cg.teamScores[0] );
		} else if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			s = va("Red leads %i to %i",cg.teamScores[0], cg.teamScores[1] );
		} else {
			s = va("Blue leads %i to %i",cg.teamScores[1], cg.teamScores[0] );
		}

		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 60;
		CG_DrawBigString( x, y, s, fade );
	}

	// scoreboard
	y = SB_HEADER;

	CG_DrawPic( SB_SCORE_X + (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardScore );
	CG_DrawPic( SB_PING_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardPing );
	CG_DrawPic( SB_TIME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardTime );
	CG_DrawPic( SB_NAME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardName );

	y = SB_TOP;

	// If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores
	if ( cg.numScores > SB_MAXCLIENTS_NORMAL ) {
		maxClients = SB_MAXCLIENTS_INTER;
		lineHeight = SB_INTER_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 16;
	} else {
		maxClients = SB_MAXCLIENTS_NORMAL;
		lineHeight = SB_NORMAL_HEIGHT;
		topBorderSize = 16;
		bottomBorderSize = 16;
	}

	localClient = qfalse;

	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1) {
		//
		// teamplay scoreboard
		//
		y += lineHeight/2;

		if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			n1 = CG_TeamScoreboard( y, TEAM_RED, fade, maxClients, lineHeight );
			CG_DrawTeamBackground( 0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard( y, TEAM_BLUE, fade, maxClients, lineHeight );
			CG_DrawTeamBackground( 0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		} else {
			n1 = CG_TeamScoreboard( y, TEAM_BLUE, fade, maxClients, lineHeight );
			CG_DrawTeamBackground( 0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard( y, TEAM_RED, fade, maxClients, lineHeight );
			CG_DrawTeamBackground( 0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		}
		n1 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients, lineHeight );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

	} else {
		//
		// free for all scoreboard
		//
		n1 = CG_TeamScoreboard( y, TEAM_FREE, fade, maxClients, lineHeight );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		n2 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight );
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
	}

	if (!localClient) {
		// draw local client at the bottom
		for ( i = 0 ; i < cg.numScores ; i++ ) {
			if ( cg.scores[i].client == cg.snap->ps.clientNum ) {
				CG_DrawClientScore( y, &cg.scores[i], fadeColor, fade, lineHeight == SB_NORMAL_HEIGHT );
				break;
			}
		}
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
}

//================================================================================

/*
================
CG_CenterGiantLine
================
*/
static void CG_CenterGiantLine( float y, const char *string ) {
	float		x;
	vec4_t		color;

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	x = 0.5 * ( 640 - GIANT_WIDTH * CG_DrawStrlen( string ) );

	CG_DrawStringExt( x, y, string, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
}

/*
=================
CG_DrawTourneyScoreboard

Draw the oversize scoreboard for tournements
=================
*/
void CG_DrawOldTourneyScoreboard( void ) {
	const char		*s;
	vec4_t			color;
	int				min, tens, ones;
	clientInfo_t	*ci;
	int				y;
	int				i;

	// request more scores regularly
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );
	}

	// draw the dialog background
	color[0] = color[1] = color[2] = 0;
	color[3] = 1;
	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	// print the mesage of the day
	s = CG_ConfigString( CS_MOTD );
	if ( !s[0] ) {
		s = "Scoreboard";
	}

	// print optional title
	CG_CenterGiantLine( 8, s );

	// print server time
	ones = cg.time / 1000;
	min = ones / 60;
	ones %= 60;
	tens = ones / 10;
	ones %= 10;
	s = va("%i:%i%i", min, tens, ones );

	CG_CenterGiantLine( 64, s );


	// print the two scores

	y = 160;
	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1) {
		//
		// teamplay scoreboard
		//
		CG_DrawStringExt( 8, y, "Red Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
		s = va("%i", cg.teamScores[0] );
		CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
		
		y += 64;

		CG_DrawStringExt( 8, y, "Blue Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
		s = va("%i", cg.teamScores[1] );
		CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
	} else {
		//
		// free for all scoreboard
		//
		for ( i = 0 ; i < MAX_CLIENTS ; i++ ) {
			ci = &cgs.clientinfo[i];
			if ( !ci->infoValid ) {
				continue;
			}
			if ( ci->team != TEAM_FREE ) {
				continue;
			}

			CG_DrawStringExt( 8, y, ci->name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
			s = va("%i", ci->score );
			CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
			y += 64;
		}
	}


}

