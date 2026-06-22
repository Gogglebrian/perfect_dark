#include <ultra64.h>
#include "constants.h"
#include "game/body.h"
#include "game/chr.h"
#include "game/mod/botvariety.h"
#include "game/modelmgr.h"
#include "game/mplayer/mplayer.h"
#include "game/propobj.h"
#include "game/title.h"
#include "lib/model.h"
#include "lib/rng.h"
#include "bss.h"

#define CHR_BOTVARIETY_FLAGS chr->convtalk // This u32 isn't used in combat simulator so we'll hackily borrow it

/// <summary>
/// Gets the initial body scale of the original model of this chr.
/// </summary>
f32 bvGetInit3DBodyScale(struct chrdata* chr) {
	return bvGetChrMatchData(chr)->initscale;
}

/// <summary>
/// True if there's a spree going on of any variant
/// </summary>
bool bvIsAnyoneSpreeing() {
	for (u8 i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		if (g_BvMatch.variantspreespawnsleft[i] > 0) {
			return true;
		}
	}

	return false;
}

/// <summary>
/// True if the variant at the passed index has a spree ongoing
/// </summary>
bool bvIsSpreeing(u8 variantIndex) {
	return (g_BvMatch.variantspreespawnsleft[variantIndex] > 0);
}

/// <summary>
/// Get the number of times this variant has to spawn before the spree ends
/// </summary>
u16 bvGetSpreeCountRemaining(u8 variantIndex) {
	return g_BvMatch.variantspreespawnsleft[variantIndex];
}

/// <summary>
/// Get the f32 spree chance for this variant depending on whether debug is enabled
/// </summary>
f32 bvGetSpreeChance(const struct bvvariant* variant) {
	if (g_BvDebugSprees) {
		return variant->spree.triggerchancedebug;
	}
	else {
		return variant->spree.triggerchance;
	}
}

/// <summary>
/// Select which of the variant's f32 chance values to use depending on whether player or bot, whether a spree is active, and whether debug is enabled.
/// </summary>
f32 bvGetSpawnChance(struct chrdata* chr, const struct bvvariant* variant, bool iscurrentplayer) {
	if (!iscurrentplayer && bvIsSpreeing(variant->index)) {
		return variant->spawnchance.spree;
	}
	else if (g_BvDebug) {
		return variant->spawnchance.debug;
	}
	else if (iscurrentplayer) {
		return variant->spawnchance.player;
	}
	else {
		return variant->spawnchance.bot;
	}
}

/// <summary>
/// Returns true if a bot has a major size variant: Mini, Wumbo
/// </summary>
bool bvHasSizeVariant(struct chrdata* chr) {
	return (CHR_BOTVARIETY_FLAGS & (BOTVARIETY_FLAG_MINI | BOTVARIETY_FLAG_WUMBO));
}

/// <summary>
/// Returns true if a bot has an Abomination variant: Superbattledroid, more to come
/// </summary>
bool bvIsAbomination(struct chrdata* chr) {
	return (CHR_BOTVARIETY_FLAGS & (BOTVARIETY_FLAG_SUPERBATTLEDROID));
}

/// <summary>
/// Enables or disables the sunglasses on the model, if the model has sunglasses.
/// Returns true if sunglasses successfully enabled, false if not enabled.
/// </summary>
bool bvspawnTryApplySunglasses(struct chrdata* chr, bool enabled) {
	struct model* model = bvGetModel(chr);
	struct modeldef* headmodeldef = g_HeadsAndBodies[chr->headnum].modeldef;
	struct modelnode* node = NULL;

	if (headmodeldef && model) {
		node = modelGetPart(headmodeldef, MODELPART_HEAD_SUNGLASSES);

		if (node) {
			union modelrwdata* rwdata = modelGetNodeRwData(model, node);

			if (rwdata) {
				rwdata->toggle.visible = enabled;
				return true;
			}
		}
	}
	return false;
}

