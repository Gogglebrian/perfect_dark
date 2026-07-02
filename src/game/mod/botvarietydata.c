#include <ultra64.h>
#include "constants.h"
#include "game/mod/botvariety.h"

const bool g_BvDebugAllVariants = false;
const bool g_BvDebugSprees = false;
const bool g_BvDebugAllowPlayerAbominations = false;

struct bvmatchdata g_BvMatch;
u8 g_BvSpreeCooldowns[BOTVARIETY_VARIANT_COUNT]; // persists across rounds

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
		0.05f,   // bot    - 1 in 20
		0.0333f, // player - 1 in 30
		0.25f,   // debug  - 1 in  4
		0.75f,   // spree  - 3 in  4
	},
	{ // spree data
		0.0025f, // trigger chance - 1 in 400
		0.01f,    // debug chance
		16,      // min spawn count
		36,      // max spawn count
	},
	&bodyMini,
	&statsMini,
	false, // debug enabled
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
	0.3077f, // damagetakenmult (=3.25x health)
	2.0f,    // bluntdamagemult
	1.0f,    // disarmdamage (default=0)
	2.0f,    // meleerangemult
};
const struct bvvariant variantWumbo = {
	BVFLAG_WUMBO, BVINDEX_WUMBO,
	{ // spawn chances
		0.0333f, // bot    - 1 in 30
		0.0333f, // player - 1 in 30
		0.25f,   // debug  - 1 in  4
		0.75f,   // spree  - 3 in  4
	},
	{ // spree data
		0.002f, // trigger chance - 1 in 500
		0.01f,   // debug chance
		12,     // min spawn count
		32,     // max spawn count
	},
	&bodyWumbo,
	&statsWumbo,
	false, // debug enabled
};

const struct bvvariantstats statsImpostor = {
	-1.0f, // movespeedmult
	-1.0f, // animspeedmult
	0.667f, // damagetakenmult (=1.5x health)
	-1.0f, // bluntdamagemult
	-1.0f, // disarmdamage (default=0)
	-1.0f, // meleerangemult
};
const struct bvvariant variantImpostor = {
	BVFLAG_IMPOSTOR, BVINDEX_IMPOSTOR,
	{ // spawn chances
		0.002f,  // bot  - 1 in 500
		0,       // player - N/A
		0.25f,   // debug  - 1 in 4
		0.5f,    // spree  - 1 in 2
	},
	{ // spree data
		0.000625f, // trigger chance - 1 in 1600
		0.01f,      // debug chance 1/x
		14,        // min spawn count
		24,        // max spawn count
	},
	NULL, // no body tweaks besides the obvious
	&statsImpostor,
	false, // debug enabled
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
	0.95f,   // movespeedmult
	0.7f,    // animspeedmult
	0.2f, // damagetakenmult (=5x health)
	4.0f,    // bluntdamagemult
	4.0f,    // disarmdamage (default=0)
	-1.0f,    // meleerangemult
};
const struct bvvariant variantSlenderman = {
	BVFLAG_SLENDERMAN, BVINDEX_SLENDERMAN,
	{ // spawn chances
		0.001f, // bot    - 1 in 1000
		0, // player - N/A
		0.1f,   // debug  - 1 in  4
		0.5f,   // spree  - 1 in  2
	},
	{ // spree data
		0.000625f, // trigger chance - 1 in 1600
		0.01f,   // debug chance
		12,     // min spawn count
		32,     // max spawn count
	},
	&bodySlenderman,
	&statsSlenderman,
	false, // debug enabled
};

const struct bvvariantstats statsSunglasses = {
	-1.0f, // movespeedmult
	-1.0f, // animspeedmult
		0.8f, // damagetakenmult -- =1.25x health
	-1.0f, // bluntdamagemult
	-1.0f, // disarmdamage (default=0)
	-1.0f, // meleerangemult
};
const struct bvvariant variantSunglasses = {
	BVFLAG_SUNGLASSES, BVINDEX_SUNGLASSES,
	{ // spawn chances
		0.01f, // bot   - 1 in 100
		0.1f,  // player - 1 in  10
		0.25f, // debug - 1 in   4
		1.0f,  // spree - all
	},
	{ // spree data
		0.002f, // trigger chance - 1 in 500
		0.01f,   // debug chance
		16,     // min spawn count
		36,     // max spawn count
	},
	NULL, // no body tweaks besides the sunglasses
	&statsSunglasses,
	false, // debug enabled
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
		0.01f, // bot   - 1 in 100
		0,  // player - N/A
		0.05f, // debug - 1 in 20
		0.75f,  // spree - 3 in 4
	},
	{ // spree data
		0.00143f, // trigger chance - 1 in 700
		0.1f,   // debug chance
		16,      // min spawn count
		36,      // max spawn count
	},
	NULL, // no body tweaks
	&statsExplosive,
	false, // debug enabled
};

const struct bvvariantstats statsGunfetti = {
	 0.75f, // movespeedmult
	 0.75f, // animspeedmult
	 0.8f, // damagetakenmult -- =1.25x health
	-1.0f, // bluntdamagemult
	-1.0f, // disarmdamage (default=0)
	-1.0f, // meleerangemult
};
const struct bvvariant variantGunfetti = {
	BVFLAG_GUNFETTI, BVINDEX_GUNFETTI,
	{ // spawn chances
		0.002f,  // bot    - 1 in 500
		0,   // player - N/A
		0.5f, // debug  - 1 in   2
		0.333f, // spree  - 1 in   3
	},
	{ // spree data
		0, // trigger chance - disabled
		0,   // debug chance
		12,     // min spawn count
		32,     // max spawn count
	},
	NULL, // no body tweaks
	&statsGunfetti,
	false, // debug enabled
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
		0.0005f,  // bot    - 1 in 2000
		0,      // player - N/A
		0.05f, // debug  - 1 in  20
		0.75f, // spree  - 3 in   4
	},
	{ // spree data
		0.0004f, // trigger chance - 1 in 2500
		0.01f,   // debug chance - 1 in 100
		16,     // min spawn count
		36,     // max spawn count
	},
	&bodySBD,
	&statsSBD,
	false, // debug enabled
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
		0.01f,  // bot    - 1 in 100
		0.1f,   // player - 1 in  10
		0.333f, // debug  - 1 in   3
		0.333f, // spree  - 1 in   3
	},
	{ // spree data
		0.002f, // trigger chance - 1 in 500
		0.1f,   // debug chance
		16,     // min spawn count
		36,     // max spawn count
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
