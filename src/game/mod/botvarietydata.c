#include <ultra64.h>
#include "constants.h"
#include "game/mod/botvariety.h"

const bool g_BvDebugAllVariants = false;
const bool g_BvDebugAllSprees = false;
const bool g_BvDebugAllowPlayerAbominations = false;

struct bvmatchdata g_BvMatch;
u16 g_BvSpreeCooldowns[BOTVARIETY_VARIANT_COUNT]; // persists across rounds

const struct bvvariantbodydata bodyMini = {
	0.605f,  // body
	1.65f,   // head
	1.3f,    // shoulder
	0.63375f,// camera height (player)
	1.18f,   // voice pitch
	NULL,    // address of xyz scales
};
const struct bvvariantstats statsMini = {
	1.1f,    // movespeedmult
	1.5f,    // animspeedmult
	-1.0f,   // damagetakenmult
	0.9f,    // bluntdamagemult
	0,       // disarmdamage (default=0)
	-1.0f,   // meleerangemult
};
const struct bvvariant variantMini = {
	BVFLAG_MINI, BVINDEX_MINI,
	{ // spawn chances
		1.0f / 20, // bot
		1.0f / 60, // player
		1.0f / 4,  // debug
		3.0f / 4,  // spree
	},
	{ // spree data
		1.0f / 600, // trigger chance
		1.0f / 2, // debug chance
		16, 36,     // min/max spawn count
		-1,        // cooldown
	},
	&bodyMini,
	&statsMini,
	false, false, // debug variant/spree
};

const struct bvvariantbodydata bodyWumbo = {
	1.5f,    // body
	0.8f,    // head
	1.4f,    // shoulder
	1.4517f, // camera height (player)
	0.8f,    // voice pitch
	NULL,    // address of xyz scales
};
const struct bvvariantstats statsWumbo = {
	0.95f,   // movespeedmult
	0.9f,    // animspeedmult
	1.0f / 3.25f, // damagetakenmult (=3.25x health)
	2.0f,    // bluntdamagemult
	1.0f,    // disarmdamage (default=0)
	2.0f,    // meleerangemult
};
const struct bvvariant variantWumbo = {
	BVFLAG_WUMBO, BVINDEX_WUMBO,
	{ // spawn chances
		1.0f / 30, // bot
		1.0f / 60, // player
		1.0f / 4,  // debug
		3.0f / 4,  // spree
	},
	{ // spree data
		1.0f / 700, // trigger chance
		1.0f / 2, // debug chance
		12, 32,     // min/max spawn count
		-1,        // cooldown
	},
	&bodyWumbo,
	&statsWumbo,
	false, false, // debug variant/spree
};

const struct bvvariantstats statsImpostor = {
	-1.0f, // movespeedmult
	-1.0f, // animspeedmult
	 1.0f / 1.5f, // damagetakenmult (=1.5x health) - doesn't apply if spree-spawned (bvShouldApplyVariantDamageTakenMult)
	-1.0f, // bluntdamagemult
	-1.0f, // disarmdamage (default=0)
	-1.0f, // meleerangemult
};
const struct bvvariant variantImpostor = {
	BVFLAG_IMPOSTOR, BVINDEX_IMPOSTOR,
	{ // spawn chances
		1.0f / 400,  // bot
		0,       // player - N/A
		1.0f / 4,   // debug
		1.0f / 2,    // spree
	},
	{ // spree data
		1.0f / 2500, // trigger chance
		1.0f / 2,  // debug chance
		14, 24,      // min/max spawn count
		250,         // cooldown
	},
	NULL, // no body tweaks besides the obvious
	&statsImpostor,
	false, false, // debug variant/spree
};

