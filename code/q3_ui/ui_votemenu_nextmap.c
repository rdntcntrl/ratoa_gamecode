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

#define ART_SELECT			"menu/art_blueish/maps_select"
#define ART_SELECTED			"menu/art_blueish/maps_selected"
#define ART_UNKNOWNMAP           	"menu/art/unknownmap"

#define ID_PICTURES			12	// 12-22

#define SIZE_OF_NAME                    32

#define NMV_MAX_MAPCOLS		3
#define NMV_MAX_MAPROWS		2
#define NMV_MAX_MAPSPERPAGE	(NMV_MAX_MAPCOLS * NMV_MAX_MAPROWS)

typedef struct {
	menuframework_s	menu;
        menutext_s	banner;
	menubitmap_s	mappics[NMV_MAX_MAPSPERPAGE];
	menubitmap_s	mapbuttons[NMV_MAX_MAPSPERPAGE];

	qboolean voted;

} votemenu_nextmap_t;

static votemenu_nextmap_t	s_votemenu_nextmap;

struct nextmapvote_maplist_s nextmapvote_maplist;

sfxHandle_t talkSound;

//static void VoteNextMapMenu_Event( void* ptr, int event )
//{
//}


void UI_VoteNextMapMenu_Close( void ) {
	if (uis.activemenu == &s_votemenu_nextmap.menu) {
		UI_PopMenu();
	}
}


static void UI_VoteNextMapMenu_Draw( void ) {
	vec4_t	bg = {0.0, 0.0, 0.0, 0.85};
	UI_FillRect( 0, 90, 640, 480, bg );
	UI_DrawString( 640/2, 150, va("Time remaining: %is", ui_nextmapvote_remaining.integer), UI_CENTER|UI_SMALLFONT, color_white );

	// standard menu drawing
	Menu_Draw( &s_votemenu_nextmap.menu );
}

static void VoteNextMapMenu_Cache( void )
{
	trap_R_RegisterShaderNoMip( ART_SELECT );
	trap_R_RegisterShaderNoMip( ART_SELECTED );

}

