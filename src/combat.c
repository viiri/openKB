/*
 *  combat.c -- a stand-alone combat emulator
 *  Copyright (C) 2011 Vitaly Driedfruit
 *
 *  This file is part of openkb.
 *
 *  openkb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  openkb is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with openkb.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"
#ifndef HAVE_LIBSDL
	#error "Sorry, there's no reason to compile without HAVE_LIBSDL define..."
#endif

#include "env.h"

#include "bounty.h"


#include "lib/kbres.h"
#include "lib/kbconf.h"
#include "lib/kbstd.h"

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include "SDL.h"
#include "SDL_net.h" 

struct KBconfig KBconf;
struct KBenv *sys;
TCPsocket sd, rsd; /* Socket descriptor, remote socket descriptor */ 
IPaddress ip, *remoteIP; 

SDLNet_SocketSet set;

#define PKT_HELLO 0
#define PKT_CHAT 1
#define PKT_GOLD 2
#define PKT_ARMY 3
#define PKT_READY 4

#define PKT_TURN 5
#define PKT_WAIT 6
#define PKT_PASS 7
#define PKT_MOVE 8
#define PKT_FLY 9
#define PKT_SHOOT 10
#define PKT_CAST 11

#define PKT_MESSAGE 12
#define PKT_DAMAGE 13
#define PKT_DEATH 14
#define PKT_HEAL 15


#define FMT_CHAR "c"
#define FMT_BYTE "b"
#define FMT_WORD "w"
#define FMT_SWORD "W"
#define FMT_DWORD "d"
#define FMT_SINT "i"
#define FMT_STR "s"

#define LEVEL_W 8

typedef struct KBpacket {
	
	int id;
	char num_args;
	char format[16];

	int	(*callback)(const char *data);

} KBpacket;

int hello_callback(const char *data);
int chat_callback(const char *data);
int gold_callback(const char *data);
int army_callback(const char *data);
int ready_callback(const char *data);

int turn_callback(const char *data);
int wait_callback(const char *data);
int pass_callback(const char *data);
int move_callback(const char *data);


KBpacket packets[] = {

	{ PKT_HELLO, 2, FMT_STR FMT_BYTE, &hello_callback }, 
	{ PKT_CHAT, 1, FMT_STR, &chat_callback },
	{ PKT_GOLD, 1, FMT_WORD, &gold_callback },
	{ PKT_ARMY, 10, 
	FMT_BYTE FMT_BYTE FMT_BYTE FMT_BYTE FMT_BYTE
	FMT_WORD FMT_WORD FMT_WORD FMT_WORD FMT_WORD, &army_callback },
	{ PKT_READY, 1, FMT_BYTE, &ready_callback },
	{ PKT_TURN, 1, FMT_BYTE, &turn_callback },
	{ PKT_WAIT, 1, FMT_BYTE, &wait_callback },
	{ PKT_PASS, 1, FMT_BYTE, &pass_callback },
	{ PKT_MOVE, 3, FMT_BYTE FMT_BYTE FMT_BYTE, &move_callback }, 
};

int num_packets = sizeof(packets) / sizeof(KBpacket);

enum {

	Undefined,
	Server,
	Client,

} MyRole = Undefined;

typedef struct KBunit {
	
	int troop_id;
	int count;

	int frame;

	int health;
	int acted;

	int y;
	int x;
} KBunit;

typedef struct KBcombat {

	int e_army[5];
	int e_nums[5];

	KBunit units[2][5];

	int omap[20][20];
	int umap[20][20];
	
	int your_turn;

	int side;//indexes into
	int unit_id;//units array

} KBcombat;

KBcombat base = { 0};

typedef struct KBconsole {

	int num_messages;
	
	struct {
	
		int delay;
		char text[80];	

	} message[20];

	char your[80];
	int pos;
	int chatting;
	
} KBconsole;

KBconsole console;

int send_data(int id, ...);

void reset_console() {

	console.num_messages = 0;
	
	console.your[0] = '\0';
	console.pos = 0;
	
	console.chatting = 0;

}


