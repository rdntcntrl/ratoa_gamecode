/*
===========================================================================
Copyright (C) 2009 Poul Sander

This file is part of the Open Arena source code.

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

#include "ui_local.h"

#define ART_BACK0			"menu/art_blueish/back_0"
#define ART_BACK1			"menu/art_blueish/back_1"
#define ART_FIGHT0			"menu/art_blueish/accept_0"
#define ART_FIGHT1			"menu/art_blueish/accept_1"
//#define ART_BACKGROUND                  "menu/art_blueish/addbotframe"
#define ART_ARROWS			"menu/art_blueish/arrows_vert_0"
#define ART_ARROWUP			"menu/art_blueish/arrows_vert_top"
#define ART_ARROWDOWN                   "menu/art_blueish/arrows_vert_bot"
#define ART_ARROWSH			"menu/art_blueish/gs_arrows_0"
#define ART_ARROWSHL			"menu/art_blueish/gs_arrows_l"
#define ART_ARROWSHR			"menu/art_blueish/gs_arrows_r"
#define ART_SELECT			"menu/art_blueish/maps_select"
#define ART_SELECTED			"menu/art_blueish/maps_selected"
#define ART_UNKNOWNMAP           	"menu/art/unknownmap"

#define ID_FILTER			8
#define ID_TYPE				9
#define ID_BACK				10
#define ID_GO				11
#define ID_PICTURES			12	// 12-22
#define ID_PREVPAGE			23
#define ID_NEXTPAGE			24

#define SIZE_OF_NAME                    32

#define MAX_MAPROWS		5
#define MAX_MAPCOLS		2
#define MAX_MAPSPERPAGE	(MAX_MAPROWS * MAX_MAPCOLS)

void UI_VoteMapMenu_Update( void );

typedef struct {
	menuframework_s	menu;
	menubitmap_s	arrows;
        menutext_s	banner;
        menutext_s	info;
	menubitmap_s	prevpage;
	menubitmap_s	nextpage;
	menubitmap_s	back;
	menubitmap_s	go;
        menufield_s	        filter;
	menulist_s		type;
	menuradiobutton_s	sort;
	menubitmap_s	mappics[MAX_MAPSPERPAGE];
	menubitmap_s	mapbuttons[MAX_MAPSPERPAGE];

	menutext_s		mapname;
	menutext_s		page;

	//int nummaps;
	int currentmap;
	int pagenum;

	//int		selected;
} votemenu_map_t;

static votemenu_map_t	s_votemenu_map;
static char pagebuffer[64];

#define MAPPAGE_TYPE_ALL 0
#define MAPPAGE_TYPE_RECOMMENDED 1
#define MAPPAGE_TYPE_NUM 2
static const char *mappage_type_items[] = {
	"All",
	"Recommended",
	NULL
};

static const char *getmappage_all_cmd = "getmappage";
static const char *getmappage_recommened_cmd = "getrecmappage";
static const char *getmappage_cmd = "getmappage";

static int ignore_next_cmd = 0;
static int mappage_in_flight = 0;

t_mappage mappage;

// XXX: must be multiple of MAX_MAPSPERPAGE
#define MAX_MAP_NUMBER 1000
struct maplist_s {
	int reset;
	int num_sent_cmds;
	int num_cmds;
	//int pagenum;
	int num_maps;
	int loaded_all;
	char mapname[MAX_MAP_NUMBER][MAX_MAPNAME_LENGTH];
};

struct maplist_s filtered_list;
struct maplist_s maplists[MAPPAGE_TYPE_NUM];

struct maplist_s *current_list;

static void InitMappage(void) {
    int i;
    memset(&mappage, 0, sizeof(mappage));
    //We need to initialize the list or it will be impossible to click on the items
    for(i=0;i<MAX_MAPSPERPAGE;i++) {
        Q_strncpyz(mappage.mapname[i],"----",5);
    }
    mappage.pagenumber = 0;
}

static void InitMaplist(void) {
	memset(&filtered_list, 0, sizeof(filtered_list));
	//memset(current_list, 0, sizeof(struct maplist_s));
}


/*
=================
VoteMapMenu_Event
=================
*/
static void VoteMapMenu_Event( void* ptr, int event )
{
	int mapidx;
	switch (((menucommon_s*)ptr)->id)
	{
            case ID_BACK:
		if (event != QM_ACTIVATED)
                    return;
                UI_PopMenu();
                break;
            case ID_GO:
                if( event != QM_ACTIVATED ) {
                    return;
                }
		if (s_votemenu_map.currentmap < 0 || s_votemenu_map.currentmap >= MAX_MAPSPERPAGE)
			return;
                //if(!Q_stricmp(mappage.mapname[s_votemenu_map.currentmap],"---"))
		mapidx = s_votemenu_map.pagenum * MAX_MAPSPERPAGE + s_votemenu_map.currentmap;
		if (mapidx > filtered_list.num_maps) {
			return;
		}
                if(!Q_stricmp(filtered_list.mapname[mapidx],"---"))
                    return; //Blank spaces have string "---"
                trap_Cmd_ExecuteText( EXEC_APPEND, va("cmd callvote map %s;", filtered_list.mapname[mapidx]) );
                UI_PopMenu();
		if (UI_PushedMenus()) {
			// this menu may be opened directly, so don't pop parent if it's not there
			UI_PopMenu();
		}
                break;
        //    case ID_GO:
        //        if( event != QM_ACTIVATED ) {
        //            return;
        //        }
        //        if(!s_votemenu_map.selected || mappage.mapname[s_votemenu_map.selected-ID_MAPNAME0][0] == 0)
        //            return;
        //        if(!Q_stricmp(mappage.mapname[s_votemenu_map.selected-ID_MAPNAME0],"---"))
        //            return; //Blank spaces have string "---"
        //        trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote map %s", mappage.mapname[s_votemenu_map.selected-ID_MAPNAME0]) );
        //        UI_PopMenu();
        //        UI_PopMenu();
        //        break;
            //default:
            //    if( event != QM_ACTIVATED ) {
            //        return;
            //    }
            //    if(s_votemenu_map.selected != ((menucommon_s*)ptr)->id) {
            //        s_votemenu_map.selected = ((menucommon_s*)ptr)->id;
            //        UI_VoteMapMenuInternal();
            //    }
            //    break;
         }
}


