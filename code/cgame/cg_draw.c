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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#ifdef MISSIONPACK
#include "../ui/ui_shared.h"

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
#else
int drawTeamOverlayModificationCount = -1;
#endif

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

static float CG_DrawPickupItem( float y );
void CG_GetScoreColor(int team, float *color);

#ifdef MISSIONPACK

int CG_Text_Width(const char *text, float scale, int limit) {
  int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
// FIXME: see ui_main.c, same problem
//	const unsigned char *s = text;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  out = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
				out += glyph->xSkip;
				s++;
				count++;
			}
    }
  }
  return out * useScale;
}

int CG_Text_Height(const char *text, float scale, int limit) {
  int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
// TTimo: FIXME
//	const unsigned char *s = text;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  max = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
	      if (max < glyph->height) {
		      max = glyph->height;
			  }
				s++;
				count++;
			}
    }
  }
  return max * useScale;
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
  float w, h;
  w = width * scale;
  h = height * scale;
  CG_AdjustFrom640( &x, &y, &w, &h );
  trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
  int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  if (text) {
// TTimo: FIXME
//		const unsigned char *s = text;
		const char *s = text;
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
      //int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
      //float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar(x + ofs, y - yadj + ofs, 
														glyph->imageWidth,
														glyph->imageHeight,
														useScale, 
														glyph->s,
														glyph->t,
														glyph->s2,
														glyph->t2,
														glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar(x, y - yadj, 
													glyph->imageWidth,
													glyph->imageHeight,
													useScale, 
													glyph->s,
													glyph->t,
													glyph->s2,
													glyph->t2,
													glyph->glyph);
				// CG_DrawPic(x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph);
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
    }
	  trap_R_SetColor( NULL );
  }
}


#endif

#ifndef MISSIONPACK

static void CG_DrawFieldFloat (float x, float y, int width, int value, qboolean centered, float char_width, float char_height) {
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	if (centered) {
		x -= char_width*(l/2.0);
		if (l % 2 == 1) {
			x -= char_width/2.0;
		} 
	} else {
		x += char_width*(width - l);
	}

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( x,y, char_width, char_height, cgs.media.numberShaders[frame] );
		x += char_width;
		ptr++;
		l--;
	}
}
/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
static void CG_DrawField (int x, int y, int width, int value, qboolean centered, int char_width, int char_height) {
	CG_DrawFieldFloat(x, y, width, value, centered, char_width, char_height);
}
#endif // MISSIONPACK

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_Draw3DHead

