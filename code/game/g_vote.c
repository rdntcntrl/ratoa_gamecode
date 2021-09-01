/*
===========================================================================
Copyright (C) 2008-2009 Poul Sander

This file is part of Open Arena source code.

Open Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Open Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Open Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

/*
==================
allowedVote
 *Note: Keep this in sync with allowedVote in ui_votemenu.c (except for cg_voteNames and g_voteNames)
==================
 */
#define MAX_VOTENAME_LENGTH 15 //currently the longest string is "/capturelimit/\0" (15 chars)
int allowedVote(char *commandStr) {
    char tempStr[MAX_VOTENAME_LENGTH];
    int length;
    char voteNames[MAX_CVAR_VALUE_STRING];
    trap_Cvar_VariableStringBuffer( "g_voteNames", voteNames, sizeof( voteNames ) );
    if(!Q_stricmp(voteNames, "*" ))
        return qtrue; //if star, everything is allowed
    length = strlen(commandStr);
    if(length>MAX_VOTENAME_LENGTH-3)
    {
        //Error: too long
        return qfalse;
    }
    //Now constructing a string that starts and ends with '/' like: "/clientkick/"
    tempStr[0] = '/';
    strncpy(&tempStr[1],commandStr,length);
    tempStr[length+1] = '/';
    tempStr[length+2] = '\0';
    if(Q_stristr(voteNames,tempStr) != NULL)
        return qtrue;
    else
        return qfalse;
}

/*
==================
getMappage
==================
 */

t_mappage getMappage(int page, qboolean largepage, qboolean recommenedonly) {
	t_mappage result;
	fileHandle_t	file;
	char *token,*pointer;
	char *buffer;
	int i, nummaps,maplen;
	int maps_in_page = largepage ? MAPS_PER_LARGEPAGE : MAPS_PER_PAGE;

	buffer = BG_Alloc(MAX_MAPNAME_BUFFER);

	memset(&result,0,sizeof(result));
        memset(buffer,0, MAX_MAPNAME_BUFFER);

	//Check if there is a votemaps.cfg
	trap_FS_FOpenFile(g_votemaps.string,&file,FS_READ);
	if (!file && recommenedonly) {
		// if no votemaps.cfg and we only want recommened maps, try recommenedmaps.cfg
		trap_FS_FOpenFile(g_recommendedMapsFile.string,&file,FS_READ);
	}
	if(file)
	{
		//there is a votemaps.cfg file, take allowed maps from there
		trap_FS_Read(buffer,MAX_MAPNAME_BUFFER-1,file);
		pointer = buffer;
		token = COM_Parse(&pointer);
		if(token[0]==0 && page == 0) {
			//First page empty
			result.pagenumber = -1;
                        trap_FS_FCloseFile(file);
			goto out;
		}
		//Skip the first pages
		for(i=0;i<maps_in_page*page;i++) {
			token = COM_Parse(&pointer);
		}
		if(!token || token[0]==0) {
			//Page empty, return to first page
                        trap_FS_FCloseFile(file);
			result = getMappage(0, largepage, recommenedonly);
			goto out;
		}
		//There is an actual page:
                result.pagenumber = page;
		for(i=0;i<maps_in_page && token;i++) {
			Q_strncpyz(result.mapname[i],token,MAX_MAPNAME);
			token = COM_Parse(&pointer);
		}
                trap_FS_FCloseFile(file);
		goto out;
	}
        //There is no votemaps.cfg file, find filelist of maps
        nummaps = trap_FS_GetFileList("maps",".bsp",buffer,MAX_MAPNAME_BUFFER);

        if(nummaps && nummaps<=maps_in_page*page) {
            result = getMappage(0, largepage, recommenedonly);
	    goto out;
	}

        pointer = buffer;
        result.pagenumber = page;

        for (i = 0; i < nummaps; i++, pointer += maplen+1) {
		int copylen = 0;
		maplen = strlen(pointer);
		copylen = maplen-3;
		if (copylen > MAX_MAPNAME) {
			copylen = MAX_MAPNAME;
		}
                if(i>=maps_in_page*page && i<maps_in_page*(page+1)) {
                    Q_strncpyz(result.mapname[i-maps_in_page*page],pointer,copylen);
                }
	}
out:
	BG_Free(buffer);
        return result;
}