void add_message(const char *msg) {

	int i;
	
	for (i = 0; i < 19; i++) {

		KB_strcpy(console.message[i].text, console.message[i+1].text);	

	}
	
	strcpy(console.message[19].text, msg);
	
	if (console.num_messages < 20) console.num_messages++;

}


void KB_iprint(char *fmt, ...) 
{ 
	char x[1024];
	va_list argptr;
	va_start(argptr, fmt);
	vfprintf(stdout, fmt, argptr);
	vsprintf(x, fmt, argptr);
	add_message(x);
	va_end(argptr);
}

void draw_console() {

	int i;
	SDL_Surface *screen = sys->screen;
	
	int y = screen->h - 21 * 8;
	
	incolor(0x000000, 0xFFFFFF);


	for (i = 0; i < 20; i++) {

	 inprint(screen, console.message[i].text, 0, y + i * 8);	

	}


	if (console.chatting) {
	
	incolor(0xFFFFFF, 0x000000);

	inprint(screen, console.your, 0, y + i * 8);
	inprint(screen, "|", console.pos * 8, y + i * 8);
	
	}
	
}


int KB_chat(SDL_keysym *kbd) {
	SDL_Event event;
	int eve = 0;
	
	int key = kbd->sym;

	if (key == SDLK_ESCAPE) {
		console.pos = 0;
		console.chatting = 0;
	}
	else if (key == SDLK_RETURN) {
		if (console.pos)
		{
			send_data(PKT_CHAT, console.your);
			char buffer[80];
			snprintf(buffer, 80, "[%s] %s", "Player1", console.your);
			add_message(buffer);
		}
		console.pos = 0;
		console.chatting = 0;
	}
	else if (key == SDLK_BACKSPACE) {
		if (console.pos > 0) {
			console.your[console.pos] = ' ';
			console.pos --;
		}
	}
	else {
	
		if ((kbd->mod & KMOD_SHIFT) && (key < 128)) { //shift -- uppercase!
			if (key >= SDLK_a && key <= SDLK_z) key -= 32;
			if (key >= SDLK_0 && key <= SDLK_9) key -= 16;
		}
	
		if (isascii(key) && console.pos < 79)
		console.your[console.pos++] = key;
	}

	if (key) {
		int i;
		for (i = 	console.pos; i < 80; i++) console.your[i] = 0;
	}
	
	return eve;
}

int KB_event() {
	SDL_Event event;
	int eve = 0;

	while (SDL_PollEvent(&event)) {

		if (event.type == SDL_QUIT) eve = 0xFF;

		if (event.type == SDL_KEYDOWN) {
			SDL_keysym *kbd = &event.key.keysym;
			{
				eve = kbd->sym;
				if (console.chatting == 1) eve = KB_chat(kbd);
				else if (kbd->sym == SDLK_t) {
					console.chatting = 1;
					console.pos = 0;
				} else if (kbd->sym == SDLK_ESCAPE) {
					eve = 0xFF;
				}
				break;
			}
			break;
		}
	}
	return eve;
}

SDL_Surface *troop_cache[MAX_TROOPS];

void alloc_troops() {
	int i;
	for (i = 0; i < MAX_TROOPS; i++) 
		troop_cache[i] = SDL_LoadRESOURCE(GR_TROOP, i, 1);
}

void free_troops() {
	int i;
	for (i = 0; i < MAX_TROOPS; i++) 
		SDL_FreeSurface(troop_cache[i]);
}

int dwmap[5][5] = {
	{ 2, 8, 10, 14, 18 },
	{ 0, 3, 0, 0, 0 },
	{ 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0 },
	{ 4, 5, 13, 21, 23 },
};

int opponent_ready = 0;
int youare_ready = 0;

int ready_callback(const char *data) {
	if (MyRole == Client) {
		int typ = READ_BYTE(data);
		if (typ == 0) youare_ready = 0;
		else if (typ == 1) youare_ready = 2;
		else opponent_ready = 1;
	} else {
		int typ = READ_BYTE(data);
		opponent_ready = 1;
	}
	return 0;
}

