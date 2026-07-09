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

/**
* Gets the initial body scale of the original model of this chr.
*/
f32 bvGetInit3DBodyScale(struct chrdata* chr) {
	return chr->bvchr->initscale;
}

/**
* True if there's a spree going on of any variant
*/
bool bvIsAnyoneSpreeing() {
	for (u8 i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		if (g_BvMatch.variantspreespawnsleft[i] > 0) {
			return true;
		}
	}

	return false;
}

/**
* True if the variant at the passed index has a spree ongoing
*/
bool bvIsSpreeing(u8 variantIndex) {
	return (g_BvMatch.variantspreespawnsleft[variantIndex] > 0);
}

/**
* True if the variant at the passed index is on cooldown from a prior spree.
*/
bool bvIsOnSpreeCooldown(u8 variantIndex) {
	return (g_BvSpreeCooldowns[variantIndex] > 0);
}

/**
* True if the variant is free to start a new spree: not currently spreeing, and not on cooldown.
*/
bool bvCanStartSpree(u8 variantIndex) {
	return (!bvIsSpreeing(variantIndex) && !bvIsOnSpreeCooldown(variantIndex));
}

/**
* Get the number of times this variant has to spawn before the spree ends
*/
u16 bvGetSpreeCountRemaining(u8 variantIndex) {
	return g_BvMatch.variantspreespawnsleft[variantIndex];
}

/**
* Get the f32 spree chance for this variant depending on whether debug is enabled
*/
f32 bvGetSpreeChance(const struct bvvariant* variant) {
	if (g_BvDebugSprees) {
		return variant->spree.triggerchancedebug;
	}
	else {
		return variant->spree.triggerchance;
	}
}

/**
* Select which of the variant's f32 chance values to use depending on whether player or bot, whether a spree is active, and whether debug is enabled.
*/
f32 bvGetSpawnChance(struct chrdata* chr, const struct bvvariant* variant, bool iscurrentplayer) {
	if (!iscurrentplayer && bvIsSpreeing(variant->index)) {
		return variant->spawnchance.spree;
	}
	else if (g_BvDebugAllVariants || variant->debug) {
		return variant->spawnchance.debug;
	}
	else if (iscurrentplayer) {
		return variant->spawnchance.player;
	}
	else {
		return variant->spawnchance.bot;
	}
}

/**
* Given the index of a variant, selects which of that variant's f32 chance values to use depending on whether player or bot, whether a spree is active, and whether debug is enabled.
*/
f32 bvGetSpawnChanceByIndex(struct chrdata* chr, u8 variantindex, bool iscurrentplayer) {
	return bvGetSpawnChance(chr, bvGetVariant(variantindex), iscurrentplayer);
}

/**
* Returns true if a chr has a major size variant: Mini, Wumbo
*/
bool bvChrHasSizeVariant(struct chrdata* chr) {
	return (CHR_BV_FLAGS & (BVFLAG_MINI | BVFLAG_WUMBO));
}

/**
* Returns true if a bot has an Abomination variant: Superbattledroid, more to come
*/
bool bvIsChrAbomination(struct chrdata* chr) {
	for (u8 i = BVINDEX_ABOMINATION_FIRST; i <= BVINDEX_ABOMINATION_LAST; i++) {
		if (CHR_BV_FLAGS & bvGetVariant(i)->flag) {
			return true;
		}
	}
	return false;
}

/**
* Enables or disables the sunglasses on the model, if the model has sunglasses.
* Returns true if sunglasses successfully enabled, false if not enabled.
*/
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

/**
* Does this model/head have a sunglasses node?
*/
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

/**
* Rolls for and applies Sunglasses variant (including minor gameplay bonuses).
* Returns true if sunglasses are enabled.
*/
bool bvspawnHandleSunglasses(struct chrdata* chr, f32 sunglasseschance) {
	if (bvCanChrWearSunglasses(chr)) {
		if (sunglasseschance > 0 && RANDOMFRAC() < sunglasseschance) { // Roll
			if (bvspawnTryApplySunglasses(chr, true)) { // enable sunglasses
				CHR_BV_FLAGS |= BVFLAG_SUNGLASSES; // set flag for gameplay bonuses
				return true;
			}
		}
		else {
			bvspawnTryApplySunglasses(chr, false); // disable sunglasses
		}
	}
	return false;
}