t_mappage getGTMappage(int page, qboolean largepage) {
	t_mappage result;
	struct maplist_s *maplist;
	int maps_in_page = largepage ? MAPS_PER_LARGEPAGE : MAPS_PER_PAGE;
	int i;
	int start;

	maplist = BG_Alloc(sizeof(*maplist));

	// FIXME: not very optimized, this gets called for every page and
	// assembles the complete list each time
	getCompleteMaplist(qfalse, G_GametypeBitsCurrent(), 0, maplist);

	memset(&result,0,sizeof(result));
	result.pagenumber = page;

	start = maps_in_page * page;
	if (start >= maplist->num) {
		if (page > 0) {
			result = getGTMappage(0, largepage);
			goto out;
		}
		goto out;
	}

	for(i = 0; i < maps_in_page && i+start < maplist->num; ++i) {
		Q_strncpyz(result.mapname[i],maplist->mapname[i+start],MAX_MAPNAME);
	}

out:
	BG_Free(maplist);
	return result;
}

static qboolean MapFilter(const char *mapname, int gametypebits_filter, int numPlayers) {
	if ((gametypebits_filter != 0 && !(gametypebits_filter & G_GametypeBitsForMap(mapname)))) {
		return qfalse;
	}
	if (numPlayers > 0) {
		int minPlayers = 0, maxPlayers = 0;
		G_MapMinMaxPlayers(mapname, &minPlayers, &maxPlayers);
		if ((minPlayers > 0 && numPlayers < minPlayers) || (maxPlayers > 0 && numPlayers > maxPlayers)) {
			return qfalse;
		}
	}
	return qtrue;
}

void getCompleteMaplist(qboolean recommenedonly, int gametypebits_filter, int numPlayers, struct maplist_s *out) {
	fileHandle_t	file;
	char *token,*pointer;
	char buffer[MAX_MAPNAME_BUFFER];
	int nummaps, maplen;

	memset(out,0,sizeof(*out));
        memset(&buffer,0,sizeof(buffer));

	//Check if there is a votemaps.cfg
	trap_FS_FOpenFile(g_votemaps.string,&file,FS_READ);
	if (!file && recommenedonly) {
		// if no votemaps.cfg and we only want recommened maps, try recommenedmaps.cfg
		trap_FS_FOpenFile(g_recommendedMapsFile.string,&file,FS_READ);
	}

	if (gametypebits_filter != 0 || numPlayers != 0) {
		G_LoadArenas();
	}

	if(file)
	{
		//there is a recommendedmaps file or a votemaps.cfg file, take allowed maps from there
		trap_FS_Read(&buffer,sizeof(buffer)-1,file);
		pointer = buffer;
		token = COM_Parse(&pointer);
		while (token && token[0] != 0 && out->num < MAX_MAPS) {
			if (MapFilter(token, gametypebits_filter, numPlayers)) {
				Q_strncpyz(out->mapname[out->num],token,MAX_MAPNAME);
				out->num++;
			}
			token = COM_Parse(&pointer);
		}
                trap_FS_FCloseFile(file);
		return;
	}
	if (!file && recommenedonly) {
		return;
	}

        nummaps = trap_FS_GetFileList("maps",".bsp",buffer,sizeof(buffer));
        pointer = buffer;

	if (nummaps > MAX_MAPS) {
		nummaps = MAX_MAPS;
	}
        for (out->num = 0; (out->num < nummaps && *pointer); pointer += maplen+1) {
		char mapname[MAX_MAPNAME];
		int copylen = 0;

		maplen = strlen(pointer);
		copylen = maplen-3;
		if (copylen > MAX_MAPNAME) {
			copylen = MAX_MAPNAME;
		}
		Q_strncpyz(mapname, pointer, copylen);
		if (MapFilter(mapname, gametypebits_filter, numPlayers)) {
			Q_strncpyz(out->mapname[out->num],pointer,copylen);
			out->num++;
		}
	}
        return;
}

/*
==================
allowedMap
==================
 */

