#include "../bounty.h"
#include "../play.h"
#include "../save.h"
#include "../lib/kbconf.h"
#include "../lib/kbres.h"
#include "../lib/kbauto.h"
#include "../lib/kbstd.h"

#include "../env.h"
struct KBconfig KBconf;
struct KBenv *sys;
int filemode = 0;

KBgame* load_land(const char *filename) {

	KBgame *game;
	FILE *f;
	int map_size, n;

	game = malloc(sizeof(KBgame));
	if (game == NULL) return NULL;

	map_size = ( MAX_CONTINENTS * LEVEL_W * LEVEL_H );
	memset(game->map, 0, sizeof(char) * map_size);

	game->map[0][0][0] = 0xFF;
	game->map[1][0][0] = 0xFF;
	game->map[2][0][0] = 0xFF;
	game->map[3][0][0] = 0xFF;

	KB_strncpy(game->savefile, filename, 64);

	f = fopen(filename, "rb");
	if (f) {

		n = fread(game->map, sizeof(char), map_size, f);
		if (n != map_size) {
			printf("Read %d bytes of %d expected\n", n, map_size);
		}
		fclose(f);

	} else {
		printf("Unable to OPEN file %s\n", filename);
	}

	return game;
}

void interactive_edit(KBgame *game) {

}

void interactive_main(KBgame *game) {

	/* Read default config file */
	wipe_config(&KBconf);
	read_env_config(&KBconf);
	//temp hack to always read local file
	KB_strcpy(KBconf.config_file, "../../openkb.ini");
	if ( read_file_config(&KBconf, KBconf.config_file) )
	{
		KB_errlog("[config] Unable to read config file '%s'\n", KBconf.config_file); 
		return;
	}

	/* Output final config to stdout */
	KB_stdlog("\n");
	report_config(&KBconf);
	KB_stdlog("\n");

	KBconfig *conf = &KBconf; //shortcut

	/* Hack? Pretend to use normal2x */
	conf->filter = 1;

	/* Start new environment (game window) */
	sys = KB_startENV(conf);

	/* That's clearly wrong: */
	conf->filter = 0;

	/* Must be successfull to continue */
	if (!sys) {
		return;
	}

	/* Module auto-discovery */
	if (conf->autodiscover)
		discover_modules(conf->data_dir, conf);

	/* --- ! ! ! --- */
	conf->module = 0;

	/* No module! (Unlikely...) */
	if (conf->num_modules == 0) {
		KB_errlog("No modules found.\n");
		return;
	}

	/* Load and use module font */
	sys->font = KB_LoadIMG8(GR_FONT, 0);
	infont(sys->font);

	/* --- X X X --- */
	KB_stdlog("=====================================================\n");
	/* Run editor loop */
	interactive_edit(game);

	/* --- @ @ @ --- */
	KB_stopENV(sys);
}