/**
* Reverts a bot's model back to its original model, if necessary 
*/
void bvspawnTryRevertToInitModel(struct chrdata* chr) {
	s16 botnum = chr->aibot->aibotnum;
	struct model* initmodel = chr->bvbot->initmodel;

	// If necessary, revert previous model
	if (chr->model != initmodel) {
		struct model* altmodel = chr->model;

		// re-prepare original model - some of this is probably unnecessary, but trying to hedge against unexpected/hard-to-debug shenanigans
		animInit(initmodel->anim);
		modelSetAnim70(initmodel, chr0f01f378);
		modelSetAnimPlaySpeed(initmodel, PALUPF(var80062968), 0);
		modelSetRootPosition(initmodel, &chr->prop->pos);
		initmodel->chr = chr;
		initmodel->unk01 = 1;
		modelSetAnimation(initmodel, ANIM_006A, 0, 0.0f, 0.5f, 0.0f);

		chr->model = initmodel;
		chr->headnum = mpGetHeadId(g_BotConfigsArray[botnum].base.mpheadnum);
		chr->bodynum = mpGetBodyId(g_BotConfigsArray[botnum].base.mpbodynum);
		chr->race = bodyGetRace(chr->bodynum);

		modelFreeVertices(VTXSTORETYPE_CHRVTX, altmodel);
		modelmgrFreeModel(altmodel);

		// If we were impstor, mark that we aren't anymore
		if (chr->bvbot->impostorof >= 0) {
			chr->bvbot->impostorof = -1;
		}
	}
}

/**
* Changes a chr's model to the new bodynum and headnum.
* Returns true if successfully applied.
*/
bool bvspawnTryApplyModelChange(struct chrdata* chr, s16 bodynum, s16 headnum) {
	struct model* newmodel = bodyAllocateModel(bodynum, headnum, 0);
	if (newmodel) {
		struct modelnode *rootnode = newmodel->definition->rootnode;
		struct modelrwdata_chrinfo *rwdata = modelGetNodeRwData(newmodel, rootnode);

		// Now get the model ready for primetime - try to replicate the exact circumstances of a conventionally-allocated model to hedge against unexpected/hard-to-debug shenanigans
		modelSetAnim70(newmodel, chr0f01f378);
		newmodel->chr = chr;
		newmodel->unk01 = 1;
		rwdata->unk01 = 1;
		modelSetAnimPlaySpeed(newmodel, PALUPF(var80062968), 0);
		modelSetRootPosition(newmodel, &chr->prop->pos);
		modelSetAnimation(newmodel, ANIM_006A, 0, 0.0f, 0.5f, 0.0f);

		// Assign new model to bot's chr
		chr->model = newmodel;
		chr->headnum = headnum;
		chr->bodynum = bodynum;
		chr->race = bodyGetRace(bodynum);

		return true;
	}
	return false;
}

/**
* Make a bot character an impostor by swapping their model to match one of the human players'.
* Returns true if successfully applied.
*/
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
		chr->bvbot->impostorof = copyplayerindex;
		return true;
	}

	return false;
}

/**
* Rolls for and applies Impostor variant - copying one of the player's models.
*/
bool bvspawnHandleImpostor(struct chrdata* chr, f32 impostorchance) {
	if (impostorchance > 0 && RANDOMFRAC() < impostorchance) { // roll
		if (bvspawnTryApplyImpostor(chr)) { // try to apply
			CHR_BV_FLAGS |= BVFLAG_IMPOSTOR; // set flag for gameplay bonuses
			return true;
		}
	}

	return false; // failed roll or failed to initialize
}


