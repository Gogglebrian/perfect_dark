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
// (replacing the vanilla height variance so as not to conflict with the major variants' scale changes).

/**
* Is Combat Simulator running with botvariety enabled?
*/
bool bvIsBotVarietyActive() {
	return g_Vars.normmplayerisrunning && g_MpSetup.options & MPOPTION_BOTVARIETY;
}

/**
* Does this character have any botvariety flags?
*/
bool bvChrHasVarietyFlags(struct chrdata* chr) {
	return CHR_BV_FLAGS != 0;
}

/**
* Should only be called from bvTickCurrentPlayerAliveUnpausedEarly or bvbotTickAliveUnpausedEarly
* Ticks unique botvariety behaviors to be procced at the beginning of any chr's (bots and players) unpaused tick
*/
void bvTickChrAliveUnpaused(struct chrdata* chr) {
	// Tick spawned timer
	chr->bvchr->spawntime += (0.016666f * g_Vars.lvupdate60freal);
	
	// Tick effects of slenderman on other characters
	if (bvslenderShouldDoVictimTick(chr)) {
		bvslenderTickVictim(chr);
	}
}

/**
* Ticks unique botvariety player-side behaviors to be procced at the beginning of a living player's unpaused tick.
*/
void bvTickCurrentPlayerAliveUnpaused(struct chrdata* chr) {
	bvTickChrAliveUnpaused(chr);	
}

/**
* Handles unique variant behaviors to be procced the moment a chr dies.
*/
void bvProcOnDeath(struct chrdata* chr, s32 killerplayernum) {
	// Explosive bots explode on death
	if (bvIsChrExplosive(chr)) {
		bvexplosiveExplode(chr, killerplayernum);
	}
	// Gunfetti bots drop a shitton of guns on death
	else if (bvIsChrGunfetti(chr)) {
		bvgunfettiPop(chr);
	}
	// slenderman makes a horrible noise on death
	else if (bvIsChrSlenderman(chr)) {
		bvslenderDie(chr);
	}
}

/**
* Handles unique variant behaviors to be procced after death as the chr's corpse begins to fade.
*/
void bvProcOnCorpseFadeBegin(struct chrdata* chr) {
	if (bvIsChrSlenderman(chr)) {
		bvslenderOnCorpseFade(chr);
	}
}

/**
* Attempts to set the blood colour for this chr per their applicable botvariety flags, if any.
* Returns true if colours set, false if not set.
*/
bool bvTryAdjustBloodColour(struct chrdata* chr, u8 *colour1, u32 *colour2) {
	if (bvIsChrSlenderman(chr)) {
		bvslenderGetBloodColours(colour1, colour2);
		return true;
	}

	return false;
}

/**
* Apply any last-minute color tweaks based on the character's applicable botvariety flags.
*/
void bvTryApplyLateColourTweaks(struct chrdata* chr, struct modelrenderdata* renderdata) {
	// Explosive bots: flash white-orange
	if (bvIsChrExplosive(chr)) {
		bvexplosiveApplyGlow(chr, renderdata);
	}
	// Slenderman: dark color
	else if (bvIsChrSlenderman(chr)) {
		bvslenderApplyColour(renderdata);
	}
}

/**
* Applies any adjustments to radar dot outline and fill colours based on the character's applicable botvariety flags.
*/
void bvTryAdjustRadarDotColour(struct chrdata* chr, u32 *fillcolour, u32 *linecolour) {
	// Explosive bots: radar dot flashes white-orange
	if (bvIsChrExplosive(chr)) {
		bvexplosiveAdjustRadarDotColour(chr, fillcolour, linecolour);
	}
}

