#include <ultra64.h>
#include "constants.h"
#include "game/propobj.h"
#include "bss.h"
#include "lib/rng.h"
#include "lib/model.h"

//=== Custom ammo mutliammocrate funcs ======================================================================
// These implement a hacky workaround the 19 ammo type limit in multiammocrates.
// - The approach is to store up to two custom ammo types/quantities when a multiammocrate is created,
// putting them in the first four of the 19 slots' modelnum fields.
// - I believe these modelnums are unused in combat simualtor, but just in case, wait to
// overwite them til after they've been passed to setupLoadModeldef.

bool ammoIsCustomType(u16 ammotype) {
	return (ammotype > AMMOTYPE_LASTFORMULTICRATE_VANILLA && ammotype <= AMMOTYPE_LASTFORMULTICRATE_CUSTOM);
}

/**
* Stores custom ammo type/qty in a combat simulator multiammocrate
* slots 0-1 for primary type/qty, slots 2-3 for secondary type-qty
*
* <returns>true if custom ammo successfully set</returns>
*/
bool ammoTrySetCustomForMultiCrate(struct multiammocrateobj* crate, bool secondary, u16 ammotype, u16 ammoquantity) {
	u16 typeslot = 0;

	if (!crate || !g_Vars.normmplayerisrunning) { // combat simulator only
		return false;
	}

	if (secondary) {
		typeslot = 2; // slots 0 and 1 for primary, 2 and 3 for secondary
	}

	if (ammotype > AMMOTYPE_LASTFORMULTICRATE_VANILLA && ammoquantity > 0) {
		crate->slots[typeslot].modelnum = ammotype;
		crate->slots[typeslot + 1].modelnum = ammoquantity;
		return true;
	}

	return false;
}

bool ammoIsCustomInMultiCrate(struct multiammocrateobj* crate, bool secondary) {
	u16 typeslot = 0;

	if (!crate || !g_Vars.normmplayerisrunning) { // combat simulator only
		return false;
	}

	if (secondary) {
		typeslot = 2; // slots 0 and 1 for primary, 2 and 3 for secondary
	}

	return (crate->slots[typeslot].modelnum != 0xffff
		&& crate->slots[typeslot].modelnum > AMMOTYPE_LASTFORMULTICRATE_VANILLA
		&& crate->slots[typeslot + 1].modelnum != 0xffff
		&& crate->slots[typeslot + 1].modelnum > 0);
}

/**
* Returns the custom ammo type or quantity from a multiammocrate, if any
*
* <returns>quantity if param getquantity==true, returns Ammotype if getquantity==false</returns>
*/
u16 ammoGetCustomDataInMultiCrate(struct multiammocrateobj* crate, bool secondary, bool getquantity) {
	u16 typeslot = 0;

	if (secondary) {
		typeslot = 2; // slots 0 and 1 for primary, 2 and 3 for secondary
	}

	if (ammoIsCustomInMultiCrate(crate, secondary)) { // this also ensures crate's not null and we're in combat simulator
		if (getquantity) {
			return crate->slots[typeslot + 1].modelnum;
		}
		else {
			return crate->slots[typeslot].modelnum;
		}
	}

	return 0;
}

u16 ammoGetCustomTypeInMultiCrate(struct multiammocrateobj* crate, bool secondary) {
	return ammoGetCustomDataInMultiCrate(crate, secondary, 0);
}

u16 ammoGetCustomQuantityInMultiCrate(struct multiammocrateobj* crate, bool secondary) {
	return ammoGetCustomDataInMultiCrate(crate, secondary, 1);
}

void ammoHandleCustomPickup(struct multiammocrateobj* crate) {
	//handle primary
	if (ammoIsCustomInMultiCrate(crate, 0)) {
		ammoHandlePickup(crate->slots[0].modelnum, crate->slots[1].modelnum, false, true);
	}

	//handle secondary
	if (ammoIsCustomInMultiCrate(crate, 1)) {
		ammoHandlePickup(crate->slots[2].modelnum, crate->slots[3].modelnum, false, true);
	}
}

/**
* Get ammotype or quantity from a multiammocrate by index (0-20) where
* - 0-18 refer to the dedicated slots for the first 19 vanilla ammotypes,
* - 19-20 refer to two custom ammo slots (primary and secondary) that can each store any custom ammo type
*
* <param name="i">0-20</param>
* <returns>ammo type, or quantity if param getquantity==true</returns>
*/
u16 ammoGetDataFromMultiCrateByIndex(struct multiammocrateobj* crate, s32 i, bool getquantity) {
	if (i < MULTIAMMOCRATE_SLOTS_COUNT_VANILLA) { // Slots for vanilla ammo types through SEDATIVE
		if (getquantity) {
			return crate->slots[i].quantity;
		}
		else {
			return i + 1;
		}
	}
	else if (i < MULTIAMMOCRATE_SLOTS_COUNT) { // 2 bespoke slots (primary and secondary) for custom ammo types
		if (getquantity) {
			return ammoGetCustomQuantityInMultiCrate(crate, i - MULTIAMMOCRATE_SLOTS_COUNT_VANILLA);
		}
		else {
			return ammoGetCustomTypeInMultiCrate(crate, i - MULTIAMMOCRATE_SLOTS_COUNT_VANILLA);
		}
	}

	return 0;
}

/**
* Get ammotype from a multiammocrate by index (0-20) where
* - 0-18 refer to the dedicated slots for the first 19 vanilla ammotypes,
* - 19-20 refer to two custom ammo slots (primary and secondary) that can each store any custom ammo type
*
* <param name="i">0-20</param>
* <returns>ammo type</returns>
*/
u16 ammoGetTypeFromMultiCrateByIndex(struct multiammocrateobj* crate, s32 i) {
	return ammoGetDataFromMultiCrateByIndex(crate, i, false);
}

/**
* Get ammo quantity from a multiammocrate by index (0-20) where
* - 0-18 refer to the dedicated slots for the first 19 vanilla ammotypes,
* - 19-20 refer to two custom ammo slots (primary and secondary) that can each store any custom ammo type
*
* <param name="i">0-20</param>
* <returns>ammo quantity</returns>
*/
u16 ammoGetQuantityFromMultiCrateByIndex(struct multiammocrateobj* crate, s32 i) {
	return ammoGetDataFromMultiCrateByIndex(crate, i, true);
}
