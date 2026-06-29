#include <ultra64.h>
#include <stdio.h>
#include "constants.h"
#include "game/mod/botvariety.h"
#include "game/mplayer/mplayer.h"
#include "game/mplayer/setup.h"
#include "game/playermgr.h"
#include "bss.h"
#include "../../port/include/system.h"

extern s32 g_MpWeaponSetNum;

u16 trials = 10000;

void weapons_to_string(const u8 weapons[6], char *buffer, size_t bufsize)
{
    if (bufsize == 0) {
        return;
    }
    
    /* Safe formatting with snprintf - produces: "1, 2, 3, 4, 5, 6,\n" */
    snprintf(buffer, bufsize, "%u, %u, %u, %u, %u, %u,",
             weapons[0], weapons[1], weapons[2],
             weapons[3], weapons[4], weapons[5]);
}

void rollrandomweapons() {
  char outstr[40];

  g_MpWeaponSetNum = WEAPONSET_RANDOM;
  mpApplyWeaponSet();

  weapons_to_string(g_MpSetup.weapons, outstr, sizeof(outstr));
  sysLogPrintf(LOG_NOTE, outstr);
}

void rngtestWeapons() {
  for (u16 i = 0; i < trials; i++) {
    rollrandomweapons();
  }
}

void setplayerstringvalue(u8 playernum, struct chrdata* chr, char outstr[]) {
  u8 targetindex = playernum * 3;
  if (CHR_BV_FLAGS & BVFLAG_MINI) {
    outstr[targetindex] = 'M';
  }
  else if (CHR_BV_FLAGS & BVFLAG_WUMBO) {
    outstr[targetindex] = 'W';
  }
  else {
    outstr[targetindex] = 'S';
  }
}

void rollplayervariants(u8 playernum, char outstr[]) {
  setCurrentPlayerNum(playernum);
  bvspawnPrepVariety(g_Vars.currentplayer->prop->chr, true);
  setplayerstringvalue(playernum, g_Vars.currentplayer->prop->chr, outstr);
}

void rollallplayersvariants() {
  char outstr[] = "-, -, -, -,";

  s32 prevplayernum = g_Vars.currentplayernum;

  rollplayervariants(0, outstr);
  if (PLAYERCOUNT() > 1) {
    rollplayervariants(1, outstr);
  }
  if (PLAYERCOUNT() > 2) {
    rollplayervariants(2, outstr);
  } 
  if (PLAYERCOUNT() > 3) {
    rollplayervariants(3, outstr);
  }

  sysLogPrintf(LOG_NOTE, outstr);
  setCurrentPlayerNum(prevplayernum);
}

void rngtestPlayerVariants() {
  for (u16 i = 0; i < trials; i++) {
    rollallplayersvariants();
  }
}