================
*/
void CG_Draw3DHead( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles, clientInfo_t *ci) {
	refdef_t		refdef;
	refEntity_t		ent;
	qboolean autoHeadColors = qfalse;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	if ((ci->forcedBrightModel || (cgs.ratFlags & (RAT_BRIGHTSHELL | RAT_BRIGHTOUTLINE) 
					&& (cg_brightShells.integer || cg_brightOutline.integer) 
					&& (cgs.gametype != GT_FFA || cgs.ratFlags & RAT_ALLOWFORCEDMODELS)))
			&& ci->team != TEAM_SPECTATOR &&
			( (cg_teamHeadColorAuto.integer && ci->team == cg.snap->ps.persistant[PERS_TEAM])
			  || (cg_enemyHeadColorAuto.integer && ci->team != cg.snap->ps.persistant[PERS_TEAM])
			)) {
		CG_PlayerAutoHeadColor(ci, ent.shaderRGBA);
		autoHeadColors = qtrue;
	} else {
		CG_PlayerGetColors(ci, qfalse, MCIDX_HEAD, ent.shaderRGBA);
	}

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	if (!ci->forcedBrightModel) {
		if (cgs.ratFlags & RAT_BRIGHTSHELL && cg_brightShells.integer) {
			ent.shaderRGBA[3] = CG_GetBrightShellAlpha();
			if (cg_brightShells.integer == 2) {
				ent.customShader = cgs.media.brightShellFlat;
			} else {
				ent.customShader = (autoHeadColors || cg_brightShells.integer == 3) ? 
					cgs.media.brightShellBlend : cgs.media.brightShell;
			}
			trap_R_AddRefEntityToScene( &ent );
		} else if (cgs.ratFlags & RAT_BRIGHTOUTLINE && cg_brightOutline.integer) {
			ent.shaderRGBA[3] = CG_GetBrightOutlineAlpha();
			ent.customShader = (autoHeadColors || cg_brightOutline.integer == 2) ? 
				cgs.media.brightOutlineSmallBlend : cgs.media.brightOutlineSmall;
			trap_R_AddRefEntityToScene( &ent );
		}
	}
	trap_R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs;

	ci = &cgs.clientinfo[ clientNum ];

	if ( cg_draw3dIcons.integer ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->headOffset, origin );

		CG_Draw3DHead( x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles, ci );
	} else if ( cg_drawIcons.integer ) {
		CG_DrawPic( x, y, w, h, ci->modelIcon );
	}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons.integer ) {

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( cg.time / 2000.0 );;

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
			if(cgs.gametype == GT_DOUBLE_D){
				if(cgs.redflag == TEAM_BLUE)
					handle = cgs.media.blueFlagModel;
				if(cgs.redflag == TEAM_FREE)
					handle = cgs.media.neutralFlagModel;
				if(cgs.redflag == TEAM_NONE)
					handle = cgs.media.neutralFlagModel;
			}
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
			if(cgs.gametype == GT_DOUBLE_D){
				if(cgs.redflag == TEAM_BLUE)
					handle = cgs.media.blueFlagModel;
				if(cgs.redflag == TEAM_FREE)
					handle = cgs.media.neutralFlagModel;
				if(cgs.redflag == TEAM_NONE)
					handle = cgs.media.neutralFlagModel;
			}
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
================
CG_DrawStatusBarHead

================
*/
#ifndef MISSIONPACK

static void CG_DrawStatusBarHead( float x ) {
	vec3_t		angles;
	float		size, stretch;
	float		frac;

	VectorClear( angles );

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)(cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
		cg.headEndPitch = 5 * cos( crandom()*M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
			cg.headEndPitch = 5 * cos( crandom()*M_PI );
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

	CG_DrawHead( x, 480 - size, size, size, 
				cg.snap->ps.clientNum, angles );
}
#endif // MISSIONPACK

/*
================
CG_DrawStatusBarFlag

================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBarFlag( float x, int team ) {
	CG_DrawFlagModel( x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, qfalse );
}
#endif // MISSIONPACK

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}

#ifndef MISSIONPACK

#define	RAT_ICON_HEIGHT			20
#define	RAT_CHAR_HEIGHT			24
static void CG_DrawRatStatusBar( void ) {
	int			color;
	centity_t	*cent;
	playerState_t	*ps;
	int			value;
	int flagx = 0;
	qhandle_t	icon_a;
	qhandle_t	icon_h;
	int team = TEAM_FREE;
	float rat_char_width = CG_HeightToWidth((float)RAT_CHAR_HEIGHT*2.0/3.0);
	float rat_icon_width = CG_HeightToWidth(RAT_ICON_HEIGHT);
	float char_width = CG_HeightToWidth((float)CHAR_HEIGHT*2.0/3.0);
	float healthx = 320.0 - rat_char_width*2 - rat_char_width*2.0/3.0;
	float armorx = 320.0 + rat_char_width*2 + rat_char_width*2.0/3.0;
	int flagteam = TEAM_NUM_TEAMS;
        

	static float colors[4][4] = { 
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		team = cg.snap->ps.persistant[PERS_TEAM];
	} else {
		team = cgs.clientinfo[ cg.snap->ps.clientNum ].team;
	}

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;


	if (cg_ratStatusbar.integer == 2) {
		//flagx = armorx + CG_HeightToWidth(ICON_SIZE + TEXT_ICON_SPACE*2) + char_width*3;
		
		// adjust for lagometer
		flagx = 640 - 16 - 48 - CG_HeightToWidth(ICON_SIZE);
	} else {
		flagx = 32;
	}
	if( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
		flagteam = TEAM_RED;
	} else if( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		flagteam = TEAM_BLUE;
	} else if( cg.predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		flagteam = TEAM_FREE;
	}
	if (flagteam != TEAM_NUM_TEAMS) {
		CG_DrawFlagModel( flagx, 480 - ICON_SIZE, CG_HeightToWidth(ICON_SIZE), ICON_SIZE, flagteam, qfalse );
	}

	//
	// ammo
	//
	if ( cent->currentState.weapon ) {
		qhandle_t icon;
		value = ps->ammo[cent->currentState.weapon];
		if ( value > -1 ) {
			if ( cg.predictedPlayerState.weaponstate == WEAPON_FIRING
				&& cg.predictedPlayerState.weaponTime > 100 ) {
				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 1 ) {
					color = 0;	// green
				} else {
					color = 1;	// red
				}
			}
			trap_R_SetColor( colors[color] );
			
			CG_DrawField (320, 454, 3, value, qtrue, rat_char_width, RAT_CHAR_HEIGHT);
			trap_R_SetColor( NULL );

		}
		icon = cg_weapons[ cg.predictedPlayerState.weapon ].weaponIcon;
		if ( icon ) {
			CG_DrawPic( 320-rat_icon_width/2, 432, rat_icon_width, RAT_ICON_HEIGHT, icon );
		}
	}

	trap_R_SetColor( NULL );
	switch (team) {
		case TEAM_BLUE:
			icon_h = cgs.media.healthIconBlue;
			icon_a = cgs.media.armorIconBlue;
			break;
		case TEAM_RED:
			icon_h = cgs.media.healthIconRed;
			icon_a = cgs.media.armorIconRed;
			break;
		default:
			icon_h = cgs.media.healthIcon;
			icon_a = cgs.media.armorIcon;
			break;
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if ( value > 100 ) {
		trap_R_SetColor( colors[3] );
	} else if (value > 30) {
		trap_R_SetColor( colors[0] );
	} else {
		trap_R_SetColor( colors[1] );
	}

	CG_DrawField ( healthx - CG_HeightToWidth(ICON_SIZE + TEXT_ICON_SPACE) - char_width*3, 432, 3, value, qfalse, char_width, CHAR_HEIGHT);
	CG_DrawPic( healthx - CG_HeightToWidth(ICON_SIZE), 432, CG_HeightToWidth(ICON_SIZE), ICON_SIZE, icon_h );

	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if (value > 0 ) {
		if (value > 100) {
			trap_R_SetColor( colors[3] );
		} else  {
			trap_R_SetColor( colors[0] );
		}
		CG_DrawField (armorx + CG_HeightToWidth(ICON_SIZE+TEXT_ICON_SPACE), 432, 3, value, qfalse, char_width, CHAR_HEIGHT);
		trap_R_SetColor( NULL );
	}
	CG_DrawPic( armorx, 432, CG_HeightToWidth(ICON_SIZE), ICON_SIZE, icon_a );
}

static vec4_t weaponColors[WP_NUM_WEAPONS] =
	{
	{ 1.0, 1.0, 1.0, 1.0 }, // WP_NONE
	{ 0.0, 0.8, 1.0, 1.0 }, // WP_GAUNTLET,
	{ 1.0, 1.0, 0.0, 1.0 }, // WP_MACHINEGUN,
	{ 1.0, 0.4, 0.0, 1.0 }, // WP_SHOTGUN,
	{ 0.0, 0.6, 0.0, 1.0 }, // WP_GRENADE_LAUNCHER,
	{ 1.0, 0.0, 0.0, 1.0 }, // WP_ROCKET_LAUNCHER,
	{ 1.0, 1.0, 0.6, 1.0 }, // WP_LIGHTNING,
	{ 0.0, 1.0, 0.0, 1.0 }, // WP_RAILGUN,
	{ 1.0, 0.0, 1.0, 1.0 }, // WP_PLASMAGUN,
	{ 0.0, 0.4, 1.0, 1.0 }, // WP_BFG,
	{ 0.4, 0.6, 0.0, 1.0 }, // WP_GRAPPLING_HOOK,
	{ 1.0, 0.6, 0.6, 1.0 }, // WP_NAILGUN,
	{ 1.0, 0.6, 0.4, 1.0 }, // WP_PROX_LAUNCHER,
	{ 0.8, 0.8, 0.8, 1.0 }, // WP_CHAINGUN,
	};

static float *CG_GetWeaponColor(int weapon) {
	if (weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS) {
		return weaponColors[0];
	}
	return weaponColors[weapon];
}


#define	DOTBAR_UNLIT_ALPHA		0.2
//#define PULSEDELEMENT_PULSETIME		200
#define PULSEDELEMENT_PULSETIME		250
#define PULSEDELEMENT_CONTINUOUS_PERIOD	400
#define PULSEDELEMENT_OVERLAY_ALPHA	0.5

float CG_PulseFactor(int lastfilledtime) {
	float f;
	f = MAX(0,MIN(PULSEDELEMENT_PULSETIME, cg.time - lastfilledtime));

	// starts at 0, ends at 0
	f = sin(f/PULSEDELEMENT_PULSETIME * M_PI);
	return f;
}

float CG_PulseFactorContinuous(int lastfilledtime) {
	float f;
	f = (float)(cg.time - lastfilledtime)/PULSEDELEMENT_CONTINUOUS_PERIOD;

	f = sin(f * M_PI);
	return f;
}
void CG_DrawPulsedElementOverlay(float x, float y, float w, float h,
	       	qhandle_t shader, qhandle_t glowshader, qhandle_t additiveglowshader,
	       	int lastfilledtime, qboolean continuousPulse, float *color) {
	float f;

	if (continuousPulse) {
		f = CG_PulseFactorContinuous(lastfilledtime);
	} else {
		f = CG_PulseFactor(lastfilledtime);
	}

	if (f > 0.01) {
		color[3] = f;
		trap_R_SetColor(color);
		CG_DrawPic( x, y, w, h, glowshader);
	}
	color[3] = 1.0;
	trap_R_SetColor(color);
	CG_DrawPic( x, y, w, h, additiveglowshader);
	// This is done by the additive glow shader itself now
	//if (shader) {
	//	color[3] = PULSEDELEMENT_OVERLAY_ALPHA;
	//	trap_R_SetColor(color);
	//	CG_DrawPic( x, y, w, h, shader);
	//}
}

static float CG_DrawDottedBar(float x, float y, dotbar_t *dotbar, int num_elements, float dotheight, float xspace, float yspace, int weapon, int value, int maxvalue) {
	static float healthbarcolors[4][4] = { 
		{ 1.0f, 1.0f, 1.0f, 1.0f },      // white
		//{ 1.0f, 0.0f, 0.0f, 1.0f },     // red
		{ 1.0f, 0.071f, 0.1177f, 1.0f },     // red
		{ 0.138f, 0.812f, 1.0f, DOTBAR_UNLIT_ALPHA },     // health > 100, not lit
		{ 0.0f, 0.8f, 1.0f, 1.0f } };   // health > 100
		//{ 0.138f, 0.812f, 1.0f, 1.0f } };   // health > 100
	float dotwidth = CG_HeightToWidth(dotheight);
	int stepvalue = maxvalue/num_elements;
	int i;
	int v;
	float xx, yy;
	vec4_t color = { 1.0, 1.0, 1.0, 1.0 };

	if (num_elements > DOTBAR_MAX_ELEMENTS) {
		return 0;
	}


	if (weapon != WP_NONE) {
		memcpy(color, CG_GetWeaponColor(weapon), sizeof(color));
	}

	xspace = CG_HeightToWidth(xspace);
	xx = x;
	yy = y;
	v = 0;
	for (i = 0; i < num_elements; ++i) {
		v += stepvalue;
		if ((value >= v && weapon == WP_NONE) || (weapon != WP_NONE && value > v - stepvalue)) {
			// this element is active, because we have that much health/armor/ammo
			if (weapon == WP_NONE) {
				// this is a health/armor bar
				if (v <= 100) {
					memcpy(color, healthbarcolors[0], sizeof(color));
				} else {
					memcpy(color, healthbarcolors[3], sizeof(color));
				}
			} else {
				// this is a weapon bar
				color[3] = 1.0;
			}

			if (dotbar->filled[i] != DB_FILLED_FULL && dotbar->filled[i] != DB_FILLED_EMPTYGLOW) {
				dotbar->lastFilledTimes[i] = cg.time;
			}
			CG_DrawPulsedElementOverlay(xx, yy,
				       	dotwidth, dotheight,
				       	cgs.media.bardot, cgs.media.bardot_transparentglow, cgs.media.bardot_additiveglow,
				       	dotbar->lastFilledTimes[i],
					qfalse,
				       	color);

			dotbar->filled[i] = DB_FILLED_FULL;

			//color[3] = alpha;
			//trap_R_SetColor(color);
			//CG_DrawPic( xx, yy, dotwidth, dotheight, cgs.media.bardot );
		} else {
			// this element is empty
			if (v <= 100 && weapon == WP_NONE) {
				// element is lighting up to signify lack of health/armor
				memcpy(color, healthbarcolors[1], sizeof(color));

				if (dotbar->filled[i] != DB_FILLED_FULL && dotbar->filled[i] != DB_FILLED_EMPTYGLOW) {
					dotbar->lastFilledTimes[i] = cg.time;
				}
				CG_DrawPulsedElementOverlay(xx, yy,
						dotwidth, dotheight,
						cgs.media.bardot, cgs.media.bardot_transparentglow, cgs.media.bardot_additiveglow,
						dotbar->lastFilledTimes[i],
						qfalse,
					       	color);

				dotbar->filled[i] = DB_FILLED_EMPTYGLOW;
			} else {
				// element is unlit
				
				if (weapon == WP_NONE) {
					memcpy(color, healthbarcolors[2], sizeof(color));
				} else {
					color[3] = DOTBAR_UNLIT_ALPHA;
				}
				trap_R_SetColor(color);
				CG_DrawPic( xx, yy, dotwidth, dotheight, cgs.media.bardot );
				//CG_DrawPic( xx, yy, dotwidth, dotheight, cgs.media.bardot_outline );
				dotbar->filled[i] = DB_FILLED_EMPTY;
				dotbar->lastFilledTimes[i] = 0;
			}
		}
		xx += xspace;
		yy += yspace;
	}
	trap_R_SetColor(NULL);
	if (yspace != 0) {
		return yy;
	} 
	return xx;
}

void CG_ResetStatusbar(void) {
	if (cg_ratStatusbar.integer) {
		memset(&cg.healthbar, 0, sizeof(cg.healthbar));
		memset(&cg.armorbar, 0, sizeof(cg.armorbar));
		memset(&cg.weaponbar, 0, sizeof(cg.weaponbar));
	}
}

#define	RSB4_BIGCHAR_HEIGHT		28
#define	RSB5_BIGCHAR_HEIGHT		20

#define	RSB4_BIGICON_HEIGHT		27

#define	RSB4_WEAPICON_HEIGHT		16
#define	RSB4_WEAPCHAR_HEIGHT		RSB4_WEAPICON_HEIGHT
#define	RSB4_WEAPCHAR_YOFFSET		7

#define RSB4_BAR_MARGIN			4
#define RSB4_BOTTOM_MARGIN		2

#define	RSB4_CENTER_SPACING		5
#define RSB4_NUMBER_XOFFSET		12
#define RSB4_NUMBER_YOFFSET		1

#define RSB5_NUMBER_YOFFSET		14

#define RSB4_UNLIT_ALPHA		0.2
#define RSB4_DECOR_ALPHA		0.5

#define RSB4_HABAR_MAXVALUE		175
// excludes the extra element (ring) at the end that lights up together with the last real element
#define RSB4_HABAR_COUNTINGELEMENTS	7

// height of the statusbar relative to the 640x480 virtual screen
#define RSB4_HEIGHT			48.0
#define RSB4_HABAR_HEIGHT		RSB4_HEIGHT

/*
 * These sizes and offsets all refer to pixel measurements in the original (1024x256) image of the health bar
 */
// THIS SHOULD BE THE HEIGHT OF THE STATUS HEALTH/ARMOR BAR IN THE DIMENSIONS OF THE ORIGINAL IMAGE(s)
// It should correspond to RSB4_HABAR_HEIGHT, i.e. in the game,
// RSB4_HABAR_ORIGHEIGHT pixels of the original image will be scaled to
// RSB4_HABAR_HEIGHT pixels on the virtual 640x480 display
#define RSB4_HABAR_ORIGHEIGHT		256

// point in the original image where the actual content starts (bottom right corner)
#define RSB4_HABAR_BASE_XOFFSET		955
#define RSB4_HABAR_BASE_YOFFSET		222
#define RSB4_HABAR_ICON_XOFFSET		167
#define RSB4_HABAR_ICON_YOFFSET		126
// how much the image content needs to be scaled to fit into the statusbar
#define RSB4_HABAR_BASE_SCALE		1.13

#define RSB4_WABAR_ICON_XOFFSET		152
#define RSB4_WABAR_ICON_YOFFSET		623
// bottom left corner
#define RSB4_WABAR_BASE_XOFFSET 	20
#define RSB4_WABAR_BASE_YOFFSET	 	704

static const int habar_decor_xoffsets[RSB4_NUM_HA_BAR_DECOR_ELEMENTS] = {
	904,
	88,
	140,
};
static const int habar_decor_yoffsets[RSB4_NUM_HA_BAR_DECOR_ELEMENTS] = {
	0,
	44,
	147,
};
static const int habar_decor_widths[RSB4_NUM_HA_BAR_DECOR_ELEMENTS] = {
	64,
	128,
	128,
};
static const int habar_decor_heights[RSB4_NUM_HA_BAR_DECOR_ELEMENTS] = {
	256,
	32,
	64,
};

static const int habar_xoffsets[RSB4_NUM_HA_BAR_ELEMENTS] = {
	791,
	693,
	595,
	497,
	399,
	301,
	203,
	39,
};
static const int habar_yoffsets[RSB4_NUM_HA_BAR_ELEMENTS] = {
	16,
	16,
	16,
	16,
	16,
	16,
	16,
	0,
};
static const int habar_widths[RSB4_NUM_HA_BAR_ELEMENTS] = {
	128,
	128,
	128,
	128,
	128,
	128,
	128,
	256,
};
static const int habar_heights[RSB4_NUM_HA_BAR_ELEMENTS] = {
	128,
	128,
	128,
	128,
	128,
	128,
	128,
	256,
};

static const int wabar_decor_xoffsets[RSB4_NUM_W_BAR_DECOR_ELEMENTS] = {
	128,
	76,
	66,
};
static const int wabar_decor_yoffsets[RSB4_NUM_W_BAR_DECOR_ELEMENTS] = {
	549,
	598,
	286,
};
static const int wabar_decor_widths[RSB4_NUM_W_BAR_DECOR_ELEMENTS] = {
	128,
	32,
	64,
};
static const int wabar_decor_heights[RSB4_NUM_W_BAR_DECOR_ELEMENTS] = {
	256,
	32,
	256,
};

static const int wabar_xoffsets[RSB4_NUM_W_BAR_ELEMENTS] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};
static const int wabar_yoffsets[RSB4_NUM_W_BAR_ELEMENTS] = {
	622,
	579,
	536,
	493,
	450,
	407,
	364,
	321,
	278,
	235,
	500,
};
static const int wabar_widths[RSB4_NUM_W_BAR_ELEMENTS] = {
	256,
	128,
	128,
	128,
	128,
	128,
	128,
	128,
	128,
	128,
	256,

};
static const int wabar_heights[RSB4_NUM_W_BAR_ELEMENTS] = {
	128,
	128,
	128,
	128,
	128,
	128,
	128,
	128,
	128,
	128,
	256,
};

// Y scale relative to height
float CG_HABarElementScaleY(int elem_height) {
	return (float)elem_height/(float)RSB4_HABAR_ORIGHEIGHT * RSB4_HABAR_BASE_SCALE;
}
// X scale relative to height
float CG_HABarElementScaleX(int elem_width) {
	return (float)elem_width/(float)RSB4_HABAR_ORIGHEIGHT * RSB4_HABAR_BASE_SCALE;
}
// how much higher up to draw the image so that the edge is aligned with the desired Y coordinate
float CG_HABarOffsetY(int elem_yoffset, int elemHeight, float statusbarHeight, qboolean vflipped) {
	if (vflipped) {
		// for Ratstatusbar 5
		return (float)(-35 + elemHeight+elem_yoffset)/(float)RSB4_HABAR_ORIGHEIGHT * statusbarHeight * RSB4_HABAR_BASE_SCALE;
	}
	return (float)(RSB4_HABAR_BASE_YOFFSET - elem_yoffset)/(float)RSB4_HABAR_ORIGHEIGHT * statusbarHeight * RSB4_HABAR_BASE_SCALE;
}
// how much more to the right to draw the image so that the edge is aligned
float CG_HABarOffsetX(int elem_xoffset, int elem_width, float statusbarHeight, qboolean isArmor) {
	int dist_from_right = - (elem_xoffset - RSB4_HABAR_BASE_XOFFSET);
	if (isArmor) {
		dist_from_right -= elem_width;
	}
	return (float)(dist_from_right)/(float)RSB4_HABAR_ORIGHEIGHT * statusbarHeight * RSB4_HABAR_BASE_SCALE;
}

float CG_WABarOffsetY(int elem_yoffset, float statusbarHeight) {
	return (float)(RSB4_WABAR_BASE_YOFFSET - elem_yoffset)/(float)RSB4_HABAR_ORIGHEIGHT * statusbarHeight * RSB4_HABAR_BASE_SCALE;
}
float CG_WABarOffsetX(int elem_xoffset, float statusbarHeight) {
	int dist_from_left =  elem_xoffset - RSB4_WABAR_BASE_XOFFSET;
	return (float)(dist_from_left)/(float)RSB4_HABAR_ORIGHEIGHT * statusbarHeight * RSB4_HABAR_BASE_SCALE;
}

static void CG_DrawHABarDecor(float x, float y, float barheight, qboolean isArmor, qboolean vflipped) {
	float w;
	float h;
	float xx;
	float yy;
	int i;
	qhandle_t *shaders;
	float color[4] = { 1.0f, 1.0, 1.0f, RSB4_DECOR_ALPHA };

	if (!cg_drawHabarDecor.integer) {
		return;
	}

	trap_R_SetColor(color);

	if (isArmor) {
		shaders = cgs.media.rsb4_armor_decorShaders;
	} else {
		shaders = cgs.media.rsb4_health_decorShaders;
	}

	xx = x;
	for (i = 0; i < RSB4_NUM_HA_BAR_DECOR_ELEMENTS; ++i) {
		h = barheight * CG_HABarElementScaleY(habar_decor_heights[i]);
		w = CG_HeightToWidth(barheight * CG_HABarElementScaleX(habar_decor_widths[i]));

		yy = y - CG_HABarOffsetY(habar_decor_yoffsets[i], habar_decor_heights[i], barheight, vflipped);

		if (isArmor) {
			xx = x + CG_HeightToWidth(CG_HABarOffsetX(habar_decor_xoffsets[i], habar_decor_widths[i], barheight, isArmor));
		} else {
			xx = x - CG_HeightToWidth(CG_HABarOffsetX(habar_decor_xoffsets[i], habar_decor_widths[i], barheight, isArmor));
		}
		CG_DrawPic( xx, yy, w, h, shaders[i]);
	}
	trap_R_SetColor(NULL);
}

static void CG_DrawHABar(float x, float y, dotbar_t *dotbar, float barheight, int value, qboolean isArmor, qboolean vflipped) {
	static float healthbarcolors[4][4] = { 
		{ 1.0f, 1.0f, 1.0f, 1.0f },      // white
		//{ 1.0f, 0.0f, 0.0f, 1.0f },     // red
		{ 1.0f, 0.071f, 0.1177f, 1.0f },     // red
		{ 0.138f, 0.812f, 1.0f, RSB4_UNLIT_ALPHA },     // health > 100, not lit
		{ 0.0f, 0.8f, 1.0f, 1.0f } };   // health > 100
		//{ 0.138f, 0.812f, 1.0f, 1.0f } };   // health > 100
	float w;
	float h;
	float xx;
	float yy;
	int i;
	int v;
	vec4_t color = { 1.0, 1.0, 1.0, 1.0 };
	qhandle_t *shaders;
	qhandle_t *glowShaders;
	qhandle_t *additiveGlowShaders;

	if (isArmor) {
		shaders = cgs.media.rsb4_armor_shaders;
		glowShaders = cgs.media.rsb4_armor_glowShaders;
		additiveGlowShaders = cgs.media.rsb4_armor_additiveGlowShaders;
	} else {
		shaders = cgs.media.rsb4_health_shaders;
		glowShaders = cgs.media.rsb4_health_glowShaders;
		additiveGlowShaders = cgs.media.rsb4_health_additiveGlowShaders;
	}

	xx = x;
	v = 0;
	for (i = 0; i < RSB4_NUM_HA_BAR_ELEMENTS; ++i) {
		v += (RSB4_HABAR_MAXVALUE/RSB4_HABAR_COUNTINGELEMENTS);

		h = barheight * CG_HABarElementScaleY(habar_heights[i]);
		w = CG_HeightToWidth(barheight * CG_HABarElementScaleX(habar_widths[i]));

		yy = y - CG_HABarOffsetY(habar_yoffsets[i], habar_heights[i], barheight, vflipped);

		if (isArmor) {
			xx = x + CG_HeightToWidth(CG_HABarOffsetX(habar_xoffsets[i], habar_widths[i], barheight, isArmor));
		} else {
			xx = x - CG_HeightToWidth(CG_HABarOffsetX(habar_xoffsets[i], habar_widths[i], barheight, isArmor));
		}

		if (value >= v || value >= RSB4_HABAR_MAXVALUE) {
			// this element is active, because we have that much health/armor/ammo
			if (v <= 100) {
				memcpy(color, healthbarcolors[0], sizeof(color));
			} else {
				memcpy(color, healthbarcolors[3], sizeof(color));
			}

			if (dotbar->filled[i] != DB_FILLED_FULL && dotbar->filled[i] != DB_FILLED_EMPTYGLOW) {
				dotbar->lastFilledTimes[i] = cg.time;
			}
			CG_DrawPulsedElementOverlay(xx, yy,
					w, h,
					shaders[i], glowShaders[i], additiveGlowShaders[i],
					dotbar->lastFilledTimes[i],
					qfalse,
				       	color);

			dotbar->filled[i] = DB_FILLED_FULL;

		} else {
			// this element is empty
			if (v <= 100) {
				// element is lighting up to signify lack of health/armor
				memcpy(color, healthbarcolors[1], sizeof(color));

				if (dotbar->filled[i] != DB_FILLED_FULL && dotbar->filled[i] != DB_FILLED_EMPTYGLOW) {
					dotbar->lastFilledTimes[i] = cg.time;
				}
				CG_DrawPulsedElementOverlay(xx, yy,
						w, h,
						shaders[i], glowShaders[i], additiveGlowShaders[i],
						dotbar->lastFilledTimes[i],
						qfalse,
					       	color);

				dotbar->filled[i] = DB_FILLED_EMPTYGLOW;
			} else {
				// element is unlit

				memcpy(color, healthbarcolors[2], sizeof(color));
				trap_R_SetColor(color);
				CG_DrawPic( xx, yy, w, h, shaders[i] );
				dotbar->filled[i] = DB_FILLED_EMPTY;
				dotbar->lastFilledTimes[i] = 0;
			}
		}
	}
	trap_R_SetColor(NULL);
}

static float CG_DrawWABarDecor(float x, float y, float barheight) {
	float w;
	float h;
	float xx;
	float yy;
	int i;
	float max_x = 0.0;
	float color[4] = { 1.0f, 1.0, 1.0f, RSB4_DECOR_ALPHA };

	trap_R_SetColor(color);

	xx = x;
	for (i = 0; i < RSB4_NUM_W_BAR_DECOR_ELEMENTS; ++i) {
		h = barheight * CG_HABarElementScaleY(wabar_decor_heights[i]);
		w = CG_HeightToWidth(barheight * CG_HABarElementScaleX(wabar_decor_widths[i]));

		yy = y - CG_WABarOffsetY(wabar_decor_yoffsets[i], barheight);

		xx = x + CG_HeightToWidth(CG_WABarOffsetX(wabar_decor_xoffsets[i], barheight));
		if (cg_drawHabarDecor.integer) {
			CG_DrawPic( xx, yy, w, h, cgs.media.rsb4_weapon_decorShaders[i]);
		}

		if (xx + w > max_x) {
			max_x = xx + w;
		}
	}
	trap_R_SetColor(NULL);
	return max_x;
}

static qboolean CG_AmmoLow(int weapon, int ammo) {
	return MIN(100, (ammo*100)/CG_FullAmmo(weapon)) <= 20;
}

// excludes the extra element (ring) at the end that lights up together with the last real element
#define RSB4_WABAR_COUNTINGELEMENTS	10
static void CG_DrawWeaponAmmoBar(float x, float y, dotbar_t *dotbar, float barheight, int weapon, int value, int maxvalue) {
	float w;
	float h;
	float xx;
	float yy;
	int i;
	int v;
	int stepvalue = maxvalue/RSB4_WABAR_COUNTINGELEMENTS;
	vec4_t color = { 1.0, 1.0, 1.0, 1.0 };

	if (weapon != WP_NONE) {
		memcpy(color, CG_GetWeaponColor(weapon), sizeof(color));
	}

	xx = x;
	v = 0;
	for (i = 0; i < RSB4_NUM_W_BAR_ELEMENTS; ++i) {
		h = barheight * CG_HABarElementScaleY(wabar_heights[i]);
		w = CG_HeightToWidth(barheight * CG_HABarElementScaleX(wabar_widths[i]));

		yy = y - CG_WABarOffsetY(wabar_yoffsets[i], barheight);

		xx = x + CG_HeightToWidth(CG_WABarOffsetX(wabar_xoffsets[i], barheight));

		// ring only lights up with full ammo
		//if (value > v || value >= maxvalue) {
		// ring lights up as long as ammo > 0
		if (value > v || (i >= RSB4_WABAR_COUNTINGELEMENTS && value > 0)) {
			// this element is active, because we have that much ammo
			if (dotbar->filled[i] != DB_FILLED_FULL && dotbar->filled[i] != DB_FILLED_EMPTYGLOW) {
				dotbar->lastFilledTimes[i] = cg.time;
			}
			CG_DrawPulsedElementOverlay(xx, yy,
					w, h,
					cgs.media.rsb4_weapon_shaders[i],
					cgs.media.rsb4_weapon_glowShaders[i],
				       	cgs.media.rsb4_weapon_additiveGlowShaders[i],
					dotbar->lastFilledTimes[i],
					CG_AmmoLow(weapon, value),
				       	color);

			dotbar->filled[i] = DB_FILLED_FULL;

		} else {
			// element is unlit

			color[3] = RSB4_UNLIT_ALPHA;
			trap_R_SetColor(color);
			CG_DrawPic( xx, yy, w, h, cgs.media.rsb4_weapon_shaders[i] );
			dotbar->filled[i] = DB_FILLED_EMPTY;
			dotbar->lastFilledTimes[i] = 0;
		}
		v += stepvalue;
	}
	trap_R_SetColor(NULL);
}

void CG_Ratstatusbar4RegisterShaders(void) {
	int i;
	int rsbtype = cg_ratStatusbar.integer;
	if (rsbtype < 4 || rsbtype > 5) {
		rsbtype = 4;
	}
	for (i = 0; i < RSB4_NUM_HA_BAR_ELEMENTS; ++i) {
		if (i > 0 && i <= 2) {
			// some of these elements are identical, so just use the same shader
			cgs.media.rsb4_health_shaders[i] = cgs.media.rsb4_health_shaders[i-1];
			cgs.media.rsb4_health_glowShaders[i] = cgs.media.rsb4_health_glowShaders[i-1];
			cgs.media.rsb4_health_additiveGlowShaders[i] = cgs.media.rsb4_health_additiveGlowShaders[i-1];

			cgs.media.rsb4_armor_shaders[i] = cgs.media.rsb4_armor_shaders[i-1];
			cgs.media.rsb4_armor_glowShaders[i] = cgs.media.rsb4_armor_glowShaders[i-1];
			cgs.media.rsb4_armor_additiveGlowShaders[i] = cgs.media.rsb4_armor_additiveGlowShaders[i-1];
		} else {
			cgs.media.rsb4_health_shaders[i] = trap_R_RegisterShader(va("rsb%i_health_e%i", rsbtype, i+1));
			cgs.media.rsb4_health_glowShaders[i] = trap_R_RegisterShader(va("rsb%i_health_e%i_glow", rsbtype, i+1));
			cgs.media.rsb4_health_additiveGlowShaders[i] = trap_R_RegisterShader(va("rsb%i_health_e%i_glow_additive", rsbtype, i+1));

			cgs.media.rsb4_armor_shaders[i] = trap_R_RegisterShader(va("rsb%i_armor_e%i", rsbtype, i+1));
			cgs.media.rsb4_armor_glowShaders[i] = trap_R_RegisterShader(va("rsb%i_armor_e%i_glow", rsbtype, i+1));
			cgs.media.rsb4_armor_additiveGlowShaders[i] = trap_R_RegisterShader(va("rsb%i_armor_e%i_glow_additive", rsbtype, i+1));
		}
	}

	if (cg_drawHabarDecor.integer) {
		for (i = 0; i < RSB4_NUM_HA_BAR_DECOR_ELEMENTS; ++i) {
			cgs.media.rsb4_health_decorShaders[i] = trap_R_RegisterShader(va("rsb%i_health_decor%i", rsbtype, i+1));
			cgs.media.rsb4_armor_decorShaders[i] = trap_R_RegisterShader(va("rsb%i_armor_decor%i", rsbtype, i+1));
		}
	}

	for (i = 0; i < RSB4_NUM_W_BAR_ELEMENTS; ++i) {
		if (i > 3 && i <= 9) {
			// some of these elements are identical, so just use the same shader
			cgs.media.rsb4_weapon_shaders[i] = cgs.media.rsb4_weapon_shaders[i-1];
			cgs.media.rsb4_weapon_glowShaders[i] = cgs.media.rsb4_weapon_glowShaders[i-1];
			cgs.media.rsb4_weapon_additiveGlowShaders[i] = cgs.media.rsb4_weapon_additiveGlowShaders[i-1];
		} else {
			cgs.media.rsb4_weapon_shaders[i] = trap_R_RegisterShader(va("rsb_weapon_e%i", i+1));
			cgs.media.rsb4_weapon_glowShaders[i] = trap_R_RegisterShader(va("rsb_weapon_e%i_glow", i+1));
			cgs.media.rsb4_weapon_additiveGlowShaders[i] = trap_R_RegisterShader(va("rsb_weapon_e%i_glow_additive", i+1));
		}
	}

	if (cg_drawHabarDecor.integer) {
		for (i = 0; i < RSB4_NUM_W_BAR_DECOR_ELEMENTS; ++i) {
			cgs.media.rsb4_weapon_decorShaders[i] = trap_R_RegisterShader(va("rsb_weapon_decor%i", i+1));
		}
	}

	if (cg_drawHabarBackground.integer) {
		cgs.media.rsb4_health_bg = trap_R_RegisterShader("rsb4_health_bg");
		cgs.media.rsb4_health_bg_border = trap_R_RegisterShader("rsb4_health_bg_border");
		cgs.media.rsb4_armor_bg = trap_R_RegisterShader("rsb4_armor_bg");
		cgs.media.rsb4_armor_bg_border = trap_R_RegisterShader("rsb4_armor_bg_border");
	}
}

static void CG_DrawRatStatusBar4( void ) {
	float x;
	float y;
	playerState_t	*ps;
	int			value;
	int k;
	int numDigits;
	qhandle_t	icon_a;
	qhandle_t	icon_h;
	int team = TEAM_FREE;
	float bigchar_width;
	float bigchar_height;
	float weaponchar_width = CG_HeightToWidth(RSB4_WEAPCHAR_HEIGHT);
	int flagteam = TEAM_NUM_TEAMS;
	int weaponSelect = CG_GetWeaponSelect();
	float flag_x = RSB4_BAR_MARGIN;
	qboolean vflipped = (cg_ratStatusbar.integer == 5);
	int number_yoffset = vflipped ? RSB5_NUMBER_YOFFSET : RSB4_NUMBER_YOFFSET;

	static float colors[2][4] = { 
		{ 1.0f, 1.0f, 1.0f, 1.0f },    // normal
		{ 1.0f, 0.071f, 0.1177f, 1.0f },     // low health/armor
	};

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	if (vflipped) {
		bigchar_height = CG_HeightToWidth(RSB5_BIGCHAR_HEIGHT);
	} else { 
		bigchar_height = CG_HeightToWidth(RSB4_BIGCHAR_HEIGHT);
	}
	bigchar_width = CG_HeightToWidth(bigchar_height);

	if (cgs.media.rsb4_shadersLoaded != cg_ratStatusbar.integer) {
		CG_Ratstatusbar4RegisterShaders();
	}


	ps = &cg.snap->ps;


	//////
	////// ammo
	//
	if ( weaponSelect && weaponSelect > WP_NONE && weaponSelect < WP_NUM_WEAPONS) {
		qhandle_t weapicon;

		if (cg_predictWeapons.integer) {
			value=cg.predictedPlayerState.ammo[weaponSelect];
		} else {
			value=cg.snap->ps.ammo[weaponSelect];
		}

		x = RSB4_BAR_MARGIN;
		y = SCREEN_HEIGHT - RSB4_BAR_MARGIN;

		flag_x = CG_DrawWABarDecor(x, y, RSB4_HABAR_HEIGHT);
		CG_DrawWeaponAmmoBar(x,
			       	y,
			       	&cg.weaponbar,
				RSB4_HABAR_HEIGHT, 
			       	weaponSelect,
				value > -1 ? value : 200,
				value > -1 ? CG_FullAmmo(weaponSelect)*2 : 200
				);

		y -= CG_WABarOffsetY(RSB4_WABAR_ICON_YOFFSET, RSB4_HABAR_HEIGHT) + RSB4_WEAPICON_HEIGHT/2.0,
		x += CG_HeightToWidth(CG_WABarOffsetX(RSB4_WABAR_ICON_XOFFSET, RSB4_HABAR_HEIGHT) - RSB4_WEAPICON_HEIGHT/2.0),
		weapicon = cg_weapons[ weaponSelect ].weaponIcon;
		if ( weapicon ) {
			CG_DrawPic( x,
					y,
					CG_HeightToWidth(RSB4_WEAPICON_HEIGHT),
					RSB4_WEAPICON_HEIGHT,
					weapicon );
		}
		y -= RSB4_WEAPCHAR_YOFFSET + RSB4_WEAPCHAR_HEIGHT;
		if (value > -1) {
			trap_R_SetColor( CG_GetWeaponColor(weaponSelect) );
			k = value;
			numDigits = 1;
			while ((k /= 10) > 0) {
				numDigits++;
			}
			CG_DrawFieldFloat(x - CG_HeightToWidth(RSB4_WEAPCHAR_HEIGHT)/2.0,
					y,
					numDigits, 
					value,
					qfalse,
					weaponchar_width,
					RSB4_WEAPCHAR_HEIGHT );
		}
	}

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		team = cg.snap->ps.persistant[PERS_TEAM];
	} else {
		team = cgs.clientinfo[ cg.snap->ps.clientNum ].team;
	}

	if( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
		flagteam = TEAM_RED;
	} else if( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		flagteam = TEAM_BLUE;
	} else if( cg.predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		flagteam = TEAM_FREE;
	}
	if (flagteam != TEAM_NUM_TEAMS) {
		CG_DrawFlagModel( flag_x + CG_HeightToWidth(3), 480 - ICON_SIZE, CG_HeightToWidth(ICON_SIZE), ICON_SIZE, flagteam, qfalse );
	}


	trap_R_SetColor( NULL );
	switch (team) {
		case TEAM_BLUE:
			icon_h = cgs.media.healthIconBlue;
			icon_a = cgs.media.armorIconBlue;
			break;
		case TEAM_RED:
			icon_h = cgs.media.healthIconRed;
			icon_a = cgs.media.armorIconRed;
			break;
		default:
			//icon_h = cgs.media.healthIcon;
			//icon_a = cgs.media.armorIcon;
			icon_h = cgs.media.healthIconRed;
			icon_a = cgs.media.armorIconRed;
			break;
	}


	//
	// health
	//
	value = ps->stats[STAT_HEALTH];

	x = SCREEN_WIDTH/2.0 - CG_HeightToWidth(RSB4_CENTER_SPACING);
	y = SCREEN_HEIGHT - RSB4_BAR_MARGIN;
	CG_DrawPic( x - CG_HeightToWidth(CG_HABarOffsetX(RSB4_HABAR_ICON_XOFFSET, 0, RSB4_HABAR_HEIGHT, qfalse) + RSB4_BIGICON_HEIGHT/2.0),
			y - CG_HABarOffsetY(RSB4_HABAR_ICON_YOFFSET, 0, RSB4_HABAR_HEIGHT, vflipped) - RSB4_BIGICON_HEIGHT/2.0,
			CG_HeightToWidth(RSB4_BIGICON_HEIGHT),
			RSB4_BIGICON_HEIGHT,
		       	icon_h );

	if (value > 30) {
		trap_R_SetColor( colors[0] );
	} else {
		trap_R_SetColor( colors[1] );
	}


	CG_DrawFieldFloat(x - CG_HeightToWidth(RSB4_NUMBER_XOFFSET) - bigchar_width * 3,
		       	SCREEN_HEIGHT - RSB4_BOTTOM_MARGIN - number_yoffset - bigchar_height,
		       	3,
		       	value,
		       	qfalse,
		       	bigchar_width,
		       	bigchar_height);
	trap_R_SetColor( NULL );

	
	CG_DrawHABarDecor(x,
			y,
			RSB4_HABAR_HEIGHT, 
			qfalse,
			vflipped);
	CG_DrawHABar(x,
			y,
			&cg.healthbar,
			RSB4_HABAR_HEIGHT, 
			value,
			qfalse,
			vflipped);

	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];

	x = SCREEN_WIDTH/2.0 + CG_HeightToWidth(RSB4_CENTER_SPACING);
	y = SCREEN_HEIGHT - RSB4_BAR_MARGIN;
	CG_DrawPic( x + CG_HeightToWidth(CG_HABarOffsetX(RSB4_HABAR_ICON_XOFFSET, 0, RSB4_HABAR_HEIGHT, qtrue) - RSB4_BIGICON_HEIGHT/2.0),
			y - CG_HABarOffsetY(RSB4_HABAR_ICON_YOFFSET, 0, RSB4_HABAR_HEIGHT, vflipped) - RSB4_BIGICON_HEIGHT/2.0,
			CG_HeightToWidth(RSB4_BIGICON_HEIGHT),
			RSB4_BIGICON_HEIGHT,
		       	icon_a );
	if (value > 0 ) {
		if (value > 30) {
			trap_R_SetColor( colors[0] );
		} else {
			trap_R_SetColor( colors[1] );
		}
		k = value;
		numDigits = 1;
		while ((k /= 10) > 0) {
			numDigits++;
		}
		CG_DrawFieldFloat (x + CG_HeightToWidth(RSB4_NUMBER_XOFFSET),
			       	SCREEN_HEIGHT - RSB4_BOTTOM_MARGIN - number_yoffset - bigchar_height,
			       	numDigits,
			       	value,
			       	qfalse,
			       	bigchar_width,
			       	bigchar_height);
		trap_R_SetColor( NULL );
	}

	CG_DrawHABarDecor(x,
			y,
			RSB4_HABAR_HEIGHT, 
			qtrue,
			vflipped);
	CG_DrawHABar(x,
			y,
			&cg.armorbar,
			RSB4_HABAR_HEIGHT, 
			value,
			qtrue,
			vflipped);


	trap_R_SetColor( NULL );
}