int prepare_for_combat() {

	int done = 0;
	
	alloc_troops();

	SDL_Surface *screen = sys->screen;	
	SDL_Surface *troop = troop_cache[0];
	
	SDL_Surface *dwell[5];
		dwell[0] = SDL_LoadRESOURCE(GR_TILE, 10, 0);
		dwell[1] = SDL_LoadRESOURCE(GR_TILE, 12, 0);
		dwell[2] = SDL_LoadRESOURCE(GR_TILE, 13, 0);
		dwell[3] = SDL_LoadRESOURCE(GR_TILE, 14, 0);
		dwell[4] = SDL_LoadRESOURCE(GR_TILE, 15, 0);

	int slot_id = 0;
	int dwelling_id = 0;
	int dwelling_slot = 0;
	int in_dwelling = 0;

	SDL_Rect src = { 0 };
	
	src.w = dwell[0]->w;
	src.h = dwell[0]->h;
	
	int army[5] = { 0 };
	int army_count[5] = { 0 };
	
	int last_ret;

	while (!done) {

		int key = 0;

		if (receive_data() < 0) {
			KB_errlog("Connection error\n");
			done = 1;
		}

		SDL_FillRect(screen, NULL, 0x4664B4);
		
		draw_console();
		
		SDL_Rect rect;
		
		rect.x = 0; rect.y = 0;
		rect.w = troop->w; rect.h = troop->h;

		int i;
		for (i = 0; i < 5; i++) {
			if (dwelling_id == i)
				put_color_pal(dwell[i], 0xFF00FF, 0x0000FF);
			else
				put_mono_pal(dwell[i]);
				
			rect.x += rect.w;

			SDL_BlitSurface(dwell[i], NULL, screen, &rect); 
		}

		inprint(screen, "PGUP/PGDN to change dwelling.", 200, 32);

		src.y = src.h;
		for (i = 0; i < 5; i++) {
		
			int tid = dwmap[dwelling_id][i];
			
			SDL_Rect dest;
			
			dest.x = 160; dest.y = 60 + i * src.h;
			dest.w = src.w; dest.h = src.h;

			if (i == dwelling_slot && in_dwelling)
				SDL_FillRect(screen, &dest, 0x00FF00);
			else			
				SDL_FillRect(screen, &dest, 0x4664B4);
		
			SDL_BlitSurface(troop_cache[tid], &src, screen, &dest); 
		
			dest.y += dest.h;
		}


		incolor(0,0x4664B4); 
		if (!in_dwelling)
		inprint(screen, "UP/DOWN - select slot   RIGHT - choose troop    ENTER - dismiss troop", 10, 250);
		else
		inprint(screen, "UP/DOWN - select troop   LEFT - choose slot    ENTER - accept troop", 10, 250);
		
inprint(screen, "T - to chat    F1 - when ready to play    ESC - quit game", 10, 260);
		
		incolor(0xFFFFFF,0x000000); 

inprint(screen, (opponent_ready ? "Your opponent IS REaDY" : "Your opponent is not ready"), 10, 280);
				
		
		src.y = 0;
		for (i = 0; i < 5; i++) {
		
			int tid = army[i];
			
			SDL_Rect dest;
			
			dest.x = 20; dest.y = 60 + i * src.h;
			dest.w = src.w; dest.h = src.h;
			
			if (i == slot_id)
				SDL_FillRect(screen, &dest, (in_dwelling  ? 0x00FF00 :  0xFF0000 ));
			else
				SDL_FillRect(screen, &dest, 0x4664B4);
		
			if (army_count[i])
			SDL_BlitSurface(troop_cache[tid], &src, screen, &dest);
			
			char buf[10];
			sprintf(buf, "% 2d", army_count[i]);
			inprint(screen, buf, dest.x + dest.w - 8, dest.y+ dest.h - 8); 
		
			dest.y += dest.h;
		}

		SDL_Flip(screen);		

		SDL_Delay(10);

		key = KB_event();

		if (key == 0xFF) done = 1;

		if (key == SDLK_BACKSPACE) dwmap[dwelling_id][dwelling_slot]--;
		if (key == SDLK_SPACE) dwmap[dwelling_id][dwelling_slot]++;

		if (youare_ready == 0) {

			if (key == SDLK_F1) {
				if (MyRole == Client) {
					youare_ready  = 1;
					send_data(PKT_ARMY,
						army[0],army[1],army[2],army[3],army[4],
						army_count[0],army_count[1],army_count[2],army_count[3],army_count[4]
						);
					send_data(PKT_READY, 1);
				} else {
					youare_ready = 2;
					send_data(PKT_READY, 2);
				}
			}

		
			if (key == SDLK_RETURN) {
				if (in_dwelling) {
					int nt = dwmap[dwelling_id][dwelling_slot];
					if (nt != army[slot_id]) 
						army_count[slot_id] = 1;
					else
					 	army_count[slot_id]++;
					army[slot_id] = dwmap[dwelling_id][dwelling_slot];
				} else {
					if (army[slot_id] == 0) in_dwelling = 1;
					else {
						if (army_count[slot_id] > 0) army_count[slot_id]--;
					}
				}
			}
			if (key == SDLK_PAGEUP) {
				if (dwelling_id > 0) {
					dwelling_id--;
				}
			}
			if (key == SDLK_PAGEDOWN) {
				if (dwelling_id < 4) {
					dwelling_id++;
				}
			}
			
			if (key == SDLK_RIGHT) {
				if (!in_dwelling)
					dwelling_slot = slot_id;
				in_dwelling = 1;
			}
			if (key == SDLK_LEFT) {
				in_dwelling = 0;
			}
			if (key == SDLK_DOWN) {
				if (last_ret == SDLK_RETURN) {
					if (slot_id < 4)
						slot_id++;
				}
				if (in_dwelling) {
					if (dwelling_slot < 4)
						dwelling_slot++;
				} else {
					if (slot_id < 4)
						slot_id++;
				}
			}
			if (key == SDLK_UP) {
				if (last_ret == SDLK_RETURN) {
					if (slot_id < 4)
						slot_id++;
				}
				if (in_dwelling) {
					if (dwelling_slot > 0)
						dwelling_slot--;
				} else {
					if (slot_id > 0)
						slot_id--;
				}
			
			}		
		}
		if (key) last_ret = key;		
		/* Really ready */
		if (opponent_ready && youare_ready == 2) done = 2;
	}
	
	/* Both are ready */
	if (done == 2) {
		if (MyRole == Server) {
			send_data(PKT_ARMY,
				army[0],army[1],army[2],army[3],army[4],
				army_count[0],army_count[1],army_count[2],army_count[3],army_count[4]
				);
		}
	}
	int i;
	for (i = 0; i < 5; i++)
	{
		base.units[0][i].troop_id = army[i];
		base.units[0][i].count = army_count[i];
		base.units[0][i].y = i;
		base.units[0][i].x = 0;
	}

	
//	printf("LAST DWELL: %d\n", dwmap[dwelling_id][dwelling_slot]);
}


