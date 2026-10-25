// Two linked mGBA cores, headless, driven by a script. See README.md.
#include <mgba/core/core.h>
#include <mgba/core/thread.h>
#include <mgba/core/log.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/savedata.h>
#include <mgba/internal/gba/sio/lockstep.h>
#include <mgba-util/vfs.h>

#include <ctype.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>

#define W 240
#define H 160
#define MAX_CMDS 4096
#define MAX_SYMS 500000

enum { C_PARTNER, C_GOTO, C_HEX, C_PRESS, C_WAIT, C_SHOT, C_DUMP, C_BARRIER, C_UNTIL_CB2, C_UNTIL_FIELD, C_EXPECT, C_UNTIL_MEM, C_BATTLE, C_MENU, C_PARTY, C_FIXTURE, C_WRITE, C_LEAD, C_MOVE, C_SIX, C_VAR, C_FLAG, C_VS, C_EXPECT_FLAG, C_EXPECT_VAR, C_NO_OBJECT, C_MAP, C_OBJECT, C_TEXT_SPEED, C_PACE, C_MUTE, C_PARTNER_AT, C_ITEM, C_EXPECT_ITEM, C_UNTIL_TEXT, C_WARP, C_PC_COUNT, C_MONS, C_NO_COPIES, C_NOTE_MONEY, C_MONEY_GAIN, C_FORCE_ENC, C_EXPECT_RANGE };

struct Cmd {
	int kind;
	unsigned keys;
	int frames;
	int barrier;
	char text[96];
	unsigned addr;
	unsigned value;
	int size;
};

struct Sym { unsigned addr; unsigned size; char type; char name[64]; };

struct Player {
	int id;
	struct mCore* core;
	struct mCoreThread thread;
	struct GBASIOLockstepNode node;
	uint32_t video[W * H];
	struct Cmd cmds[MAX_CMDS];
	int ncmds, pc, step, frame;
	int waitMask, awake;
	int32_t cyclesPosted;
	int barrierReached;
	int stable;
	int done;
	unsigned moneyBase;
    unsigned vsFlags;
    unsigned muteFrames, mutedOpcode;
    unsigned encSaved[2], encForced;
};

static struct Player P[2];
static struct GBASIOLockstep sLockstep;
static pthread_mutex_t sLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sOut = PTHREAD_MUTEX_INITIALIZER;
static struct Sym* sSyms;
static int sNumSyms;
static const char* sOutDir = "/tmp/coop_harness";
static int sFailures;
static int sMaxFrames = 200000;
static FILE* sRecord;
static pthread_mutex_t sRecLock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t sSnap[2][W * H];
static int sSnapFrame[2];
static int sRecorded;
static unsigned sLastCb2[2];
static int sCb2Logs[2];

static void out(const char* fmt, ...) {
	va_list ap;
	pthread_mutex_lock(&sOut);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	fflush(stdout);
	pthread_mutex_unlock(&sOut);
}

static void loadSyms(const char* path) {
	FILE* f = fopen(path, "r");
	char line[256];
	if (!f) { fprintf(stderr, "no symbol file %s\n", path); exit(1); }
	sSyms = calloc(MAX_SYMS, sizeof(*sSyms));
	while (fgets(line, sizeof(line), f) && sNumSyms < MAX_SYMS) {
		unsigned a, s = 0; char t; char n[64];
		if (sscanf(line, "%x %x %c %63s", &a, &s, &t, n) == 4 || (s = 0, sscanf(line, "%x %c %63s", &a, &t, n) == 3)) {
			sSyms[sNumSyms].addr = a;
			sSyms[sNumSyms].size = s;
			sSyms[sNumSyms].type = t;
			strcpy(sSyms[sNumSyms].name, n);
			sNumSyms++;
		}
	}
	fclose(f);
}

static unsigned symValue(const char* name) {
	char* end = NULL;
	unsigned v = strtoul(name, &end, 0);
	if (end != name && *end == '\0') return v;
	int best = -1;
	for (int i = 0; i < sNumSyms; i++) {
		if (strcmp(sSyms[i].name, name)) continue;
		if (best < 0 || sSyms[i].size > sSyms[best].size) best = i;
	}
	if (best < 0) {
		fprintf(stderr, "unknown value %s\n", name);
		exit(1);
	}
	unsigned addr = sSyms[best].addr;
	if (sSyms[best].type == 't' || sSyms[best].type == 'T') addr |= 1u;
	return addr;
}

static unsigned symAddr(const char* name) {
	for (int i = 0; i < sNumSyms; i++)
		if (!strcmp(sSyms[i].name, name))
			return (sSyms[i].type == 'T' || sSyms[i].type == 't') ? sSyms[i].addr & ~1u : sSyms[i].addr;
	fprintf(stderr, "unknown symbol %s\n", name);
	exit(1);
}

static const char* symName(unsigned addr) {
	static __thread char buf[80];
	addr &= ~1u;
	if (!addr) return "NULL";
	for (int i = 0; i < sNumSyms; i++)
		if ((sSyms[i].type == 'T' || sSyms[i].type == 't') && (sSyms[i].addr & ~1u) == addr)
			return sSyms[i].name;
	snprintf(buf, sizeof(buf), "%08x", addr);
	return buf;
}

static const char* symAtAny(unsigned addr) {
	static __thread char buf[120];
	int best = -1;
	for (int i = 0; i < sNumSyms; i++)
		if (sSyms[i].addr >= 0x02000000 && sSyms[i].addr <= addr && (best < 0 || sSyms[i].addr > sSyms[best].addr))
			best = i;
	if (best < 0) snprintf(buf, sizeof(buf), "%08x", addr);
	else snprintf(buf, sizeof(buf), "%s+0x%x", sSyms[best].name, addr - sSyms[best].addr);
	return buf;
}

static const char* symAt(unsigned addr) {
	static __thread char buf[120];
	int best = -1;
	addr &= ~1u;
	for (int i = 0; i < sNumSyms; i++)
		if ((sSyms[i].type == 'T' || sSyms[i].type == 't') && sSyms[i].addr >= 0x02000000 && (sSyms[i].addr & ~1u) <= addr
		 && (best < 0 || sSyms[i].addr > sSyms[best].addr))
			best = i;
	if (best < 0) snprintf(buf, sizeof(buf), "%08x", addr);
	else snprintf(buf, sizeof(buf), "%s+0x%x", sSyms[best].name, addr - (sSyms[best].addr & ~1u));
	return buf;
}

static unsigned rd8(struct Player* p, unsigned a) { return p->core->busRead8(p->core, a); }
static unsigned rd16(struct Player* p, unsigned a) { return p->core->busRead16(p->core, a); }
static unsigned rd32(struct Player* p, unsigned a) { return p->core->busRead32(p->core, a); }
static unsigned rdN(struct Player* p, unsigned a, int n) { return n == 1 ? rd8(p, a) : n == 2 ? rd16(p, a) : rd32(p, a); }

static void writePng(struct Player* p, const char* name) {
	char path[512];
	snprintf(path, sizeof(path), "%s/%s_p%d.png", sOutDir, name, p->id);
	size_t rawLen = (size_t)H * (W * 3 + 1);
	unsigned char* raw = malloc(rawLen);
	for (int y = 0; y < H; y++) {
		unsigned char* row = raw + y * (W * 3 + 1);
		row[0] = 0;
		for (int x = 0; x < W; x++) {
			uint32_t c = p->video[y * W + x];
			row[1 + x * 3] = c & 0xFF;
			row[2 + x * 3] = (c >> 8) & 0xFF;
			row[3 + x * 3] = (c >> 16) & 0xFF;
		}
	}
	uLongf zLen = compressBound(rawLen);
	unsigned char* z = malloc(zLen);
	compress(z, &zLen, raw, rawLen);
	FILE* f = fopen(path, "wb");
	static const unsigned char sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	fwrite(sig, 1, 8, f);
	unsigned char ihdr[13] = { 0, 0, 0, W, 0, 0, 0, H, 8, 2, 0, 0, 0 };
	struct { const char* type; unsigned char* data; uint32_t len; } chunks[3] = {
		{ "IHDR", ihdr, 13 }, { "IDAT", z, (uint32_t)zLen }, { "IEND", NULL, 0 }
	};
	for (int i = 0; i < 3; i++) {
		unsigned char hdr[8] = { chunks[i].len >> 24, chunks[i].len >> 16, chunks[i].len >> 8, chunks[i].len };
		memcpy(hdr + 4, chunks[i].type, 4);
		fwrite(hdr, 1, 8, f);
		if (chunks[i].len) fwrite(chunks[i].data, 1, chunks[i].len, f);
		uLong crc = crc32(0, hdr + 4, 4);
		if (chunks[i].len) crc = crc32(crc, chunks[i].data, chunks[i].len);
		unsigned char c[4] = { crc >> 24, crc >> 16, crc >> 8, crc };
		fwrite(c, 1, 4, f);
	}
	fclose(f);
	free(raw);
	free(z);
}

static unsigned sMain, sObjEvents, sAvatar, sSave1, sBattleFlags, sLockCtl, sLinkErr, sScriptStatus;
static unsigned sCoopBss;

static unsigned symFind(const char* name) {
	for (int i = 0; i < sNumSyms; i++)
		if (!strcmp(sSyms[i].name, name))
			return sSyms[i].addr;
	return 0;
}

static void printCb2Trace(struct Player* p) {
	unsigned nAddr = symFind("gCb2TraceN");
	unsigned cbAddr = symFind("gCb2TraceCb");
	unsigned lrAddr = symFind("gCb2TraceLr");
	if (!nAddr || !cbAddr || !lrAddr) return;
	unsigned n = rd8(p, nAddr);
	out("[p%d] callback trace n=%u live=%08x\n", p->id, n, rd32(p, sMain + 4));
	unsigned start = n > 8 ? n - 8 : 0;
	for (unsigned i = start; i < n; i++) {
		unsigned cb = rd32(p, cbAddr + (i & 7) * 4);
		unsigned lr = rd32(p, lrAddr + (i & 7) * 4);
		out("  set %08x %s from %s\n", cb, symName(cb), symAt(lr));
	}
}

static unsigned coopStatic(const char* name) {
	char full[80];
	snprintf(full, sizeof(full), "coop.%s", name);
	return symAddr(full);
}

static void dump(struct Player* p, const char* tag);

static unsigned partyMon(int trainer, int slot);
static const unsigned char sSub0[24] = {0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 2, 3, 1, 1, 2, 3, 2, 3, 1, 1, 2, 3, 2, 3};

static void readLead(struct Player* p, unsigned* species, unsigned* exp, unsigned* held) {
	unsigned mon = partyMon(0, 0);
	unsigned personality = rd32(p, mon);
	unsigned ot = rd32(p, mon + 4);
	unsigned key = personality ^ ot;
	unsigned words[12];
	for (int i = 0; i < 12; i++)
		words[i] = rd32(p, mon + 32 + i * 4) ^ key;
	unsigned sub = sSub0[personality % 24] * 3;
	*species = words[sub] & 0x7FF;
	*held = (words[sub] >> 16) & 0x3FF;
	*exp = words[sub + 1] & 0x1FFFFF;
}

