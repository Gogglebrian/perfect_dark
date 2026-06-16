#include <ultra64.h>
#include "constants.h"
#include "game/mod/botvariety.h"

bool g_BvDebug = true;
bool g_BvDebugSprees = false;

struct bvmatchdata g_BvMatch;

struct bvvariantxyzscales jointspost_superbattledroid = {
	-1.0f, -1.0f, -1.0f, // body x, y, z
	false, {0}, // x
	 true, {0.55f, 0.55f, 0.55f, 0.55f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 0.55f, -1.0f, -1.0f, 0.55f, -1.0f, -1.0f}, // y
	 true, {0.55f, 0.55f, 0.55f, 0.55f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 0.55f, -1.0f, -1.0f, 0.55f, -1.0f, -1.0f}, // z
//  use,   neck, waist, lshdr, rshdr,  ????, rwrst, rhand, lwrst, lhand, rknee, rankl, rfoot, lknee, lankl, lfoot
};

struct bvvariantxyzscales jointspost_paperflat = {
	-1.0f, -1.0f, -1.0f, // body x, y, z
	false, {0}, // x
	false, {0}, // y
	 true, {-1.0f,  0.1f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f}, // z
//  use,   neck, waist, lshdr, rshdr,  ????, rwrst, rhand, lwrst, lhand, rknee, rankl, rfoot, lknee, lankl, lfoot
};

// 1D joint scales template
// false, {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f}, // x