/// <summary>
/// Does this model/head have a sunglasses node?
/// </summary>
bool bvCanChrWearSunglasses(struct chrdata* chr) {
	struct model* model = bvGetModel(chr);
	struct modeldef* headmodeldef = g_HeadsAndBodies[chr->headnum].modeldef;
	struct modelnode* node;

	if (model && headmodeldef) {
		node = modelGetPart(headmodeldef, MODELPART_HEAD_SUNGLASSES);

		if (node) {
			return true;
		}
	}

	return false;
}

/// <summary>
/// Rolls for and applies Sunglasses variant (including minor gameplay bonuses).
/// Returns true if sunglasses are enabled.
/// </summary>
bool bvspawnHandleSunglasses(struct chrdata* chr, f32 sunglasseschance) {
	if (bvCanChrWearSunglasses(chr)) {
		if (RANDOMFRAC() < sunglasseschance) { // Roll
			if (bvspawnTryApplySunglasses(chr, true)) { // enable sunglasses
				CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_SUNGLASSES; // set flag for gameplay bonuses
				return true;
			}
		}
		else {
			bvspawnTryApplySunglasses(chr, false); // disable sunglasses
		}
	}
	return false;
}

/// <summary>
/// Reverts a bot's model back to its original model, if necessary 
/// </summary>
void bvspawnTryRevertToInitModel(struct chrdata* chr) {
	s16 botnum = chr->aibot->aibotnum;
	struct bvchrdata * bvchr = &g_BvMatch.bots[botnum]; 
	struct model* initmodel = bvchr->initmodel;

// If necessary, revert previous model
	if (chr->model != initmodel) {
		struct model* altmodel = chr->model;
		chr->model = initmodel;
		chr->headnum = mpGetHeadId(g_BotConfigsArray[botnum].base.mpheadnum);
		chr->bodynum = mpGetBodyId(g_BotConfigsArray[botnum].base.mpbodynum);
		chr->race = bodyGetRace(chr->bodynum);

		modelFreeVertices(VTXSTORETYPE_CHRVTX, altmodel);
		modelmgrFreeModel(altmodel);

		// If we were impstor, mark that we aren't anymore
		if (bvchr->impostorof >= 0) {
			bvchr->impostorof = -1;
		}
	}
}

/// <summary>
/// Changes a chr's model to the new bodynum and headnum.
/// Returns true if successfully applied.
/// </summary>
bool bvspawnTryApplyModelChange(struct chrdata* chr, s16 bodynum, s16 headnum) {
	struct model* newmodel = bodyAllocateModel(bodynum, headnum, 0);
	if (newmodel) {
		// Now get the model ready for primetime
		modelSetAnim70(newmodel, chr0f01f378);
		newmodel->chr = chr;
		newmodel->unk01 = 1;
		modelSetAnimPlaySpeed(newmodel, PALUPF(var80062968), 0);
		modelSetRootPosition(newmodel, &chr->prop->pos);

		// Assign new model to bot's chr
		chr->model = newmodel;
		chr->headnum = headnum;
		chr->bodynum = bodynum;
		chr->race = bodyGetRace(bodynum);

		return true;
	}
	return false;
}

/// <summary>
/// Make a bot character an impostor by swapping their model to match one of the human players'.
/// Returns true if successfully applied.
/// </summary>
bool bvspawnTryApplyImpostor(struct chrdata* chr) {
	u8 copyplayerindex;
	struct chrdata* copyplayerchr;

	// select a player at random
	copyplayerindex = rngRandom() % getNumPlayers();
	copyplayerchr = mpGetChrFromPlayerIndex(copyplayerindex);

	if (bvIsChrBond(copyplayerchr)) { // band-aid for bond models causing seg faults when trying to apply sunglasses to OTHER chrs
		return false;
	}

	// Try to allocate and switch to a new model copying the player
	if (bvspawnTryApplyModelChange(chr, copyplayerchr->bodynum, copyplayerchr->headnum)) {
		// Success: Mark that we're impersonating the player
		g_BvMatch.bots[chr->aibot->aibotnum].impostorof = copyplayerindex;
		return true;
	}

	return false;
}

