#ifndef IN_GAME_CUSTOMAMMO_H
#define IN_GAME_CUSTOMAMMO_H
#include <ultra64.h>
#include "data.h"
#include "types.h"

bool ammoTrySetCustomForMultiCrate(struct multiammocrateobj* crate, bool secondary, u16 ammotype, u16 quantity);
bool ammoIsCustomType(u16 ammotype);
bool ammoIsCustomInMultiCrate(struct multiammocrateobj* crate, bool secondary);
void ammoHandleCustomPickup(struct multiammocrateobj* crate);
u16 ammoGetCustomTypeInMultiCrate(struct multiammocrateobj* crate, bool secondary);
u16 ammoGetCustomQuantityInMultiCrate(struct multiammocrateobj* crate, bool secondary);
u16 ammoGetTypeFromMultiCrateByIndex(struct multiammocrateobj* crate, s32 i);
u16 ammoGetQuantityFromMultiCrateByIndex(struct multiammocrateobj* crate, s32 i);

#endif
