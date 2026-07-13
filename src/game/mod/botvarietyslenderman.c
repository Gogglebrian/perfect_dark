#include <ultra64.h>
#include "constants.h"
#include "game/bondview.h"
#include "game/bot.h"
#include "game/chr.h"
#include "game/chraction.h"
#include "game/game_006900.h"
#include "game/game_1531a0.h"
#include "game/mod/botvariety.h"
#include "game/mplayer/mplayer.h"
#include "game/propsnd.h"
#include "game/smoke.h"
#include "game/title.h"
#include "bss.h"
#include "lib/model.h"
#include "lib/rng.h"
#include "lib/vi.h"
#include "gbiex.h"

//**** Slenderman functionality overview **********
// Spawning
// + Bots only (bvspawnPrepVariety)
// Targeting
// + Slenderman should update distance and LoS to its target every frame. (bvShouldCalcTargetDistEveryFrame, bvShouldCalcTargetLoSEveryFrame)
// + Slenderman can see through cloaks. (bvCanSeeThroughCloak)
// Attacks/Weapons
// + Slenderman can't pick up or use any weapons, nor can he disarm. (bvbotCanUseWeapon, bvCanChrPickupWeapon)
// + Slenderman can't attack a chr unless aggroed against them, either by taking damage from them, or by rushing them due to high exposure (see below).
// Exposure
// + As long as Slenderman is spawned, each chr's exposure timer for that Slenderman ticks up to (or if beyond, back down to) their Exposure Cap. (bvslenderTickVictim, bvslenderTickVictimExposure)
// + The character's Exposure Cap is dynamically adjusted based on several factors: (bvslenderGetExposureCap)
//   + First, the Exposure Cap is raised to the highest of the three values from four contributing factors:
//     + whether in live Slenderman's LoS; in turn affected by
//        + the total time that Slenderman has had line of sight on the chr since either spawned
//     + vicinity to live Slenderman (value tapers off with distance)
//     + whether Slenderman is on the player's screen (or for bots, in mutual LoS)
//   + Second, the Exposure Cap can be limited in turn by each of three limiting factors:
//     + whether Slenderman is dying or dead (limit tapers down to zero as corpse fades)
//     + whether Slenderman is a teammate
//     + if the chr is neither Slenderman's target nor is he aggroed against the chr
//   + Naturally, the Exposure Cap will be zero after Slenderman despawns, so chrs with outstanding exposure will have it (quickly) untick down to zero.
// + The practical upshot of the Exposure Cap feature is that the highest exposure thresholds (eg Rushing described below) can only be reached if Slenderman is onscreen
//   (or in mutual LoS for bots).
// + If BVVARIANT_SLENDERMAN->debug is enabled, each player's highest current exposure value will be displayed on the hud.
// Effects of Exposure
// + Players' screens will gradually fill with static as their exposure timer fills up. In case of multiple slendermen, the highest exposure valeu will be used. (bvslenderApplyVictimStatic)
// + The initially-invisible Slenderman becomes visible to his target as their exposure increases. (bvslenderTickVictim, bvslenderUpdateVisibilityForChr, bvslenderGetAlpha, chrRender)
// + Target's exposure > exposurethreshold_freezeonscreen: Slenderman can't move while on his target's screen (or in mutual LoS for other bots), until
// + Target's exposure > exposurethreshold_rush: Slenderman aggroes and rushes the target at high speed and with devastating melee damage.
// + If Slenderman is aggro'd early by taking damage, his melee damage and speed will scale up with exposure until reaching the rush threshold.
// + Exposure to one Slenderman does not affect a chr's status with any other Slendermen.
// Look and sounds
// + Slenderman's visibility will also increase for all players while aggroed or dead. (bvslenderUpdateVisibilityForChr)
// + Slenderman wears a suit unless he's also an Impostor. (bvspawnHandleSlenderman)
// + Slenderman is dark. (bvTryApplyLateColourTweaks, bvslenderApplyColour)
// + Slenderman bleeds black. (bvslenderGetBloodColours)
// + Slenderman vocalizes on death. (bvslenderDie)
// + Slenderman's corpse dissapates into a cloud of smoke. (bvslenderOnCorpseFade)
// + Slenderman doesn't appear on radar (unless he's survived for a suspiciously long time and we think he's stuck somewhere). (bvslenderIsVisibleOnRadar)