int actually_move_unit(int whos, int id, int ox, int oy) {
	KBunit *u = &base.units[whos][id];

	int nx = u->x + ox;
	int ny = u->y + oy;

	if (whos == 0) {
	if (base.umap[ny][nx] >= 6
	&& base.umap[ny][nx] <= 10) {

		send_data(PKT_CHAT, "ATTACK!");
		add_message("ATTACK!");
		
		return 1;
	} }
	if (whos == 1) {
	if (base.umap[ny][nx] >= 1
	&& base.umap[ny][nx] <= 5) {

		send_data(PKT_CHAT, "ATTACK!");
		add_message("ATTACK!");
		
		return 1;
	} }

	base.umap[u->y][u->x] = -1;
	base.umap[ny][nx] = (whos * 5) + id + 1;
	
	u->x = nx;
	u->y = ny;
	return 0;
}

int move_callback(const char *data) {

	int id = data[0];
	int ox = data[1] - 3;
	int oy = data[2] - 3;

	if (MyRole == Server) {

		/* Move */
		if (!actually_move_unit(1, id, -ox, oy))
			/* Confirm */
			send_data(PKT_MOVE, id, ox + 3, oy + 3);

	} else {

		int whos = 0;
		if (id > 4) { 
			id -= 5; whos = 1; ox *= -1; 
		}

		actually_move_unit(whos, id, ox, oy);
	
	}
}
void move_unit(int id, int ox, int oy) {
	KBunit *u = &base.units[0][id];

	if (u->x + ox < 0 || u->x + ox > LEVEL_W - 1
	|| u->y + oy < 0 || u->y + oy > 4) return;//screen border

	if (base.omap[u->y + oy][u->x + ox]) return;//obstacle

	if (base.umap[u->y + oy][u->x + ox] >= 1
	&& base.umap[u->y + oy][u->x + ox] <= 5) return;//own troop
  

	if (MyRole == Server) {
		if (!actually_move_unit(0, id, ox, oy))
			send_data(PKT_MOVE, id + 5, ox + 3, oy + 3);
	}
	else
		send_data(PKT_MOVE, id, ox + 3, oy + 3);

}

