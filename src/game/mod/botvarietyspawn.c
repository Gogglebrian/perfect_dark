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
f32 bvGetInitBodyScale(struct chrdata* chr) {
	return bvGetChrMatchData(chr)->initscale;
}

/// <summary>
/// Select which of the variant's f32 chance values to use depending on whether player or bot, and whether debug is enabled.
/// </summary>
f32 bvGetChance(struct chrdata* chr, const struct bvvariantchance* chance) {
	if (g_BvDebug) {
		return chance->debug;
	}
	else if (bvIsChrCurrentPlayer(chr)) {
		return chance->player;
	}
	else {
		return chance->bot;
	}
}

/// <summary>
/// Enables or disables the sunglasses on the model, if the model has sunglasses.
/// </summary>
void bvspawnApplySunglasses(struct chrdata* chr, bool enabled) {
	struct model* model = bvGetModel(chr);
	struct modeldef* headmodeldef = g_HeadsAndBodies[chr->headnum].modeldef;
	struct modelnode* node;

	if (headmodeldef && model) {
		node = modelGetPart(headmodeldef, MODELPART_HEAD_SUNGLASSES);

		if (node) {
			union modelrwdata* rwdata = modelGetNodeRwData(model, node);

			if (rwdata) {
				rwdata->toggle.visible = enabled;
			}
		}
	}
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
/// Rolls for and applies Sunglasses variant - sunglasses + minor gameplay bonuses.
/// Returns true if sunglasses are enabled.
/// </summary>
bool bvspawnHandleSunglasses(struct chrdata* chr, f32 sunglasseschance) {
	if (bvCanChrWearSunglasses(chr)) {
		if (RANDOMFRAC() < sunglasseschance) { // Roll
			CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_SUNGLASSES; // set flag for gameplay bonuses
			bvspawnApplySunglasses(chr, true); // enable sunglasses
			return true;
		}
		else {
			bvspawnApplySunglasses(chr, false); // disable sunglasses
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
		struct model* impostormodel = chr->model;
		chr->model = initmodel;
		chr->headnum = mpGetHeadId(g_BotConfigsArray[botnum].base.mpheadnum);
		chr->bodynum = mpGetBodyId(g_BotConfigsArray[botnum].base.mpbodynum);
		chr->race = bodyGetRace(chr->bodynum);

		modelFreeVertices(VTXSTORETYPE_CHRVTX, impostormodel);
		modelmgrFreeModel(impostormodel);

		// If we were impstor, mark that we aren't anymore
		if (bvchr->impostorof >= 0) {
			bvchr->impostorof = -1;
		}
	}
}

/// <summary>
/// Make a bot character an impostor by swapping their model to match one of the human players'.
/// Returns true if successfully applied.
/// </summary>
bool bvspawnTryApplyImpostor(struct chrdata* chr) {
	u8 copyplayerindex;
	struct chrdata* copyplayerchr;
	struct model* impostormodel;

	// select a player at random
	copyplayerindex = rngRandom() % getNumPlayers();
	copyplayerchr = mpGetChrFromPlayerIndex(copyplayerindex);

	// make a copy of the player's model
	impostormodel = bodyAllocateModel(copyplayerchr->bodynum, copyplayerchr->headnum, 0);
	if (impostormodel) {
		// Now get the model ready for primetime
		modelSetAnim70(impostormodel, chr0f01f378);
		impostormodel->chr = chr;
		impostormodel->unk01 = 1;
		modelSetAnimPlaySpeed(impostormodel, PALUPF(var80062968), 0);
		modelSetRootPosition(impostormodel, &chr->prop->pos);

		// Assign new model to bot's chr
		chr->model = impostormodel;
		chr->headnum = copyplayerchr->headnum;
		chr->bodynum = copyplayerchr->bodynum;
		chr->race = bodyGetRace(copyplayerchr->bodynum);

		// Mark that we're impersonating the player
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
/// Scales the chr's body with regard to its applicable botvariety flags and applies minor random height variance.
/// </summary>
void bvspawnApplyBodyScale(struct chrdata* chr) {
	f32 scale = bvGetInitBodyScale(chr);
	f32 randfracsizevariance = RANDOMFRAC(); // minor height variance e.g. 95%-105%

	// Mini
	if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_MINI) {
		scale *= VARIANT_MINI.body.scalebody;
		scale *= randfracsizevariance * 0.05f + 0.975f; // Apply minor height variance between 97.5% and 102.5%
	}
	// Wumbo
	else if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_WUMBO) {
		scale *= VARIANT_WUMBO.body.scalebody;
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
/// Rolls for and applies size variants: Mini and Wumbo, and applies minor height variance
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

	// Apply major variants' body scale multipliers and minor height variance
	bvspawnApplyBodyScale(chr);
}

/// <summary>
/// Call when a bot or player spawns/respawns to roll for variants, set flags, and apply
/// initial changes such as model changes, body scaling, and sunglasses.
/// </summary>
void bvspawnPrepVariety(struct chrdata* chr, bool iscurrentplayer) {
	f32 impostorchance, minichance, wumbochance, sunglasseschance;
	
	// Initialize bvchrdata on the first spawn of the match
	bvTryInitChr(chr, iscurrentplayer);

	// Reset variant flags
	CHR_BOTVARIETY_FLAGS = 0;

	// Get base chances for each variant (depending on player/bot/debug)
	impostorchance = bvGetChance(chr, &VARIANT_IMPOSTOR.chance);
	minichance = bvGetChance(chr, &VARIANT_MINI.chance);
	wumbochance = bvGetChance(chr, &VARIANT_WUMBO.chance);
	sunglasseschance = bvGetChance(chr, &VARIANT_SUNGLASSES.chance);

	// Handle model changes first -- bots only
	if (!iscurrentplayer) {
		bvspawnTryRevertToInitModel(chr); // reset any prior model changes

		// Impostors - copy one of the player's models
		if (bvspawnHandleImpostor(chr, impostorchance)) {
			// Impostors are more likely to have other variants
			minichance *= 2.0f;
			wumbochance *= 2.0f;
			sunglasseschance *= 2.0f;
		}
	}

	// Size variants (mini/wumbo) and height variance
	bvspawnHandleSize(chr, minichance, wumbochance);

	// Sunglasses - this is done after model changes so we can check for sunglassability
	bvspawnHandleSunglasses(chr, sunglasseschance);
}

#undef CHR_BOTVARIETY_FLAGS