int allowedMap(char *mapname) {
    int length;
    fileHandle_t	file;           //To check that the map actually exists.
    char                buffer[MAX_MAPS_TEXT];
    char                *token,*pointer;
    qboolean            found;

    trap_FS_FOpenFile(va("maps/%s.bsp",mapname),&file,FS_READ);
    if(!file)
        return qfalse; //maps/MAPNAME.bsp does not exist
    trap_FS_FCloseFile(file);

    //Now read the file votemaps.cfg to see what maps are allowed
    trap_FS_FOpenFile(g_votemaps.string,&file,FS_READ);

    if(!file)
        return qtrue; //if no file, everything is allowed
    length = strlen(mapname);
    if(length>MAX_MAPNAME_LENGTH-3)
    {
        //Error: too long
        trap_FS_FCloseFile(file);
        return qfalse;
    }

    memset(buffer, 0, sizeof(buffer));

    //Add file checking here
    trap_FS_Read(&buffer,MAX_MAPS_TEXT-1,file);
    found = qfalse;
    pointer = buffer;
    token = COM_Parse(&pointer);
    while(token[0]!=0 && !found) {
        if(!Q_stricmp(token,mapname))
            found = qtrue;
        token = COM_Parse(&pointer);
    }

    trap_FS_FCloseFile(file);
    //The map was not found
    return found;
}

/*
==================
allowedGametype
==================
 */
#define MAX_GAMETYPENAME_LENGTH 5 //currently the longest string is "/12/\0" (5 chars)
int allowedGametype(char *gametypeStr) {
    char tempStr[MAX_GAMETYPENAME_LENGTH];
    int length;
    char voteGametypes[MAX_CVAR_VALUE_STRING];
    trap_Cvar_VariableStringBuffer( "g_voteGametypes", voteGametypes, sizeof( voteGametypes ) );
    if(!Q_stricmp(voteGametypes, "*" ))
        return qtrue; //if star, everything is allowed
    length = strlen(gametypeStr);
    if(length>MAX_GAMETYPENAME_LENGTH-3)
    {
        //Error: too long
        return qfalse;
    }
    tempStr[0] = '/';
    strncpy(&tempStr[1],gametypeStr,length);
    tempStr[length+1] = '/';
    tempStr[length+2] = '\0';
    if(Q_stristr(voteGametypes,tempStr) != NULL)
        return qtrue;
    else {
        return qfalse;
    }
}

// these are a bit arbitrary, but do prevent some integer overruns
#define MAX_TIMELIMIT (60*24)
#define MAX_FRAGLIMIT (1 << 16)
#define MAX_CAPTURELIMIT (1 << 16)
/*
==================
allowedTimelimit
==================
 */
int allowedTimelimit(int limit) {
    int min, max;
    min = g_voteMinTimelimit.integer;
    max = g_voteMaxTimelimit.integer;
    if (limit > MAX_TIMELIMIT) {
	    // hard limit
	    return qfalse;
    }
    if(limit<min && limit != 0)
        return qfalse;
    if(max!=0 && limit>max)
        return qfalse;
    if(limit==0 && max > 0)
        return qfalse;
    return qtrue;
}

/*
==================
allowedBots
==================
 */
int allowedBots(int numbots) {
    int min, max;
    min = g_voteMinBots.integer;
    max = g_voteMaxBots.integer;
    if (numbots <= max && numbots >= min) {
	    return qtrue;
    }
    return qfalse;
}

/*
==================
allowedFraglimit
==================
 */
int allowedFraglimit(int limit) {
    int min, max;
    min = g_voteMinFraglimit.integer;
    max = g_voteMaxFraglimit.integer;
    if (limit > MAX_FRAGLIMIT) {
	    // hard limit
	    return qfalse;
    }
    if(limit<min && limit != 0)
        return qfalse;
    if(max != 0 && limit>max)
        return qfalse;
    if(limit==0 && max > 0)
        return qfalse;
    return qtrue;
}

int allowedCapturelimit(int limit) {
    int min, max;
    min = g_voteMinCapturelimit.integer;
    max = g_voteMaxCapturelimit.integer;
    if (limit > MAX_CAPTURELIMIT) {
	    // hard limit
	    return qfalse;
    }
    if(limit<min && limit != 0)
        return qfalse;
    if(max != 0 && limit>max)
        return qfalse;
    if(limit==0 && max > 0)
        return qfalse;
    return qtrue;
}