// Explicit test fixture: seed a real encrypted lead immediately below a level
// boundary. Battle EXP, move UI, evolution, and save writes still run in-game.
static void fixtureLead(struct Player* p, unsigned trainer, unsigned species, unsigned level, unsigned exp) {
    static const unsigned char sub1[24] = {1,1,2,3,2,3,0,0,0,0,0,0,2,3,1,1,3,2,2,3,1,1,3,2};
    unsigned mon = partyMon(trainer, 0), pid = rd32(p, mon), key = pid ^ rd32(p, mon + 4), words[12];
    for (int i = 0; i < 12; i++) words[i] = rd32(p, mon + 32 + i * 4) ^ key;
    unsigned a = sSub0[pid % 24] * 3, b = sub1[pid % 24] * 3;
    words[a] = (words[a] & ~0x7FFu) | species;
    words[a + 1] = (words[a + 1] & ~0x1FFFFFu) | exp;
    // Four distinct moves, none of the boundary moves used by these scenarios.
    words[b] = 10 | (45u << 16);
    words[b + 1] = 52 | (64u << 16);
    words[b + 2] = 0x23232323;
    unsigned checksum = 0;
    for (int i = 0; i < 12; i++) {
        checksum += (words[i] & 0xFFFF) + (words[i] >> 16);
        p->core->busWrite32(p->core, mon + 32 + i * 4, words[i] ^ key);
    }
    p->core->busWrite16(p->core, mon + 28, checksum);
    p->core->busWrite8(p->core, mon + 84, level);
    out("[p%d] fixture species=%u level=%u exp=%u (cached stats unchanged)\n", p->id, species, level, exp);
}

static unsigned leadMove(struct Player* p, unsigned slot) {
    static const unsigned char sub1[24] = {1,1,2,3,2,3,0,0,0,0,0,0,2,3,1,1,3,2,2,3,1,1,3,2};
    unsigned mon = partyMon(0, 0), pid = rd32(p, mon), key = pid ^ rd32(p, mon + 4);
    unsigned word = rd32(p, mon + 32 + sub1[pid % 24] * 12 + (slot / 2) * 4) ^ key;
    return (word >> ((slot % 2) * 16)) & 0x7FF;
}

static int bagHas(struct Player* p, unsigned item) {
	unsigned pockets = symAddr("gBagPockets");
	unsigned slots = rd32(p, pockets + 4 * 8);
	if (slots < 0x02000000) return 0;
	for (int i = 0; i < 30; i++) {
		unsigned id = rd16(p, slots + i * 4);
		if (id == item) return rd16(p, slots + i * 4 + 2);
		if (id == 0) break;
	}
	return 0;
}

// A game string as ASCII: letters, digits, space, and a little punctuation;
// anything else is '~'. Control codes and line breaks become a space.
static void gameText(struct Player* p, unsigned addr, char* out, int max) {
	int n = 0;
	for (int i = 0; i < 420 && n < max - 1; i++) {
		unsigned c = rd8(p, addr + i);
		char ch;
		if (c == 0xFF) break;
		if (c >= 0xBB && c <= 0xD4) ch = 'A' + (c - 0xBB);
		else if (c >= 0xD5 && c <= 0xEE) ch = 'a' + (c - 0xD5);
		else if (c >= 0xA1 && c <= 0xAA) ch = '0' + (c - 0xA1);
		else if (c == 0x00) ch = ' ';
		else if (c == 0xAB) ch = '!';
		else if (c == 0xAD) ch = '.';
		else if (c == 0xAE) ch = '-';
		else if (c == 0xB4) ch = '\'';
		else if (c == 0xF0) ch = ':';
		else if (c == 0xB8) ch = ',';
		else if (c == 0x1B) ch = 'e';
		else if (c >= 0xF9) { ch = ' '; if (c == 0xFC || c == 0xFD) i++; }
		else ch = '~';
		out[n++] = ch;
	}
	out[n] = 0;
}

// Species of each local party slot, decrypted (0 = empty).
static void showPartySpecies(struct Player* p) {
	char line[120]; int o = 0;
	for (int slot = 0; slot < 6; slot++) {
		unsigned mon = partyMon(0, slot), personality = rd32(p, mon), key = personality ^ rd32(p, mon + 4);
		unsigned w = rd32(p, mon + 32 + sSub0[personality % 24] * 12) ^ key;
		o += snprintf(line + o, sizeof(line) - o, "%u ", (rd8(p, mon + 19) & 2) ? (w & 0x7FF) : 0);
	}
	out("[f%06d p%d party-species] %s count=%u\n", p->frame, p->id, line, rd8(p, symAddr("gPartiesCount")));
}

// Species and moves of a party slot of any trainer, decrypted.
static void showMonAddr(struct Player* p, unsigned mon, int trainer, int slot) {
	static const unsigned char sub1[24] = {1,1,2,3,2,3,0,0,0,0,0,0,2,3,1,1,3,2,2,3,1,1,3,2};
	unsigned pid = rd32(p, mon), key = pid ^ rd32(p, mon + 4);
	unsigned w[12];
	for (int i = 0; i < 12; i++) w[i] = rd32(p, mon + 32 + i * 4) ^ key;
	unsigned a = sSub0[pid % 24] * 3, b = sub1[pid % 24] * 3;
	out("[f%06d p%d mon %d.%d] species=%u moves=%u,%u,%u,%u pid=%08x flags=%02x\n", p->frame, p->id, trainer, slot,
	    w[a] & 0x7FF, w[b] & 0xFFFF, w[b] >> 16, w[b + 1] & 0xFFFF, w[b + 1] >> 16, pid, rd8(p, mon + 19));
}

static void showMonAt(struct Player* p, int trainer, int slot) {
	showMonAddr(p, partyMon(trainer, slot), trainer, slot);
}

static void showParty(struct Player* p) {
	unsigned species, exp, held;
	readLead(p, &species, &exp, &held);
	unsigned hp = rd16(p, partyMon(0, 0) + 86);
	unsigned maxhp = rd16(p, partyMon(0, 0) + 88);
	unsigned lv = rd8(p, partyMon(0, 0) + 84);
	unsigned pid = rd32(p, partyMon(0, 0));
	unsigned slots = rd32(p, symAddr("gBagPockets") + 4 * 8);
	char keys[80];
	int o = 0;
	keys[0] = 0;
	for (int i = 0; i < 6 && o < 70; i++) {
		unsigned id = rd16(p, slots + i * 4);
		unsigned qty = rd16(p, slots + i * 4 + 2);
		if (!id) break;
		o += snprintf(keys + o, sizeof(keys) - o, "%u x%u ", id, qty);
	}
	out("[p%d] lead species=%u lv=%u hp=%u/%u exp=%u item=%u pid=%08x keys=[%s] raw=%04x %04x %04x %04x\n",
	    p->id, species, lv, hp, maxhp, exp, held, pid, keys,
	    rd16(p, slots), rd16(p, slots + 2), rd16(p, slots + 4), rd16(p, slots + 6));
}

static unsigned partyMon(int trainer, int slot) {
	return symAddr("gParties") + ((trainer * 6) + slot) * 100u;
}

static void checkBattle(struct Player* p, int moves) {
	unsigned flags = rd32(p, sBattleFlags);
	unsigned linkId = rd8(p, symAddr("gLink") + 2);
	unsigned err = rd8(p, sLinkErr);
	const unsigned need = (1u << 0) | (1u << 1) | (1u << 5) | (1u << 6) | (1u << 14);
	char why[180] = "";
	if ((rd32(p, sMain + 4) & ~1u) != symAddr("BattleMainCB2"))
		snprintf(why, sizeof(why), "not in battle");
	else if ((flags & need) != need || (flags & (1u << 3)))
		snprintf(why, sizeof(why), "flags %08x", flags);
	else if (((flags & (1u << 2)) != 0) != (linkId == 0))
		snprintf(why, sizeof(why), "master bit %08x linkId %u", flags, linkId);
	else if (err)
		snprintf(why, sizeof(why), "link error %u", err);
	else if (rd8(p, symAddr("gBattlersCount")) != 4)
		snprintf(why, sizeof(why), "battlers %u", rd8(p, symAddr("gBattlersCount")));
	else if (!moves) {
		unsigned own = rd32(p, partyMon(0, 0));
		unsigned partner = rd32(p, partyMon(2, 0));
		struct Player* other = &P[1 - p->id];
		unsigned mons = symAddr("gBattleMons");
		unsigned foe = 0, foeCount = 0, hpOk = 1;
		unsigned localBattler = linkId == 0 ? 0 : 2;
		unsigned ppOk = rd8(p, mons + localBattler * 140 + 37) != 0 && rd16(p, mons + localBattler * 140 + 42) != 0;
		for (int i = 0; i < 4; i++) {
			unsigned sp = rd16(p, mons + i * 140);
			unsigned lv = rd8(p, mons + i * 140 + 44);
			if (lv == 2 && sp) {
				if (foe && foe != sp) foe = 0xFFFF;
				else foe = sp;
				foeCount++;
			}
			if (lv == 2 && !rd16(p, mons + i * 140 + 42)) hpOk = 0;
		}
		unsigned otherFoe = 0, otherCount = 0;
		for (int i = 0; i < 4; i++) {
			unsigned sp = rd16(other, mons + i * 140);
			if (rd8(other, mons + i * 140 + 44) == 2 && sp) {
				otherFoe = sp;
				otherCount++;
			}
		}
		if (!own || rd32(p, partyMon(0, 1)) || !partner || rd32(p, partyMon(2, 1)) || own == partner)
			snprintf(why, sizeof(why), "roster personality own %08x partner %08x", own, partner);
		else if (own != rd32(other, partyMon(2, 0)) || partner != rd32(other, partyMon(0, 0)))
			snprintf(why, sizeof(why), "personality not mirrored");
		else if (rd8(p, partyMon(0, 0) + 84) != 5 || rd8(p, partyMon(2, 0) + 84) != 5)
			snprintf(why, sizeof(why), "starter levels %u %u", rd8(p, partyMon(0, 0) + 84), rd8(p, partyMon(2, 0) + 84));
		else if (linkId == 0 && (!ppOk || !hpOk))
			snprintf(why, sizeof(why), "zero hp or pp");
		else if (linkId == 0 && (foeCount != 2 || foe == 0xFFFF))
			snprintf(why, sizeof(why), "expected two level-2 foes, saw %u", foeCount);
		else if (foeCount && otherCount && foe != otherFoe)
			snprintf(why, sizeof(why), "foe %u vs %u", foe, otherFoe);
		else
			out("[p%d] ok battle roster lv5 hp %u/%u foe=%u linkId=%u\n", p->id, rd16(p, partyMon(0, 0) + 86), rd16(p, partyMon(0, 0) + 88), foe, linkId);
	} else {
		if (moves == 2) {
			// Party HP/maxHP sit in the clear tail of struct Pokemon (level at +84).
			// The guest does not run DoBattleIntro, so its gBattleMons stays zero;
			// the party is what both games actually keep; UI snapshots are not engine state.
			unsigned hp0 = rd16(p, partyMon(0, 0) + 86), max0 = rd16(p, partyMon(0, 0) + 88);
			unsigned hp2 = rd16(p, partyMon(2, 0) + 86), max2 = rd16(p, partyMon(2, 0) + 88);
			struct Player* o = &P[1 - p->id];
			unsigned ohp0 = rd16(o, partyMon(0, 0) + 86), omax0 = rd16(o, partyMon(0, 0) + 88);
			unsigned ohp2 = rd16(o, partyMon(2, 0) + 86), omax2 = rd16(o, partyMon(2, 0) + 88);
			unsigned mons = symAddr("gBattleMons");
			unsigned battleHp = rd16(p, mons + 42);
			if (linkId != 0) {
				if (hp0 != ohp2 || max0 != omax2 || hp2 != ohp0 || max2 != omax0)
					snprintf(why, sizeof(why), "party hp %u/%u and %u/%u vs other %u/%u and %u/%u", hp0, max0, hp2, max2, ohp0, omax0, ohp2, omax2);
				else
					out("[p%d] ok party hp matches the master (own %u/%u partner %u/%u)\n", p->id, hp0, max0, hp2, max2);
			} else if (rd16(p, mons) == 0 || battleHp == 0)
				snprintf(why, sizeof(why), "master battle mon empty");
			else if (hp0 != ohp2 || max0 != omax2 || hp2 != ohp0 || max2 != omax0)
				snprintf(why, sizeof(why), "party hp %u/%u and %u/%u vs guest %u/%u and %u/%u", hp0, max0, hp2, max2, ohp0, omax0, ohp2, omax2);
			else if ((rd32(o, symAddr("gBattleMainFunc")) & ~1u) != (symAddr("BeginBattleIntroDummy") & ~1u))
				snprintf(why, sizeof(why), "guest battle engine is running");
			else
				out("[p%d] ok party hp own %u/%u partner %u/%u; guest uses a UI snapshot; master mon0 hp %u pp %u\n",
				    p->id, hp0, max0, hp2, max2, battleHp, rd8(p, mons + 37));
		} else if (linkId != 0) {
			out("[p%d] ok move record is on the link master\n", p->id);
		} else {
			unsigned mons = symAddr("gBattleMons");
			unsigned move0 = rd16(p, symAddr("gChosenMoveByBattler"));
			unsigned move2 = rd16(p, symAddr("gChosenMoveByBattler") + 4);
			int slot0 = -1, slot2 = -1;
			for (int i = 0; i < 4; i++) {
				if (rd16(p, mons + 12 + i * 2) == move0) slot0 = i;
				if (rd16(p, mons + 2 * 140 + 12 + i * 2) == move2) slot2 = i;
			}
			unsigned bs = rd32(p, symAddr("gBattleStruct"));
			unsigned target0 = rd8(p, bs + 288);
			unsigned target2 = rd8(p, bs + 290);
			if (!move0 || !move2 || move0 == move2 || slot0 < 0 || slot2 < 0)
				snprintf(why, sizeof(why), "moves %u/%u slots %d/%d", move0, move2, slot0, slot2);
			else if (slot0 == slot2)
				snprintf(why, sizeof(why), "both used slot %d (moves %u and %u)", slot0, move0, move2);
			else if (target0 > 3 || target2 > 3)
				snprintf(why, sizeof(why), "targets %u and %u", target0, target2);
			else
				out("[p%d] ok slots %d,%d moves %u,%u targets %u,%u\n", p->id, slot0, slot2, move0, move2, target0, target2);
		}
	}
	if (why[0]) {
		out("[p%d] FAIL battle: %s\n", p->id, why);
		dump(p, "fail");
		sFailures++;
	}
}

