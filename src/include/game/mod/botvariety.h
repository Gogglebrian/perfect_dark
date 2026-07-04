#ifndef _IN_GAME_MOD_BOTVARIETY_H
#define _IN_GAME_MOD_BOTVARIETY_H
#include <ultra64.h>
#include "data.h"
#include "types.h"

 // This u32 isn't used in combat simulator so we'll hackily borrow it for our variant flags
#define CHR_BV_FLAGS chr->convtalk

// Flags
#define BVFLAG_MINI             0x00000001
#define BVFLAG_WUMBO            0x00000002
#define BVFLAG_IMPOSTOR         0x00000004
#define BVFLAG_SLENDERMAN       0x00000008
#define BVFLAG_SUNGLASSES       0x00000010
#define BVFLAG_EXPLOSIVE        0x00000020
#define BVFLAG_GUNFETTI         0x00000040
#define BVFLAG_SBD              0x00000080

// Index and count
#define BVINDEX_MINI               0
#define BVINDEX_WUMBO              1
#define BVINDEX_IMPOSTOR           2
#define BVINDEX_SLENDERMAN         3
#define BVINDEX_SUNGLASSES         4
#define BVINDEX_EXPLOSIVE          5
#define BVINDEX_GUNFETTI           6
#define BVINDEX_SBD                7
// If adding variants, remember to update BOTVARIETY_VARIANT_COUNT in constants.h

// Variants
#define BVVARIANT_MINI             gc_BvVariants[BVINDEX_MINI]
#define BVVARIANT_WUMBO            gc_BvVariants[BVINDEX_WUMBO]
#define BVVARIANT_IMPOSTOR         gc_BvVariants[BVINDEX_IMPOSTOR]
#define BVVARIANT_SLENDERMAN       gc_BvVariants[BVINDEX_SLENDERMAN]
#define BVVARIANT_SUNGLASSES       gc_BvVariants[BVINDEX_SUNGLASSES]
#define BVVARIANT_EXPLOSIVE        gc_BvVariants[BVINDEX_EXPLOSIVE]
#define BVVARIANT_GUNFETTI         gc_BvVariants[BVINDEX_GUNFETTI]
#define BVVARIANT_SBD              gc_BvVariants[BVINDEX_SBD]

// Abominations
#define BVINDEX_ABOMINATION_FIRST BVINDEX_SBD
#define BVINDEX_ABOMINATION_LAST  BVINDEX_SBD
#define BV_ABOMINATION_COUNT 1

// botvariety.c
bool bvIsBotVarietyActive();
bool bvChrHasVarietyFlags(struct chrdata* chr);
void bvTickCurrentPlayerAliveUnpausedEarly(struct chrdata* chr);
void bvProcOnDeath(struct chrdata* chr, s32 killerplayernum);
void bvProcOnCorpseFadeBegin(struct chrdata* chr);
void bvProcOnDamageTaken(struct chrdata* achr, struct chrdata* vchr,  struct gset* gset, f32 damage);
void bvTryApplyLateColourTweaks(struct chrdata* chr, struct modelrenderdata* renderdata);
f32 bvTryAdjust3DJointScale(struct chrdata* chr, s32 joint, f32 scale);
void bvTryApplyXYZJointScales(struct chrdata* chr, s32 joint, Mtxf* mtx);
void bvTryApplyXZBodyScale(struct chrdata* chr, Mtxf* mtx);
f32 bvGetVoicePitch(struct chrdata* chr);
f32 bvTryAdjustMoveSpeed(struct chrdata* chr, f32 speed);
f32 bvTryAdjustAnimSpeed(struct chrdata* chr, f32 animspeed);
f32 bvTryAdjustDamage(struct chrdata* achr, struct chrdata* vchr, struct gset* gset, f32 damage);
bool bvTryAdjustBloodColour(struct chrdata* chr, u8 *colour1, u32 *colour2);
void bvTryAdjustCurrentPlayerCameraHeight();
f32 bvTryAdjustCurrentPlayerMeleeRange(f32 range);

// botvarietybot.c
bool bvbotCanUseWeapon(struct chrdata* chr, s32 weaponnum, s32 funcnum);
bool bvbotCanPickupWeapon(struct chrdata* chr, s32 weaponnum);
bool bvbotCanSeeThroughCloak(struct chrdata* chr);
bool bvbotShouldCalcTargetLoSEveryFrame(struct chrdata* chr);
bool bvbotShouldCalcTargetDistEveryFrame(struct chrdata* chr);
bool bvbotShouldChangeTargetByLoS(struct chrdata* chr);
bool bvbotCanAttack(struct chrdata* chr);
bool bvbotShouldStandStill(struct chrdata* chr);
bool bvbotCanSeeChr(struct chrdata* botchr, struct chrdata* otherchr);
bool bvbotGuessCrouchPos(struct chrdata* chr, s32* crouchpos);
void bvbotTickAliveUnpausedEarly(struct chrdata* chr);

// botvarietyspawn.c
void bvspawnPrepVariety(struct chrdata* chr, bool iscurrentplayer);

// botvarietyinit.c
void bvInit();
void bvInitMatch();
bool bvTryInitChr(struct chrdata* chr, bool iscurrentplayer);
void bvEndMatch();
void bvResetChrDataForSpawn(struct bvchrdata* bvchr);

// botvarietycommon.c
const struct bvvariant* bvGetVariant(u8 index);
struct model* bvGetModel(struct chrdata* chr);
struct bvchrdata* bvGetChrMatchData(struct chrdata* chr);
bool bvIsChrCurrentPlayer(struct chrdata* chr);
s32 bvIsChrBond(struct chrdata* chr);

// botvarietyexplosive.c
bool bvIsChrExplosive(struct chrdata* chr);
void bvTickExplosiveBot(struct chrdata* botchr);
void bvApplyExplosiveBotGlow(struct chrdata* botchr, struct modelrenderdata* renderdata);
void bvExplodeBot(struct chrdata* chr, s32 killerplayernum);
f32 bvApplyExplosiveBotExplosionDamageMult(f32 damage);

// botvarietygunfetti.c
bool bvIsChrGunfetti(struct chrdata* chr);
void bvPopGunfettiBot(struct chrdata* chr);

// botvarietyslenderman.c
bool bvIsChrSlenderman(struct chrdata* chr);
bool bvslendermanCanSpawn();
bool bvslendermanShouldOtherChrTick(struct chrdata* chr);
bool bvslendermanShouldDoStatic(struct chrdata* chr);
bool bvslendermanIsVisibleToChr(struct chrdata* chr);
bool bvslendermanCanAttack();
struct chrdata* bvslendermanGetChr();
struct chrdata* bvslendermanGetTargetChr();
void bvslendermanSpawn(struct chrdata* chr);
void bvslendermanDie(struct chrdata* chr);
void bvslendermanDespawn();
void bvslendermanOnDamageTaken(struct chrdata* achr);
void bvslendermanOnCorpseFade(struct chrdata* chr);
void bvslendermanTick(struct chrdata* chr);
void bvslendermanTickOtherChr(struct chrdata* chr);
void bvslendermanGetBloodColours(u8 *colour1, u32 *colour2);
f32 bvslendermanGetMeleeDamageMult();
f32 bvslendermanGetSpeedMult();
f32 bvslendermanGetAnimSpeedMult();
u8 bvslendermanGetAlpha();
void bvslendermanApplyColour(struct chrdata* botchr, struct modelrenderdata* renderdata);
Gfx *bvslendermanApplyVictimStatic(Gfx *gdl);

#endif