const struct bvvariantxyzscales scalesSlenderman = {
	0.7f, 0.7f, // body scale x, z
	1.43f, 1.43f, // beyond pelvis scale mult x, z 
	 true, { -1.0f, 0.6f,  1.4f,  1.4f, -1.0f, 1.4f,  1.2f, 1.4f,  1.2f, 1.1f, -1.0f, -1.0f, 1.1f, -1.0f, -1.0f}, // x
	 true, { 0.8f, -1.0f, 0.8f, 0.8f,  -1.0f,   0.9f,   0.8f,  0.9f,  0.8f,  0.6f,  1.0f, -1.0f,  0.6f,  1.0f, -1.0f}, // y
	 true, { -1.0f, 0.7f,  0.6f, 0.6f, -1.0f,  0.9f,   0.8f,  0.9f,  0.8f,  0.5f,  1.0f, -1.0f,  0.5f,  1.0f, -1.0f}, // z
//  use,   neck, waist, lshdr, rshdr,  ????, rwrst, rhand, lwrst, lhand, rknee, rankl, rfoot, lknee, lankl, lfoot
};
const struct bvvariantbodydata bodySlenderman = {
	1.65f,    // body
	0.8f,    // head
	1.1f,    // shoulder
	-1.0f, // camera height (player)
	0.2f,    // voice pitch
	&scalesSlenderman,  // address of xyz scales
};
const struct bvvariantstats statsSlenderman = {
	1.4f,   // movespeedmult
	1.4f,    // animspeedmult
	1.0f / 2.25f, // damagetakenmult (=2.25x health)
	 1.5f,    // bluntdamagemult
	-1.0f,    // disarmdamage (default=0)
	-1.0f,    // meleerangemult
};
const struct bvvariant variantSlenderman = {
	BVFLAG_SLENDERMAN, BVINDEX_SLENDERMAN,
	{ // spawn chances
		1.0f / 125,  // bot
		0, // player - N/A
		1.0f / 2,   // debug
		1.0f / 2, // spree
	},
	{ // spree data (N/A)
		1.0f / 1500, // trigger chance
		1.0f / 10,   // debug chance
		12, 24,      // min/max spawn count
		150,         // cooldown
	},
	&bodySlenderman,
	&statsSlenderman,
	false, false, // debug variant/spree
};

const struct bvvariantstats statsSunglasses = {
	-1.0f, // movespeedmult
	-1.0f, // animspeedmult
	 1.0f / 1.25f, // damagetakenmult -- =1.25x health - doesn't apply if spree-spawned or if Impostor health bonus already applied (bvShouldApplyVariantDamageTakenMult)
	-1.0f, // bluntdamagemult
	-1.0f, // disarmdamage (default=0)
	-1.0f, // meleerangemult
};
const struct bvvariant variantSunglasses = {
	BVFLAG_SUNGLASSES, BVINDEX_SUNGLASSES,
	{ // spawn chances
		1.0f / 100, // bot
		1.0f / 20,  // player
		1.0f / 4, // debug
		1.0f,  // spree - all
	},
	{ // spree data
		1.0f / 1250, // trigger chance
		1.0f / 100,  // debug chance
		16, 36,      // min/max spawn count
		120,         // cooldown
	},
	NULL, // no body tweaks besides the sunglasses
	&statsSunglasses,
	false, false, // debug variant/spree
};

const struct bvvariantstats statsExplosive = {
	 1.05f, // movespeedmult
	 1.05f, // animspeedmult
	-1.0f, // damagetakenmult
	-1.0f, // bluntdamagemult
	-1.0f, // disarmdamage (default=0)
	-1.0f, // meleerangemult
};
const struct bvvariant variantExplosive = {
	BVFLAG_EXPLOSIVE, BVINDEX_EXPLOSIVE,
	{ // spawn chances
		1.0f / 110, // bot
		0,  // player - N/A
		1.0f / 15, // debug
		3.0f / 4,  // spree
	},
	{ // spree data
		1.0f / 1000, // trigger chance
		1.0f / 10,   // debug chance
		16, 36,      // min/max spawn count
		150,         // cooldown
	},
	NULL, // no body tweaks
	&statsExplosive,
	false, false, // debug variant/spree
};

