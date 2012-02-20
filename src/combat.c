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
/* 
 * Random notes:
 * 
 * Complete gamestate is never transmitted nor even exists,
 * both players' in-memory representation has "your" army 
 * on the left side of the screen and "opponent's" army on the
 * right side (and the X coordinate is often flipped in the
 * packet-receiving functions).
 * 
 * Thus, it's very easy to get out of sync. *VERY* EASY. 
 * 
 * The authority over the game is shared, altho server has more power.
 * Both client and server check each other for errors and disconnect
 * if something is wrong. Nothing should ever be wrong, because
 * the rules of the game are extremly simple. Imagine playing a chess
 * match and seeing your opponent's pawn moving like a queen, naturally
 * your reasonable response would be is slapping him in the face.
 * If something gets out of sync it means either party is cheating
 * or there's a bug in this software. Which should be easy to resolve
 * given the rules are very simple.
 *
 * Neither client nor server wait for confirmations when doing their thing,
 * but sometimes client has to wait for server's decision.
 * 
 * TODO:
 * What would be neat to add is some crypto routine and an RNG, so after
 * the match is over, client would be able to verify all the dice rolls.
 * That would pretty much nullify all possibility of cheating, while
 * keeping the model as simple as it is now.
 *
 * As of now, server can cheat to it's heart's content with the dice rolls,
 * any other tampering would be directly evident to the client. Client
 * has no ability to cheat at all.
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

#include "SDL.h"
#include "SDL_net.h" 

struct KBconfig KBconf;
struct KBenv *sys;
TCPsocket sd, rsd; /* Socket descriptor, remote socket descriptor */ 
IPaddress ip, *remoteIP; 
SDLNet_SocketSet set;

KBcombat base = { 0 };
int max_gold = 0;

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

#define PKT_CLEAN 16
#define PKT_GRID 17

#define FMT_CHAR "c"
#define FMT_BYTE "b"
#define FMT_WORD "w"
#define FMT_SWORD "W"
#define FMT_DWORD "d"
#define FMT_SINT "i"
#define FMT_STR "s"

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

int clean_callback(const char *data);
int grid_callback(const char *data);

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
	{ 0 },
	{ 0 },
	{ 0 },

	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ PKT_CLEAN, 1, FMT_BYTE, &clean_callback },
	{ PKT_GRID, 3, FMT_BYTE FMT_BYTE FMT_BYTE, &grid_callback },

};
int num_packets = sizeof(packets) / sizeof(KBpacket);

enum {

	Undefined,
	Server,
	Client,

} MyRole = Undefined;

#define MAX_MESSAGES 20

typedef struct KBconsole {

	int num_messages;
	
	struct {
	
		int delay;
		char text[80];	

	} message[MAX_MESSAGES];

	char your[80];
	int pos;
	int chatting;
	
} KBconsole;

KBconsole console;

#if 1
void printx(const char *data, int max) {
	int i;
	for (i = 0; i < max; i++)
		printf("[%02x] ", data[i]);
	printf("\n");
}
#endif

