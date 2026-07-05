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
// + Slenderman can't pick up or use any weapons. (bvbotCanUseWeapon, bvCanChrPickupWeapon)
// Unique attack method: Analogue Horror Static
// + Players' screens will gradually fill with static as their victimprogress timer fills up. (bvslendermanApplyVictimStatic)
// + Factors in victimprogress: (bvslendermanGetProgressLimit, bvslendermanTickOtherChr, bvslendermanTickOtherVictimProgress)
//   + distance to Slenderman -- raises maximum progress limit proportional to distance
//   + whether Slenderman has LoS on them (limited effect without other factors) -- raises maximum progress level to a low threshold
//   + whether Slenderman is on their screen (players only) -- no limit, eventually results in full staatic
// + The victimprogress timer will decrease at the same rate if none of the above are true, or faster is Slenderman is dead. (bvslendermanTickOtherChr)
// Onscreen behavior
// + Slenderman can't move if he's on his target's screen until their progress reaches a certain threshold, at which point his speed massively increases. 
//    (bvslendermanTick, bvslendermanUpdateSpeedMult, bvslendermanGetSpeedMult, bvslendermanGetAnimSpeedMult, bvShouldChrStandStill)
// + While not moving, slenderman won't attack. (bvslendermanCanAttack)
// + Slenderman's damage massively increases beyond that same threshold. (bvslendermanGetMeleeDamageMult)
// Look and sounds
// + Slenderman is dark (bvTryApplyLateColourTweaks, bvslendermanApplyColour)
// + Slenderman starts invisible but becomes opaque as victimprogress increases or when aggroed (chrRender, bvslendermanGetAlpha)
// + Slenderman bleeds black
// + Slenderman releases a cloud of smoke on death (bvslendermanOnCorpseFade)

const u8 maxstaticamount = 96;
const u8 maxopacity = 255;
const f32 opacityuprate_dead = 255.0f;

const f32 deathcry_volume = 2.0f;

const f32 progressthreshold_minstatic = 0; // seconds of exposure before the static starts
const f32 progressthreshold_maxstatic = 10.0f; // seconds of exposure before reaching max static
const f32 progress_max_distance = 8.0f; 
const f32 progress_max = 15.0f;

const f32 progresslimit_onlyhaslos = 5.0f; // max level of exposure from just being in slenderman's los
const f32 progresslimit_teammate = 5.0f; // max level of exposure/static for slenderman's teammates
const f32 progresslimit_dead = 3.0f;

const f32 progressthreshold_opacitystart = 2.5f; // seconds of exposure before slenderman begins to become visible
const f32 progressthreshold_opacitymax = 5.5f; // seconds of exposure before slenderman becomes fully opaque
const f32 progressthreshold_cantmoveifonscreen = 1.5f; // seconds of exposure before slenderman must freeze if onscreen
const f32 progressthreshold_rush = 10.5f; // seconds of exposure beyond which slenderman rushes the target

const f32 distthreshold_noapproachuntilrush = 500.0f;
const f32 distthreshold_near_maxeffect = 550.0f; // near distance at which slenderman's vicinity effects hit their maximum
const f32 distthreshold_far_zeroeffect = 1200.0f; // far distance at which slenderman's vicinity effects reach zero

const f32 speedmult_rush = 2.5f;
const f32 animspeedmult_rush = 2.0f;
const f32 damagemult_rush = 2.5f;

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
	struct bvchrdata* bvchr = bvGetChrMatchData(chr);

	// Don't do this tick if we ARE slenderman
	if (g_BvMatch.slendermanchr == chr) {
		return false;
	}

	// Do the tick if slenderman is spawned or we have extant progress left to decay after he despawned
	if (g_BvMatch.slendermanchr || bvchr->slendermanvictimprogress > 0) {
		return true;
	}
	return false;
}

bool bvslendermanShouldDoStatic(struct chrdata* chr) {
	struct bvchrdata* bvchr = bvGetChrMatchData(chr);
	return (bvchr->slendermanvictimprogress > 0);
}