const f32 exposure_max = 20.0f; // absolute max value for exposure

// Exposure Cap, Contributing Factors:
const f32 exposurecap_onscreen = exposure_max; // Contributing factor: max level exposure from Slenderman being on player's screen (even if hidden)
const f32 exposurecap_distance = 5.0f; // Contributing factor: max level of exposure from being in slenderman's vicinity (actual value will taper off with distance) 
const f32 exposurecap_onlyhaslos = 3.5f; // Contributing factor: max level of exposure just from being in slenderman's los
const f32 distthreshold_near_maxeffect = 500.0f; // near distance at which slenderman's vicinity effects hit their maximum
const f32 distthreshold_far_zeroeffect = 1500.0f; // far distance at which slenderman's vicinity effects reach zero

// Exposure Cap, Limiting Factors:
const f32 exposurecap_teammate = 5.0f; // Limiting factor: max level of exposure for slenderman's teammates
const f32 exposurecap_dead = 5.0f; // Limiting factor: max level of exposure allowed after slenderman's dead (actual value will taper off as corpse fades)
const f32 exposurecap_nottargetoraggroed = 6.0f; // Limiting factor: max level of exposure if yer not slenderman's target nor have you attacked him

// Screen static
const f32 exposurethreshold_minstatic = 0; // seconds of exposure before the static starts
const f32 exposurethreshold_maxstatic = 10.0f; // seconds of exposure before reaching max static
const u8 maxstaticamount = 128;

// Visibility
const f32 exposurethreshold_visibilitystart = 2.5f; // seconds of exposure before slenderman begins to become visible to a chr
const f32 exposurethreshold_visibilitymax = 5.5f; // seconds of exposure before slenderman becomes fully opaque to a chr
const u8 maxvisibility = 255;
const f32 visibilityuprate_deadoraggro = 255.0f; // rate that visibility ticks up per second for all players after slenderman is dead or aggroed

// Standard aggro damage and speed will start scaling up beyond this exposure threshold (of the target's)
const f32 exposurethreshold_startscalingdamage = 5.0f;
const f32 exposurethreshold_startscalingspeed = 8.5f;

// Stalking and Rushing
const f32 exposurethreshold_freezeonscreen = 1.5f; // seconds of exposure before slenderman must freeze if onscreen
const f32 exposurethreshold_rush = 10.5f; // seconds of exposure beyond which slenderman rushes the target
const f32 distthreshold_stalking = 500.0f; // slenderman will stalk victim from this distance until they cross the exposurethreshold_rush
const f32 speedmult_rush = 2.5f;
const f32 animspeedmult_rush = 2.0f;
const f32 damagemult_rush = 2.5f;

// slenderman sounds - pitch determined by BVVARIANT_SLENDERMAN->body->voicepitch
const f32 deathcry_volume = 2.0f;
const u32 slendermandeathsounds[10] = {
	SFX_02A1,
	SFX_M0_SHE_GOT_ME,
	SFX_M0_DONT_SHOOT_ME,
	SFX_M0_YOU_WIN_I_SURRENDER,
	SFX_M1_IM_BLEEDING,
	SFX_0303,
	SFX_0300, 
	SFX_0318,
	SFX_M1_WHY_ME,
	SFX_M1_CHOKING,
};

