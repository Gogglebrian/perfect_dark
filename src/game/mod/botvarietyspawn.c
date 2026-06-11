#include <ultra64.h>
#include "constants.h"
#include "game/body.h"
#include "game/chr.h"
#include "game/mod/botvariety.h"
#include "game/modelmgr.h"
#include "game/mplayer/mplayer.h"
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
/// Gets the float chance value from a given size variant based on the chr's existing properties.
/// </summary>
f32 bvGetSizeChanceByIndex(struct chrdata* chr, u8 variantIndex) {
	if (variantIndex > BOTVARIETY_VARIANT_COUNT || variantIndex < 0) {
		return 0;
	}

	if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_IMPOSTOR) {
		return 0.5f;
	}
	else {
		return bvGetChance(chr, &g_BvVariants[variantIndex].chance);
	}
}

/// <summary>
/// Enables or disables the sunglasses on the model, if the model has sunglasses.
/// </summary>
void bvSetSunglasses(struct chrdata* chr, bool enabled) {
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
/// Handles applying or removing sunglasses according to the chr's sunglasses flag.
/// </summary>
void bvspawnHandleSunglasses(struct chrdata* chr) {
	bvSetSunglasses(chr, CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_SUNGLASSES);
}

/// <summary>
/// Make a bot character an impostor by swapping their model to match one of the human players',
/// or revert back to the original model
/// </summary>
void bvspawnHandleImpostor(struct chrdata* chr) {
	s16 botnum = chr->aibot->aibotnum;
	struct model* initmodel = g_BvMatch.bots[botnum].initmodel;

	// If necessary, revert previous impostor
	if (initmodel && chr->model != initmodel) {
		struct model* impostormodel = chr->model;
		chr->model = initmodel;
		chr->headnum = mpGetHeadId(g_BotConfigsArray[botnum].base.mpheadnum);
		chr->bodynum = mpGetBodyId(g_BotConfigsArray[botnum].base.mpbodynum);
		chr->race = bodyGetRace(chr->bodynum);

		/*
		impostormodel->anim->unk70 = NULL;
		impostormodel->anim = NULL;
		impostormodel = NULL;
		/*
		modelFreeVertices(VTXSTORETYPE_CHRVTX, impostormodel);
		modelmgrFreeModel(impostormodel);
		*/
	}

	// Apply new impostor
	if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_IMPOSTOR) {
		u8 copyplayerindex;
		struct chrdata* copyplayerchr;
		struct model* impostormodel;

		// select a player at random
		copyplayerindex = rngRandom() % getNumPlayers();
		copyplayerchr = mpGetChrFromPlayerIndex(copyplayerindex);

		// make a copy of the player's model, and get it ready for primetime
		impostormodel = bodyAllocateModel(copyplayerchr->bodynum, copyplayerchr->headnum, 0);
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
	}
}

/// <summary>
/// Scales the chr's body with regard to its applicable botvariety flags and applies minor random height variance.
/// </summary>
void bvspawnHandleBodyScale(struct chrdata* chr) {
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
/// Rolls and sets flags for major and minor variants.
/// </summary>
void bvspawnRollForVariants(struct chrdata* chr, bool iscurrentplayer) {
	f32 randfracsizevariant = RANDOMFRAC(); // mini or wumbo
	bool isimpostor = false;

	// Impostor -- bots only
	if (!iscurrentplayer && RANDOMFRAC() < bvGetChance(chr, &VARIANT_IMPOSTOR.chance)) {
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_IMPOSTOR;
		isimpostor = true;
	}

	// Mini
	if (randfracsizevariant < bvGetSizeChanceByIndex(chr, INDEX_MINI)) {
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_MINI;
	}
	// Wumbo
	else if (randfracsizevariant > (1 - bvGetSizeChanceByIndex(chr, INDEX_WUMBO))) {
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_WUMBO;
	}

	// Sunglasses
	if (bvCanChrWearSunglasses(chr) && RANDOMFRAC() < bvGetChance(chr, &VARIANT_SUNGLASSES.chance)) {
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_SUNGLASSES;
	}
}

/// <summary>
/// Call when a bot or player spawns/respawns to roll for variants, set flags, and apply
/// initial changes such as model changes, body scaling, and sunglasses.
/// </summary>
void bvspawnPrepVariety(struct chrdata* chr, bool iscurrentplayer) {
	bvTryInitChr(chr, iscurrentplayer);

	CHR_BOTVARIETY_FLAGS = 0; // reset variant flags
	bvspawnRollForVariants(chr, iscurrentplayer);

	if (!iscurrentplayer) {
		bvspawnHandleImpostor(chr);
	}

	bvspawnHandleBodyScale(chr);
	bvspawnHandleSunglasses(chr);
}

#undef CHR_BOTVARIETY_FLAGS