int next_unit(int whos, int id) {

	int i;
	for (i = id + 1; i < 5; i++) {
		KBunit *u = &base.units[whos][i];
		if (!u->count) continue;
		if (u->acted) continue;
		return i;
	}	
	for (i = 0; i < 5; i++) {
		KBunit *u = &base.units[whos][i];
		if (!u->count) continue;
		if (u->acted) continue;
		return i;
	}

	return -1;
}

void combat_wait(int *troop_id) {

}


void reset_turn() {
		int i, j;
		for (j = 0; j < 2; j++)
		for (i = 0; i < 5; i++) {
			base.units[j][i].acted = 0;
		}
}

int turn_callback(const char *data) {


	if (MyRole == Client) {
	
		base.your_turn = 1 - base.your_turn;
		base.side = 1 - base.side;
		
		reset_turn();

		base.unit_id = next_unit(base.side, -1);
	
	}
}

int wait_callback(const char *data) {

	if (MyRole == Client) {
	
	
	}

}

/* Opponent says a unit passes it's turn. */
int pass_callback(const char *data) {

	int id = data[0];

	if (MyRole == Client) {

		base.unit_id = next_unit(1, id);

	}

	if (MyRole == Server) {
		KBunit *u = &base.units[1][id];
		u->acted = 1;

		int t = next_unit(1, id);

		if (t == -1) {
			base.side = 0;
			base.your_turn = 1;
			reset_turn();
			send_data(PKT_TURN, 0);
			base.unit_id = next_unit(base.side, -1);
		} else {
			base.unit_id = t;
			send_data(PKT_PASS, id);
		}


	}

}

void combat_pass(int troop_id) {
	KBunit *u = &base.units[0][troop_id];

	u->acted = 1;

	if (MyRole == Client) {
	
		send_data(PKT_PASS, troop_id);
		return;
	}

	int t = next_unit(0, troop_id);
	if (t == -1) {
		base.side = 1;
		base.your_turn = 0;
		reset_turn();
		send_data(PKT_TURN, 0);
		base.unit_id = next_unit(1, -1);
	} else {
		base.unit_id = t;
		send_data(PKT_PASS, troop_id);
	}
}


void reset_match() {

	int j;
	int i;
	for (j = 0; j < 2; j++) 
	for (i = 0; i < 2; i++)
	{ 
		KBunit *u = &base.units[j][i];
		if (!u->count) continue;
		base.umap[u->y][u->x] = (j * 5) + i + 1;
	}

	base.your_turn = 0;
	base.side = 1;
	base.unit_id = 0;

	if (MyRole == Server)
	{
		base.side = 0;
		base.your_turn = 1;
	}
	
	reset_turn();
}