/// <summary>
/// Rolls for and applies Impostor variant - copying one of the player's models.
/// </summary>
bool bvspawnHandleImpostor(struct chrdata* chr, f32 impostorchance) {
	if (RANDOMFRAC() < impostorchance) { // roll
		if (bvspawnTryApplyImpostor(chr)) { // try to apply
			CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_IMPOSTOR; // set flag for gameplay bonuses
			return true;
		}
	}

	return false; // failed roll or failed to initialize
}


/// <summary>
/// Rolls for and applies Slenderman variant - wear a Bond suit and be real tall
/// </summary>
bool bvspawnHandleSlenderman(struct chrdata* chr, f32 slendermanchance) {
	if (RANDOMFRAC() < slendermanchance) { // roll
		if (bvspawnTryApplyModelChange(chr, BODY_PRESIDENT, chr->headnum)) { // try to apply model change
			CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_SLENDERMAN; // set flag for gameplay bonuses
			return true;
		}
	}

	return false; // failed roll or failed to initialize
}

/// <summary>
/// Scales the chr's body with regard to its applicable botvariety flags and applies minor random height variance.
/// </summary>
void bvspawnApply3DBodyScale(struct chrdata* chr) {
	f32 scale = bvGetInit3DBodyScale(chr);
	f32 randfracsizevariance = RANDOMFRAC(); // minor height variance e.g. 95%-105%
	const struct bvvariant* variant = NULL;
	u8 i;

	// Apply major variant body scaling
	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = &gc_BvVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag && variant->body.scalebody > 0) {
			scale *= variant->body.scalebody;
		}
	}

	// Apply minor height variance, which differs a bit for mini and wumbo
	// Mini
	if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_MINI) {
		scale *= randfracsizevariance * 0.05f + 0.975f; // Apply minor height variance between 97.5% and 102.5%
	}
	// Wumbo
	else if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_WUMBO) {
		scale *= randfracsizevariance * 0.05f + 0.95f; // Apply random height variance between 95% and 100%
	}
	// Normal
	else if (g_HeadsAndBodies[chr->bodynum].canvaryheight) {
		scale *= randfracsizevariance * 0.1f + 0.95f; // Apply random height variance between 95% and 105%
	}

	// Apply body scale
	modelSetScale(bvGetModel(chr), scale);
}

/// <summary>
/// Rolls for and sets flags for size variants: Mini and Wumbo
/// </summary>
void bvspawnHandleSize(struct chrdata* chr, f32 minichance, f32 wumbochance) {
	f32 randfracsizevariant= RANDOMFRAC(); // mini or wumbo;
	
	// Mini
	if (randfracsizevariant < minichance) {
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_MINI;
	}
	// Wumbo
	else if (randfracsizevariant > (1 - wumbochance)) {
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_WUMBO;
	}
}

void debugSpree(){
	u16 spree_impsotor = g_BvMatch.variantspreespawnsleft[INDEX_IMPOSTOR];
	u16 spree_mini = g_BvMatch.variantspreespawnsleft[INDEX_MINI];
	u16 spree_wumbo = g_BvMatch.variantspreespawnsleft[INDEX_WUMBO];
	u16 spree_sunglasses = g_BvMatch.variantspreespawnsleft[INDEX_SUNGLASSES];
}

/// <summary>
/// Tick down the spree spawns remaining for any sprees that this chr is a part of.
/// </summary>
void bvspawnCountAgainstSpreeSpawns(struct chrdata* chr) {
	bool changed = false;
	for (u8 i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		if (bvIsSpreeing(i) && CHR_BOTVARIETY_FLAGS & gc_BvVariants[i].flag) {
			g_BvMatch.variantspreespawnsleft[i]--;
			changed = true;
		}
	}
	if (g_BvDebugSprees && changed) {
		debugSpree();
	}
}