/*
 * NETWORK Receive/Send functions.
 *
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

	char tmp_buffer[1024];	/* for shifting left */
	char buffer[512];
	
	int n, res = 0;

	/* Ensure socket is ready */
	if (SDLNet_CheckSockets(set, 1) == -1) return -1;
	if (!SDLNet_SocketReady(rsd)) return 0;

	/* Read data */
	n = SDLNet_TCP_Recv(rsd, buffer, 512);

	/* Fatal error */
	if (n <= 0) return -1;
	if (n + have_bytes > 1023) { KB_errlog("Input buffer overflow! (%d bytes)\n", n + have_bytes); return -1;}

	/* Append new bytes */
	memcpy(&old_buffer[have_bytes], buffer, n);
	have_bytes += n;

	/* Minimum for a packet is 3 bytes */
	while (have_bytes > 2) {
		/* Because 2 contain 'packet length'. */
		word len = SDLNet_Read16(old_buffer);

		/* We read it out and compare to number of bytes we have. */
		if (have_bytes >= len - 2) {
			res = read_data(&old_buffer[2]);
			if (res) break;
		}
		else break;

		/* Shift leftovers to the left */
		/* By copying everything to a tmp buffer */
		memcpy(tmp_buffer, old_buffer, have_bytes);
		/* And then copying back only the relevant portion */
		memcpy(old_buffer, &tmp_buffer[len + 2], have_bytes - len - 2);

		/* We have that much bytes left: */

		have_bytes -= len;	/* Packet Body */ 
		have_bytes -= 2;	/* Packet Header */
	}

	return res;
}
int send_data(int id, ...) {
	va_list argptr;

	int i, j, l;
	int val; char *str_val;

	char buffer[1024] = { 0 };
	int len = 2;
	/* We leave 2 bytes to insert 'packet length' later */

	KBpacket *pkt = &packets[id];

	/* Then, 1 byte packet-id */
	buffer[len] = (char)id;
	len++;

	va_start(argptr, id);

	/* And then the actual arguments, in varying formats */
	for (i = 0; i < pkt->num_args; i++) {
		switch (pkt->format[i]) {
			case 'c':
				val = va_arg(argptr, int);
				buffer[len] = (char) val;
				len += 1;
			break;
			case 'b':
				val = va_arg(argptr, int);
				buffer[len] = (unsigned char) val;
				len += 1;
			break;
			case 'w':
				val = va_arg(argptr, int);
				SDLNet_Write16(val, &buffer[len]);
				len += 2;				
			break;
			case 's':
				str_val = va_arg(argptr, char*);
				l = strlen(str_val);
				if (l > 80) l = 80; /* all strings are always 80 char long */
				for (j = 0; j < l; j++) 
					buffer[len++] = str_val[j];
				for (; j < 80; j++) /* pad the rest with spaces */
					buffer[len++] = ' ';
			break;
			default: 
			printf("Superbad '%c' | %s / %d / %d\n", pkt->format[i], pkt->format, i, id);
			exit(-2);
			break;
		}
	}

	va_end(argptr);

//	printf("(%d) Sending data <%s>:\n", len, pkt->format);
//	printx(buffer, len);

	/* Insert calculated packet length into beginning of the packet */
	SDLNet_Write16((len - 2), &buffer[0]);

	/* SEND! */
	if (SDLNet_TCP_Send(rsd, (void *)buffer, len) < len) {
		SDL_Event event;
		event.type = SDL_USEREVENT;
		event.user.code = 0xFF;
		event.user.data1 = 0;
		event.user.data2 = 0;
		SDL_PushEvent(&event); 
		KB_errlog("SDLNet_TCP_Send: %s\n", SDLNet_GetError()); 
		return -1; 
	} 

	return len;
}

/*
 * KBconsole "interface"
 */
void reset_console() {

	console.num_messages = 0;

	console.your[0] = '\0';
	console.pos = 0;

	console.chatting = 0;
}

void add_message(const char *msg) {
	int i;

	for (i = 0; i < MAX_MESSAGES - 1; i++) {

		KB_strcpy(console.message[i].text, console.message[i+1].text);	

	}

	KB_strcpy(console.message[MAX_MESSAGES - 1].text, msg);

	if (console.num_messages < MAX_MESSAGES) console.num_messages++;
}