/**
* Returns a voice pitch multiplier with regard to the character's applicable botvariety flags, if the botvariety system is active.
* Returns -1 if no changes
*/
f32 bvGetVoicePitch(struct chrdata* chr) {
	const struct bvvariant* variant = NULL;
	u8 i;
	f32 pitch = -1;

	if (!bvChrHasVarietyFlags(chr)) {
		return -1;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = bvGetVariant(i);

		if (CHR_BV_FLAGS & variant->flag && variant->body && variant->body->voicepitch > 0) {
			if (pitch < 0) {
				pitch = 1.0f;
			}
			pitch *= variant->body->voicepitch;
		}
	}

	return pitch;
}

/**
* Adjusts the current player's camera height with regard to player chr's applicable botvariety flags, if the botvariety system is active.
*/
void bvTryAdjustCurrentPlayerCameraHeight() {
	struct chrdata* chr = g_Vars.currentplayer->prop->chr;
	f32 mult = 1.0f;
	bool changed = false;

	if (CHR_BV_FLAGS & BVFLAG_MINI && BVVARIANT_MINI->body->camheight > 0) {
		mult = BVVARIANT_MINI->body->camheight;
		changed = true;
	}
	else if (CHR_BV_FLAGS & BVFLAG_WUMBO && BVVARIANT_WUMBO->body->camheight > 0) {
		mult = BVVARIANT_WUMBO->body->camheight;
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

#define ATTACKER_BV_FLAGS achr->bvchr->flags
#define VICTIM_BV_FLAGS   vchr->bvchr->flags

/**
* Call when a chr takes damage to proc any unique botvariety behaviors.
*/
void bvProcOnDamageTaken(struct chrdata* vchr, struct chrdata* achr,  struct gset* gset, f32 damage) {
	if (bvIsChrSlenderman(vchr)) {
		bvslenderOnDamageTaken(vchr, achr);
	}
}

/**
* Checks whether this variant's unshielded damage-taken multiplier should be applied to this chr right now.
* Default true
*/
bool bvShouldApplyVariantDamageTakenMult(struct chrdata* vchr, const struct bvvariant* variant) {
	if (vchr->bvbot) {
		if (variant == BVVARIANT_IMPOSTOR && vchr->bvbot->spreeflags & BVFLAG_IMPOSTOR) { // Impostor health bonus doesn't apply to spree-spawns
			return false;
		}
		else if (variant == BVVARIANT_SUNGLASSES && VICTIM_BV_FLAGS & BVFLAG_IMPOSTOR) { // Sunglasses health bonus doesn't apply if bot is also an Impostor, UNLESS Impostor mult was skipped due to spree
			if (vchr->bvbot->spreeflags & BVFLAG_IMPOSTOR) {
				return true;
			}
			return false;
		}
	}
	return true;
}

/**
* Adjusts damage with regard to the attacker and victims' respective applicable botvariety flags, if the botvariety system is active.
*/
f32 bvTryAdjustDamage(struct chrdata* achr, struct chrdata* vchr, struct gset* gset, f32 damage) {
	const struct bvvariant* variant = NULL;
	u8 i;

	if (!(bvChrHasVarietyFlags(achr) || bvChrHasVarietyFlags(vchr))) {
		return damage;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = bvGetVariant(i);

		// Handle attacker damage factors
		if (ATTACKER_BV_FLAGS & variant->flag && variant->stat) {
			// Handle blunt damage
			if (gsetHasFunctionFlags(gset, FUNCFLAG_BLUNTIMPACT)) {
				// Handle disarm - set to flat value if it's higher than the current value (default 0), but else apply general bluntdamagemult
				if (gsetHasFunctionFlags(gset, FUNCFLAG_DISARM) && variant->stat->disarmdamage > 0 && damage < variant->stat->disarmdamage) {
					damage = variant->stat->disarmdamage;
				}
				// Handle non-disarm blunt damage multiplier
				else if (variant->stat->bluntdamagemult >= 0) {
					damage *= variant->stat->bluntdamagemult;
				}
			}
		}

		// Handle victim damage factors
		if (VICTIM_BV_FLAGS & variant->flag && variant->stat) {
			// Handle unshielded damage taken mult
			if (vchr->cshield <= 0
				&& variant->stat->damagetakenmult > 0
				&& bvShouldApplyVariantDamageTakenMult(vchr, variant)) {
				damage *= variant->stat->damagetakenmult;
			}
		}
	}

	// Apply situational slenderman damage mult
	if (bvIsChrSlenderman(achr)) {
		damage *= bvslenderGetMeleeDamageMult(achr);
	}

	return damage;
}
#undef ATTACKER_BV_FLAGS
#undef VICTIM_BV_FLAGS 

/**
* Adjusts the passed melee range value with regard to the currentplayer chr's applicable botvariety flags, if the botvariety system is active.
*/
f32 bvTryAdjustCurrentPlayerMeleeRange(f32 range) {
	struct chrdata* chr = g_Vars.currentplayer->prop->chr;
	const struct bvvariant* variant = NULL;
	u8 i;

	if (!bvChrHasVarietyFlags(chr)) {
		return range;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = bvGetVariant(i);

		if (CHR_BV_FLAGS & variant->flag && variant->stat && variant->stat->meleerangemult > 0) {
			range *= variant->stat->meleerangemult;
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

// the four bond bodies have different joint numbers
#define BOND_JOINT_RKNEE 5
#define BOND_JOINT_RANKLE 6
#define BOND_JOINT_RFOOT 7
#define BOND_JOINT_LKNEE 8
#define BOND_JOINT_LANKLE 9
#define BOND_JOINT_LFOOT 10
#define BOND_JOINT_RWRIST 11
#define BOND_JOINT_RHAND 12
#define BOND_JOINT_LWRIST 13
#define BOND_JOINT_LHAND 14

/**
* Given a jointnumber from a Bond model, returns the normal jointnumber for that joint
*/
s32 bvGetJointFromBondJoint(s32 joint) {
	switch (joint) {
		case JOINT_NECK:
		case JOINT_WAIST:
		case JOINT_RSHOULDER:
		case JOINT_LSHOULDER:
		case 4:
			return joint;
		case BOND_JOINT_RWRIST:
		case BOND_JOINT_RHAND:
		case BOND_JOINT_LWRIST:
		case BOND_JOINT_LHAND:
			return joint - 6;
		case BOND_JOINT_RKNEE:
		case BOND_JOINT_RANKLE:
		case BOND_JOINT_RFOOT:
		case BOND_JOINT_LKNEE:
		case BOND_JOINT_LANKLE:
		case BOND_JOINT_LFOOT:
			return joint + 4;
		default:
			return joint;
	}
}

/**
* Adjusts the passed scale value for the joint with regard to the chr's applicable botvariety flags, if the botvariety system is active.
*/
f32 bvTryAdjust3DJointScale(struct chrdata* chr, s32 joint, f32 scale) {
	f32 mult = 1.0f;
	const struct bvvariant* variant = NULL;
	u8 i;
	f32 jointscale;

	if (!bvChrHasVarietyFlags(chr)) {
		return scale;
	}

	if (bvIsChrBond(chr)) {
		joint = bvGetJointFromBondJoint(joint);
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = bvGetVariant(i);
		jointscale = -1.0f;

		if (CHR_BV_FLAGS & variant->flag && variant->body) {
			switch (joint) {
			case JOINT_NECK:
				jointscale = variant->body->scalehead;     break;
			case JOINT_LSHOULDER:
			case JOINT_RSHOULDER:
				jointscale = variant->body->scaleshoulder; break;
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

/**
* Applies variants' separate 1D joint scales to the chr's joint, per the chr's applicable botvariety flags, if the botvariety system is active.
*/
void bvTryApplyXYZJointScales(struct chrdata* chr, s32 joint, Mtxf* mtx) {
	const struct bvvariant* variant = NULL;
	const struct bvvariantxyzscales* scales = NULL;
	f32 mult_x = 1.0f;
	f32 mult_y = 1.0f;
	f32 mult_z = 1.0f;
	u8 i;

	if (!bvChrHasVarietyFlags(chr)) {
		return;
	}

	if (bvIsChrBond(chr)) {
		joint = bvGetJointFromBondJoint(joint);
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = bvGetVariant(i);

		if (CHR_BV_FLAGS & variant->flag && variant->body) {
			scales = variant->body->xyzscales;
			
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

				if (scales->beyondpelvismult_x >= 0) {
					if (joint == JOINT_WAIST) {
						mult_x *= scales->beyondpelvismult_x;
					}
					else if (joint == JOINT_LKNEE || joint == JOINT_RKNEE) { // apply mult to Y for limbs (because arms and legs are both splayed sidewise in the Tpose)
						mult_y *= scales->beyondpelvismult_x;
					}
				}
				if (scales->beyondpelvismult_z >= 0) {
					if (joint == JOINT_WAIST || joint == JOINT_LKNEE || joint == JOINT_RKNEE) {
						mult_z *= scales->beyondpelvismult_z;
					}
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

/**
* Applies variants' whole-model X and Z scalings, per the chr's applicable botvariety flags, if the botvariety system is active.
* Mainly used for shrinking or growing the pelvis, which isn't considered a joint for scaling purposes.
*/
void bvTryApplyXZBodyScale(struct chrdata* chr, Mtxf* mtx) {
	const struct bvvariant* variant = NULL;
	const struct bvvariantxyzscales* scales = NULL;
	f32 mult_x = 1.0f;
	f32 mult_z = 1.0f;
	u8 i;

	if (!bvChrHasVarietyFlags(chr)) {
		return;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = bvGetVariant(i);

		if (CHR_BV_FLAGS & variant->flag && variant->body) {
			scales = variant->body->xyzscales;
			
			// Multiply into the running multiplier
			if (scales) {
				if (scales->bodyscale_x >= 0) {
					mult_x *= scales->bodyscale_x;
				}
				if (scales->bodyscale_z >= 0) {
					mult_z *= scales->bodyscale_z;
				}
			}
		}	
	}

	// Actually apply the net multiplier(s) to the body mtx
	if (mult_x >= 0 && mult_x != 1.0f) {
		mtx00015df0(mult_x, mtx);
	}
	if (mult_z >= 0 && mult_z != 1.0f) {
		mtx00015ea8(mult_z, mtx);
	}
}

/**
* Adjusts the passed speed value with regard to the chr's applicable botvariety flags, if the botvariety system is active.
*/
f32 bvTryAdjustMoveSpeed(struct chrdata* chr, f32 speed) {
	const struct bvvariant* variant = NULL;
	u8 i;

	if (!bvChrHasVarietyFlags(chr)) {
		return speed;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = bvGetVariant(i);

		if (CHR_BV_FLAGS & variant->flag && variant->stat && variant->stat->movespeedmult > 0) {
			speed *= variant->stat->movespeedmult;
		}
	}

	// Apply situational slenderman damage mult
	if (bvIsChrSlenderman(chr)) {
		speed *= bvslenderGetSpeedMult(chr);
	}

	return speed;
}

/**
* Adjusts the passed animspeed value with regard to the chr's applicable botvariety flags, if the botvariety system is active.
*/
f32 bvTryAdjustAnimSpeed(struct chrdata* chr, f32 animspeed) {
	const struct bvvariant* variant = NULL;
	u8 i;

	if (!bvChrHasVarietyFlags(chr)) {
		return animspeed;
	}

	for (i = 0; i < BOTVARIETY_VARIANT_COUNT; i++) {
		variant = bvGetVariant(i);

		if (CHR_BV_FLAGS & variant->flag && variant->stat && variant->stat->animspeedmult > 0) {
			animspeed *= variant->stat->animspeedmult;
		}
	}

	// Apply situational slenderman damage mult
	if (bvIsChrSlenderman(chr)) {
		animspeed *= bvslenderGetAnimSpeedMult(chr);
	}

	return animspeed;
}