static void CG_DrawRatStatusBar4Bg( void ) {
	float bg_color[4] = { 0.0, 0.0, 0.0, 0.33 };
	float border_color[4] = { 0.33, 0.33, 0.33, 1.0 };
	float h = RSB4_HABAR_HEIGHT*RSB4_HABAR_BASE_SCALE;
	float w = CG_HeightToWidth(4*h);
	float y = SCREEN_HEIGHT - h * 0.92;
	int team = TEAM_FREE;

	if (!cg_drawHabarBackground.integer) {
		return;
	}

	trap_R_SetColor(bg_color);

	CG_DrawPic( SCREEN_WIDTH/2.0 - w,
			y,
			w,
			h,
			cgs.media.rsb4_health_bg );
	CG_DrawPic( SCREEN_WIDTH/2.0,
			y,
			w,
			h,
			cgs.media.rsb4_armor_bg );

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		team = cg.snap->ps.persistant[PERS_TEAM];
	} else {
		team = cgs.clientinfo[ cg.snap->ps.clientNum ].team;
	}
	if (team != TEAM_FREE) {
		CG_GetScoreColor(team, border_color);
	}
	border_color[3] = RSB4_DECOR_ALPHA;
	trap_R_SetColor(border_color);

	CG_DrawPic( SCREEN_WIDTH/2.0 - w,
			y,
			w,
			h,
			cgs.media.rsb4_health_bg_border );
	CG_DrawPic( SCREEN_WIDTH/2.0,
			y,
			w,
			h,
			cgs.media.rsb4_armor_bg_border );

	trap_R_SetColor(NULL);
}

#define	RSB3_BIGCHAR_HEIGHT		32
#define	RSB3_BIGICON_HEIGHT		24
#define	RSB3_TEXT_ICON_SPACE		1
#define	RSB3_CENTER_SPACING		10

#define RSB3_WEAPONMARGIN		4
#define	RSB3_WEAPICON_HEIGHT		24
#define	RSB3_WEAPCHAR_HEIGHT		24
#define	RSB3_WEAPBAR_YSPACE		10
#define RSB3_WEAPBAR_DOT_HEIGHT		15


//#define RSB3_BAR_MARGIN			52
//#define RSB3_BAR_DOT_HEIGHT		15
#define RSB3_BAR_DOT_HEIGHT		20
//#define RSB3_BAR_BOTTOM_MARGIN		4
#define RSB3_BAR_MARGIN			4
#define RSB3_BAR_ALPHA			0.7
//#define RSB3_BARBORDER			1
//#define RSB3_BARBORDERALPHA		0.4
//
void CG_Ratstatusbar3RegisterShaders(void) {
	cgs.media.bardot = trap_R_RegisterShaderNoMip("rat_bardot");
	//cgs.media.bardot_outline = trap_R_RegisterShaderNoMip("rat_bardot_outline");
	cgs.media.bardot_additiveglow = trap_R_RegisterShaderNoMip("rat_bardot_additiveglow");
	cgs.media.bardot_transparentglow = trap_R_RegisterShaderNoMip("rat_bardot_transparentglow");
}

static void CG_DrawRatStatusBar3( void ) {
	float x;
	playerState_t	*ps;
	int			value;
	qhandle_t	icon_a;
	qhandle_t	icon_h;
	int team = TEAM_FREE;
	float bigchar_width = CG_HeightToWidth((float)RSB3_BIGCHAR_HEIGHT*2.0/3.0);
	float weaponchar_width = CG_HeightToWidth((float)RSB3_WEAPCHAR_HEIGHT*2.0/3.0);
	int flagteam = TEAM_NUM_TEAMS;
	int weaponSelect = CG_GetWeaponSelect();

	static float colors[4][4] = { 
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}
	
	if (!cgs.media.bardot || !cgs.media.bardot_additiveglow || !cgs.media.bardot_transparentglow) {
		CG_Ratstatusbar3RegisterShaders();
	}

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		team = cg.snap->ps.persistant[PERS_TEAM];
	} else {
		team = cgs.clientinfo[ cg.snap->ps.clientNum ].team;
	}

	ps = &cg.snap->ps;

	if( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
		flagteam = TEAM_RED;
	} else if( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		flagteam = TEAM_BLUE;
	} else if( cg.predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		flagteam = TEAM_FREE;
	}
	if (flagteam != TEAM_NUM_TEAMS) {
		CG_DrawFlagModel( 32, 480 - ICON_SIZE, CG_HeightToWidth(ICON_SIZE), ICON_SIZE, flagteam, qfalse );
	}

	//////
	////// ammo
	//
	if ( weaponSelect && weaponSelect > WP_NONE && weaponSelect < WP_NUM_WEAPONS) {
		qhandle_t weapicon;
		float y;

		if (cg_predictWeapons.integer) {
			value=cg.predictedPlayerState.ammo[weaponSelect];
		} else {
			value=cg.snap->ps.ammo[weaponSelect];
		}

		x = RSB3_BAR_MARGIN;


		CG_DrawDottedBar(x,
				SCREEN_HEIGHT - RSB3_BAR_MARGIN - RSB3_WEAPBAR_DOT_HEIGHT,
				&cg.weaponbar,
				10,
				RSB3_WEAPBAR_DOT_HEIGHT,
				0,
				-RSB3_WEAPBAR_YSPACE,
				weaponSelect,
				value > -1 ? value : 200,
				value > -1 ? CG_FullAmmo(weaponSelect)*2 : 200
				);

		x += CG_HeightToWidth(RSB3_WEAPBAR_DOT_HEIGHT + RSB3_TEXT_ICON_SPACE*3);
		y = SCREEN_HEIGHT - RSB3_WEAPONMARGIN - RSB3_WEAPICON_HEIGHT;
		weapicon = cg_weapons[ weaponSelect ].weaponIcon;
		if ( weapicon ) {
			CG_DrawPic( x, 
				y,
			       	CG_HeightToWidth(RSB3_WEAPICON_HEIGHT),
			       	RSB3_WEAPICON_HEIGHT,
			       	weapicon );
		}
		y -= 0 + RSB3_WEAPCHAR_HEIGHT;
		if (value > -1) {
			int k;
			int numDigits;
			trap_R_SetColor( CG_GetWeaponColor(weaponSelect) );
			k = value;
			numDigits = 1;
			while ((k /= 10) > 0) {
				numDigits++;
			}
			CG_DrawField(x, 
					y,
					numDigits, 
					value,
					qfalse,
					weaponchar_width,
					RSB3_WEAPCHAR_HEIGHT );
		}
	}

	trap_R_SetColor( NULL );
	switch (team) {
		case TEAM_BLUE:
			icon_h = cgs.media.healthIconBlue;
			icon_a = cgs.media.armorIconBlue;
			break;
		case TEAM_RED:
			icon_h = cgs.media.healthIconRed;
			icon_a = cgs.media.armorIconRed;
			break;
		default:
			icon_h = cgs.media.healthIcon;
			icon_a = cgs.media.armorIcon;
			break;
	}


	//
	// health
	//
	value = ps->stats[STAT_HEALTH];

	x = SCREEN_WIDTH/2.0 - CG_HeightToWidth(RSB3_BIGICON_HEIGHT + RSB3_CENTER_SPACING);
	CG_DrawPic( x, SCREEN_HEIGHT - (RSB3_BIGCHAR_HEIGHT + RSB3_BIGICON_HEIGHT)/2.0, CG_HeightToWidth(RSB3_BIGICON_HEIGHT), RSB3_BIGICON_HEIGHT, icon_h );

	if ( value > 100 ) {
		trap_R_SetColor( colors[3] );
	} else if (value > 30) {
		trap_R_SetColor( colors[0] );
	} else {
		trap_R_SetColor( colors[1] );
	}

	x -= CG_HeightToWidth(RSB3_TEXT_ICON_SPACE) + bigchar_width*3;
        CG_DrawField(x, SCREEN_HEIGHT-RSB3_BIGCHAR_HEIGHT, 3, value, qfalse, bigchar_width, RSB3_BIGCHAR_HEIGHT);

	x -= CG_HeightToWidth(RSB3_TEXT_ICON_SPACE + RSB3_BAR_DOT_HEIGHT);

	CG_DrawDottedBar(x,
		       	SCREEN_HEIGHT - RSB3_BAR_MARGIN - RSB3_BAR_DOT_HEIGHT,
			&cg.healthbar,
			7,
		       	RSB3_BAR_DOT_HEIGHT,
		       	-RSB3_BAR_DOT_HEIGHT,
			0,
			WP_NONE,
			value,
		       	175
			);


	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];

	x = SCREEN_WIDTH/2.0 + CG_HeightToWidth(RSB3_CENTER_SPACING);
	CG_DrawPic( x, SCREEN_HEIGHT- (RSB3_BIGCHAR_HEIGHT + RSB3_BIGICON_HEIGHT)/2.0, CG_HeightToWidth(RSB3_BIGICON_HEIGHT), RSB3_BIGICON_HEIGHT, icon_a );
	x += CG_HeightToWidth(RSB3_BIGICON_HEIGHT + RSB3_TEXT_ICON_SPACE);
	if (value > 0 ) {
		if (value > 100) {
			trap_R_SetColor( colors[3] );
		} else  {
			trap_R_SetColor( colors[0] );
		}
		CG_DrawField (x, SCREEN_HEIGHT-RSB3_BIGCHAR_HEIGHT, 3, value, qfalse, bigchar_width, RSB3_BIGCHAR_HEIGHT);
		trap_R_SetColor( NULL );
	}
	x += bigchar_width*3 + CG_HeightToWidth( RSB3_TEXT_ICON_SPACE);

	CG_DrawDottedBar(x,
		       	SCREEN_HEIGHT - RSB3_BAR_MARGIN - RSB3_BAR_DOT_HEIGHT,
			&cg.armorbar,
			7,
		       	RSB3_BAR_DOT_HEIGHT,
		       	RSB3_BAR_DOT_HEIGHT,
			0,
			WP_NONE,
			value,
		       	175
			);

	trap_R_SetColor( NULL );
}


#endif
/*
================
CG_DrawStatusBar

================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBar( void ) {
	int			color;
	centity_t	*cent;
	playerState_t	*ps;
	int			value;
	vec4_t		hcolor;
	vec3_t		angles;
	vec3_t		origin;
        qhandle_t	handle;
        
	static float colors[4][4] = { 
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	if (cg_drawTeamBackground.integer) {
		// draw the team background
		if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) //If not following anybody:
			CG_DrawTeamBackground( 0, 420, 640, 60, 0.33f, cg.snap->ps.persistant[PERS_TEAM] );
		else //Sago: If we follow find the teamcolor of the guy we follow. It might not be our own team!
			CG_DrawTeamBackground( 0, 420, 640, 60, 0.33f, cgs.clientinfo[ cg.snap->ps.clientNum ].team );
	}

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized
	if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin( cg.time / 1000.0 );
		CG_Draw3DModel( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
	}

	CG_DrawStatusBarHead( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE );

	if( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED );
	} else if( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE );
	} else if( cg.predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE );
	}

	if ( ps->stats[ STAT_ARMOR ] ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
		CG_Draw3DModel( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cgs.media.armorModel, 0, origin, angles );
	}
        
        if( cgs.gametype == GT_HARVESTER || cgs.gametype == GT_TREASURE_HUNTER) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
		if( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			handle = cgs.gametype == GT_HARVESTER ? cgs.media.redCubeModel : cgs.media.blueCubeModel;
		} else {
			handle = cgs.gametype == GT_HARVESTER ? cgs.media.blueCubeModel : cgs.media.redCubeModel;
		}
		CG_Draw3DModel( 470 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, handle, 0, origin, angles );
	}
        
        
	//
	// ammo
	//
	if ( cent->currentState.weapon ) {
		value = ps->ammo[cent->currentState.weapon];
		if ( value > -1 ) {
			if ( cg.predictedPlayerState.weaponstate == WEAPON_FIRING
				&& cg.predictedPlayerState.weaponTime > 100 ) {
				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;	// green
				} else {
					color = 1;	// red
				}
			}
			trap_R_SetColor( colors[color] );
			
			CG_DrawField (0, 432, 3, value, qfalse, CHAR_WIDTH, CHAR_HEIGHT);
			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
				qhandle_t	icon;

				icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
				if ( icon ) {
					CG_DrawPic( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, icon );
				}
			}
		}
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if ( value > 100 ) {
		trap_R_SetColor( colors[3] );		// white
	} else if (value > 25) {
		trap_R_SetColor( colors[0] );	// green
	} else if (value > 0) {
		color = (cg.time >> 8) & 1;	// flash
		trap_R_SetColor( colors[color] );
	} else {
		trap_R_SetColor( colors[1] );	// red
	}

	// stretch the health up when taking damage
	CG_DrawField ( 185, 432, 3, value, qfalse, CHAR_WIDTH, CHAR_HEIGHT);
	CG_ColorForHealth( hcolor );
	trap_R_SetColor( hcolor );


	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if (value > 0 ) {
		trap_R_SetColor( colors[0] );
		CG_DrawField (370, 432, 3, value, qfalse, CHAR_WIDTH, CHAR_HEIGHT);
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			CG_DrawPic( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon );
		}

	}
        
        //Skulls!
        if(cgs.gametype == GT_HARVESTER || cgs.gametype == GT_TREASURE_HUNTER)
        {
            value = ps->generic1;
            if (value > 0 ) {
                    trap_R_SetColor( colors[0] );
                    CG_DrawField (470, 432, 3, value, qfalse, CHAR_WIDTH, CHAR_HEIGHT);
                    trap_R_SetColor( NULL );
                    // if we didn't draw a 3D icon, draw a 2D icon for skull
                    if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
                            if( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
                                    handle = cgs.gametype == GT_HARVESTER ? cgs.media.redCubeIcon : cgs.media.blueCubeIcon;
                            } else {
                                    handle = cgs.gametype == GT_HARVESTER ? cgs.media.blueCubeIcon : cgs.media.redCubeIcon;
                            }
                            CG_DrawPic( 470 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, handle );
                    }

            }
        }
}
#endif

#define HUD_DAMAGE_TIME 500
#define HUD_DAMAGE_SIZE (48*2.25)
static void CG_DrawHudDamageIndicator(void) {
	int			t;
	int			maxTime;
	float color[4] = { 1.0, 0.0, 0.0, 0.5 };
	float x,y, w, h;
	//float damageModifier;
	float directionThreshold = 0.4;
	qboolean left, right, top, bottom;
	float size = HUD_DAMAGE_SIZE * cg_hudDamageIndicatorScale.value;
	float offsetFactor = MAX(0.0, MIN(1.0,cg_hudDamageIndicatorOffset.value));


	if (cg_hudDamageIndicator.integer != 1) {
		return;
	}

	if ( !cg.damageValue || !cg.snap ) {
		return;
	}

	maxTime = HUD_DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if ( t <= 0 || t >= maxTime ) {
		return;
	}

	//damageModifier = MIN(1.0, (float)cg.snap->ps.damageCount/100.0);

	//color[3] = (0.2 + 0.3 * damageModifier) * ( 1.0 - ((float)t / maxTime) );
	//color[3] = (0.4 + 0.5 * damageModifier) * ( 1.0 - ((float)t / maxTime) );
	color[3] = MAX(0.0, MIN(1.0, cg_hudDamageIndicatorAlpha.value)) * ( 1.0 - ((float)t / maxTime) );
	trap_R_SetColor(color);

	left = right = top = bottom = qfalse;
	top =    (cg.damageY > directionThreshold);
	bottom = (cg.damageY < -directionThreshold);
	left =   (cg.damageX < -directionThreshold);
	right =  (cg.damageX > directionThreshold);

	if (!(left || top || bottom || right)) {
		left = right = top = bottom = qtrue;
	}

	h = size;
	w = 4.0 * CG_HeightToWidth(h);
	x = (SCREEN_WIDTH - w)/2.0;
	if (bottom) {
		// bottom
		y = SCREEN_HEIGHT - h * (1-offsetFactor);
		CG_DrawPic( x, y, w, h, cgs.media.damageIndicatorBottom );
	}
	if (top) {
		// top
		y = 0 - h * offsetFactor;
		CG_DrawPic( x, y, w, h, cgs.media.damageIndicatorTop );
	}
	h = 4.0 * size;
	w = CG_HeightToWidth(size);
	y = (SCREEN_HEIGHT - h)/2.0;
	if (left) {
		// left
		x = 0 - w * offsetFactor;
		CG_DrawPic( x, y, w, h, cgs.media.damageIndicatorLeft );
	}
	if (right) {
		// right
		x = SCREEN_WIDTH - w * (1-offsetFactor);
		CG_DrawPic( x, y, w, h, cgs.media.damageIndicatorRight );
	}

	trap_R_SetColor(NULL);

}

#define HUD_MOVEMENT_SIZE (24)
static void CG_DrawMovementKeys(void) {
	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
	float hue, sat, val;
	float x,y, w, h;
	qboolean left, right, up, down, jump, crouch;
	float size = HUD_MOVEMENT_SIZE * cg_hudMovementKeysScale.value;
	int movement_keys;


	if (cg_hudMovementKeys.integer != 1) {
		return;
	}

	if ( !cg.snap ) {
		return;
	}

	CG_PlayerColorFromString(cg_hudMovementKeysColor.string, &hue, &sat, &val);
	Q_HSV2RGB(hue,sat,val, color);
	color[3] = cg_hudMovementKeysAlpha.value;
	trap_R_SetColor(color);

	movement_keys = cg.predictedPlayerState.stats[STAT_MOVEMENT_KEYS];

	jump   = movement_keys & MOVEMENT_KEY_JUMP;
	crouch = movement_keys & MOVEMENT_KEY_CROUCH;
	up     = movement_keys & MOVEMENT_KEY_UP;
	down   = movement_keys & MOVEMENT_KEY_DOWN;
	left   = movement_keys & MOVEMENT_KEY_LEFT;
	right  = movement_keys & MOVEMENT_KEY_RIGHT;

	if (!(left || up || down || right || crouch || jump)) {
		return;
	}

	h = size;
	w = CG_HeightToWidth(h);
	x = (SCREEN_WIDTH - w)/2.0;
	y = (SCREEN_HEIGHT - h)/2.0 + h;
	if (down) {
		// down
		CG_DrawPic( x, y, w, h, cgs.media.movementKeyIndicatorDown );
	}
	if (crouch) {
		// crouch
		CG_DrawPic( x, y + h/4.0, w, h, cgs.media.movementKeyIndicatorCrouch );
	}
	y = (SCREEN_HEIGHT - h)/2.0 - h;
	if (up) {
		// up
		CG_DrawPic( x, y, w, h, cgs.media.movementKeyIndicatorUp );
	}
	if (jump) {
		// jump
		// y = 0 - h * offsetFactor;
		CG_DrawPic( x, y - h/4.0, w, h, cgs.media.movementKeyIndicatorJump );
	}
	y = (SCREEN_HEIGHT - h)/2.0;
	if (left) {
		// left
		x = (SCREEN_WIDTH - w)/2.0 - w;
		CG_DrawPic( x, y, w, h, cgs.media.movementKeyIndicatorLeft );
	}
	if (right) {
		// right
		x = (SCREEN_WIDTH - w)/2.0 + w;
		CG_DrawPic( x, y, w, h, cgs.media.movementKeyIndicatorRight );
	}

	trap_R_SetColor(NULL);

}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker( float y ) {
	int			t;
	float		size;
	vec3_t		angles;
	const char	*info;
	const char	*name;
	int			clientNum;
	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
	float char_width = CG_HeightToWidth(cg_attackerScale.value * MEDIUMCHAR_WIDTH);
	float char_height = cg_attackerScale.value * MEDIUMCHAR_HEIGHT;


	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if ( !cg.attackerTime ) {
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) {
		return y;
	}

	t = cg.time - cg.attackerTime;
	if ( t > ATTACKER_HEAD_TIME ) {
		cg.attackerTime = 0;
		return y;
	}

	//size = ICON_SIZE * 1.25;
	size = ICON_SIZE * 0.75;

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;
	CG_DrawHead( SCREEN_WIDTH - CG_HeightToWidth(size), y, CG_HeightToWidth(size), size, clientNum, angles );

	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = Info_ValueForKey(  info, "n" );
	y += size;
	//CG_DrawBigString( 640 - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), y, name, 0.5 );
	color[3] = 0.5;
	CG_DrawStringExtFloat( SCREEN_WIDTH - CG_DrawStrlen( name ) * char_width - CG_HeightToWidth(2),
		       	y,
		       	name,
		       	color, qfalse, qtrue,
			char_width,
			char_height,
		       	0 );

	return y + char_height + 2;
}

/*
================
CG_DrawSpeedMeter

================
*/
static float CG_DrawSpeedMeter( float y ) {
	char        *s;
	int         w;
	vec_t       *vel;
	int         speed;
	float	color[4];
	int char_width = CG_HeightToWidth(cg_speedScale.value * BIGCHAR_WIDTH);
	int char_height = cg_speedScale.value * BIGCHAR_WIDTH;

	/* speed meter can get in the way of the scoreboard */
	if ( cg.scoreBoardShowing ) {
		return y;
	}

	vel = cg.snap->ps.velocity;
	if (cg_drawSpeed3D.integer) { 
		speed = VectorLength(vel);
	} else {
		/* ignore vertical component of velocity */
		speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
	}

	s = va( "%iu/s", speed );

	w = CG_DrawStrlen( s ) * char_width;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = cg_speedAlpha.value;

	if (cg_drawSpeed.integer == 1) {
		/* top left-hand corner of screen */
		//CG_DrawBigString( 635 - w, y + 2, s, 1.0F);
		CG_DrawStringExt( 635 -w, y + 2, s, color, qfalse, qtrue, char_width, char_height, 0 );
		return y + char_height + 4;
	} else {
		/* center of screen */
		//CG_DrawBigString( 320 - w / 2, 300, s, 1.0F);
		CG_DrawStringExt( 320 - w / 2, 300, s, color, qfalse, qtrue, char_width, char_height, 0 );
		return y;
	}
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

#define	FPS_FRAMES	4
static int CG_FPS( void ) {
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	static	int	previous;
	int		t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		return 1000 * FPS_FRAMES / total;
	}

	return -1;
}