char *parseCustomVote(char *buf, t_customvote *result) {
	char	*token,*pointer;
	char	key[MAX_TOKEN_CHARS];

	pointer = buf;

	token = COM_Parse( &pointer );
        if ( !token[0] ) {
            return NULL;
	}

        if ( strcmp( token, "{" ) ) {
		Com_Printf( "Missing { in votecustom.cfg\n" );
		return NULL;
	}

        memset(result,0,sizeof(*result));

	result->lightvote = qtrue;
	result->passRatio = -1;

        while ( 1 ) {
            token = COM_ParseExt( &pointer, qtrue );
            if ( !token[0] ) {
                    Com_Printf( "Unexpected end of customvote.cfg\n" );
                    return NULL;
            }
            if ( !strcmp( token, "}" ) ) {
                    return pointer;
            }
            Q_strncpyz( key, token, sizeof( key ) );

            token = COM_ParseExt( &pointer, qfalse );
            if ( !token[0] ) {
                Com_Printf("Invalid/missing argument to %s in customvote.cfg\n",key);
            }
            if(!Q_stricmp(key,"votecommand")) {
                Q_strncpyz(result->votename,token,sizeof(result->votename));
            } else if(!Q_stricmp(key,"displayname")) {
                Q_strncpyz(result->displayname,token,sizeof(result->displayname));
            } else if(!Q_stricmp(key,"command")) {
                Q_strncpyz(result->command,token,sizeof(result->command));
            } else if(!Q_stricmp(key,"description")) {
                Q_strncpyz(result->description,token,sizeof(result->description));
            } else if(!Q_stricmp(key,"lightvote")) {
		result->lightvote = atoi(token) > 0 ? qtrue : qfalse;
            } else if(!Q_stricmp(key,"passratio")) {
		result->passRatio = atof(token);
		if (result->passRatio <= 0) {
			result->passRatio = -1;
		} else if (result->passRatio > 1.0) {
			result->passRatio = 1.0;
		}
            } else {
                Com_Printf("Unknown key in customvote.cfg: %s\n",key);
            }

	}
	return NULL;
}

/*
==================
getCustomVote
 *Returns a custom vote. This will go beyond MAX_CUSTOM_VOTES.
==================
 */
t_customvote getCustomVote(char* votecommand) {
    t_customvote result;
    fileHandle_t	file;
    char *pointer;
    char            buffer[128*1024];

    trap_FS_FOpenFile(g_votecustom.string,&file,FS_READ);

    if(!file) {
        memset(&result,0,sizeof(result));
        return result;
    }

    memset(&buffer,0,sizeof(buffer));

    trap_FS_Read(&buffer,sizeof(buffer)-1,file);

    pointer = buffer;

    while ( qtrue ) {
	if (!(pointer = parseCustomVote(pointer, &result))) {
		break;
	}

        if(!Q_stricmp(result.votename,votecommand)) {
            return result;
        }
    }

    //Nothing was found
    memset(&result,0,sizeof(result));
        return result;
}

#define MAX_CUSTOM_VOTES    48

char            custom_vote_info[2048];

/*
==================
VoteParseCustomVotes
 *Reads the file votecustom.cfg. Finds all the commands that can be used with
 *"/callvote custom COMMAND" and adds the commands to custom_vote_info
==================
 */
int VoteParseCustomVotes ( void ) {
    t_customvote result;
    fileHandle_t	file;
    char            buffer[128*1024];
    char *pointer;
    int numCommands = 0;

    memset(&custom_vote_info, 0, sizeof(custom_vote_info));

    trap_FS_FOpenFile(g_votecustom.string,&file,FS_READ);

    if(!file) {
        return numCommands;
    }

    memset(&buffer,0,sizeof(buffer));

    trap_FS_Read(&buffer,sizeof(buffer)-1,file);

    pointer = buffer;

    while ( numCommands < MAX_CUSTOM_VOTES ) {

	if (!(pointer = parseCustomVote(pointer, &result))) {
		break;
	}

	if (strlen(result.votename)) {
		Q_strcat(custom_vote_info,sizeof(custom_vote_info),va("%s ",result.votename));
		numCommands++;
	}



    }
    return numCommands;
}

/*
==================
VotePrintCustomVotes
==================
 */