const struct bvvariantstats statsGunfetti = {
	 0.75f, // movespeedmult
	 0.75f, // animspeedmult
	 1.0f / 1.25f, // damagetakenmult -- =1.25x health
	-1.0f, // bluntdamagemult
	-1.0f, // disarmdamage (default=0)
	-1.0f, // meleerangemult
};
const struct bvvariant variantGunfetti = {
	BVFLAG_GUNFETTI, BVINDEX_GUNFETTI,
	{ // spawn chances
		1.0f / 650,  // bot
		0,   // player - N/A
		01.0f / 8, // debug
		0, // spree  - N/A
	},
	{ // spree data
		0, // trigger chance - disabled
		0,   // debug chance
		10, 16, // min/max spawn count
		250,    // cooldown
	},
	NULL, // no body tweaks
	&statsGunfetti,
	false, false, // debug variant/spree
};

const struct bvvariantxyzscales scalesSBD = {
	-1.0f, -1.0f, // body scale x, z
	-1.0f, -1.0f, // beyond pelvis scale mult x, z
	false, {0}, // x
	 true, {0.55f, 0.55f, 0.55f, 0.55f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 0.55f, -1.0f, -1.0f, 0.55f, -1.0f, -1.0f}, // y
	 true, {0.55f, 0.55f, 0.55f, 0.55f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 0.55f, -1.0f, -1.0f, 0.55f, -1.0f, -1.0f}, // z
//  use,   neck, waist, lshdr, rshdr,  ????, rwrst, rhand, lwrst, lhand, rknee, rankl, rfoot, lknee, lankl, lfoot
};
const struct bvvariantbodydata bodySBD = {
	-1.0f, // body
	-1.0f, // head
	-1.0f, // shoulder
	-1.0f, // camera height (player)
	-1.0f, // voice pitch
	&scalesSBD, // address of xyz scales
};
const struct bvvariantstats statsSBD = {
	 1.1f, // movespeedmult
	 1.1f, // animspeedmult
	-1.0f, // damagetakenmult
	-1.0f, // bluntdamagemult
	-1.0f, // disarmdamage (default=0)
	-1.0f, // meleerangemult
};
const struct bvvariant variantSuperBattleDroid = {
	BVFLAG_SBD, BVINDEX_SBD,
	{ // spawn chances
		1.0f / 1250,  // bot
		0,      // player - N/A
		1.0f / 20, // debug
		3.0f / 4, // spree
	},
	{ // spree data
		1.0f / 2500, // trigger chance
		1.0f / 100,  // debug chance
		16, 36,      // min/max spawn count
		250,         // cooldown
	},
	&bodySBD,
	&statsSBD,
	false, false, // debug variant/spree
};

const struct bvvariant* gc_BvVariants[BOTVARIETY_VARIANT_COUNT] = {
	&variantMini,
	&variantWumbo,
	&variantImpostor,
	&variantSlenderman,
	&variantSunglasses,
	&variantExplosive,
	&variantGunfetti,
	&variantSuperBattleDroid,
};

/* Variant template

const struct bvvariantbodydata bodyX = {
	-1.0f, // body
	-1.0f, // head
	-1.0f, // shoulder
	-1.0f, // camera height (player)
	-1.0f, // voice pitch
	NULL, // address of xyz scales
};

const struct bvvariantstats statsX = {
	-1.0f, // movespeedmult
	-1.0f, // animspeedmult
	-1.0f, // damagetakenmult
	-1.0f, // bluntdamagemult
	-1.0f, // disarmdamage (default=0)
	-1.0f, // meleerangemult
};

const struct bvvariant variantX = {
	BVFLAG_X, BVINDEX_X,
	{ // spawn chances
		1.0f / 100,  // bot
		1.0f / 10,   // player
		1.0f / 3, // debug
		1.0f / 3, // spree
	},
	{ // spree data
		1.0f / 500, // trigger chance
		1.0f / 10,  // debug chance
		16, 36,     // min/max spawn count
		150,        // cooldown
	},
	&bodyX,
	&statsX,
	false, // debug enabled
};

1D joint scales template:

struct bvvariantxyzscales scales_ = {
	-1.0f, -1.0f, // body scale x, z
	-1.0f, -1.0f, // beyond pelvis scale mult x, z
	false, {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f}, // x
	false, {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f}, // y
	false, {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f}, // z
	//use,   neck, waist, lshdr, rshdr,  ????, rwrst, rhand, lwrst, lhand, rknee, rankl, rfoot, lknee, lankl, lfoot
};

*/
