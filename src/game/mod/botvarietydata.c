#include <ultra64.h>
#include "constants.h"
#include "game/mod/botvariety.h"

bool g_BvDebug = true;

struct bvmatchdata g_BvMatch;

const struct bvvariant g_BvVariants[4] = {
	{   BOTVARIETY_FLAG_MINI,
			{ // spawn chances
				0.05f,   // bot    - 1 in 20
				0.0333f, // player - 1 in 30
				0.333f,  // debug  - 1 in  3
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
	}, {BOTVARIETY_FLAG_WUMBO,
			{ // spawn chances
				0.0333f, // bot    - 1 in 30
				0.0333f, // player - 1 in 30
				0.333f,  // debug  - 1 in  3
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
				0.2857f, // damagetakenmult (= 3.5* health)
				2.0f,    // bluntdamagemult
				1.0f,    // disarmdamage (default=0)
				2.0f,    // meleerangemult
			},
	}, {BOTVARIETY_FLAG_IMPOSTOR,
			{ // spawn chances
				0.0333f, // bot    - 1 in 30
				0.0333f, // player - 1 in 30
				0.333f,  // debug  - 1 in  3
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
				-1.0f, // damagetakenmult
				-1.0f, // bluntdamagemult
				-1.0f, // disarmdamage (default=0)
				-1.0f, // meleerangemult
			},
	},
	{BOTVARIETY_FLAG_SUNGLASSES,
			{ // spawn chances
				0.01f, // bot   - 1 in 100
				0.1f, // player - 1 in  10
				0.333f, // debug- 1 in   3
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
				-1.0f, // damagetakenmult
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
