#include "ui_local.h"


static void UI_ConfirmTrackStats_Draw(void) {
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 0, "This gives servers permission to publish your", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 1, "player name & game data to a stats website.", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 2, "Note: This setting can be changed", UI_CENTER|UI_SMALLFONT, color_white );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 3, "at any time in SETUP.", UI_CENTER|UI_SMALLFONT, color_white );
}

void UI_TrackConsentAction( qboolean result ) {
	trap_Cvar_SetValue("ui_trackConsentConfigured", 1);

	if (result) {
		trap_Cvar_SetValue("cg_trackConsent", 1);
	} else {
		trap_Cvar_SetValue("cg_trackConsent", 0);
	}
}


void UI_TrackConsentMenu( void (*action)( qboolean result ) ) {
	UI_ConfirmMenu_Style2(
			"Track me",
			"on Ratstats?",
			qtrue,
			UI_CENTER|UI_SMALLFONT,
			UI_ConfirmTrackStats_Draw,
			action);
}