static qboolean VoteMaps_Filtered(char *map) {
        if (!s_votemenu_map.filter.field.buffer[0]) {
		return qtrue;
	}

	return Q_stristr(map, s_votemenu_map.filter.field.buffer) == NULL ? qfalse : qtrue;

}

static int QDECL Mapnames_Compare( const void *arg1, const void *arg2 ) {
	const char *n1 = arg1;
	const char *n2 = arg2;
	return Q_stricmp(n1, n2);
}

static void UpdateFilter(void) {
	int i;
	memset( &filtered_list, 0 ,sizeof(filtered_list) );
	for (i=0; i < current_list->num_maps; ++i) {
		if (VoteMaps_Filtered(current_list->mapname[i])) {
			Q_strncpyz(filtered_list.mapname[filtered_list.num_maps], current_list->mapname[i], MAX_MAPNAME_LENGTH);
			filtered_list.num_maps++;
		}
	}
	if (s_votemenu_map.sort.curvalue) {
		qsort( filtered_list.mapname, filtered_list.num_maps, MAX_MAPNAME_LENGTH, Mapnames_Compare);
	}
}

static void ResetMaplist(void) {
	InitMaplist();
	InitMappage();
	s_votemenu_map.pagenum = 0;
	s_votemenu_map.currentmap = 0;
	UpdateFilter();
	UI_VoteMapMenu_Update();
	//trap_Cmd_ExecuteText( EXEC_APPEND,va("%s 0", getmappage_cmd) );
	//maplist.num_sent_cmds++;
}


void Maplist_RequestNextPage( struct maplist_s *list ) {
	int mappage;
	if (list->loaded_all) {
		return;
	}
	mappage = (list->num_maps / MAPPAGE_NUM) + ((list->num_maps % MAPPAGE_NUM == 0) ? 0 : 1);
	trap_Cmd_ExecuteText( EXEC_APPEND,va("%s %i",getmappage_cmd, mappage));
	mappage_in_flight = 1;
	current_list->num_sent_cmds++;
}