/**
* Rolls for and applies Slenderman variant - wear a Bond suit and be real tall, and attack with spooky analog horror static.
* See botvarietyslenderman.c for functionality overview.
*/
bool bvspawnHandleSlenderman(struct chrdata* chr, f32 slendermanchance, bool isimpostor) {
	if (slendermanchance > 0 && RANDOMFRAC() < slendermanchance) { // roll
		// If chr is already an impostor, then don't change the model again
		if (isimpostor) { 
			CHR_BV_FLAGS |= BVFLAG_SLENDERMAN; // set flag for gameplay bonuses
			return true;
		}
		// Otherwise under normal circumstances, put a classy suit on em
		else if (bvspawnTryApplyModelChange(chr, BODY_PRESIDENT, chr->headnum)) { // try to apply model change
			CHR_BV_FLAGS |= BVFLAG_SLENDERMAN; // set flag for gameplay bonuses
			return true;
		}
	}

	return false; // failed roll or failed to initialize
}

/**
* Scales the chr's body with regard to its applicable botvariety flags and applies minor random height variance.
*/
void bvspawnApply3DBodyScale(struct chrdata* chr) {
	f32 scale = bvGetInit3DBodyScale(chr);
	f32 randfracsizevariance = RANDOMFRAC(); // minor height variance e.g. 95%-105%
	const struct bvvariant* variant = NULL;
	u8 i;

	// Apply major variant body scaling
	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = bvGetVariant(i);

		if (CHR_BV_FLAGS & variant->flag && variant->body && variant->body->scalebody > 0) {
			scale *= variant->body->scalebody;
		}
	}

	// Apply minor height variance, which differs a bit for mini and wumbo
	// Mini
	if (CHR_BV_FLAGS & BVFLAG_MINI) {
		scale *= randfracsizevariance * 0.05f + 0.975f; // Apply minor height variance between 97.5% and 102.5%
	}
	// Wumbo
	else if (CHR_BV_FLAGS & BVFLAG_WUMBO) {
		scale *= randfracsizevariance * 0.05f + 0.95f; // Apply random height variance between 95% and 100%
	}
	// Normal
	else if (g_HeadsAndBodies[chr->bodynum].canvaryheight) {
		scale *= randfracsizevariance * 0.1f + 0.95f; // Apply random height variance between 95% and 105%
	}

	// Apply body scale
	modelSetScale(bvGetModel(chr), scale);
}

/**
* Rolls for and sets flags for size variants: Mini and Wumbo
*/
void bvspawnHandleSize(struct chrdata* chr, f32 minichance, f32 wumbochance) {
	f32 randfracsizevariant = RANDOMFRAC(); // mini or wumbo;
	
	// Mini
	if (randfracsizevariant < minichance) {
		CHR_BV_FLAGS |= BVFLAG_MINI;
	}
	// Wumbo
	else if (randfracsizevariant > (1.0f - wumbochance)) {
		CHR_BV_FLAGS |= BVFLAG_WUMBO;
	}
}

// Index shorthand for the rest of the spawn logic, undef'd at eof
#define MINI       BVINDEX_MINI
#define WUMBO      BVINDEX_WUMBO
#define IMPOSTOR   BVINDEX_IMPOSTOR
#define SLENDERMAN BVINDEX_SLENDERMAN
#define SUNGLASSES BVINDEX_SUNGLASSES
#define EXPLOSIVE  BVINDEX_EXPLOSIVE
#define GUNFETTI   BVINDEX_GUNFETTI
#define SBD        BVINDEX_SBD

/**
* workaround for my debugger not showing global variables
*/
void debugSpree(){
	u16 spree_impostor = g_BvMatch.variantspreespawnsleft[IMPOSTOR];
	u16 spree_mini = g_BvMatch.variantspreespawnsleft[MINI];
	u16 spree_wumbo = g_BvMatch.variantspreespawnsleft[WUMBO];
	u16 spree_sunglasses = g_BvMatch.variantspreespawnsleft[SUNGLASSES];
	u16 spree_explosive = g_BvMatch.variantspreespawnsleft[EXPLOSIVE];

	u16 cooldown_impostor = g_BvSpreeCooldowns[IMPOSTOR];
	u16 cooldown_mini = g_BvSpreeCooldowns[MINI];
	u16 cooldown_wumbo = g_BvSpreeCooldowns[WUMBO];
	u16 cooldown_sunglasses = g_BvSpreeCooldowns[SUNGLASSES];
	u16 cooldown_explosive = g_BvSpreeCooldowns[EXPLOSIVE];
	;
}