int VotePrintCustomVotes (gentity_t *ent) {
    t_customvote result;
    fileHandle_t	file;
    char            buffer[128*1024];
    char *pointer;
    char printBuf[512];
    const char *delim = " - " S_COLOR_GREEN;
    const char *linestart = S_COLOR_WHITE;
    int numCommands = 0;


    trap_FS_FOpenFile(g_votecustom.string,&file,FS_READ);

    if(!file) {
        return numCommands;
    }

    memset(&buffer,0,sizeof(buffer));
    memset(&printBuf, 0, sizeof(printBuf));
    Q_strncpyz(printBuf, S_COLOR_CYAN "Custom vote commands are: \n", sizeof(printBuf));

    trap_FS_Read(&buffer,sizeof(buffer)-1,file);

    pointer = buffer;

    while ( numCommands < MAX_CUSTOM_VOTES ) {

	if (!(pointer = parseCustomVote(pointer, &result))) {
		break;
	}
	if (strlen(result.command) && strlen(result.votename)) {
		if (strlen(printBuf) + 
				strlen(linestart) +
				strlen(result.votename) + 
				(result.description[0] ? (strlen(delim) + strlen(result.description)) : 0)
			 	+ 2 >= sizeof(printBuf)) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\"", printBuf) );
			memset(&printBuf, 0, sizeof(printBuf));
		}
		Q_strcat(printBuf, sizeof(printBuf), linestart);
		Q_strcat(printBuf, sizeof(printBuf), result.votename);
		if (result.description[0]) {
			Q_strcat(printBuf, sizeof(printBuf), delim);
			Q_strcat(printBuf, sizeof(printBuf), result.description);
		}
		Q_strcat(printBuf, sizeof(printBuf), "\n");
		numCommands++;
	}

    }

    if (printBuf[0]) {
	    trap_SendServerCommand( ent-g_entities, va("print \"%s\"", printBuf) );
    }

    return numCommands;
}


void G_SendVoteResult(qboolean passed) {
	if (!g_usesRatVM.integer) {
		return;
	}
	trap_SendServerCommand( -1, va("vresult %s", passed ? "p" : "f"));
}

void G_SetVoteExecTime(void) {
	level.voteExecuteTime = level.realtime + 3000;
	// make sure vote gets executed if it passed just at the end of a warmup
	if (level.warmupTime > 0 && level.warmupTime <= level.voteExecuteTime) {
		level.voteExecuteTime = level.warmupTime - 3*1000.0/sv_fps.integer;
		if (level.voteExecuteTime <= level.realtime + 1000/sv_fps.integer) {
			level.voteExecuteTime = level.realtime;
		}
	}
}

#define VOTE_REJECTED_REPEAT_TIME (120*1000)
qboolean G_CheckRejectedVote(void) {
	return (level.lastFailedVoteTime > 0 
			&& level.time - level.lastFailedVoteTime < VOTE_REJECTED_REPEAT_TIME
			&& Q_strncmp (level.voteString, level.lastFailedVote, sizeof(level.voteString)) == 0);
}

void G_ResetRejectedVote(void) {
	level.lastFailedVoteTime = 0;
	level.lastFailedVote[0] = '\0';
}

void G_SaveRejectedVote(void) {
	if (!g_voteRepeatLimit.integer) {
		return;
	}
	if (!G_CheckRejectedVote()) {
		level.lastFailedVoteCount = 0;
	}
	level.lastFailedVoteTime = level.time;
	Q_strncpyz(level.lastFailedVote, level.voteString, sizeof(level.lastFailedVote));
	level.lastFailedVoteCount++;
}