/**
* Gets the current status of slenderchr's effects and situation relative to victimchr: 
* - exposure - victimchr's level of exposure to slenderchr
* - visibility - slenderchr's visibility (and render opacity) to victimchr 
* - dist - distance between them
* - haslos - whether slenderchr has LoS on victimchr
* - onscreen - for player victimchr, if slenderchr is onscreen; always false for bot victimchrs
* - istarget - if victimchr is slenderchr's target
* - aggro - degree to which slenderchr is aggroed against victimchr; 0 for not, 1 for basic aggro, 2 for rushing
* - slenderdead - if slenderchr is dead
*/
struct bvslendervictimstatus* bvslenderGetStatus(struct chrdata* victimchr, struct chrdata* slenderchr) {
	if (!victimchr || !slenderchr || victimchr == slenderchr) {
		return NULL;
	}
	return victimchr->bvchr->slendervicstatus[slenderchr->bvindex];
}

bool bvIsChrSlenderman(struct chrdata* chr) {
	return (chr->aibot && CHR_BV_FLAGS & BVFLAG_SLENDERMAN);
}

/**
* Are there any slendermen on the field right now?
*/
bool bvslenderIsSpawned() {
	for (u8 i = 0; i < g_BotCount; i++) {
		if (g_BvMatch.bots[i] && g_BvMatch.bots[i]->flags & BVFLAG_SLENDERMAN) {
			return true;
		}
	}
	return false;
}

/**
* Does this chr have any outstanding exposure to a slenderman?
*/
bool bvslenderHasExposure(struct chrdata* chr) {
	for (u8 i = 0; i < g_BotCount; i++) {
		if (chr->bvchr->slendervicstatus[i]
			&& chr->bvchr->slendervicstatus[i]->exposure > 0) {
			return true;
		}
	}
	return false;
}

/**
* Returns the highest value of exposure this chr has to any slenderman.
*/
f32 bvslenderGetChrHighestExposure(struct chrdata* chr) {
	f32 highest = 0;
	for (u8 i = 0; i < g_BotCount; i++) {
		if (chr->bvchr->slendervicstatus[i]
			&& chr->bvchr->slendervicstatus[i]->exposure > highest) {
			highest = chr->bvchr->slendervicstatus[i]->exposure;
		}
	}
	return highest;
}


/**
* Should this chr be ticked as a potential victim of Slenderman-related behaviors?
*/
bool bvslenderShouldDoVictimTick(struct chrdata* chr) {
	if (bvslenderIsSpawned() || bvslenderHasExposure(chr)) {
		return true;
	}

	return false;
}

/**
* Should screen static be applied to the current player's screen?
*/
bool bvslenderShouldDoStatic() {
	return (bvslenderHasExposure(g_Vars.currentplayer->prop->chr));
}

/**
* Is slenderchr visible to this chr?
* True if visibility > 0
*/
bool bvslenderIsVisibleToChr(struct chrdata* chr, struct chrdata* slender) {
	return (bvslenderGetStatus(chr, slender)->visibility > 0);
}

/**
* Does this slenderchr appear on this playerchr's radar right now?
* Answer: generally not unless they somehow survive (particularly unharmed) for a suspiciously long time
*/
bool bvslenderIsVisibleOnRadar(struct chrdata* playerchr, struct chrdata* slenderchr) {
	if ((slenderchr->damage == 0 && slenderchr->bvchr->spawntime > 60.0f) || slenderchr->bvchr->spawntime > 120.0f) { // insurance policy against invisible slenderbot getting stuck
		return true;
	}

	return false;
}

/**
* Is this Slenderman aggro against any chr right now?
*/
bool bvslenderIsAggro(struct chrdata* slenderchr) {
	u8 i;

	for (i = 0; i < g_MpNumChrs; i++) {
		if (slenderchr->bvbot->slendervics[i]
		&& slenderchr->bvbot->slendervics[i]->aggro) {
			return true;
		}
	}

	return false;
}

/**
* Checks if Slenderman can attack its target based on whether it's been damaged or if its rushing.
*/
bool bvslenderCanAttack(struct chrdata* slenderchr) {
	// can attack if rushing
	if (slenderchr->bvbot->slenderspeedmult > 2.0f) {
		return true;
	}
	// can attack once aggroed by taking damage
	if (bvslenderIsAggro(slenderchr)) {
		return true;
	}

	return false;
}