static void dump(struct Player* p, const char* tag) {
	unsigned cb1 = rd32(p, sMain), cb2 = rd32(p, sMain + 4);
	unsigned objId = rd8(p, sAvatar + 5);
	unsigned obj = sObjEvents + objId * 0x24;
	unsigned save1 = rd32(p, sSave1);
	int px = (int16_t)rd16(p, obj + 0x10), py = (int16_t)rd16(p, obj + 0x12);
	int mg = (int8_t)rd8(p, save1 + 4), mn = (int8_t)rd8(p, save1 + 5);
	char partner[64] = "none";
	for (int i = 0; i < 16; i++) {
		unsigned o = sObjEvents + i * 0x24;
		if ((rd8(p, o) & 1) && rd8(p, o + 8) == 0xFC) {
			snprintf(partner, sizeof(partner), "obj%d@(%d,%d)", i, (int16_t)rd16(p, o + 0x10), (int16_t)rd16(p, o + 0x12));
		}
	}
	unsigned remote = coopStatic("sPartner");
	out("[f%06d p%d %s] cb1=%s(%08x) cb2=%s(%08x) map=%d.%d pos=(%d,%d) partner=%s lock=%u script=%u battle=%08x linkErr=%u"
	    " linkId=%u remote=%u.%u@(%u,%u) q=%u/%u coop{state=%u req=%u wait=%u slot=%u kind=%u inBattle=%u valid=%u guest=%u hold=%u}\n",
	    p->frame, p->id, tag, symName(cb1), cb1, symName(cb2), cb2, mg, mn, px, py, partner, rd8(p, sLockCtl),
	    sScriptStatus ? rd8(p, sScriptStatus) : 0, rd32(p, sBattleFlags), rd8(p, sLinkErr),
	    rd8(p, symAddr("gLink") + 2),
	    rd8(p, remote + 2), rd8(p, remote + 3), rd8(p, remote + 4), rd8(p, remote + 5),
	    rd8(p, symAddr("gLink") + 0x339), rd8(p, symAddr("gLink") + 0x33c + 0xC81),
	    rd8(p, coopStatic("sLinkState")), rd8(p, coopStatic("sRequest")), rd8(p, coopStatic("sWaitKind")),
	    rd8(p, coopStatic("sSlot")), rd8(p, coopStatic("sBattleKind")), rd8(p, coopStatic("sInBattle")),
	    rd8(p, coopStatic("sPartnerValid")), rd8(p, coopStatic("sIsGuest")), rd8(p, symAddr("gCoopHoldLink")));
	if ((cb2 & ~1u) == (symFind("CB1_Overworld") & ~1u))
		printCb2Trace(p);
}

static void battleDump(struct Player* p) {
	char ctl[4][64];
	unsigned funcs = symAddr("gBattlerControllerFuncs");
	for (int i = 0; i < 4; i++)
		snprintf(ctl[i], sizeof(ctl[i]), "%s", symName(rd32(p, funcs + i * 4)));
	out("[f%06d p%d battle] main=%s exec=%08x comm=%02x %02x %02x %02x ctl={%s, %s, %s, %s} linkState=%u multiId=%u\n",
	    p->frame, p->id, symName(rd32(p, symAddr("gBattleMainFunc"))), rd32(p, symAddr("gBattleControllerExecFlags")),
	    rd8(p, symAddr("gBattleCommunication")), rd8(p, symAddr("gBattleCommunication") + 1),
	    rd8(p, symAddr("gBattleCommunication") + 2), rd8(p, symAddr("gBattleCommunication") + 3),
	    ctl[0], ctl[1], ctl[2], ctl[3], rd8(p, symAddr("gLink") + 1), rd8(p, symAddr("gLink") + 2));
	{
		char instr[120];
		snprintf(instr, sizeof(instr), "%s", symAtAny(rd32(p, symAddr("gBattlescriptCurrInstr"))));
		out("[f%06d p%d bscript] instr=%s action=%u turnAction=%u/%u attacker=%u target=%u\n", p->frame, p->id, instr,
		    rd8(p, symAddr("gCurrentActionFuncId")), rd8(p, symAddr("gCurrentTurnActionNumber")), rd8(p, symAddr("gBattlersCount")),
		    rd8(p, symAddr("gBattlerAttacker")), rd8(p, symAddr("gBattlerTarget")));
	}
	unsigned ends = symAddr("gBattlerControllerEndFuncs");
	out("[f%06d p%d bend] vbl1=%u vbl2=%u cb1=%s count=%u end={%s, %s, %s, %s}\n", p->frame, p->id, rd32(p, sMain + 0x20), rd32(p, sMain + 0x24),
	    symName(rd32(p, sMain)), rd8(p, symAddr("gBattlersCount")),
	    symName(rd32(p, ends)), symName(rd32(p, ends + 4)), symName(rd32(p, ends + 8)), symName(rd32(p, ends + 12)));
	unsigned tasks = symAddr("gTasks");
	unsigned st = tasks + rd8(p, symAddr("battle_controllers.sLinkSendTaskId")) * 0x28;
	unsigned rt = tasks + rd8(p, symAddr("battle_controllers.sLinkReceiveTaskId")) * 0x28;
	unsigned bs = symAddr("link.sBlockSend");
	out("[f%06d p%d blink] send{state=%d start=%d end=%d delay=%d} recv{start=%d end=%d} linkCb=%s blockSend{active=%u pos=%u size=%u} blockRecv=%02x%02x linkq{send=%u recv=%u}\n",
	    p->frame, p->id, (int16_t)rd16(p, st + 8 + 22), (int16_t)rd16(p, st + 8 + 30), (int16_t)rd16(p, st + 8 + 28), (int16_t)rd16(p, st + 8 + 26),
	    (int16_t)rd16(p, rt + 8 + 30), (int16_t)rd16(p, rt + 8 + 28), symName(rd32(p, symAddr("gLinkCallback"))),
	    rd8(p, bs + 8), rd16(p, bs), rd16(p, bs + 2), rd8(p, symAddr("gBlockReceivedStatus")), rd8(p, symAddr("gBlockReceivedStatus") + 1),
	    rd8(p, symAddr("gLink") + 0x18 + 0x321), rd8(p, symAddr("gLink") + 0x33c + 0xc81));
}

static bool inField(struct Player* p) {
	static unsigned overworld;
	if (!overworld) overworld = symAddr("CB2_Overworld");
	if ((rd32(p, sMain + 4) & ~1u) != overworld) return false;
	if (rd8(p, sLockCtl)) return false;
	// 0 is running and 1 is waiting on a message. 2 is shut down, which is idle.
	if (sScriptStatus && rd8(p, sScriptStatus) != 2) return false;
	return true;
}

static int wantRecord(struct Player* p) {
	const char* all = getenv("RECORD_ALL");
	if (all && all[0] == '1') return 1;
	static unsigned battle;
	if (!battle) {
		battle = symFind("BattleMainCB2") & ~1u;
		if (!battle) battle = 1;
	}
	if (battle == 1) return 0;
	unsigned mine = rd32(p, sMain + 4) & ~1u;
	unsigned other = rd32(&P[1 - p->id], sMain + 4) & ~1u;
	return mine == battle || other == battle;
}

static void recordSide(struct Player* p) {
	if (!sRecord) return;
	pthread_mutex_lock(&sRecLock);
	memcpy(sSnap[p->id], p->video, sizeof(p->video));
	sSnapFrame[p->id] = p->frame;
	// The two cores do not finish a frame on the same count. Pair this
	// core's just-finished frame with the other core's latest finished frame.
	int ready = p->id == 0 && sSnapFrame[1] > 0;
	pthread_mutex_unlock(&sRecLock);
	if (!ready || !wantRecord(p)) return;
	enum { SIDE = W * 2 + 2 };
	static unsigned char* rgb;
	if (!rgb) rgb = malloc(SIDE * H * 3);
	for (int y = 0; y < H; y++) {
		unsigned char* row = rgb + y * SIDE * 3;
		memset(row, 0, SIDE * 3);
		for (int x = 0; x < W; x++) {
			uint32_t c = sSnap[0][y * W + x];
			row[x * 3] = c;
			row[x * 3 + 1] = c >> 8;
			row[x * 3 + 2] = c >> 16;
		}
		for (int x = 0; x < W; x++) {
			uint32_t c = sSnap[1][y * W + x];
			unsigned char* px = row + (W + 2 + x) * 3;
			px[0] = c;
			px[1] = c >> 8;
			px[2] = c >> 16;
		}
	}
	fwrite(rgb, 1, SIDE * H * 3, sRecord);
	sRecorded++;
}

