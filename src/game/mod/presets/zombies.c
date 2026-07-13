#include <ultra64.h>
#include "constants.h"
#include "game/mplayer/mplayer.h"
#include "bss.h"
#include "lib/str.h"

char zombiesname[] = "Zombies!";
u8 zombieheads[8] = { MPHEAD_GARETH, MPHEAD_BEAU1, MPHEAD_MARK2, MPHEAD_SILKE, MPHEAD_ROBERT, MPHEAD_MOTO,  MPHEAD_KEN,   MPHEAD_JOEL };

void mpSetUpConfigZombiesSimulants(struct mpconfigfull* config, struct mpsetup* setup) {
	u8 i, j;
	char number[2] = "0";
	struct mpconfigsim* thissimulant;

	// Customize bots
	for (i = 0; i < MAX_BOTS; i++) {
		thissimulant = &config->config.simulants[i];

		// Set team
		thissimulant->team = MPTEAM_6; // Green team

		// Set type and difficulty
		thissimulant->type = BOTTYPE_FIST;
		for (j = 0; j < MAX_PLAYERS; j++) {
			thissimulant->difficulties[j] = BOTDIFF_NORMAL;
		}

		// Set appearance
		thissimulant->mpheadnum = zombieheads[i];
		thissimulant->mpbodynum = MPBODY_DDSHOCK;

		// Set name
		char name[15] = "FistSim:";
		number[0] = '1' + i;
		strcat(name, number);
		strcpy(config->strings.aibotnames[i], name);
	}

	// enable all bots
	setup->chrslots |= 0x0ff0;
}

void mpSetUpConfigZombies(struct mpconfigfull* config) {
	struct mpsetup* setup = &config->config.setup;
	u8 i;

	// Set options
	setup->options |= MPOPTION_TEAMSENABLED | MPOPTION_NOAUTOAIM | MPOPTION_BOTVARIETY | MPOPTION_FRIENDLYFIRE | MPOPTION_AUTORANDOMWEAPON_END;
	setup->stagenum = STAGE_MP_RANDOM;
	setup->timelimit = 11;
	setup->scorelimit = 100;
	setup->teamscorelimit = 400;

	// Set up simulants
	mpSetUpConfigZombiesSimulants(config, setup);

	// Set player team
	for (i = 0; i < MAX_PLAYERS; i++) {
		g_PlayerConfigsArray[i].base.team = MPTEAM_4; // Cyan team
	}

	// Set setup name (only used when saving a customized copy of this setup)
	strcpy(setup->name, zombiesname);

	// Random weapon filters
	for (i = 0; i < ARRAYCOUNT(g_MpWeapons); i++) {
		mpApplyDefaultRandomFilter(i);
	}
}
