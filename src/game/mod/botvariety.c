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

// Bot variety flags
#define BOTVARIETY_FLAG_MINI  0x00000001
#define BOTVARIETY_FLAG_WUMBO 0x00000002

struct botvarietyvariant botvarietyVariants[] = {
	{   BOTVARIETY_FLAG_MINI,
				0.002,    // chance (1 in 500) 0.3333f,
				0.605f,   // bodyscale
				1.65f,    // headscale
				1.3f,     // shoulderscale
				1.1f,     // movespeedmult
				1.5f,     // animspeedmult
			 -1.0f,     // damagetakenmult - disabled
				0.9f,     // bluntdamagemult
				0,        // disarmdamage (default=0)
			 -1.0f,     // meleerangemult - disabled
				0.63375f, // camheightmult (player easter egg)
	}, {BOTVARIETY_FLAG_WUMBO,
				0.001,    // chance (1 in 1000) 0.3333f,
				1.5f,     // bodyscale
				0.8f,     // headscale
				1.4f,     // shoulderscale
				0.95f,    // movespeedmult
				0.9f,     // animspeedmult
				0.5f,     // damagetakenmult
				2.0f,     // bluntdamagemult
				1.0f,     // disarmdamage (default=0)
				2.0f,     // meleerangemult
				1.4517f,  // camheightmult (player easter egg)
	}
};

#define MINI  botvarietyVariants[0]
#define WUMBO botvarietyVariants[1]
u8 botvarietyCount = ARRAYCOUNT(botvarietyVariants);

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

	if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_MINI) {
		mult = MINI.camheightmult;
		changed = true;
	}
	else if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_WUMBO) {
		mult = WUMBO.camheightmult;
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

	for (i = 0; i < botvarietyCount; i++) {
		variant = &botvarietyVariants[i];

		// Handle attacker damage factors
		if (ATTACKER_BOTVARIETY_FLAGS & variant->flag) {
			// Handle blunt damage
			if (gsetHasFunctionFlags(gset, FUNCFLAG_BLUNTIMPACT)) {
				// Handle disarm - set to flat value if 0, but else apply general bluntdamagemult
				if (gsetHasFunctionFlags(gset, FUNCFLAG_DISARM) && damage <= 0) {
					damage = variant->disarmdamage;
				}
				// Handle non-disarm blunt damage multiplier
				else {
					damage *= variant->bluntdamagemult;
				}
			}
		}

		// Handle victim damage factors
		if (VICTIM_BOTVARIETY_FLAGS & variant->flag) {
			// Handle unshielded damage taken mult
			if (vchr->cshield <= 0 && variant->damagetakenmult > 0) {
				damage *= variant->damagetakenmult;
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

	for (i = 0; i < botvarietyCount; i++) {
		variant = &botvarietyVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag && variant->meleerangemult > 0) {
			range *= variant->meleerangemult;
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
	struct botvarietyvariant* variant = NULL;
	u8 i;

	if (!botvarietyIsActive() || !botvarietyChrHasVarietyFlags(chr)) {
		return scale;
	}

	for (i = 0; i < botvarietyCount; i++) {
		variant = &botvarietyVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag) {
			switch (joint) {
			case neck:
				scale = variant->headscale;     break;
			case lshoulder:
			case rshoulder:
				scale = variant->shoulderscale; break;
			}
		}
	}

	return scale;
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

	for (i = 0; i < botvarietyCount; i++) {
		variant = &botvarietyVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag) {
			speed *= variant->movespeedmult;
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

	for (i = 0; i < botvarietyCount; i++) {
		variant = &botvarietyVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag) {
			animspeed *= variant->animspeedmult;
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

f32 botvarietyInitBodyScalesPlayer[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
f32 botvarietyInitBodyScalesBot[8] = { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };

f32 botvarietyHandleScaleInit(struct chrdata* chr, bool iscurrentplayer, bool respawning) {
	// Set or get initial scale value
	if (iscurrentplayer) {
		if (!respawning || botvarietyInitBodyScalesPlayer[g_Vars.currentplayerindex] < 0) {
			botvarietyInitBodyScalesPlayer[g_Vars.currentplayerindex] = g_Vars.currentplayer->model00d4->scale;
			return g_Vars.currentplayer->model00d4->scale;
		}
		else {
			return g_Vars.currentplayer->model00d4->scale;
		}
	}
	else { // bot
		if (!respawning || botvarietyInitBodyScalesBot[chr->aibot->aibotnum] < 0) {
			botvarietyInitBodyScalesBot[chr->aibot->aibotnum] = chr->model->scale;
			return chr->model->scale;
		}
		else {
			return botvarietyInitBodyScalesBot[chr->aibot->aibotnum];
		}
	}
}

void botvarietyHandleSize(struct chrdata* chr, s32 bodynum, bool iscurrentplayer, bool respawning) {
	f32 scale = botvarietyHandleScaleInit(chr, iscurrentplayer, respawning);
	f32 randfrac_size_variant = RANDOMFRAC();  // mini or wumbo
	f32 randfrac_size_variance = RANDOMFRAC(); // minor height variance e.g. 95%-105%

	// Mini
	if (randfrac_size_variant < MINI.chance) {
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_MINI;
		scale *= MINI.bodyscale;
		scale *= randfrac_size_variance * 0.05f + 0.975f; // Apply minor height variance between 97.5% and 102.5%
	}
	// Wumbo
	else if (randfrac_size_variant > (1 - WUMBO.chance)) {
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_WUMBO;
		scale *= WUMBO.bodyscale;
		scale *= randfrac_size_variance * 0.05f + 0.95f; // Apply random height variance between 95% and 100%
	}
	// Normal
	else if (g_HeadsAndBodies[bodynum].canvaryheight) {
		scale *= randfrac_size_variance * 0.1f + 0.95f; // Apply random height variance between 95% and 105%
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

void botvarietyApplyOnSpawn(struct chrdata* chr, bool iscurrentplayer, bool respawning) {
	s8 bodynum = chr->bodynum;
	s8 headnum = chr->headnum;

	CHR_BOTVARIETY_FLAGS = 0;

	botvarietyHandleSize(chr, bodynum, iscurrentplayer, respawning); // roll for a size variation
	botvarietyHandleSunglasses(chr, headnum, iscurrentplayer);
}
#undef CHR_BOTVARIETY_FLAGS
#undef MINI
#undef WUMBO
