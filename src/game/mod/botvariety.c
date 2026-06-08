#include <ultra64.h>
#include "constants.h"
#include "game/bot.h"
#include "game/chr.h"
#include "game/game_0b0fd0.h"
#include "game/mod/botvariety.h"
#include "game/mplayer/mplayer.h"
#include "bss.h"
#include "lib/rng.h"
#include "lib/model.h"
#include "lib/ailist.h"

//=== Botvariety funcs ======================================================================================
// Bespoke system for variations that can be applied randomly as bots spawn and respawn, altering their
// appearance and properties.
// As an easter egg, players can also spawn mini or wumbo.
// - Size variants: Mini, Wumbo

bool botvarietyDebug = true;

// Bot variety flags
#define BOTVARIETY_FLAG_MINI  0x00000001
#define BOTVARIETY_FLAG_WUMBO 0x00000002

struct botvarietyvariant botvarietyVariants[] = {
	{   BOTVARIETY_FLAG_MINI,
			{ // spawn chances
				0.05f,   // bot
				0.0333f, // player
				0.333f,  // debug
			},
			{ // scale mults (neg to disable)
				0.605f,  // body
				1.65f,   // head
				1.3f,    // shoulder
				0.63375f,// camera height (player)
			},
			{ // stats (neg to disable)
				1.1f,    // movespeedmult
				1.5f,    // animspeedmult
				-1.0f,   // damagetakenmult - disabled
				0.9f,    // bluntdamagemult
				0,       // disarmdamage (default=0)
				-1.0f,   // meleerangemult - disabled
			}
	}, {BOTVARIETY_FLAG_WUMBO,
			{ // spawn chances
				0.0333f, // bot
				0.0333f, // player
				0.333f,  // debug
			},
			{ // scale mults
				1.5f,    // body
				0.8f,    // head
				1.4f,    // shoulder
				1.4517f, // camera height (player)
			},
			{ // stats (neg to disable)
				0.95f,   // movespeedmult
				0.9f,    // animspeedmult
				0.2857f, // damagetakenmult (= 3.5* health)
				2.0f,    // bluntdamagemult
				1.0f,    // disarmdamage (default=0)
				2.0f,    // meleerangemult
			},
	}
};

#define INDEX_MINI 0
#define INDEX_WUMBO 1
#define VARIANT_MINI  botvarietyVariants[INDEX_MINI]
#define VARIANT_WUMBO botvarietyVariants[INDEX_WUMBO]

u8 botvarietyVariantCount = ARRAYCOUNT(botvarietyVariants);

#define BOTVARIETY_SUNGLASSES_CHANCE_ONEOUTOF 125 
#define BOTVARIETY_SUNGLASSES_CHANCE_PLAYER   20

#define CHR_BOTVARIETY_FLAGS chr->convtalk // This u32 isn't used in combat simulator so we'll hackily borrow it

bool botvarietyIsActive() {
	return g_Vars.normmplayerisrunning && g_MpSetup.options & MPOPTION_BOTVARIETY;
}

bool botvarietyChrHasVarietyFlags(struct chrdata* chr) {
	return CHR_BOTVARIETY_FLAGS != 0;
}

void botvarietyTryAdjustCurrentPlayerCameraHeight() {
	struct chrdata* chr = g_Vars.currentplayer->prop->chr;
	f32 mult = 1.0f;
	bool changed = false;

	if (!botvarietyIsActive()) {
		return;
	}

	if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_MINI && VARIANT_MINI.scale.camheight > 0) {
		mult = VARIANT_MINI.scale.camheight;
		changed = true;
	}
	else if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_WUMBO && VARIANT_WUMBO.scale.camheight > 0) {
		mult = VARIANT_WUMBO.scale.camheight;
		changed = true;
	}

	if (changed) { // vanilla adjustments copied from smalljo code
		if (g_Vars.currentplayer->bondmovemode == MOVEMODE_WALK) { // if moving
			g_Vars.currentplayer->bond2.unk10.y += (g_Vars.currentplayer->crouchoffsetreal - g_Vars.currentplayer->crouchoffsetrealsmall);
		}

		g_Vars.currentplayer->bond2.unk10.y = (g_Vars.currentplayer->bond2.unk10.y - g_Vars.currentplayer->vv_manground) * mult;
		g_Vars.currentplayer->bond2.unk10.y += g_Vars.currentplayer->vv_manground;
	}
}

#define ATTACKER_BOTVARIETY_FLAGS achr->convtalk
#define VICTIM_BOTVARIETY_FLAGS   vchr->convtalk