/**
* Gets a melee damage mult for Slenderman depending on his aggro status.
*/
f32 bvslenderGetMeleeDamageMult(struct chrdata* slenderchr) {
	if (slenderchr->target != -1) {
		struct chrdata* targetchr = chrGetTargetProp(slenderchr)->chr;
		struct bvslendervictimstatus* status = bvslenderGetStatus(targetchr, slenderchr);
		
		if (status->aggro >= 2) {
			return damagemult_rush;
		} else if (status->aggro == 1 && status->exposure > exposurethreshold_startscalingdamage) { // standard aggro, scale damage based on exposure time
			return ((status->exposure - exposurethreshold_startscalingdamage) / (exposurethreshold_rush - exposurethreshold_startscalingdamage)) * (damagemult_rush - 1.0f) + 1.0f;
		}
	}
	return 1.0f;
}

/**
* Gets an anim speed mult for Slenderman depending on his aggro status.
*/
f32 bvslenderGetAnimSpeedMult(struct chrdata* slenderchr) {
	if (chrIsDead(slenderchr)) {
		return 1.0f / BVVARIANT_SLENDERMAN->stat->animspeedmult; // anim speed returns to normal chr anim speed on death
	}

	if (slenderchr->target != -1) {
		struct chrdata* targetchr = chrGetTargetProp(slenderchr)->chr;
		// Target is past the rush threshold, rush em
		if (bvslenderGetStatus(targetchr, slenderchr)->aggro >= 2) {
			return animspeedmult_rush;
		}
	}
	
	return 1.0f;
}

/**
* Gets a speed mult for Slenderman depending on his aggro status.
*/
f32 bvslenderGetSpeedMult(struct chrdata* slenderchr) {
	return slenderchr->bvbot->slenderspeedmult;
}

/**
* Gets an alpha value to render Slenderman on the current player's screen.
*/
u8 bvslenderGetAlpha(struct chrdata* slenderchr) {
	u32 alpha;
	struct chrdata* currentplayerchr = g_Vars.currentplayer->prop->chr;
	
	alpha = (u32)bvslenderGetStatus(currentplayerchr, slenderchr)->visibility;

	if (alpha >= 255) {
		return 255;
	}
	else {
		return alpha;
	}
}

/**
* Gets slenderman's blood colours in RGBA and RGB5A1 formats
*/
void bvslenderGetBloodColours(u8 *colour1, u32 *colour2) {
	if (colour1) {
		colour1[0] = 10;
		colour1[1] = 10;
		colour1[2] = 10;
	}
	if (colour2) {
		colour2[0] = 0xb0b030a0;
		colour2[1] = 0xe0e030a0;
		colour2[2] = 0xe0e050a0;
	}
}

/**
* Applies shadowy color to slenderman
*/
void bvslenderApplyColour(struct modelrenderdata* renderdata) {
	renderdata->fogcolour = 0x000000C8;
}

/**
* Calculates speed mult for Slenderman depending on his health, whether or not he's in sight, and his target's exposure.
*/
void bvslenderUpdateSpeedMult(struct chrdata* slenderchr) {
	struct chrdata* targetchr = NULL;
	struct bvslendervictimstatus* targetstatus = NULL;
	bool hastarget = (slenderchr->target != -1);
	bool insight = false;
	bool invicinity = false;

	if (hastarget) {
		targetchr = chrGetTargetProp(slenderchr)->chr;
		targetstatus = bvslenderGetStatus(targetchr, slenderchr);

		// Target is past the rush threshold, rush em
		if (targetstatus->aggro >= 2) {
			slenderchr->bvbot->slenderspeedmult = speedmult_rush;
			return;
		}
		// If standard aggro, move normally
		if (targetstatus->aggro == 1) {
			if (targetstatus->exposure > exposurethreshold_startscalingspeed) {
				slenderchr->bvbot->slenderspeedmult = ((targetstatus->exposure - exposurethreshold_startscalingspeed) / (exposurethreshold_rush - exposurethreshold_startscalingspeed)) * (speedmult_rush - 1.0f) + 1.0f;
			}
			else {
				slenderchr->bvbot->slenderspeedmult = 1.0f;
			}
			return;
		} 
		// *** not aggroed beyond this point ***

		// Determine if slenderman is in sight:
		//  Target is a player with slenderman onscreen
		if (targetstatus->onscreen) { // always false for bots
			insight = true;
		}
		//  Target is bot in slenderman's LoS
		else if (targetchr->aibot && targetstatus->haslos) {
			insight = true;
		}

		// Determine if slenderman is in vicinity
		invicinity = targetstatus->dist < distthreshold_stalking;
		
		// Slenderman has to stalk his target from a distance for a while before attacking
		// Slenderman has to freeze if in sight until he hits the rush threshold
		if ((insight || invicinity) && targetstatus->exposure > exposurethreshold_freezeonscreen) {
			slenderchr->bvbot->slenderspeedmult = 0;
			return;
		}
	}

	slenderchr->bvbot->slenderspeedmult = 1.0f;
}