/*
==================
CG_DrawFPS
==================
*/
static float CG_DrawFPS( float y ) {
	char		*s;
	int		w;
	int		fps;
	float	color[4];
	int	char_height = BIGCHAR_HEIGHT * cg_fpsScale.value;
	int	char_width = CG_HeightToWidth(BIGCHAR_WIDTH * cg_fpsScale.value);

	fps = CG_FPS();
	if ( fps != -1 ) {
		s = va( "%ifps", fps );

		//CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

		color[0] = color[1] = color[2] = 1.0;
		color[3] = cg_fpsAlpha.value;
		if (cg_drawFPS.integer == 1) {
			w = CG_DrawStrlen( s ) * char_width;
			CG_DrawStringExt( 635-w, y + 2, s, color, qfalse, qtrue, char_width, char_height, 0 );
		} else if (cg_drawFPS.integer == 2 || cg_drawFPS.integer == 3) {
			char_width *= 0.75;
			char_height *= 0.75;
			w = CG_DrawStrlen( s ) * char_width;
			CG_DrawStringExt( SCREEN_WIDTH - (48 + w)/2.0, SCREEN_HEIGHT - 48*1.0/3.0 - char_height/2.0,
				       	s, color, qfalse, qtrue, char_width, char_height, 0 );
		}
	}

	return y + char_height + 4;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;
	float	color[4];
	int char_width = CG_HeightToWidth(BIGCHAR_WIDTH * cg_timerScale.value);
	int char_height = BIGCHAR_HEIGHT * cg_timerScale.value;
	qboolean negative = qfalse;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = cg_timerAlpha.value;

	msec = cg.time - cgs.levelStartTime - cgs.timeoutOvertime;

	if (msec < 0) {
		msec = -msec;
		negative = qtrue;
	}

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%s%i:%i%i", negative ? "-" : "", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * char_width;
	//CG_DrawBigString( 635 - w, y + 2, s, 1.0F);
	switch (cg_timerPosition.integer) {
		case 1:
			CG_DrawStringExt( (SCREEN_WIDTH-w)/2.0, 10, s, color, qfalse, qtrue, char_width, char_height, 0 );
			return y;
			break;
	        case 2:
			CG_DrawStringExt( (SCREEN_WIDTH-w)/2.0,
					  SCREEN_HEIGHT/2 + cg_crosshairSize.integer,
					  s, color, qfalse, qtrue, char_width, char_height, 0 );
			return y;
			break;
	        case 3:
			CG_DrawStringExt( (SCREEN_WIDTH-w)/2.0,
					  SCREEN_HEIGHT / 2 - cg_crosshairSize.integer - char_height,
					  s, color, qfalse, qtrue, char_width, char_height, 0 );
			return y;
			break;
		default:
			CG_DrawStringExt( 635-w, y+2, s, color, qfalse, qtrue, char_width, char_height, 0 );
			return y + char_height + 4;
			break;
	}

}

/*
=================
CG_DrawTimeout
=================
*/
static float CG_DrawTimeout( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;
	float	color[4];
	int char_width = CG_HeightToWidth(BIGCHAR_WIDTH * cg_timerScale.value);
	int char_height = BIGCHAR_HEIGHT * cg_timerScale.value;

	if (cg.time >= cgs.timeoutEnd) {
		return y;
	}

	color[0] = color[1] = 0.0;
       	color[2] = 1.0;
	color[3] = cg_timerAlpha.value;

	msec = cgs.timeoutEnd - cg.time;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "Timeout ends: %i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * char_width;

	//CG_DrawBigString( 635 - w, y + 2, s, 1.0F);
	CG_DrawStringExt( 635-w, y+2, s, color, qfalse, qtrue, char_width, char_height, 0 );

	return y + char_height + 4;
}

/*
CG_DrawDoubleDominationThings
 *
 *Sago: Might be relevant for debugging missionpack.
*/

/*static float CG_DrawDoubleDominationThings( float y ) {
	char		*s;
	int			w;
	int 		statusA, statusB;
	statusA = cgs.redflag;
	statusB = cgs.blueflag;

	if(statusA == TEAM_NONE) {
		s = va("Point A not spawned");
	} else
	if(statusA == TEAM_FREE) {
		s = va("Point A is not controlled");
	} else
	if(statusA == TEAM_RED) {
		s = va("Point A is controlled by RED");
	} else
	if(statusA == TEAM_BLUE) {
		s = va("Point A is controlled by BLUE");
	} else
		s = va("Point A has an error");
	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallString( 635 - w, y + 2, s, 1.0F);
	y+=SMALLCHAR_HEIGHT+4;

	if(statusB == TEAM_NONE) {
		s = va("Point B not spawned");
	} else
	if(statusB == TEAM_FREE) {
		s = va("Point B is not controlled");
	} else
	if(statusB == TEAM_RED) {
		s = va("Point B is controlled by RED");
	} else
	if(statusB == TEAM_BLUE) {
		s = va("Point B is controlled by BLUE");
	} else
		s = va("Point B has an error");
	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallString( 635 - w, y + 2, s, 1.0F);

	if( ( ( statusB == statusA ) && ( statusA == TEAM_RED ) ) ||
		( ( statusB == statusA ) && ( statusA == TEAM_BLUE ) ) ) {
		s = va("Capture in: %i",(cgs.timetaken+10*1000-cg.time)/1000+1);
		w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
		y+=SMALLCHAR_HEIGHT+4;
		CG_DrawSmallString( 635 - w, y + 2, s, 1.0F);
	}

	return y + SMALLCHAR_HEIGHT+4;
}*/

/*
=================
CG_DrawLMSmode
=================
*/

static float CG_DrawLMSmode( float y ) {
	char		*s;
	int		w;

	if(cgs.lms_mode == 0) {
		s = va("LMS: Point/round + OT");
	} else
	if(cgs.lms_mode == 1) {
		s = va("LMS: Point/round - OT");
	} else
	if(cgs.lms_mode == 2) {
		s = va("LMS: Point/kill + OT");
	} else
	if(cgs.lms_mode == 3) {
		s = va("LMS: Point/kill - OT");
	} else
		s = va("LMS: Unknown mode");

	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallString( 635 - w, y + 2, s, 1.0F);

	return y + SMALLCHAR_HEIGHT+4;
}

/*
=================
CG_DrawCTFoneway
=================
*/

static float CG_DrawCTFoneway( float y ) {
	char		*s;
	int		w;
	vec4_t		color;

	if(cgs.gametype != GT_CTF_ELIMINATION)
		return y;

	memcpy(color,g_color_table[ColorIndex(COLOR_WHITE)],sizeof(color));

	if( (cgs.elimflags&EF_ONEWAY)==0) {
		return y; //nothing to draw
	} else
	if(cgs.attackingTeam == TEAM_BLUE) {
		memcpy(color,g_color_table[ColorIndex(COLOR_BLUE)],sizeof(color));
		s = va("Blue team on offence");
	} else
	if(cgs.attackingTeam == TEAM_RED) {
		memcpy(color,g_color_table[ColorIndex(COLOR_RED)],sizeof(color));
		s = va("Red team on offence");
	} else
		s = va("Unknown team on offence");

	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallStringColor( 635 - w, y + 2, s, color);

	return y + SMALLCHAR_HEIGHT+4;
}

/*
=================
CG_DrawEliminationDeathMessage
=================
*/

/*static float CG_DrawEliminationDeathMessage( float y ) {
	char		*s;
	int			w;

	s = va("You are waiting for a new round");
	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallString( 635 - w, y + 2, s, 1.0F);

	return y + SMALLCHAR_HEIGHT+4;
}*/

/*
=================
CG_DrawDomStatus
=================
*/

static float CG_DrawDomStatus( float y ) {
	int 		i,w; 
	char		*s;
	vec4_t		color;
	
	for(i = 0;i < cgs.domination_points_count;i++) {
		switch(cgs.domination_points_status[i]) {
			case TEAM_RED:
				memcpy(color,g_color_table[ColorIndex(COLOR_RED)],sizeof(color));
				break;
			case TEAM_BLUE:
				memcpy(color,g_color_table[ColorIndex(COLOR_BLUE)],sizeof(color));
				break;
			default:
				memcpy(color,g_color_table[ColorIndex(COLOR_WHITE)],sizeof(color));
				break;			
		}
		
		s = va("%s",cgs.domination_points_names[i]);
		w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
		CG_DrawSmallStringColor( 635 - w, y + 2, s, color);
		y += SMALLCHAR_HEIGHT+4;
			
	}

	return y;
}


/*
=================
CG_DrawEliminationTimer
=================
*/
static float CG_DrawEliminationTimer( float y ) {
	char		*s;
	float			w;
	int			mins, seconds, tens, sec;
	int			msec;
	vec4_t			color;
	const char	*st;
	//float scale;
	int cw;
	int rst;

        

	rst = cgs.roundStartTime;

        if(cg.time>rst && !cgs.roundtime) {
            return y;
        }

	//default color is white
	memcpy(color,g_color_table[ColorIndex(COLOR_WHITE)],sizeof(color));

	//msec = cg.time - cgs.levelStartTime;
	if(cg.time>rst) //We are started
	{
		msec = cgs.roundtime*1000 - (cg.time -rst);
		if(msec<=30*1000-1) //<= 30 seconds
			memcpy(color,g_color_table[ColorIndex(COLOR_YELLOW)],sizeof(color));
		if(msec<=10*1000-1) //<= 10 seconds
			memcpy(color,g_color_table[ColorIndex(COLOR_RED)],sizeof(color));
		msec += 1000; //120-1 instead of 119-0
	}
	else
	{
		//Warmup
		msec = -cg.time +rst;
		memcpy(color,g_color_table[ColorIndex(COLOR_GREEN)],sizeof(color));
		sec = msec/1000;
		msec += 1000; //5-1 instead of 4-0
/***
Lots of stuff
****/
	if(cg.warmup == 0)
	{
		st = va( "Round in: %i", sec + 1 );
		if ( sec != cg.warmupCount ) {
			cg.warmupCount = sec;
			switch ( sec ) {
			case 0:
				trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
				break;
			case 1:
				trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
				break;
			case 2:
				trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
				break;
			default:
				break;
			}
		} 
		//scale = 0.45f;
		switch ( cg.warmupCount ) {
		case 0:
			cw = 28;
			//scale = 0.54f;
			break;
		case 1:
			cw = 24;
			//scale = 0.51f;
			break;
		case 2:
			cw = 20;
			//scale = 0.48f;
			break;
		default:
			cw = 16;
			//scale = 0.45f;
			break;
		}

	#ifdef MISSIONPACK
			//w = CG_Text_Width(s, scale, 0);
			//CG_Text_Paint(320 - w / 2, 125, scale, colorWhite, st, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
	#else
                
                    w = CG_DrawStrlen( st );
                    CG_DrawStringExt( 320 - CG_HeightToWidth(w * (float)cw/2.0), 70, st, colorWhite,
				qfalse, qtrue, (int)CG_HeightToWidth(cw), (int)(cw * 1.5), 0 );
	#endif
	}
/*
Lots of stuff
*/
	}

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	if(msec>=0)
		s = va( " %i:%i%i", mins, tens, seconds );
	else
		s = va( " Overtime");
	w = CG_DrawStrlen( s ) * CG_HeightToWidth(BIGCHAR_WIDTH);
	
	CG_DrawStringExt( 635 -w , y + 2, s, color, qtrue, qtrue, CG_HeightToWidth(BIGCHAR_WIDTH), BIGCHAR_HEIGHT, 0 );

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTreasureHuntTimer
=================
*/
static float CG_DrawTreasureHuntTimer( float y ) {
	char		*s, *phase;
	float			w;
	int			mins, seconds, sec;
	int			msec = -1;
	vec4_t			color;
	const char	*st;
	int cw;
	int rst;


	rst = cgs.th_roundStart;

	//default color is white
	memcpy(color,g_color_table[ColorIndex(COLOR_WHITE)],sizeof(color));

	if (rst == 0) {
		return y;
	}

	switch (cgs.th_phase) {
		case TH_INIT:
		case TH_HIDE:
			phase = "Hiding";
			break;
		case TH_INTER:
		case TH_SEEK:
			phase = "Seeking";
			break;
		default:
			return y;
			break;
	}

	//msec = cg.time - cgs.levelStartTime;
	if(cg.time > rst) { //We are started
		if (cgs.th_roundDuration > 0) {
			msec = cgs.th_roundDuration*1000 - (cg.time - rst);
			if(msec<=30*1000-1) //<= 30 seconds
				memcpy(color,g_color_table[ColorIndex(COLOR_YELLOW)],sizeof(color));
			if(msec<=10*1000-1) //<= 10 seconds
				memcpy(color,g_color_table[ColorIndex(COLOR_RED)],sizeof(color));
			msec += 1000; //120-1 instead of 119-0
		}
	}
	else
	{
		//Warmup
		msec = - cg.time + rst;
		memcpy(color,g_color_table[ColorIndex(COLOR_GREEN)],sizeof(color));
		sec = msec/1000;
		msec += 1000; //5-1 instead of 4-0

		if(cg.warmup == 0)
		{
			st = va( "%s in: %i", phase,sec + 1 );
			if ( sec != cg.warmupCount ) {
				cg.warmupCount = sec;
				switch ( sec ) {
					case 0:
						trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
						break;
					case 1:
						trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
						break;
					case 2:
						trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
						break;
					default:
						break;
				}
			} 
			switch ( cg.warmupCount ) {
				case 0:
					cw = 28;
					break;
				case 1:
					cw = 24;
					break;
				case 2:
					cw = 20;
					break;
				default:
					cw = 16;
					break;
			}


			w = CG_DrawStrlen( st );
			CG_DrawStringExt( 320 - CG_HeightToWidth(w * (float)cw/2.0), 70, st, colorWhite,
					qfalse, qtrue, (int)CG_HeightToWidth(cw), (int)(cw * 1.5), 0 );
		}
	}

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;


	if(msec>=0)
		s = va( "%.1s %i:%02i", phase, mins, seconds );
	else
		s = va( "%s", phase);
	w = CG_DrawStrlen( s ) * CG_HeightToWidth(BIGCHAR_WIDTH);
	
	CG_DrawStringExt( 635 -w , y + 2, s, color, qtrue, qtrue, CG_HeightToWidth(BIGCHAR_WIDTH), BIGCHAR_HEIGHT, 0 );

	return y + BIGCHAR_HEIGHT + 4;
}


static void CG_DrawHealthBar(float x, float y, float width, float height, int health, int armor) {
	int hp = CG_GetTotalHitPoints(health, armor);
	float r = hp/400.0;
	float color[4];

	
	if (r > 1.0) {
		r = 1.0;
	}

	// CROSSREF CG_GetPlayerSpriteShader()
	if (hp > 300) {
		color[0] = 0.0;
		color[1] = 0.31;
		color[2] = 1.0;
	} else if (hp > 200) {
		color[0] = 0.0;
		color[1] = 1.0;
		color[2] = 1.0;
	} else if (hp > 150) {
		color[0] = 0.0;
		color[1] = 1.0;
		color[2] = 0.0;
	} else if (hp > 100) {
		color[0] = 1.0;
		color[1] = 1.0;
		color[2] = 0.0;
	} else if (hp > 80) {
		color[0] = 1.0;
		color[1] = 0.47;
		color[2] = 0.0;
	} else {
		color[0] = 1.0;
		color[1] = 0.0;
		color[2] = 0.0;
	}

	color[3] = 0.6;
	CG_FillRect(x, y, r*width, height, color);
	color[0] = 0.5;
	color[1] = 0.5;
	color[2] = 0.5;
	color[3] = 0.6;
	CG_DrawRect(x, y, width, height, 1, color);

}

static vec_t *CG_GetColorFromLocation(const char *loc) {
	qboolean red = Q_stristr(loc, "red") != NULL;
	qboolean blue = Q_stristr(loc, "blue") != NULL;

	if (red && !blue) {
		return g_color_table[ColorIndex(COLOR_RED)];
	} else if (blue && !red) {
		return g_color_table[ColorIndex(COLOR_BLUE)];
	} else if (Q_stristr(loc, "mega")) {
		return  g_color_table[ColorIndex(COLOR_CYAN)];
	}
	return g_color_table[ColorIndex(COLOR_WHITE)];
}

/*
=================
CG_DrawTeamOverlay
=================
*/

static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	float x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t	*item;
	int ret_y, count;
	float char_width = 8 * cg_teamOverlayScale.value;
	float char_height = 12 * cg_teamOverlayScale.value;
	int team = TEAM_SPECTATOR;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		if (!cgs.clientinfo[ cg.snap->ps.clientNum ].infoValid) {
			return y;
		}
		team = cgs.clientinfo[ cg.snap->ps.clientNum ].team;
	} else {
		team = cg.snap->ps.persistant[PERS_TEAM];
	}

	if (team != TEAM_BLUE && team != TEAM_RED) { 
		return y; // Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = CG_HeightToWidth((pwidth + lwidth + 5 + 7) * char_width);

	if ( right )
		x = 640 - w;
	else
		x = 0;

	h = plyrs * char_height;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	// don't draw blue background
	//trap_R_SetColor( hcolor );
	//CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	//trap_R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			if (ci->powerups & (( 1<< PW_REDFLAG ) | ( 1<<PW_BLUEFLAG))) {
				hcolor[0] = 1.0f;
				hcolor[1] = 1.0f;
				hcolor[2] = 1.0f;
				hcolor[3] = 0.25;
				trap_R_SetColor( hcolor );
				CG_FillRect( x, y, w, char_height, hcolor);
				trap_R_SetColor( NULL );
			}

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + CG_HeightToWidth(char_width);

			CG_DrawStringExtFloat( xx, y,
				ci->name, hcolor, qfalse, qfalse,
				CG_HeightToWidth(char_width), char_height, pwidth);

			xx = x + CG_HeightToWidth(char_width * (pwidth + 2));

			CG_DrawHealthBar(xx, y,
					       	CG_HeightToWidth(char_width * 9),
					       	char_height,
					      	ci->health, ci->armor);

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );

			if (cgs.gametype != GT_ELIMINATION || ci->health > 0) {
				Com_sprintf (st, sizeof(st), "%3i %i", ci->health,	ci->armor);
			} else if (ci->respawnTime > 0 ) {
				hcolor[0] = hcolor[1] = 1.0;
				hcolor[2] = 0.0;
				hcolor[3] = 1.0;
				Com_sprintf (st, sizeof(st), "re: %i", MAX(0,(int)ceil(((float)(ci->respawnTime - cg.time)/1000.0))));
			} else {
				st[0] = '\0';
			}

			xx = x + CG_HeightToWidth(char_width * (pwidth + 2 + 1));

			CG_DrawStringExtFloat( xx, y,
				st, hcolor, qfalse, qfalse,
				CG_HeightToWidth(char_width), char_height, 0 );

			// draw weapon icon
			xx += CG_HeightToWidth(char_width * 3);

			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, CG_HeightToWidth(char_width), char_height, 
					cg_weapons[ci->curWeapon].weaponIcon );
			} else if (cgs.gametype != GT_ELIMINATION) {
				CG_DrawPic( xx, y, CG_HeightToWidth(char_width), char_height, 
					cgs.media.deferShader );
			}


			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			if (lwidth) {
				vec_t *autocolor = hcolor;
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p) {
					p = "unknown";
				} else if (cg_teamOverlayAutoColor.integer) {
					autocolor = CG_GetColorFromLocation(p);
				}
				//len = CG_DrawStrlen(p);
				//if (len > lwidth)
				//	len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth + 
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + CG_HeightToWidth(char_width * (3 + pwidth + 9));
				CG_DrawStringExtFloat( xx, y,
					p, autocolor, qfalse, qfalse, CG_HeightToWidth(char_width), char_height,
					lwidth);
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - CG_HeightToWidth(char_width);
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( xx, y, CG_HeightToWidth(char_width), char_height, 
						trap_R_RegisterShader( item->icon ) );
						if (right) {
							xx -= CG_HeightToWidth(char_width);
						} else {
							xx += CG_HeightToWidth(char_width);
						}
					}
				}
			}
			// Draw pinglocation marker
			if (ci->lastPinglocationTime > 0 && cg.time - ci->lastPinglocationTime < cg_pingLocationTime.integer) {
				CG_DrawPic( xx, y, CG_HeightToWidth(char_width), char_height, cgs.media.pingLocation);
				//if (right) {
				//	xx -= CG_HeightToWidth(char_width);
				//} else {
				//	xx += CG_HeightToWidth(char_width);
				//}
			}

			y += char_height;
		}
	}

	return ret_y;
//#endif
}

//static float CG_DrawFollowMessage( float y ) {
//	char		*s;
//	int			w;	
//
//	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) || ( ( cgs.elimflags & EF_NO_FREESPEC ) && (cgs.gametype == GT_ELIMINATION || cgs.gametype == GT_CTF_ELIMINATION ) ) ) {
//		return y;
//	}
//
//	s = va("USE_ITEM to stop following");
//	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
//	CG_DrawSmallString( 635 - w, y + 2, s, 1.0F);
//
//	return y + SMALLCHAR_HEIGHT+4;
//}

#define SCORE_BACKGROUND_ALPHA 0.33f
void CG_GetScoreColor(int team, float *color) {
	color[3] = SCORE_BACKGROUND_ALPHA;
	switch (team) {
		case TEAM_RED:
			color[0] = 1.0;
			color[1] = 0.0;
			color[2] = 0.0;
			break;
		case TEAM_BLUE:
			//color[0] = 0.0;
			//color[1] = 0.8;
			//color[2] = 1.0;

			//color[0] = 0.0;
			//color[1] = 0.0;
			//color[2] = 1.0;

			color[0] = 0.0628;
			color[1] = 0.326;
			color[2] = 1.0;
			break;
		default:
			color[0] = 0.5;
			color[1] = 0.5;
			color[2] = 0.5;
			break;
	}
}

