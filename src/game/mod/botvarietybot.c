#include <ultra64.h>
#include "constants.h"
#include "game/bot.h"
#include "game/chr.h"
#include "game/game_0b0fd0.h"
#include "game/game_006900.h"
#include "game/mod/botvariety.h"
#include "game/mplayer/mplayer.h"
#include "game/propobj.h"
#include "bss.h"
#include "lib/ailist.h"
#include "lib/mtx.h"
#include "lib/rng.h"

// this file includes botvariety system functions that are specific to bots, not players.

void bvTickChrAliveUnpaused(struct chrdata* chr); // declared here but defined in botvariety.c

/**
* Checks if this chr can use this weapon/func based on their botvariety flags.
* Default true
*/
bool bvbotCanUseWeapon(struct chrdata* chr, s32 weaponnum, s32 funcnum) {
	// Explosive bots can only punch
	if (bvIsChrExplosive(chr) && weaponnum != WEAPON_UNARMED) {
		return false;
	}
	// Slenderman can only punch, not disarm
	else if (bvIsChrSlenderman(chr) && (weaponnum != WEAPON_UNARMED || funcnum != FUNC_PRIMARY)) {
		return false;
	}

	return true;
}

/**
* Checks if this chr can pick up this weapon based on their botvariety flags.
* Default true
*/
bool bvbotCanPickupWeapon(struct chrdata* chr, s32 weaponnum) {
	// Explosive and Slenderman bots ignore weapon pickups
	if (bvIsChrExplosive(chr) || bvIsChrSlenderman(chr)) {
		return false;
	}

	return true;
}

/**
* Checks if this chr can attack right now based on their botvariety flags and mechanics.
* Default true
*/
bool bvbotCanAttack(struct chrdata* chr) {
	// slenderman can't attack while he's stalking
	if (bvIsChrSlenderman(chr)) {
		return bvslendermanCanAttack();
	}

	return true;
}

/**
* Can this bot see this other chr? (in other words, is the otherchr visible/invisible to this bot?)
* Default true
*/
bool bvbotCanSeeChr(struct chrdata* botchr, struct chrdata* otherchr) {
	if (bvIsChrSlenderman(otherchr) && !bvslendermanIsVisibleToChr(botchr)){
		return false;
	}

	return true;
}

/**
* Checks if this chr should stand still right now based on their botvariety flags and mechanics.
* Default false
*/
bool bvbotShouldStandStill(struct chrdata* chr) {
	// slenderman has to stop and stare sometimes.
	if (bvIsChrSlenderman(chr) && g_BvMatch.slendermanspeedmult == 0) {
		return true;
	}

	return false;
}

/**
* Should this bot recalculate the distance to its general target every frame, or defer to the normal behavior?
* Default false
*/
bool bvbotShouldCalcTargetDistEveryFrame(struct chrdata* chr) {
	// Explosive bots want to update the distance to the target continuously
	if (bvIsChrExplosive(chr)) {
		return true;
	}
	// Slenderman's dist-based exposure should be as precise as possible
	if (bvIsChrSlenderman(chr)) {
		return true;
	}

	return false;
}

/**
* Should this bot recalculate the LoS on its general target every frame, or defer to the normal behavior?
* Default false
*/
bool bvbotShouldCalcTargetLoSEveryFrame(struct chrdata* chr) {
	// Slenderman's los-based exposure benefits from continuous LoS updates
	if (bvIsChrSlenderman(chr)) {
		return true;
	}

	return false;
}

/**
* Should this bot change its general target if it gets LoS on another valid target first?
* Default true
*/
bool bvbotShouldChangeTargetByLoS(struct chrdata* chr) {
	// not using this for slenderman as originally planned, but leaving it here for future use
	return true;
}

/**
* Can this bot see through cloaks?
* Default false
*/
bool bvbotCanSeeThroughCloak(struct chrdata* chr) {
	// Slenderman can see through cloaks so that he'll doggedly attmept to stare at his target
	if (bvIsChrSlenderman(chr)) {
		return true;
	}
	
	return false;
}

/**
* Determines a bot's crouch position with regard to its applicable botvariety flags, if the botvariety system is active.
* Returns true if crouchpos was changed.
*/
bool bvbotGuessCrouchPos(struct chrdata* chr, s32* crouchpos) {
	if (!bvIsBotVarietyActive()) {
		return false;
	}

	// Mini bots never have to crouch
	if (CHR_BV_FLAGS & BVFLAG_MINI) {
		*crouchpos = CROUCHPOS_STAND;
		return true;
	}

	// Wumbo and slender bots skip middle-crouch
	if (CHR_BV_FLAGS & (BVFLAG_WUMBO | BVFLAG_SLENDERMAN)) {
		if (chr->height <= 135) {
			*crouchpos = CROUCHPOS_SQUAT;
			return true;
		}
		else {
			*crouchpos = CROUCHPOS_STAND;
			return true;
		}
	}

	return false;
}


/**
* Ticks unique bot variant behaviors to be procced at the beginning of a living bot's unpaused tick.
*/
void bvbotTickAliveUnpausedEarly(struct chrdata* chr) {
	// Tick explosive bot's flashing and beeping
	if (bvIsChrExplosive(chr)) {
		bvTickExplosiveBot(chr);
	}
	else if (bvIsChrSlenderman(chr)) {
		bvslendermanTick(chr);
	}

	bvTickChrAliveUnpaused(chr);	
}