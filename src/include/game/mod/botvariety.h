#ifndef _IN_GAME_MOD_BOTVARIETY_H
#define _IN_GAME_MOD_BOTVARIETY_H
#include <ultra64.h>
#include "data.h"
#include "types.h"

bool botvarietyIsActive();
bool botvarietyChrHasVarietyFlags(struct chrdata* chr);
void botvarietyApplyOnSpawn(struct chrdata* chr, bool iscurrentplayer, bool respawning);
f32 botvarietyTryAdjustJointScale(struct chrdata* chr, s32 joint, f32 scale);
f32 botvarietyTryAdjustMoveSpeed(struct chrdata* chr, f32 speed);
f32 botvarietyTryAdjustAnimSpeed(struct chrdata* chr, f32 animspeed);
f32 botvarietyTryAdjustDamage(struct chrdata* achr, struct chrdata* vchr, struct gset* gset, f32 damage);
void botvarietyTryAdjustCurrentPlayerCameraHeight();
bool botvarietyGuessCrouchpos(struct chrdata* chr, s32* crouchpos);
f32 botvarietyTryAdjustCurrentPlayerMeleeRange(f32 range);

#endif
