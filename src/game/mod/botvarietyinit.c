#include <ultra64.h>
#include "constants.h"
#include "game/mod/botvariety.h"
#include "game/modelmgr.h"
#include "game/mplayer/mplayer.h"
#include "game/title.h"
#include "bss.h"

/// <summary>
/// Call in StartMatch to init match data before chrs are loaded.
/// </summary>
void bvInitMatch() {
	u8 i;

	// Clear player data
	for (i = 0; i < MAX_PLAYERS; i++) {
		g_BvMatch.players[i].initscale = -1.0f;
	}

	// Clear bot data
	for (i = 0; i < MAX_BOTS; i++) {
		g_BvMatch.bots[i].initscale = -1.0f;
		g_BvMatch.bots[i].initmodel = NULL;
	}
}

/// <summary>
/// Call to initialize a chr's initial scale/model data if it hasn't already been.
/// </summary>
void bvTryInitChr(struct chrdata* chr, bool iscurrentplayer) {
	if (iscurrentplayer && g_BvMatch.players[g_Vars.currentplayernum].initscale <= 0) {
		g_BvMatch.players[g_Vars.currentplayernum].initscale = g_Vars.currentplayer->model00d4->scale;
	}
	else if (chr->aibot && g_BvMatch.bots[chr->aibot->aibotnum].initscale <= 0) {
		g_BvMatch.bots[chr->aibot->aibotnum].initscale = chr->model->scale;
		g_BvMatch.bots[chr->aibot->aibotnum].initmodel = chr->model;
	}
}


/// <summary>
/// Clears out any remaining data set aside for variants
/// </summary>
void bvEndMatch() {
	u8 i;
	struct model* initmodel;

	/*
	// Clear out any remaining models left over from impostors
	for (i = 0; i < MAX_BOTS; i++) {
		initmodel = g_BvMatch.bots[i].initmodel;
		if (initmodel) {
			modelmgrFreeModel(initmodel);
			initmodel = NULL;
		}
	}
	*/
}