void stdout_game(KBgame *game) {
	int cont, k, j, i;

	printf("Difficulty: %d\n", game->difficulty);
	printf("Scepter: %d, X=%d, Y=%d\n", game->scepter_continent, game->scepter_x, game->scepter_y);

	printf("Player: %s the %s\n", game->name, "Player");
	printf("Player: Level %d\n", game->rank);
	printf("Player: Mount - %d\n", game->mount);
	
	printf("Player: Location: Continent %d, X=%d, Y=%d\n", game->continent, game->x, game->y);
	printf("Player: Visited: Continent %d, X=%d, Y=%d\n", game->continent, game->last_x, game->last_y);

	printf("Boat: Exists: %s\n", game->boat != 0xFF ? "YES" : "NO");
	printf("Boat: Location: Continent %d, X=%d, Y=%d\n", game->boat, game->boat_x, game->boat_y);
	
	printf("Player: Siege Weapons: %s\n", game->siege_weapons ? "YES" : "NO");
	printf("Player: Knows Magic: %s\n", game->knows_magic ? "YES" : "NO");
	printf("Player: Owns Boat: %s\n", game->boat != 0xFF ? "YES" : "NO");

	printf("Artifcats: ");
	for (i = 0; i < MAX_ARTIFACTS; i++) {
		printf("[%c] ", game->artifact_found[i] ? 'X' : ' ');
	}
	printf("\n");
	
	printf("Villains: ");
	for (i = 0; i < MAX_VILLAINS; i++) {
		printf("[%c] ", game->villain_caught[i] ? 'X' : ' ');
	}
	printf("\n");

	printf("NavMaps: ");
	for (i = 0; i < MAX_CONTINENTS; i++) {
		printf("[%c] ", game->continent_found[i] ? 'X' : ' ');
	}
	for (i = 0; i < MAX_CONTINENTS; i++) {
		printf("%d,%d ", game->map_coords[i][0], game->map_coords[i][1]);
	}
	printf("\n");
	
	printf("MapOrbs: ");
	for (i = 0; i < MAX_CONTINENTS; i++) {
		printf("[%c] ", game->orb_found[i] ? 'X' : ' ');
	}
	for (i = 0; i < MAX_CONTINENTS; i++) {
		printf("%d,%d ", game->orb_coords[i][0], game->orb_coords[i][1]);
	}
	printf("\n");

	printf("Telecaves: ");
	for (i = 0; i < MAX_CONTINENTS; i++) {
		printf("[%d] ", i);
		printf("%d,%d->", game->teleport_coords[i][0][0], game->teleport_coords[i][0][1]);
		printf("%d,%d  ", game->teleport_coords[i][1][0], game->teleport_coords[i][1][1]);
	}
	printf("\n");

	for (cont = 0; cont < MAX_CONTINENTS; cont++) {
		printf("Continent %d: \n", cont);
		for (j = LEVEL_H - 1; j >= 0; j--) {
			printf("Map%d,%02d:", cont, j);
			for (i = 0; i < LEVEL_W; i++) {
				byte m = game->map[cont][j][i]; 
				char c = '?';
				
				if (IS_WATER(m)) c = '~';
				if (IS_GRASS(m)) c = '.';
				if (IS_DESERT(m)) c = ',';
				if (IS_ROCK(m)) c = '#';
				if (IS_CASTLE(m)) c = '^';
				if (IS_TREE(m)) c = '*';
				if (IS_MAPOBJECT(m)) c = '!';
				if (IS_INTERACTIVE(m)) c = 'X';

				if (game->scepter_continent == cont && 
					i == game->scepter_x &&
					j == game->scepter_y) c = 'O';		
				if (game->boat == cont && 
					i == game->boat_x &&
					j == game->boat_y) c = 'B';

				printf("%c", c);
			}
			printf("\n");
		}
	}

	for (cont = 0; cont < MAX_CONTINENTS; cont++) {
		printf("Dwellings %d: ", cont);
		for (i = 0; i < MAX_DWELLINGS; i++) {
			printf("%02x ", 
				game->dwelling_troop[cont][i] // type
			);
		}
		printf("| ");
		for (i = 0; i < MAX_DWELLINGS; i++) {
			printf("%d,%d ", 
				game->dwelling_coords[cont][i][0],//x
				game->dwelling_coords[cont][i][1]//y
			);
		}
		printf("\n");
	}

//#define TEST_SCEPTER_TILES 0x58 //AX at 0788:0A8B
#ifdef TEST_SCEPTER_TILES
	for (cont = 0; cont < MAX_CONTINENTS; cont++) {
		printf("Puzzle%d: ", cont);
		int gtiles = TEST_SCEPTER_TILES;
		for (j = 0; j < LEVEL_H; j++) {
			for (i = 0; i < LEVEL_W; i++) {
				if (!(game->map[cont][j][i] & 0x7F)) {//no "tiling" bits sets
					gtiles--;
					if (gtiles == 0) {
						printf("X=%d, Y=%d\n", i, j);
						break;
					}
				}

			}
			if (gtiles == 0) break;
		}
		if (gtiles != 0) {
			printf("Not found\n");
		}
	}
#endif

	char *names[] = { 
		"Mury","Hack","Ammi","Baro","Drea","Cane","Mora","Barr",
		"Barg","Rina","Ragf","Mahk","Auri","Czar","Magu","Urth","Arec" };

	char *troops[] = {
		"Peasants","Sprites","Milita","Wolfs","Skeletons","Zombies","Gnomes","Orcs",
		"Archers","Elves","Pikemen","Nomads","Dwarves","Ghosts","Knights","Ogres", 
		"Barbarians","Trolls","Cavalery","Druids","Arcmages","Vampires","Giants",
		"Demons","Dragons" };

	for (j = 0; j < MAX_VILLAINS; j++) {
		printf("Villain%d: %s [%s]\n", j, names[j], game->villain_caught[j] ? "X" : " ");
		if (game->villain_caught[j]) continue;
		for (k = 0; k < MAX_CASTLES; k++) {
			if ((game->castle_owner[k] & 0x7F) == j) {
				/* Army */
				for (i = 0; i < 5; i++) {
					if (game->castle_numbers[k][i] == 0) break;
					printf("Villain%d: Castle%2d: Troop%d: %02d x %s\n", j, k, i, game->castle_numbers[k][i], troops[game->castle_troops[k][i]]);
				}
				break;
			}
		}
	}
	
	for (k = 0; k < MAX_CASTLES; k++) {
		//if ((game->castle_owner[k] & 0x7F) == j) {
			/* Army */
			for (i = 0; i < 5; i++) {
				if (game->castle_numbers[k][i] == 0) break;
				printf("Monster Castle%2d: Troop%d: %02d x %s\n", k, i, game->castle_numbers[k][i], troops[game->castle_troops[k][i]]);
			}
		//}
	}
	
}

int main(int argc, char* argv[]) {

	int be_interactive = 1;

	KBgame *game;

	if (argc < 3) {
	
		printf("Usage: ./kbmaped [OPTION] SAVEFILE\n");
		printf("OPTIONS are:\n");
		printf("\t--dump\t Print textual representation of SAVEFILE and exit\n");
		printf("\t--cheat\t Fill target SAVEFILE with dragons\n");
		printf("\t--land\t Treat SAVEFILE as land.org\n");
	
		return -1;
	}

	if (!strcasecmp(argv[1], "--land")) {

		game = load_land(argv[2]);

	}
	else

		game = KB_loadDAT(argv[2]);
	
	if (game == NULL) {
	
		printf("Unable to load game %s\n", argv[2]);
		
		return -2;
	}

	if (!strcasecmp(argv[1], "--dump")) {

		be_interactive = 0;
		printf("DUMPING\n");
		stdout_game(game);
		
	}
	
	if (be_interactive) {

		interactive_main(game);

	}
	
	free(game);

	return 0;
}