/*
==================
CheckVote
==================
*/
void CheckVote( void ) {
	if ( level.voteExecuteTime && level.voteExecuteTime < level.realtime ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString ) );
	}
	if ( !level.voteTime ) {
		return;
	}
	if ( level.realtime - level.voteTime >= VOTE_TIME ) {
            if(g_dmflags.integer & DF_LIGHT_VOTING && level.voteLightAllowed) {
                //Let pass if there was at least twice as many for as against
                if ( level.voteYes > level.voteNo*2 ) {
                    trap_SendServerCommand( -1, "print \"Vote passed. At least 2 of 3 voted yes\n\"" );
		    G_SendVoteResult(qtrue);
		    G_SetVoteExecTime();
                } else {
                    //Let pass if there is more yes than no and at least 2 yes votes and at least 30% yes of all on the server
                    if ( level.voteYes > level.voteNo && level.voteYes >= 2 && (level.voteYes*10)>(level.numVotingClients*3) ) {
                        trap_SendServerCommand( -1, "print \"Vote passed. More yes than no.\n\"" );
			G_SendVoteResult(qtrue);
			G_SetVoteExecTime();
                    } else {
                        trap_SendServerCommand( -1, "print \"Vote failed.\n\"" );
		    	G_SendVoteResult(qfalse);
		    }
                }
	    } else if (level.votePassRatio > 0) {
                trap_SendServerCommand( -1, va("print \"Vote failed (requires %i percent of the votes to pass).\n\"", (int)(level.votePassRatio*100)));
		G_SendVoteResult(qfalse);
		G_SendVoteResult(qfalse);
            } else {
                trap_SendServerCommand( -1, "print \"Vote failed.\n\"" );
		G_SendVoteResult(qfalse);
            }
	} else if (level.votePassRatio > 0) {
		if ((float)level.voteYes/level.numVotingClients > level.votePassRatio) {
			trap_SendServerCommand( -1, "print \"Vote passed.\n\"" );
		    	G_SendVoteResult(qtrue);
			G_SetVoteExecTime();
		} else if ((float)level.voteNo/level.numVotingClients >= 1.0-level.votePassRatio) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, va("print \"Vote failed (requires %i percent of the votes to pass).\n\"", (int)(level.votePassRatio*100)));
		    	G_SendVoteResult(qfalse);

			// vote was actively rejected, prevent same client from calling it again immediately
			G_SaveRejectedVote();
		} else {
			// still waiting for a majority
			return;
		}
	} else {
		// ATVI Q3 1.32 Patch #9, WNF
		if ( level.voteYes > (level.numVotingClients)/2 ) {
			// execute the command, then remove the vote
			trap_SendServerCommand( -1, "print \"Vote passed.\n\"" );
		    	G_SendVoteResult(qtrue);
			G_SetVoteExecTime();
		} else if ( level.voteNo >= (level.numVotingClients)/2 ) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, "print \"Vote failed.\n\"" );
		    	G_SendVoteResult(qfalse);

			// vote was actively rejected, prevent same client from calling it again immediately
			G_SaveRejectedVote();
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.voteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );

}

void ForceFail( void ) {
    level.voteTime = 0;
    level.voteExecuteTime = 0;
    level.voteString[0] = 0;
    level.voteDisplayString[0] = 0;
    level.voteKickClient = -1;
    level.voteKickType = 0;
    trap_SetConfigstring( CS_VOTE_TIME, "" );
    trap_SetConfigstring( CS_VOTE_STRING, "" );	
    trap_SetConfigstring( CS_VOTE_YES, "" );
    trap_SetConfigstring( CS_VOTE_NO, "" );
}


/*
==================
CountVotes

 Iterates through all the clients and counts the votes
==================
*/
void CountVotes( void ) {
    int i;
    int yes=0,no=0;

    level.numVotingClients=0;

    for ( i = 0 ; i < level.maxclients ; i++ ) {
            if ( level.clients[ i ].pers.connected != CON_CONNECTED 
			    && (level.clients[ i ].pers.connected != CON_CONNECTING || level.clients[ i ].pers.clientFlags & CLF_FIRSTCONNECT))  
                continue; // client not connected and isn't being carried over from last level

            if (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR)
		continue; //Don't count spectators

            if ( g_entities[i].r.svFlags & SVF_BOT )
                continue; //Is a bot

            //The client can vote
            level.numVotingClients++;

            if ( level.clients[ i ].pers.connected != CON_CONNECTED )
                continue; //Client was not fully connected, and can't vote (yet)

            //Did the client vote yes?
            if(level.clients[i].vote>0)
                yes++;

            //Did the client vote no?
            if(level.clients[i].vote<0)
                no++;
    }

    //See if anything has changed
    if(level.voteYes != yes) {
        level.voteYes = yes;
        trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
    }

    if(level.voteNo != no) {
        level.voteNo = no;
        trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );
    }
}

void ClientLeaving(int clientNumber) {
    if(clientNumber == level.voteKickClient) {
            ForceFail();
    }
}