static void frameCallback(struct mCoreThread* t) {
	struct Player* p = t->userData;
	p->frame++;
    // Controlled application-heartbeat outage: serial hardware keeps running.
    // Patch only this emulated core's callback, and always restore its opcode.
    if (p->muteFrames && --p->muteFrames == 0)
        p->core->rawWrite16(p->core, symAddr("Coop_LinkCallback"), -1, p->mutedOpcode);
	if (sMain) {
		unsigned cb2 = rd32(p, sMain + 4);
		if (cb2 != sLastCb2[p->id]) {
			sLastCb2[p->id] = cb2;
			if (sCb2Logs[p->id]++ < 30)
				out("[f%06d p%d] cb2 %08x %s\n", p->frame, p->id, cb2, symName(cb2));
		}
	}
	recordSide(p);
	if (p->frame > sMaxFrames && !p->done) {
		out("[p%d] global frame limit hit at pc=%d\n", p->id, p->pc);
		dump(p, "timeout");
		sFailures++;
		p->done = 1;
	}
    // Capture actual intro indicator data, not the party data used to build it.
    unsigned vsFunc = symFind("InitLinkBattleVsScreen");
    unsigned tasks = symFind("gTasks");
    if (vsFunc && tasks) for (unsigned i = 0; i < 16; i++) {
        unsigned task = tasks + i * 40;
        if (rd8(p, task + 4) && (rd32(p, task) & ~1u) == (vsFunc & ~1u))
            p->vsFlags = rd16(p, task + 14);
    }

	unsigned keys = 0;
	while (!p->done) {
		if (p->pc >= p->ncmds) { p->done = 1; break; }
		struct Cmd* c = &p->cmds[p->pc];
		bool advance = false;
		switch (c->kind) {
        case C_FORCE_ENC: {
            // FIXTURE: WildEncounterCheck returns TRUE (movs r0,#1; bx lr), so
            // the next grass step rolls an encounter. Off restores the code.
            unsigned addr = symAddr("WildEncounterCheck") & ~1u;
            if (c->value && !p->encForced) {
                p->encSaved[0] = rd16(p, addr);
                p->encSaved[1] = rd16(p, addr + 2);
                p->core->rawWrite16(p->core, addr, -1, 0x2001);
                p->core->rawWrite16(p->core, addr + 2, -1, 0x4770);
                p->encForced = 1;
            } else if (!c->value && p->encForced) {
                p->core->rawWrite16(p->core, addr, -1, p->encSaved[0]);
                p->core->rawWrite16(p->core, addr + 2, -1, p->encSaved[1]);
                p->encForced = 0;
            }
            advance = true;
            break;
        }
        case C_MUTE:
            if (!p->muteFrames) {
                unsigned addr = symAddr("Coop_LinkCallback");
                p->mutedOpcode = rd16(p, addr);
                p->core->rawWrite16(p->core, addr, -1, 0x4770); // bx lr
            }
            p->muteFrames = c->frames;
            advance = true;
            break;
		case C_PRESS:
			if (p->step < c->frames) keys = c->keys;
			if (++p->step >= c->frames + 6) advance = true;
			break;
		case C_WAIT:
			if (++p->step >= c->frames) advance = true;
			break;
		case C_SHOT:
			writePng(p, c->text);
			advance = true;
			break;
		case C_PARTNER: {
			// The other game's live tile, against the sprite this game draws for it.
			struct Player* o = &P[1 - p->id];
			unsigned oobj = sObjEvents + rd8(o, sAvatar + 5) * 0x24;
			int ox = (int16_t)rd16(o, oobj + 0x10), oy = (int16_t)rd16(o, oobj + 0x12);
			int omg = (int8_t)rd8(o, rd32(o, sSave1) + 4), omn = (int8_t)rd8(o, rd32(o, sSave1) + 5);
			int mg = (int8_t)rd8(p, rd32(p, sSave1) + 4), mn = (int8_t)rd8(p, rd32(p, sSave1) + 5);
			bool want = (omg == mg && omn == mn) == (c->value != 0);
			int found = -1, count = 0;
			for (int i = 0; i < 16; i++) {
				unsigned ob = sObjEvents + i * 0x24;
				if ((rd8(p, ob) & 1) && rd8(p, ob + 8) == 0xFC) { found = i; count++; }
			}
			char why[160] = "";
			if (count > 1) {
				snprintf(why, sizeof(why), "%d partner sprites", count);
			} else if (c->value == 0) {
				if (found >= 0) snprintf(why, sizeof(why), "partner sprite still shown");
			} else if (!want) {
				snprintf(why, sizeof(why), "players not on the same map (%d.%d vs %d.%d)", mg, mn, omg, omn);
			} else if (found < 0) {
				snprintf(why, sizeof(why), "no partner sprite");
			} else {
				unsigned ob = sObjEvents + found * 0x24;
				unsigned spr = symAddr("gSprites") + rd8(p, ob + 0x23) * 0x44;
				unsigned pobj = sObjEvents + rd8(p, sAvatar + 5) * 0x24;
				int lx = (int16_t)rd16(p, pobj + 0x10), ly = (int16_t)rd16(p, pobj + 0x12);
				int sx = (int16_t)rd16(p, ob + 0x10), sy = (int16_t)rd16(p, ob + 0x12);
				unsigned objFlags = rd16(p, ob), sprFlags = rd16(p, spr + 0x3E);
				// Coop_PartnerTarget: a shared tile is drawn on a free neighbouring tile.
				int expectX = ox, expectY = oy;
				// A shared tile is drawn on a free neighbour, south first.
				int shared = ox == lx && oy == ly;
				if (shared) expectY++;
				if (objFlags & (1u << 13)) snprintf(why, sizeof(why), "partner object invisible");
				else if (!(sprFlags & 1) || ((sprFlags & 4) && !(objFlags & (1u << 14)))) snprintf(why, sizeof(why), "partner sprite hidden (flags %04x)", sprFlags);
				else if (shared && (sx - lx) * (sx - lx) + (sy - ly) * (sy - ly) == 1)
					;
				else if (sx != expectX || sy != expectY)
					snprintf(why, sizeof(why), "partner at (%d,%d), expected (%d,%d)", sx, sy, expectX, expectY);
			}
			if (why[0]) {
				out("[p%d] FAIL %s: %s\n", p->id, c->text, why);
				dump(p, "fail");
				sFailures++;
			} else {
				out("[p%d] ok %s\n", p->id, c->text);
			}
			advance = true;
			break;
		}
		case C_PARTNER_AT: {
			struct Player* o = &P[1 - p->id];
			unsigned oobj = sObjEvents + rd8(o, sAvatar + 5) * 0x24;
			int ox = (int16_t)rd16(o, oobj + 0x10), oy = (int16_t)rd16(o, oobj + 0x12);
			int ex = ox + (int)c->addr, ey = oy + (int)c->value;
			int found = -1, count = 0;
			for (int i = 0; i < 16; i++) {
				unsigned ob = sObjEvents + i * 0x24;
				if ((rd8(p, ob) & 1) && rd8(p, ob + 8) == 0xFC) { found = i; count++; }
			}
			char why[160] = "";
			if (found < 0) snprintf(why, sizeof(why), "no partner sprite");
			else if (count > 1) snprintf(why, sizeof(why), "%d partner sprites", count);
			else {
				unsigned ob = sObjEvents + found * 0x24;
				unsigned spr = symAddr("gSprites") + rd8(p, ob + 0x23) * 0x44;
				int sx = (int16_t)rd16(p, ob + 0x10), sy = (int16_t)rd16(p, ob + 0x12);
				unsigned objFlags = rd16(p, ob), sprFlags = rd16(p, spr + 0x3E);
				unsigned save = rd32(p, sSave1);
				if (rd8(p, ob + 9) != rd8(p, save + 5) || rd8(p, ob + 10) != rd8(p, save + 4))
					snprintf(why, sizeof(why), "partner object tagged %u.%u, not this map", rd8(p, ob + 10), rd8(p, ob + 9));
				else if (objFlags & (1u << 13)) snprintf(why, sizeof(why), "partner object invisible");
				else if (!(sprFlags & 1) || (sprFlags & 4)) snprintf(why, sizeof(why), "partner sprite hidden (flags %04x)", sprFlags);
				else if (sx != ex || sy != ey) snprintf(why, sizeof(why), "partner at (%d,%d), expected (%d,%d)", sx, sy, ex, ey);
			}
			if (why[0]) {
				out("[p%d] FAIL %s: %s\n", p->id, c->text, why);
				dump(p, "fail");
				sFailures++;
			} else out("[p%d] ok %s\n", p->id, c->text);
			advance = true;
			break;
		}
		case C_GOTO: {
			unsigned obj = sObjEvents + rd8(p, sAvatar + 5) * 0x24;
			int x = (int16_t)rd16(p, obj + 0x10) - 7, y = (int16_t)rd16(p, obj + 0x12) - 7;
			int tx = (int)c->addr, ty = c->value;
			if (x == tx && y == ty) {
				// currentCoords flips at the start of the last step. Let it finish.
				if (++p->stable >= 16) { p->stable = 0; advance = true; }
				break;
			}
			p->stable = 0;
			keys = x < tx ? 1u << 4 : x > tx ? 1u << 5 : y > ty ? 1u << 6 : 1u << 7;
			if (++p->step >= c->frames) {
				out("[p%d] FAIL goto (%d,%d) stuck at (%d,%d)\n", p->id, tx, ty, x, y);
				dump(p, "fail");
				writePng(p, "fail");
				sFailures++;
				p->done = 1;
			}
			break;
		}
		case C_HEX: {
			char line[600]; int o = 0;
			for (int i = 0; i < c->size && o < 580; i++)
				o += snprintf(line + o, sizeof(line) - o, "%02x%s", rd8(p, c->addr + i), (i & 15) == 15 ? " | " : " ");
			out("[f%06d p%d hex %s] %s\n", p->frame, p->id, c->text, line);
			advance = true;
			break;
		}
		case C_DUMP:
			dump(p, c->text);
			if ((rd32(p, sMain + 4) & ~1u) == symAddr("BattleMainCB2")) battleDump(p);
			{
				struct ARMCore* cpu = ((struct GBA*)p->core->board)->cpu;
				char pc[120];
				snprintf(pc, sizeof(pc), "%s", symAt(cpu->gprs[15]));
				out("[f%06d p%d cpu] pc=%s lr=%s mode=%x\n", p->frame, p->id, pc, symAt(cpu->gprs[14]), cpu->cpsr.priv);
			}
			advance = true;
			break;
		case C_BARRIER:
			pthread_mutex_lock(&sLock);
			if (p->barrierReached < c->barrier) p->barrierReached = c->barrier;
			advance = P[1 - p->id].barrierReached >= c->barrier || P[1 - p->id].done;
			pthread_mutex_unlock(&sLock);
			break;
		case C_UNTIL_CB2:
		case C_UNTIL_FIELD:
		case C_UNTIL_MEM: {
			int nbytes = c->size < 0 ? -c->size : c->size;
			unsigned got = c->kind == C_UNTIL_MEM ? rdN(p, c->addr, nbytes) : 0;
			unsigned want = c->value;
			if ((want & 1u) && nbytes == 4) { got &= ~1u; want &= ~1u; }
			bool ok = c->kind == C_UNTIL_CB2 ? (rd32(p, sMain + 4) & ~1u) == c->addr
			        : c->kind == C_UNTIL_FIELD ? inField(p)
			        : c->size < 0 ? got != 0
			        : got == want;
			if (c->kind == C_UNTIL_FIELD) {
				p->stable = ok ? p->stable + 1 : 0;
				ok = p->stable >= 20;
			}
			if (ok) { p->stable = 0; advance = true; break; }
			if (c->keys && (p->step % 24) < 2) keys = c->keys;
			// mash_field in a battle's party menu (a forced switch after a
			// faint): step the cursor down before each A, so a fainted
			// first slot does not hold the masher on "no energy left".
			if (c->kind == C_UNTIL_FIELD && c->keys == 1 && symFind("CB2_UpdatePartyMenu")
			 && (rd32(p, sMain + 4) & ~1u) == (symFind("CB2_UpdatePartyMenu") & ~1u) && rd8(p, symAddr("gMain") + 0x439) & 2) {
				unsigned t = p->step % 120;
				keys = t < 2 ? 0x80 : ((t >= 40 && t < 42) || (t >= 90 && t < 92)) ? 1 : 0;
			}
			if (c->kind == C_UNTIL_FIELD && c->keys == 1 && symFind("MainCB2") && symFind("CB2_InitSummaryScreen")
			 && rd8(p, symAddr("gMain") + 0x439) & 2
			 && ((rd32(p, sMain + 4) & ~1u) == (symFind("MainCB2") & ~1u) || (rd32(p, sMain + 4) & ~1u) == (symFind("CB2_InitSummaryScreen") & ~1u))) {
				keys = (p->step % 24) < 2 ? 2 : 0; // B out of a summary screen
			}
			if (++p->step >= c->frames) {
				out("[p%d] FAIL timeout waiting for %s (line cmd %d)\n", p->id, c->text, p->pc);
				dump(p, "fail");
				if ((rd32(p, sMain + 4) & ~1u) == symAddr("BattleMainCB2")) battleDump(p);
				writePng(p, "fail");
				sFailures++;
				p->done = 1;
			}
			break;
		}
		case C_FIXTURE:
            fixtureLead(p, c->frames && p->id == 0 ? 2 : 0, c->addr, c->size, c->value);
            advance = true;
            break;
        case C_WRITE:
            if (c->size == 1) p->core->busWrite8(p->core, c->addr, c->value);
            else if (c->size == 2) p->core->busWrite16(p->core, c->addr, c->value);
            else p->core->busWrite32(p->core, c->addr, c->value);
            advance = true;
            break;
        case C_NO_OBJECT: {
            unsigned save = rd32(p, sSave1), found = 0;
            for (unsigned i = 0; i < 16; i++) {
                unsigned obj = sObjEvents + i * 0x24;
                if ((rd8(p, obj) & 1) && rd8(p, obj + 8) == c->value
                    && rd8(p, obj + 9) == rd8(p, save + 5) && rd8(p, obj + 10) == rd8(p, save + 4)) found++;
            }
            if (found) { out("[p%d] FAIL story NPC %u still visible\n", p->id, c->value); sFailures++; }
            else out("[p%d] ok story NPC %u removed\n", p->id, c->value);
            advance = true;
            break;
        }
        case C_PACE: {
            if ((rd32(p, sMain + 4) & ~1u) == (symAddr("BattleMainCB2") & ~1u)) { advance = true; break; }
            // pace_until_mem: stop when the value matches instead.
            if (c->addr && ((c->size == 1 ? rd8(p, c->addr) : c->size == 2 ? rd16(p, c->addr) : rd32(p, c->addr)) == c->value)) { advance = true; break; }
            keys = ((p->step / 24) & 1) ? (1u << 7) : (1u << 6);
            if (++p->step >= c->frames) {
                out("[p%d] FAIL no battle while pacing\n", p->id);
                dump(p, "fail");
                sFailures++;
                p->done = 1;
            }
            break;
        }
        case C_TEXT_SPEED: {
            unsigned addr = rd32(p, symAddr("gSaveBlock2Ptr")) + 0x14;
            p->core->busWrite16(p->core, addr, (rd16(p, addr) & ~7u) | (c->value & 7u));
            out("[p%d] text speed %u\n", p->id, c->value);
            advance = true;
            break;
        }
        case C_MAP: {
            unsigned save = rd32(p, sSave1);
            unsigned mg = rd8(p, save + 4), mn = rd8(p, save + 5);
            if (mg != c->addr || mn != c->value) {
                out("[p%d] FAIL map %u.%u expected %u.%u\n", p->id, mg, mn, c->addr, c->value);
                dump(p, "fail");
                sFailures++;
            } else out("[p%d] ok map %u.%u\n", p->id, mg, mn);
            advance = true;
            break;
        }
        case C_OBJECT: {
            unsigned save = rd32(p, sSave1);
            int found = 0, fx = 0, fy = 0;
            for (unsigned i = 0; i < 16; i++) {
                unsigned obj = sObjEvents + i * 0x24;
                if ((rd8(p, obj) & 1) && rd8(p, obj + 8) == c->value
                    && rd8(p, obj + 9) == rd8(p, save + 5) && rd8(p, obj + 10) == rd8(p, save + 4)) {
                    found = 1;
                    fx = (int16_t)rd16(p, obj + 0x10) - 7;
                    fy = (int16_t)rd16(p, obj + 0x12) - 7;
                }
            }
            int anywhere = c->addr == 0xFFFF;
            if (!found || (!anywhere && (fx != (int)c->addr || fy != c->size))) {
                out("[p%d] FAIL object %u at (%d,%d) found=%d, expected (%u,%d)\n", p->id, c->value, fx, fy, found, c->addr, c->size);
                sFailures++;
            } else out("[p%d] ok object %u at (%d,%d)\n", p->id, c->value, fx, fy);
            advance = true;
            break;
        }
        case C_VS: {
            unsigned counts[2] = {0, 0};
            for (unsigned side = 0; side < 2; side++)
                for (unsigned slot = 0; slot < 3; slot++)
                    counts[side] += ((p->vsFlags >> (side * 6 + slot * 2)) & 3) != 0;
            if (counts[0] != c->addr || counts[1] != c->value) {
                out("[p%d] FAIL intro party indicators %u/%u expected %u/%u\n", p->id, counts[0], counts[1], c->addr, c->value); sFailures++;
            } else out("[p%d] ok intro party indicators %u/%u\n", p->id, counts[0], counts[1]);
            advance = true;
            break;
        }
        case C_EXPECT_VAR:
        case C_EXPECT_FLAG: {
            unsigned value;
            if (c->kind == C_EXPECT_VAR)
                value = rd16(p, rd32(p, sSave1) + symValue("harness.saveVarsOffset") + (c->addr - 0x4000) * 2);
            else
                value = (rd8(p, rd32(p, sSave1) + symValue("harness.saveFlagsOffset") + c->addr / 8) >> (c->addr % 8)) & 1;
            if (value != c->value) { out("[p%d] FAIL saved state %x=%u expected %u\n", p->id, c->addr, value, c->value); sFailures++; }
            else out("[p%d] ok saved state %x=%u\n", p->id, c->addr, value);
            advance = true;
            break;
        }
        case C_VAR:
            p->core->busWrite16(p->core, rd32(p, sSave1) + symValue("harness.saveVarsOffset") + (c->addr - 0x4000) * 2, c->value);
            advance = true;
            break;
        case C_FLAG: {
            unsigned addr = rd32(p, sSave1) + symValue("harness.saveFlagsOffset") + c->addr / 8;
            unsigned mask = 1u << (c->addr % 8), value = rd8(p, addr);
            p->core->busWrite8(p->core, addr, c->value ? value | mask : value & ~mask);
            advance = true;
            break;
        }
        case C_MONS: {
            // Every party and PC Pokemon decrypts with a matching checksum and
            // a real species (no bad eggs); count how many are SPECIES.
            unsigned storage = rd32(p, symAddr("gPokemonStoragePtr"));
            unsigned boxes = storage + symValue("harness.storageBoxesOffset"), size = symValue("harness.boxMonSize");
            unsigned bad = 0, total = 0, match = 0, badAt = 0;
            for (unsigned i = 0; i < 6 + 14 * 30; i++) {
                unsigned mon = i < 6 ? partyMon(0, i) : boxes + (i - 6) * size;
                unsigned flags = rd8(p, mon + 19);
                if (!(flags & 2)) continue;
                unsigned pid = rd32(p, mon), key = pid ^ rd32(p, mon + 4), sum = 0, w[12];
                for (int k = 0; k < 12; k++) { w[k] = rd32(p, mon + 32 + k * 4) ^ key; sum += (w[k] & 0xFFFF) + (w[k] >> 16); }
                unsigned species = w[sSub0[pid % 24] * 3] & 0x7FF;
                total++;
                if ((sum & 0xFFFF) != rd16(p, mon + 28) || (flags & 1) || species == 0 || species >= 1600) { bad++; badAt = i; }
                if (species == c->value) match++;
            }
            if (bad) { out("[p%d] FAIL %u of %u Pokemon corrupt (first at slot %u)\n", p->id, bad, total, badAt); sFailures++; }
            else if (c->size >= 0 && match != (unsigned)c->size) { out("[p%d] FAIL species %u count %u, expected %d\n", p->id, c->value, match, c->size); sFailures++; }
            else out("[p%d] ok %u Pokemon intact, species %u x%u\n", p->id, total, c->value, match);
            advance = true;
            break;
        }
        case C_NOTE_MONEY:
        case C_MONEY_GAIN: {
            unsigned key = rd32(p, rd32(p, symAddr("gSaveBlock2Ptr")) + symValue("harness.encryptionKeyOffset"));
            unsigned money = rd32(p, rd32(p, sSave1) + symValue("harness.moneyOffset")) ^ key;
            if (c->kind == C_NOTE_MONEY) {
                p->moneyBase = money;
                out("[p%d] money %u\n", p->id, money);
            } else {
                struct Player* o = &P[1 - p->id];
                unsigned okey = rd32(o, rd32(o, symAddr("gSaveBlock2Ptr")) + symValue("harness.encryptionKeyOffset"));
                unsigned omoney = rd32(o, rd32(o, sSave1) + symValue("harness.moneyOffset")) ^ okey;
                int mine = (int)(money - p->moneyBase), theirs = (int)(omoney - o->moneyBase);
                if (mine <= 0 || mine != theirs) {
                    out("[p%d] FAIL prize: this save +%d, other save +%d\n", p->id, mine, theirs);
                    sFailures++;
                } else out("[p%d] ok prize +%d on both saves\n", p->id, mine);
            }
            advance = true;
            break;
        }
        case C_NO_COPIES: {
            // No Pokemon on this save (party or PC) shares a personality value
            // with one on the other save: nothing was duplicated across.
            struct Player* o = &P[1 - p->id];
            unsigned shared = 0, mine = 0;
            for (unsigned i = 0; i < 6 + 14 * 30; i++) {
                unsigned sm = rd32(p, symAddr("gPokemonStoragePtr")) + symValue("harness.storageBoxesOffset");
                unsigned a = i < 6 ? partyMon(0, i) : sm + (i - 6) * symValue("harness.boxMonSize");
                if (!(rd8(p, a + 19) & 2)) continue;
                mine++;
                unsigned pid = rd32(p, a);
                for (unsigned j = 0; j < 6 + 14 * 30; j++) {
                    unsigned so = rd32(o, symAddr("gPokemonStoragePtr")) + symValue("harness.storageBoxesOffset");
                    unsigned b = j < 6 ? partyMon(0, j) : so + (j - 6) * symValue("harness.boxMonSize");
                    if ((rd8(o, b + 19) & 2) && rd32(o, b) == pid) { shared++; break; }
                }
            }
            if (shared) { out("[p%d] FAIL %u of %u Pokemon also on the other save\n", p->id, shared, mine); sFailures++; }
            else out("[p%d] ok none of %u Pokemon copied from the other save\n", p->id, mine);
            advance = true;
            break;
        }
        case C_PC_COUNT: {
            // Occupied PC slots across all boxes (BoxPokemon.hasSpecies).
            unsigned storage = rd32(p, symAddr("gPokemonStoragePtr")), count = 0;
            unsigned boxes = storage + symValue("harness.storageBoxesOffset"), size = symValue("harness.boxMonSize");
            for (unsigned i = 0; i < 14 * 30; i++)
                if (rd8(p, boxes + i * size + 19) & 2) count++;
            if (count != c->value) {
                out("[p%d] FAIL pc count %u, expected %u\n", p->id, count, c->value);
                sFailures++;
            } else out("[p%d] ok pc count %u\n", p->id, count);
            advance = true;
            break;
        }
        case C_WARP: {
            // Fixture teleport: the engine's own map load to a tile (no warp
            // event). Landing on water starts surfing, as a Dive exit would.
            unsigned dest = symAddr("overworld.sWarpDestination");
            p->core->busWrite8(p->core, dest, c->addr >> 8);
            p->core->busWrite8(p->core, dest + 1, c->addr & 0xFF);
            p->core->busWrite8(p->core, dest + 2, 0xFF);
            p->core->busWrite16(p->core, dest + 4, c->value >> 16);
            p->core->busWrite16(p->core, dest + 6, c->value & 0xFFFF);
            // What WarpIntoMap would have done: the save's location and pos.
            {
                unsigned save = rd32(p, sSave1);
                p->core->busWrite16(p->core, save + 0, c->value >> 16);
                p->core->busWrite16(p->core, save + 2, c->value & 0xFFFF);
                p->core->busWrite8(p->core, save + 4, c->addr >> 8);
                p->core->busWrite8(p->core, save + 5, c->addr & 0xFF);
                p->core->busWrite8(p->core, save + 6, 0xFF);
                p->core->busWrite16(p->core, save + 8, c->value >> 16);
                p->core->busWrite16(p->core, save + 10, c->value & 0xFFFF);
            }
            p->core->busWrite32(p->core, sMain + 4, symAddr("CB2_LoadMap") | 1);
            p->core->busWrite8(p->core, sMain + 0x438, 0);
            out("[p%d] fixture warp to %u.%u (%u,%u)\n", p->id, c->addr >> 8, c->addr & 0xFF, c->value >> 16, c->value & 0xFFFF);
            advance = true;
            break;
        }
        case C_UNTIL_TEXT: {
            // Wait for a battle message containing the text (spaces as '_').
            char text[440];
            gameText(p, symAddr("gDisplayedStringBattle"), text, sizeof(text));
            if (strstr(text, c->text + 16)) {
                out("[p%d] ok battle text \"%s\"\n", p->id, text);
                advance = true;
            } else if (++p->step >= c->frames) {
                out("[p%d] FAIL battle text never had \"%s\" (last \"%s\")\n", p->id, c->text + 16, text);
                dump(p, "fail");
                sFailures++;
                advance = true;
            }
            break;
        }
        case C_ITEM:
        case C_EXPECT_ITEM: {
            // gBagPockets[pocket] = { ItemSlot *slots; u8 capacity; u8 id; }.
            // Quantities in the bag are XORed with the save's encryption key.
            unsigned pocket = symAddr("gBagPockets") + c->size * 8;
            unsigned slots = rd32(p, pocket), cap = rd16(p, pocket + 4) & 0x3FF;
            unsigned key = rd16(p, rd32(p, symAddr("gSaveBlock2Ptr")) + symValue("harness.encryptionKeyOffset"));
            unsigned i, have = 0, freeSlot = 0xFFFF;
            for (i = 0; i < cap; i++) {
                unsigned id = rd16(p, slots + i * 4);
                if (id == c->addr) { have = rd16(p, slots + i * 4 + 2) ^ key; freeSlot = i; break; }
                if (id == 0 && freeSlot == 0xFFFF) freeSlot = i;
            }
            if (c->kind == C_ITEM) {
                if (freeSlot == 0xFFFF) { out("[p%d] FAIL fixture_item: pocket full\n", p->id); sFailures++; }
                else {
                    p->core->busWrite16(p->core, slots + freeSlot * 4, c->addr);
                    p->core->busWrite16(p->core, slots + freeSlot * 4 + 2, (c->value ^ key) & 0xFFFF);
                    out("[p%d] fixture item %u x%u in pocket %d\n", p->id, c->addr, c->value, c->size);
                }
            } else if (have != c->value) {
                out("[p%d] FAIL item %u: have %u, expected %u\n", p->id, c->addr, have, c->value);
                sFailures++;
            } else out("[p%d] ok item %u x%u\n", p->id, c->addr, have);
            advance = true;
            break;
        }
        case C_SIX:
            for (unsigned slot = 1; slot < 6; slot++)
                for (unsigned off = 0; off < 100; off++)
                    p->core->busWrite8(p->core, partyMon(0, slot) + off, rd8(p, partyMon(0, 0) + off));
            p->core->busWrite8(p->core, symAddr("gPartiesCount"), 6);
            advance = true;
            break;
        case C_MOVE: {
            unsigned i;
            for (i = 0; i < 4; i++) if (leadMove(p, i) == c->value) break;
            if ((i < 4) != (c->size != 0)) { out("[p%d] FAIL move %u present=%u\n", p->id, c->value, i < 4); sFailures++; }
            else out("[p%d] ok move %u present=%u\n", p->id, c->value, i < 4);
            advance = true;
            break;
        }
        case C_LEAD: {
            unsigned species, exp, held;
            readLead(p, &species, &exp, &held);
            if (species != c->addr || rd8(p, partyMon(0, 0) + 84) != c->size) {
                out("[p%d] FAIL lead species=%u level=%u\n", p->id, species, rd8(p, partyMon(0, 0) + 84));
                sFailures++;
            } else out("[p%d] ok lead species=%u level=%u exp=%u\n", p->id, species, c->size, exp);
            advance = true;
            break;
        }
		case C_PARTY:
			showParty(p);
			showPartySpecies(p);
			showMonAt(p, 0, 0);
			showMonAt(p, 1, 0);
			showMonAt(p, 1, 1);
			showMonAddr(p, coopStatic("sCatches"), 9, 0);
			showMonAddr(p, coopStatic("sCatches") + 100, 9, 1);
			advance = true;
			break;
		case C_BATTLE:
			checkBattle(p, c->value);
			advance = true;
			break;
		case C_MENU: {
			unsigned actions = symAddr("start_menu.sCurrentStartMenuActions");
			unsigned n = rd8(p, symAddr("start_menu.sNumStartMenuActions"));
			unsigned cur = rd8(p, symAddr("start_menu.sStartMenuCursorPos"));
			int target = -1;
			for (unsigned i = 0; i < n && i < 9; i++)
				if (rd8(p, actions + i) == c->value) target = (int)i;
			if (n == 0 || target < 0) {
				if (++p->step > 90) {
					out("[p%d] FAIL menu has no action %u\n", p->id, c->value);
					dump(p, "fail");
					sFailures++;
					p->done = 1;
				}
				break;
			}
			if ((int)cur == target) { advance = true; break; }
			if ((p->step % 16) < 2)
				keys = target > (int)cur ? (1u << 7) : (1u << 6);
			if (++p->step > 16 * 10) {
				out("[p%d] FAIL menu cursor stuck at %u, want %d\n", p->id, cur, target);
				dump(p, "fail");
				sFailures++;
				p->done = 1;
			}
			break;
		}
		case C_EXPECT_RANGE: {
			unsigned v = rdN(p, c->addr, c->size);
			if (v < c->value || v > (unsigned)c->frames) {
				out("[p%d] FAIL expect %s: got %u\n", p->id, c->text, v);
				dump(p, "fail");
				sFailures++;
			} else {
				out("[p%d] ok %s (%u)\n", p->id, c->text, v);
			}
			advance = true;
			break;
		}
		case C_EXPECT: {
			int nbytes = c->size < 0 ? -c->size : c->size;
			unsigned v = rdN(p, c->addr, nbytes);
			if ((c->value & 1u) && nbytes == 4) v &= ~1u;
			unsigned expect = (c->value & 1u) && nbytes == 4 ? c->value & ~1u : c->value;
			if (c->size < 0) {
				if (!v) {
					out("[p%d] FAIL expect %s: got 0\n", p->id, c->text);
					dump(p, "fail");
					sFailures++;
				} else {
					out("[p%d] ok %s (%u)\n", p->id, c->text, v);
				}
				advance = true;
				break;
			}
			if (v != expect) {
				out("[p%d] FAIL expect %s: got %u want %u\n", p->id, c->text, v, expect);
				dump(p, "fail");
				sFailures++;
			} else {
				out("[p%d] ok %s\n", p->id, c->text);
			}
			advance = true;
			break;
		}
		}
		if (!advance) break;
		p->pc++;
		p->step = 0;
		if (c->kind == C_PRESS || c->kind == C_WAIT) break;
	}
	p->core->setKeys(p->core, keys);
}

