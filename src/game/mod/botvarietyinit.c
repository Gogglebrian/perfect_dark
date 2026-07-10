#include <ultra64.h>
#include "constants.h"
#include "game/mod/botvariety.h"
#include "game/modelmgr.h"
#include "game/mplayer/mplayer.h"
#include "game/title.h"
#include "bss.h"
#include "lib/memp.h"

// **** Botvariety initialization/allocation overview *****
// Overview of this file's functions, in roughly chronological order:
// - bvInit
//   + Called on game init.
//   + Initializes global vars that persist across rounds, such as spree cooldowns.
// - bvInitMatch
//   + Called from mpStartMatch - before the new stage is loaded, and thus before chrs are allocated.
//   + Initializes match data, such as nulling pointers and zeroing spree data.
// - bvAllocateChrData
//   + Called from botmgrAllocate or playerTickChrBody after a chr is allocated. 
//    (For reference, the order of allocation is all bots (in setupCreateProps, called from lvReset) then all players (in a player loop in farther along in lvReset).)
//   + Allocates bvchrdata, bvbotdata for bots, and slenderman victim data.
//   + Sets pointers and bot or player index (bvindex).
// - bvTryInitChrForFirstSpawn
//   + Called from bvspawnPrepVariety every time a chr is spawned, but only does its work on the first spawn, and only returns false on subsequent spawns.
//   + Sets aside initial model scale for the match.
//   + Populates remaining pointers to other chrs or their data that couldn't be set at allocation time.
// - bvResetChrDataForSpawn
//   + Called from bvspawnPrepVariety on all but the first spawn.
//   + Resets CHR_BV_FLAGS
//   + Clears extra data for Slenderman or Explosive bot features as appropriate
// - bvEndMatch
//   + Called from mpEndMatch
//   + Doesn't do anything but might someday

/**
* Call on game init to initialize global vars that persist across rounds.
*/
void bvInit() {
	for (u8 i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		g_BvSpreeCooldowns[i] = 0;
	}
}

/**
* Call from StartMatch (before new stage is loaded and chrs are allocated) to initialize match data, such as nulling pointers and zeroing spree data.
*/
void bvInitMatch() {
	u8 i;

	// Clear bv chr data pointers
	for (i = 0; i < MAX_PLAYERS + MAX_BOTS; i++) {
		g_BvMatch.allchrs[i] = NULL;
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		g_BvMatch.players[i] = NULL;
	}
	for (i = 0; i < MAX_BOTS; i++) {
		g_BvMatch.bots[i] = NULL;
	}

	// Clear spree data
	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		g_BvMatch.variantspreespawnsleft[i] = 0;
	}
}

