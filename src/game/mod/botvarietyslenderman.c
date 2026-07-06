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
// + Only one slenderman can exist at a time. (bvspawnPrepVariety, bvslendermanCanSpawn)
// Targeting
// + Slenderman should update distance and LoS to its target every frame. (bvShouldCalcTargetDistEveryFrame, bvShouldCalcTargetLoSEveryFrame)
// + Slenderman can see through cloaks. (bvCanSeeThroughCloak)
// Attacks/Weapons
// + Slenderman can't pick up or use any weapons, nor can he disarm. (bvbotCanUseWeapon, bvCanChrPickupWeapon)
// + Slenderman can't attack a chr unless aggroed against them, either by taking damage from them, or by rushing them due to high exposure (see below).
// Exposure
// + As long as Slenderman is spawned, each chr's exposure timer ticks up to (or if beyond, back down to) their Exposure Cap. (bvslendermanTickOtherChr, bvslendermanTickOtherExposure)
// + The character's Exposure Cap is dynamically adjusted based on several factors: (bvslendermanGetExposureCap)
//   + First, the Exposure Cap is raised to the highest of the three values from three contributing factors:
//     + whether in live Slenderman's LoS
//     + vicinity to live Slenderman (value tapers off with distance)
//     + whether Slenderman is on the player's screen (or for bots, in mutual LoS)
//   + Second, the Exposure Cap can be limited in turn by each of three limiting factors:
//     + whether Slenderman is dying or dead (limit tapers down to zero as corpse fades)
//     + whether Slenderman is a teammate
//     + if the chr is neither Slenderman's target nor is he aggroed against the chr
//   + Naturally, the Exposure Cap will be zero after Slenderman despawns, so chrs with outstanding exposure will have it (quickly) untick down to zero.
// + The practical upshot of the Exposure Cap feature is that the highest exposure thresholds (eg Rushing described below) can only be reached if Slenderman is onscreen
//   (or in mutual LoS for bots).
// + If BVVARIANT_SLENDERMAN->debug is enabled, each player's current exposure will be displayed on the hud.
// Effects of Exposure
// + Players' screens will gradually fill with static as their exposure timer fills up. (bvslendermanApplyVictimStatic)
// + The initially-invisible Slenderman becomes visible to his target as their exposure increases. (bvslendermanTickOtherChr, bvslendermanUpdateOpacityForChr, bvslendermanGetAlpha, chrRender)
// + Target's exposure > exposurethreshold_freezeonscreen: Slenderman can't move while on a player's screen (or in mutual LoS for other bots), until
// + Target's exposure > exposurethreshold_rush: Slenderman aggroes and rushes the target at high speed and with devastating melee damage.
// Look and sounds
// + Slenderman's opacity will also increase for all players while aggroed or dead. (bvslendermanUpdateOpacityForChr)
// + Slenderman wears a suit unless he's also an Impostor. (bvspawnHandleSlenderman)
// + Slenderman is dark. (bvTryApplyLateColourTweaks, bvslendermanApplyColour)
// + Slenderman bleeds black. (bvSlendermanGetBloodColours)
// + Slenderman vocalizes on death. (bvslendermanDie)
// + Slenderman's corpse dissapates into a cloud of smoke. (bvslendermanOnCorpseFade)

const f32 exposure_max = 15.0f; // absolute max value for exposure

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
const u8 maxstaticamount = 96;

// Opacity
const f32 exposurethreshold_opacitystart = 2.5f; // seconds of exposure before slenderman begins to become visible to a chr
const f32 exposurethreshold_opacitymax = 5.5f; // seconds of exposure before slenderman becomes fully opaque to a chr
const u8 maxopacity = 255;
const f32 opacityuprate_deadoraggro = 255.0f; // rate that opacity ticks up per second for all players after slenderman is dead or aggroed

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

bool bvIsChrSlenderman(struct chrdata* chr) {
	return (g_BvMatch.slendermanchr == chr && CHR_BV_FLAGS & BVFLAG_SLENDERMAN);
}

bool bvslendermanIsSpawned() {
	return g_BvMatch.slendermanchr != NULL;
}

bool bvslendermanIsAlive() {
	return (g_BvMatch.slendermanchr != NULL && g_BvMatch.slendermanchr->actiontype != ACT_DIE && g_BvMatch.slendermanchr->actiontype != ACT_DEAD);
}

/**
* Returns true if Slenderman is spawned and has a target.
*/
bool bvslendermanHasTarget() {
	return g_BvMatch.slendermanchr && g_BvMatch.slendermanchr->target != -1;
}

