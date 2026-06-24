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

/// <summary>
/// Call on death to make a Gunfetti chr drop lots of random guns.
/// </summary>
void bvPopGunfettiBot(struct chrdata* chr) {
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

/* Cut feature - gunfetti gun rattling (it didn't sound good enough)

const f32 rattlevolumemax = 0.3f;
const f32 rattlevolumemin = 0.1f;

const f32 rattlepitchmax = 1.25f;
const f32 rattlepitchmin = 0.75f;

const f32 rattlelagmax = 0.07f;
const f32 rattlelagmin = 0.04f;

const u16 rattlesounds[] = {
  //SFX_PICKUP_AMMO,
  //SFX_RELOAD_04FB,
  SFX_80F6,
  SFX_01D7,
  SFX_01D8,
  //SFX_01DA,
  //SFX_05C5,
	//SFX_05C6,
};

/// <summary>
/// Plays a random gun pickup/rattle sound with slightly randomized pitch and volume.
/// </summary>
void bvDoGunfettiRattleNoise(struct chrdata* chr) {
  u32 random = rngRandom();
  u8 soundbits = random & 0xFF;
  u8 volumebits = (random >> 8)  & 0xFF;
  u8 pitchbits  = (random >> 16) & 0xFF; 

  u16 sound = soundbits % ARRAYCOUNT(rattlesounds);
  f32 volume = (volumebits / 255.0f) * (rattlevolumemax - rattlevolumemin) + rattlevolumemin;
  f32 pitch = (pitchbits / 255.0f) * (rattlepitchmax - rattlepitchmin) + rattlepitchmin;
  
  psCreate(NULL, chr->prop, rattlesounds[sound], -1, volume, 0, 0, PSTYPE_GENERAL, NULL, pitch, NULL, -1, -1, -1, -1);
}

/// <summary>
/// Initializes a gunfetti bot's rattle-noise timer.
/// </summary>
void bvResetGunfettiBot(struct chrdata* chr) {
  struct bvchrdata* bvbot = bvGetChrMatchData(chr);
  bvbot->gunfettitimetonextrattle = -1.0f;
}

/// <summary>
/// Ticks a Gunfetti bot's internal timer and has it make a little gun-rattling noise after each footstep.
/// </summary>
void bvTickGunfettiBot(struct chrdata* chr) {
  struct bvchrdata* bvbot = bvGetChrMatchData(chr);

  // if footstep this frame, cue a rattle sound to play shortly
  if (chr->footstep > 0) {
    bvbot->gunfettitimetonextrattle = (RANDOMFRAC() * (rattlelagmax - rattlelagmin) + rattlelagmin) / modelGetAnimSpeed(chr->model);  
  }
  // otherwise run the timer down until rattle plays
  else {
    // 0 to -1.0f means time to rattle; -1.0f or less means disabled
    if (bvbot->gunfettitimetonextrattle <= 0 && bvbot->gunfettitimetonextrattle > -0.99f) {
      bvDoGunfettiRattleNoise(chr);
      bvbot->gunfettitimetonextrattle = -1.0f; // disable
    }
    // time left on the timer, tick it down
    else if (bvbot->gunfettitimetonextrattle > 0) {
      bvbot->gunfettitimetonextrattle -= 0.016666f * g_Vars.lvupdate60freal;
    }
  }
}
*/
