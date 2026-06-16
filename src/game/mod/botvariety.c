#include <ultra64.h>
#include "constants.h"
#include "game/bot.h"
#include "game/chr.h"
#include "game/game_0b0fd0.h"
#include "game/game_006900.h"
#include "game/mod/botvariety.h"
#include "game/mplayer/mplayer.h"
#include "game/propobj.h"
#include "bss.h"
#include "lib/ailist.h"
#include "lib/mtx.h"
#include "lib/rng.h"

//=== Botvariety system explained ===============================================================================
// If enabled, bots can randomly spawn/respawn as new special variants like Mini, Wumbo, or Impostor,
// each with unique appearance and properties.
// Some are major variants with sweeping gameplay effects (eg Mini/Wumbo), while others are mostly cosmetic
// (eg Sunglasses or Impostor).
// Some of these variants are mutually exclusive (eg Mini/Wumbo), while others can overlap
// (eg Sunglasses with anything else).
// Some variations affect the chances of other variations (eg Impostor is more likely to be Mini/Wumbo).
// As an easter egg, players also have a chance to spawn as some variants (eg Mini, Wumbo, Sunglasses)
// including (most) gameplay effects.
// The system can also apply minor variance to all bots regardless of variant, such as minor height variance
// (replacing the vanilla height variance so as not to conflict with the major variants' scale changes)
// or speed variance.

#define CHR_BOTVARIETY_FLAGS chr->convtalk // This u32 isn't used in combat simulator so we'll hackily borrow it

/// <summary>
/// Is Combat Simulator running with botvariety enabled?
/// </summary>
bool bvIsBotVarietyActive() {
	return g_Vars.normmplayerisrunning && g_MpSetup.options & MPOPTION_BOTVARIETY;
}

/// <summary>
/// Does this character have any botvariety flags?
/// </summary>
bool bvChrHasVarietyFlags(struct chrdata* chr) {
	return CHR_BOTVARIETY_FLAGS != 0;
}

/// <summary>
/// Returns a voice pitch multiplier with regard to the character's applicable botvariety flags, if the botvariety system is active.
/// Returns -1 if no changes
/// </summary>
f32 bvGetVoicePitch(struct chrdata* chr) {
	const struct bvvariant* variant = NULL;
	u8 i;
	f32 pitch = -1;

	if (!bvIsBotVarietyActive() || !bvChrHasVarietyFlags(chr)) {
		return -1;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = &gc_BvVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag && variant->body.voicepitch > 0) {
			if (pitch < 0) {
				pitch = 1.0f;
			}
			pitch *= variant->body.voicepitch;
		}
	}

	return pitch;
}