static void VoteMapMenu_FilterEvent( void* ptr, int event ) {
	if (event != QM_ACTIVATED ) {
		return;
	}
	trap_Cvar_Set( "ui_mapvote_filter", va("%s", s_votemenu_map.filter.field.buffer));
	trap_Cvar_Set( "ui_mapvote_sort", va("%i", s_votemenu_map.sort.curvalue));
	ResetMaplist();
	//s_votemenu_map.pagenum = 0;
	//s_votemenu_map.currentmap = 0;
	//UI_VoteMapMenu_Update();
}

static void UI_VoteMapMenu_PreviousPageEvent( void* ptr, int event ) {
	if (event != QM_ACTIVATED ) {
		return;
	}

        //trap_Cmd_ExecuteText( EXEC_APPEND,va("%s %d",getmappage_cmd, mappage.pagenumber-1) );
	if (s_votemenu_map.pagenum > 0) {
		s_votemenu_map.pagenum--;
		UI_VoteMapMenu_Update();
	}
}


static void UI_VoteMapMenu_NextPageEvent( void* ptr, int event ) {
	if (event != QM_ACTIVATED) {
		return;
	}

	if (filtered_list.num_maps > (s_votemenu_map.pagenum + 1) * MAX_MAPSPERPAGE) {
		s_votemenu_map.pagenum++;
		UI_VoteMapMenu_Update();
	}

        //trap_Cmd_ExecuteText( EXEC_APPEND,va("%s %d",getmappage_cmd, mappage.pagenumber+1) );
}


static void VoteMapMenu_TypeEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED) {
		return;
	}
	if (s_votemenu_map.type.curvalue == MAPPAGE_TYPE_RECOMMENDED) {
		getmappage_cmd = getmappage_recommened_cmd;
		current_list = &maplists[MAPPAGE_TYPE_RECOMMENDED];
	} else {
		getmappage_cmd = getmappage_all_cmd;
		current_list = &maplists[MAPPAGE_TYPE_ALL];
	}
	ResetMaplist();
	if (!mappage_in_flight) {
		Maplist_RequestNextPage(current_list);
	} else {
		ignore_next_cmd = 1;
	}
}

/*
=================
UI_VoteMapMenu_Draw
=================
*/
static void UI_VoteMapMenu_Draw( void ) {
	vec4_t	bg = {0.0, 0.0, 0.0, 0.85};
	//UI_DrawBannerString( 320, 16, "CALL VOTE MAP", UI_CENTER, color_white );
	//UI_DrawNamedPic( 320-233, 240-166, 466, 332, ART_BACKGROUND );
	UI_FillRect( 0, 0, 640, 480, bg );

	// standard menu drawing
	Menu_Draw( &s_votemenu_map.menu );

	UI_DrawString( 630, 70, va("%s (%i maps)", current_list->loaded_all ? "Done" : "Loading", current_list->num_maps ), UI_RIGHT|UI_SMALLFONT, color_white );
}

/*
=================
VoteMapMenu_Cache
=================
*/
static void VoteMapMenu_Cache( void )
{
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FIGHT0 );
	trap_R_RegisterShaderNoMip( ART_FIGHT1 );
	trap_R_RegisterShaderNoMip( ART_ARROWS );
	trap_R_RegisterShaderNoMip( ART_ARROWUP );
	trap_R_RegisterShaderNoMip( ART_ARROWDOWN );
	trap_R_RegisterShaderNoMip( ART_UNKNOWNMAP );
	trap_R_RegisterShaderNoMip( ART_ARROWSH );
	trap_R_RegisterShaderNoMip( ART_ARROWSHL );
	trap_R_RegisterShaderNoMip( ART_ARROWSHR );
	trap_R_RegisterShaderNoMip( ART_SELECT );
	trap_R_RegisterShaderNoMip( ART_SELECTED );

}