f32 botvarietyTryAdjustDamage(struct chrdata* achr, struct chrdata* vchr, struct gset* gset, f32 damage) {
	struct botvarietyvariant* variant = NULL;
	u8 i;

	if (!botvarietyIsActive()) {
		return damage;
	}
	if (!(botvarietyChrHasVarietyFlags(achr) || botvarietyChrHasVarietyFlags(vchr))) {
		return damage;
	}

	for (i = 0; i < botvarietyVariantCount; i++) {
		variant = &botvarietyVariants[i];

		// Handle attacker damage factors
		if (ATTACKER_BOTVARIETY_FLAGS & variant->flag) {
			// Handle blunt damage
			if (gsetHasFunctionFlags(gset, FUNCFLAG_BLUNTIMPACT)) {
				// Handle disarm - set to flat value if 0, but else apply general bluntdamagemult
				if (gsetHasFunctionFlags(gset, FUNCFLAG_DISARM) && damage <= 0 && variant->stat.disarmdamage > 0) {
					damage = variant->stat.disarmdamage;
				}
				// Handle non-disarm blunt damage multiplier
				else if (variant->stat.bluntdamagemult >= 0) {
					damage *= variant->stat.bluntdamagemult;
				}
			}
		}

		// Handle victim damage factors
		if (VICTIM_BOTVARIETY_FLAGS & variant->flag) {
			// Handle unshielded damage taken mult
			if (vchr->cshield <= 0 && variant->stat.damagetakenmult >= 0) {
				damage *= variant->stat.damagetakenmult;
			}
		}
	}

	return damage;
}

f32 botvarietyTryAdjustCurrentPlayerMeleeRange(f32 range) {
	struct chrdata* chr = g_Vars.currentplayer->prop->chr;
	struct botvarietyvariant* variant = NULL;
	u8 i;

	if (!botvarietyIsActive() || !botvarietyChrHasVarietyFlags(chr)) {
		return range;
	}

	for (i = 0; i < botvarietyVariantCount; i++) {
		variant = &botvarietyVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag && variant->stat.meleerangemult > 0) {
			range *= variant->stat.meleerangemult;
		}
	}

	return range;
}

#undef ATTACKER_BOTVARIETY_FLAGS
#undef VICTIM_BOTVARIETY_FLAGS 

#define neck            0
#define waist           1
#define lshoulder       2
#define rshoulder       3
// what's 4?
#define rwrist          5
#define rhand           6
#define lwrist          7
#define lhand           8
#define rknee           9
#define rankle         10
#define rfoot          11
#define lknee          12
#define lankle         13
#define lfoot          14

f32 botvarietyTryAdjustJointScale(struct chrdata* chr, s32 joint, f32 scale) {
	f32 mult = 1.0f;
	struct botvarietyvariant* variant = NULL;
	u8 i;

	if (!botvarietyIsActive() || !botvarietyChrHasVarietyFlags(chr)) {
		return scale;
	}

	for (i = 0; i < botvarietyVariantCount; i++) {
		variant = &botvarietyVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag) {
			switch (joint) {
			case neck:
				mult = variant->scale.head;     break;
			case lshoulder:
			case rshoulder:
				mult = variant->scale.shoulder; break;
			}
		}
	}

	if (mult > 0) {
		return scale * mult;
	}
	else {
		return scale;
	}
}

#undef neck
#undef waist
#undef lshoulder
#undef rshoulder
// what's 4?
#undef rwrist
#undef rhand
#undef lwrist
#undef lhand
#undef rknee
#undef rankle
#undef rfoot
#undef lknee
#undef lankle
#undef lfoot

f32 botvarietyTryAdjustMoveSpeed(struct chrdata* chr, f32 speed) {
	struct botvarietyvariant* variant = NULL;
	u8 i;

	if (!botvarietyIsActive()) { // note: we don't care if any specific flags are set for this function 'cause all bots get speed variance
		return speed;
	}

	for (i = 0; i < botvarietyVariantCount; i++) {
		variant = &botvarietyVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag && variant->stat.movespeedmult > 0) {
			speed *= variant->stat.movespeedmult;
		}
	}

	// Apply random speed variance to all bots
	if (chr->aibot) {
		speed *= RANDOMFRAC() * 0.1f + 0.95f; // between 95% and 105%
	}

	return speed;
}

f32 botvarietyTryAdjustAnimSpeed(struct chrdata* chr, f32 animspeed) {
	struct botvarietyvariant* variant = NULL;
	u8 i;

	if (!botvarietyIsActive() || !botvarietyChrHasVarietyFlags(chr)) {
		return animspeed;
	}

	for (i = 0; i < botvarietyVariantCount; i++) {
		variant = &botvarietyVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag && variant->stat.animspeedmult >= 0) {
			animspeed *= variant->stat.animspeedmult;
		}
	}

	return animspeed;
}