#define SCOREBOX_HEIGHT (SCOREBOX_CHAR_HEIGHT+8)
#define SCOREBOX_CHAR_HEIGHT (BIGCHAR_HEIGHT)
#define SCOREBOX_CHAR_WIDTH (BIGCHAR_WIDTH)
#define SCOREBOX_CHAR_MIN_WIDTH (SCOREBOX_CHAR_WIDTH * 2 + 8)
#define SCOREBOX_TEAMINDICATOR_SIZE 10
float CG_DrawScoreBox(float x, float y, int team, const char *s, qboolean selected) {
	float color[4];
	float thickness = 1;
	float w;
	float h = SCOREBOX_HEIGHT;
	float char_width = CG_HeightToWidth(SCOREBOX_CHAR_WIDTH);
	float char_height = SCOREBOX_CHAR_HEIGHT;
	int len = CG_DrawStrlen(s);

	w = CG_HeightToWidth(SCOREBOX_CHAR_WIDTH * MAX(2,len) + 8);
	x -= w;

	CG_GetScoreColor(team, color);
	CG_FillRect(x, y, w, h, color);
	
	color[3] = 1.0;
	if (selected) {
		color[0] = 1.0;
		color[1] = 0.9883;
		color[2] = 0.193;
		CG_DrawCorners(x, y, w, h, h/5.0, thickness, color);
	}

	color[0] = color[1] = color[2] = color[3] = 1.0;
	CG_DrawStringExtFloat(
		       	x + (w - len * char_width)/2.0,
		       	y + (h - char_height)/2.0,
		       	s, color, qfalse, qfalse,
			char_width, char_height,
		       	0 );
	//if (selected) {
	//	color[0] = 1.0;
	//	color[1] = 0.9883;
	//	color[2] = 0.193;
	//	color[3] = 1.0;
	//	trap_R_SetColor(color);
	//	CG_DrawPic( x + w - CG_HeightToWidth(SCOREBOX_TEAMINDICATOR_SIZE),
	//		       	y + (h - SCOREBOX_TEAMINDICATOR_SIZE)/2.0,
	//		       	CG_HeightToWidth(SCOREBOX_TEAMINDICATOR_SIZE),
	//		       	SCOREBOX_TEAMINDICATOR_SIZE,
	//		       	cgs.media.teamIndicator );
	//	trap_R_SetColor(NULL);
	//}

	return w;
}

void CG_DrawEliminationStatus(void) {
	const char	*s;
	int y = 240;
	float x = 640;
	float xx;
	float	w;
	float color[4] = { 1.0, 1.0, 1.0, 1.0 };

	if (cgs.gametype != GT_ELIMINATION && cgs.gametype != GT_CTF_ELIMINATION) {
		return;
	}
	
	if (cg.elimLastPlayerTime && cg.elimLastPlayerTime + 3000 > cg.time) {
		int w;
		float *c;
		s = "You are the chosen one!";
		c = CG_FadeColor( cg.elimLastPlayerTime, 3000);
		w = CG_HeightToWidth(BIGCHAR_WIDTH * CG_DrawStrlen(s));
		xx = ( SCREEN_WIDTH - w) / 2.0;
		CG_DrawStringExt( xx, 70 , s, c,
				qfalse, qtrue, CG_HeightToWidth(BIGCHAR_WIDTH), BIGCHAR_WIDTH * 1.5, 0 );
	}

	xx = x;
	s = va( "%i", cgs.blueLivingCount );
	w = CG_DrawScoreBox(xx, y, TEAM_BLUE, s, cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE);
	xx -= w;

	s = va( "%i", cgs.redLivingCount );
	w = CG_DrawScoreBox(xx, y, TEAM_RED, s, cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED);
	xx -= w;

	if (cg_drawTeamOverlay.integer) {
		int i;
		int numPlayers = 0;
		int totalhp = 0;
		int maxTeamHp;
		y -= SCOREBOX_CHAR_HEIGHT+4;
		xx = x;
		for (i = 0; i < MAX_CLIENTS; ++i) {
			clientInfo_t *ci = &cgs.clientinfo[i];
			if (!ci->infoValid) {
				continue;
			}
			if (ci->team != cg.snap->ps.persistant[PERS_TEAM]) {
				continue;
			}
			totalhp += CG_GetTotalHitPoints(ci->health, ci->armor);
			numPlayers++;
		}
		maxTeamHp = numPlayers * CG_GetTotalHitPoints(cgs.elimStartHealth, cgs.elimStartArmor);
		if (maxTeamHp > 0 && maxTeamHp >= totalhp) {
			s = va( "%i%%", (totalhp*100)/maxTeamHp );
			w = CG_HeightToWidth(CG_DrawStrlen( s ) * SCOREBOX_CHAR_WIDTH + 8);
			xx -= w;
			color[0] = color[1] = color[2] = color[3] = 1.0;
			CG_DrawStringExtFloat(
					xx + CG_HeightToWidth(4),
					y,
					s, color, qfalse, qfalse,
					CG_HeightToWidth(SCOREBOX_CHAR_WIDTH), SCOREBOX_CHAR_HEIGHT,
					0 );
		}
	}
	if (cgs.elimNextRespawnTime > 0 ) {
		int nextRespawnTime = MAX(0,ceil(((float)cgs.elimNextRespawnTime-cg.time)/1000.0));
		if (nextRespawnTime > 0) {
			y -= SCOREBOX_CHAR_HEIGHT+4;
			xx = x;
			s = va( "%i", nextRespawnTime);
			w = CG_HeightToWidth(CG_DrawStrlen( s ) * SCOREBOX_CHAR_WIDTH + 8);
			xx -= w;
			color[0] = color[1] = color[3] = 1.0;
			color[2] = 0;
			CG_DrawStringExtFloat(
					xx + CG_HeightToWidth(4),
					y,
					s, color, qfalse, qfalse,
					CG_HeightToWidth(SCOREBOX_CHAR_WIDTH), SCOREBOX_CHAR_HEIGHT,
					0 );
		}
	}

}

void CG_DrawTreasureHunterStatus(void) {
	const char	*s;
	int y;
	float		x, w;

	if (cgs.gametype != GT_TREASURE_HUNTER) {
		return;
	}

	y = 240;
	x = 640;

	s = va( "%i", cgs.th_blueTokens );
	w = CG_DrawScoreBox(x, y, TEAM_BLUE, s, cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE);
	x -= w;


	s = va( "%i", cgs.th_redTokens );
	w = CG_DrawScoreBox(x, y, TEAM_RED, s, cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED);
	x -= w;
}


/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight(stereoFrame_t stereoFrame)
{
	float	y;

	y = 0;

	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1 && cg_drawTeamOverlay.integer == 1 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	}
	/*if ( cgs.gametype == GT_DOUBLE_D ) {
		y = CG_DrawDoubleDominationThings(y);
	} 
	else*/
	if ( cgs.gametype == GT_LMS && cg.showScores ) {
		y = CG_DrawLMSmode(y);
	}
	else
	if ( cgs.gametype == GT_CTF_ELIMINATION ) {
		y = CG_DrawCTFoneway(y);
	}
	else
	if ( cgs.gametype == GT_DOMINATION ) {
		y = CG_DrawDomStatus(y);
	}
	
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if (cg_drawFPS.integer == 1 && (stereoFrame == STEREO_CENTER || stereoFrame == STEREO_RIGHT)) {
		y = CG_DrawFPS( y );
	}
	if (cgs.gametype==GT_ELIMINATION || cgs.gametype == GT_CTF_ELIMINATION || cgs.gametype==GT_LMS) {
		y = CG_DrawEliminationTimer( y );
		/*if (cgs.clientinfo[ cg.clientNum ].isDead)
			y = CG_DrawEliminationDeathMessage( y);*/
	}

	if (cgs.gametype==GT_TREASURE_HUNTER) {
		y = CG_DrawTreasureHuntTimer( y );
	}

	//y = CG_DrawFollowMessage( y );
	
	if ( cg_drawSpeed.integer ) {
		y = CG_DrawSpeedMeter( y );
	}

	if ( cg_drawTimer.integer) {
		y = CG_DrawTimer( y );
	}
	if ( cg_drawPickup.integer == 1) {
		y = CG_DrawPickupItem( y );
	}
	y = CG_DrawTimeout( y );
	if ( cg_drawAttacker.integer ) {
		y = CG_DrawAttacker( y );
	}

}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

//static float CG_DrawScores( float y ) {
//	const char	*s;
//	int			s1, s2, score;
//	float			x, w;
//	int			v;
//	float		y1;
//	gitem_t		*item;
//        int statusA,statusB;
//        
//        statusA = cgs.redflag;
//        statusB = cgs.blueflag;
//
//	s1 = cgs.scores1;
//	s2 = cgs.scores2;
//
//	y -=  BIGCHAR_HEIGHT + 8;
//
//	y1 = y;
//
//	// draw from the right side to left
//	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1) {
//		x = 640;
//		s = va( "%2i", s2 );
//		w = CG_HeightToWidth(CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8);
//		x -= w;
//		CG_DrawScoreBox(x, y-4, w, BIGCHAR_HEIGHT+8, TEAM_BLUE);
//		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
//			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
//		}
//		CG_DrawBigStringAspect( x + CG_HeightToWidth(4), y, s, 1.0F);
//
//		if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION) {
//			// Display flag status
//			item = BG_FindItemForPowerup( PW_BLUEFLAG );
//
//			if (item) {
//				y1 = y - BIGCHAR_HEIGHT - 8;
//				if( cgs.blueflag >= 0 && cgs.blueflag <= 2 ) {
//					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.blueFlagShader[cgs.blueflag] );
//					// TODO: use this for fixed aspect ratio, but needs a new icon:
//					//CG_DrawPic( x + (w - CG_HeightToWidth(BIGCHAR_HEIGHT+8))/2.0,
//					//	       	y1-4, CG_HeightToWidth(BIGCHAR_HEIGHT+8), BIGCHAR_HEIGHT+8, cgs.media.blueFlagShader[cgs.blueflag] );
//				}
//			}
//		}
//                
//                if ( cgs.gametype == GT_DOUBLE_D ) {
//			// Display Domination point status
//			
//				y1 = y - 32;//BIGCHAR_HEIGHT - 8;
//				if( cgs.redflag >= 0 && cgs.redflag <= 3 ) {
//					CG_DrawPic( x, y1-4, w, 32, cgs.media.ddPointSkinB[cgs.blueflag] );
//				}
//		}
//                
//		s = va( "%2i", s1 );
//		w = CG_HeightToWidth(CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8);
//		x -= w;
//		CG_DrawScoreBox(x, y-4, w, BIGCHAR_HEIGHT+8, TEAM_RED);
//		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
//			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
//		}
//		CG_DrawBigStringAspect( x + CG_HeightToWidth(4), y, s, 1.0F);
//
//		if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION ) {
//			// Display flag status
//			item = BG_FindItemForPowerup( PW_REDFLAG );
//
//			if (item) {
//				y1 = y - BIGCHAR_HEIGHT - 8;
//				if( cgs.redflag >= 0 && cgs.redflag <= 2 ) {
//					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.redFlagShader[cgs.redflag] );
//					// TODO: use this for fixed aspect ratio, but needs a new icon:
//					//CG_DrawPic( x + (w - CG_HeightToWidth(BIGCHAR_HEIGHT+8))/2.0,
//					//	       	y1-4, CG_HeightToWidth(BIGCHAR_HEIGHT+8), BIGCHAR_HEIGHT+8, cgs.media.redFlagShader[cgs.redflag] );
//				}
//			}
//		}
//                
//                if ( cgs.gametype == GT_DOUBLE_D ) {
//			// Display Domination point status
//			
//				y1 = y - 32;//BIGCHAR_HEIGHT - 8;
//				if( cgs.redflag >= 0 && cgs.redflag <= 3 ) {
//					CG_DrawPic( x, y1-4, w, 32, cgs.media.ddPointSkinA[cgs.redflag] );
//				}
//                                
//                        
//                                
//                        //Time till capture:
//                        if( ( ( statusB == statusA ) && ( statusA == TEAM_RED ) ) ||
//                            ( ( statusB == statusA ) && ( statusA == TEAM_BLUE ) ) ) {
//                                s = va("%i",(cgs.timetaken+10*1000-cg.time)/1000+1);
//                                w = CG_HeightToWidth(CG_DrawStrlen( s ) * BIGCHAR_WIDTH);
//                                CG_DrawBigStringAspect( x + CG_HeightToWidth(32+8-w/2.0), y-28, s, 1.0F);
//                        }
//		}
//                
//                if ( cgs.gametype == GT_OBELISK ) {
//                    s = va("^1%3i%% ^4%3i%%",cg.redObeliskHealth,cg.blueObeliskHealth);
//                    CG_DrawSmallString( x, y-28, s, 1.0F);
//                }
//                
//                
//
//		if ( cgs.gametype >= GT_CTF && cgs.ffa_gt==0) {
//			v = cgs.capturelimit;
//		} else {
//			v = cgs.fraglimit;
//		}
//		if ( v ) {
//			s = va( "%2i", v );
//			w = CG_HeightToWidth(CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8);
//			x -= w;
//			CG_DrawBigStringAspect( x + CG_HeightToWidth(4), y, s, 1.0F);
//		}
//
//	} else {
//		qboolean	spectator;
//
//		x = 640;
//		score = cg.snap->ps.persistant[PERS_SCORE];
//		spectator = ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR );
//
//		// always show your score in the second box if not in first place
//		if ( s1 != score ) {
//			s2 = score;
//		}
//		if ( s2 != SCORE_NOT_PRESENT ) {
//			s = va( "%2i", s2 );
//			w = CG_HeightToWidth(CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8);
//			x -= w;
//			if ( !spectator && score == s2 && score != s1 ) {
//				CG_DrawScoreBox(x, y-4, w, BIGCHAR_HEIGHT+8, TEAM_RED);
//				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
//			} else {
//				CG_DrawScoreBox(x, y-4, w, BIGCHAR_HEIGHT+8, TEAM_FREE);
//			}	
//			CG_DrawBigStringAspect( x + CG_HeightToWidth(4), y, s, 1.0F);
//		}
//
//		// first place
//		if ( s1 != SCORE_NOT_PRESENT ) {
//			s = va( "%2i", s1 );
//			w = CG_HeightToWidth(CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8);
//			x -= w;
//			if ( !spectator && score == s1 ) {
//				CG_DrawScoreBox(x, y-4, w, BIGCHAR_HEIGHT+8, TEAM_BLUE);
//				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
//			} else {
//				CG_DrawScoreBox(x, y-4, w, BIGCHAR_HEIGHT+8, TEAM_FREE);
//			}	
//			CG_DrawBigStringAspect( x + CG_HeightToWidth(4), y, s, 1.0F);
//		}
//
//		if ( cgs.fraglimit ) {
//			s = va( "%2i", cgs.fraglimit );
//			w = CG_HeightToWidth(CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8);
//			x -= w;
//			CG_DrawBigStringAspect( x + CG_HeightToWidth(4), y, s, 1.0F);
//		}
//
//	}
//
//	return y1 - 8;
//}

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
#ifndef MISSIONPACK
//#define SCORE_FLAGICON_HEIGHT (SCOREBOX_CHAR_MIN_WIDTH * 0.75)
#define SCORE_FLAGICON_HEIGHT (SCOREBOX_CHAR_MIN_WIDTH * 0.6)
static float CG_DrawScores( float y ) {
	const char	*s;
	int			s1, s2, score;
	float			x, w, w2;
	int			v;
	float		y1;
	gitem_t		*item;
        int statusA,statusB;
	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
        
        statusA = cgs.redflag;
        statusB = cgs.blueflag;

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -= SCOREBOX_HEIGHT + 4;

	y1 = y;

	// draw from the right side to left
	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1) {
		x = 640;
		s = va( "%i", s2 );
		w = CG_DrawScoreBox(x, y, TEAM_BLUE, s, cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE);
		x -= w;
		if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_BLUEFLAG );

			if (item) {
				y1 = y - SCORE_FLAGICON_HEIGHT;
				if( cgs.blueflag >= 0 && cgs.blueflag <= 2 ) {
					w2 = CG_HeightToWidth(SCORE_FLAGICON_HEIGHT);
					CG_DrawPic( x + (w - w2)/2.0, y1, w2, SCORE_FLAGICON_HEIGHT, cgs.media.blueFlagShader[cgs.blueflag] );
				}
			}
		}
                
                if ( cgs.gametype == GT_DOUBLE_D ) {
			// Display Domination point status
			
				y1 = y - 32 - 4;//BIGCHAR_HEIGHT - 8;
				if( cgs.redflag >= 0 && cgs.redflag <= 3 ) {
					CG_DrawPic( x, y1, w, 32, cgs.media.ddPointSkinB[cgs.blueflag] );
				}
		}
                
		s = va( "%i", s1 );
		w = CG_DrawScoreBox(x, y, TEAM_RED, s, cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED);
		x -= w;

		if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_REDFLAG );

			if (item) {
				y1 = y - SCORE_FLAGICON_HEIGHT;
				if( cgs.redflag >= 0 && cgs.redflag <= 2 ) {
					w2 = CG_HeightToWidth(SCORE_FLAGICON_HEIGHT);
					CG_DrawPic( x + (w - w2)/2.0, y1, w2, SCORE_FLAGICON_HEIGHT, cgs.media.redFlagShader[cgs.redflag] );
				}
			}
		}
                
                if ( cgs.gametype == GT_DOUBLE_D ) {
			// Display Domination point status
			
				y1 = y - 32 - 4;//BIGCHAR_HEIGHT - 8;
				if( cgs.redflag >= 0 && cgs.redflag <= 3 ) {
					CG_DrawPic( x, y1, w, 32, cgs.media.ddPointSkinA[cgs.redflag] );
				}
                                
                        
                                
                        //Time till capture:
                        if( ( ( statusB == statusA ) && ( statusA == TEAM_RED ) ) ||
                            ( ( statusB == statusA ) && ( statusA == TEAM_BLUE ) ) ) {
                                s = va("%i",(cgs.timetaken+10*1000-cg.time)/1000+1);
                                w2 = CG_HeightToWidth(CG_DrawStrlen( s ) * BIGCHAR_WIDTH);
                                CG_DrawBigStringAspect( x + w - w2/2.0, y-28, s, 1.0F);
                        }
		}
                
                if ( cgs.gametype == GT_OBELISK ) {
			y1 = y - 28;
			s = va("^1%3i%% ^4%3i%%",cg.redObeliskHealth,cg.blueObeliskHealth);
			CG_DrawSmallString( x, y1, s, 1.0F);
                }
                
                

		if ( cgs.gametype >= GT_CTF && cgs.ffa_gt==0) {
			v = cgs.capturelimit;
		} else {
			v = cgs.fraglimit;
		}
		if ( v ) {
			s = va( "%i", v );
			w = CG_HeightToWidth(CG_DrawStrlen( s ) * SCOREBOX_CHAR_WIDTH + 8);
			x -= w;
			CG_DrawStringExtFloat(
					x + CG_HeightToWidth(4),
					y + (SCOREBOX_HEIGHT - SCOREBOX_CHAR_HEIGHT)/2.0,
					s, 
					color, qfalse, qfalse,
					CG_HeightToWidth(SCOREBOX_CHAR_WIDTH),
					SCOREBOX_CHAR_HEIGHT,
					0);
		}

	} else {
		qboolean	spectator;

		x = 640;
		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR );

		// always show your score in the second box if not in first place
		if ( s1 != score ) {
			s2 = score;
		}
		if ( s2 != SCORE_NOT_PRESENT ) {
			s = va( "%i", s2 );
			if ( !spectator && score == s2 && score != s1 ) {
				w = CG_DrawScoreBox(x, y, TEAM_RED, s, qtrue);
			} else {
				w = CG_DrawScoreBox(x, y, TEAM_FREE, s, qfalse);
			}	
			x -= w;
		}

		// first place
		if ( s1 != SCORE_NOT_PRESENT ) {
			s = va( "%i", s1 );
			if ( !spectator && score == s1 ) {
				w = CG_DrawScoreBox(x, y, TEAM_BLUE, s, qtrue);
			} else {
				w = CG_DrawScoreBox(x, y, TEAM_FREE, s, qfalse);
			}	
			x -= w;
		}

		if ( cgs.fraglimit ) {
			s = va( "%i", cgs.fraglimit );
			w = CG_HeightToWidth(CG_DrawStrlen( s ) * SCOREBOX_CHAR_WIDTH + 8);
			x -= w;
			CG_DrawStringExtFloat(
					x + CG_HeightToWidth(4),
					y + (SCOREBOX_HEIGHT - SCOREBOX_CHAR_HEIGHT)/2.0,
					s, 
					color, qfalse, qfalse,
					CG_HeightToWidth(SCOREBOX_CHAR_WIDTH),
					SCOREBOX_CHAR_HEIGHT,
					0);
		}

	}

	return y1 - 4;
}
#endif // MISSIONPACK

/*
================
CG_DrawPowerups
================
*/
#ifndef MISSIONPACK

//#define DPW_MARGIN 1
//#define DPW_DECOR_WIDTH 48
//#define DPW_DECOR_HEIGHT 43
//#define DPW_CHAR_SIZE 20
//#define DPW_NUMBER_XOFFSET (0.672*DPW_DECOR_WIDTH)
//#define DPW_NUMBER_YOFFSET (0.084*DPW_DECOR_WIDTH)
//#define DPW_ICON_XOFFSET (0.319*DPW_DECOR_WIDTH)
//#define DPW_ICON_YOFFSET (0.418*DPW_DECOR_WIDTH)
//#define DPW_ICON_SIZE 20