//static void setMapMenutext(menutext_s *menu,int y,int id,char * text) {
//    menu->generic.type            = MTYPE_PTEXT;
//    menu->color               = color_red;
//    menu->generic.flags       = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
//    if(s_votemenu_map.selected == id)
//         menu->color       = color_orange;
//    menu->generic.x           = 320-80;
//    menu->generic.y           = y;
//    menu->generic.id          = id;
//    menu->generic.callback    = VoteMapMenu_Event;
//    menu->string              = text;
//    menu->style               = UI_LEFT|UI_SMALLFONT;
//}

/*
===============
VoteMapMenu_LevelshotDraw
===============
*/
static void VoteMapMenu_LevelshotDraw( void *self ) {
	menubitmap_s	*b;
	int				x;
	int				y;
	int				w;
	int				h;
	int				n;
        //const char		*info;

	b = (menubitmap_s *)self;

	if( !b->generic.name ) {
		return;
	}

	if( b->generic.name && !b->shader ) {
		b->shader = trap_R_RegisterShaderNoMip( b->generic.name );
		if( !b->shader && b->errorpic ) {
			b->shader = trap_R_RegisterShaderNoMip( b->errorpic );
		}
	}

	if( b->focuspic && !b->focusshader ) {
		b->focusshader = trap_R_RegisterShaderNoMip( b->focuspic );
	}

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height;
	if( b->shader ) {
		UI_DrawHandlePic( x, y, w, h, b->shader );
	}

	x = b->generic.x;
	y = b->generic.y + b->height;
	UI_FillRect( x, y, b->width, 28, colorBlack );

	x += b->width / 2;
	y += 4;
	n = s_votemenu_map.pagenum * MAX_MAPSPERPAGE + b->generic.id - ID_PICTURES;

	if (n > MAX_MAP_NUMBER) {
		n = MAX_MAP_NUMBER;
	}
        
	UI_DrawString( x, y, filtered_list.mapname[n], UI_CENTER|UI_SMALLFONT, color_orange );

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height + 28;
	if( b->generic.flags & QMF_HIGHLIGHT ) {	
		UI_DrawHandlePic( x, y, w, h, b->focusshader );
	}
}

static int CountMappageMaps( void ) {
	int i;
	for (i = 0; i < MAPPAGE_NUM; ++i) {
		if (!Q_stricmp(mappage.mapname[i],"---")) {
			break;
		}
	}
	return i;
}

void UI_VoteMapMenu_Update( void ) {
	int				i;
	int 		top;
	static	char	picname[MAX_MAPSPERPAGE][64];
	char			mapname[MAX_MAPNAME_LENGTH];

	top = s_votemenu_map.pagenum * MAX_MAPSPERPAGE;
	//s_votemenu_map.nummaps = maplist.num_maps - s_votemenu_map.pagenumber * MAX_MAPSPERPAGE;

	for (i=0; i<MAX_MAPSPERPAGE; i++)
	{
		if (top + i >= filtered_list.num_maps)
			break;

		Q_strncpyz( mapname, filtered_list.mapname[top+i], MAX_MAPNAME_LENGTH );
		Q_strupr( mapname );

		Com_sprintf( picname[i], sizeof(picname[i]), "levelshots/%s", mapname );
                
		s_votemenu_map.mappics[i].generic.flags &= ~((unsigned int)QMF_HIGHLIGHT);
		s_votemenu_map.mappics[i].generic.name   = picname[i];
		s_votemenu_map.mappics[i].shader         = 0;

		// reset
		s_votemenu_map.mapbuttons[i].generic.flags |= QMF_PULSEIFFOCUS;
		s_votemenu_map.mapbuttons[i].generic.flags &= ~((unsigned int)QMF_INACTIVE);
	}

	for (; i<MAX_MAPSPERPAGE; i++)
	{
		s_votemenu_map.mappics[i].generic.flags &= ~((unsigned int)QMF_HIGHLIGHT);
		s_votemenu_map.mappics[i].generic.name   = NULL;
		s_votemenu_map.mappics[i].shader         = 0;

		// disable
		s_votemenu_map.mapbuttons[i].generic.flags &= ~((unsigned int)QMF_PULSEIFFOCUS);
		s_votemenu_map.mapbuttons[i].generic.flags |= QMF_INACTIVE;
	}


	// no maps to vote for
	if( !filtered_list.num_maps ) {
		s_votemenu_map.go.generic.flags |= QMF_INACTIVE;

		// set the map name
		strcpy( s_votemenu_map.mapname.string, "NO MAPS FOUND" );
	}
	else {
		// set the highlight
		s_votemenu_map.go.generic.flags &= ~((unsigned int)QMF_INACTIVE);
		i = s_votemenu_map.currentmap;
		if ( i >=0 && i < MAX_MAPSPERPAGE ) 
		{
			s_votemenu_map.mappics[i].generic.flags    |= QMF_HIGHLIGHT;
			s_votemenu_map.mapbuttons[i].generic.flags &= ~((unsigned int)QMF_PULSEIFFOCUS);
		}

		// set the map name
		Q_strncpyz( s_votemenu_map.mapname.string, filtered_list.mapname[top+s_votemenu_map.currentmap], MAX_MAPNAME_LENGTH);
	}
	
	i = filtered_list.num_maps / MAX_MAPSPERPAGE + ((filtered_list.num_maps % MAX_MAPSPERPAGE == 0) ? 0 : 1);
	if (i <= 0) {
		i = 1;
	}
	Q_strupr( s_votemenu_map.mapname.string );
	Com_sprintf( pagebuffer, sizeof(pagebuffer), "Page %i/%i", 
			s_votemenu_map.pagenum+1,
		  	i);
}

