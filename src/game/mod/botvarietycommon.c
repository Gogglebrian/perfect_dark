#include <ultra64.h>
#include "constants.h"
#include "game/mod/botvariety.h"
#include "game/mplayer/mplayer.h"
#include "bss.h"

/**
* Gets the address of the botvariety variant at the given index.
*/
const struct bvvariant* bvGetVariant(u8 index) {
	return gc_BvVariants[index];
}

/**
* Checks if the chr is the current player's chr
*/
bool bvIsChrCurrentPlayer(struct chrdata* chr) {
	return chr == g_Vars.currentplayer->prop->chr;
}

/**
* True if chr is using any of the four Bond character bodies
*/
s32 bvIsChrBond(struct chrdata* chr) {
	return (chr->bodynum <= BODY_MOORE);
}

/**
* Gets a chr model to use depending on whether the chr is the current player's, or a bot's
*/
struct model* bvGetModel(struct chrdata* chr) {
	if (bvIsChrCurrentPlayer(chr)) {
		return g_Vars.currentplayer->model00d4;
	}
	else {
		return chr->model;
	}
}

/**
* Returns the chr's botvariety data set aside for this match, like initial model and scale value.
*/
struct bvchrdata* bvGetChrMatchData(struct chrdata* chr) {
	if (bvIsChrCurrentPlayer(chr)) {
		return &g_BvMatch.players[g_Vars.currentplayerindex];
	}
	else { // bot
		return &g_BvMatch.bots[chr->aibot->aibotnum];
	}
}

