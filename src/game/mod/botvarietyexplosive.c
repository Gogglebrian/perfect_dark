#include <ultra64.h>
#include "constants.h"
#include "game/bot.h"
#include "game/game_006900.h"
#include "game/explosions.h"
#include "game/mod/botvariety.h"
#include "game/propobj.h"
#include "game/propsnd.h"
#include "game/title.h"
#include "bss.h"
#include "lib/model.h"

// Explosive bots take bonus damage from other explosions
const f32 explosivedamagetakenmult = 3.0f;

/**
* Applies a constant multiplier to boost the explosion damage taken by Explosive Bots
*/
f32 bvApplyExplosiveBotExplosionDamageMult(f32 damage) {
	damage *= explosivedamagetakenmult;
	return damage;
}

// Explosive bots beep and flash, signalling their explosive nature and proximity to their target.
// At flashbeepdistance_max or greater from their target: longest interval, dimmest flash, and deepest-pitch beep.
// At flashbeepdistance_min or lesser from their target: shortest interval, brightest flash, and highest-pitch beep.
const f32 flashbeepdistance_max = 8000.0f;
const f32 flashbeepdistance_min = 100.0f;

// The flash colour will be somewhere between these two colours, depending on the glowweight.
const u32 flashcolor_cool = 0xFFA300E1; // 255, 163, 0 (orange)
const u32 flashcolor_hot  = 0xFFF7EAE1; // 255, 247, 234 (near-white orange)

// Maximum glowweight for the flash colour to be blended with the base colour on the character model, and the hot/cool colours to be blended.
const f32 maxglowweight_near = 245.0f; // bright white-orange, high opacity flash
const f32 maxglowweight_far  = 10.0f;  // dim orange, low opacity flash

// Beep settings
const f32 beepvolume = 2.5f; // fixed value
const f32 beeppitch_far = 0.6f; // deepest pitch at the max distance
const f32 beeppitch_near = 1.1f; // highest pitch at the min distance

// Flash/beep interval
const f32 cycletime_min = 1.0f / 10.0f; // in seconds
const f32 cycletime_max = 2.5f; // in seconds
const f32 flashheatuptime_min = cycletime_min / 3.0f; // the minimum rampup time of the flash colour/opacity, in seconds

/**
* Is this chr an Explosive variant? This summary is a completionist formality
*/
bool bvIsChrExplosive(struct chrdata* chr) {
	return chr->aibot && CHR_BV_FLAGS & BVFLAG_EXPLOSIVE;
}

/**
* Zeroes out explosive bot glow weight, timer, and beep status
*/
void bvResetExplosiveBot(struct chrdata* botchr) {
	struct bvchrdata* bvbot = &g_BvMatch.bots[botchr->aibot->aibotnum];
	bvbot->explosiveglowweight = 0;
	bvbot->explosivetimer = 0;
	bvbot->explosivebeepdone = false;
}

/**
* Beeps
*/
void bvexplosiveDoBeep(struct chrdata* botchr, f32 beeppitch) {
	psCreate(NULL, botchr->prop, SFX_PICKUP_MINE, -1, beepvolume, 0, 0, PSTYPE_GENERAL, NULL, beeppitch, NULL, -1, -1, -1, -1);
}

/**
* Ticks the explosive glow weight up, starting with a beep
*/
void bvexplosiveTickBeepAndHeatup(struct chrdata* botchr, struct bvchrdata* bvbot, f32 maxglowweight, f32 heatuptime, f32 beeppitch){
	if (!bvbot->explosivebeepdone) {
		bvexplosiveDoBeep(botchr, beeppitch);
		bvbot->explosivebeepdone = true;
	}
	if (bvbot->explosiveglowweight < maxglowweight) {
		bvbot->explosiveglowweight += (maxglowweight/heatuptime) * g_Vars.lvupdate60freal * 0.016666f;
		if (bvbot->explosiveglowweight > maxglowweight) {
			bvbot->explosiveglowweight = maxglowweight;
		}
	}
}

/**
* Ticks the explosive glow weight down to zero if necessary
*/
void bvexplosiveTickCooldownAndWait(struct bvchrdata* bvbot, f32 maxglowweight, f32 heatuptime) {
	if (bvbot->explosiveglowweight > 0) {
		bvbot->explosiveglowweight -= (maxglowweight/heatuptime) * g_Vars.lvupdate60freal * 0.016666f;

		if (bvbot->explosiveglowweight < 0) {
			bvbot->explosiveglowweight = 0;
		}
	}
}