/**
* Tick Slenderman behaviors:
*  + update move speed multiplier
*/
void bvslenderTick(struct chrdata* slenderchr) {
	bvslenderUpdateSpeedMult(slenderchr);
}

/**
* Call when Slenderman takes damage to aggro agaisnt the chr what damaged him.
*/
void bvslenderOnDamageTaken(struct chrdata* slenderchr, struct chrdata* achr) {
	s32 wasaggroagainstanyone;
	struct bvslendervictimstatus* status = NULL;

	if (!achr || chrIsDead(achr) || slenderchr == achr) {
		return;
	}
	
	wasaggroagainstanyone = bvslenderIsAggro(slenderchr);

	// if this slenderman wasn't already aggroed against this achr, then set to basic aggro
	status = bvslenderGetStatus(achr, slenderchr);
	if (status && status->aggro == 0) {
		status->aggro = 1;
	}

	// if a living nonteammate freshly-aggroed slenderman, he should target them
	if (!wasaggroagainstanyone 
		&& achr != chrGetTargetProp(slenderchr)->chr
		&& !chrCompareTeams(slenderchr, achr, COMPARE_FRIENDS)) {
		botSetTarget(slenderchr, achr->prop - g_Vars.props);
	}
}

/**
* Call when Slenderman dies to play some spooky noises.
*/
void bvslenderDie(struct chrdata* slenderchr) {
	u8 soundindex = rngRandom() % ARRAYCOUNT(slendermandeathsounds);
	psCreate(NULL, slenderchr->prop, slendermandeathsounds[soundindex], -1, deathcry_volume, 0, 0, PSTYPE_GENERAL, NULL, BVVARIANT_SLENDERMAN->body->voicepitch, NULL, -1, -1, -1, -1);
}


/**
* Call when Slenderman's corpse starts to fade to release spooky smoke.
*/
void bvslenderOnCorpseFade(struct chrdata* slenderchr) {
	smokeCreateSimple(&slenderchr->prop->pos, slenderchr->prop->rooms, SMOKETYPE_MEDIUM);
}

/**
* Call when Slenderman despawns to deregister as the sole Slenderman and clear most Slenderman-related data.
*/
void bvslenderDespawn(struct chrdata* slenderchr) {
	struct bvslendervictimstatus* vicstatus;
	u8 i;

	// Clear slenderman data for all chrs
	for (i = 0; i < g_MpNumChrs; i++) {
		vicstatus = slenderchr->bvbot->slendervics[i];
		if (vicstatus) {
			vicstatus->aggro = 0;
			vicstatus->dist = -1.0f;
			vicstatus->totalinsighttime = 0;
			vicstatus->visibility = 0;
			vicstatus->haslos = false;
			vicstatus->onscreen = false;
			//exposure intentionally omitted: an outstanding value will be ticked down over time
		}
	}
}

