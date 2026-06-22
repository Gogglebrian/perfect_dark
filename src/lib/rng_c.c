#include <ultra64.h>
#include "constants.h"
#include "bss.h"
#include "lib/rng.h"
#include "data.h"
#include "types.h"

s32 g_BetterRng = true;

u64 g_RngSeed = 0xab8d9f7781280783;

/**
 * Applies xorshift64* rng to the given seed and returns a random number between 0 and 4294967295.
 */
u32 rngApplyXorshift64Star(u64* seed) {
	u64 x = *seed;

	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;

	*seed = x;

	return (u32)((x * 0x2545F4914F6CDD1DULL) >> 32);
}

/**
 * Generate a random number between 0 and 4294967295.
 */
u32 rngRandom(void)
{
	if (g_BetterRng) { // xorshift64*
		return rngApplyXorshift64Star(&g_RngSeed);
	}
	else { // original
		g_RngSeed = ((g_RngSeed << 63) >> 31 | (g_RngSeed << 31) >> 32) ^ (g_RngSeed << 44) >> 32;
		g_RngSeed = ((g_RngSeed >> 20) & 0xfff) ^ g_RngSeed;
		return g_RngSeed;
	}
}

/**
 * Set the given seed as the RNG seed. Add 1 to make sure it isn't 0.
 */
void rngSetSeed(u64 seed)
{
	g_RngSeed = seed + 1;
}

/**
 * Rotate the given seed using the original algorithm. This func is rarely used, but changing its implementation messes with checksums and invalidates older save data.
 *
 * Store the new 64-bit seed at the pointed address and return the same seed
 * cast as a u32.
 */
u32 rngRotateSeed(u64 *seed)
{
	*seed = ((*seed << 63) >> 31 | (*seed << 31) >> 32) ^ (*seed << 44) >> 32;
	*seed = ((*seed >> 20) & 0xfff) ^ *seed;
	return *seed;
}