int run_match(KBconfig *conf) {

	SDL_Surface *screen = sys->screen;
	int done = 0;

	reset_match();
	base.unit_id = next_unit(base.side, -1);

	Uint32 last = 0;
	while (!done) {
	
		int tick = 0;
		Uint32 now = SDL_GetTicks();
		
		if (now - last > 300) {
			last = now;
			tick = 1;	
		}

		if (tick)
		base.units[base.side][base.unit_id].frame++;
		if (base.units[base.side][base.unit_id].frame > 3)
			base.units[base.side][base.unit_id].frame = 0;

		int key = 0;

		if (receive_data() < 0) {
			KB_errlog("Connection error\n");
			done = 1;
		}

		SDL_FillRect(screen, NULL, (MyRole == Server ? 0x000000 : 0xFF0000 ));

		int i, j;
		for (j = 0; j < 2; j++) {
			for (i = 0; i < 5; i++) {
			
				KBunit *u = &base.units[j][i];
				
				if (u->count == 0) continue;
				
				SDL_Rect src = { u->frame * 48, j * 34, 48, 34 };
				SDL_Rect dst = { 0, 0, 48, 34 };
				
				dst.x = u->x * 48;
				dst.y = u->y * 34;
				
	
				SDL_BlitSurface(troop_cache[u->troop_id], &src, screen, &dst);
			}
		}
		
		draw_console();

		key = KB_event();

		if (key == 0xFF) done = 1;

		if (base.side == 0) {
			if (key == SDLK_RIGHT) {
				move_unit(base.unit_id, 1, 0);
			}
			if (key == SDLK_LEFT) {
				move_unit(base.unit_id, -1, 0);
			}
			if (key == SDLK_DOWN) {
				move_unit(base.unit_id, 0, 1);
			}
			if (key == SDLK_UP) {
				move_unit(base.unit_id, 0, -1);
			}
			if (key == SDLK_w) {
				combat_wait(&base.unit_id);
			}
			if (key == SDLK_SPACE) {
				combat_pass(base.unit_id);
			}
		}


		SDL_Flip(screen);
		
		SDL_Delay(10);	
	}
	
	free_troops();


	return 0;
}

int hello_callback(const char *data) {

	printf("Hello there! <%c %c %c %c> [%02x] [%02x] [%02x] [%02x]\n",data[0],data[1],data[2],data[3], data[0],data[1],data[2],data[3]);

	return 0;
}

int chat_callback(const char *data) {

	char buffer[80];
	snprintf(buffer, 80, "[%s] %s", "Player2", data);
	add_message(buffer);

	return 0;
}

int gold_callback(const char *data) {

	printf("Hello there! <%c %c %c %c> [%02x] [%02x] [%02x] [%02x]\n",data[0],data[1],data[2],data[3], data[0],data[1],data[2],data[3]);

	return 0;
}

int voidl(const char *data, int max) {
int i;
for (i = 0; i < max; i++) {
	printf("[%02x] ", data[i]);
}
printf("\n");
}

int army_callback(const char *data) {

	int i;
	char *p = data;
	for (i = 0; i < 5; i++)
	{
		base.e_army[i] = data[i];
	}

	voidl(data, 10);

	for (i = 0; i < 5; i++) {
		base.e_nums[i] 	= SDLNet_Read16(&data[5 + i * 2]);
	}
	
	for (i = 0; i < 5; i++)
	{
		base.units[1][i].troop_id = base.e_army[i];
		base.units[1][i].count = base.e_nums[i];
		base.units[1][i].y = i;
		base.units[1][i].x = LEVEL_W - 1;
	}
	
	if (MyRole == Server) /* Verify and accept army */
		send_data(PKT_READY, 1);


	return 0;
}





/*
 * The protocol / algorithm is the simplest ever.
 *
 */
int read_data(const char *buffer) {

	byte id = READ_BYTE(buffer);

	KBpacket *pkt = &packets[id];
	
	int res = (pkt->callback)(buffer);

	return res;
}

