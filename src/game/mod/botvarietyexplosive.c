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

const u32 flashcolor_cool = 0xFFA300E1; // 255, 163, 0 (orange)
const u32 flashcolor_hot  = 0xFFF7EAE1; // 255, 247, 234 (near-white orange)
const f32 maxweight_near = 245.0f;
const f32 maxweight_far  = 10.0f;
const u32 flashbeepdistance_max = 8000.0f;
const u32 flashbeepdistance_min = 100.0f;
const f32 beepvolume = 2.5f;
const f32 beeppitch_far = 0.6f;
const f32 beeppitch_near = 1.1f;

const f32 cycletime_min = 1.0f / 10.0f; // in seconds
const f32 cycletime_max = 2.5f; // in seconds
const f32 flashheatuptime_min = cycletime_min / 3.0f; // in seconds

#define CHR_BOTVARIETY_FLAGS chr->convtalk // This u32 isn't used in combat simulator so we'll hackily borrow it

/// <summary>
/// Is this chr an Explosive variant? This summary is a completionist formality
/// </summary>
bool bvIsChrExplosive(struct chrdata* chr) {
	return chr->aibot && CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_EXPLOSIVE;
}

/// <summary>
/// Zeroes out explosive bot glow weight, timer, and beep status
/// </summary>
void bvResetExplosiveBot(struct chrdata* botchr) {
	struct bvchrdata* bvbot = &g_BvMatch.bots[botchr->aibot->aibotnum];
	bvbot->explosiveglowweight = 0;
	bvbot->explosivetimer = 0;
	bvbot->explosivebeepdone = false;
}

/// <summary>
/// Beeps
/// </summary>
void bvexplosiveDoBeep(struct chrdata* botchr, f32 beeppitch) {
	psCreate(NULL, botchr->prop, SFX_PICKUP_MINE, -1, beepvolume, 0, 0, PSTYPE_GENERAL, NULL, beeppitch, NULL, -1, -1, -1, -1);
}

/// <summary>
/// Ticks the explosive glow weight up, starting with a beep
/// </summary>
void bvexplosiveTickBeepAndHeatup(struct chrdata* botchr, struct bvchrdata* bvbot, f32 maxweight, f32 heatuptime, f32 beeppitch){
	if (!bvbot->explosivebeepdone) {
		bvexplosiveDoBeep(botchr, beeppitch);
		bvbot->explosivebeepdone = true;
	}
	if (bvbot->explosiveglowweight < maxweight) {
		bvbot->explosiveglowweight += (maxweight/heatuptime) * g_Vars.lvupdate60freal * 0.016666f;
		if (bvbot->explosiveglowweight > maxweight) {
			bvbot->explosiveglowweight = maxweight;
		}
	}
}

/// <summary>
/// Ticks the explosive glow weight down to zero if necessary
/// </summary>
void bvexplosiveTickCooldownAndWait(struct bvchrdata* bvbot, f32 maxweight, f32 heatuptime) {
	if (bvbot->explosiveglowweight > 0) {
		bvbot->explosiveglowweight -= (maxweight/heatuptime) * g_Vars.lvupdate60freal * 0.016666f;

		if (bvbot->explosiveglowweight < 0) {
			bvbot->explosiveglowweight = 0;
		}
	}
}

/// <summary>
/// Ticks an explosive bot's internal timers to manage its flashing and beeping, which gets faster as the bot gets closer to the target
/// </summary>
void bvTickExplosiveBot(struct chrdata* botchr) {
	struct bvchrdata* bvbot = &g_BvMatch.bots[botchr->aibot->aibotnum];
	f32 dist = botGetDistanceToTarget(botchr);
	bool hastarget = botchr->target != -1;
	f32 maxweight, cycletime, beeppitch;

	// No target: Quickly cool off glow
	if (!hastarget) {
		bvexplosiveTickCooldownAndWait(bvbot,maxweight_near,0.0f);
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
		maxweight = ((maxweight_far * distfrac) + (maxweight_near * (1.0f - distfrac)));
		beeppitch = ((beeppitch_far * distfrac) + (beeppitch_near * (1.0f - distfrac)));

		// Calculate and clamp the heatuptime (the brief portion of the cycletime during which the flash-glow increases)
		f32 flashheatuptime = cycletime / 4.0f;
		if (flashheatuptime < cycletime_min) {
			flashheatuptime = cycletime_min;
		}

		// first little bit of a cycle, beep and heat up
		if (bvbot->explosivetimer <= flashheatuptime) {
			bvexplosiveTickBeepAndHeatup(botchr, bvbot, maxweight, flashheatuptime, beeppitch);
		}
		// then cool off and wait for the next cycle
		else if (bvbot->explosivetimer > flashheatuptime) {
			bvexplosiveTickCooldownAndWait(bvbot, maxweight, flashheatuptime);
		}

		// increment timer
		bvbot->explosivetimer += 0.016666f * g_Vars.lvupdate60freal;
		if (bvbot->explosivetimer > cycletime) {
			bvbot->explosivetimer = 0;
			bvbot->explosivebeepdone = false;
		}
	}
}

/// <summary>
/// Applies a glow to the bot's renderdata based on its explosiveglowweight (ticked elsewhere).
/// Assumes the botchr is an explosive bot, so check first
/// </summary>
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

	if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_MINI) {
		explosiontype = EXPLOSIONTYPE_BVMINI;
	}
	else if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_WUMBO) {
		explosiontype = EXPLOSIONTYPE_BVWUMBO;
	}
	else {
		explosiontype = EXPLOSIONTYPE_ROCKET;
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
	CHR_BOTVARIETY_FLAGS &= ~BOTVARIETY_FLAG_EXPLOSIVE;
}

#undef CHR_BOTVARIETY_FLAGS