/**
* Gets the chr who is the current Slenderman; null if there isn't one.
*/
struct chrdata* bvslendermanGetChr() {
	return g_BvMatch.slendermanchr;
}

/**
* Gets the chr that Slenderman is currently targeting.
* Returns null if no slenderman or if he has no target.
*/
struct chrdata* bvslendermanGetTargetChr() {
	if (g_BvMatch.slendermanchr == NULL || g_BvMatch.slendermanchr->target == -1) {
		return NULL;
	}
	return chrGetTargetProp(g_BvMatch.slendermanchr)->chr; 
}

/**
* Should this non-Slenderman chr be ticked for Slenderman-related behaviors?
*/
bool bvslendermanShouldOtherChrTick(struct chrdata* chr) {
	// Don't do this tick if we ARE slenderman
	if (g_BvMatch.slendermanchr == chr) {
		return false;
	}

	// Do the tick if slenderman is spawned or we have extant exposure left to decay after he despawned
	if (g_BvMatch.slendermanchr || chr->bvchr->slendermanexposure > 0) {
		return true;
	}
	return false;
}

bool bvslendermanShouldDoStatic(struct chrdata* chr) {
	return (chr->bvchr->slendermanexposure > 0);
}

bool bvslendermanIsVisibleToChr(struct chrdata* chr) {
	return (chr->bvchr->slendermanopacity > 0);
}

/**
* Can a Slenderman bot spawn right now?
* Returns true if there is no extant Slenderman.
*/
bool bvslendermanCanSpawn() {
	return (g_BvMatch.slendermanchr == NULL);
}

/**
* Call when Slenderman spawns to register as the current and sole Slenderman.
*/
void bvslendermanSpawn(struct chrdata* chr) {
	g_BvMatch.slendermanchr = chr;
	g_BvMatch.slendermanspeedmult = 1.0f;
}

/**
* Call when Slenderman's corpse starts to fade to release spooky smoke.
*/
void bvslendermanOnCorpseFade(struct chrdata* chr) {
	smokeCreateSimple(&chr->prop->pos, chr->prop->rooms, SMOKETYPE_MEDIUM);
}

/**
* Call when Slenderman dies to play some spooky noises.
*/
void bvslendermanDie(struct chrdata* chr) {
	u8 soundindex = rngRandom() % ARRAYCOUNT(slendermandeathsounds);
	psCreate(NULL, chr->prop, slendermandeathsounds[soundindex], -1, deathcry_volume, 0, 0, PSTYPE_GENERAL, NULL, BVVARIANT_SLENDERMAN->body->voicepitch, NULL, -1, -1, -1, -1);
}

/**
* Is Slenderman aggro against any chr right now?
*/
bool bvslendermanIsAggro() {
	struct bvchrdata* trybvchr;
	u8 i;

	for (i = 0; i < g_MpNumChrs; i++) {
		trybvchr = mpGetChrFromPlayerIndex(i)->bvchr;
		if (trybvchr->slendermanaggro > 0) {
			return true;
		}
	} 

	return false;
}

/**
* Call when Slenderman takes damage to aggro agaisnt the chr what damaged him.
*/
void bvslendermanOnDamageTaken(struct chrdata* achr) {
	s32 wasaggroagainstanyone;

	if (!achr || chrIsDead(achr)) {
		return;
	}
	
	wasaggroagainstanyone = bvslendermanIsAggro();

	if (achr->bvchr->slendermanaggro == 0) {
		achr->bvchr->slendermanaggro = 1;
	}

	// if a living nonteammate freshly-aggroed slenderman, he should target them
	if (!wasaggroagainstanyone 
		&& achr != bvslendermanGetTargetChr()
		&& !chrCompareTeams(g_BvMatch.slendermanchr, achr, COMPARE_FRIENDS)) {
		botSetTarget(g_BvMatch.slendermanchr, achr->prop - g_Vars.props);
	}
}

/**
* Increments or decrements the exposure timer.
* f32 ratemult - determines dierction and rate
* f32 min, max - exposure will be clamped between these two values.
*/
void bvslendermanIncrementExposure(struct bvchrdata* bvchr, f32 ratemult, f32 min, f32 max) {
	bvchr->slendermanexposure += (0.016666f * ratemult * g_Vars.lvupdate60freal);

	if (bvchr->slendermanexposure < min) {
		bvchr->slendermanexposure = min;
	} else if (bvchr->slendermanexposure > max) {
		bvchr->slendermanexposure = max;
	}
}

