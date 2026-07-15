#ifndef IN_GAME_MOD_RANDOMWEAPONS_H
#define IN_GAME_MOD_RANDOMWEAPONS_H
#include <ultra64.h>
#include "data.h"
#include "types.h"

void randomweaponsInitTally();
void randomweaponsUpdateTally();
void randomweaponsRoll(bool randomfive);
void randomweaponsDebugTestWeightedRolls(u16 trials);

#endif