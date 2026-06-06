#include <ultra64.h>
#include "constants.h"
#include "game/bot.h"
#include "game/game_0b0fd0.h"
#include "game/mod/botvariety.h"
#include "game/mplayer/mplayer.h"
#include "bss.h"
#include "lib/rng.h"
#include "lib/model.h"

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
				0.3333f,  // chance 
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
				0.3333f,  // chance 
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

#define lshoulderjoint 2
#define rshoulderjoint 3
#define waistjoint     1
#define neckjoint      0

f32 botvarietyTryAdjustJointScale(struct chrdata* chr, s32 joint, f32 scale) {
	struct botvarietyvariant* variant = NULL;
	u8 i;

	if (!botvarietyIsActive() || !botvarietyChrHasVarietyFlags(chr)) {
		return scale;
	}

	for (i = 0; i < botvarietyCount; i++) {
		variant = &botvarietyVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag) {
			if (joint == neckjoint) {
				scale = variant->headscale;
			}
			else if (joint == lshoulderjoint || joint == rshoulderjoint) {
				scale = variant->shoulderscale;
			}
		}
	}

	return scale;
}

#undef lshoulderjoint
#undef rshoulderjoint
#undef waistjoint
#undef neckjoint

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

void botvarietyHandleSize(struct chrdata* chr, s32 bodynum, bool iscurrentplayer) {
	f32 initscale = g_HeadsAndBodies[bodynum].scale * 0.10000001f;
	f32 scalemult = 1.0f;
	f32 animscale = g_HeadsAndBodies[bodynum].animscale;
	f32 randfrac_size_variant = RANDOMFRAC();  // mini or wumbo
	f32 randfrac_size_variance = RANDOMFRAC(); // minor height variance e.g. 95%-105%

	// Mini
	if (randfrac_size_variant < MINI.chance) {
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_MINI;
		scalemult *= MINI.bodyscale;
		scalemult *= randfrac_size_variance * 0.05f + 0.975f; // Apply minor height variance between 97.5% and 102.5%
	}
	// Wumbo
	else if (randfrac_size_variant > (1 - WUMBO.chance)) {
		CHR_BOTVARIETY_FLAGS |= BOTVARIETY_FLAG_WUMBO;
		scalemult *= WUMBO.bodyscale;
		scalemult *= randfrac_size_variance * 0.05f + 0.95f; // Apply random height variance between 95% and 100%
	}
	// Normal
	else if (g_HeadsAndBodies[bodynum].canvaryheight) {
		scalemult *= randfrac_size_variance * 0.1f + 0.95f; // Apply random height variance between 95% and 105%
	}

	// Apply body scale (player)
	if (iscurrentplayer && g_Vars.currentplayer->model00d4) {
		modelSetScale(g_Vars.currentplayer->model00d4, scalemult * initscale);
		modelSetAnimScale(g_Vars.currentplayer->model00d4, animscale);
	} // Apply body scale (bot)
	else if (chr->model) {
		modelSetScale(chr->model, scalemult * initscale);
		modelSetAnimScale(chr->model, animscale);
	}
}

void botvarietyApplyOnSpawn(struct chrdata* chr, bool iscurrentplayer) {
	s32 bodynum = chr->bodynum;
	s32 headnum;

	CHR_BOTVARIETY_FLAGS = 0;

	botvarietyHandleSize(chr, bodynum, iscurrentplayer); // roll for a size variation
}
#undef CHR_BOTVARIETY_FLAGS
#undef MINI
#undef WUMBO
//=== End of botvariety funcs ===============================================================================