/**
* Tick down the spree spawns remaining for any sprees that this chr is a part of.
* Also tick down the cooldown counters of any variants on cooldown from a prior spree.
*/
void bvspawnCountAgainstSpreeSpawns(struct chrdata* chr) {
	bool changed = false;
	for (u8 i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		if (bvIsSpreeing(i)) {
			if (CHR_BV_FLAGS & bvGetVariant(i)->flag) {
				g_BvMatch.variantspreespawnsleft[i]--;
				changed = true;
			}
		}
		else if (bvIsOnSpreeCooldown(i)) { // not spreeing and on cooldown
			g_BvSpreeCooldowns[i]--; // decrement cooldown counter for every bot spawn regardless of flags
			changed = true;
		}
	}
	if (g_BvDebugSprees && changed) {
		debugSpree();
	}
}

/**
* Starts a spree by determining and setting the number of times the variant will spawn before the spree ends.
* Also initializes the spree cooldown to the same number, but it won't begin counting down until the spree is over.
*/
void bvspawnStartSpree(u8 variantIndex) {
	const struct bvvariant * variant = bvGetVariant(variantIndex);
	u16 min = variant->spree.minspawncount;
	u16 max = variant->spree.maxspawncount;
	u16 count = min + (rngRandom() % (max + 1 - min));
	g_BvMatch.variantspreespawnsleft[variantIndex] = count;
	g_BvSpreeCooldowns[variantIndex] = count;
}

/**
* Rolls for and starts a spree for the given variant, independently from any other active sprees or cooldowns.
*/
void bvspawnTryStartStandardSpree(u8 variantIndex) {
	if (bvCanStartSpree(variantIndex) && RANDOMFRAC() < bvGetSpreeChance(bvGetVariant(variantIndex))) {
		bvspawnStartSpree(variantIndex);
	}
}

/**
* Rolls for and starts variant spawning sprees as appropriate.
*/
void bvspawnHandleStartingSprees() {
	bool impostorspree = false;

	// Rather than loop through, I'm gonna handle each separately in turn,
	// because some sprees might affect the chances of other sprees
	// and so on and so forth.

	// Model-change sprees
	bvspawnTryStartStandardSpree(BVINDEX_IMPOSTOR);
	bvspawnTryStartStandardSpree(BVINDEX_SLENDERMAN); // compatible with Impostor

	// Mini/wumbo sprees -- only start either if neither is already spreeing
	if (!bvIsSpreeing(MINI) && !bvIsSpreeing(WUMBO)) {
		f32 randfracsizespree = RANDOMFRAC();
		if (!bvIsOnSpreeCooldown(MINI) && randfracsizespree < bvGetSpreeChance(BVVARIANT_MINI)) {
			bvspawnStartSpree(MINI);
		}
		else if (!bvIsOnSpreeCooldown(WUMBO) && randfracsizespree > (1.0f - bvGetSpreeChance(BVVARIANT_WUMBO))) {
			bvspawnStartSpree(WUMBO);
		}
	}

	// Other independent sprees
	bvspawnTryStartStandardSpree(SUNGLASSES);
	bvspawnTryStartStandardSpree(EXPLOSIVE);
}

/**
* Rolls for and applies the flag for, at most, one Abomination variant.
*/
void bvspawnHandleAbominations(struct chrdata* chr, f32 chancemult, f32 variantchances[], bool iscurrentplayer) {
	u8 variantchoiceoffset; 
	u8 variantchoice;
	f32 chance;
	u8 i;

	if (chancemult <= 0) {
		return;
	}

	variantchoiceoffset = rngRandom() % BV_ABOMINATION_COUNT; 

	for (i = 0; i < BV_ABOMINATION_COUNT; i++) {
		variantchoice = variantchoiceoffset + BVINDEX_ABOMINATION_FIRST;
		const struct bvvariant* variant = bvGetVariant(variantchoice);
		chance = variantchances[variantchoice] *= chancemult;
		if (chance > 0 && RANDOMFRAC() < chance) {
			CHR_BV_FLAGS |= variant->flag;
			break;	
		}

		variantchoiceoffset = (variantchoiceoffset + 1) % BV_ABOMINATION_COUNT;
	} 
}