/// <summary>
/// Starts a spree by determining and setting the number of times the variant will spawn before the spree ends.
/// </summary>
void bvspawnStartSpree(u8 variantIndex) {
	const struct bvvariant * variant = &gc_BvVariants[variantIndex];
	u16 min = variant->spree.minspawncount;
	u16 max = variant->spree.maxspawncount;
	u16 count = min + (rngRandom() % (max + 1 - min));
	g_BvMatch.variantspreespawnsleft[variantIndex] = count;

}

/// <summary>
/// Rolls for and starts variant spawning sprees as appropriate.
/// </summary>
void bvspawnHandleStartingSprees() {
	bool impostorspree = false;

	// Rather than loop through, I'm gonna handle each separately in turn,
	// because some sprees might affect the chances of other sprees
	// and so on and so forth.

	// Impostor spree
	if (!bvIsSpreeing(INDEX_IMPOSTOR) && RANDOMFRAC() < bvGetSpreeChance(&VARIANT_IMPOSTOR)) {
		bvspawnStartSpree(INDEX_IMPOSTOR);
	}

	// Mini/wumbo sprees -- only start either if neither is already spreeing
	if (!bvIsSpreeing(INDEX_MINI) && !bvIsSpreeing(INDEX_WUMBO)) {
		f32 randfracsizespree = RANDOMFRAC();
		if (randfracsizespree < bvGetSpreeChance(&VARIANT_MINI)) {
			bvspawnStartSpree(INDEX_MINI);
		}
		else if (randfracsizespree > (1.0f - bvGetSpreeChance(&VARIANT_WUMBO))) {
			bvspawnStartSpree(INDEX_WUMBO);
		}
	}

	// Sunglasses
	if (!bvIsSpreeing(INDEX_SUNGLASSES) && RANDOMFRAC() < bvGetSpreeChance(&VARIANT_SUNGLASSES)) {
		bvspawnStartSpree(INDEX_SUNGLASSES);
	}

	// Explosive
	if (!bvIsSpreeing(INDEX_EXPLOSIVE) && RANDOMFRAC() < bvGetSpreeChance(&VARIANT_EXPLOSIVE)) {
		bvspawnStartSpree(INDEX_EXPLOSIVE);
	}
}

/// <summary>
/// Rolls for and applies the flag for, at most, one Abomination variant.
/// </summary>
void bvspawnHandleAbominations(struct chrdata* chr, f32 chancemult, bool iscurrentplayer) {
	u8 variantchoiceoffset = rngRandom() % ABOMINATION_COUNT;
	u8 variantchoice;
	u8 i;
	f32 chance;

	for (i = 0; i < ABOMINATION_COUNT; i++) {
		variantchoice = variantchoiceoffset + INDEX_ABOMINATION_FIRST;
		chance = bvGetSpawnChance(chr, &gc_BvVariants[variantchoice], iscurrentplayer);
		chance *= chancemult;
		if (RANDOMFRAC() < chance) {
			CHR_BOTVARIETY_FLAGS |= gc_BvVariants[variantchoice].flag;
			break;	
		}

		variantchoiceoffset = (variantchoiceoffset + 1) % ABOMINATION_COUNT;
	} 
}

/// <summary>
/// Rolls for and applies the flag for the explosive variant
/// returns true if explosive flag applied
/// </summary>
bool bvspawnHandleExplosive(struct chrdata* chr, f32 explosivechance) {
	if (RANDOMFRAC() < explosivechance) { // roll
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_EXPLOSIVE;
		bvResetExplosiveBot(chr);
		return true;
	}

	return false; // failed roll
}