/**
* Ticks an explosive bot's internal timers to manage its flashing and beeping, which gets faster as the bot gets closer to the target
*/
void bvTickExplosiveBot(struct chrdata* botchr) {
	struct bvchrdata* bvbot = bvGetChrMatchData(botchr);
	f32 dist = botGetDistanceToTarget(botchr);
	bool hastarget = botchr->target != -1;
	f32 maxglowweight, cycletime, beeppitch;

	// No target: Quickly cool off glow
	if (!hastarget) {
		bvexplosiveTickCooldownAndWait(bvbot,maxglowweight_near,0.0f);
	}
	// Has target: Flash and beep periodically, brighter and faster as we get closer to the target
	else {
		// Get the fraction of the way the target dist is between the min flashbeep distance (fastest flashing/beeping) to the max flashbeep distance (slowest flashing/beeping)
		f32 distfrac = (dist - flashbeepdistance_min) / (flashbeepdistance_max - flashbeepdistance_min);
		
		// clamp that distance fraction to [0.0f, 1.0f]
		if (distfrac > 1.0f){
			distfrac = 1.0f;
		}
		else if (distfrac < 0) {
			distfrac = 0;
		}

		// use that fraction to lerp between the max and min cycle times, max weights, and pitches
		cycletime = ((cycletime_max * distfrac) + (cycletime_min  * (1.0f - distfrac)));
		maxglowweight = ((maxglowweight_far * distfrac) + (maxglowweight_near * (1.0f - distfrac)));
		beeppitch = ((beeppitch_far * distfrac) + (beeppitch_near * (1.0f - distfrac)));

		// Calculate and clamp the heatuptime (the brief portion of the cycletime during which the flash-glow increases)
		f32 flashheatuptime = cycletime / 4.0f;
		if (flashheatuptime < cycletime_min) {
			flashheatuptime = cycletime_min;
		}

		// first little bit of a cycle, beep and heat up
		if (bvbot->explosivetimer <= flashheatuptime) {
			bvexplosiveTickBeepAndHeatup(botchr, bvbot, maxglowweight, flashheatuptime, beeppitch);
		}
		// then cool off and wait for the next cycle
		else if (bvbot->explosivetimer > flashheatuptime) {
			bvexplosiveTickCooldownAndWait(bvbot, maxglowweight, flashheatuptime);
		}

		// increment timer
		bvbot->explosivetimer += 0.016666f * g_Vars.lvupdate60freal;
		if (bvbot->explosivetimer > cycletime) {
			bvbot->explosivetimer = 0;
			bvbot->explosivebeepdone = false;
		}
	}
}

/**
* Applies a glow to the bot's renderdata based on its explosiveglowweight (ticked elsewhere).
* Assumes the botchr is an explosive bot, so check first
*/
void bvApplyExplosiveBotGlow(struct chrdata* botchr, struct modelrenderdata* renderdata) {
	struct bvchrdata* bvbot = &g_BvMatch.bots[botchr->aibot->aibotnum];
	if (bvbot->explosiveglowweight > 0.0f) {
		u32 glowcolour = colourBlend(flashcolor_hot, flashcolor_cool, bvbot->explosiveglowweight);
		renderdata->fogcolour = colourBlend(glowcolour, renderdata->fogcolour, bvbot->explosiveglowweight); // the glow becomes more intense while the env colours get overwhelmed
	}
}


void bvExplodeBot(struct chrdata* chr, s32 killerplayernum) {
	s32 explosionplayer;
	u8 explosiontype;
	
	if (!chr->aibot){
		return;
	}

	if (CHR_BV_FLAGS & BVFLAG_MINI) {
		explosiontype = EXPLOSIONTYPE_BVMINI;
	}
	else if (CHR_BV_FLAGS & BVFLAG_WUMBO) {
		explosiontype = EXPLOSIONTYPE_BVWUMBO;
	}
	else {
		explosiontype = EXPLOSIONTYPE_BVSTANDARD;
	}

	// by default, the bot gets credit for the explosion and any kills it achieves
	explosionplayer = chr->aibot->aibotnum + getNumPlayers();

	// but if bot got killed by someone else, they get the credit
	if (killerplayernum >= 0 && killerplayernum != explosionplayer) { 
		explosionplayer = killerplayernum;
	}

	explosionCreateComplex(NULL, &chr->prop->pos, chr->prop->rooms, explosiontype, explosionplayer);
	psStopSound(chr->prop, PSTYPE_GENERAL, 0); // stop em from grunting
	if (chr->model) {
		modelSetScale(chr->model, 0.001f); // quick and easy disappear em
	}

	// remove the explosive bot flag so we don't explode again
	CHR_BV_FLAGS &= ~BVFLAG_EXPLOSIVE;
}