int receive_data() {

	static char old_buffer[1024] = { 0 };
	static int have_bytes = 0;

	char buffer[512];
	
	int n;

	/* Ensure socket is ready */
	if (SDLNet_CheckSockets(set, 1) == -1) return -1;
    if (!SDLNet_SocketReady(rsd)) return 0;

	/* Read data */
	n = SDLNet_TCP_Recv(rsd, buffer, 512);

	/* Fatal error */
	if (n <= 0) return -1;

	/* Append new bytes */
	memcpy(&old_buffer[have_bytes], buffer, n);
	have_bytes += n;

	/* Parse packets */
	char *p = &old_buffer[0];
	
	/* Minimum for a packet is 3 bytes */
	while (have_bytes > 2) {
		word len = SDLNet_Read16(old_buffer);

		/* Yep, have a packet right there */
		if (have_bytes >= len - 2) {
			read_data(&old_buffer[2]);
		} else break;
		//printf("Shift it!\n");
		char tmp[1024];//ok... this clearly sucks 
		memcpy(tmp, old_buffer, have_bytes);
		
		/* Shift buffer left */
		memcpy(old_buffer, &tmp[len + 2], have_bytes - len - 2);
		have_bytes -= len;
		have_bytes -= 2;
	}

	return 0;
}
int send_data(int id, ...) {
	va_list argptr;
	
	va_start(argptr, id);
	
	KBpacket *pkt = &packets[id];

	int i;
	
	char buffer[1024] = { 0 };
	int len = 2;
	
	buffer[len] = (char)id;
	len++;

	for (i = 0; i < pkt->num_args; i++) {
	
		int fmt = pkt->format[i];
		
		switch (fmt) {
			case 'c':
			{
				int val = va_arg(argptr, int);
				buffer[len] = (char) val;
				len += 1;
			}
			break;
			case 'b':
			{
				int val = va_arg(argptr, int);
				buffer[len] = (unsigned char) val;
				len += 1;
			}
			break;
			case 'w':
			{
				int val = va_arg(argptr, int);
				SDLNet_Write16(val, &buffer[len]);
				len += 2;				
			}
			break;
			case 's':
			{
				char *val = va_arg(argptr, char*);
				int i, l = strlen(val);
				if (l > 80) l = 80;
				for (i = 0; i < l; i++) {
					buffer[len] = val[i];
					len++;	
				}
				for (; i < 80; i++) {
					buffer[len] = ' ';
					len++;
				}
			}
			break;
			default: 
			printf("Superbad\n");
			exit(-2);
			break;
		}

	}

	va_end(argptr);

	//printf("(%d) Sending data <%s>:\n", len, pkt->format);
	//voidl(buffer, len);

	SDLNet_Write16((len - 2), &buffer[0]);

	if (SDLNet_TCP_Send(rsd, (void *)buffer, len) < len) { 
		KB_errlog("SDLNet_TCP_Send: %s\n", SDLNet_GetError()); 
		return -1; 
	} 

	return len;
}

