#ifndef _IN_GAME_MOD_BOTVARIETY_H
#define _IN_GAME_MOD_BOTVARIETY_H
#include <ultra64.h>
#include "data.h"
#include "types.h"

// botvariety.c
bool bvIsBotVarietyActive();
bool bvChrHasVarietyFlags(struct chrdata* chr);
f32 bvTryAdjust3DJointScale(struct chrdata* chr, s32 joint, f32 scale);
void bvTryApplyXYZJointScales(struct chrdata* chr, s32 joint, Mtxf* mtx, bool afterpositioned);
f32 bvGetVoicePitch(struct chrdata* chr);
f32 bvTryAdjustMoveSpeed(struct chrdata* chr, f32 speed);
f32 bvTryAdjustAnimSpeed(struct chrdata* chr, f32 animspeed);
f32 bvTryAdjustDamage(struct chrdata* achr, struct chrdata* vchr, struct gset* gset, f32 damage);
bool bvGuessBotCrouchPos(struct chrdata* chr, s32* crouchpos);
void bvTryAdjustCurrentPlayerCameraHeight();
f32 bvTryAdjustCurrentPlayerMeleeRange(f32 range);

// botvarietyspawn.c
void bvspawnPrepVariety(struct chrdata* chr, bool iscurrentplayer);

// botvarietyinit.c
void bvInitMatch();
bool bvTryInitChr(struct chrdata* chr, bool iscurrentplayer);
void bvEndMatch();

// botvarietycommon.c
struct model* bvGetModel(struct chrdata* chr);
struct bvchrdata* bvGetChrMatchData(struct chrdata* chr);
bool bvIsChrCurrentPlayer(struct chrdata* chr);

// Flags
#define BOTVARIETY_FLAG_MINI             0x00000001
#define BOTVARIETY_FLAG_WUMBO            0x00000002
#define BOTVARIETY_FLAG_IMPOSTOR         0x00000004
#define BOTVARIETY_FLAG_SUNGLASSES       0x00000008
#define BOTVARIETY_FLAG_SUPERBATTLEDROID 0x00000010
#define BOTVARIETY_FLAG_PAPERFLAT        0x00000020

#define INDEX_MINI             0
#define INDEX_WUMBO            1
#define INDEX_IMPOSTOR         2
#define INDEX_SUNGLASSES       3
#define INDEX_SUPERBATTLEDROID 4
#define INDEX_PAPERFLAT        5

#define VARIANT_MINI             g_BvVariants[INDEX_MINI]
#define VARIANT_WUMBO            g_BvVariants[INDEX_WUMBO]
#define VARIANT_IMPOSTOR         g_BvVariants[INDEX_IMPOSTOR]
#define VARIANT_SUNGLASSES       g_BvVariants[INDEX_SUNGLASSES]
#define VARIANT_SUPERBATTLEDROID g_BvVariants[INDEX_SUPERBATTLEDROID]
#define VARIANT_PAPERFLAT        g_BvVariants[INDEX_PAPERFLAT]

#define INDEX_ABOMINATION_FIRST INDEX_SUPERBATTLEDROID
#define INDEX_ABOMINATION_LAST  INDEX_SUPERBATTLEDROID // paperflat not enabled
#define ABOMINATION_COUNT 1

#endif
