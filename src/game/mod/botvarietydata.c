#include <ultra64.h>
#include "constants.h"
#include "game/mod/botvariety.h"

bool g_BvDebug = false;
bool g_BvDebugSprees = false;

struct bvmatchdata g_BvMatch;

const struct bvvariant g_BvVariants[BOTVARIETY_VARIANT_COUNT] = {
	{   BOTVARIETY_FLAG_MINI, INDEX_MINI,
			{ // spawn chances
				0.05f,   // bot    - 1 in 20
				0.0333f, // player - 1 in 30
				0.25f,  // debug   - 1 in  4
				0.75f,  // spree   - 3 in  4
			},
			{ // spree data
				0.0025f, // trigger chance - 1 in 400
				0.1f,  // debug chance
				16,  // min spawn count
				36,  // max spawn count
			},
			{ // body mults
				0.605f,  // body
				1.65f,   // head
				1.3f,    // shoulder
				0.63375f,// camera height (player)
				1.18f,   // voice pitch
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
				0.25f,  // debug   - 1 in  4
				0.75f,  // spree   - 3 in  4
			},
			{ // spree data
				0.002f, // trigger chance - 1 in 500
				0.1f,  // debug chance 1/x
				12,  // min spawn count
				32,  // max spawn count
			},
			{ // scale mults
				1.5f,    // body
				0.8f,    // head
				1.4f,    // shoulder
				1.4517f, // camera height (player)
				0.8f,    // voice pitch
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
				0.00125f,  // bot  - 1 in 800
				0,       // player - N/A
				0.25f,   // debug  - 1 in 4
				0.5f,    // spree  - 1 in 2
			},
			{ // spree data
				0.0005f, // trigger chance - 1 in 2000 
				0.0f,   // debug chance 1/x
				8,   // min spawn count
				16,  // max spawn count
			},
			{ // body mults
				-1.0f, // body scale
				-1.0f, // head scale
				-1.0f, // shoulder scale
				-1.0f, // camera height (player)
				-1.0f, // voice pitch
			},
			{ // gameplay stats
				-1.0f, // movespeedmult
				-1.0f, // animspeedmult
				 0.667f, // damagetakenmult (=1.5x health)
				-1.0f, // bluntdamagemult
				-1.0f, // disarmdamage (default=0)
				-1.0f, // meleerangemult
			},
	},
	{BOTVARIETY_FLAG_SUNGLASSES, INDEX_SUNGLASSES,
			{ // spawn chances
				0.01f, // bot   - 1 in 100
				0.1f, // player - 1 in  10
				0.25f, // debug - 1 in   4
				1.0f,  // spree - all
			},
			{ // spree data
				0.002f, // trigger chance - 1 in 500
				0.1f,  // debug chance
				16,  // min spawn count
				36,  // max spawn count
			},
			{ // body mults
				-1.0f, // body scale
				-1.0f, // head scale
				-1.0f, // shoulder scale
				-1.0f, // camera height (player)
				-1.0f, // voice pitch
			},
			{ // gameplay stats
				-1.0f, // movespeedmult
				-1.0f, // animspeedmult
				 0.8f, // damagetakenmult -- =1.25x health 
				-1.0f, // bluntdamagemult
				-1.0f, // disarmdamage (default=0)
				-1.0f, // meleerangemult
			},
	},
};

/* Variant template
			{ // spawn chances
				0.01f,  // bot    - 1 in 100
				0.1f,   // player - 1 in  10
				0.333f, // debug  - 1 in   3
			},
			{ // body mults (neg to disable)
				-1.0f, // body
				-1.0f, // head
				-1.0f, // shoulder
				-1.0f, // camera height (player)
				-1.0f, // voice pitch
			},
			{ // gameplay stats (neg to disable)
				-1.0f, // movespeedmult
				-1.0f, // animspeedmult
				-1.0f, // damagetakenmult
				-1.0f, // bluntdamagemult
				-1.0f, // disarmdamage (default=0)
				-1.0f, // meleerangemult
			},

*/