/**
* Gets a maximum value for the exposure timer based on whether Slenderman's onscreen, dead, has LoS, is neaby, or is a teammate.
*/
f32 bvslenderGetExposureCap(struct bvslendervictimstatus* status) {
	struct chrdata* victimchr = status->victimchr;
	struct chrdata* slenderchr = status->slenderchr; 
	f32 maxlimit = 0;

	// Determine starting limit based on LoS (both ways)
	if (status->onscreen) { // will never be true for bots
		maxlimit = exposurecap_onscreen;
	}
	else if (!status->slenderdead && victimchr->aibot && status->haslos) { // bot in slenderman's sight
		maxlimit = exposurecap_onscreen;
	}
	else if (!status->slenderdead && status->haslos) { // player in slenderman's sight
		maxlimit = exposurecap_onlyhaslos;
		if (status->totalinsighttime > (exposurecap_onlyhaslos * 2.0f)) { // eventually start ticking up the in-los exposure cap if slender's had plenty of LoS on us since spawn
			maxlimit = status->totalinsighttime * 0.4f;
			if (maxlimit > exposure_max) {
				maxlimit = exposure_max;
			}
		}
	}

	// If he's dead
	if (status->slenderdead) {
		// Limit if slenderman's dying (death animation is playing)
		if (slenderchr->actiontype == ACT_DIE) {
			if (maxlimit > exposurecap_dead) {
				maxlimit = exposurecap_dead;
			}
		}
		// if he's dead (animation done and corpse fading) lerp the limit down as he fades out
		else if (slenderchr->actiontype == ACT_DEAD) {
			f32 fadefrac = slenderchr->act_dead.fadetimer60 / TICKS(90); // value pulled from chrTickDead
			f32 fadelimit = exposurecap_dead;
			if (fadefrac > 1.0f) {
				fadelimit = 0;
			}
			else if (fadefrac < 0) {
				fadelimit = exposurecap_dead;
			}
			else {
				fadelimit = (1.0f - fadefrac) * exposurecap_dead; // 0 when fully faded
			}

			//apply the limit
			if (maxlimit > fadelimit) {
				maxlimit = fadelimit;
			}
		}
	}
	// If he's alive
	else { 
		// Determine and try to raise to the max distance allowed by vicinity to slenderman
		if (maxlimit < exposure_max && status->dist <= distthreshold_far_zeroeffect) {
			// Get the fraction of the way the chr's dist is between the min distance (greatest static effect) to the max distance (weakest effect)
			f32 distfrac = (status->dist - distthreshold_near_maxeffect) / (distthreshold_far_zeroeffect - distthreshold_near_maxeffect);
			f32 maxbydist = 0;

			// clamp that distance fraction to [0.0f, 1.0f]
			if (distfrac > 1.0f){
				distfrac = 1.0f;
			} else if (distfrac < 0) {
				distfrac = 0;
			}

			maxbydist = (1.0f - distfrac) * exposurecap_distance; // lerp between 0 and exposure_max_distance

			// raise maximum
			if (maxlimit < maxbydist) {
				maxlimit = maxbydist;
			}
		}

		// If the chr is neither Slenderman's target, nor is he aggroed against them, apply limit
		if (!status->istarget && status->aggro == 0) {
			if (maxlimit > exposurecap_nottargetoraggroed) {
				maxlimit = exposurecap_nottargetoraggroed;
			}
		}
	}

	// Limit for teammates
	if (chrCompareTeams(victimchr, slenderchr, COMPARE_FRIENDS)) {
		if (maxlimit > exposurecap_teammate) {
			maxlimit = exposurecap_teammate;
		}
	}

	return maxlimit;
}

/**
* Increments or decrements the exposure timer.
* f32 ratemult - determines dierction and rate
* f32 min, max - exposure will be clamped between these two values.
*/
void bvslenderIncrementExposure(struct bvslendervictimstatus* status, f32 ratemult, f32 min, f32 max) {
	status->exposure += (0.016666f * ratemult * g_Vars.lvupdate60freal);

	if (status->exposure < min) {
		status->exposure = min;
	} else if (status->exposure > max) {
		status->exposure = max;
	}
}