/// <summary>
/// Call when a bot or player spawns/respawns to roll for variants, set flags, and apply
/// initial changes such as model changes, body scaling, and sunglasses.
/// </summary>
void bvspawnPrepVariety(struct chrdata* chr, bool iscurrentplayer) {
	f32 impostorchance, slendermanchance, minichance, wumbochance, sunglasseschance, explosivechance;
	f32 abominationchancemult = 1.0f;
	bool isimpostor = false;
	
	CHR_BOTVARIETY_FLAGS = 0; // Reset chr's variant flags

	// Initialize bvchrdata on this chr's first spawn of the match
	bool firstspawn = bvTryInitChr(chr, iscurrentplayer);
	
	// On subsequent spawns for each bot we'll roll for sprees
	if (!firstspawn && !iscurrentplayer) {
		bvspawnHandleStartingSprees();
	}

	// Get base chances for each variant (depending on player/bot/debug)
	impostorchance   = bvGetSpawnChance(chr, &VARIANT_IMPOSTOR,   iscurrentplayer);
	slendermanchance = bvGetSpawnChance(chr, &VARIANT_SLENDERMAN, iscurrentplayer);
	minichance       = bvGetSpawnChance(chr, &VARIANT_MINI,       iscurrentplayer);
	wumbochance      = bvGetSpawnChance(chr, &VARIANT_WUMBO,      iscurrentplayer);
	sunglasseschance = bvGetSpawnChance(chr, &VARIANT_SUNGLASSES, iscurrentplayer);
	explosivechance  = bvGetSpawnChance(chr, &VARIANT_EXPLOSIVE,  iscurrentplayer);
	//slendermanchance = 0;

	union modelrwdata* rwdata = NULL;
	struct model* model = chr->model;
	struct modeldef* headmodeldef = g_HeadsAndBodies[chr->headnum].modeldef;
	struct modeldef* bodymodeldef = g_HeadsAndBodies[chr->bodynum].modeldef;
	struct modelnode* node = modelGetPart(headmodeldef, MODELPART_HEAD_SUNGLASSES);
	if (node && headmodeldef) {
		rwdata = modelGetNodeRwData(model, node);
	}

	// Handle model changes first -- bots only
if (!iscurrentplayer) {
		bvspawnTryRevertToInitModel(chr); // reset any prior model changes

		// Impostors - copy one of the player's models
		if (bvspawnHandleImpostor(chr, impostorchance)) {
			// Impostors are more likely to have other variants, but only if there's no ongoing spree
			if (!bvIsSpreeing(INDEX_MINI) && !bvIsSpreeing(INDEX_WUMBO)) {
				minichance = 0.333f;
				wumbochance = 0.333f;
				abominationchancemult = 2.0f;
			}
			isimpostor = true;
		} else if (bvspawnHandleSlenderman(chr, slendermanchance)) {
			wumbochance = 0; // no slenderwumbos, that'd be too darn tall
			minichance = 0; // temporary?
			abominationchancemult = 0; // also no slenderbominations, that'd be too upsetting
			explosivechance = 0;
		}
	}

	// Size variants (mini/wumbo)
	bvspawnHandleSize(chr, minichance, wumbochance);

	// Other bot-only variants
	if (!iscurrentplayer || g_BvDebugAllowPlayerAbominations) {
		// Abominations - rare, freaky body changes (normally bots only)
		bvspawnHandleAbominations(chr, abominationchancemult, iscurrentplayer);
	}
	if (!iscurrentplayer) {
		// Exploding bots
		bvspawnHandleExplosive(chr, explosivechance);
	}

	// an impstor that doesn't have a size variant should wear sunglasses if possible
	if (isimpostor && !bvHasSizeVariant(chr)) {
		sunglasseschance = 1.0f;
	}

	// Sunglasses - this is done after model changes so we can check for sunglassability
	bvspawnHandleSunglasses(chr, sunglasseschance);

	// Apply major variants' body scale multipliers and minor height variance
	bvspawnApply3DBodyScale(chr);

	// If bot spawned as part of a spree,
	if (!iscurrentplayer) {
		bvspawnCountAgainstSpreeSpawns(chr);
	}
}

#undef CHR_BOTVARIETY_FLAGS