/*
=================
VoteMapMenu_MapEvent
=================
*/
static void VoteMapMenu_MapEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED) {
		return;
	}

	s_votemenu_map.currentmap = (((menucommon_s*)ptr)->id - ID_PICTURES);
	UI_VoteMapMenu_Update();
}

static qboolean UI_VoteMapMenuParseMappage(void) {
	int i;
	char *mapp, *p;
	char mapList[1024] = "";
	int cvarNum;
	int cvarMaps;
	
	memset(&mappage, 0, sizeof(mappage));
	mappage.pagenumber = UI_Cvar_VariableInteger("ui_mappage_pagenum");

	cvarNum = 0;
	cvarMaps = 0;
	trap_Cvar_VariableStringBuffer(va("ui_mappage_page%i", cvarNum), mapList, sizeof(mapList));
	mapp = mapList;
	for (i = 0 ; i < MAPPAGE_NUM && mapp; ++i) {
		if (cvarMaps == 7) {
			trap_Cvar_VariableStringBuffer(va("ui_mappage_page%i", ++cvarNum), mapList, sizeof(mapList));
			cvarMaps = 0;
			mapp = mapList;
		}
		if ((p = strchr(mapp, ' ')) != NULL) {
			*p = '\0';
			p++;
		} 
		if (strlen(mapp) <= 0) {
			return qfalse;
		}
		Q_strncpyz(mappage.mapname[i], mapp, MAX_MAPNAME_LENGTH);
		mapp = p;
		++cvarMaps;

	}
	if (i != MAPPAGE_NUM) {
		return qfalse;
	}

	return qtrue;
}


/*
=================
UI_VoteMapMenuInternal
=================
*/
void UI_VoteMapMenuInternal( void )
{
	int i;

	if (!UI_VoteMapMenuParseMappage()) {
		Com_Printf("failed to parse mappage\n");
		return;
	}

	mappage_in_flight = 0;
	if (ignore_next_cmd) {
		ignore_next_cmd = 0;
		if (current_list && !current_list->loaded_all) {
			Maplist_RequestNextPage(current_list);
		}
		return;
	}

	if (!current_list) {
		return;
	}

	if (current_list->num_cmds > 0) {
		if (mappage.pagenumber == 0) {
			// returned to page 0, we're finished loading
			current_list->loaded_all = 1;
			//Com_Printf("Loaded all maps!\n");
		}
	}
	if (uis.activemenu != &s_votemenu_map.menu) {
		// menu not showing anymore, ignore
		ResetMaplist();
		return;
	}
	current_list->num_cmds++;
	if (!current_list->loaded_all) {
		for (i = 0; i < CountMappageMaps(); ++i) {
			Q_strncpyz(current_list->mapname[current_list->num_maps], mappage.mapname[i], MAX_MAPNAME_LENGTH);
			if (current_list->num_maps < MAX_MAP_NUMBER) {
				current_list->num_maps++;
			}
			//Com_Printf("adding map no. %i, %s\n", current_list->num_maps, mappage.mapname[i]);
		}
		//trap_Cmd_ExecuteText( EXEC_APPEND,va("%s %d",getmappage_cmd, mappage.pagenumber+1) );
		//current_list->num_sent_cmds++;
		Maplist_RequestNextPage(current_list);
	}

	
	UpdateFilter();
	UI_VoteMapMenu_Update();
}