static void VoteNextMapMenu_LevelshotDraw( void *self ) {
	menubitmap_s	*b;
	int				x;
	int				y;
	int				w;
	int				h;
	int				n;

	b = (menubitmap_s *)self;

	n = b->generic.id - ID_PICTURES;
	if (n < 0 || n >= NEXTMAPVOTE_MAP_NUM) {
		n = 0;
	}

	if( !b->shader ) {
		b->shader = trap_R_RegisterShaderNoMip( va("levelshots/%s", nextmapvote_maplist.mapname[n]));
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
        
	UI_DrawString( x, y, nextmapvote_maplist.mapname[n], UI_CENTER|UI_SMALLFONT, color_orange );

	if (nextmapvote_maplist.votes[n]) {
		UI_DrawProportionalString( x, b->generic.y + b->height/2 - PROP_HEIGHT/2, va( "%i", nextmapvote_maplist.votes[n] ),
				UI_CENTER | UI_DROPSHADOW, color_yellow );

	}

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height + 28;
	if( b->generic.flags & QMF_HIGHLIGHT ) {	
		UI_DrawHandlePic( x, y, w, h, b->focusshader );
	}

}

static void VoteNextMapMenu_MapEvent( void* ptr, int event ) {
	int mapNo = 0;
	int i;
	if (s_votemenu_nextmap.voted) {
		return;
	}
	if( event != QM_ACTIVATED) {
		return;
	}
	if (((menucommon_s*)ptr)->id >= ID_PICTURES
			&& ((menucommon_s*)ptr)->id < ID_PICTURES+NMV_MAX_MAPSPERPAGE) {
		mapNo = ((menucommon_s*)ptr)->id - ID_PICTURES;
		if (mapNo < 0 || mapNo > NEXTMAPVOTE_MAP_NUM) {
			return;
		}
		trap_Cmd_ExecuteText( EXEC_APPEND,va("nextmapvote %i", mapNo));
		s_votemenu_nextmap.voted = qtrue;
		s_votemenu_nextmap.mappics[mapNo].generic.flags    |= QMF_HIGHLIGHT;
		for (i=0; i<NMV_MAX_MAPSPERPAGE; i++) {
			s_votemenu_nextmap.mapbuttons[i].generic.flags    &= ~(QMF_HIGHLIGHT|QMF_PULSEIFFOCUS);
		}
	}

}



void UI_VoteNextMapMenu( void ) {
    int x,y,i;
    
    VoteNextMapMenu_Cache();

    memset( &s_votemenu_nextmap, 0 ,sizeof(votemenu_nextmap_t) );

    s_votemenu_nextmap.menu.wrapAround = qtrue;
    s_votemenu_nextmap.menu.fullscreen = qfalse;
    s_votemenu_nextmap.menu.draw = UI_VoteNextMapMenu_Draw;

    s_votemenu_nextmap.banner.generic.type  = MTYPE_BTEXT;
    s_votemenu_nextmap.banner.generic.x	  = 320;
    s_votemenu_nextmap.banner.generic.y	  = 100;
    s_votemenu_nextmap.banner.string		  = "VOTE FOR NEXT MAP";
    s_votemenu_nextmap.banner.color	      = color_white;
    s_votemenu_nextmap.banner.style	      = UI_CENTER;


    for (i=0; i<NMV_MAX_MAPSPERPAGE; i++)
    {
    	x = (640-NMV_MAX_MAPCOLS*200)/2 + ( (i % NMV_MAX_MAPCOLS) * 200 ) + (200-128)/2;
    	y = 190 + ( (i / NMV_MAX_MAPCOLS) * 140 );
    
    	s_votemenu_nextmap.mappics[i].generic.type   = MTYPE_BITMAP;
    	s_votemenu_nextmap.mappics[i].generic.flags  = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
    	s_votemenu_nextmap.mappics[i].generic.x	    = x;
    	s_votemenu_nextmap.mappics[i].generic.y	    = y;
    	s_votemenu_nextmap.mappics[i].generic.id		= ID_PICTURES+i;
    	s_votemenu_nextmap.mappics[i].width  		= 128;
    	s_votemenu_nextmap.mappics[i].height  	    = 96;
    	s_votemenu_nextmap.mappics[i].focuspic       = ART_SELECTED;
    	s_votemenu_nextmap.mappics[i].errorpic       = ART_UNKNOWNMAP;
    	s_votemenu_nextmap.mappics[i].generic.ownerdraw = VoteNextMapMenu_LevelshotDraw;
    
    	s_votemenu_nextmap.mapbuttons[i].generic.type     = MTYPE_BITMAP;
    	s_votemenu_nextmap.mapbuttons[i].generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_NODEFAULTINIT;
    	s_votemenu_nextmap.mapbuttons[i].generic.id       = ID_PICTURES+i;
    	s_votemenu_nextmap.mapbuttons[i].generic.callback = VoteNextMapMenu_MapEvent;
    	s_votemenu_nextmap.mapbuttons[i].generic.x	     = x - 30;
    	s_votemenu_nextmap.mapbuttons[i].generic.y	     = y - 32;
    	s_votemenu_nextmap.mapbuttons[i].width  		     = 256;
    	s_votemenu_nextmap.mapbuttons[i].height  	     = 248;
    	s_votemenu_nextmap.mapbuttons[i].generic.left     = x;
    	s_votemenu_nextmap.mapbuttons[i].generic.top  	 = y;
    	s_votemenu_nextmap.mapbuttons[i].generic.right    = x + 128;
    	s_votemenu_nextmap.mapbuttons[i].generic.bottom   = y + 128;
    	s_votemenu_nextmap.mapbuttons[i].focuspic         = ART_SELECT;
    }
    
    for (i=0; i<NMV_MAX_MAPSPERPAGE; i++)
    {
    	Menu_AddItem( &s_votemenu_nextmap.menu, &s_votemenu_nextmap.mappics[i] );
    	Menu_AddItem( &s_votemenu_nextmap.menu, &s_votemenu_nextmap.mapbuttons[i] );
    }

    Menu_AddItem( &s_votemenu_nextmap.menu, (void*) &s_votemenu_nextmap.banner );

    trap_Cvar_Set( "cl_paused", "0" ); //We cannot send server commands while paused!


    UI_PushMenu( &s_votemenu_nextmap.menu );
}