/**
* Tick Slenderman exposure for potential victim characters:
*  + Tick down exposure after Slenderman despawns.
*  + Tick exposure up or down appropriately
*  + Initiate rush when exposure crosses the relevant threshold by setting aggro to 2
* Precondition: slendermanonscreen, slendermanhaslos, and slendermandist values already updated this tick
*/
void bvslenderTickVictimExposure(struct bvslendervictimstatus* status) {
	f32 exposurecap = 0;

	// If chr is no longer slenderman (has despawned and respawned)
	if (!bvIsChrSlenderman(status->slenderchr)) {
		if (status->exposure > 0) {
			bvslenderIncrementExposure(status, -2.0f, 0, exposure_max);
		}
		return;
	}

	exposurecap = bvslenderGetExposureCap(status);

	// If beyond the limit, tick down but not further
	if (status->exposure > exposurecap) {
		bvslenderIncrementExposure(status, -2.0f, exposurecap, status->exposure);
	}
	// Below the limit, tick up to it
	else if (status->exposure < exposurecap) {
		bvslenderIncrementExposure(status, 1.0f, status->exposure, exposurecap);
	}

	// If slenderman's ready to rush the victim, set aggro
	if (status->exposure >= exposurethreshold_rush) {
		status->aggro = 2;
	}
}

/**
* Updates Slenderman's visibility for the given chr based on their exposure.
*/
void bvslenderUpdateVisibilityForChr(struct bvslendervictimstatus* status) {
	bool unaggroed = !bvslenderIsAggro(status->slenderchr);

	// If we're slenderman's target and he's alive and unaggroedd, his visibility is determined by our own exposure
	if (!status->slenderdead && unaggroed && status->istarget) {
		if (status->exposure <= exposurethreshold_visibilitystart) {
			status->visibility = 0;
		}
		else if (status->exposure >= exposurethreshold_visibilitymax) {
			status->visibility = (f32)maxvisibility;
		}
		else {
			status->visibility = ((f32)maxvisibility * ((status->exposure - exposurethreshold_visibilitystart) / (exposurethreshold_visibilitymax - exposurethreshold_visibilitystart)));
		}
	}
	// If slenderman's aggroed or dead, tick visibility up
	else if ((!unaggroed || status->slenderdead) && status->visibility < 255.0f) {
		status->visibility += (0.016666f * visibilityuprate_deadoraggro * g_Vars.lvupdate60freal);
		if (status->visibility > 255.0f) {
			status->visibility = 255.0f;
		}
	}
	else { // tick visibility down
		if (status->visibility > 0) {
			status->visibility -= (0.016666f * visibilityuprate_deadoraggro * g_Vars.lvupdate60freal);
			if (status->visibility < 0) {
				status->visibility = 0;
			}
		}
	}
}

/**
* Tick Slenderman effects on possible victim character:
*  + Updates slendermanonscreen, slendermanhaslos, and slendermandist values
*  + Tick exposure up and down as appropriate
*  + Updates slenderman's visibility from this chr's perspective (used for alpha on players' screens, and whether visible to other bots)
*/
void bvslenderTickVictim(struct chrdata* chr) {
	struct bvchrdata* botbvchr = NULL;
	struct bvslendervictimstatus* status = NULL;
	u8 i;

	// Loop through bots looking for Slendermen to tick for
	for (i = 0; i < g_BotCount; i++) {
		botbvchr = g_BvMatch.bots[i];
		if (!botbvchr || botbvchr == chr->bvchr) { // Skip empty bots and this chr
			continue;
		}

		status = chr->bvchr->slendervicstatus[i];
		if (!status) { // skip if no status struct allocated
			continue;
		}
		if (status->exposure == 0 && !(botbvchr->flags & BVFLAG_SLENDERMAN)) { // skip bots who aren't Slenderman unless we have outstanding exposure
			continue;
		}

		// Update our status relative to Slenderman
		status->istarget = (chr == chrGetTargetProp(status->slenderchr)->chr);
		status->slenderdead = chrIsDead(status->slenderchr);
		status->haslos = status->slenderchr->aibot->chrsinsight[chr->mpindex];
		status->dist = status->slenderchr->aibot->chrdistances[chr->mpindex];
		if (!chr->aibot) { // players only: check if slender on this player's screen
			status->onscreen = (status->haslos && status->slenderchr->prop->flags & PROPFLAG_ONTHISSCREENTHISTICK);
		}
	
		// Tick total in sight time
		if (status->haslos) {
			status->totalinsighttime += (0.016666f * g_Vars.lvupdate60freal);
		}

		bvslenderTickVictimExposure(status);
		bvslenderUpdateVisibilityForChr(status);
	}
}

