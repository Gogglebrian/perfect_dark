#include <ultra64.h>
#include "constants.h"
#include "game/mod/botvariety.h"
#include "game/modelmgr.h"
#include "game/mplayer/mplayer.h"
#include "game/title.h"
#include "bss.h"

/**
* Call on game init to initialize global vars that persist across rounds.
*/
void bvInit() {
	for (u8 i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		g_BvSpreeCooldowns[i] = 0;
	}
}

/**
* Clears botvariety chr data to default values
*/
void bvResetChrData(struct bvchrdata * bvchr) {
	bvchr->initscale = -1.0f;
	bvchr->initmodel = NULL;
	bvchr->impostorof = -1;
	bvchr->explosiveglowweight = 0;
	bvchr->explosivetimer = 0;
	bvchr->explosivebeepdone = 0;
}

/**
* Call in StartMatch to init match data before chrs are loaded.
*/
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

	// Clear spree data
	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		g_BvMatch.variantspreespawnsleft[i] = 0;
	}
}

/**
* Call to initialize a chr's initial scale/model data if it hasn't already been.
* Returns true if values were initialized, false if they already had been.
*/
bool bvTryInitChr(struct chrdata* chr, bool iscurrentplayer) {
	if (iscurrentplayer && g_BvMatch.players[g_Vars.currentplayernum].initscale <= 0) {
		g_BvMatch.players[g_Vars.currentplayernum].initscale = g_Vars.currentplayer->model00d4->scale;
		return true;
	}
	else if (chr->aibot && g_BvMatch.bots[chr->aibot->aibotnum].initscale <= 0) {
		g_BvMatch.bots[chr->aibot->aibotnum].initscale = chr->model->scale;
		g_BvMatch.bots[chr->aibot->aibotnum].initmodel = chr->model;
		return true;
	}
	
	return false;
}

/**
* Stub, don't actually need this yet, maybe someday
*/
void bvEndMatch() {
	;
}