int wait_for_connection(int port) {

	int done = 0;
	char buffer[512];

	/* Resolving the host using NULL make network interface to listen */
	if (SDLNet_ResolveHost(&ip, NULL, port) < 0) {
		KB_errlog("SDLNet_ResolveHost: %s\n", SDLNet_GetError()); 
		return -1;
	}

	/* Open a connection with the IP provided (listen on the host's port) */ 
	if (!(sd = SDLNet_TCP_Open(&ip))) {
		KB_errlog("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
		return -1;
	}

	KB_iprint("Waiting for a connection on port %d\n", port);

	/* Wait for a connection */ 
	while (!done) { 
		 if ((rsd = SDLNet_TCP_Accept(sd))) 
		 { 
			if ((remoteIP = SDLNet_TCP_GetPeerAddress(rsd)))
			{ 
				KB_iprint("Client connected: %x %d\n", SDLNet_Read32(&remoteIP->host), SDLNet_Read16(&remoteIP->port));
				done = 1;
			}
			else KB_errlog("SDLNet_TCP_GetPeerAddress: %s\n", SDLNet_GetError()); 

			/* Close the listening socket */ 
		 	SDLNet_TCP_Close(sd);
		}
	}

	return 0;
}

int connect_to_host(const char *remote_host, int remote_port) {
	int quit, len; char buffer[512]; 

	/* Resolve the host we are connecting to */
	if (SDLNet_ResolveHost(&ip, remote_host, remote_port) < 0) { //argv[1], atoi(argv[2])) < 0) {
 		KB_errlog("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
 		return -1;
 	}

	KB_iprint("Connecting to %s:%d\n", remote_host, remote_port);

	/* Open a connection with the IP provided (listen on the host's port) */ 
	if (!(rsd = SDLNet_TCP_Open(&ip))) { 
 		KB_errlog("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
 		return -1;
 	}
 	
 	KB_iprint("Connected!\n");

	send_data(PKT_HELLO, "Hello", '!');
}

KBenv *KB_startENV(KBconfig *conf) {

	KBenv *nsys = malloc(sizeof(KBenv));

	if (!nsys) return NULL; 

    SDL_Init( SDL_INIT_VIDEO );

    nsys->screen = SDL_SetVideoMode( 640, 480, 32, SDL_HWSURFACE );

    nsys->conf = conf;

	nsys->font = NULL;

	RESOURCE_DefaultConfig(conf);

	prepare_inline_font();	// <-- inline font

	return nsys;
}

void KB_stopENV(KBenv *env) {

	if (env->font) SDL_FreeSurface(env->font);

	kill_inline_font();

	free(env);

	SDL_Quit();
}

int main_loop(KBconfig *conf, const char *host, int port) {

	int playing = 0;

	/* Init SDLNet */
	if (SDLNet_Init() < 0) { 
		KB_errlog("SDLNet_Init: %s\n", SDLNet_GetError()); 
		return -1; 
	} 

	if (MyRole == Server)	
	 	playing = wait_for_connection(port);
	 else
	 	playing = connect_to_host(host, port);	

	if (playing < 0) {
		SDLNet_Quit(); 
		return -1;
	}


	set = SDLNet_AllocSocketSet(1);
	if (!set) {
	    KB_errlog("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
	    return -1;
	}

//	int
	 SDLNet_TCP_AddSocket(set, rsd);

	/* Start new environment (game window) */
	sys = KB_startENV(conf);

	reset_console();

	/* Must be successfull to continue */
	if (!sys) {
		SDLNet_Quit(); 
		return -1;
	}

	/* Module auto-discovery */
	if (conf->autodiscover)
		discover_modules(conf->data_dir, conf);

	/* --- ! ! ! --- */
	conf->module = 0;

	/* No module! (Unlikely...) */
	if (conf->num_modules == 0) {
		KB_errlog("No modules found.\n");
		SDLNet_Quit(); 
		KB_stopENV(sys);
		return -1;
	}

	/* Load and use module font */
	sys->font = KB_LoadIMG8(GR_FONT, 0);
	infont(sys->font);

	/* --- X X X --- */
	KBcombat *battle = prepare_for_combat();
	playing = run_match(battle);

	/* Stop environment */
	SDLNet_Quit(); 
	KB_stopENV(sys);
	
	return 0;
}

void show_usage(const char *binary_name) {
	KB_stdlog("Usage:\n\t%s --host [PORT]\n", binary_name);
	KB_stdlog("or\t%s --join HOST [PORT]\n", binary_name);
}

int main(int argc, char* argv[]) {

	int playing = 1;	/* Play 1 game of KB */
	
	/* Lots of very boring things must happen for a proper initialisation... */
	KB_stdlog("openKB COMBAT EMULATOR version " PACKAGE_VERSION "\n");
	KB_stdlog("=====================================================\n");

	int port = 198995;
	char host[1024] = { 0 };

	if (argc >= 2) {
		if (!strcasecmp(argv[1], "--host")) MyRole = Server;
		if (!strcasecmp(argv[1], "--join")) MyRole = Client;
	}
	if (MyRole == Client && argc >= 3) {
		KB_strcpy(host, argv[2]);
	}
	if (MyRole == Client && argc >= 4) {
		port = atoi(argv[3]);
	}
	if (MyRole == Server && argc >= 3) {
		port = atoi(argv[2]);
	}

	/* Simple parameter checking */ 
	if (MyRole == Undefined) {
		show_usage(argv[0]); 
		KB_errlog("Must provide a --host or --join switch\n");
		return 1; 
	}
	if (MyRole == Client && host[0] == '\0') {
		show_usage(argv[0]); 
		KB_errlog("Must provide a HOST when --join'ining\n");
		return 1; 
	}

	/* Read default config file */
	wipe_config(&KBconf);
	read_env_config(&KBconf);
	if ( read_file_config(&KBconf, KBconf.config_file) )
	{
		KB_errlog("[config] Unable to read config file '%s'\n", KBconf.config_file); 
		return 1;
	}

	/* Output final config to stdout */
	KB_stdlog("\n");
	report_config(&KBconf);
	KB_stdlog("\n");

	/* IF SERVER */
	KB_stdlog("=====================================================\n");
	/* Play the game! */
	while (playing > 0)
		playing = main_loop(&KBconf, host, port);

	if (!playing) KB_stdlog("Thank you for playing!\n");
	else KB_errlog("A Fatal Error has occured, terminating!\n");

	return playing;
}