/**
* Rolls against the givenchance and if successful sets the given flag.
* Returns true if flag set.
*/
bool bvspawnRollForStandardVariant(struct chrdata* chr, f32 chance, u32 flag) {
	if (chance > 0 && RANDOMFRAC() < chance) {
		CHR_BV_FLAGS |= flag;
		return true;
	}

	return false;
}

/**
* Call when a bot or player spawns/respawns to roll for variants, set flags, and apply
* initial changes such as model changes, body scaling, and sunglasses.
*/
void bvspawnPrepVariety(struct chrdata* chr, bool iscurrentplayer) {
	f32 chances[BOTVARIETY_VARIANT_COUNT];
	f32 abominationchancemult = 1.0f;
	bool isimpostor = false;
	u8 i;

	// Initialize bvchrdata on this chr's first spawn of the match
	bool firstspawn = bvTryInitChrForFirstSpawn(chr, iscurrentplayer);
	
	// On subsequent spawns, reset temporary chrdata
	if (!firstspawn) {
		bvResetChrDataForSpawn(chr); // clears data including CHR_BV_FLAGS but doesn't revert changed model

		// On subsequent bot spawns, roll for sprees
		if (!iscurrentplayer) {
			bvspawnHandleStartingSprees();
		}
	}

	// Get base chances for each variant (depending on player/bot/debug)
	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		chances[i] = bvGetSpawnChanceByIndex(chr, i, iscurrentplayer);
	}

	// Handle model changes first -- bots only
	if (!iscurrentplayer) {
		bvspawnTryRevertToInitModel(chr); // reset any prior model changes

		// Impostors - copy one of the player's models
		if (bvspawnHandleImpostor(chr, chances[IMPOSTOR])) {
			// Impostors are more likely to have other variants, but only if there's no ongoing spree
			if (!bvIsSpreeing(MINI) && !bvIsSpreeing(WUMBO)) {
				chances[MINI] = 0.333f;
				chances[WUMBO] = 0.25f;
				abominationchancemult = 3.0f;
			}
			isimpostor = true;
		}

		// Slenderman - compatible with impostor
		if (bvspawnHandleSlenderman(chr, chances[SLENDERMAN], isimpostor)) {
			// Slenderman is incompatible with all following variants
			for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
				chances[i] = 0;
			}
		}
	}

	// Size variants (mini/wumbo)
	bvspawnHandleSize(chr, chances[MINI], chances[WUMBO]);

	// Abominations - rare, freaky body changes (normally bots only, but optionally players too for debug purposes)
	if (!iscurrentplayer || g_BvDebugAllowPlayerAbominations) {
		bvspawnHandleAbominations(chr, abominationchancemult, chances, iscurrentplayer);
	}
	// Other bot-only variants
	if (!iscurrentplayer) {
		// Pick at most one of the following: gunfetti, explosive. Rolled in order from rarest to most common
		// Gunfetti bots
		if (bvspawnRollForStandardVariant(chr, chances[GUNFETTI], BVFLAG_GUNFETTI)) {
			;
		}
		// Explosive bots
		else if (bvspawnRollForStandardVariant(chr, chances[EXPLOSIVE], BVFLAG_EXPLOSIVE)) {
			;
		}
	}

	// an impstor that doesn't have a size variant should wear sunglasses if possible
	if (isimpostor && !bvChrHasSizeVariant(chr)) {
		chances[SUNGLASSES] = 1.0f;
	}

	// Sunglasses - this is done after model changes so we can check for sunglassability
	bvspawnHandleSunglasses(chr, chances[SUNGLASSES]);

	// Apply major variants' body scale multipliers and minor height variance
	bvspawnApply3DBodyScale(chr);

	// If bot spawned as part of a spree,
	if (!iscurrentplayer) {
		bvspawnCountAgainstSpreeSpawns(chr);
	}
}

#undef MINI
#undef WUMBO
#undef IMPOSTOR
#undef SLENDERMAN
#undef SUNGLASSES
#undef EXPLOSIVE
#undef GUNFETTI
#undef SBD
