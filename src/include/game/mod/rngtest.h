#ifndef _IN_GAME_MOD_RNGTEST_H
#define _IN_GAME_MOD_RNGTEST_H
#include <ultra64.h>
#include "data.h"
#include "types.h"

void rngtestWeapons();
void rngtestPlayerVariants();
void weapons_to_string(const u8 weapons[6], char *buffer, size_t bufsize);

#endif