void KB_iprint(char *fmt, ...) 
{ 
	char buf[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(buf, 255, fmt, argptr);
	vfprintf(stdout, buf, NULL);
	add_message(buf);
	va_end(argptr);
}

void draw_console() {
	int i;
	SDL_Surface *screen = sys->screen;

	int y = screen->h - (MAX_MESSAGES+1) * 8;

	incolor(0x000000, 0xFFFFFF);
	for (i = 0; i < MAX_MESSAGES; i++)
		inprint(screen, console.message[i].text, 0, y + i * 8);	

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
		if (event.type == SDL_USEREVENT) eve = event.user.code;

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

int dwmap[5][5];

void fill_dwmap() {
	int dw_off[5] = { 0 };
	int i;
	for (i = 0; i < MAX_TROOPS; i++) {
		int dwells = troops[i].dwells;
		/* Hack -- cycle dwellings so 'castle' is first, 'plains' second */
		dwells++;
		if (dwells > DWELLING_CASTLE) dwells = DWELLING_PLAINS;
		dwmap[dwells][dw_off[dwells]++] = i;
	}
}

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

KBcombat* prepare_for_combat() {

	int done = 0;
	
	alloc_troops();

	fill_dwmap();

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

	KBcombat *war = &base;
	
	while (!done) {

		int key = 0;
#if 1
		if (receive_data() < 0) {
			KB_errlog("Connection error\n");
			war = NULL;
			done = 1;
		}
#endif
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

inprint(screen, (opponent_ready ? "Your opponent IS READY" : "Your opponent is not ready"), 10, 280);
inprint(screen, (youare_ready ? "You are READY" : "You are not ready! Press F1!"), 10, 290);
				
		
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

		if (key == 0xFF) { war = NULL; done = 1; }

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

	SDL_FreeSurface(dwell[0]);
	SDL_FreeSurface(dwell[1]);
	SDL_FreeSurface(dwell[2]);
	SDL_FreeSurface(dwell[3]);
	SDL_FreeSurface(dwell[4]);

	if (war == NULL) free_troops();

	return war;
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
	return 0;
}
void move_unit(int id, int ox, int oy) {
	KBunit *u = &base.units[0][id];

	if (u->x + ox < 0 || u->x + ox > CLEVEL_W - 1
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

void combat_wait() {

}


int turn_callback(const char *data) {

	if (MyRole == Client) {
	
		base.your_turn = 1 - base.your_turn;
		base.side = 1 - base.side;
		
		reset_turn(&base);

		base.unit_id = next_unit(base.side, -1);
	
	}
	return 0;
}

int wait_callback(const char *data) {
	if (MyRole == Client) {
	
	
	}
	return 0;
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
			reset_turn(&base);
			send_data(PKT_TURN, 0);
			base.unit_id = next_unit(base.side, -1);
		} else {
			base.unit_id = t;
			send_data(PKT_PASS, id);
		}


	}
	return 0;
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
		reset_turn(&base);
		send_data(PKT_TURN, 0);
		base.unit_id = next_unit(1, -1);
	} else {
		base.unit_id = t;
		send_data(PKT_PASS, troop_id);
	}
}


void net_reset_match() {

	int i, j;

	base.your_turn = 0;
	base.side = 1;
	base.unit_id = 0;

	if (MyRole == Server)
	{
		base.side = 0;
		base.your_turn = 1;

		send_data(PKT_CLEAN, 0);
		for (j = 0; j < CLEVEL_H; j++)
		for (i = 1; i < CLEVEL_W - 2; i++)
			if (base.omap[j][i])
				send_data(PKT_GRID, j, i, base.omap[j][i]);
	}
}

int run_match(KBcombat *war) {

	SDL_Surface *screen = sys->screen;
	int done = 0;

	reset_match();
	net_reset_match();
	base.unit_id = next_unit(base.side, -1);

	SDL_Surface *comtiles = SDL_LoadRESOURCE(GR_COMTILES, 0, 0);

	Uint32 last = 0;
	while (!done) {
	
		int tick = 0;
		Uint32 now = SDL_GetTicks();
		
		if (now - last > 300) {
			last = now;
			tick = 1;	
		}

		if (tick && ++base.units[base.side][base.unit_id].frame > 3)
			base.units[base.side][base.unit_id].frame = 0;

		int key = 0;

		if (receive_data() < 0) {
			KB_errlog("Connection error\n");
			done = 1;
		}

		SDL_FillRect(screen, NULL, (MyRole == Server ? 0x000000 : 0xFF0000 ));
		
		int i, j;

		for (j = 0; j < CLEVEL_H; j++)		
		for (i = 0; i < CLEVEL_W; i++) {

			SDL_Rect src = { base.omap[j][i] * 48, 0, 48, 34 };
			SDL_Rect dst = { i * 48, j * 34, 48, 34 };

			SDL_BlitSurface(comtiles, &src, screen, &dst);
		}

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
				combat_wait(base.unit_id);
			}
			if (key == SDLK_SPACE) {
				combat_pass(base.unit_id);
			}
		}

		SDL_Flip(screen);

		SDL_Delay(10);	
	}

	SDL_FreeSurface(comtiles);

	free_troops();

	return 0;
}