/**
* Call when Slenderman despawns to deregister as the sole Slenderman and clear most Slenderman-related data.
*/
void bvslendermanDespawn() {
	struct chrdata* chr = g_BvMatch.slendermanchr;
	struct bvchrdata* trybvchr;
	u8 i;
	g_BvMatch.slendermanchr = NULL;

	//smokeCreateSimple(&chr->prop->pos, chr->prop->rooms, SMOKETYPE_MEDIUM);

	// Clear slenderman data for all chrs
	for (i = 0; i < g_MpNumChrs; i++) {
		trybvchr = mpGetChrFromPlayerIndex(i)->bvchr;
		trybvchr->slendermandist = -1.0f;
		trybvchr->slendermanonscreen = false;
		trybvchr->slendermanhaslos = false;
		trybvchr->slendermanaggro = 0;
		trybvchr->slendermanopacity = 0;
		//exposure intentionally omitted: an outstanding value will be ticked down over time
	}
}

f32 bvslendermanGetMeleeDamageMult() {
	if (bvslendermanHasTarget()) {
		struct chrdata* targetchr = bvslendermanGetTargetChr();

		if (targetchr->bvchr->slendermanaggro >= 2) {
			return damagemult_rush;
		}
	}
	return 1.0f;
}

/**
* Checks if Slenderman can attack its target based on whether it's been damaged or if its rushing.
*/
bool bvslendermanCanAttack() {
	// can attack if rushing
	if (g_BvMatch.slendermanspeedmult > 2.0f) {
		return true;
	}
	// can attack once aggroed by taking damage
	if (bvslendermanIsAggro()) {
		return true;
	}

	return false;
}

/**
* Gets a speed mult for Slenderman depending on whether or not he's rushing his target.
*/
f32 bvslendermanGetAnimSpeedMult() {
	if (!bvslendermanIsAlive()) {
		return 1 / BVVARIANT_SLENDERMAN->stat->animspeedmult;
	}

	if (bvslendermanHasTarget()) {
		struct chrdata* targetchr = bvslendermanGetTargetChr();
		// Target is past the rush threshold, rush em
		if (targetchr->bvchr->slendermanaggro >= 2) {
			return animspeedmult_rush;
		}
	}
	
	return 1.0f;
}

/**
* Gets a speed mult for Slenderman depending on his health, whether or not he's in sight, and his target's exposure.
*/
f32 bvslendermanGetSpeedMult() {
	return g_BvMatch.slendermanspeedmult;
}