#define DPW_MARGIN 1
#define DPW_DECOR_WIDTH 58
#define DPW_DECOR_HEIGHT (0.75*DPW_DECOR_WIDTH)
#define DPW_CHAR_SIZE (DPW_DECOR_WIDTH*0.45)
#define DPW_NUMBER_XOFFSET (0.672*DPW_DECOR_WIDTH)
#define DPW_NUMBER_YOFFSET (0.084*DPW_DECOR_WIDTH)
#define DPW_ICON_XOFFSET (0.319*DPW_DECOR_WIDTH)
#define DPW_ICON_YOFFSET (0.418*DPW_DECOR_WIDTH)
#define DPW_ICON_SIZE 27
static float CG_DrawPowerups( float y ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	playerState_t	*ps;
	int		t;
	gitem_t	*item;
	float		decor_x = 0;
	float	size;
	float	f;
	float char_width, char_height;
	float icon_sz;
	float powerup_height;
	static float colors[3][4] = { 
		{ 1.0f, 1.0f, 1.0f, 1.0f } , 
		{ 1.0f, 0.2f, 0.2f, 1.0f } ,
		{ 1.0f, 0.071f, 0.1177f, 1.0f },
	};

	ps = &cg.snap->ps;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if (cg_ratStatusbar.integer >= 4 && cg_ratStatusbar.integer <= 5) {
		char_height = DPW_CHAR_SIZE;
		char_width = CG_HeightToWidth(char_height);
		icon_sz = DPW_ICON_SIZE;

		powerup_height = DPW_DECOR_HEIGHT;

		decor_x = SCREEN_WIDTH - char_width * 2 - CG_HeightToWidth(DPW_MARGIN + DPW_NUMBER_XOFFSET);
	} else {
		char_width = CG_HeightToWidth(CHAR_WIDTH);
		char_height = CHAR_HEIGHT;
		icon_sz = ICON_SIZE;

		powerup_height = icon_sz;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}
		t = ps->powerups[ i ] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if ( t < 0 || t > 999000) {
			continue;
		}

                item = BG_FindItemForPowerup( i );
                if ( item && item->giType == IT_PERSISTANT_POWERUP)
                    continue; //Don't draw persistant powerups here!

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_FindItemForPowerup( sorted[i] );

		if (item) {


			y -= powerup_height;

			if (cg_ratStatusbar.integer >= 4 && cg_ratStatusbar.integer <= 5) {
				int numDigits, v;

				CG_DrawPic( decor_x,
					       	y, 
						CG_HeightToWidth(DPW_DECOR_WIDTH),
					       	DPW_DECOR_WIDTH, 
						cgs.media.powerupFrameShader);

				v = sortedTime[ i ] / 1000;
				numDigits = 1;
				while (numDigits < 2 && (v /= 10)) {
					numDigits++;
				}
				if (sortedTime[ i ] / 1000 <= 5) {
					trap_R_SetColor( colors[2] );
				} else {
					trap_R_SetColor( colors[0] );
				}
				CG_DrawField( decor_x + CG_HeightToWidth(DPW_NUMBER_XOFFSET),
					       	y + DPW_NUMBER_YOFFSET, numDigits, sortedTime[ i ] / 1000, qfalse, char_width, char_height);
			} else {
				trap_R_SetColor( colors[1] );
				CG_DrawField( SCREEN_WIDTH - CG_HeightToWidth(icon_sz) - char_width * 2, y, 2, sortedTime[ i ] / 1000, qfalse, char_width, char_height);
			}

			t = ps->powerups[ sorted[i] ];
			if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
				trap_R_SetColor( NULL );
			} else {
				vec4_t	modulate;

				f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
				trap_R_SetColor( modulate );
			}

			if ( cg.powerupActive == sorted[i] && 
					cg.time - cg.powerupTime < PULSE_TIME ) {
				f = 1.0 - ( ( (float)cg.time - cg.powerupTime ) / PULSE_TIME );
				size = icon_sz * ( 1.0 + ( PULSE_SCALE - 1.0 ) * f );
			} else {
				size = icon_sz;
			}
			
			if (cg_ratStatusbar.integer >= 4 && cg_ratStatusbar.integer <= 5) {
				CG_DrawPic( decor_x + CG_HeightToWidth(DPW_ICON_XOFFSET - size/2.0),
					       	y + DPW_ICON_YOFFSET - size/2.0,
						CG_HeightToWidth(size), size, trap_R_RegisterShader( item->icon ) );
			} else {
				CG_DrawPic( SCREEN_WIDTH - CG_HeightToWidth(size), y + icon_sz / 2 - size / 2, 
						CG_HeightToWidth(size), size, trap_R_RegisterShader( item->icon ) );
			}
		}
	}
	trap_R_SetColor( NULL );

	return y;
}
#endif // MISSIONPACK

#define RADAR_SIZE 50
#define RADARDOT_SIZE 6
#define RADAR_RADIUS 16
#define RADAR_MINRADIUS 4
#define RADAR_RANGE 2000.0
#define RADAR_CARRIERINFO_HEIGHT (SUPERTINYCHAR_HEIGHT+1)
#define RADAR_CARRIERINFO_WIDTH (RADAR_SIZE*1.5)

static float CG_DrawRadar( float y ) {
	float		color[4];
	centity_t *cent;
	int i;
	clientInfo_t *ci;
	centity_t *fc = NULL;
	vec3_t viewvec;
	vec3_t angles;
	float flagangle;
	float u,r;
	float w,h;
	float x, y2 = 0;
	float ci_namewidth = 0;
	float distance;


	if (cgs.gametype != GT_CTF &&
			cgs.gametype != GT_CTF_ELIMINATION &&
			cgs.gametype != GT_1FCTF) {
		return y;
	}

	if (!(cgs.ratFlags & RAT_FLAGINDICATOR)
			|| !cg_radar.integer
			|| (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
			|| (cg.snap->ps.pm_flags & PMF_FOLLOW)
			|| cg.scoreBoardShowing) {
		return y;
	}

	for (i = 0; i < MAX_CLIENTS; ++i) {
		if (i == cg.clientNum) {
			continue;
		}
		cent = &cg_entities[i];

		if (!cent->currentValid || cent->currentState.eType != ET_PLAYER) {
			continue;
		}

		ci = &cgs.clientinfo[cent->currentState.clientNum];

		if (cg.snap->ps.persistant[PERS_TEAM] != ci->team) {
			continue;
		}

		if (cent->currentState.powerups & 
				( ( 1 << PW_REDFLAG)
				 |( 1 << PW_BLUEFLAG)
				 |( 1 << PW_NEUTRALFLAG))) {
			fc = cent;
			break;
		}
	}

	if (!fc) {
		return y;
	}

	vectoangles(cg.refdef.viewaxis[0], angles);
	flagangle = angles[1];
	VectorSubtract( cent->lerpOrigin, cg.refdef.vieworg, viewvec);
	vectoangles(viewvec, angles);
	flagangle = flagangle - angles[1];
	if (flagangle < 0.0) {
		flagangle += 360.0;
	}

	flagangle = flagangle*M_PI/180.0;

	distance = VectorLength(viewvec);

	u = (RADAR_RADIUS * MIN(1.0, distance/RADAR_RANGE) + RADAR_MINRADIUS) * cos(flagangle);
	r = (RADAR_RADIUS * MIN(1.0, distance/RADAR_RANGE) + RADAR_MINRADIUS) * sin(flagangle);


	h = RADAR_SIZE;
	w = CG_HeightToWidth(h);

	switch (cg_radar.integer) {
		case 2:
			x = SCREEN_WIDTH - w/2.0 - 2.0;
			y = y - 2.0 - h/2.0;
			y2 = y - h/2.0 - RADAR_CARRIERINFO_HEIGHT;
			break;
		case 1:
		default:
			x = SCREEN_WIDTH/2.0;
			y = 32 + RADAR_CARRIERINFO_HEIGHT;
			//y = 48 - RADAR_CARRIERINFO_HEIGHT;
			break;
	}

	ci_namewidth = MIN(CG_DrawStrlen(ci->name), (RADAR_CARRIERINFO_WIDTH-2)/SUPERTINYCHAR_WIDTH) * CG_HeightToWidth(SUPERTINYCHAR_WIDTH);
	CG_DrawHealthBar(cg_radar.integer == 2 ? (x + w/2.0 - CG_HeightToWidth(RADAR_CARRIERINFO_WIDTH)) : (x - CG_HeightToWidth(RADAR_CARRIERINFO_WIDTH/2.0)), 
			y - h/2.0 - RADAR_CARRIERINFO_HEIGHT,
			CG_HeightToWidth(RADAR_CARRIERINFO_WIDTH),
			RADAR_CARRIERINFO_HEIGHT,
			ci->health, ci->armor);
	color[0] = 1.0;
	color[1] = 1.0;
	color[2] = 1.0;
	color[3] = 1.0;
	CG_DrawStringExt(cg_radar.integer == 2 ? (x + w/2.0 - CG_HeightToWidth(RADAR_CARRIERINFO_WIDTH-2)/2.0 - ci_namewidth/2.0) : (x - ci_namewidth/2.0),
		       	y - h/2.0 - (RADAR_CARRIERINFO_HEIGHT-1),
			ci->name, color, qfalse, qtrue,
			CG_HeightToWidth(SUPERTINYCHAR_WIDTH), RADAR_CARRIERINFO_HEIGHT-1,
		       	(RADAR_CARRIERINFO_WIDTH-2)/SUPERTINYCHAR_WIDTH);



	color[0] = 0.8;
	color[1] = 0.8;
	color[2] = 0.8;
	color[3] = 0.75;
	trap_R_SetColor(color);

	//CG_DrawPic(320-w/2.0, 32 - h/2.0, w, h, cgs.media.radarShader);
	CG_DrawPic(x-w/2.0, y - h/2.0, w, h, cgs.media.radarShader);

	color[3] = 1.0;
	if (fc->currentState.powerups & ( 1 << PW_REDFLAG)) {
		color[0] = 1.0;
		color[1] = 0.0;
		color[2] = 0.0;
	} else if (fc->currentState.powerups & ( 1 << PW_BLUEFLAG)) {
		color[0] = 0.0;
		color[1] = 0.33;
		color[2] = 1.0;
	} else {
		color[0] = 1.0;
		color[1] = 1.0;
		color[2] = 1.0;
	}
	trap_R_SetColor(color);
	h = RADARDOT_SIZE;
	w = CG_HeightToWidth(h);

	CG_DrawPic(x + r - w/2.0, 
			y - u - h/2.0, w, h, cgs.media.radarDotShader);

	//CG_DrawPic(320 + r - w/2.0, 
	//		32 - u - h/2.0, w, h, cgs.media.radarDotShader);

	return y2;
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	//float		x;
	//vec4_t		color;
	const char	*name;
	char string[1024];

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		return qfalse;
	}

	if ( cg.scoreBoardShowing ) {
		return qfalse;
	}

	//color[0] = 1;
	//color[1] = 1;
	//color[2] = 1;
	//color[3] = 1;


	//CG_DrawSmallString( 320 - 9 * 8, 32, "following", 1.0F );

	name = cgs.clientinfo[ cg.snap->ps.clientNum ].name;

	//Com_sprintf(string, sizeof(string), "following %s", name);
	Com_sprintf(string, sizeof(string), "%s", name);

	//y -= SMALLCHAR_HEIGHT;

	switch (cg_drawFollowPosition.integer) {
		case 1:
			CG_DrawSmallString( 0.85 * 640 - (SMALLCHAR_WIDTH * CG_DrawStrlen(string)), 4, string, 1.0F );
			break;
		//case 2:
		//	CG_DrawStringExt( 0.85 * 640 - (TINYCHAR_WIDTH * CG_DrawStrlen(string)), 4,  name, colorWhite, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
		//	break;
		default:
			CG_DrawSmallString( 0.5 * (640 - SMALLCHAR_WIDTH * CG_DrawStrlen(string)), 32, string, 1.0F );
			break;
	}
	//CG_DrawSmallString( SCREEN_WIDTH -  (SMALLCHAR_WIDTH * CG_DrawStrlen(string)), y, string, 1.0F );

	//x = 0.5 * ( 640 - GIANT_WIDTH * CG_DrawStrlen( name ) );

	//CG_DrawStringExt( x, 40, name, color, qfalse, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );

	return qtrue;
}

/*
=====================
CG_DrawLowerRight

=====================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerRight( void ) {
	float	y;

	y = 480 - ICON_SIZE;

	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1 && cg_drawTeamOverlay.integer == 2 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qfalse );
	} 

	y = CG_DrawScores( y );

	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1 && cg_drawTeamOverlay.integer == 4 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qfalse );
	} 

	if (cg_radar.integer == 2) {
		y = CG_DrawRadar( y );
	}

	//y = CG_DrawFollow( y );

	y = CG_DrawPowerups( y );
}
#endif // MISSIONPACK

/*
===================
CG_DrawPickupItem
===================
*/
#ifndef MISSIONPACK
static float CG_DrawPickupItem( float y ) {
	int		value;
	float	*fadeColor;
	float iconSz = cg_pickupScale.value * MEDIUMCHAR_HEIGHT;
	float char_width = CG_HeightToWidth(cg_pickupScale.value * MEDIUMCHAR_WIDTH);
	float char_height = cg_pickupScale.value * MEDIUMCHAR_HEIGHT;

	if (!cg_drawPickup.integer) {
		return y;
	}

	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if (cg_drawPickup.integer == 2) {
		y -= iconSz;
	} 

	value = cg.itemPickup;
	if ( value ) {
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor ) {
			CG_RegisterItemVisuals( value );
			trap_R_SetColor( fadeColor );
			if (cg_drawPickup.integer == 2) {
				CG_DrawPic( CG_HeightToWidth(8), y, CG_HeightToWidth(iconSz), iconSz, cg_items[ value ].icon );
				CG_DrawStringExtFloat( CG_HeightToWidth(iconSz + 8 + 4),
						y + (iconSz - char_height)/2.0,
						bg_itemlist[ value ].pickup_name,
						fadeColor,
						qfalse, qfalse, char_width, char_height, 0 );
			} else {
				int len = CG_DrawStrlen(bg_itemlist[ value ].pickup_name);
				float x = SCREEN_WIDTH - CG_HeightToWidth(iconSz + 6);
				CG_DrawPic( x, y, CG_HeightToWidth(iconSz), iconSz, cg_items[ value ].icon );
				x -= char_width/2.0 + len * char_width;
				CG_DrawStringExtFloat( x,
						y + (iconSz - char_height)/2.0,
						bg_itemlist[ value ].pickup_name,
						fadeColor,
						qfalse, qfalse, char_width, char_height, 0 );
				y += iconSz;
			}
			trap_R_SetColor( NULL );
		}
	}
	
	return	y;
}
#endif // MISSIONPACK

/*
=====================
CG_DrawLowerLeft

=====================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerLeft( void ) {
	float	y;

	y = 480 - ICON_SIZE;

	if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1 && cg_drawTeamOverlay.integer == 3 ) {
		y = CG_DrawTeamOverlay( y, qfalse, qfalse );
	} 


	if (cg_drawPickup.integer == 2) {
		y = CG_DrawPickupItem( y );
	}
}
#endif // MISSIONPACK


//===========================================================================================

/*
=================
CG_DrawTeamChat
=================
*/
#ifndef MISSIONPACK
static void CG_DrawTeamChat( void ) {
	//int w, h;
	int w;
	int i, len;
	vec4_t		hcolor;
	int		chatHeight;

	if (cg_newConsole.integer) {
		return;
	}

//#define CHATLOC_Y 420 // bottom end
#define CHATLOC_X 0

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		//h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT * cg_teamChatScaleY.value;

		w = 0;

		for (i = cgs.teamLastChatPos; i < cgs.teamChatPos; i++) {
			len = CG_DrawStrlen(cgs.teamChatMsgs[i % chatHeight]);
			if (len > w)
				w = len;
		}
		w *= TINYCHAR_WIDTH * cg_teamChatScaleX.value;
		w += TINYCHAR_WIDTH * cg_teamChatScaleX.value * 2;

		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		} else {
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		// don't draw blue background
		//trap_R_SetColor( hcolor );
		//CG_DrawPic( CHATLOC_X, cg_teamChatY.integer - h, 640, h, cgs.media.teamStatusBar );
		//trap_R_SetColor( NULL );

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
			CG_DrawStringExt( CHATLOC_X + TINYCHAR_WIDTH * cg_teamChatScaleX.value, 
				cg_teamChatY.integer - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT * cg_teamChatScaleY.value, 
				cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH * cg_teamChatScaleX.value, TINYCHAR_HEIGHT * cg_teamChatScaleY.value, 0 );
		}
	}
}
#endif // MISSIONPACK

float CG_FontResAdjust(void) {
	float f = 1.0;
	if (cgs.glconfig.vidHeight < 1024) {
		f *= 1024.0/cgs.glconfig.vidHeight;
	}
	return f;
}

float CG_ConsoleDistortionFactorX(void) {
	return ((cgs.screenXScale > cgs.screenYScale) ? (cgs.screenYScale / cgs.screenXScale) : 1.0);
}

float CG_ConsoleDistortionFactorY(void) {
	return ((cgs.screenYScale > cgs.screenXScale) ? (cgs.screenXScale / cgs.screenYScale) : 1.0);
}

float CG_ConsoleAdjustSizeX(float sizeX) {
	return CG_FontResAdjust() * sizeX * CG_ConsoleDistortionFactorX();
}

float CG_ConsoleAdjustSizeY(float sizeY) {
	return CG_FontResAdjust() * sizeY * CG_ConsoleDistortionFactorY();
}

int CG_GetChatHeight(int maxlines) {
	if (maxlines < CONSOLE_MAXHEIGHT)
		return maxlines;
	return CONSOLE_MAXHEIGHT;
}

int CG_ConsoleChatPositionY(float consoleSizeY, float chatSizeY) {
	return CG_GetChatHeight(cg_consoleLines.integer) * consoleSizeY + chatSizeY/2;
}


void CG_ConsoleUpdateIdx(console_t *console, int chatHeight) {
	if (console->insertIdx < console->displayIdx) {
		console->displayIdx = console->insertIdx;
	}

	if (console->insertIdx - console->displayIdx > chatHeight) {
		console->displayIdx = console->insertIdx - chatHeight;
	}
}


static void CG_DrawGenericConsole( console_t *console, int maxlines, int time, int x, int y, float sizeX, float sizeY ) {
	int i, j;
	vec4_t	hcolor;
	int	chatHeight;

	if (!cg_newConsole.integer) {
		return;
	}

	chatHeight = CG_GetChatHeight(maxlines);

	if (chatHeight <= 0)
		return; // disabled

	CG_ConsoleUpdateIdx(console, chatHeight);

	hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0f;

	j = 0;
	for (i = console->displayIdx; i < console->insertIdx ; ++i) {
		if (console->msgTimes[i % CONSOLE_MAXHEIGHT] + time < cg.time) {
			continue;
		}
		CG_DrawStringExtFloat( x + 1,
				  y + j * sizeY,
				  console->msgs[i % CONSOLE_MAXHEIGHT],
				  hcolor, qfalse, cg_fontShadow.integer ? qtrue : qfalse,
				  sizeX,  sizeY, 0 );
		j++;

	}
}

void CG_AddToGenericConsole( const char *str, console_t *console ) {
	int len;
	char *p, *ls;
	int lastcolor;

	len = 0;

	p = console->msgs[console->insertIdx % CONSOLE_MAXHEIGHT];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while (*str) {
		if (*str == '\n' || len > CONSOLE_WIDTH - 1) {
			if (*str == '\n') {
				str++;
				if (*str == '\0') {
					continue;
				}
			} else if (ls) {
				str -= (p - ls);
				str++;
				p -= (p - ls);
			}
			*p = 0;

			console->msgTimes[console->insertIdx % CONSOLE_MAXHEIGHT] = cg.time;

			console->insertIdx++;
			p = console->msgs[console->insertIdx % CONSOLE_MAXHEIGHT];
			*p = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len = 0;
			ls = NULL;
		}

		if ( Q_IsColorString( str ) ) {
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if (*str == ' ') {
			ls = p;
		}
		*p++ = *str++;
		len++;
	}
	*p = 0;

	console->msgTimes[console->insertIdx % CONSOLE_MAXHEIGHT] = cg.time;
	console->insertIdx++;

}

/*
===================
CG_DrawHoldableItem
===================
*/
#ifndef MISSIONPACK
static void CG_DrawHoldableItem( void ) { 
	int		value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( 640-CG_HeightToWidth(ICON_SIZE), (SCREEN_HEIGHT-ICON_SIZE)/2, CG_HeightToWidth(ICON_SIZE), ICON_SIZE, cg_items[ value ].icon );
	}

}
#endif // MISSIONPACK

#ifndef MISSIONPACK
/*
===================
CG_DrawPersistantPowerup
===================
*/
#if 1 // sos001208 - DEAD // sago - ALIVE
static void CG_DrawPersistantPowerup( void ) { 
	int		value;

	value = cg.snap->ps.stats[STAT_PERSISTANT_POWERUP];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2 - ICON_SIZE, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
	}
}
#endif
#endif // MISSIONPACK

int CG_Reward2Time(int idx) {
	return REWARD2_TIME + cg.reward2SoundDelay[idx] + 200;
}

static void CG_DrawReward( void ) { 
	float	*color;
	int	i;
	float	x,xx, y, yy;
	char	buf[16];
	int numMedals;
	int skip;
	float lineLength = 0.0;
	float iconWidth;
	float iconHeight;
	float charWidth;
	float charHeight;
	float scale;

	iconWidth = CG_HeightToWidth(ICON_SIZE);
	skip = 0;
	numMedals = 0;
	for (i = 0; i < MAX_REWARDROW; ++i) {
		if (cg.reward2RowTimes[i] == -1) {
			cg.reward2RowTimes[i] = cg.time;
		}
		if (cg.reward2RowTimes[i] != 0 && cg.reward2RowTimes[i] + CG_Reward2Time(i) > cg.time) {
			++numMedals;
			lineLength +=  iconWidth * CG_FadeScale(cg.reward2RowTimes[i], CG_Reward2Time(i));
			// shift backwards over empty slots
			if (skip) {
				cg.reward2RowTimes[i-skip] = cg.reward2RowTimes[i];
				cg.reward2Shader[i-skip] = cg.reward2Shader[i];
				cg.reward2Count[i-skip] = cg.reward2Count[i];
				cg.reward2SoundDelay[i-skip] = cg.reward2SoundDelay[i];

				cg.reward2RowTimes[i] = 0;
			}
		} else {
			skip++;
		}
	}


	y = 56;
	x = 320 - lineLength/2.0;
	for ( i = 0 ; i < numMedals ; ++i ) {
		color = CG_FadeColor( cg.reward2RowTimes[i], CG_Reward2Time(i) );
		scale = CG_FadeScale( cg.reward2RowTimes[i], CG_Reward2Time(i) );
		iconHeight = (float)ICON_SIZE * scale;
		trap_R_SetColor( color );

		yy = y + (ICON_SIZE - iconHeight)/2.0;

		CG_DrawPicSquareByHeight( x+CG_HeightToWidth(2), yy, MAX(0.0,iconHeight-4), cg.reward2Shader[i] );

		charWidth  = CG_HeightToWidth((float)SMALLCHAR_WIDTH * scale);
		charHeight = (float)SMALLCHAR_HEIGHT * scale;
		iconWidth = CG_HeightToWidth(iconHeight);

		Com_sprintf(buf, sizeof(buf), "%d", cg.reward2Count[i]);
		xx = x + (iconWidth - charWidth * CG_DrawStrlen(buf)) / 2;
		CG_DrawStringExt( xx, yy+iconHeight, buf, color, qfalse, qtrue,
								charWidth, charHeight, 0 );
		x += iconWidth;
	}

	trap_R_SetColor( NULL );
}

static void CG_DrawReward2( void ) { 
	float	*color;
	int		i, count;
	float	x, y;
	char	buf[32];

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			
			// some awards may not have sounds
			if (cg.rewardSound[0]) {
				trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);  
			}

		} else {
			return;
		}
	}

	trap_R_SetColor( color );

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.rewardCount[0] >= 10 ) {
		y = 56;
		x = 320 - ICON_SIZE/2;
		CG_DrawPic( x+2, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = ( SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen( buf ) ) / 2;
		CG_DrawStringExt( x, y+ICON_SIZE, buf, color, qfalse, qtrue,
								SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
	}
	else {

		count = cg.rewardCount[0];

		y = 56;
		x = 320 - count * ICON_SIZE/2;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x+2, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
			x += ICON_SIZE;
		}
	}
	trap_R_SetColor( NULL );
}

/*
===================
CG_DrawReady
===================
*/
static void CG_DrawReady ( void ) {
	char *s;
	int w;
	float char_width, char_height;

	if ( !cgs.startWhenReady )
		return;
	if ( cg.warmup >= 0 )
		return;

	if ( cg.readyMask & ( 1 << cg.snap->ps.clientNum ) ) {
		s = "^2You are ready!";
	} else {
		s = "^1Type \\ready to ready up!";
	}

	char_height = 15;
	char_width = CG_HeightToWidth(10);
	w = CG_DrawStrlen( s ) * char_width;
	CG_DrawStringFloat((SCREEN_WIDTH - w)/2.0, 90, s, 1.0, char_width, char_height);
}