/**
* Decays chr's exposure timers for all slendermen.
*/
void bvslenderDecrementAllExposure(struct chrdata* chr) {
	struct bvslendervictimstatus* vicstatus;
	for (u8 i = 0; i < g_BotCount; i++) {
		vicstatus = chr->bvchr->slendervicstatus[i];
		if (vicstatus && vicstatus->exposure > 0) {
			bvslenderIncrementExposure(vicstatus, -1.0f, 0, exposure_max); // dissolve static while dying
		}
	}
}


void bvslenderResetVictimDataForSpawn(struct chrdata* victimchr) {
	struct bvslendervictimstatus* vicstatus;
	for (u8 i = 0; i < g_BotCount; i++) {
		struct bvslendervictimstatus* vicstatus = victimchr->bvchr->slendervicstatus[i];
		if (vicstatus) {
			vicstatus->exposure = 0;
			vicstatus->visibility = 0;
			vicstatus->dist = -1.0f;
			vicstatus->totalinsighttime = 0;
			vicstatus->aggro = 0;
			vicstatus->onscreen = false;
			vicstatus->haslos = false;
			vicstatus->istarget = false;
			vicstatus->slenderdead = false;
		}
	}
}

/**
* Applies a varying amount of static to the current player's screen based on their exposure timer.
* Decays static level if player is dead.
*/
Gfx *bvslenderApplyVictimStatic(Gfx *gdl) {
	u32 staticlevel;
	struct chrdata* chr = g_Vars.currentplayer->prop->chr;
	f32 exposure = bvslenderGetChrHighestExposure(chr);

	if (exposure <= exposurethreshold_minstatic) {
		return gdl;
	}

	// Decay exposure if player is dead
	if (g_Vars.currentplayer->isdead) {
		bvslenderDecrementAllExposure(chr);
	}

	staticlevel = (u32)((f32)maxstaticamount * ((exposure - exposurethreshold_minstatic) / (exposurethreshold_maxstatic - exposurethreshold_minstatic)));

	if (staticlevel > maxstaticamount) {
		staticlevel = maxstaticamount;
	}

	return bviewDrawStatic(gdl, 0xffffffff, staticlevel);
}

/**
* Displays exposure onscreen for debug purposes. Renders in the same position as the framerate counter so turn that off
*/
Gfx *bvslenderDisplayExposure(Gfx *gdl)
{
	f32 exposure = bvslenderGetChrHighestExposure(g_Vars.currentplayer->prop->chr);
	s32 x = viGetViewLeft() + 27;
	s32 y = viGetViewTop() + 13;
	u32 color = 0x00ff00a0;
	char buffer[16];

	if (g_CharsNumeric && g_FontNumeric) {
		snprintf(buffer, sizeof buffer, "%.2f", exposure);

		gSPSetExtraGeometryModeEXT(gdl++, g_HudAlignModeL);

		gdl = text0f153628(gdl);
		gdl = textRender(gdl, &x, &y, buffer, g_CharsNumeric, g_FontNumeric, color, 0x000000a0, viGetWidth(), viGetHeight(), 0, 0);
		gdl = text0f153780(gdl);

		gSPClearExtraGeometryModeEXT(gdl++, g_HudAlignModeL);
	}

	return gdl;
}