/**
* Call when chr is allocated to allocate bvchrdata and set pointers and indexes.
*
* Two things to bear in mind:
*  - All bots' chrs are allocated before all players' chrs; therefore, players can safely access bots' chr/bvchrdata from within this func, but not vice versa.
*  - g_MpNumChrs and g_BotCount are incremented AS bots are allocated, not before.
*
* Preconditions:
*  - (Bots) aibot and aibot->aibotnum set
*  - (Bots) g_MpNumChrs has been incremented for this bot, but not yet for the next bot (if any)
*/
void bvAllocateChrData(struct chrdata* chr) {
	struct bvchrdata* bvchr = mempAlloc(sizeof(struct bvchrdata), MEMPOOL_STAGE);
	u8 i;

	if (!chr->aibot) { // player
		g_BvMatch.players[g_Vars.currentplayerindex] = bvchr;
		chr->bvindex = g_Vars.currentplayerindex;
		chr->bvbot = NULL;
		bvchr->bvbot = NULL;
		bvchr->player = g_Vars.currentplayer;
	}
	else { // bot
		g_BvMatch.bots[chr->aibot->aibotnum] = bvchr;
		chr->bvindex = chr->aibot->aibotnum;
		chr->bvbot = mempAlloc(sizeof(struct bvbotdata), MEMPOOL_STAGE);
		bvchr->bvbot = chr->bvbot;
		bvchr->player = NULL;

		// init/allocate bot-specific data
		chr->bvbot->chr = chr;
		chr->bvbot->bvchr = bvchr;
		chr->bvbot->initmodel = chr->model;
		chr->bvbot->impostorof = -1;
		chr->bvbot->explosiveglowweight = 0;
		chr->bvbot->explosivetimer = 0;
		chr->bvbot->explosivebeepdone = false;
		chr->bvbot->slenderspeedmult = 1.0f;
		for (i = 0; i < MAX_BOTS + MAX_PLAYERS; i++) {
			bvchr->bvbot->slendervics[i] = NULL; // These pointers will need to be initialized AFTER all chrs are allocated, like on first spawn
		}
	}

	// allocate/init this chr's slenderman-victim status as to each bot in the round (skipping self)
	for (i = 0; i < MAX_BOTS; i++) {
		bvchr->slendervicstatus[i] = NULL;
		if (g_MpSetup.chrslots & (1 << (i + MAX_PLAYERS))) { // if bot in slot
			if (chr->aibot && chr->aibot->aibotnum == i) { // skip over if we are the bot in that slot
				continue;
			}
			struct bvslendervictimstatus* vicstatus = mempAlloc(sizeof(struct bvslendervictimstatus), MEMPOOL_STAGE);
			vicstatus->slenderchr = NULL; // This pointer will need to be initialized AFTER all chrs are allocated, like on first spawn
			vicstatus->victimchr = chr;
			vicstatus->exposure = 0;
			vicstatus->dist = -1.0f;
			vicstatus->visibility = 0;
			vicstatus->aggro = 0;
			vicstatus->totalinsighttime = 0;
			vicstatus->onscreen = false;
			vicstatus->haslos = false;
			vicstatus->istarget = false;
			vicstatus->slenderdead = false;
			bvchr->slendervicstatus[i] = vicstatus;
		}
	}

	bvchr->flags = 0;
	bvchr->initscale = -1.0f;
	bvchr->spawntime = 0;
	bvchr->chr = chr;

	chr->bvchr = bvchr;
	g_BvMatch.allchrs[chr->mpindex] = bvchr;
}

/**
* Call on spawn to reset botvariety chr data that are relevant to a single spawn/life, while preserving match-wide values like initial scale and model.
*/
void bvResetChrDataForSpawn(struct chrdata* chr) {
	if (chr->bvbot && CHR_BV_FLAGS & BVFLAG_SLENDERMAN) {
		bvslenderDespawn(chr); // if we were slenderman last spawn, clear some of that status data pertaining to other chrs we may have victimized
	}

	CHR_BV_FLAGS = 0;
	chr->bvchr->spawntime = 0;

	if (chr->bvbot) {
		//bvchr->bvbotimpostorof intentionally omitted; it should be unset when the model is reverted.
		bvexplosiveResetDataForSpawn(chr);
	}
	bvslenderResetVictimDataForSpawn(chr);
}

/**
* Call on spawn to try to initialize any bvchr data that wasn't done on allocation, if it hasn't already been:
* - set aside initial model scale for the match
* - populate remaining slenderman data pointers
*
* Returns true if values were initialized, false if they already had been.
*/
bool bvTryInitChrForFirstSpawn(struct chrdata* chr, bool iscurrentplayer) {
	u8 i;

	// uninitialized
	if (chr->bvchr->initscale <= 0) {
		if (iscurrentplayer) {
			chr->bvchr->initscale = g_Vars.currentplayer->model00d4->scale;
		}
		else if (chr->aibot) {
			chr->bvchr->initscale = chr->model->scale;

			// set pointers to other chr's victimstatus pertaining to this bot's Slenderman behaviour
			for (i = 0; i < g_MpNumChrs; i++) {
				chr->bvbot->slendervics[i] = mpGetChrFromPlayerIndex(i)->bvchr->slendervicstatus[chr->bvindex];
			}
		}

		// set pointers to slendermen now that all chrs are allocated
		for (i = 0; i < MAX_BOTS; i++) {
			if (chr->bvchr->slendervicstatus[i]) {
				chr->bvchr->slendervicstatus[i]->slenderchr = g_BvMatch.bots[i]->chr;
			}
		}

		return true;
	}
	// was already initialized
	return false;
}

/**
* Stub, don't actually need this yet, maybe someday
*/
void bvEndMatch() {
	;
}