const struct bvvariant gc_BvVariants[BOTVARIETY_VARIANT_COUNT] = {
	{   BOTVARIETY_FLAG_MINI, INDEX_MINI,
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
			{ // body mults
				0.605f,  // body
				1.65f,   // head
				1.3f,    // shoulder
				0.63375f,// camera height (player)
				1.18f,   // voice pitch
				NULL,    // address of xyz scales
				NULL,    // address of xyz scales (global transform applied after animations)
			},
			{ // gameplay stats
				1.1f,    // movespeedmult
				1.5f,    // animspeedmult
				-1.0f,   // damagetakenmult
				0.9f,    // bluntdamagemult
				0,       // disarmdamage (default=0)
				-1.0f,   // meleerangemult
			},
	}, {BOTVARIETY_FLAG_WUMBO, INDEX_WUMBO,
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
			{ // scale mults
				1.5f,    // body
				0.8f,    // head
				1.4f,    // shoulder
				1.4517f, // camera height (player)
				0.8f,    // voice pitch
				NULL,    // address of xyz scales
				NULL,    // address of xyz scales (global transform applied after joint is positioned)
			},
			{ // stats (neg to disable)
				0.95f,   // movespeedmult
				0.9f,    // animspeedmult
				0.3077f, // damagetakenmult (=3.25x health)
				2.0f,    // bluntdamagemult
				1.0f,    // disarmdamage (default=0)
				2.0f,    // meleerangemult
			},
	}, {BOTVARIETY_FLAG_IMPOSTOR, INDEX_IMPOSTOR,
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
			{ // body mults
				-1.0f, // body scale
				-1.0f, // head scale
				-1.0f, // shoulder scale
				-1.0f, // camera height (player)
				-1.0f, // voice pitch
				NULL, // address of xyz scales
				NULL, // address of xyz scales (global transform applied after joint is positioned)
			},
			{ // gameplay stats
				-1.0f, // movespeedmult
				-1.0f, // animspeedmult
				 0.667f, // damagetakenmult (=1.5x health)
				-1.0f, // bluntdamagemult
				-1.0f, // disarmdamage (default=0)
				-1.0f, // meleerangemult
			},
	}, {BOTVARIETY_FLAG_SUNGLASSES, INDEX_SUNGLASSES,
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
			{ // body mults
				-1.0f, // body scale
				-1.0f, // head scale
				-1.0f, // shoulder scale
				-1.0f, // camera height (player)
				-1.0f, // voice pitch
				NULL, // address of xyz scales
				NULL, // address of xyz scales (global transform applied after joint is positioned)
			},
			{ // gameplay stats
				-1.0f, // movespeedmult
				-1.0f, // animspeedmult
				 0.8f, // damagetakenmult -- =1.25x health 
				-1.0f, // bluntdamagemult
				-1.0f, // disarmdamage (default=0)
				-1.0f, // meleerangemult
			},
	}, {BOTVARIETY_FLAG_EXPLOSIVE, INDEX_EXPLOSIVE,
			{ // spawn chances
				0.005f, // bot   - 1 in 200
				0,  // player - N/A
				0.33f, // debug - 1 in 10
				0.75f,  // spree - all
			},
			{ // spree data
				0.00143f, // trigger chance - 1 in 700
				0.1f,   // debug chance
				20,     // min spawn count
				45,     // max spawn count
			},
			{ // body mults
				-1.0f, // body scale
				-1.0f, // head scale
				-1.0f, // shoulder scale
				-1.0f, // camera height (player)
				-1.0f, // voice pitch
				NULL, // address of xyz scales
				NULL, // address of xyz scales (global transform applied after joint is positioned)
			},
			{ // gameplay stats
				 1.05f, // movespeedmult
				 1.05f, // animspeedmult
				-1.0f, // damagetakenmult -- =1.25x health 
				-1.0f, // bluntdamagemult
				-1.0f, // disarmdamage (default=0)
				-1.0f, // meleerangemult
			},
	},{BOTVARIETY_FLAG_SUPERBATTLEDROID, INDEX_SUPERBATTLEDROID,
			{ // spawn chances
				0.0005f,  // bot    - 1 in 2000
				0,      // player - N/A
				0.01f, // debug  - 1 in   100
			},
			{ // spree data
				0.0004f, // trigger chance - 1 in 2500
				0.01f,   // debug chance
				16,     // min spawn count
				36,     // max spawn count
			},
			{ // body mults (neg to disable)
				-1.0f, // body
				-1.0f, // head
				-1.0f, // shoulder
				-1.0f, // camera height (player)
				-1.0f, // voice pitch
			  NULL, // address of xyz scales
				&jointspost_superbattledroid, // address of xyz scales (global transform applied after joint is positioned)
			},
			{ // gameplay stats (neg to disable)
				 1.1f, // movespeedmult
				 1.1f, // animspeedmult
				-1.0f, // damagetakenmult
				-1.0f, // bluntdamagemult
				-1.0f, // disarmdamage (default=0)
				-1.0f, // meleerangemult
			},
	}, {BOTVARIETY_FLAG_PAPERFLAT, INDEX_PAPERFLAT,
			{ // spawn chances
				0.0005f, // bot    - 1 in 2000
				0,      // player - N/A
				0.2f, // debug  - 1 in   5
			},
			{ // spree data
				0.0004f, // trigger chance - 1 in 2500
				0.1f,   // debug chance
				16,     // min spawn count
				36,     // max spawn count
			},
			{ // body mults (neg to disable)
				-1.0f, // body
				-1.0f, // head
				-1.0f, // shoulder
				-1.0f, // camera height (player)
				-1.0f, // voice pitch
			  NULL, // address of xyz scales
				&jointspost_paperflat, // address of xyz scales (global transform applied after joint is positioned)
			},
			{ // gameplay stats (neg to disable)
				-1.0f, // movespeedmult
				-1.0f, // animspeedmult
				-1.0f, // damagetakenmult
				-1.0f, // bluntdamagemult
				-1.0f, // disarmdamage (default=0)
				-1.0f, // meleerangemult
			},
	},
};

/* Variant template
		{BOTVARIETY_FLAG_, INDEX_,
			{ // spawn chances
				0.01f,  // bot    - 1 in 100
				0.1f,   // player - 1 in  10
				0.333f, // debug  - 1 in   3
			},
			{ // spree data
				0.002f, // trigger chance - 1 in 500
				0.1f,   // debug chance
				16,     // min spawn count
				36,     // max spawn count
			},
			{ // body mults (neg to disable)
				-1.0f, // body
				-1.0f, // head
				-1.0f, // shoulder
				-1.0f, // camera height (player)
				-1.0f, // voice pitch
				NULL, // address of xyz scales
				NULL, // address of xyz scales (global transform applied after joint is positioned)
			},
			{ // gameplay stats (neg to disable)
				-1.0f, // movespeedmult
				-1.0f, // animspeedmult
				-1.0f, // damagetakenmult
				-1.0f, // bluntdamagemult
				-1.0f, // disarmdamage (default=0)
				-1.0f, // meleerangemult
			},
		}

*/