/*
===================
CG_DrawQueue
===================
*/
static void CG_DrawQueue ( void ) {
	char *s;
	int w;
	float char_width, char_height;

	if (cgs.gametype < GT_TEAM || cgs.ffa_gt == 1) {
		return;
	}

	if (!cgs.clientinfo[ cg.clientNum ].infoValid ||
		       	cgs.clientinfo[ cg.clientNum ].team != TEAM_SPECTATOR) {
		return;
	}

	switch (cg.spectatorGroup) {
		case SPECTATORGROUP_QUEUED:
			s = "Waiting to auto-join";
			break;
		case SPECTATORGROUP_QUEUED_BLUE:
			s = S_COLOR_BLUE "Waiting to join blue";
			break;
		case SPECTATORGROUP_QUEUED_RED:
			s = S_COLOR_RED "Waiting to join red";
			break;
		default:
			return;
	}

	char_height = 15;
	char_width = CG_HeightToWidth(10);
	w = CG_DrawStrlen( s ) * char_width;
	CG_DrawStringFloat((SCREEN_WIDTH - w)/2.0, 90, s, 1.0, char_width, char_height);
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	//lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	//
	// this might not be always 100% accurate, but it's better than
	// displaying a 999 ping whenever snap->ping is above 250 or so
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = CG_ReliablePingFromSnaps(cg.snap, snap);

	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;  // bk010215 - FIXME char message[1024];

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart // bk 0102165 - FIXME
		return;
	}

	if (cg.time < cgs.timeoutEnd) {
		return;
	}

	// also add text in center of screen
	s = "Connection Interrupted"; // bk 010215 - FIXME
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
}


#define BOTTOM_FPS_SIZE 48
static void CG_DrawBottomFPS( void ) {
	float h = BOTTOM_FPS_SIZE;
	float w = CG_HeightToWidth(h);
	float w_s;
	float x = SCREEN_WIDTH - w;
	float y = SCREEN_HEIGHT - BOTTOM_FPS_SIZE;
	int fps;
	float	color[4];
	float	char_height = 13;
	float	char_width = CG_HeightToWidth(8);
	//float	char_height = 12;
	//float	char_width = CG_HeightToWidth(6);
	char		*s;

	if (cg_lagometer.integer && !cgs.localServer) {
		if (cg.scoreBoardShowing) {
			// draw it over the lagometer in case the scoreboard is
			// up
			CG_DrawFPS(0);
			return;
		}
		x -= 48;
	}

	fps = CG_FPS();

	color[0] = color[1] = color[2] = color[3] = 1.0;
	color[3] = RSB4_DECOR_ALPHA;
	trap_R_SetColor(color);
	CG_DrawPic(x, y, w, h, cgs.media.bottomFPSShaderDecor );
	trap_R_SetColor(NULL);
	CG_DrawPic(x, y, w, h, cgs.media.bottomFPSShaderColor );

	if (fps == -1) {
		return;
	}

	s = va( "%i", fps );
	w_s = CG_DrawStrlen( s ) * char_width;

	color[0] = color[1] = color[2] = color[3] = 1.0;
	CG_DrawStringExt( x + CG_HeightToWidth(33) - w_s, SCREEN_HEIGHT-2-char_height, s, color, qfalse, qfalse, char_width, char_height, 0 );
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300
#define	LAGOMETER_ALPHA		0.25

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;
	float colortable[5][4] = {
		{ 0.0, 0.8, 1.0, 1.0 },
		{ 0.0, 0.8, 1.0, LAGOMETER_ALPHA },
		{ 1.0, 0.9883, 0.193, 1.0 },
		{ 1.0, 0.9883, 0.193, LAGOMETER_ALPHA },
		{ 1.0, 0.0, 0.0, LAGOMETER_ALPHA },
	};

	if ( !cg_lagometer.integer || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
#ifdef MISSIONPACK
	x = 640 - 48;
	y = 480 - 144;
#else
	x = 640 - 48;
	y = 480 - 48;
#endif

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				//trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				trap_R_SetColor(colortable[2]);
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				//trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
				trap_R_SetColor(colortable[0]);
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					//trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
					trap_R_SetColor(colortable[3]);
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					//trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
					trap_R_SetColor(colortable[1]);
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				//trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
				trap_R_SetColor(colortable[4]);
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_DrawBigString( ax, ay, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int		x, y, w;
#ifdef MISSIONPACK // bk010221 - unused else
  int h;
#endif
	float	*color;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

#ifdef MISSIONPACK
		w = CG_Text_Width(linebuffer, 0.5, 0);
		h = CG_Text_Height(linebuffer, 0.5, 0);
		x = (SCREEN_WIDTH - w) / 2;
		CG_Text_Paint(x, y + h, 0.5, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		y += h + 6;
#else
		w = cg.centerPrintCharWidth * CG_DrawStrlen( linebuffer );

		x = ( SCREEN_WIDTH - w ) / 2;

		CG_DrawStringExt( x, y, linebuffer, color, qfalse, qtrue,
			cg.centerPrintCharWidth, (int)(cg.centerPrintCharWidth * 1.5), 0 );

		y += cg.centerPrintCharWidth * 1.5;
#endif
		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}

/*
=====================
CG_DrawCenter1FctfString
=====================
*/
static void CG_DrawCenter1FctfString( void ) {
    #ifndef MISSIONPACK
    int		x, y, w;
    float       *color;
    char        *line;
    int status;
    
    if(cgs.gametype != GT_1FCTF)
        return;
    
    status = cgs.flagStatus;
    
    //Sago: TODO: Find the proper defines instead of hardcoded values.
    switch(status)
    {
        case 2:
            line = va("Red has the flag!");
            color = colorRed;
            break;
        case 3:
            line = va("Blue has the flag!");
            color = colorBlue;
            break;
        case 4:
            line = va("Flag dropped!");
            color = colorWhite;
            break;
        default:
            return;
            
    };
    y = 100;
    
    
    w = cg.centerPrintCharWidth * CG_DrawStrlen( line );

    x = ( SCREEN_WIDTH - w ) / 2;

    CG_DrawStringExt( x, y, line, color, qfalse, qtrue,
		cg.centerPrintCharWidth, (int)(cg.centerPrintCharWidth * 1.5), 0 );
    
    
    #endif
}



/*
=====================
CG_DrawCenterDDString
=====================
*/
static void CG_DrawCenterDDString( void ) {
    #ifndef MISSIONPACK
    int		x, y, w;
    float       *color;
    char        *line;
    int 		statusA, statusB;
    int sec;
    static int lastDDSec = -100;

    
    if (cgs.gametype != GT_DOUBLE_D) {
        return;
    }
    
    statusA = cgs.redflag;
    statusB = cgs.blueflag;
    
    if( ( ( statusB == statusA ) && ( statusA == TEAM_RED ) ) ||
            ( ( statusB == statusA ) && ( statusA == TEAM_BLUE ) ) ) {
      }
    else
        return; //No team is dominating
    
    if(statusA == TEAM_BLUE) {
        line = va("Blue scores in %i",(cgs.timetaken+10*1000-cg.time)/1000+1);
        color = colorBlue;
    } else if(statusA == TEAM_RED) {
        line = va("Red scores in %i",(cgs.timetaken+10*1000-cg.time)/1000+1);
        color = colorRed;
    } else {
        lastDDSec = -100;
        return;
    }

    sec = (cgs.timetaken+10*1000-cg.time)/1000+1;
    if(sec!=lastDDSec) {
        //A new number is being displayed... play the sound!
        switch ( sec ) {
            case 1:
                trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
                break;
            case 2:
                trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
                break;
            case 3:
                trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
                break;
            case 10:
                trap_S_StartLocalSound( cgs.media.doublerSound , CHAN_ANNOUNCER );
                break;
            default:
                break;
        }
    }
    lastDDSec = sec;
    
    y = 100;
    
    
    w = cg.centerPrintCharWidth * CG_DrawStrlen( line );

    x = ( SCREEN_WIDTH - w ) / 2;

    CG_DrawStringExt( x, y, line, color, qfalse, qtrue,
		cg.centerPrintCharWidth, (int)(cg.centerPrintCharWidth * 1.5), 0 );
    
    #endif
}


/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(void)
{
	float		w, h;
	qhandle_t	hShader, hShaderOutline;
	float		f;
	float		x, y;
	int			ca = 0; //only to get rid of the warning(not useful)
	int 		currentWeapon;
	vec4_t         color;
	
	currentWeapon = cg.predictedPlayerState.weapon;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

	// set color based on health
	if ( cg_crosshairHealth.integer ) {

		switch (cg_crosshairHealth.integer) {
			case 2:
				CG_ColorForHealth2( color );
				break;
			default:
				CG_ColorForHealth( color );
				break;
		}
	} else {
                color[0]=cg_crosshairColorRed.value;
                color[1]=cg_crosshairColorGreen.value;
                color[2]=cg_crosshairColorBlue.value;
                color[3]=1.0f;
	}
	if (cg_crosshairHit.integer &&
		       	cg.time - cg.lastHitTime <= cg_crosshairHitTime.integer &&
		       	cg.lastHitDamage > 0)  {
		float h,s,v;
		float fcolor[4];
		float frac;
		int hueRange = 60;
		int maxDamage = 100;
		int fadeTime = MIN(75.0, cg_crosshairHitTime.integer);

		CG_PlayerColorFromString(cg_crosshairHitColor.string, &h, &s, &v);
		frac = 1.0;
		// adjust color based on damage
		if (cg_crosshairHitStyle.integer == 1) {
			// style based on difference between hit color and default crosshair color
			frac = 0.5+0.5*(float)MIN(maxDamage, cg.lastHitDamage)/(float)maxDamage;
		} else if (cg_crosshairHitStyle.integer == 2) {
			// style based on hue range, positive direction
			h = h + (1.0-(float)MIN(maxDamage, cg.lastHitDamage)/(float)maxDamage) * hueRange;
			if (h >= 360.0) {
				h -= 360.0;
			}
		} else {
			// style based on hue range, negative direction
			h = h - (1.0 - (float)MIN(maxDamage, cg.lastHitDamage)/(float)maxDamage) * hueRange;
			if (h < 0) {
				h += 360;
			}
		}
		Q_HSV2RGB(h,s,v, fcolor);
		fcolor[3] = 1.0f;
		// fade color over time
		if (cg_crosshairHitTime.integer - (cg.time - cg.lastHitTime) <= fadeTime) {
			frac *= (float)(cg_crosshairHitTime.integer - (cg.time - cg.lastHitTime))/(float)fadeTime;
		}
		color[0] = color[0] + frac * (fcolor[0] - color[0]);
		color[1] = color[1] + frac * (fcolor[1] - color[1]);
		color[2] = color[2] + frac * (fcolor[2] - color[2]);

	}
	trap_R_SetColor( color );

	if( cg_differentCrosshairs.integer == 1 ){
		switch( currentWeapon ){
			case 1:
				w = h = cg_ch1size.value;
				ca = cg_ch1.integer;
				break;
			case 2:
				w = h = cg_ch2size.value;
				ca = cg_ch2.integer;
				break;
			case 3:
				w = h = cg_ch3size.value;
				ca = cg_ch3.integer;
				break;
			case 4:
				w = h = cg_ch4size.value;
				ca = cg_ch4.integer;
				break;
			case 5:
				w = h = cg_ch5size.value;
				ca = cg_ch5.integer;
				break;
			case 6:
				w = h = cg_ch6size.value;
				ca = cg_ch6.integer;
				break;
			case 7:
				w = h = cg_ch7size.value;
				ca = cg_ch7.integer;
				break;
			case 8:
				w = h = cg_ch8size.value;
				ca = cg_ch8.integer;
				break;
			case 9:
				w = h = cg_ch9size.value;
				ca = cg_ch9.integer;
				break;
			case 10:
				w = h = cg_ch10size.value;
				ca = cg_ch10.integer;
				break;
			case 11:
				w = h = cg_ch11size.value;
				ca = cg_ch11.integer;
				break;
			case 12:
				w = h = cg_ch12size.value;
				ca = cg_ch12.integer;
				break;
			case 13:
				w = h = cg_ch13size.value;
				ca = cg_ch13.integer;
				break;
                        default:
                                w = h = cg_crosshairSize.value;
                                ca = cg_drawCrosshair.integer;
                                break;
		}
	}
	else{
		w = h = cg_crosshairSize.value;
		ca = cg_drawCrosshair.integer;
	}

	w = CG_HeightToWidth(h);
	
	if( cg_crosshairPulse.integer ){
		// pulse the size of the crosshair when picking up items
		f = cg.time - cg.itemPickupBlendTime;
		if ( f > 0 && f < ITEM_BLOB_TIME ) {
			f /= ITEM_BLOB_TIME;
			w *= ( 1 + f );
			h *= ( 1 + f );
		}
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640( &x, &y, &w, &h );

	if (ca < 1) {
		ca = 1;
	}

	hShader = cgs.media.crosshairShader[ (ca-1) % NUM_CROSSHAIRS ];
	hShaderOutline = cgs.media.crosshairOutlineShader[ (ca-1) % NUM_CROSSHAIRS ];

        if(!hShader) {
            hShader = cgs.media.crosshairShader[ 0 ];
	    hShaderOutline = cgs.media.crosshairOutlineShader[ 0 ];
	}

	// draw the outline under the white crosshair
	if (hShaderOutline) {
		trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), 
				y + cg.refdef.y + 0.5 * (cg.refdef.height - h), 
				w, h, 0, 0, 1, 1, hShaderOutline );
	}

	trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), 
		y + cg.refdef.y + 0.5 * (cg.refdef.height - h), 
		w, h, 0, 0, 1, 1, hShader );
}

/*
=================
CG_DrawCrosshair3D
=================
*/
static void CG_DrawCrosshair3D(void)
{
	float		w, h;
	qhandle_t	hShader;
	float		f;
	int			ca;

	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

        if(!hShader)
            hShader = cgs.media.crosshairShader[ ca % 10 ];

	// Use a different method rendering the crosshair so players don't see two of them when
	// focusing their eyes at distant objects with high stereo separation
	// We are going to trace to the next shootable object and place the crosshair in front of it.

	// first get all the important renderer information
	trap_Cvar_VariableStringBuffer("r_zProj", rendererinfos, sizeof(rendererinfos));
	zProj = atof(rendererinfos);
	trap_Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos, sizeof(rendererinfos));
	stereoSep = zProj / atof(rendererinfos);
	
	xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);
	
	// let the trace run through until a change in stereo separation of the crosshair becomes less than one pixel.
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	VectorMA(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, endpos, 0, MASK_SHOT);
	
	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;
	
	VectorCopy(trace.endpos, ent.origin);
	
	// scale the crosshair so it appears the same size for all distances
	ent.radius = w / 640 * xmax * trace.fraction * maxdist / zProj;
	ent.customShader = hShader;

	trap_R_AddRefEntityToScene(&ent);
}



/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;
	qboolean throughwall = (cgs.ratFlags & RAT_FRIENDSWALLHACK && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR);
	qboolean lookedThroughWall = qfalse;
	int clientNum;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[0], end );


	CG_Trace( &trace, start, vec3_origin, vec3_origin, end, 
		cg.snap->ps.clientNum, 
		CONTENTS_SOLID |CONTENTS_BODY );
	if ( trace.entityNum >= MAX_CLIENTS 
			&& !CG_IsFrozenPlayer(&cg_entities[trace.entityNum])) {
		if (throughwall) {
			CG_Trace( &trace, start, vec3_origin, vec3_origin, end, 
					cg.snap->ps.clientNum, 
					CONTENTS_BODY );
			if ( trace.entityNum >= MAX_CLIENTS 
					&& !CG_IsFrozenPlayer(&cg_entities[trace.entityNum])) {
				return;
			}
			lookedThroughWall = qtrue;
		}  else {
			return;
		}
	}

	if (trace.entityNum >= MAX_CLIENTS) {
		clientNum = cg_entities[trace.entityNum].currentState.clientNum;
		if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
			return;
		}
	} else {
		clientNum = trace.entityNum;
	}

	// if the player is in fog, don't show it
	content = CG_PointContents( trace.endpos, cg.snap->ps.clientNum );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// if the player is invisible, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
		return;
	}

	// hide enemies during Treasure Hunter Hiding phase
	if (cgs.gametype == GT_TREASURE_HUNTER 
			&& trace.entityNum < MAX_CLIENTS
			&& !CG_THPlayerVisible(&cg_entities[ trace.entityNum ])) {
		return;
	}

	if (throughwall && lookedThroughWall) {
		// XXX: technically, this could give an enemy's position away
		// when he obscures the position of a friend
		clientInfo_t	*ci;
		ci = &cgs.clientinfo[clientNum];
	       	if (!ci->infoValid) {
			return;
		}
		if (ci->team != cg.snap->ps.persistant[PERS_TEAM]) {
			return;
		}
	}

	// update the fade timer
	cg.crosshairClientNum = clientNum;
	cg.crosshairClientTime = cg.time;
}

static void CG_DrawZoomScope( void ) {
	int weapon = cg.predictedPlayerState.weapon;
	qhandle_t shader;
	float h,w;
	float color[4] = { 0.0, 0.0, 0.0, 1.0 };
	float hue, sat, val;

	if (!cg_drawZoomScope.integer || !cg.zoomed) {
		return;
	}

	switch (weapon) {
		case WP_MACHINEGUN:
			shader = cgs.media.zoomScopeMGShader;
			CG_PlayerColorFromString(cg_zoomScopeMGColor.string, &hue, &sat, &val);
			Q_HSV2RGB(hue,sat,val, color);
			break;
		case WP_RAILGUN:
			shader = cgs.media.zoomScopeRGShader;
			CG_PlayerColorFromString(cg_zoomScopeRGColor.string, &hue, &sat, &val);
			Q_HSV2RGB(hue,sat,val, color);
			break;
		default:
			return;
	}
	color[3] = 1.0;

	h = SCREEN_HEIGHT * cg_zoomScopeSize.value;
	w = CG_HeightToWidth(h);
	trap_R_SetColor(color);
	CG_DrawPic((SCREEN_WIDTH - w)/2.0, (SCREEN_HEIGHT-h)/2.0, w, h, shader);
	color[0] = color[1] = color[2] = 0.0;
	color[3] = 0.6;
	if (h < SCREEN_HEIGHT) {
		CG_FillRect((SCREEN_WIDTH - w)/2.0, 0, w, (SCREEN_HEIGHT-h)/2.0, color);
		CG_FillRect((SCREEN_WIDTH - w)/2.0, SCREEN_HEIGHT-(SCREEN_HEIGHT-h)/2.0, w, (SCREEN_HEIGHT-h)/2.0, color);
	}
	if (w < SCREEN_WIDTH) {
		CG_FillRect(0, 0, (SCREEN_WIDTH-w)/2.0, SCREEN_HEIGHT, color);
		CG_FillRect((SCREEN_WIDTH + w)/2.0, 0, (SCREEN_WIDTH-w)/2.0, SCREEN_HEIGHT, color);
	}
	trap_R_SetColor(NULL);
}


static void CG_DrawEmptyIndicator( void ) {
	int ammo, ammoSaved;
	vec4_t color = { 1.0, 0.0, 0.0, 1.0 };
	int weapon = cg.predictedPlayerState.weapon;
	int char_height;
	int char_width;
	char *s;
	float h;
	float w;

	if (!cg_emptyIndicator.integer) {
		return;
	}

	if (weapon < 0 || weapon >= MAX_WEAPONS) {
		return;
	}

	if (cg_predictWeapons.integer) {
		ammoSaved=cg.predictedPlayerState.ammo[weapon];
	} else {
		ammoSaved=cg.snap->ps.ammo[weapon];
	}

	ammo = ammoSaved;
	ammo = MIN(100, (ammo*100)/CG_FullAmmo(weapon));

	if (ammo > 20 
			|| ammo < 0
			|| cg.predictedPlayerState.weaponstate == WEAPON_DROPPING
			|| cg.predictedPlayerState.weaponstate == WEAPON_RAISING
			|| cg.predictedPlayerState.pm_type == PM_DEAD
			) {
		return;
	}

	char_height = 10; 
	char_width = CG_HeightToWidth(char_height);
	s = ammoSaved == 0 ? "EMPTY" : "LOW";

	h = 180;

	w = CG_DrawStrlen(s) * char_width;
	CG_DrawStringExtFloat( (SCREEN_WIDTH - w) / 2.0, (SCREEN_HEIGHT + h - char_height)/2.0,
			s, color, qtrue, qfalse,
			char_width,
			char_height, 0 );
	if (ammoSaved == 0) {
		color[3] = 0.5;
		w = CG_HeightToWidth(h*1.6);
		CG_DrawCorners((SCREEN_WIDTH - w)/2.0,
				(SCREEN_HEIGHT - h)/2.0,
				w,
				h,
				h/5.0,
				4,
				color);
	}
}

#define MAX_RELOADTIME RAIL_RELOAD_REGULAR
static void CG_DrawReloadIndicator( void ) {
	int time = cg.predictedPlayerState.weaponTime;
	float width;
	int ammo, ammoSaved;
	vec4_t color;
	int weapon = cg.predictedPlayerState.weapon;

	if (!cg_reloadIndicator.integer
			|| cg.predictedPlayerState.pm_type == PM_DEAD
			) {
		return;
	}

	if (weapon < 0 || weapon >= MAX_WEAPONS) {
		return;
	}

	if (cg_predictWeapons.integer) {
		ammoSaved=cg.predictedPlayerState.ammo[weapon];
	} else {
		ammoSaved=cg.snap->ps.ammo[weapon];
	}

	ammo = ammoSaved;
	ammo = MIN(100, (ammo*100)/CG_FullAmmo(weapon));

	if (ammo <= 20) {
		color[0] = 1.0;
		color[1] = 0.0;
		color[2] = 0.0;
	} else if (ammo <= 50) {
		color[0] = 1.0;
		color[1] = 1.0;
		color[2] = 0.0;
	} else {
		color[0] = 1.0;
		color[1] = 1.0;
		color[2] = 1.0;
	}
	color[3] = MAX(0.0, MIN(1.0, cg_reloadIndicatorAlpha.value));


	if (ammoSaved != 0) {
		if (cg.predictedPlayerState.weaponstate != WEAPON_FIRING || time <= 0) {
			return;
		}

		switch ( weapon ) {
			case WP_LIGHTNING:
			case WP_MACHINEGUN:
			case WP_PLASMAGUN:
			case WP_BFG:
			case WP_CHAINGUN:
				return;
		}
		width = MIN(1.0,(float)time/MAX_RELOADTIME) * CG_HeightToWidth(cg_reloadIndicatorWidth.value);
	} else {
		width = CG_HeightToWidth(cg_reloadIndicatorWidth.value);
	}

	CG_FillRect((SCREEN_WIDTH - width)/2,
			cg_reloadIndicatorY.value,
			width,
			cg_reloadIndicatorHeight.value,
			color);

}

