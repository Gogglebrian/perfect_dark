#include <ultra64.h>
#include "constants.h"
#include "game/mod/botvariety.h"
#include "game/playermgr.h"
#include "game/propobj.h"
#include "game/propsnd.h"
#include "bss.h"
#include "lib/model.h"
#include "lib/rng.h"

bool bvIsChrGunfetti(struct chrdata* chr) {
  return (CHR_BV_FLAGS & BVFLAG_GUNFETTI);
}

const u8 countrange_standard[2] = {30, 40}; 
const u8 countrange_mini[2] =     {20, 30};
const u8 countrange_wumbo[2] =    {40, 50};

const f32 pitch_standard = 2.0f;
const f32 pitch_mini = 2.25f;
const f32 pitch_wumbo = 1.75;

const f32 volume_standard = 2.0f;
const f32 volume_mini = 1.75f;
const f32 volume_wumbo = 2.25f;

const u8 guns[] = {
  MPWEAPON_FALCON2_SCOPE,
  MPWEAPON_MAGSEC4,
  MPWEAPON_MAULER,
  MPWEAPON_PHOENIX,
  MPWEAPON_DY357MAGNUM,
  MPWEAPON_DY357LX,
  MPWEAPON_CMP150,
  MPWEAPON_CYCLONE,
  MPWEAPON_CALLISTO,
  MPWEAPON_RCP120,
  MPWEAPON_LAPTOPGUN,
  MPWEAPON_DRAGON,
  MPWEAPON_K7AVENGER,
  MPWEAPON_AR34,
  MPWEAPON_SUPERDRAGON,
  MPWEAPON_SHOTGUN,
  MPWEAPON_REAPER,
  MPWEAPON_SNIPERRIFLE,
  MPWEAPON_FARSIGHT,
  MPWEAPON_DEVASTATOR,
  MPWEAPON_ROCKETLAUNCHER,
  MPWEAPON_SLAYER,
  MPWEAPON_CROSSBOW,
  MPWEAPON_LASER,
  MPWEAPON_PP9I,
  MPWEAPON_CC13,
  MPWEAPON_KL01313,
  MPWEAPON_KF7SPECIAL,
  MPWEAPON_ZZT,
  MPWEAPON_DMC,
  MPWEAPON_AR53,
  MPWEAPON_RCP45,
  MPWEAPON_U13ERKL01313,
};

/**
* Call on death to make a Gunfetti chr drop lots of random guns.
*/
void bvgunfettiPop(struct chrdata* chr) {
  u8 i;
  u8 count, countmin, countmax;
  f32 pitch;
  f32 volume;
  f32 newanimspeed;

  // select values based on bot size flags
  if (CHR_BV_FLAGS & BVFLAG_MINI) {
    countmin = countrange_mini[0];
    countmax = countrange_mini[1];
    pitch = pitch_mini;
    volume = volume_mini;
  }
  else if (CHR_BV_FLAGS & BVFLAG_WUMBO) {
    countmin = countrange_wumbo[0];
    countmax = countrange_wumbo[1];
    pitch = pitch_wumbo;
    volume = volume_wumbo;
  }
  else {
    countmin = countrange_standard[0];
    countmax = countrange_standard[1];
    pitch = pitch_standard;
    volume = volume_standard;
  }

  // limit upper bound to g_MPMaxDroppedWeaponsOnscreen
  countmax = MIN(countmax, g_MPMaxDroppedWeaponsOnscreen);
  
  // if that flattened the range, don't bother rolling
  if (countmax <= countmin) {
    count = countmax;
  }
  // otherwise roll for count
  else {
    count = (rngRandom() % (countmax + 1 - countmin)) + countmin;
  }

  // spawn and drop that many guns
  for (i = 0; i < count; i++) {
    // pick gun
    u8 randindex = rngRandom() % ARRAYCOUNT(guns);
    u8 mpweapon = guns[randindex];
    u8 weaponnum = g_MpWeapons[mpweapon].weaponnum;
    s32 modelnum = playermgrGetModelOfWeapon(weaponnum);

    // create prop
    if (modelnum > 0) {
      struct prop *prop = weaponCreateForChr(chr, modelnum, weaponnum, OBJFLAG_WEAPON_AICANNOTUSE, NULL, NULL);

      // drop
      if (prop) {
        objSetDropped(prop, DROPTYPE_DEFAULT);
        objDrop(prop, true);
      }
    }
  }

  // play sound
  psCreate(NULL, chr->prop, SFX_LAUNCH_ROCKET, -1, volume, 0, 0, PSTYPE_GENERAL, NULL, pitch, NULL, -1, -1, -1, -1);

  // revert anim speed debuff
  newanimspeed = modelGetAnimSpeed(chr->model) / BVVARIANT_GUNFETTI->stat->animspeedmult;
  modelSetAnimSpeed(chr->model, newanimspeed, 0);
}