/// <summary>
/// Adjusts the current player's camera height with regard to player chr's applicable botvariety flags, if the botvariety system is active.
/// </summary>
void bvTryAdjustCurrentPlayerCameraHeight() {
	struct chrdata* chr = g_Vars.currentplayer->prop->chr;
	f32 mult = 1.0f;
	bool changed = false;

	if (!bvIsBotVarietyActive()) {
		return;
	}

	if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_MINI && VARIANT_MINI.body.camheight > 0) {
		mult = VARIANT_MINI.body.camheight;
		changed = true;
	}
	else if (CHR_BOTVARIETY_FLAGS & BOTVARIETY_FLAG_WUMBO && VARIANT_WUMBO.body.camheight > 0) {
		mult = VARIANT_WUMBO.body.camheight;
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

/// <summary>
/// Adjusts damage with regard to the attacker and victims' respective applicable botvariety flags, if the botvariety system is active.
/// </summary>
f32 bvTryAdjustDamage(struct chrdata* achr, struct chrdata* vchr, struct gset* gset, f32 damage) {
	const struct bvvariant* variant = NULL;
	u8 i;

	if (!bvIsBotVarietyActive()) {
		return damage;
	}
	if (!(bvChrHasVarietyFlags(achr) || bvChrHasVarietyFlags(vchr))) {
		return damage;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = &gc_BvVariants[i];

		// Handle attacker damage factors
		if (ATTACKER_BOTVARIETY_FLAGS & variant->flag) {
			// Handle blunt damage
			if (gsetHasFunctionFlags(gset, FUNCFLAG_BLUNTIMPACT)) {
				// Handle disarm - set to flat value if it's higher than the current value (default 0), but else apply general bluntdamagemult
				if (gsetHasFunctionFlags(gset, FUNCFLAG_DISARM) && variant->stat.disarmdamage > 0 && damage < variant->stat.disarmdamage) {
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
			if (vchr->cshield <= 0 && variant->stat.damagetakenmult > 0) {
				damage *= variant->stat.damagetakenmult;
			}
		}
	}

	return damage;
}
#undef ATTACKER_BOTVARIETY_FLAGS
#undef VICTIM_BOTVARIETY_FLAGS 

/// <summary>
/// Adjusts the passed melee range value with regard to the currentplayer chr's applicable botvariety flags, if the botvariety system is active.
/// </summary>
f32 bvTryAdjustCurrentPlayerMeleeRange(f32 range) {
	struct chrdata* chr = g_Vars.currentplayer->prop->chr;
	const struct bvvariant* variant = NULL;
	u8 i;

	if (!bvIsBotVarietyActive() || !bvChrHasVarietyFlags(chr)) {
		return range;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = &gc_BvVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag && variant->stat.meleerangemult > 0) {
			range *= variant->stat.meleerangemult;
		}
	}

	return range;
}

// Human skeleton joint numbers
#define JOINT_NECK            0
#define JOINT_WAIST           1
#define JOINT_LSHOULDER       2
#define JOINT_RSHOULDER       3
// what's 4?
#define JOINT_RWRIST          5
#define JOINT_RHAND          6
#define JOINT_LWRIST          7
#define JOINT_LHAND          8
#define JOINT_RKNEE           9
#define JOINT_RANKLE         10
#define JOINT_RFOOT         11
#define JOINT_LKNEE          12
#define JOINT_LANKLE         13
#define JOINT_LFOOT         14

/// <summary>
/// Adjusts the passed scale value for the joint with regard to the chr's applicable botvariety flags, if the botvariety system is active.
/// </summary>
f32 bvTryAdjust3DJointScale(struct chrdata* chr, s32 joint, f32 scale) {
	f32 mult = 1.0f;
	const struct bvvariant* variant = NULL;
	u8 i;
	f32 jointscale;

	if (!bvIsBotVarietyActive() || !bvChrHasVarietyFlags(chr)) {
		return scale;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = &gc_BvVariants[i];
		jointscale = -1.0f;

		if (CHR_BOTVARIETY_FLAGS & variant->flag) {
			switch (joint) {
			case JOINT_NECK:
				jointscale = variant->body.scalehead;     break;
			case JOINT_LSHOULDER:
			case JOINT_RSHOULDER:
				jointscale = variant->body.scaleshoulder; break;
			}

			if (jointscale > 0) {
				mult *= jointscale;
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

/// <summary>
/// Applies variants' separate 1D joint scales to the chr's joint, per the chr's applicable botvariety flags, if the botvariety system is active.
/// </summary>
void bvTryApplyXYZJointScales(struct chrdata* chr, s32 joint, Mtxf* mtx, bool afterpositioned) {
	const struct bvvariant* variant = NULL;
	struct bvvariantxyzscales* scales = NULL;
	f32 mult_x = 1.0f;
	f32 mult_y = 1.0f;
	f32 mult_z = 1.0f;
	u8 i;

	if (!bvIsBotVarietyActive() || !bvChrHasVarietyFlags(chr)) {
		return;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = &gc_BvVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag) {
			// Determine which set of scales to use, if any
			if (afterpositioned && variant->body.xyzscales_postpositioned) {
				scales = variant->body.xyzscales_postpositioned;
			}
			else if (!afterpositioned && variant->body.xyzscales) {
				scales = variant->body.xyzscales;
			}
			
			// Multiply into the running multiplier
			if (scales) {
				if (scales->usejoints_x && scales->joints_x && scales->joints_x[joint] >= 0) {
					mult_x *= scales->joints_x[joint];
				}
				if (scales->usejoints_y && scales->joints_y && scales->joints_y[joint] >= 0) {
					mult_y *= scales->joints_y[joint];
				}
				if (scales->usejoints_z && scales->joints_z && scales->joints_z[joint] >= 0) {
					mult_z *= scales->joints_z[joint];
				}
			}
		}	
	}

	// Actually apply the net multiplier(s) to the joint
	if (mult_x >= 0 && mult_x != 1.0f) {
		mtx00015df0(mult_x, mtx);
	}
	if (mult_y >= 0 && mult_y != 1.0f) {
		mtx00015e4c(mult_y, mtx);
	}
	if (mult_z >= 0 && mult_z != 1.0f) {
		mtx00015ea8(mult_z, mtx);
	}
}

/// <summary>
/// Adjusts the passed speed value with regard to the chr's applicable botvariety flags, if the botvariety system is active.
/// </summary>
f32 bvTryAdjustMoveSpeed(struct chrdata* chr, f32 speed) {
	const struct bvvariant* variant = NULL;
	u8 i;

	if (!bvIsBotVarietyActive()) { // note: we don't care if any specific flags are set for this function 'cause all bots get speed variance
		return speed;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = &gc_BvVariants[i];

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

/// <summary>
/// Adjusts the passed animspeed value with regard to the chr's applicable botvariety flags, if the botvariety system is active.
/// </summary>
f32 bvTryAdjustAnimSpeed(struct chrdata* chr, f32 animspeed) {
	const struct bvvariant* variant = NULL;
	u8 i;

	if (!bvIsBotVarietyActive() || !bvChrHasVarietyFlags(chr)) {
		return animspeed;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = &gc_BvVariants[i];

		if (CHR_BOTVARIETY_FLAGS & variant->flag && variant->stat.animspeedmult > 0) {
			animspeed *= variant->stat.animspeedmult;
		}
	}

	return animspeed;
}

/// <summary>
/// Determines a bot's crouch position with regard to its applicable botvariety flags, if the botvariety system is active.
/// Returns true if crouchpos was changed.
/// </summary>
bool bvGuessBotCrouchPos(struct chrdata* chr, s32* crouchpos) {
	if (!bvIsBotVarietyActive()) {
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

#undef CHR_BOTVARIETY_FLAGS