/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	char		*name;
	float		w;
	int	char_width;
	int	char_height = 8;


	if ( !cg_drawCrosshair.integer ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
#ifdef MISSIONPACK
	color[3] *= 0.5f;
	w = CG_Text_Width(name, 0.3f, 0);
	CG_Text_Paint( 320 - w / 2, 190, 0.3f, color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
#else
	char_width = CG_HeightToWidth(char_height);
	w = CG_DrawStrlen( name ) * char_width;
	color [0] = color[1] = color[2] = color[3] = 1.0;
	CG_DrawStringExt( 320 -w / 2, cg_crosshairNamesY.integer, name, color, qfalse, qfalse,
		       	char_width,
		       	char_height, 0 );
	if (cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1 && cg_crosshairNamesHealth.integer) {
		if (cgs.clientinfo[cg.crosshairClientNum].team == cg.snap->ps.persistant[PERS_TEAM]) {
			int health = cgs.clientinfo[cg.crosshairClientNum].health;
			int armor = cgs.clientinfo[cg.crosshairClientNum].armor;
			vec4_t *healthcolor;
			w = 8 * char_width;
			if (health >= 100) {
				healthcolor = &colorGreen;
			} else if (health >= 50) {
				healthcolor = &colorYellow;
			} else {
				healthcolor = &colorRed;
			}
			CG_DrawStringExt( 320 - 3 * char_width - char_width/2.5,
				       	cg_crosshairNamesY.integer + (char_height*1.20),
				       	va("%3i", cgs.clientinfo[cg.crosshairClientNum].health),
					*healthcolor,
				       	qfalse, qfalse,
					char_width,
					char_height,
				       	0 );
			CG_DrawStringExt( 320 - 1 * char_width/2.0,
				       	cg_crosshairNamesY.integer + (char_height*1.20),
					"|",
					colorWhite, qfalse, qfalse,
					char_width,
					char_height,
				       	0 );
			if (armor >= 100) {
				healthcolor = &colorGreen;
			} else if (armor >= 50) {
				healthcolor = &colorYellow;
			} else {
				healthcolor = &colorRed;
			}
			CG_DrawStringExt( 320 + char_width/2.5,
				       	cg_crosshairNamesY.integer + (char_height*1.20),
				       	va("%i", cgs.clientinfo[cg.crosshairClientNum].armor),
					*healthcolor,
				       	qfalse, qfalse,
					char_width,
					char_height,
					0 );

		}
	}
#endif
	trap_R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {
	if (cg.scoreBoardShowing) {
		return;
	}
	if (cg.spectatorHelpDrawTime == 0) {
		cg.spectatorHelpDrawTime = cg.time;
	}
	CG_DrawBigString(320 - 9 * 8, 440, "SPECTATOR", 1.0F);
	if (cg.time <= cg.spectatorHelpDrawTime + 40000) {
		char *s = va("Try %s\\help%s and %s\\doc%s for game configuration docs",
				S_COLOR_GREEN, S_COLOR_WHITE, S_COLOR_GREEN, S_COLOR_WHITE);
		CG_DrawSmallScoreString(320 - (CG_DrawStrlen(s)*SCORESMALLCHAR_WIDTH)/2, 460, s, 1.0F);
	} else {
		if ( cgs.gametype == GT_TOURNAMENT ) {
			CG_DrawSmallScoreString(320 - (15 * SCORESMALLCHAR_WIDTH)/2, 460, "waiting to play", 1.0F);
		}
		else if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1) {
			CG_DrawSmallScoreString(320 - (39 * SCORESMALLCHAR_WIDTH)/2, 460, "press ESC and use the JOIN menu to play", 1.0F);
		}
	}
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	char	*s;
	int		sec;

	if ( !cgs.voteTime ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
#ifdef MISSIONPACK
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo);
	CG_DrawSmallString( 0, 58, s, 1.0F );
	s = "or press ESC then click Vote";
	CG_DrawSmallString( 0, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0F );
#else
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo );
	//CG_DrawSmallString( 0, 58, s, 1.0F );
	CG_DrawSmallString( SCREEN_WIDTH/2 - (CG_DrawStrlen(s)*SMALLCHAR_WIDTH)/2, 10, s, 1.0F );
#endif
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;

	if ( cgs.clientinfo[cg.clientNum].team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo[cg.clientNum].team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
							cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	CG_DrawSmallString( 0, 90, s, 1.0F );
}

static qboolean CG_DrawThawing(void) {
        char *s;
        int w;
	unsigned int frozenState = cg.snap->ps.stats[STAT_FROZENSTATE];
	float thawFrac;
	float color[4];
	float width, height, x, y;

	if (!(frozenState & FROZENSTATE_FROZEN)) {
		return qfalse;
	}

	thawFrac = (float)(frozenState >> 2)/0xff;
	s = va("Thawing: %i%%", (int)(thawFrac * 100));
	w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallStringColor(320-w/2,350, s, colorYellow);

	color[3] = 1.0;
	if (frozenState & FROZENSTATE_THAWING) {
		color[0] = 0.0;
		color[1] = 1.0;
		color[2] = 0.0;
	} else {
		color[0] = 0.0;
		color[1] = 0.31;
		color[2] = 1.0;
	}

	height = 10;
	width = 180;
	x = 320 - width/2;
	y = 335;
	CG_FillRect(x,y, thawFrac*width, height, color);
	CG_DrawRect(x,y, width, height, 1, color);

	return qtrue;
}


static qboolean CG_DrawScoreboard( void ) {
#ifdef MISSIONPACK
	static qboolean firstTime = qtrue;
	float fade, *fadeColor;

	if (menuScoreboard) {
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}
	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// should never happen in Team Arena
	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			firstTime = qtrue;
			return qfalse;
		}
		fade = *fadeColor;
	}																					  


	if (menuScoreboard == NULL) {
		if ( cgs.gametype >= GT_TEAM && cgs.ffa_gt!=1) {
			menuScoreboard = Menus_FindByName("teamscore_menu");
		} else {
			menuScoreboard = Menus_FindByName("score_menu");
		}
	}

	if (menuScoreboard) {
		if (firstTime) {
			CG_SetScoreSelection(menuScoreboard);
			firstTime = qfalse;
		}
		Menu_Paint(menuScoreboard, qtrue);
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
#else
        char        *s;
        int w;
	if (!CG_DrawThawing() && cg.respawnTime && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR 
			&& (cgs.gametype < GT_CTF_ELIMINATION || cgs.gametype > GT_LMS) ) {
		if (cgs.gametype == GT_ELIMINATION) {
			if (cg.respawnTime > 0 && cg.respawnTime > cg.time) {
				s = va("Respawn in: %2.2f",((double)cg.respawnTime-(double)cg.time)/1000.0);
				w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
				CG_DrawSmallStringColor(320-w/2,350, s, colorYellow);
			}
		} else if (cg.respawnTime>cg.time) {
			s = va("Respawn in: %2.2f",((double)cg.respawnTime-(double)cg.time)/1000.0);
			w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
			CG_DrawSmallStringColor(320-w/2,400, s, colorYellow);
		} else {
			s = va("Click FIRE to respawn");
			w = CG_DrawStrlen( s ) * SMALLCHAR_WIDTH;
			CG_DrawSmallStringColor(320-w/2,400, "Click FIRE to respawn", colorGreen);
		}
        }
	if (cg_ratScoreboard.integer) {
		return CG_DrawRatScoreboard();
	} else {
		return CG_DrawOldScoreboard();
	}
#endif
}

#define ACCBOARD_XPOS 500
#define ACCBOARD_YPOS 150
#define ACCBOARD_HEIGHT 20
#define ACCBOARD_WIDTH 75
#define ACCITEM_SIZE 16

qboolean CG_DrawAccboard( void ) {
        int counter, i;

        i = 0;

        if( !cg.showAcc ){
                return qfalse;
        }
        trap_R_SetColor( colorWhite );

        for( counter = 0; counter < WP_NUM_WEAPONS ; counter++ ){
                if( cg_weapons[counter+2].weaponIcon && counter != WP_PROX_LAUNCHER && counter != WP_GRAPPLING_HOOK )
                        i++;
        }

        CG_DrawTeamBackground( ACCBOARD_XPOS, ACCBOARD_YPOS, ACCBOARD_WIDTH, ACCBOARD_HEIGHT*(i + 1), 0.33f, TEAM_BLUE );

        i = 0;

        for( counter = 0 ; counter < WP_NUM_WEAPONS ; counter++ ){
                if( cg_weapons[counter+2].weaponIcon && counter != WP_PROX_LAUNCHER && counter != WP_GRAPPLING_HOOK ){
                        CG_DrawPic( ACCBOARD_XPOS + 10, ACCBOARD_YPOS + 10 +i*ACCBOARD_HEIGHT, ACCITEM_SIZE, ACCITEM_SIZE, cg_weapons[counter+2].weaponIcon );
                        if( cg.accuracys[counter][0] > 0 )
                                CG_DrawSmallStringColor(ACCBOARD_XPOS + 10 + ACCITEM_SIZE + 10, ACCBOARD_YPOS + 10 +i*ACCBOARD_HEIGHT + ACCITEM_SIZE/2 - SMALLCHAR_HEIGHT/2 ,
                                         va("%i%s",(int)(((float)cg.accuracys[counter][1]*100)/((float)(cg.accuracys[counter][0]))),"%"), colorWhite);
                        else
                                CG_DrawSmallStringColor(ACCBOARD_XPOS + 10 + ACCITEM_SIZE + 10, ACCBOARD_YPOS + 10 +i*ACCBOARD_HEIGHT + ACCITEM_SIZE/2 - SMALLCHAR_HEIGHT/2 , "-%", colorWhite);
                        i++;
                }
        }

        trap_R_SetColor(NULL);
        return qtrue;
}


void CG_DrawMessagePromptBackground(void) {
	if ( cg_newConsole.integer && (trap_Key_GetCatcher() & KEYCATCH_MESSAGE)) {
		float color[4];
		color[0] = color[1] = color[2] = 0.0;
		color[3] = 0.8;
		CG_FillRect(0, 0, 560, 18, color);
	}
}


/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
//	int key;
#ifdef MISSIONPACK
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	return;
	//}
#else
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		CG_DrawCenterString();
		return;
	}
#endif
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();

	CG_DrawMessagePromptBackground();

	if (cgs.nextmapVoteEndTime) {
		int remaining = ceil((float)(cgs.nextmapVoteEndTime - cg.time)/1000.0);
		if (remaining < 0) {
			remaining = 0;
		}
		trap_Cvar_Set("ui_nextmapvote_remaining", va("%i", remaining));
	}
}




/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
	const char	*s;
	int			w;

	//Don't report in instant gib same with RA
	if(cgs.nopickup)
		return;

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 64, s, 1.0F);
}


//#ifdef MISSIONPACK
/*
=================
CG_DrawProxWarning
=================
*/
static void CG_DrawProxWarning( void ) {
	char s [32];
	int			w;
  static int proxTime;
  static int proxCounter;
  static int proxTick;

	if( !(cg.snap->ps.eFlags & EF_TICKING ) ) {
    proxTime = 0;
		return;
	}

  if (proxTime == 0) {
    proxTime = cg.time + 5000;
    proxCounter = 5;
    proxTick = 0;
  }

  if (cg.time > proxTime) {
    proxTick = proxCounter--;
    proxTime = cg.time + 1000;
  }

  if (proxTick != 0) {
    Com_sprintf(s, sizeof(s), "INTERNAL COMBUSTION IN: %i", proxTick);
  } else {
    Com_sprintf(s, sizeof(s), "YOU HAVE BEEN MINED");
  }

	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigStringColor( 320 - w / 2, 64 + BIGCHAR_HEIGHT, s, g_color_table[ColorIndex(COLOR_RED)] );
}
//#endif


/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	int			w;
	int			sec;
	int			i;
#ifdef MISSIONPACK
	float scale;
#endif 
	clientInfo_t	*ci1, *ci2;
	int			cw;
	const char	*s;

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
		if (cgs.voteTime) {
			return;
		}
		s = "Waiting for players";		
		w = CG_DrawStrlen( s ) * MEDIUMCHAR_WIDTH;
		//CG_DrawBigString(320 - w / 2, 24, s, 1.0F);
		CG_DrawMediumString(320 - w / 2, 10, s, 0.75F);
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_TOURNAMENT) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
				if ( !ci1 ) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if ( ci1 && ci2 ) {
			s = va( "%s %svs %s", ci1->name, S_COLOR_WHITE, ci2->name );
#ifdef MISSIONPACK
			w = CG_Text_Width(s, 0.6f, 0);
			CG_Text_Paint(320 - w / 2, 60, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
			w = CG_DrawStrlen( s );
			if ( w > 640 / BIGCHAR_WIDTH ) {
				cw = 640 / w;
			} else {
				cw = BIGCHAR_WIDTH;
			}
			CG_DrawStringExt( 320 - w * cw/2, 8,s, colorWhite, 
					qfalse, qtrue, cw, (int)(cw * 1.5f), 0 );
#endif
		}
	} 
//	else {
//		if ( cgs.gametype == GT_FFA ) {
//			s = "Free For All";
//		} else if ( cgs.gametype == GT_TEAM ) {
//			s = "Team Deathmatch";
//		} else if ( cgs.gametype == GT_CTF ) {
//			s = "Capture the Flag";
//		} else if ( cgs.gametype == GT_ELIMINATION ) {
//			s = "Elimination";
//		} else if ( cgs.gametype == GT_CTF_ELIMINATION ) {
//			s = "CTF Elimination";
//		} else if ( cgs.gametype == GT_LMS ) {
//			s = "Last Man Standing";
//		} else if ( cgs.gametype == GT_DOUBLE_D ) {
//			s = "Double Domination";
//		} else if ( cgs.gametype == GT_1FCTF ) {
//			s = "One Flag CTF";
//		} else if ( cgs.gametype == GT_OBELISK ) {
//			s = "Overload";
//		} else if ( cgs.gametype == GT_HARVESTER ) {
//			s = "Harvester";
//                } else if ( cgs.gametype == GT_DOMINATION ) {
//			s = "Domination";
//		} else {
//			s = "";
//		}
//#ifdef MISSIONPACK
//		w = CG_Text_Width(s, 0.6f, 0);
//		CG_Text_Paint(320 - w / 2, 90, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
//#else
//		w = CG_DrawStrlen( s );
//		if ( w > 640 / BIGCHAR_WIDTH ) {
//			cw = 640 / w;
//		} else {
//			cw = BIGCHAR_WIDTH;
//		}
//		CG_DrawStringExt( 320 - w * cw/2, 8,s, colorWhite, 
//				qfalse, qtrue, cw, (int)(cw * 1.1f), 0 );
//#endif
//	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}
	s = va( "Starts in: %i", sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;
		switch ( sec ) {
		case 0:
			trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
			break;
		case 1:
			trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
			break;
		case 2:
			trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
			break;
		default:
			break;
		}
	}
#ifdef MISSIONPACK
	scale = 0.45f;
#endif
	switch ( cg.warmupCount ) {
	case 0:
		cw = 28;
#ifdef MISSIONPACK
		scale = 0.54f;
#endif
		break;
	case 1:
		cw = 24;
#ifdef MISSIONPACK
		scale = 0.51f;
#endif
		break;
	case 2:
		cw = 20;
#ifdef MISSIONPACK
		scale = 0.48f;
#endif
		break;
	default:
		cw = 16;
#ifdef MISSIONPACK
		scale = 0.45f;
#endif
		break;
	}

#ifdef MISSIONPACK
		w = CG_Text_Width(s, scale, 0);
		CG_Text_Paint(320 - w / 2, 125, scale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
	w = CG_DrawStrlen( s );
	CG_DrawStringExt( 320 - w * cw/2, 90, s, colorWhite, 
			qfalse, qtrue, cw, (int)(cw * 1.5), 0 );
#endif
}

//==================================================================================
#ifdef MISSIONPACK
/* 
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus( void ) {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}
#endif

void CG_DrawEngineSupport(void) {
	if (!CG_SupportsOggVorbis()) {
		static int drawEngineMessageTime = 0;
		const char *s = "WARNING: Engine lacks Ogg Vorbis support, some sounds might not play!";
		float char_h = 12;
		float char_w = CG_HeightToWidth(6);
		float color[4] = { 1.0, 1.0, 0.0, 1.0 };

		if (drawEngineMessageTime == 0) {
			drawEngineMessageTime = cg.time;
		} else if (drawEngineMessageTime + 60 * 1000 < cg.time) {
			return;
		}
		CG_DrawStringExt( SCREEN_WIDTH * 0.5 - (CG_DrawStrlen(s) * char_w)/2.0,
			       	1,
			       	s,
			       	color,
			       	qtrue,
			       	qfalse,
			       	char_w,
				char_h,
				0);
	}
}


/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(stereoFrame_t stereoFrame)
{
#ifdef MISSIONPACK
	if (cgs.orderPending && cg.time > cgs.orderTime) {
		CG_CheckOrderPending();
	}
#endif
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}

	CG_DrawEngineSupport();

	if (cg_newConsole.integer) {
		float consoleSizeY = CG_ConsoleAdjustSizeY(cg_consoleSizeY.value);
		float consoleSizeX = CG_ConsoleAdjustSizeX(cg_consoleSizeX.value);
		float chatSizeY = CG_ConsoleAdjustSizeY(cg_chatSizeY.value);
		float chatSizeX = CG_ConsoleAdjustSizeX(cg_chatSizeX.value);
		float teamChatSizeY = CG_ConsoleAdjustSizeY(cg_teamChatSizeY.value);
		float teamChatSizeX = CG_ConsoleAdjustSizeX(cg_teamChatSizeX.value);

		int consoleLines = CG_GetChatHeight(cg_consoleLines.integer);
		int commonConsoleLines = CG_GetChatHeight(cg_commonConsoleLines.integer);
		int chatLines = CG_GetChatHeight(cg_chatLines.integer);
		int teamChatLines = CG_GetChatHeight(cg_teamChatLines.integer);

		int lowestChatPos = CG_ConsoleChatPositionY(consoleSizeY, chatSizeY) + chatLines * chatSizeY;
		float f;

		if (lowestChatPos > RATSB_HEADER-2) {
			f = (RATSB_HEADER-2.0)/lowestChatPos;
			consoleSizeX *= f;
			consoleSizeY *= f;
			chatSizeX *= f;
			chatSizeY *= f;
			teamChatSizeX *= f;
			teamChatSizeY *= f;
		}
		f = cg_fontScale.value;
		consoleSizeX *= f;
		consoleSizeY *= f;
		chatSizeX *= f;
		chatSizeY *= f;
		teamChatSizeX *= f;
		teamChatSizeY *= f;

		if ( cg.snap->ps.pm_type == PM_INTERMISSION || cg_commonConsole.integer ||
				((cgs.gametype == GT_ELIMINATION || cgs.gametype == GT_CTF_ELIMINATION) && cg.warmup != -1 && cg.time < cgs.roundStartTime)) {
			CG_DrawGenericConsole(&cgs.commonConsole, commonConsoleLines, cg_chatTime.integer, 
					0, 0, 
					chatSizeX,
					chatSizeY
					);
		} else {
			CG_DrawGenericConsole(&cgs.console, consoleLines, cg_consoleTime.integer, 
					0, 0, 
					consoleSizeX,
					consoleSizeY
					);
			CG_DrawGenericConsole(&cgs.chat, chatLines, cg_chatTime.integer, 
					0, 
					CG_ConsoleChatPositionY(consoleSizeY, chatSizeY),
					chatSizeX,
					chatSizeY
					);

			CG_DrawGenericConsole(&cgs.teamChat, teamChatLines, cg_teamChatTime.integer, 
					0, 
					cg_teamChatY.integer - teamChatLines*teamChatSizeY,
					teamChatSizeX,
					teamChatSizeY
				       	);
		}


	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}


/*
	if (cg.cameraMode) {
		return;
	}
*/
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR /*|| cg.snap->ps.pm_type == PM_SPECTATOR*/ ) {
		CG_DrawSpectator();

		if(stereoFrame == STEREO_CENTER)
			CG_DrawCrosshair();

		CG_DrawReloadIndicator();
		CG_DrawEmptyIndicator();
		CG_DrawCrosshairNames();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {
			CG_DrawZoomScope();
			if ((cg_ratStatusbar.integer >= 4 && cg_ratStatusbar.integer <= 5) && cgs.gametype != GT_HARVESTER && cgs.gametype != GT_TREASURE_HUNTER) {
				CG_DrawRatStatusBar4Bg();
			}
			CG_DrawHudDamageIndicator();
			CG_DrawMovementKeys();

#ifdef MISSIONPACK
			if ( cg_drawStatus.integer ) {
				Menu_PaintAll();
				CG_DrawTimedMenus();
			}
#else
			if (cg_ratStatusbar.integer && cgs.gametype != GT_HARVESTER && cgs.gametype != GT_TREASURE_HUNTER) {
				switch (cg_ratStatusbar.integer) {
					case 4:
					case 5:
						CG_DrawRatStatusBar4();
						break;
					case 1:
					case 2:
						CG_DrawRatStatusBar();
						break;
					case 3:
						CG_DrawRatStatusBar3();
						break;
					default:
						CG_DrawRatStatusBar();
						break;
				}
			} else {
				CG_DrawStatusBar();
			}
#endif
     			CG_DrawThawing(); 

			CG_DrawAmmoWarning();

			CG_DrawProxWarning();
			if(stereoFrame == STEREO_CENTER)
				CG_DrawCrosshair();
			CG_DrawReloadIndicator();
			CG_DrawEmptyIndicator();
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();

                        #ifndef MISSIONPACK
			CG_DrawHoldableItem();
			CG_DrawPersistantPowerup();
			#endif

			if (cg_drawRewards.integer) {
				if (cg_drawRewards.integer == 2) {
					CG_DrawReward2();
				} else {
					CG_DrawReward();
				}
			}
		}
    
	}

#ifndef MISSIONPACK
	CG_DrawTeamChat();
#endif


	CG_DrawVote();
	CG_DrawTeamVote();

	CG_DrawLagometer();
	if (cg_drawFPS.integer == 2) {
		CG_DrawFPS(0);
	} else if (cg_drawFPS.integer == 3) {
		CG_DrawBottomFPS();
	}

#ifdef MISSIONPACK
	if (!cg_paused.integer) {
		CG_DrawUpperRight(stereoFrame);
	}
#else
	CG_DrawUpperRight(stereoFrame);
#endif

#ifndef MISSIONPACK
	CG_DrawLowerRight();
	CG_DrawLowerLeft();
#endif

	//if ( !CG_DrawFollow() ) {
	//	CG_DrawWarmup();
	//}
	
	CG_DrawFollow();
	CG_DrawWarmup();

	if (cg_radar.integer == 1) {
		CG_DrawRadar(0);
	}

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if ( !cg.scoreBoardShowing) {
		CG_DrawEliminationStatus();
		CG_DrawTreasureHunterStatus();
                CG_DrawCenterDDString();
                CG_DrawCenter1FctfString();
		CG_DrawCenterString();


		if ( cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR )
			CG_DrawReady();

		CG_DrawQueue();
	}

        cg.accBoardShowing = CG_DrawAccboard();

	CG_DrawMessagePromptBackground();
}


static void CG_DrawTourneyScoreboard( void ) {
#ifdef MISSIONPACK
#else
	CG_DrawOldTourneyScoreboard();
#endif
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		CG_DrawTourneyScoreboard();
		return;
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	if(stereoView != STEREO_CENTER)
		CG_DrawCrosshair3D();

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// draw status bar and other floating elements
 	CG_Draw2D(stereoView);
}



