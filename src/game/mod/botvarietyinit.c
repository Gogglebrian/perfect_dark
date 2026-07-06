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
* Call when chr is allocated to set pointers and index.
* Precondition: aibot and aibot->aibutonum
*/
void bvInitChrDataOnAllocate(struct chrdata* chr) {
	if (chr->aibot) {
		chr->bvchr = &g_BvMatch.bots[chr->aibot->aibotnum]; 
		chr->bvchr->index = chr->aibot->aibotnum;
		chr->bvchr->mpindex = -1; // will be set to its actual value on the first spawn of the match
	}
	else {
		chr->bvchr = &g_BvMatch.players[g_Vars.currentplayerindex];
		chr->bvchr->index = g_Vars.currentplayerindex;
		chr->bvchr->player = g_Vars.currentplayer;
		chr->bvchr->mpindex = g_Vars.currentplayernum;
	}
	chr->bvchr->chr = chr;
	chr->bvchr->flags = 0;
}

/**
* Call on spawn to reset botvariety chr data that are relevant to a single spawn/life, while preserving match-wide values like initial scale and model.
*/
void bvResetChrDataForSpawn(struct bvchrdata* bvchr) {
	//bvchr->impostorof intentionally omitted; it should be unset when the model is reverted.
	bvchr->flags = 0;
	bvchr->explosiveglowweight = 0;
	bvchr->explosivetimer = 0;
	bvchr->explosivebeepdone = false;
	bvchr->slendermanexposure = 0;
	bvchr->slendermanopacity = 0;
	bvchr->slendermandist = 0;
	bvchr->slendermanonscreen = false;
	bvchr->slendermanhaslos = false;
	bvchr->slendermanaggro = 0;
}

/**
* Call on match start to clear botvariety chr data to null/default values, including match-wide values like initial scale and model.
*/
void bvClearAllChrData(struct bvchrdata * bvchr) {
	//bvchr->mpindex intentionally omitted; for players it is set when chr allocated, for bots on the first spawn in bvTryInitChrForMatch
	bvchr->initscale = -1.0f;
	bvchr->initmodel = NULL;
	bvchr->impostorof = -1;
	bvResetChrDataForSpawn(bvchr);
}

/**
* Call in StartMatch to init match data before chrs are loaded.
*/
void bvInitMatch() {
	u8 i;

	// Clear player data
	for (i = 0; i < MAX_PLAYERS; i++) {
		bvClearAllChrData(&g_BvMatch.players[i]);
	}

	// Clear bot data
	for (i = 0; i < MAX_BOTS; i++) {
		bvClearAllChrData(&g_BvMatch.bots[i]);
	}

	// Clear spree data
	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		g_BvMatch.variantspreespawnsleft[i] = 0;
	}

	// Clear slenderman data
	g_BvMatch.slendermanchr = NULL;
	g_BvMatch.slendermanspeedmult = 1.0f;
}

/**
* Call to initialize a chr's initial scale/model data and mpindex if it hasn't already been.
* Returns true if values were initialized, false if they already had been.
*/
bool bvTryInitChrForMatch(struct chrdata* chr, bool iscurrentplayer) {
	if (iscurrentplayer && chr->bvchr->initscale <= 0) {
		chr->bvchr->initscale = g_Vars.currentplayer->model00d4->scale;
		return true;
	}
	else if (chr->aibot && chr->bvchr->initscale <= 0) {
		chr->bvchr->mpindex = mpPlayerGetIndex(chr);
		chr->bvchr->initscale = chr->model->scale;
		chr->bvchr->initmodel = chr->model;
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