static unsigned parseKeys(const char* s) {
	static const char* names[] = { "A", "B", "SELECT", "START", "RIGHT", "LEFT", "UP", "DOWN", "R", "L" };
	unsigned k = 0;
	char buf[96];
	strncpy(buf, s, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = 0;
	for (char* tok = strtok(buf, "+"); tok; tok = strtok(NULL, "+")) {
		bool found = false;
		for (int i = 0; i < 10; i++)
			if (!strcasecmp(tok, names[i])) { k |= 1u << i; found = true; }
		if (!found && strcasecmp(tok, "NONE")) { fprintf(stderr, "bad key %s\n", tok); exit(1); }
	}
	return k;
}

static void addCmd(int who, struct Cmd c) {
	for (int i = 0; i < 2; i++) {
		if (who == i || who == 2) {
			if (P[i].ncmds >= MAX_CMDS) { fprintf(stderr, "script too long\n"); exit(1); }
			P[i].cmds[P[i].ncmds++] = c;
		}
	}
}

static int sBarrierCount;

static void loadScript(const char* path, int depth) {
	FILE* f = fopen(path, "r");
	char line[256];
	if (!f || depth > 8) { fprintf(stderr, "cannot read script %s\n", path); exit(1); }
	while (fgets(line, sizeof(line), f)) {
		char* hash = strchr(line, '#');
		if (hash) *hash = 0;
		char who[8], cmd[32], a[96] = "", b[96] = "", d[96] = "", e[96] = "";
		int n = sscanf(line, "%7s %31s %95s %95s %95s %95s", who, cmd, a, b, d, e);
		if (n < 1) continue;
		if (!strcmp(who, "include")) {
			char inc[512], rel[256];
			const char* slash = strrchr(path, '/');
			if (sscanf(line, "%*s %255s", rel) != 1) continue;
			if (rel[0] == '/')
				snprintf(inc, sizeof(inc), "%s", rel);
			else
				snprintf(inc, sizeof(inc), "%.*s%s", slash ? (int)(slash - path + 1) : 0, path, rel);
			loadScript(inc, depth + 1);
			continue;
		}
		if (n < 2) continue;
		int w = !strcmp(who, "0") ? 0 : !strcmp(who, "1") ? 1 : !strcmp(who, "b") ? 2 : -1;
		if (w < 0) { fprintf(stderr, "bad player %s\n", who); exit(1); }
		struct Cmd c = { 0 };
		if (w == 2 && strcmp(cmd, "press") && strcmp(cmd, "wait")) {
			struct Cmd bar = { .kind = C_BARRIER, .barrier = ++sBarrierCount };
			addCmd(2, bar);
		}
		if (!strcmp(cmd, "press")) {
			c.kind = C_PRESS; c.keys = parseKeys(a); c.frames = n >= 4 ? atoi(b) : 2;
			int repeat = n >= 5 ? atoi(d) : 1;
			for (int r = 0; r < repeat; r++) addCmd(w, c);
			continue;
		} else if (!strcmp(cmd, "wait")) {
			c.kind = C_WAIT; c.frames = atoi(a);
		} else if (!strcmp(cmd, "shot")) {
			c.kind = C_SHOT; strncpy(c.text, a, sizeof(c.text) - 1);
		} else if (!strcmp(cmd, "expect_partner") || !strcmp(cmd, "expect_no_partner")) {
			c.kind = C_PARTNER; c.value = strcmp(cmd, "expect_partner") == 0;
			snprintf(c.text, sizeof(c.text), "%s %s", cmd, n >= 3 ? a : "");
			if (w == 2) {
				// Both stand still for the whole check: barrier, check, barrier.
				addCmd(w, c);
				struct Cmd bar = { .kind = C_BARRIER, .barrier = ++sBarrierCount };
				addCmd(2, bar);
				continue;
			}
		} else if (!strcmp(cmd, "expect_partner_at")) {
			// The partner drawn at the other player's own tile shifted by DX,DY:
			// the connection offset into this map's grid, taken from the map data.
			c.kind = C_PARTNER_AT; c.addr = (unsigned)atoi(a); c.value = (unsigned)atoi(b);
			snprintf(c.text, sizeof(c.text), "expect_partner_at %s %s %s", a, b, n >= 5 ? d : "");
			if (w == 2) {
				addCmd(w, c);
				struct Cmd bar = { .kind = C_BARRIER, .barrier = ++sBarrierCount };
				addCmd(2, bar);
				continue;
			}
		} else if (!strcmp(cmd, "goto")) {
			c.kind = C_GOTO; c.addr = atoi(a); c.value = atoi(b); c.frames = n >= 5 ? atoi(d) : 600;
		} else if (!strcmp(cmd, "hex")) {
			c.kind = C_HEX; c.addr = symAddr(a) + (n >= 5 ? strtoul(d, NULL, 0) : 0); c.size = n >= 4 ? atoi(b) : 16;
			strncpy(c.text, a, sizeof(c.text) - 1);
		} else if (!strcmp(cmd, "dump")) {
			c.kind = C_DUMP; strncpy(c.text, n >= 3 ? a : "-", sizeof(c.text) - 1);
		} else if (!strcmp(cmd, "sync")) {
			if (w != 2) { fprintf(stderr, "sync needs b\n"); exit(1); }
			continue;
		} else if (!strcmp(cmd, "until_cb2")) {
			c.kind = C_UNTIL_CB2; c.addr = symAddr(a); c.frames = n >= 4 ? atoi(b) : 3000;
			if (n >= 5) c.keys = parseKeys(d);
			snprintf(c.text, sizeof(c.text), "cb2 %s", a);
		} else if (!strcmp(cmd, "until_field") || !strcmp(cmd, "mash_field")) {
			c.kind = C_UNTIL_FIELD; c.frames = n >= 3 ? atoi(a) : 3000;
			if (cmd[0] == 'm') c.keys = n >= 4 ? parseKeys(b) : 1;
			strcpy(c.text, "field control");
		} else if (!strcmp(cmd, "until_nz") || !strcmp(cmd, "expect_nz")) {
			char sym[96]; unsigned off = 0; int size = 1;
			strncpy(sym, a, sizeof(sym) - 1);
			char* colon = strchr(sym, ':');
			if (colon) { size = atoi(colon + 1); *colon = 0; }
			char* plus = strchr(sym, '+');
			if (plus) { off = strtoul(plus + 1, NULL, 0); *plus = 0; }
			c.kind = cmd[0] == 'u' ? C_UNTIL_MEM : C_EXPECT;
			c.addr = symAddr(sym) + off;
			c.size = -size;
			c.frames = 4000;
			if (n >= 4 && isdigit((unsigned char)b[0])) c.frames = atoi(b);
			else if (n >= 4) c.keys = parseKeys(b);
			snprintf(c.text, sizeof(c.text), "%s != 0", a);
        } else if (!strcmp(cmd, "fixture_lead") || !strcmp(cmd, "fixture_guest")) {
            c.kind = C_FIXTURE;
            c.frames = !strcmp(cmd, "fixture_guest");
            c.addr = strtoul(a, NULL, 0);
            c.size = atoi(b);
            c.value = strtoul(d, NULL, 0);
        } else if (!strcmp(cmd, "expect_no_object")) {
            c.kind = C_NO_OBJECT;
            c.value = strtoul(a, NULL, 0);
        } else if (!strcmp(cmd, "force_encounter")) {
            c.kind = C_FORCE_ENC;
            c.value = strtoul(a, NULL, 0);
        } else if (!strcmp(cmd, "mute_coop")) {
            c.kind = C_MUTE;
            c.frames = atoi(a);
        } else if (!strcmp(cmd, "pace_until_mem")) {
            // pace_until_mem SYM[:size] VALUE [timeout]
            char sym[96]; unsigned off = 0; int size = 1;
            snprintf(sym, sizeof(sym), "%s", a);
            char* colon = strchr(sym, ':');
            if (colon) { size = atoi(colon + 1); *colon = 0; }
            char* plus = strchr(sym, '+');
            if (plus) { off = strtoul(plus + 1, NULL, 0); *plus = 0; }
            c.kind = C_PACE;
            c.addr = symAddr(sym) + off;
            c.size = size;
            c.value = strtoul(b, NULL, 0);
            c.frames = n >= 5 ? atoi(d) : 6000;
        } else if (!strcmp(cmd, "pace_until_battle")) {
            // Walk up and down in place (grass) until a battle starts.
            c.kind = C_PACE;
            c.frames = n >= 3 ? atoi(a) : 6000;
        } else if (!strcmp(cmd, "text_speed")) {
            // 0 slow, 1 mid, 2 fast. A player setting, not story state.
            c.kind = C_TEXT_SPEED;
            c.value = strtoul(a, NULL, 0);
        } else if (!strcmp(cmd, "expect_map")) {
            c.kind = C_MAP;
            c.addr = strtoul(a, NULL, 0);
            c.value = strtoul(b, NULL, 0);
        } else if (!strcmp(cmd, "expect_object")) {
            // LOCAL_ID X Y in map-local tiles, on the current map.
            c.kind = C_OBJECT;
            c.value = strtoul(a, NULL, 0);
            c.addr = strtoul(b, NULL, 0);
            c.size = atoi(d);
        } else if (!strcmp(cmd, "expect_any_object")) {
            // LOCAL_ID present somewhere on the current map.
            c.kind = C_OBJECT;
            c.value = strtoul(a, NULL, 0);
            c.addr = 0xFFFF;
            c.size = -1;
        } else if (!strcmp(cmd, "expect_vs_party") || !strcmp(cmd, "expect_flag") || !strcmp(cmd, "expect_var")) {
            c.kind = !strcmp(cmd, "expect_vs_party") ? C_VS : !strcmp(cmd, "expect_flag") ? C_EXPECT_FLAG : C_EXPECT_VAR;
            c.addr = strtoul(a, NULL, 0);
            c.value = strtoul(b, NULL, 0);
        } else if (!strcmp(cmd, "fixture_var") || !strcmp(cmd, "fixture_flag")) {
            c.kind = !strcmp(cmd, "fixture_var") ? C_VAR : C_FLAG;
            c.addr = strtoul(a, NULL, 0);
            c.value = strtoul(b, NULL, 0);
        } else if (!strcmp(cmd, "note_money")) {
            c.kind = C_NOTE_MONEY;
        } else if (!strcmp(cmd, "expect_same_prize")) {
            c.kind = C_MONEY_GAIN;
        } else if (!strcmp(cmd, "expect_no_copies")) {
            c.kind = C_NO_COPIES;
        } else if (!strcmp(cmd, "expect_mons_intact")) {
            // expect_mons_intact [SPECIES COUNT]
            c.kind = C_MONS;
            c.value = n >= 3 ? strtoul(a, NULL, 0) : 0xFFFF;
            c.size = n >= 4 ? atoi(b) : -1;
        } else if (!strcmp(cmd, "expect_pc_count")) {
            c.kind = C_PC_COUNT;
            c.value = strtoul(a, NULL, 0);
        } else if (!strcmp(cmd, "fixture_warp")) {
            // fixture_warp GROUP NUM X Y
            c.kind = C_WARP;
            c.addr = (atoi(a) << 8) | (atoi(b) & 0xFF);
            c.value = ((unsigned)atoi(d) << 16) | ((unsigned)atoi(e) & 0xFFFF);
        } else if (!strcmp(cmd, "until_battle_text")) {
            // until_battle_text Some_words_here [timeout]
            c.kind = C_UNTIL_TEXT;
            c.frames = n >= 4 ? atoi(b) : 3000;
            snprintf(c.text, sizeof(c.text), "until_battle_text%s", "");
            snprintf(c.text + 16, sizeof(c.text) - 16, "%s", a);
            for (char* q = c.text + 16; *q; q++) if (*q == '_') *q = ' ';
        } else if (!strcmp(cmd, "fixture_item") || !strcmp(cmd, "expect_item")) {
            // POCKET (0 items, 1 balls, 2 TM/HM, 3 berries, 4 key) ITEM QTY
            c.kind = !strcmp(cmd, "fixture_item") ? C_ITEM : C_EXPECT_ITEM;
            c.size = atoi(a);
            c.addr = strtoul(b, NULL, 0);
            c.value = strtoul(d, NULL, 0);
        } else if (!strcmp(cmd, "fixture_six")) {
            c.kind = C_SIX;
        } else if (!strcmp(cmd, "expect_move") || !strcmp(cmd, "expect_no_move")) {
            c.kind = C_MOVE;
            c.size = !strcmp(cmd, "expect_move");
            c.value = strtoul(a, NULL, 0);
        } else if (!strcmp(cmd, "expect_lead")) {
            c.kind = C_LEAD;
            c.addr = strtoul(a, NULL, 0);
            c.size = atoi(b);
		} else if (!strcmp(cmd, "party")) {
			c.kind = C_PARTY;
		} else if (!strcmp(cmd, "menu_to")) {
			c.kind = C_MENU;
			if (!strcmp(a, "SAVE")) c.value = 5;
			else if (!strcmp(a, "BAG")) c.value = 2;
			else if (!strcmp(a, "POKEMON")) c.value = 1;
			else if (!strcmp(a, "COOP")) c.value = 15;
			else { fprintf(stderr, "menu_to %s\n", a); exit(1); }
			snprintf(c.text, sizeof(c.text), "menu %s", a);
		} else if (!strcmp(cmd, "expect_battle")) {
			c.kind = C_BATTLE;
			c.value = n >= 3 && !strcmp(a, "moves") ? 1 : n >= 3 && !strcmp(a, "hp") ? 2 : 0;
			snprintf(c.text, sizeof(c.text), "battle %s", c.value ? "moves" : "roster");
		} else if (!strcmp(cmd, "expect_range")) {
			// expect_range SYM[+off]:size LOW HIGH (inclusive)
			char sym[96]; unsigned off = 0; int size = 1;
			strncpy(sym, a, sizeof(sym) - 1);
			char* colon = strchr(sym, ':');
			if (colon) { size = atoi(colon + 1); *colon = 0; }
			char* plus = strchr(sym, '+');
			if (plus) { off = strtoul(plus + 1, NULL, 0); *plus = 0; }
			c.kind = C_EXPECT_RANGE;
			c.addr = symAddr(sym) + off;
			c.size = size;
			c.value = symValue(b);
			c.frames = (int)symValue(d);
			snprintf(c.text, sizeof(c.text), "%s in %s..%s", a, b, d);
		} else if (!strcmp(cmd, "until_mem") || !strcmp(cmd, "expect") || !strcmp(cmd, "write")) {
			// until_mem SYM[+off]:size value [timeout]
			char sym[96]; unsigned off = 0; int size = 1;
			strncpy(sym, a, sizeof(sym) - 1);
			char* colon = strchr(sym, ':');
			if (colon) { size = atoi(colon + 1); *colon = 0; }
			char* plus = strchr(sym, '+');
			if (plus) { off = strtoul(plus + 1, NULL, 0); *plus = 0; }
			c.kind = !strcmp(cmd, "write") ? C_WRITE : !strcmp(cmd, "expect") ? C_EXPECT : C_UNTIL_MEM;
			c.addr = (!strncmp(sym, "coop.", 5) ? symAddr(sym) : symAddr(sym)) + off;
			c.size = size;
			c.value = symValue(b);
			c.frames = n >= 5 ? atoi(d) : 3000;
            if (n >= 6) c.keys = parseKeys(e);
			snprintf(c.text, sizeof(c.text), "%s == %s", a, b);
		} else {
			fprintf(stderr, "bad command %s\n", cmd);
			exit(1);
		}
		addCmd(w, c);
	}
	fclose(f);
}

// Lockstep glue, ported from mGBA 0.10.2 MultiplayerController for two players.
static void lsLock(struct mLockstep* l) { (void)l; pthread_mutex_lock(&sLock); }
static void lsUnlock(struct mLockstep* l) { (void)l; pthread_mutex_unlock(&sLock); }
static bool lsSignal(struct mLockstep* l, unsigned mask) {
	(void)l;
	struct Player* p = &P[0];
	p->waitMask &= ~mask;
	if (!p->waitMask && p->awake < 1) {
		mCoreThreadStopWaiting(&p->thread);
		p->awake = 1;
		return true;
	}
	return false;
}
static bool lsWait(struct mLockstep* l, unsigned mask) {
	(void)l;
	struct Player* p = &P[0];
	p->waitMask |= mask;
	if (p->awake > 0) {
		mCoreThreadWaitFromThread(&p->thread);
		p->awake = 0;
		return true;
	}
	return false;
}
static void lsAddCycles(struct mLockstep* l, int id, int32_t cycles) {
	(void)l;
	if (cycles < 0) abort();
	if (!id) {
		struct Player* p = &P[1];
		p->cyclesPosted += cycles;
		if (p->awake < 1) p->node.nextEvent += p->cyclesPosted;
		mCoreThreadStopWaiting(&p->thread);
		p->awake = 1;
	} else {
		P[id].cyclesPosted += cycles;
	}
}
static int32_t lsUseCycles(struct mLockstep* l, int id, int32_t cycles) {
	(void)l;
	struct Player* p = &P[id];
	p->cyclesPosted -= cycles;
	if (p->cyclesPosted <= 0) {
		mCoreThreadWaitFromThread(&p->thread);
		p->awake = 0;
	}
	return p->cyclesPosted;
}
static int32_t lsUnusedCycles(struct mLockstep* l, int id) { (void)l; return P[id].cyclesPosted; }
static void lsUnload(struct mLockstep* l, int id) {
	(void)l;
	if (id) {
		P[id].cyclesPosted = 0;
		struct Player* p = &P[0];
		p->waitMask &= ~(1 << id);
		if (!p->waitMask && p->awake < 1) {
			mCoreThreadStopWaiting(&p->thread);
			p->awake = 1;
		}
	} else {
		struct Player* p = &P[1];
		p->cyclesPosted += sLockstep.players[0]->eventDiff;
		if (p->awake < 1) {
			p->node.nextEvent += p->cyclesPosted;
			mCoreThreadStopWaiting(&p->thread);
			p->awake = 1;
		}
	}
}

static void quietLog(struct mLogger* l, int cat, enum mLogLevel lvl, const char* fmt, va_list args) {
	(void)l; (void)cat;
	if (lvl & (mLOG_FATAL)) vfprintf(stderr, fmt, args), fputc('\n', stderr);
}

int main(int argc, char** argv) {
	if (argc < 6) {
		fprintf(stderr, "usage: %s rom syms save0 save1 script [outdir] [maxframes]\n", argv[0]);
		return 2;
	}
	if (argc > 6) sOutDir = argv[6];
	if (argc > 7) sMaxFrames = atoi(argv[7]);
	static struct mLogger logger = { .log = quietLog };
	mLogSetDefaultLogger(&logger);
	loadSyms(argv[2]);
	sMain = symAddr("gMain");
	sObjEvents = symAddr("gObjectEvents");
	sAvatar = symAddr("gPlayerAvatar");
	sSave1 = symAddr("gSaveBlock1Ptr");
	sBattleFlags = symAddr("gBattleTypeFlags");
	sLockCtl = symAddr("sLockFieldControls");
	sLinkErr = symAddr("gLinkErrorOccurred");
	for (int i = 0; i < sNumSyms; i++)
		if (!strcmp(sSyms[i].name, "sGlobalScriptContextStatus")) sScriptStatus = sSyms[i].addr;
	(void)sCoopBss;
	char cmdline[600];
	snprintf(cmdline, sizeof(cmdline), "mkdir -p '%s'", sOutDir);
	if (system(cmdline)) return 1;

	for (int i = 0; i < 2; i++) P[i].id = i;
	loadScript(argv[5], 0);

	GBASIOLockstepInit(&sLockstep);
	sLockstep.d.lock = lsLock;
	sLockstep.d.unlock = lsUnlock;
	sLockstep.d.signal = lsSignal;
	sLockstep.d.wait = lsWait;
	sLockstep.d.addCycles = lsAddCycles;
	sLockstep.d.useCycles = lsUseCycles;
	sLockstep.d.unusedCycles = lsUnusedCycles;
	sLockstep.d.unload = lsUnload;

	for (int i = 0; i < 2; i++) {
		struct Player* p = &P[i];
		p->core = mCoreFind(argv[1]);
		if (!p->core || !p->core->init(p->core)) { fprintf(stderr, "core init failed\n"); return 1; }
		mCoreInitConfig(p->core, NULL);
		p->core->setVideoBuffer(p->core, (color_t*)p->video, W);
		if (!mCoreLoadFile(p->core, argv[1])) { fprintf(stderr, "rom load failed\n"); return 1; }
		struct VFile* sav = VFileOpen(argv[3 + i], O_CREAT | O_RDWR);
		if (!sav || !p->core->loadSave(p->core, sav)) { fprintf(stderr, "save load failed\n"); return 1; }
		GBASIOLockstepNodeCreate(&p->node);
		GBASIOLockstepAttachNode(&sLockstep, &p->node);
		struct GBA* gba = p->core->board;
		GBASIOSetDriver(&gba->sio, &p->node.d, SIO_MULTI);
		GBASIOSetDriver(&gba->sio, &p->node.d, SIO_NORMAL_32);
		p->awake = 1;
		p->thread.core = p->core;
		p->thread.userData = p;
		p->thread.frameCallback = frameCallback;
		p->thread.logger.logger = &logger;
	}
	const char* record = getenv("RECORD");
	if (record && record[0]) {
		char cmd[900];
		snprintf(cmd, sizeof(cmd),
			"ffmpeg -y -loglevel error -f rawvideo -pix_fmt rgb24 -s %dx%d -r 60 -i - "
			"-vf \"drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf:"
			"text='P1':x=4:y=2:fontsize=12:fontcolor=white:box=1:boxcolor=black@0.6,"
			"drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf:"
			"text='P2':x=248:y=2:fontsize=12:fontcolor=white:box=1:boxcolor=black@0.6,"
			"drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf:"
			"text='f %%{n}':x=4:y=148:fontsize=10:fontcolor=white:box=1:boxcolor=black@0.6\" "
			"-c:v libx264 -preset veryfast -crf 20 -pix_fmt yuv420p -movflags +faststart '%s'",
			W * 2 + 2, H, record);
		sRecord = popen(cmd, "w");
		if (!sRecord) { fprintf(stderr, "ffmpeg failed\n"); return 1; }
	}
	for (int i = 0; i < 2; i++)
		if (!mCoreThreadStart(&P[i].thread)) { fprintf(stderr, "thread start failed\n"); return 1; }

	while (!(P[0].done && P[1].done)) {
		usleep(20000);
		if (mCoreThreadHasCrashed(&P[0].thread) || mCoreThreadHasCrashed(&P[1].thread)) {
			fprintf(stderr, "core crashed\n");
			sFailures++;
			break;
		}
	}
	// Tearing down a live lockstep pair can deadlock. Write any dirty
	// save sectors first; exiting here skips the normal core shutdown.
	for (int i = 0; i < 2; i++) {
		struct GBA* gba = P[i].core->board;
		GBASavedataClean(&gba->memory.savedata, 0);
		mCoreThreadInterrupt(&P[i].thread);
	}
	if (sRecord) {
		pclose(sRecord);
		out("recorded %d side-by-side frames\n", sRecorded);
	}
	out("frames p0=%d p1=%d failures=%d\n", P[0].frame, P[1].frame, sFailures);
	_exit(sFailures ? 1 : 0);
}