int chat_callback(const char *data) {

	char buffer[80];
	snprintf(buffer, 80, "[%s] %s", "Player2", data);
	add_message(buffer);

	return 0;
}

int gold_callback(const char *data) {
	word gold = SDLNet_Read16(data);
	
	max_gold = gold;
	youare_ready = 0;
	opponent_ready = 0;

	return 0;
}

int clean_callback(const char *data) {
	int i, j;
	for (j = 0; j < CLEVEL_H; j++)		
	for (i = 0; i < CLEVEL_W; i++)
		base.omap[j][i] = 0;

	return 0;	
}
int grid_callback(const char *data) {
	Uint8 y = data[0];
	Uint8 x = data[1];
	Uint8 o = data[2];

	base.omap[y][x] = o;
	return 0;
}

int army_callback(const char *data) {

	int i;

	for (i = 0; i < 5; i++)
	{
		base.units[1][i].troop_id = data[i];
	}

	printx(data, 10);

	for (i = 0; i < 5; i++)
	{
		base.units[1][i].count 	= SDLNet_Read16(&data[5 + i * 2]);
	}
	
	for (i = 0; i < 5; i++)
	{
		base.units[1][i].y = i;
		base.units[1][i].x = CLEVEL_W - 1;
	}

	if (MyRole == Server) /* Verify and accept army */
		send_data(PKT_READY, 1);

	return 0;
}

int hello_callback(const char *data) {

	printf("Hello there! <%c %c %c %c> [%02x] [%02x] [%02x] [%02x]\n",data[0],data[1],data[2],data[3], data[0],data[1],data[2],data[3]);

	return 0;
}

int wait_for_connection(int port) {
	int done = 0;

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

	/* Resolve the host we are connecting to */
	if (SDLNet_ResolveHost(&ip, remote_host, remote_port) < 0) {
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

	send_data(PKT_HELLO, "Hello", PACKAGE_VERSION);
	return 0;
}

int main_loop(KBconfig *conf, const char *host, int port) {

	int playing = 0;

	/*
	 * Start networking 
	 */
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
	/* To use non-blocking sockets in SDL, a set must be created: */
	set = SDLNet_AllocSocketSet(1);
	if (!set) {
	    KB_errlog("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
	    SDLNet_Quit();
	    return -1;
	}
	SDLNet_TCP_AddSocket(set, rsd); /* And we put our lonely socket there */

	/*
	 * Start the regular stuff
	 */

	/* Hack? Pretend to use normal2x */
	conf->filter = 1;

	/* Start new environment (game window) */
	sys = KB_startENV(conf);

	/* That's clearly wrong: */
	conf->filter = 0;

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
	if (battle)
		run_match(battle);

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
		if (!strcasecmp(argv[1], "--host") || !strcasecmp(argv[1], "-h")) MyRole = Server;
		if (!strcasecmp(argv[1], "--join") || !strcasecmp(argv[1], "-j")) MyRole = Client;
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
	//temp hack to always read local file
	KB_strcpy(KBconf.config_file, "./openkb.ini");
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