bool bvslendermanIsVisibleToChr(struct chrdata* chr) {
	struct bvchrdata* bvchr = bvGetChrMatchData(chr);
	return (bvchr->slendermanopacity > 0);
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

	for (i = 0; i < MAX_PLAYERS + MAX_BOTS; i++) {
		if (i > MAX_PLAYERS) {
			trybvchr = &g_BvMatch.bots[i - MAX_PLAYERS];
		}
		else {
			trybvchr = &g_BvMatch.players[i];
		}
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
	struct bvchrdata* bvchr;
	s32 wasaggro;

	if (!achr) {
		return;
	}
	
	bvchr = bvGetChrMatchData(achr);
	wasaggro = bvslendermanIsAggro();

	if (!wasaggro) {
		bvchr->slendermanaggro = 1;
	}

	// if a living nonteammate freshly-aggroed slenderman, he should target them
	if (!wasaggro 
		&& achr != bvslendermanGetTargetChr() 
		&& !chrIsDead(achr)
		&& !chrCompareTeams(g_BvMatch.slendermanchr, achr, COMPARE_FRIENDS)) {
		botSetTarget(g_BvMatch.slendermanchr, achr->prop - g_Vars.props);
	}
}

/**
* Increments or decrements the victimprogress timer.
* f32 ratemult - determines dierction and rate
* f32 min, max - victimprogress will be clamped between these two values.
*/
void bvslendermanIncrementVictimProgress(struct bvchrdata* bvchr, f32 ratemult, f32 min, f32 max) {
	bvchr->slendermanvictimprogress += (0.016666f * ratemult * g_Vars.lvupdate60freal);

	if (bvchr->slendermanvictimprogress < min) {
		bvchr->slendermanvictimprogress = min;
	} else if (bvchr->slendermanvictimprogress > max) {
		bvchr->slendermanvictimprogress = max;
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
	for (i = 0; i < MAX_PLAYERS + MAX_BOTS; i++) {
		if (i > MAX_PLAYERS) {
			trybvchr = &g_BvMatch.bots[i - MAX_PLAYERS];
		}
		else {
			trybvchr = &g_BvMatch.players[i];
		}
		trybvchr->slendermandist = -1.0f;
		trybvchr->slendermanonscreen = false;
		trybvchr->slendermanhaslos = false;
		trybvchr->slendermanaggro = 0;
		trybvchr->slendermanopacity = 0;
		//victimprogress intentionally omitted: an outstanding value will be ticked down over time
	}
}

f32 bvslendermanGetMeleeDamageMult() {
	if (bvslendermanHasTarget()) {
		struct chrdata* targetchr = bvslendermanGetTargetChr();
		struct bvchrdata* bvchr = bvGetChrMatchData(targetchr);

		if (bvchr->slendermanaggro >= 2) {
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
		struct bvchrdata* bvchr = bvGetChrMatchData(targetchr);
		// Target is past the rush threshold, rush em
		if (bvchr->slendermanaggro >= 2) {
			return animspeedmult_rush;
		}
	}
	
	return 1.0f;
}

/**
* Gets a speed mult for Slenderman depending on his health, whether or not he's in sight, and his target's victimprogress.
*/
f32 bvslendermanGetSpeedMult() {
	return g_BvMatch.slendermanspeedmult;
}

/**
* Gets a maximum value for the victimprogress timer based on whether Slenderman's onscreen, dead, has LoS, is neaby, or is a teammate.
*/
f32 bvslendermanGetProgressLimit(struct chrdata* chr, struct bvchrdata* bvchr) {
	f32 maxlimit = 0;
	bool isalive = bvslendermanIsAlive();

	// Determine starting limit based on LoS (both ways)
	if (bvchr->slendermanonscreen) { // will never be true for bots
		maxlimit = progress_max;
	}
	else if (isalive && chr->aibot && bvchr->slendermanhaslos) { // bot in slenderman's sight
		maxlimit = progress_max;
	}
	else if (isalive && bvchr->slendermanhaslos) { // player in slenderman's sight
		maxlimit = progresslimit_onlyhaslos;
	}

	// Limit if slenderman's dead
	if (!isalive) {
		if (maxlimit > progresslimit_dead) {
			maxlimit = progresslimit_dead;
		}
	}
	// If he's alive, Determine and apply the max distance allowed by distance to slenderman
	else if (maxlimit < progress_max && bvchr->slendermandist <= distthreshold_far_zeroeffect) {
		// Get the fraction of the way the chr's dist is between the min distance (greatest static effect) to the max distance (weakest effect)
		f32 distfrac = (bvchr->slendermandist - distthreshold_near_maxeffect) / (distthreshold_far_zeroeffect - distthreshold_near_maxeffect);
		f32 maxbydist = 0;

		// clamp that distance fraction to [0.0f, 1.0f]
		if (distfrac > 1.0f){
			distfrac = 1.0f;
		} else if (distfrac < 0) {
			distfrac = 0;
		}

		maxbydist = distfrac * progress_max_distance; // lerp between 0 and progress_max_distance

		// raise maximum
		if (maxbydist > maxlimit) {
			maxlimit = maxbydist;
		}
	}

	// Limit for teammates
	if (chrCompareTeams(chr, g_BvMatch.slendermanchr, COMPARE_FRIENDS)) {
		if (maxlimit > progresslimit_teammate) {
			maxlimit = progresslimit_teammate;
		}
	}

	return maxlimit;
}

/**
* Calculates speed mult for Slenderman depending on his health, whether or not he's in sight, and his target's victimprogress.
*/
void bvslendermanUpdateSpeedMult() {
	struct chrdata* targetchr = NULL;
	struct bvchrdata* bvchr = NULL;
	bool hastarget = bvslendermanHasTarget();
	bool insight = false;
	bool invicinity = false;

	if (hastarget) {
		targetchr = bvslendermanGetTargetChr();
		bvchr = bvGetChrMatchData(targetchr);

		// Target is past the rush threshold, rush em
		if (bvchr->slendermanaggro >= 2) {
			g_BvMatch.slendermanspeedmult = speedmult_rush;
			return;
		}
	}

	// If standard aggro, move normally
	if (bvslendermanIsAggro()) {
		g_BvMatch.slendermanspeedmult = 1.0f;
		return;
	}

	if (hastarget) {
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
		invicinity = bvchr->slendermandist < distthreshold_noapproachuntilrush;
		
		// Slenderman has to stalk his target from a distance for a while before attacking
		// Slenderman has to freeze if in sight until he hits the rush threshold
		if ((insight || invicinity) && bvchr->slendermanvictimprogress > progressthreshold_cantmoveifonscreen) {
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
* Tick Slenderman victimprogress for non-Slenderman characters:
*  + Tick down victimprogress after Slenderman despawns.
*  + Update slendermanonscreen, slendermanhaslos, and slendermandist values
*  + Tick victimprogress up or down appropriately
*  + Initiate rush when progress crosses the relevant threshold by setting aggro to 2
*/
void bvslendermanTickOtherVictimProgress(struct chrdata* chr, struct bvchrdata* bvchr) {
	s32 chrindex = mpPlayerGetIndex(chr);
	f32 progresslimit = 0;

	// Slenderman has despawned, tick down victim progress quickly and we're done
	if (g_BvMatch.slendermanchr == NULL) {
		if (bvchr->slendermanvictimprogress > 0) {
			bvslendermanIncrementVictimProgress(bvchr, -2.0f, 0, progress_max);
		}
		return;
	} // *** Slenderman known to be spawned past this point ***

	// Get our status relative to Slenderman
	bvchr->slendermanhaslos = g_BvMatch.slendermanchr->aibot->chrsinsight[chrindex];
	if (!chr->aibot) { // players only: check if this player's screen
		bvchr->slendermanonscreen = (g_BvMatch.slendermanchr->prop->flags & PROPFLAG_ONTHISSCREENTHISTICK && bvchr->slendermanhaslos);
	}
	bvchr->slendermandist = g_BvMatch.slendermanchr->aibot->chrdistances[chrindex];

	progresslimit = bvslendermanGetProgressLimit(chr, bvchr);

	// If beyond the limit, tick down but not further
	if (bvchr->slendermanvictimprogress > progresslimit) {
		bvslendermanIncrementVictimProgress(bvchr, -2.0f, progresslimit, bvchr->slendermanvictimprogress);
	}
	// Below the limit, tick up to it
	else if (bvchr->slendermanvictimprogress < progresslimit) {
		bvslendermanIncrementVictimProgress(bvchr, 1.0f, bvchr->slendermanvictimprogress, progresslimit);
	}

	// If slenderman's ready to rush the victim, set aggro
	if (bvchr->slendermanvictimprogress >= progressthreshold_rush) {
		bvchr->slendermanaggro = 2;
	}
}

/**
* Updates Slenderman's opacity for the given chr based on their victimprogress.
*/
void bvslendermanUpdateOpacityForChr(struct chrdata* chr, struct bvchrdata* bvchr) {
	bool slendermanalive = bvslendermanIsAlive();
	bool christarget = (bvslendermanGetTargetChr() == chr);
	bool unaggroed = !bvslendermanIsAggro();

	// If we're slenderman's target and he's alive and unaggroedd, his opacity is determined by our own victimprogress
	if (slendermanalive && unaggroed && christarget) {
		if (bvchr->slendermanvictimprogress <= progressthreshold_opacitystart) {
			bvchr->slendermanopacity = 0;
		}
		else if (bvchr->slendermanvictimprogress >= progressthreshold_opacitymax) {
			bvchr->slendermanopacity = (f32)maxopacity;
		}
		else {
			bvchr->slendermanopacity = ((f32)maxopacity * ((bvchr->slendermanvictimprogress - progressthreshold_opacitystart) / (progressthreshold_opacitymax - progressthreshold_opacitystart)));
		}
	}
	// If slenderman's aggroed or dead, tick opacity up
	else if ((!unaggroed || !slendermanalive) && bvchr->slendermanopacity < 255.0f) {
		bvchr->slendermanopacity += (0.016666f * opacityuprate_dead * g_Vars.lvupdate60freal);
		if (bvchr->slendermanopacity > 255.0f) {
			bvchr->slendermanopacity = 255.0f;
		}
	}
	else { // tick opacity down
		if (bvchr->slendermanopacity > 0) {
			bvchr->slendermanopacity -= (0.016666f * opacityuprate_dead * g_Vars.lvupdate60freal);
			if (bvchr->slendermanopacity < 0) {
				bvchr->slendermanopacity = 0;
			}
		}
	}
	
}

/**
* Tick Slenderman effects on living non-Slenderman characters:
*  + Tick victimprogress up and down as appropriate
*  + Update slendermanonscreen, slendermanhaslos, and slendermandist values
*  + Updates slenderman's opacity from this chr's perspective (used for alpha on players' screens, and whether visible to other bots)
*/
void bvslendermanTickOtherChr(struct chrdata* chr) {
	struct bvchrdata* bvchr = bvGetChrMatchData(chr);
	
	bvslendermanTickOtherVictimProgress(chr, bvchr); // also updates slendermanonscreen, slendermanhaslos, and slendermandist
	bvslendermanUpdateOpacityForChr(chr, bvchr);
}

/**
* Applies a varying amount of static to the current player's screen based on their victimprogress timer.
* Decays static level if player is dead.
*/
Gfx *bvslendermanApplyVictimStatic(Gfx *gdl) {
	//return bviewDrawStatic(gdl, 0x4fffffff, 255);

	u32 staticlevel;
	struct bvchrdata* bvchr = bvGetChrMatchData(g_Vars.currentplayer->prop->chr);
	f32 progress = bvchr->slendermanvictimprogress;

	if (progress <= progressthreshold_minstatic) {
		return gdl;
	}
	if (g_Vars.currentplayer->isdead) {
		bvslendermanIncrementVictimProgress(bvchr, -1.0f, 0, progress_max); // dissolve static while dying
	}

	staticlevel = (u32)((f32)maxstaticamount * ((progress - progressthreshold_minstatic) / (progressthreshold_maxstatic - progressthreshold_minstatic)));

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
	struct bvchrdata* bvchr = bvGetChrMatchData(currentplayerchr);
	
	alpha = (u32)bvchr->slendermanopacity;

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

Gfx *bvslendermanDebugRenderProgress(Gfx *gdl)
{
	f32 progress = bvGetChrMatchData(g_Vars.currentplayer->prop->chr)->slendermanvictimprogress;
	s32 x = viGetViewLeft() + 27;
	s32 y = viGetViewTop() + 13;
	//x *= (g_Vars.currentplayerindex + 1);
	u32 color = 0x00ff00a0;
	char buffer[16];

	if (g_CharsNumeric && g_FontNumeric) {
		snprintf(buffer, sizeof buffer, "%.2f", progress);

		gSPSetExtraGeometryModeEXT(gdl++, g_HudAlignModeL);

		gdl = text0f153628(gdl);
		gdl = textRender(gdl, &x, &y, buffer, g_CharsNumeric, g_FontNumeric, color, 0x000000a0, viGetWidth(), viGetHeight(), 0, 0);
		gdl = text0f153780(gdl);

		gSPClearExtraGeometryModeEXT(gdl++, g_HudAlignModeL);
	}

	return gdl;
}

