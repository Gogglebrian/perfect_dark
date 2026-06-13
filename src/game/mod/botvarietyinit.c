#include <ultra64.h>
#include "constants.h"
#include "game/mod/botvariety.h"
#include "game/modelmgr.h"
#include "game/mplayer/mplayer.h"
#include "game/title.h"
#include "bss.h"

/// <summary>
/// Clears botvariety chr data to default values
/// </summary>
void bvResetChrData(struct bvchrdata * bvchr) {
	bvchr->initscale = -1.0f;
	bvchr->initmodel = NULL;
	bvchr->impostorof = -1;
}

/// <summary>
/// Call in StartMatch to init match data before chrs are loaded.
/// </summary>
void bvInitMatch() {
	u8 i;

	// Clear player data
	for (i = 0; i < MAX_PLAYERS; i++) {
		bvResetChrData(&g_BvMatch.players[i]);
	}

	// Clear bot data
	for (i = 0; i < MAX_BOTS; i++) {
		bvResetChrData(&g_BvMatch.bots[i]);
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
/// Stub, don't actually need this yet, maybe someday
/// </summary>
void bvEndMatch() {
	;
}