bool botvarietyGuessCrouchpos(struct chrdata* chr, s32* crouchpos) {
	if (!botvarietyIsActive()) {
		return false;
	}

	// Mini bots never have to crouch
	if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_MINI) {
		*crouchpos = CROUCHPOS_STAND;
		return true;
	}

	// Wumbo bots skip middle-crouch
	if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_WUMBO) {
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

// use these to remember each player/bot's scale when first handled
f32 botvarietyInitBodyScalesPlayer[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
f32 botvarietyInitBodyScalesBot[8] = { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };

f32 botvarietyBasemodelScaleInit(struct chrdata* chr, bool iscurrentplayer) {
	// Set or get initial scale value
	if (iscurrentplayer) {
		if (botvarietyInitBodyScalesPlayer[g_Vars.currentplayerindex] < 0) {
			botvarietyInitBodyScalesPlayer[g_Vars.currentplayerindex] = g_Vars.currentplayer->model00d4->scale;
			return g_Vars.currentplayer->model00d4->scale;
		}
		else {
			return botvarietyInitBodyScalesPlayer[g_Vars.currentplayerindex];
		}
	}
	else { // bot
		if (botvarietyInitBodyScalesBot[chr->aibot->aibotnum] < 0) {
			botvarietyInitBodyScalesBot[chr->aibot->aibotnum] = chr->model->scale;
			return chr->model->scale;
		}
		else {
			return botvarietyInitBodyScalesBot[chr->aibot->aibotnum];
		}
	}
}

void botvarietyHandleBodyScale(struct chrdata* chr, s32 bodynum, bool iscurrentplayer) {
	f32 scale = botvarietyBasemodelScaleInit(chr, iscurrentplayer);
	f32 randfracsizevariance = RANDOMFRAC(); // minor height variance e.g. 95%-105%

	// Mini
	if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_MINI) {
		scale *= VARIANT_MINI.scale.body;
		scale *= randfracsizevariance * 0.05f + 0.975f; // Apply minor height variance between 97.5% and 102.5%
	}
	// Wumbo
	else if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_WUMBO) {
		scale *= VARIANT_WUMBO.scale.body;
		scale *= randfracsizevariance * 0.05f + 0.95f; // Apply random height variance between 95% and 100%
	}
	// Normal
	else if (g_HeadsAndBodies[bodynum].canvaryheight) {
		scale *= randfracsizevariance * 0.1f + 0.95f; // Apply random height variance between 95% and 105%
	}

	// Apply body scale (player)
	if (iscurrentplayer && g_Vars.currentplayer->model00d4) {
		modelSetScale(g_Vars.currentplayer->model00d4, scale);
	} // Apply body scale (bot)
	else if (chr->model) {
		modelSetScale(chr->model, scale);
	}
}

void botvarietyApplySunglasses(struct model *model, s32 headnum, bool applysunglasses) {
	struct modeldef *headmodeldef = g_HeadsAndBodies[headnum].modeldef;
	struct modelnode* node;

	if (headmodeldef && model) {
		node = modelGetPart(headmodeldef, MODELPART_HEAD_SUNGLASSES);

		if (node) {
			union modelrwdata* rwdata = modelGetNodeRwData(model, node);

			if (rwdata) {
				rwdata->toggle.visible = applysunglasses;
			}
		}
	}
}

void botvarietyHandleSunglasses(struct chrdata* chr, s32 headnum, bool iscurrentplayer) {
	struct model *model;
	u8 chanceoutof = 1;

	if (iscurrentplayer) {
		chanceoutof = BOTVARIETY_SUNGLASSES_CHANCE_PLAYER;
		model = g_Vars.currentplayer->model00d4;
	}
	else {
		chanceoutof = BOTVARIETY_SUNGLASSES_CHANCE_ONEOUTOF;
		model = chr->model;
	}

	botvarietyApplySunglasses(model, headnum, rngRandom() % chanceoutof == 0);
}

f32 botvarietyGetVariantChance(u8 variantIndex, bool iscurrentplayer) {
	if (variantIndex > botvarietyVariantCount || variantIndex < 0) {
		return 0;
	}

	if (botvarietyDebug) {
		return botvarietyVariants[variantIndex].chance.debug;
	}
	else if (iscurrentplayer) {
		return botvarietyVariants[variantIndex].chance.player;
	}
	else {
		return botvarietyVariants[variantIndex].chance.bot;
	}
}

void botvarietyRollForVariants(struct chrdata* chr, bool iscurrentplayer) {
	f32 randfracsizevariant = RANDOMFRAC();  // mini or wumbo

	// Mini
	if (randfracsizevariant < botvarietyGetVariantChance(INDEX_MINI, iscurrentplayer)) {
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_MINI;
	}
	// Wumbo
	else if (randfracsizevariant > (1 - botvarietyGetVariantChance(INDEX_WUMBO, iscurrentplayer))) {
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_WUMBO;
	}
}

void botvarietyApplyOnSpawn(struct chrdata* chr, bool iscurrentplayer) {
	s8 bodynum = chr->bodynum;
	s8 headnum = chr->headnum;

	CHR_BOTVARIETY_FLAGS = 0; // reset variant flags
	botvarietyRollForVariants(chr, iscurrentplayer);

	botvarietyHandleBodyScale(chr, bodynum, iscurrentplayer);
	botvarietyHandleSunglasses(chr, headnum, iscurrentplayer);
}
#undef CHR_BOTVARIETY_FLAGS
