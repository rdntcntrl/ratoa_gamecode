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
#define MAX_VOTENAME_LENGTH 14 //currently the longest string is "/map_restart/\0" (14 chars)
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
	char buffer[MAX_MAPNAME_BUFFER];
	int i, nummaps,maplen;
	int maps_in_page = largepage ? MAPS_PER_LARGEPAGE : MAPS_PER_PAGE;

	memset(&result,0,sizeof(result));
        memset(&buffer,0,sizeof(buffer));

	//Check if there is a votemaps.cfg
	trap_FS_FOpenFile(g_votemaps.string,&file,FS_READ);
	if (!file && recommenedonly) {
		// if no votemaps.cfg and we only want recommened maps, try recommenedmaps.cfg
		trap_FS_FOpenFile(g_recommendedMapsFile.string,&file,FS_READ);
	}
	if(file)
	{
		//there is a votemaps.cfg file, take allowed maps from there
		trap_FS_Read(&buffer,sizeof(buffer)-1,file);
		pointer = buffer;
		token = COM_Parse(&pointer);
		if(token[0]==0 && page == 0) {
			//First page empty
			result.pagenumber = -1;
                        trap_FS_FCloseFile(file);
			return result;
		}
		//Skip the first pages
		for(i=0;i<maps_in_page*page;i++) {
			token = COM_Parse(&pointer);
		}
		if(!token || token[0]==0) {
			//Page empty, return to first page
                        trap_FS_FCloseFile(file);
			return getMappage(0, largepage, recommenedonly);
		}
		//There is an actual page:
                result.pagenumber = page;
		for(i=0;i<maps_in_page && token;i++) {
			Q_strncpyz(result.mapname[i],token,MAX_MAPNAME);
			token = COM_Parse(&pointer);
		}
                trap_FS_FCloseFile(file);
		return result;
	}
        //There is no votemaps.cfg file, find filelist of maps
        nummaps = trap_FS_GetFileList("maps",".bsp",buffer,sizeof(buffer));

        if(nummaps && nummaps<=maps_in_page*page)
            return getMappage(0, largepage, recommenedonly);

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
        return result;
}

void getCompleteMaplist(qboolean recommenedonly, int gametypebits_filter, struct maplist_s *out) {
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

	if (gametypebits_filter != 0) {
		G_LoadArenas();
	}

	if(file)
	{
		//there is a recommendedmaps file or a votemaps.cfg file, take allowed maps from there
		trap_FS_Read(&buffer,sizeof(buffer)-1,file);
		pointer = buffer;
		token = COM_Parse(&pointer);
		while (token && token[0] != 0 && out->num < MAX_MAPS) {
			if (gametypebits_filter == 0 || (gametypebits_filter & G_GametypeBitsForMap(token))) {
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
		if (gametypebits_filter == 0 || (gametypebits_filter & G_GametypeBitsForMap(mapname))) {
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

#define MAX_CUSTOM_VOTES    48

char            custom_vote_info[2048];

/*
==================
VoteParseCustomVotes
 *Reads the file votecustom.cfg. Finds all the commands that can be used with
 *"/callvote custom COMMAND" and adds the commands to custom_vote_info
==================
 */
int VoteParseCustomVotes( void ) {
    fileHandle_t	file;
    char            buffer[8*1024];
    int             commands;
    char	*token,*pointer;

    trap_FS_FOpenFile(g_votecustom.string,&file,FS_READ);

    if(!file)
        return 0;

    memset(&buffer,0,sizeof(buffer));
    memset(&custom_vote_info,0,sizeof(custom_vote_info));

    commands = 0;

    trap_FS_Read(&buffer,sizeof(buffer)-1,file);
    
    pointer = buffer;

    while ( commands < MAX_CUSTOM_VOTES ) {
	token = COM_Parse( &pointer );
        if ( !token[0] ) {
            break;
	}

        if ( !strcmp( token, "votecommand" ) ) {
            token = COM_Parse( &pointer );
            Q_strcat(custom_vote_info,sizeof(custom_vote_info),va("%s ",token));
            commands++;
	}
    }

    trap_FS_FCloseFile(file);

    return commands;
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
    char            buffer[8*1024];
    char	*token,*pointer;
    char	key[MAX_TOKEN_CHARS];

    trap_FS_FOpenFile(g_votecustom.string,&file,FS_READ);

    if(!file) {
        memset(&result,0,sizeof(result));
        return result;
    }

    memset(&buffer,0,sizeof(buffer));

    trap_FS_Read(&buffer,sizeof(buffer)-1,file);

    pointer = buffer;

    while ( qtrue ) {
	token = COM_Parse( &pointer );
        if ( !token[0] ) {
            break;
	}

        if ( strcmp( token, "{" ) ) {
		Com_Printf( "Missing { in votecustom.cfg\n" );
		break;
	}

        memset(&result,0,sizeof(result));

	result.lightvote = qtrue;

        while ( 1 ) {
            token = COM_ParseExt( &pointer, qtrue );
            if ( !token[0] ) {
                    Com_Printf( "Unexpected end of customvote.cfg\n" );
                    break;
            }
            if ( !strcmp( token, "}" ) ) {
                    break;
            }
            Q_strncpyz( key, token, sizeof( key ) );

            token = COM_ParseExt( &pointer, qfalse );
            if ( !token[0] ) {
                Com_Printf("Invalid/missing argument to %s in customvote.cfg\n",key);
            }
            if(!Q_stricmp(key,"votecommand")) {
                Q_strncpyz(result.votename,token,sizeof(result.votename));
            } else if(!Q_stricmp(key,"displayname")) {
                Q_strncpyz(result.displayname,token,sizeof(result.displayname));
            } else if(!Q_stricmp(key,"command")) {
                Q_strncpyz(result.command,token,sizeof(result.command));
            } else if(!Q_stricmp(key,"lightvote")) {
		result.lightvote = atoi(token) > 0 ? qtrue : qfalse;
            } else {
                Com_Printf("Unknown key in customvote.cfg: %s\n",key);
            }

	}

        if(!Q_stricmp(result.votename,votecommand)) {
            return result;
        }
    }

    //Nothing was found
    memset(&result,0,sizeof(result));
        return result;
}

void G_SendVoteResult(qboolean passed) {
	if (!g_usesRatVM.integer) {
		return;
	}
	trap_SendServerCommand( -1, va("vresult %s", passed ? "p" : "f"));
}

void G_SetVoteExecTime(void) {
	level.voteExecuteTime = level.time + 3000;
	// make sure vote gets executed if it passed just at the end of a warmup
	if (level.warmupTime > 0 && level.warmupTime <= level.voteExecuteTime) {
		level.voteExecuteTime = level.warmupTime - 3*1000.0/sv_fps.integer;
		if (level.voteExecuteTime <= level.time + 1000/sv_fps.integer) {
			level.voteExecuteTime = level.time;
		}
	}
}

/*
==================
CheckVote
==================
*/
void CheckVote( void ) {
	if ( level.voteExecuteTime && level.voteExecuteTime < level.time ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString ) );
	}
	if ( !level.voteTime ) {
		return;
	}
	if ( level.time - level.voteTime >= VOTE_TIME ) {
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
            } else {
                trap_SendServerCommand( -1, "print \"Vote failed.\n\"" );
		G_SendVoteResult(qfalse);
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
            if ( level.clients[ i ].pers.connected != CON_CONNECTED )
                continue; //Client was not connected

            if (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR)
		continue; //Don't count spectators

            if ( g_entities[i].r.svFlags & SVF_BOT )
                continue; //Is a bot

            //The client can vote
            level.numVotingClients++;

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