/**
* Gets a maximum value for the exposure timer based on whether Slenderman's onscreen, dead, has LoS, is neaby, or is a teammate.
*/
f32 bvslendermanGetExposureCap(struct chrdata* chr, struct bvchrdata* bvchr) {
	f32 maxlimit = 0;
	bool isalive = bvslendermanIsAlive();

	// Determine starting limit based on LoS (both ways)
	if (bvchr->slendermanonscreen) { // will never be true for bots
		maxlimit = exposurecap_onscreen;
	}
	else if (isalive && chr->aibot && bvchr->slendermanhaslos) { // bot in slenderman's sight
		maxlimit = exposurecap_onscreen;
	}
	else if (isalive && bvchr->slendermanhaslos) { // player in slenderman's sight
		maxlimit = exposurecap_onlyhaslos;
	}

	// If he's dead
	if (!isalive) {
		// Limit if slenderman's dying (death animation is playing)
		if (g_BvMatch.slendermanchr->actiontype == ACT_DIE) {
			if (maxlimit > exposurecap_dead) {
				maxlimit = exposurecap_dead;
			}
		}
		// if he's dead (animation done and corpse fading) lerp the limit down as he fades out
		else if (g_BvMatch.slendermanchr->actiontype == ACT_DEAD) {
			f32 fadefrac = g_BvMatch.slendermanchr->act_dead.fadetimer60 / TICKS(90); // value pulled from chrTickDead
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
		bool istarget = (bvslendermanGetTargetChr() == chr);

		// Determine and try to raise to the max distance allowed by vicinity to slenderman
		if (maxlimit < exposure_max && bvchr->slendermandist <= distthreshold_far_zeroeffect) {
			// Get the fraction of the way the chr's dist is between the min distance (greatest static effect) to the max distance (weakest effect)
			f32 distfrac = (bvchr->slendermandist - distthreshold_near_maxeffect) / (distthreshold_far_zeroeffect - distthreshold_near_maxeffect);
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
		if (!istarget && bvchr->slendermanaggro == 0) {
			if (maxlimit > exposurecap_nottargetoraggroed) {
				maxlimit = exposurecap_nottargetoraggroed;
			}
		}
	}

	// Limit for teammates
	if (chrCompareTeams(chr, g_BvMatch.slendermanchr, COMPARE_FRIENDS)) {
		if (maxlimit > exposurecap_teammate) {
			maxlimit = exposurecap_teammate;
		}
	}

	return maxlimit;
}

/**
* Calculates speed mult for Slenderman depending on his health, whether or not he's in sight, and his target's exposure.
*/
void bvslendermanUpdateSpeedMult() {
	struct chrdata* targetchr = NULL;
	struct bvchrdata* bvchr = NULL;
	bool hastarget = bvslendermanHasTarget();
	bool insight = false;
	bool invicinity = false;

	if (hastarget) {
		targetchr = bvslendermanGetTargetChr();
		bvchr = targetchr->bvchr;

		// Target is past the rush threshold, rush em
		if (bvchr->slendermanaggro >= 2) {
			g_BvMatch.slendermanspeedmult = speedmult_rush;
			return;
		}
		// If standard aggro, move normally
		if (bvchr->slendermanaggro == 1) {
			g_BvMatch.slendermanspeedmult = 1.0f;
			return;
		} 
		// *** not aggroed beyond this point ***

		// Determine if slenderman is in sight:
		//  Target is a player with slenderman onscreen
		if (bvchr->slendermanonscreen) { // always false for bots
			insight = true;
		}
		//  Target is bot in slenderman's LoS
		else if (targetchr->aibot && bvchr->slendermanhaslos) {
			insight = true;
		}

		// Determine if slenderman is in vicinity
		invicinity = bvchr->slendermandist < distthreshold_stalking;
		
		// Slenderman has to stalk his target from a distance for a while before attacking
		// Slenderman has to freeze if in sight until he hits the rush threshold
		if ((insight || invicinity) && bvchr->slendermanexposure > exposurethreshold_freezeonscreen) {
			g_BvMatch.slendermanspeedmult = 0;
			return;
		}
	}

	g_BvMatch.slendermanspeedmult = 1.0f;
}

/**
* Tick Slenderman behaviors:
*  + update move speed multiplier
*/
void bvslendermanTick(struct chrdata* chr) {
	bvslendermanUpdateSpeedMult();
}

/**
* Tick Slenderman exposure for non-Slenderman characters:
*  + Tick down exposure after Slenderman despawns.
*  + Tick exposure up or down appropriately
*  + Initiate rush when exposure crosses the relevant threshold by setting aggro to 2
* Precondition: slendermanonscreen, slendermanhaslos, and slendermandist values already updated this tick
*/
void bvslendermanTickOtherChrExposure(struct chrdata* chr, struct bvchrdata* bvchr) {
	f32 exposurecap = 0;

	// Slenderman has despawned, tick down victim exposure quickly and we're done
	if (g_BvMatch.slendermanchr == NULL) {
		if (bvchr->slendermanexposure > 0) {
			bvslendermanIncrementExposure(bvchr, -2.0f, 0, exposure_max);
		}
		return;
	}
	// *** Slenderman known to be spawned past this point ***

	exposurecap = bvslendermanGetExposureCap(chr, bvchr);

	// If beyond the limit, tick down but not further
	if (bvchr->slendermanexposure > exposurecap) {
		bvslendermanIncrementExposure(bvchr, -2.0f, exposurecap, bvchr->slendermanexposure);
	}
	// Below the limit, tick up to it
	else if (bvchr->slendermanexposure < exposurecap) {
		bvslendermanIncrementExposure(bvchr, 1.0f, bvchr->slendermanexposure, exposurecap);
	}

	// If slenderman's ready to rush the victim, set aggro
	if (bvchr->slendermanexposure >= exposurethreshold_rush) {
		bvchr->slendermanaggro = 2;
	}
}

/**
* Updates Slenderman's opacity for the given chr based on their exposure.
*/
void bvslendermanUpdateOpacityForChr(struct chrdata* chr, struct bvchrdata* bvchr) {
	bool slendermanalive = bvslendermanIsAlive();
	bool christarget = (bvslendermanGetTargetChr() == chr);
	bool unaggroed = !bvslendermanIsAggro();

	// If we're slenderman's target and he's alive and unaggroedd, his opacity is determined by our own exposure
	if (slendermanalive && unaggroed && christarget) {
		if (bvchr->slendermanexposure <= exposurethreshold_opacitystart) {
			bvchr->slendermanopacity = 0;
		}
		else if (bvchr->slendermanexposure >= exposurethreshold_opacitymax) {
			bvchr->slendermanopacity = (f32)maxopacity;
		}
		else {
			bvchr->slendermanopacity = ((f32)maxopacity * ((bvchr->slendermanexposure - exposurethreshold_opacitystart) / (exposurethreshold_opacitymax - exposurethreshold_opacitystart)));
		}
	}
	// If slenderman's aggroed or dead, tick opacity up
	else if ((!unaggroed || !slendermanalive) && bvchr->slendermanopacity < 255.0f) {
		bvchr->slendermanopacity += (0.016666f * opacityuprate_deadoraggro * g_Vars.lvupdate60freal);
		if (bvchr->slendermanopacity > 255.0f) {
			bvchr->slendermanopacity = 255.0f;
		}
	}
	else { // tick opacity down
		if (bvchr->slendermanopacity > 0) {
			bvchr->slendermanopacity -= (0.016666f * opacityuprate_deadoraggro * g_Vars.lvupdate60freal);
			if (bvchr->slendermanopacity < 0) {
				bvchr->slendermanopacity = 0;
			}
		}
	}
	
}

/**
* Tick Slenderman effects on living non-Slenderman characters:
*  + Updates slendermanonscreen, slendermanhaslos, and slendermandist values
*  + Tick exposure up and down as appropriate
*  + Updates slenderman's opacity from this chr's perspective (used for alpha on players' screens, and whether visible to other bots)
*/
void bvslendermanTickOtherChr(struct chrdata* chr) {
	struct bvchrdata* bvchr = chr->bvchr;

	if (g_BvMatch.slendermanchr != NULL) {
		s32 chrindex = mpPlayerGetIndex(chr);

		// Update our status relative to Slenderman
		bvchr->slendermanhaslos = g_BvMatch.slendermanchr->aibot->chrsinsight[chrindex];
		if (!chr->aibot) { // players only: check if this player's screen
			bvchr->slendermanonscreen = (g_BvMatch.slendermanchr->prop->flags & PROPFLAG_ONTHISSCREENTHISTICK && bvchr->slendermanhaslos);
		}
		bvchr->slendermandist = g_BvMatch.slendermanchr->aibot->chrdistances[chrindex];
	}
	
	bvslendermanTickOtherChrExposure(chr, bvchr);
	bvslendermanUpdateOpacityForChr(chr, bvchr);
}

/**
* Applies a varying amount of static to the current player's screen based on their exposure timer.
* Decays static level if player is dead.
*/
Gfx *bvslendermanApplyVictimStatic(Gfx *gdl) {
	//return bviewDrawStatic(gdl, 0x4fffffff, 255);

	u32 staticlevel;
	struct bvchrdata* bvchr = g_Vars.currentplayer->prop->chr->bvchr;
	f32 exposure = bvchr->slendermanexposure;

	if (exposure <= exposurethreshold_minstatic) {
		return gdl;
	}
	if (g_Vars.currentplayer->isdead) {
		bvslendermanIncrementExposure(bvchr, -1.0f, 0, exposure_max); // dissolve static while dying
	}

	staticlevel = (u32)((f32)maxstaticamount * ((exposure - exposurethreshold_minstatic) / (exposurethreshold_maxstatic - exposurethreshold_minstatic)));

	if (staticlevel > maxstaticamount) {
		staticlevel = maxstaticamount;
	}

	return bviewDrawStatic(gdl, 0xffffffff, staticlevel);
}

/**
* Gets an alpha value to render Slenderman on the current player's screen.
* Also ticks slenderman opacity after death.
*/
u8 bvslendermanGetAlpha() {
	u32 alpha;
	struct chrdata* currentplayerchr = g_Vars.currentplayer->prop->chr;
	
	alpha = (u32)currentplayerchr->bvchr->slendermanopacity;

	if (alpha >= 255) {
		return 255;
	}
	else {
		return alpha;
	}
}

/**
* Applies shadowy color to slenderman
*/
void bvslendermanApplyColour(struct chrdata* botchr, struct modelrenderdata* renderdata) {
	renderdata->fogcolour = 0x000000C8; // colourBlend(0x000000FF, renderdata->fogcolour, 200);
}

void bvslendermanGetBloodColours(u8 *colour1, u32 *colour2) {
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
* Displays exposure onscreen for debug purposes. Renders in the same position as the framerate counter so turn that off
*/
Gfx *bvslendermanDisplayExposure(Gfx *gdl)
{
	f32 exposure = g_Vars.currentplayer->prop->chr->bvchr->slendermanexposure;
	s32 x = viGetViewLeft() + 27;
	s32 y = viGetViewTop() + 13;
	//x *= (g_Vars.currentplayerindex + 1);
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

