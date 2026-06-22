#ifndef _IN_GAME_MOD_BOTVARIETY_H
#define _IN_GAME_MOD_BOTVARIETY_H
#include <ultra64.h>
#include "data.h"
#include "types.h"

// botvariety.c
bool bvIsBotVarietyActive();
bool bvChrHasVarietyFlags(struct chrdata* chr);
f32 bvTryAdjust3DJointScale(struct chrdata* chr, s32 joint, f32 scale);
void bvTryApplyXYZJointScales(struct chrdata* chr, s32 joint, Mtxf* mtx);
void bvTryApplyXZBodyScale(struct chrdata* chr, Mtxf* mtx);
f32 bvGetVoicePitch(struct chrdata* chr);
f32 bvTryAdjustMoveSpeed(struct chrdata* chr, f32 speed);
f32 bvTryAdjustAnimSpeed(struct chrdata* chr, f32 animspeed);
f32 bvTryAdjustDamage(struct chrdata* achr, struct chrdata* vchr, struct gset* gset, f32 damage);
bool bvGuessBotCrouchPos(struct chrdata* chr, s32* crouchpos);
void bvTryAdjustCurrentPlayerCameraHeight();
f32 bvTryAdjustCurrentPlayerMeleeRange(f32 range);
bool bvIsChrSlenderman(struct chrdata* chr);

// botvarietyspawn.c
void bvspawnPrepVariety(struct chrdata* chr, bool iscurrentplayer);

// botvarietyinit.c
void bvInit();
void bvInitMatch();
bool bvTryInitChr(struct chrdata* chr, bool iscurrentplayer);
void bvEndMatch();

// botvarietycommon.c
const struct bvvariant* bvGetVariant(u8 index);
struct model* bvGetModel(struct chrdata* chr);
struct bvchrdata* bvGetChrMatchData(struct chrdata* chr);
bool bvIsChrCurrentPlayer(struct chrdata* chr);
s32 bvIsChrBond(struct chrdata* chr);

// botvarietyexplosive.c
bool bvIsChrExplosive(struct chrdata* chr);
void bvTickExplosiveBot(struct chrdata* botchr);
void bvResetExplosiveBot(struct chrdata* botchr);
void bvApplyExplosiveBotGlow(struct chrdata* botchr, struct modelrenderdata* renderdata);
void bvExplodeBot(struct chrdata* chr, s32 killerplayernum);
f32 bvApplyExplosiveBotExplosionDamageMult(f32 damage);

// Flags
#define BOTVARIETY_FLAG_MINI             0x00000001
#define BOTVARIETY_FLAG_WUMBO            0x00000002
#define BOTVARIETY_FLAG_IMPOSTOR         0x00000004
#define BOTVARIETY_FLAG_SLENDERMAN       0x00000008
#define BOTVARIETY_FLAG_SUNGLASSES       0x00000010
#define BOTVARIETY_FLAG_EXPLOSIVE        0x00000020
#define BOTVARIETY_FLAG_SUPERBATTLEDROID 0x00000040

// Index and count
#define INDEX_MINI               0
#define INDEX_WUMBO              1
#define INDEX_IMPOSTOR           2
#define INDEX_SLENDERMAN         3
#define INDEX_SUNGLASSES         4
#define INDEX_EXPLOSIVE          5
#define INDEX_SUPERBATTLEDROID   6
// If adding variants, remember to update BOTVARIETY_VARIANT_COUNT in constants.h

// Variants
#define VARIANT_MINI             gc_BvVariants[INDEX_MINI]
#define VARIANT_WUMBO            gc_BvVariants[INDEX_WUMBO]
#define VARIANT_IMPOSTOR         gc_BvVariants[INDEX_IMPOSTOR]
#define VARIANT_SLENDERMAN       gc_BvVariants[INDEX_SLENDERMAN]
#define VARIANT_SUNGLASSES       gc_BvVariants[INDEX_SUNGLASSES]
#define VARIANT_EXPLOSIVE        gc_BvVariants[INDEX_EXPLOSIVE]
#define VARIANT_SUPERBATTLEDROID gc_BvVariants[INDEX_SUPERBATTLEDROID]

// Abominations
#define INDEX_ABOMINATION_FIRST INDEX_SUPERBATTLEDROID
#define INDEX_ABOMINATION_LAST  INDEX_SUPERBATTLEDROID
#define ABOMINATION_COUNT 1

#endif
