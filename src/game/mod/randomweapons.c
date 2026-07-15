#include <ultra64.h>
#include "constants.h"
#include "game/challenge.h"
#include "game/mod/rngtest.h"
#include "game/mplayer/mplayer.h"
#include "bss.h"
#include "lib/rng.h"
#include "../../port/include/system.h"

bool g_FewerRandomWeaponRepeats = false;
bool g_DebugLogRandomWeapons = true;
bool g_WarehouseUsesSixthSlot = false; // Default false: In vanilla, Warehouse doesn't actually spawn the weapon in the sixth slot

#define FLAG_PREVMATCH        0x01
#define FLAG_SECONDPREVMATCH  0x02
#define FLAG_THIRDPREVMATCH   0x04

u8 randomweaponsTallyFlags[NUM_MPWEAPONS]; // Each of the first three bits flags whether the mpweapon was in the first, second, or third previous round respectively.
u8 tallycountweights[] = {2, 2, 1, 0}; // The random-roll weights to use for each possible tally sum.

/**
 * Gets the random-roll weight for a weapon with respect to how many times it's appeared in the past few matches.
 */
u8 randomweaponsGetWeightByTally(u8 mpweapon) {
  u8 tally = randomweaponsTallyFlags[mpweapon];
  u8 nummatches = ((tally & FLAG_PREVMATCH) > 0) + ((tally & FLAG_SECONDPREVMATCH) > 0) + ((tally & FLAG_THIRDPREVMATCH) > 0);
  return tallycountweights[nummatches];
}

/**
 * Call on game init to zero out the running random weapons tally.
 */
void randomweaponsInitTally() {
  for (u8 i = 0; i < NUM_MPWEAPONS; i++) {
    randomweaponsTallyFlags[i] = 0;
  }
}

/**
 * Updates the running tally of weapons that have appeared in the past few Combat Simulator matches.
 * Does not account for duplicates within the same match.
 */
void randomweaponsUpdateTally() {
  bool inmatch = false;

  for (u8 i = 0; i < NUM_MPWEAPONS; i++) {
    inmatch = mpIsWeaponInMatch(g_MpWeapons[i].weaponnum);

    // Correct for the fact that vanilla Warehouse doesn't actually use the sixth weapon slot
    if (inmatch
        && g_Vars.stagenum == STAGE_MP_WAREHOUSE
        && !g_WarehouseUsesSixthSlot
        && g_MpSetup.weapons[5] == i) {
      inmatch = false;
    }

    // move the previous matches' values left by 1 bit, then clear the 1st bit (where we'll set this match's value) and the fourth bit
    if (randomweaponsTallyFlags[i] != 0) {
      randomweaponsTallyFlags[i] = (randomweaponsTallyFlags[i] << 1) & 0x06; // 0x06 = 0000 0110
    }

    // Weapon is in match
    if (inmatch) {
      randomweaponsTallyFlags[i] |= FLAG_PREVMATCH;
    }
  }
}

/**
 * Writes the current selection of weapons to the log.
 */
void randomweaponsLog() {
  char outstr[100];
  weapons_to_string(g_MpSetup.weapons, outstr, sizeof(outstr));
  sysLogPrintf(LOG_NOTE, outstr);
}

/**
 * Rolls for and selects weapons for 5 or 6 slots, with equal chance for each unlocked and unfiltered weapon.
 */
void randomweaponsDoVanillaRoll(bool randomfive) {
  u8 i;
  u8 slotscount = ARRAYCOUNT(g_MpSetup.weapons) - randomfive;
  u8 randomweapons[NUM_MPWEAPONS];

  mpSetRandomWeapons(randomweapons);
  for (i = 0; i < slotscount; i++) {
    mpSetWeaponSlot(i, randomweapons[rngRandom() % g_MpWeaponRandomFilterNum]);
  }
}

/**
 * Rolls for and selects weapons for 5 or 6 slots, with chances weighted against excessive repetition across matches.
 */
void randomweaponsDoWeightedRoll(bool randomfive) {
  u8 i, j;
  u8 slotscount = ARRAYCOUNT(g_MpSetup.weapons) - randomfive;
  u8 weights[NUM_MPWEAPONS];
  u16 totalweight = 0;
  u16 roll;
  u16 picksum = 0;

  // Update weights for each mpweapon and add up totalweight
  for (i = 0; i < NUM_MPWEAPONS; i++) {
    if (!challengeIsFeatureUnlocked(g_MpWeapons[i].unlockfeature)) { // weapon is locked
      weights[i] = 0;
    } else if (g_MpWeaponSetRandomFilters[i] == 0) { // weapon is filtered out
      weights[i] = 0;
    } else {
      weights[i] = randomweaponsGetWeightByTally(i); // weight derived from running tally
      totalweight += weights[i];
    }
  }

  if (totalweight == 0) { // this basically means some tomfoolery is afoot and we'll have none of it
    randomweaponsDoVanillaRoll(randomfive);
    return;
  }
  
  // Pick weapons
  for (i = 0; i < slotscount; i++) {
    roll = rngRandom() % totalweight; // roll [0, totalweight)
    picksum = 0;

    for (j = 0; j < NUM_MPWEAPONS; j++) {
      if (weights[j] > 0) {
        picksum += weights[j];
        if (roll < picksum) {
          g_MpSetup.weapons[i] = j;
          break;
        }
      }
    }
  }
  /*
  u8(*debugtally)[NUM_MPWEAPONS] = &randomweaponsTallyFlags;
  u8(*debugweapons)[NUM_MPWEAPONSLOTS] = &g_MpSetup.weapons;
  ;*/

  }

/**
 * Rolls for and selects weapons for 5 or 6 slots.
 */
void randomweaponsRoll(bool randomfive) {
  if (g_FewerRandomWeaponRepeats) {
    randomweaponsDoWeightedRoll(randomfive);
  } else {
    randomweaponsDoVanillaRoll(randomfive);
  }

  if (g_DebugLogRandomWeapons) {
    randomweaponsLog();
  }
}

/**
 * Debug: tests the weighted random system by simulating a bajillion rolls (updating the tally 
 * after each as if the round had been started and ended) and logging the results.
 */
void randomweaponsDebugTestWeightedRolls(u16 trials) {
  for (u16 i = 0; i < trials; i++) {
    randomweaponsDoWeightedRoll(false);
    randomweaponsUpdateTally();
    randomweaponsLog();
  }
}

#undef FLAG_PREVMATCH
#undef FLAG_SECONDPREVMATCH
#undef FLAG_THIRDPREVMATCH