/*
=================
UI_VoteMapMenu
 *Called from outside
=================
*/
void UI_VoteMapMenu( void ) {
    int x,y,i;
    static char mapnamebuffer[MAX_MAPNAME_LENGTH*2];
    //memset( &s_votemenu_map, 0 ,sizeof(votemenu_map_t) );
    
    VoteMapMenu_Cache();

    memset( &s_votemenu_map, 0 ,sizeof(votemenu_map_t) );

    s_votemenu_map.menu.wrapAround = qtrue;
    s_votemenu_map.menu.fullscreen = qfalse;
    s_votemenu_map.menu.draw = UI_VoteMapMenu_Draw;

    s_votemenu_map.banner.generic.type  = MTYPE_BTEXT;
    s_votemenu_map.banner.generic.x	  = 320;
    s_votemenu_map.banner.generic.y	  = 16;
    s_votemenu_map.banner.string		  = "CALL VOTE MAP";
    s_votemenu_map.banner.color	      = color_white;
    s_votemenu_map.banner.style	      = UI_CENTER;

    s_votemenu_map.go.generic.type			= MTYPE_BITMAP;
    s_votemenu_map.go.generic.name			= ART_FIGHT0;
    s_votemenu_map.go.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_votemenu_map.go.generic.id			= ID_GO;
    s_votemenu_map.go.generic.callback		= VoteMapMenu_Event;
    s_votemenu_map.go.generic.x			= 640;
    s_votemenu_map.go.generic.y			= 480-64;
    s_votemenu_map.go.width  				= 128;
    s_votemenu_map.go.height  				= 64;
    s_votemenu_map.go.focuspic				= ART_FIGHT1;

    s_votemenu_map.back.generic.type		= MTYPE_BITMAP;
    s_votemenu_map.back.generic.name		= ART_BACK0;
    s_votemenu_map.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_votemenu_map.back.generic.id			= ID_BACK;
    s_votemenu_map.back.generic.callback	= VoteMapMenu_Event;
    s_votemenu_map.back.generic.x			= 0;
    s_votemenu_map.back.generic.y			= 480-64;
    s_votemenu_map.back.width				= 128;
    s_votemenu_map.back.height				= 64;
    s_votemenu_map.back.focuspic			= ART_BACK1;

    s_votemenu_map.filter.generic.type		= MTYPE_FIELD;
    s_votemenu_map.filter.generic.name		= "Filter:";
    s_votemenu_map.filter.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
    s_votemenu_map.filter.generic.callback	= VoteMapMenu_FilterEvent;
    s_votemenu_map.filter.generic.id			= ID_FILTER;
    s_votemenu_map.filter.generic.x			= 75;
    s_votemenu_map.filter.generic.y			= 70;
    s_votemenu_map.filter.field.widthInChars	= 18;
    s_votemenu_map.filter.field.maxchars	= MAX_MAPNAME_LENGTH;


    s_votemenu_map.type.generic.type		= MTYPE_SPINCONTROL;
    s_votemenu_map.type.generic.name		= "Type:";
    s_votemenu_map.type.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
    s_votemenu_map.type.generic.callback	= VoteMapMenu_TypeEvent;
    s_votemenu_map.type.generic.id		= ID_TYPE;
    s_votemenu_map.type.generic.x		= 280;
    s_votemenu_map.type.generic.y		= 70;
    s_votemenu_map.type.itemnames		= mappage_type_items;

    s_votemenu_map.sort.generic.type        = MTYPE_RADIOBUTTON;
    s_votemenu_map.sort.generic.name	      = "Sort:";
    s_votemenu_map.sort.generic.callback	= VoteMapMenu_FilterEvent;
    s_votemenu_map.sort.generic.id			= ID_FILTER;
    s_votemenu_map.sort.generic.flags	      = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
    s_votemenu_map.sort.generic.x	          = 440;
    s_votemenu_map.sort.generic.y	          = 70;

    for (i=0; i<MAX_MAPSPERPAGE; i++)
    {
    	//x =	(i % MAX_MAPCOLS) * (128+8) + 188;
    	//y = (i / MAX_MAPROWS) * (128+8) + 96;
    	//x = (640-MAX_MAPROWS*140)/2 + ( (i % MAX_MAPROWS) * 140 );
    	x = (640-MAX_MAPROWS*128)/2 + ( (i % MAX_MAPROWS) * 128 );
    	y = 96 + ( (i / MAX_MAPROWS) * 140 );
    
    	s_votemenu_map.mappics[i].generic.type   = MTYPE_BITMAP;
    	s_votemenu_map.mappics[i].generic.flags  = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
    	s_votemenu_map.mappics[i].generic.x	    = x;
    	s_votemenu_map.mappics[i].generic.y	    = y;
    	s_votemenu_map.mappics[i].generic.id		= ID_PICTURES+i;
    	s_votemenu_map.mappics[i].width  		= 128;
    	s_votemenu_map.mappics[i].height  	    = 96;
    	s_votemenu_map.mappics[i].focuspic       = ART_SELECTED;
    	s_votemenu_map.mappics[i].errorpic       = ART_UNKNOWNMAP;
    	s_votemenu_map.mappics[i].generic.ownerdraw = VoteMapMenu_LevelshotDraw;
    
    	s_votemenu_map.mapbuttons[i].generic.type     = MTYPE_BITMAP;
    	s_votemenu_map.mapbuttons[i].generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_NODEFAULTINIT;
    	s_votemenu_map.mapbuttons[i].generic.id       = ID_PICTURES+i;
    	s_votemenu_map.mapbuttons[i].generic.callback = VoteMapMenu_MapEvent;
    	s_votemenu_map.mapbuttons[i].generic.x	     = x - 30;
    	s_votemenu_map.mapbuttons[i].generic.y	     = y - 32;
    	s_votemenu_map.mapbuttons[i].width  		     = 256;
    	s_votemenu_map.mapbuttons[i].height  	     = 248;
    	s_votemenu_map.mapbuttons[i].generic.left     = x;
    	s_votemenu_map.mapbuttons[i].generic.top  	 = y;
    	s_votemenu_map.mapbuttons[i].generic.right    = x + 128;
    	s_votemenu_map.mapbuttons[i].generic.bottom   = y + 128;
    	s_votemenu_map.mapbuttons[i].focuspic         = ART_SELECT;
    }
    
    s_votemenu_map.arrows.generic.type  = MTYPE_BITMAP;
    s_votemenu_map.arrows.generic.name  = ART_ARROWSH;
    s_votemenu_map.arrows.generic.flags = QMF_INACTIVE;
    s_votemenu_map.arrows.generic.x	   = 260;
    s_votemenu_map.arrows.generic.y	   = 400;
    s_votemenu_map.arrows.width  	   = 128;
    s_votemenu_map.arrows.height  	   = 32;
    
    s_votemenu_map.prevpage.generic.type	    = MTYPE_BITMAP;
    s_votemenu_map.prevpage.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_votemenu_map.prevpage.generic.callback = UI_VoteMapMenu_PreviousPageEvent;
    s_votemenu_map.prevpage.generic.id	    = ID_PREVPAGE;
    s_votemenu_map.prevpage.generic.x		= 260;
    s_votemenu_map.prevpage.generic.y		= 400;
    s_votemenu_map.prevpage.width  		    = 64;
    s_votemenu_map.prevpage.height  		    = 32;
    s_votemenu_map.prevpage.focuspic         = ART_ARROWSHL;
    
    s_votemenu_map.nextpage.generic.type	    = MTYPE_BITMAP;
    s_votemenu_map.nextpage.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_votemenu_map.nextpage.generic.callback = UI_VoteMapMenu_NextPageEvent;
    s_votemenu_map.nextpage.generic.id	    = ID_NEXTPAGE;
    s_votemenu_map.nextpage.generic.x		= 321;
    s_votemenu_map.nextpage.generic.y		= 400;
    s_votemenu_map.nextpage.width  		    = 64;
    s_votemenu_map.nextpage.height  		    = 32;
    s_votemenu_map.nextpage.focuspic         = ART_ARROWSHR;

    s_votemenu_map.page.generic.type		= MTYPE_TEXT;
    s_votemenu_map.page.generic.flags	= QMF_CENTER_JUSTIFY|QMF_SMALLFONT;
    s_votemenu_map.page.generic.x		= 320;
    s_votemenu_map.page.generic.y		= 368;
    s_votemenu_map.page.string			= pagebuffer;
    s_votemenu_map.page.color         = color_white;
    s_votemenu_map.page.style         = UI_CENTER;

    
    s_votemenu_map.mapname.generic.type  = MTYPE_PTEXT;
    s_votemenu_map.mapname.generic.flags = QMF_CENTER_JUSTIFY|QMF_INACTIVE;
    s_votemenu_map.mapname.generic.x	    = 320;
    s_votemenu_map.mapname.generic.y	    = 440;
    s_votemenu_map.mapname.string        = mapnamebuffer;
    s_votemenu_map.mapname.style         = UI_CENTER|UI_BIGFONT;
    s_votemenu_map.mapname.color         = text_color_normal;
    
    
    Menu_AddItem( &s_votemenu_map.menu, &s_votemenu_map.type );
    
    for (i=0; i<MAX_MAPSPERPAGE; i++)
    {
    	Menu_AddItem( &s_votemenu_map.menu, &s_votemenu_map.mappics[i] );
    	Menu_AddItem( &s_votemenu_map.menu, &s_votemenu_map.mapbuttons[i] );
    }

    Menu_AddItem( &s_votemenu_map.menu, (void*) &s_votemenu_map.banner );
    Menu_AddItem( &s_votemenu_map.menu, (void*) &s_votemenu_map.back );
    Menu_AddItem( &s_votemenu_map.menu, (void*) &s_votemenu_map.go );
    Menu_AddItem( &s_votemenu_map.menu, (void*) &s_votemenu_map.arrows );
    Menu_AddItem( &s_votemenu_map.menu, (void*) &s_votemenu_map.prevpage );
    Menu_AddItem( &s_votemenu_map.menu, (void*) &s_votemenu_map.nextpage );
    Menu_AddItem( &s_votemenu_map.menu, (void*) &s_votemenu_map.mapname );
    Menu_AddItem( &s_votemenu_map.menu, (void*) &s_votemenu_map.page );
    Menu_AddItem( &s_votemenu_map.menu, (void*) &s_votemenu_map.filter );
    Menu_AddItem( &s_votemenu_map.menu, (void*) &s_votemenu_map.sort );

    s_votemenu_map.sort.curvalue = trap_Cvar_VariableValue( "ui_mapvote_sort") ? 1 : 0;

    trap_Cvar_VariableStringBuffer( "ui_mapvote_filter", 
		    s_votemenu_map.filter.field.buffer,
		    sizeof(s_votemenu_map.filter.field.buffer));

    getmappage_cmd = getmappage_all_cmd;

    current_list = &maplists[MAPPAGE_TYPE_ALL];
    ResetMaplist();
    if (!current_list->loaded_all) {
	    Maplist_RequestNextPage(current_list);
	    //trap_Cmd_ExecuteText( EXEC_APPEND,va("%s 0", getmappage_cmd) );
	    //current_list->num_sent_cmds++;
    }

    trap_Cvar_Set( "cl_paused", "0" ); //We cannot send server commands while paused!


    UI_PushMenu( &s_votemenu_map.menu );
}
