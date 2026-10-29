#include <signal.h>
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

enum { C_PARTNER, C_GOTO, C_HEX, C_PRESS, C_WAIT, C_SHOT, C_DUMP, C_BARRIER, C_UNTIL_CB2, C_UNTIL_FIELD, C_EXPECT, C_UNTIL_MEM, C_BATTLE, C_MENU, C_PARTY, C_FIXTURE, C_WRITE, C_LEAD, C_MOVE, C_SIX, C_VAR, C_FLAG, C_VS, C_EXPECT_FLAG, C_EXPECT_VAR, C_NO_OBJECT, C_MAP, C_OBJECT, C_TEXT_SPEED, C_PACE, C_MUTE, C_PARTNER_AT, C_ITEM, C_EXPECT_ITEM, C_UNTIL_TEXT, C_WARP, C_PC_COUNT, C_MONS, C_NO_COPIES, C_NOTE_MONEY, C_MONEY_GAIN, C_FORCE_ENC, C_EXPECT_RANGE, C_WALK, C_NO_WILD, C_CROSS, C_WALK_BATTLE, C_SMART, C_STRONG, C_UNTIL_TASK, C_MIRROR, C_SPECIES, C_NOTE_PARTY, C_SAME_PARTY, C_PATCH_RET, C_SLOT, C_TXN, C_MONEY_SAME, C_DEX, C_SWAP, C_GENDER, C_FILL, C_UNFILL, C_WRITEP, C_TINT, C_UNTIL_FTEXT, C_NICK, C_GRIND, C_CATCH, C_HUNT, C_SWITCH_LEAD, C_PARTY_SLOT, C_HEAL_BELOW, C_TALK_OBJ, C_BOXES, C_TEST_HOOK, C_WATCH, C_EXPECT_WRITES, C_SURF, C_BAG_ITEM, C_FLY_MAP, C_STEP_TO, C_PARTY_ACTION, C_DIVE, C_RIDE, C_LEGEND, C_OWNED, C_PCFILL, C_TASKS, C_ALLOW_RESET, C_ROMWRITE, C_OBJBYTE, C_NPC_SYNC, C_BATTLE_ITEM, C_FIELD_ITEM, C_FISH, C_FORCE_MOVE, C_SCREEN_SAME, C_EXPECT_TEXT, C_MONEY_DELTA, C_SET_MONEY, C_NOTE_TEXT, C_EXPECT_CB2, C_MEGA_NEXT, C_LEAK };

struct Cmd {
	unsigned args[8];
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
	// gMain.callback2 history from the store hook: every change, even one
	// that lasts less than a frame (until_cb2 looks back to its command start).
	unsigned cb2Hist[64];
	volatile unsigned cb2HistN;
	unsigned cb2Mark;
	int resetAllowed;          // allow_reset: this scenario resets the game on purpose
	volatile int unexpectedReset;
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
    unsigned noWild;
    unsigned crashStart; // frames of START to leave an assert crash screen
    unsigned boxSum; int boxSumSet;
    unsigned boxCopy[240];
    int crashSeen;
    unsigned smartMoves;
    int forceMove, forceTarget; // force_move: -1 off; target 4 = the ally
    int megaPress, megaPhase, megaLeave; // mega_next: START presses left on a move menu
    int paceSettle;                      // pace: frames the player has stood still since the stop condition
    int learnPhase, learnTarget; unsigned learnStart, partyBackFrame;
    int lastSent; unsigned lastSentFrame;   // the last replacement chosen (both battlers down)
    unsigned strongBattlers;
    unsigned weakFoes;
    int leakNote[5];
    bool leakNoted;
    unsigned offField;
    unsigned noPath;      // walk: consecutive field frames with no path
    unsigned char partyNote[600];
    unsigned patchAddr, patchOpcode, patchFrames;
    int grindCount, grindInBattle, grindSwitched;
    int catchBallDowns;   // catch with a chosen ball: DOWN presses still to make (-1 done)
    int catchFightLeft, catchWasMenu;   // catch: action menus to fight at first
    unsigned maxSessionTasks;   // most live co-op session tasks seen in a frame (C_TASKS max)
    int switchLead, switchState;
    int itemPhase;        // battle_item: step within the bag
    int healPct, healState, healsLeft, healTarget, healUses; unsigned healFrame;
    char noteText[64]; int noteSeen;   // note_text / expect_noted
    unsigned monNote[8]; int monNoteSet[8];   // expect_mon ... @K
};

static struct Player P[2];

// WATCH_BOX=1: a write watch on the first 12 slots of PC box 1 (set from the
// box watch below). Every store into it is logged with the code doing it.
static unsigned sWatchLo[2], sWatchHi[2];
// A second watched range (debug: WATCH2=SYM:LEN), same log.
static unsigned sWatch2Lo, sWatch2Hi;
static int sWatchLogs[2];
static unsigned sCb2Addr, sCopyrightCb2;   // gMain.callback2 and the boot callback
static volatile int sAbortRun;              // a critical failure: stop the scenario
static char sAbortWhy[160];
static int sWatchHits[2];
static int sWatchQuiet[2];
static struct ARMMemory sOrigMem[2];
static int watchPlayer(struct ARMCore* cpu) {
	for (int i = 0; i < 2; i++)
		if (P[i].core && ((struct GBA*)P[i].core->board)->cpu == cpu) return i;
	return -1;
}
static const char* symAt(unsigned addr);
static void out(const char* fmt, ...);
static void watchHit(struct ARMCore* cpu, int id, uint32_t addr, const char* kind, uint32_t val) {
	sWatchHits[id]++;
	if (sWatchLogs[id] >= 400) return;
	if (sWatchQuiet[id] && sWatchLogs[id] >= 20 && !getenv("WATCH_ALL")) return;
	// The save-block shuffle at each battle start copies the boxes legitimately.
	if (!strncmp(symAt(cpu->gprs[14]), "MoveSaveBlocks", 14)) return;
	sWatchLogs[id]++;
	char pc[120];
	snprintf(pc, sizeof(pc), "%s", symAt(cpu->gprs[15] - ((cpu->cpsr.t) ? 4 : 8)));
	out("[f%06d p%d WRITE] %s at %08x (slot %u) v=%08x pc=%s lr=%s\n", P[id].frame, id, kind, addr, (addr - sWatchLo[id]) / 80, val, pc, symAt(cpu->gprs[14]));
}
static void hookStore32(struct ARMCore* cpu, uint32_t a, int32_t v, int* c) {
	int id = watchPlayer(cpu);
	if (id >= 0 && a == sCb2Addr) {
		struct Player* pl = &P[id];
		pl->cb2Hist[pl->cb2HistN & 63] = (uint32_t)v & ~1u;
		pl->cb2HistN++;
		if (sCopyrightCb2 && ((uint32_t)v & ~1u) == sCopyrightCb2 && pl->frame > 60 && !pl->resetAllowed) pl->unexpectedReset++;
	}
	if (id >= 0 && ((a >= sWatchLo[id] && a < sWatchHi[id]) || (a >= sWatch2Lo && a < sWatch2Hi))) watchHit(cpu, id, a, "str32", (uint32_t)v);
	sOrigMem[id < 0 ? 0 : id].store32(cpu, a, v, c);
}
static void hookStore16(struct ARMCore* cpu, uint32_t a, int16_t v, int* c) {
	int id = watchPlayer(cpu);
	if (id >= 0 && ((a >= sWatchLo[id] && a < sWatchHi[id]) || (a >= sWatch2Lo && a < sWatch2Hi))) watchHit(cpu, id, a, "str16", (uint16_t)v);
	sOrigMem[id < 0 ? 0 : id].store16(cpu, a, v, c);
}
static void hookStore8(struct ARMCore* cpu, uint32_t a, int8_t v, int* c) {
	int id = watchPlayer(cpu);
	if (id >= 0 && ((a >= sWatchLo[id] && a < sWatchHi[id]) || (a >= sWatch2Lo && a < sWatch2Hi))) watchHit(cpu, id, a, "str8", (uint8_t)v);
	sOrigMem[id < 0 ? 0 : id].store8(cpu, a, v, c);
}
static uint32_t hookStoreMultiple(struct ARMCore* cpu, uint32_t base, int mask, enum LSMDirection dir, int* c) {
	int id = watchPlayer(cpu);
	int n = __builtin_popcount(mask & 0xFFFF);
	uint32_t lo = (dir & LSM_D) ? base - n * 4 : base, hi = (dir & LSM_D) ? base : base + n * 4;
	if (id >= 0 && hi > sWatchLo[id] && lo < sWatchHi[id]) watchHit(cpu, id, lo, "stm", 0);
	return sOrigMem[id < 0 ? 0 : id].storeMultiple(cpu, base, mask, dir, c);
}
static void installWatch(struct Player* p) {
	struct ARMCore* cpu = ((struct GBA*)p->core->board)->cpu;
	if (cpu->memory.store32 == hookStore32) return;
	sOrigMem[p->id] = cpu->memory;
	cpu->memory.store32 = hookStore32;
	cpu->memory.store16 = hookStore16;
	cpu->memory.store8 = hookStore8;
	cpu->memory.storeMultiple = hookStoreMultiple;
}
// Was callback2 set to ADDR at any moment since the current command began?
static bool cb2Seen(struct Player* p, unsigned addr) {
	unsigned n = p->cb2HistN, from = p->cb2Mark;
	if (n - from > 64) from = n - 64;
	for (unsigned i = from; i < n; i++)
		if (p->cb2Hist[i & 63] == (addr & ~1u)) return true;
	return false;
}
static struct GBASIOLockstep sLockstep;
// Reset trap (always on): a branch into the BIOS vectors 0x00-0x0F (reset,
// undefined, SWI with a real BIOS, abort) or a SoftReset/RegisterRamReset
// SWI. Logs the registers, the stack and the last region changes, so an
// unexplained reboot can be traced to its caller.
struct RegionHop { uint32_t to, lr, sp; };
static struct RegionHop sHops[2][32];
static unsigned sHopPos[2];
static void (*sOrigSetRegion[2])(struct ARMCore*, uint32_t);
static void (*sOrigSwi16[2])(struct ARMCore*, int);
static void (*sOrigSwi32[2])(struct ARMCore*, int);
static int sResetLogs[2];
static void resetTrapLog(struct ARMCore* cpu, int id, const char* why, uint32_t to) {
	if (sResetLogs[id] == 0 && !P[id].resetAllowed) P[id].unexpectedReset = P[id].unexpectedReset ? P[id].unexpectedReset : 1;
	if (sResetLogs[id]++ >= 6) return;
	out("[p%d RESET-TRAP] %s to=%08x lr=%08x(%s) sp=%08x cpsr=%08x\n", id, why, to, cpu->gprs[14], symAt(cpu->gprs[14]), cpu->gprs[13], cpu->cpsr.packed);
	char line[400]; int n = 0;
	for (int r = 0; r < 13; r++) n += snprintf(line + n, sizeof(line) - n, "r%d=%08x ", r, cpu->gprs[r]);
	out("[p%d RESET-TRAP] %s\n", id, line);
	for (int k = 0; k < 24; k++) {
		uint32_t a = cpu->gprs[13] + k * 4;
		if (a < 0x03000000 || a >= 0x03008000) break;
		int cyc = 0; uint32_t v = cpu->memory.load32(cpu, a, &cyc);
		if (v >= 0x08000000 && v < 0x0A000000) out("[p%d RESET-TRAP] stack+%02x %08x %s\n", id, k * 4, v, symAt(v & ~1u));
	}
	for (unsigned k = 0; k < 32; k++) {
		struct RegionHop* h = &sHops[id][(sHopPos[id] + k) % 32];
		if (!h->to && !h->lr) continue;
		char a[80]; snprintf(a, sizeof(a), "%s", symAt(h->to));
		out("[p%d RESET-TRAP] hop to=%08x %s lr=%s\n", id, h->to, a, symAt(h->lr));
	}
}
static void hookSetRegion(struct ARMCore* cpu, uint32_t address) {
	int id = watchPlayer(cpu); if (id < 0) id = 0;
	if (address < 0x10 && address != 0x8) resetTrapLog(cpu, id, "branch-to-vector", address);
	if (address >= 0x10 && address < 0x4000) {
		// IRQs land in the BIOS often; don't fill the ring with them.
	} else {
		struct RegionHop* h = &sHops[id][sHopPos[id]++ % 32];
		h->to = address; h->lr = cpu->gprs[14]; h->sp = cpu->gprs[13];
	}
	sOrigSetRegion[id](cpu, address);
}
static void hookSwi16(struct ARMCore* cpu, int imm) {
	int id = watchPlayer(cpu); if (id < 0) id = 0;
	if (imm == 0 || imm == 0x26) resetTrapLog(cpu, id, imm ? "swi-HardReset" : "swi-SoftReset", cpu->gprs[15]);
	sOrigSwi16[id](cpu, imm);
}
static void hookSwi32(struct ARMCore* cpu, int imm) {
	int id = watchPlayer(cpu); if (id < 0) id = 0;
	int n = imm >> 16;
	if (n == 0 || n == 0x26) resetTrapLog(cpu, id, n ? "swi32-HardReset" : "swi32-SoftReset", cpu->gprs[15]);
	sOrigSwi32[id](cpu, imm);
}
static void installResetTrap(struct Player* p) {
	struct ARMCore* cpu = ((struct GBA*)p->core->board)->cpu;
	if (cpu->memory.setActiveRegion == hookSetRegion) return;
	sOrigSetRegion[p->id] = cpu->memory.setActiveRegion;
	sOrigSwi16[p->id] = cpu->irqh.swi16;
	sOrigSwi32[p->id] = cpu->irqh.swi32;
	cpu->memory.setActiveRegion = hookSetRegion;
	cpu->irqh.swi16 = hookSwi16;
	cpu->irqh.swi32 = hookSwi32;
}

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

static int symCacheGet(const char* name, int kind, unsigned* out);
static void symCachePut(const char* name, int kind, unsigned val);
static unsigned symValueSlow(const char* name);
static unsigned symValue(const char* name) {
	unsigned v;
	if (symCacheGet(name, 2, &v)) return v;
	v = symValueSlow(name);
	symCachePut(name, 2, v);
	return v;
}
static unsigned symValueSlow(const char* name) {
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

// Lookups are memoised by name (a copy is kept), so per-frame lookups stay
// cheap.
#define SYM_CACHE 4096
static struct { char* key; unsigned val; int kind; } sSymCache[SYM_CACHE];
static pthread_mutex_t sSymLock = PTHREAD_MUTEX_INITIALIZER;
static unsigned symHash(const char* name, int kind) {
	unsigned h = 2166136261u ^ (unsigned)kind;
	for (; *name; name++) h = (h ^ (unsigned char)*name) * 16777619u;
	return h & (SYM_CACHE - 1);
}
static int symCacheGet(const char* name, int kind, unsigned* out) {
	int found = 0;
	pthread_mutex_lock(&sSymLock);
	for (unsigned i = symHash(name, kind), n = 0; n < SYM_CACHE && sSymCache[i].key; i = (i + 1) & (SYM_CACHE - 1), n++)
		if (sSymCache[i].kind == kind && !strcmp(sSymCache[i].key, name)) { *out = sSymCache[i].val; found = 1; break; }
	pthread_mutex_unlock(&sSymLock);
	return found;
}
static void symCachePut(const char* name, int kind, unsigned val) {
	pthread_mutex_lock(&sSymLock);
	for (unsigned i = symHash(name, kind), n = 0; n < SYM_CACHE; i = (i + 1) & (SYM_CACHE - 1), n++)
		if (!sSymCache[i].key) { sSymCache[i].key = strdup(name); sSymCache[i].kind = kind; sSymCache[i].val = val; break; }
	pthread_mutex_unlock(&sSymLock);
}

// Address-sorted indexes for the reverse lookups (built once).
static int* sTextIdx; static int sNumText;   // code symbols, by address
static int* sDataIdx; static int sNumData;   // everything >= 0x02000000
static pthread_once_t sIdxOnce = PTHREAD_ONCE_INIT;
static int cmpTextIdx(const void* a, const void* b) {
	unsigned x = sSyms[*(const int*)a].addr & ~1u, y = sSyms[*(const int*)b].addr & ~1u;
	return x < y ? -1 : x > y;
}
static int cmpDataIdx(const void* a, const void* b) {
	unsigned x = sSyms[*(const int*)a].addr, y = sSyms[*(const int*)b].addr;
	return x < y ? -1 : x > y;
}
static void buildSymIdx(void) {
	sTextIdx = malloc(sizeof(int) * (sNumSyms + 1));
	sDataIdx = malloc(sizeof(int) * (sNumSyms + 1));
	for (int i = 0; i < sNumSyms; i++) {
		if ((sSyms[i].type == 'T' || sSyms[i].type == 't') && sSyms[i].addr >= 0x02000000) sTextIdx[sNumText++] = i;
		if (sSyms[i].addr >= 0x02000000) sDataIdx[sNumData++] = i;
	}
	// Stable: keep the first symbol (file order) among equal addresses.
	qsort(sTextIdx, sNumText, sizeof(int), cmpTextIdx);
	qsort(sDataIdx, sNumData, sizeof(int), cmpDataIdx);
}
// Index of the last entry with address <= addr (or -1).
static int idxFloor(const int* idx, int n, unsigned addr, int text) {
	int lo = 0, hi = n - 1, best = -1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		unsigned a = text ? (sSyms[idx[mid]].addr & ~1u) : sSyms[idx[mid]].addr;
		if (a <= addr) { best = mid; lo = mid + 1; } else hi = mid - 1;
	}
	return best;
}

static unsigned symAddrSlow(const char* name);
static unsigned symAddr(const char* name) {
	unsigned v;
	if (symCacheGet(name, 1, &v)) return v;
	v = symAddrSlow(name);
	symCachePut(name, 1, v);
	return v;
}

static unsigned symAddrSlow(const char* name) {
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
	pthread_once(&sIdxOnce, buildSymIdx);
	int k = idxFloor(sTextIdx, sNumText, addr, 1);
	if (k >= 0 && (sSyms[sTextIdx[k]].addr & ~1u) == addr) {
		// The first-listed symbol at this address, as the linear scan gave.
		while (k > 0 && (sSyms[sTextIdx[k - 1]].addr & ~1u) == addr) k--;
		int best = sTextIdx[k];
		for (int j = k; j < sNumText && (sSyms[sTextIdx[j]].addr & ~1u) == addr; j++)
			if (sTextIdx[j] < best) best = sTextIdx[j];
		return sSyms[best].name;
	}
	snprintf(buf, sizeof(buf), "%08x", addr);
	return buf;
}

static const char* symAtAny(unsigned addr) {
	static __thread char buf[120];
	pthread_once(&sIdxOnce, buildSymIdx);
	int k = idxFloor(sDataIdx, sNumData, addr, 0);
	if (k < 0) snprintf(buf, sizeof(buf), "%08x", addr);
	else { int best = sDataIdx[k]; snprintf(buf, sizeof(buf), "%s+0x%x", sSyms[best].name, addr - sSyms[best].addr); }
	return buf;
}

static const char* symAt(unsigned addr) {
	static __thread char buf[120];
	addr &= ~1u;
	pthread_once(&sIdxOnce, buildSymIdx);
	int k = idxFloor(sTextIdx, sNumText, addr, 1);
	if (k < 0) snprintf(buf, sizeof(buf), "%08x", addr);
	else { int best = sTextIdx[k]; snprintf(buf, sizeof(buf), "%s+0x%x", sSyms[best].name, addr - (sSyms[best].addr & ~1u)); }
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
	unsigned v;
	if (symCacheGet(name, 2, &v)) return v;
	v = 0;
	for (int i = 0; i < sNumSyms; i++)
		if (!strcmp(sSyms[i].name, name)) { v = sSyms[i].addr; break; }
	symCachePut(name, 2, v);
	return v;
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
// Position of substruct 3 (misc: met location, met level, IVs) per order.
static const unsigned char sSub3[24] = {3, 2, 3, 2, 1, 1, 3, 2, 3, 2, 1, 1, 3, 2, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0};

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
	o = 0;
	for (int slot = 0; slot < 6; slot++) {
		unsigned mon = partyMon(0, slot);
		if (!(rd8(p, mon + 19) & 2)) break;
		o += snprintf(line + o, sizeof(line) - o, "Lv%u %u/%u  ", rd8(p, mon + 84), rd16(p, mon + 86), rd16(p, mon + 88));
	}
	out("[f%06d p%d party-levels] %s\n", p->frame, p->id, line);
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
		// A copy of this player's own lead would share personality AND OT ID.
		// Personality alone can match: both cores boot in the same second, the
		// RTC seeds the RNG, and two different starters can roll the same value.
		bool copied = own == partner && rd32(p, partyMon(0, 0) + 4) == rd32(p, partyMon(2, 0) + 4);
		if (!own || rd32(p, partyMon(0, 1)) || !partner || rd32(p, partyMon(2, 1)) || copied)
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
	// A script that is not idle: where it is, and the tasks running.
	if (sScriptStatus && rd8(p, sScriptStatus) != 2) {
		unsigned sc = symAddr("sGlobalScriptContext");
		if (!sc) sc = symAddr("script.sGlobalScriptContext");
		char tasks[400]; int n = 0; unsigned tk = symAddr("gTasks");
		for (int t = 0; t < 16 && n < 360; t++)
			if (rd8(p, tk + t * 0x28 + 4)) n += snprintf(tasks + n, sizeof(tasks) - n, " %s", symName(rd32(p, tk + t * 0x28)));
		tasks[n] = 0;
		if (sc) {
			char nat[80], ptr[80];
			snprintf(nat, sizeof(nat), "%s", symAt(rd32(p, sc + 4)));
			snprintf(ptr, sizeof(ptr), "%s", symAt(rd32(p, sc + 8)));
			out("[f%06d p%d %s script] mode=%u depth=%u native=%s ptr=%s (%08x) stack0=%08x tasks:%s\n", p->frame, p->id, tag,
			    rd8(p, sc + 1), rd8(p, sc), nat, ptr, rd32(p, sc + 8), rd32(p, sc + 12), tasks);
		}
	}
}

static void battleDump(struct Player* p) {
	char ctl[4][64];
	unsigned funcs = symAddr("gBattlerControllerFuncs");
	for (int i = 0; i < 4; i++)
		snprintf(ctl[i], sizeof(ctl[i]), "%s@%x", symName(rd32(p, funcs + i * 4)), rd32(p, funcs + i * 4));
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
	{
		// Animation state, each battler's healthbox flags (BattleHealthboxInfo is
		// 16 bytes; animationState at +4), active tasks and sprites in use: a move animation
		// that never ends shows here.
		unsigned tasks = symAddr("gTasks"), spr = symAddr("gSprites");
		char tl[400]; int n = 0, used = 0;
		for (int i = 0; i < 16 && n < (int)sizeof(tl) - 40; i++)
			if (rd8(p, tasks + i * 0x28 + 4))
				n += snprintf(tl + n, sizeof(tl) - n, " %s", symName(rd32(p, tasks + i * 0x28)));
		for (int i = 0; i < 64; i++)
			if (rd8(p, spr + i * 0x44 + 0x3E) & 1) used++;
		unsigned hb = rd32(p, rd32(p, symAddr("gBattleSpritesDataPtr")) + 4);
		for (int b = 0; b < 4; b++)
			out("[f%06d p%d bhealthbox] b%d flags=%02x %02x state=%u\n", p->frame, p->id, b,
			    rd8(p, hb + b * 16), rd8(p, hb + b * 16 + 1), rd8(p, hb + b * 16 + 4));
		out("[f%06d p%d banim] scriptActive=%u visual=%u sound=%u sprites=%d tasks:%s\n", p->frame, p->id,
		    rd8(p, symAddr("gAnimScriptActive")), rd8(p, symAddr("gAnimVisualTaskCount")), rd8(p, symAddr("gAnimSoundTaskCount")), used, tl);
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
	// A script stopped at waitstate whose battle has ended (Rock Smash's
	// encounter, as in the original game) is also left at 1 with the
	// controls free and no message box: the player is in the field.
	if (sScriptStatus && rd8(p, sScriptStatus) != 2) {
		static unsigned msgMode;
		if (!msgMode) msgMode = symAddr("sFieldMessageBoxMode");
		if (!(rd8(p, sScriptStatus) == 1 && msgMode && rd8(p, msgMode) == 0)) return false;
	}
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


// Keys a player would press when not in control: tap A through messages
// and battles; in a battle's party menu (forced switch) step to a healthy
// Pokemon (DOWN, A, A); back out of a summary screen with B.
// The player stands in the free field facing the co-op partner's sprite.
// A tapped there opens the partner menu (BATTLE, TRADE, ...), which a
// scenario that only mashes through text never means to do.
static bool facingPartner(struct Player* p) {
	if (!inField(p)) return false;
	unsigned obj = sObjEvents + rd8(p, sAvatar + 5) * 0x24;
	int x = (int16_t)rd16(p, obj + 0x10), y = (int16_t)rd16(p, obj + 0x12);
	switch (rd8(p, obj + 0x18) & 0xF) {
	case 1: y++; break;
	case 2: y--; break;
	case 3: x--; break;
	case 4: x++; break;
	default: return false;
	}
	for (int i = 0; i < 16; i++) {
		unsigned o = sObjEvents + i * 0x24;
		if ((rd8(p, o) & 1) && rd8(p, o + 8) == 0xFC
		 && (int16_t)rd16(p, o + 0x10) == x && (int16_t)rd16(p, o + 0x12) == y)
			return true;
	}
	return false;
}

static unsigned autoKeys(struct Player* p, unsigned step) {
	static unsigned partyCb, summaryMain, summaryInit;
	if (!partyCb) {
		partyCb = symFind("CB2_UpdatePartyMenu") & ~1u;
		summaryMain = symFind("MainCB2") & ~1u;
		summaryInit = symFind("CB2_InitSummaryScreen") & ~1u;
	}
	unsigned cbNow = rd32(p, sMain + 4) & ~1u;
	bool battling = rd8(p, sMain + 0x439) & 2;
	// A move with no PP left under the cursor: move the cursor to the first
	// move that has some (as a player would), else A loops on "no PP left".
	if (battling && p->forceMove < 0) {
		unsigned funcs = symAddr("gBattlerControllerFuncs");
		for (int b = 0; b < 4; b++) {
			if (strcmp(symName(rd32(p, funcs + b * 4)), "HandleInputChooseMove")) continue;
			unsigned res = rd32(p, symAddr("gBattleResources")), cur = symAddr("gMoveSelectionCursor");
			unsigned m = res + 0x10 + b * 0x200 + 4, at = rd8(p, cur + b);
			if (res && at < 4 && rd8(p, m + 8 + at) == 0)
				for (int i = 0; i < 4; i++)
					if (rd16(p, m + i * 2) && rd8(p, m + 8 + i)) { p->core->busWrite8(p->core, cur + b, i); break; }
		}
	}
	// switch_lead SLOT (driver aid): in a solo battle, the first action is to
	// switch the lead out for party slot SLOT, so a weak lead shares the
	// experience (Gen 6+ full experience) without fighting. 0 is off.
	if (!battling) p->switchState = 0;
	if (battling && p->switchLead && p->switchState < 3 && !(rd32(p, symAddr("gBattleTypeFlags")) & (1u << 6))) {
		unsigned t = step % 20;
		if (partyCb && cbNow == partyCb) {
			int slot = (signed char)rd8(p, symAddr("gPartyMenu") + 9);
			if (rd8(p, symAddr("gPartyMenu") + 11) == 1) p->switchState = 3;   // forced replacement: normal handling
			else if (p->switchState == 2) return (step % 30) == 0 ? 1 : 0;      // SHIFT in the submenu
			else if (t == 0) {
				if (slot != p->switchLead) return 1u << 7;
				p->switchState = 2;
				return 1;
			} else return 0;
		} else if (p->switchState == 2) {
			p->switchState = 3;
		} else {
			int atMenu = 0;
			unsigned funcs = symAddr("gBattlerControllerFuncs");
			for (int b = 0; b < 4; b++)
				if (!strcmp(symName(rd32(p, funcs + b * 4)), "HandleInputChooseAction")) atMenu = 1;
			if (atMenu) {
				if (rd16(p, partyMon(0, p->switchLead) + 86) == 0
				 || rd16(p, symAddr("gBattlerPartyIndexes")) == (unsigned)p->switchLead) { p->switchState = 3; }
				else {
					unsigned cur = rd8(p, symAddr("gActionSelectionCursor"));
					p->switchState = 1;
					if (t != 0) return 0;
					if (cur & 1) return 1u << 5;
					if (!(cur & 2)) return 1u << 7;
					return 1;
				}
			}
		}
	}
	// heal_below PCT [USES] (driver aid): at this player's action menu, when
	// its Pokemon is below PCT% HP, use a POTION or SUPER POTION from the
	// bag on it (up to USES times per battle).
	if (!battling) { p->healState = 0; p->healsLeft = -1; }
	// Co-op trainer battles are link battles: the bag is closed there
	// ("Items can't be used now."); co-op wild battles allow it.
	if (battling && p->healPct && (!(rd32(p, symAddr("gBattleTypeFlags")) & (1u << 1)) || !(rd32(p, symAddr("gBattleTypeFlags")) & (1u << 3)))) {
		static unsigned bagCb;
		if (!bagCb) bagCb = symAddr("CB2_BagMenuRun") & ~1u;
		if (p->healsLeft < 0) p->healsLeft = p->healUses;
		unsigned bm = symAddr("gBattleMons"), bmSize = symValue("harness.battleMonSize");
		unsigned bmHp = symValue("harness.battleMonHp"), bmMax = symValue("harness.battleMonMaxHp");
		unsigned t = step % 20;
		if (p->healState == 0 && p->healsLeft > 0) {
			int me = -1;
			unsigned funcs = symAddr("gBattlerControllerFuncs");
			for (int b = 0; b < 4; b++)
				if (!strcmp(symName(rd32(p, funcs + b * 4)), "HandleInputChooseAction")) me = b;
			if (me >= 0) {
				// Party data, not gBattleMons: the co-op guest's gBattleMons
				// trails the host's, but its own party is kept current.
				unsigned pidx = rd16(p, symAddr("gBattlerPartyIndexes") + me * 2);
				unsigned hp = pidx < 6 ? rd16(p, partyMon(0, pidx) + 86) : 0, mx = pidx < 6 ? rd16(p, partyMon(0, pidx) + 88) : 0;
				(void)bm; (void)bmSize; (void)bmHp; (void)bmMax;
				// The potion's place in the ITEMS pocket list.
				unsigned pocket = symAddr("gBagPockets"), slots = rd32(p, pocket), cap = rd16(p, pocket + 4) & 0x3FF;
				int k = -1, n = 0;
				for (unsigned i = 0; i < cap && i < 64; i++) {
					unsigned id = rd16(p, slots + i * 4);
					if (!id) continue;
					if (k < 0 && (id == 28 || id == 29)) k = n;
					n++;
				}
				if (hp && mx && hp * 100 < (unsigned)p->healPct * mx && k >= 0) {
					p->healState = 1; p->healTarget = k; p->healFrame = p->frame;
					out("[f%06d p%d heal] battler %d hp %u/%u: potion at list %d\n", p->frame, p->id, me, hp, mx, k);
				}
			}
		}
		if (p->healState && p->frame - p->healFrame > 1500) { p->healState = 0; p->healsLeft--; }   // give up
		if (p->healState == 1) {
			if (cbNow == bagCb) {
				unsigned bp = symAddr("gBagPosition");
				unsigned pk = rd8(p, bp + 5);
				if (t != 0) return 0;
				if (pk != 0) return 1u << 5;
				int idx = rd16(p, bp + 8) + rd16(p, bp + 18);
				if (idx < p->healTarget) return 1u << 7;
				if (idx > p->healTarget) return 1u << 6;
				p->healState = 2;
				return 1;
			}
			int atMenu = -1;
			unsigned funcs = symAddr("gBattlerControllerFuncs");
			for (int b = 0; b < 4; b++)
				if (!strcmp(symName(rd32(p, funcs + b * 4)), "HandleInputChooseAction")) atMenu = b;
			if (atMenu >= 0) {
				unsigned cur = rd8(p, symAddr("gActionSelectionCursor") + atMenu);
				if (t != 0) return 0;
				if (!(cur & 1)) return 1u << 4;
				if (cur & 2) return 1u << 6;
				return 1;
			}
			return 0;
		}
		if (p->healState == 2) {
			// The item's small menu (USE first), then the party menu.
			if (partyCb && cbNow == partyCb) { p->healState = 3; return 0; }
			return (step % 30) == 0 ? 1 : 0;
		}
		if (p->healState == 3) {
			if (partyCb && cbNow == partyCb) {
				int slot = (signed char)rd8(p, symAddr("gPartyMenu") + 9);
				if (t != 0) return 0;
				if (slot != 0) return 1u << 6;
				p->healState = 4;
				return 1;
			}
			return 0;
		}
		if (p->healState == 4) {
			if (partyCb && cbNow == partyCb) return (step % 30) == 0 ? 1 : 0;   // "HP was restored" text
			p->healState = 0; p->healsLeft--;
			out("[f%06d p%d heal] used, %d left this battle\n", p->frame, p->id, p->healsLeft);
		}
	}
	// Default battle input: keep the action cursor on FIGHT (the game
	// remembers the last action, so after one BAG the A presses would keep
	// opening the bag), and leave a bag nobody asked for.
	if (battling && p->healState == 0) {
		static unsigned bagCb2;
		if (!bagCb2) bagCb2 = symAddr("CB2_BagMenuRun") & ~1u;
		if (cbNow == bagCb2) return (step % 30) < 2 ? 2 : 0;
		unsigned funcs = symAddr("gBattlerControllerFuncs");
		for (int b = 0; b < 4; b++)
			if (!strcmp(symName(rd32(p, funcs + b * 4)), "HandleInputChooseAction")) {
				unsigned cur = rd8(p, symAddr("gActionSelectionCursor") + b);
				if (cur != 0) return (step % 16) == 0 ? ((cur & 1) ? (1u << 5) : (1u << 6)) : 0;
			}
	}
	if (battling && partyCb && cbNow == partyCb) {
		// Replacement or switch: move the cursor to a slot whose Pokemon
		// can fight (HP > 0), then A (the mon) and A (SHIFT). Coming back
		// from a summary screen, B first closes the submenu that opened it.
		unsigned t = step % 120;
		if (p->learnPhase) { p->learnPhase = 0; p->partyBackFrame = p->frame; }
		if (p->partyBackFrame && p->frame - p->partyBackFrame < 30) return (p->frame - p->partyBackFrame) < 2 ? 2 : 0;
		int slot = (signed char)rd8(p, symAddr("gPartyMenu") + 9);
		unsigned bflags = rd32(p, symAddr("gBattleTypeFlags"));
		bool multi = bflags & (1u << 6), dbl = bflags & 1;
		// An optional switch (the "Will you switch?" prompt; the menu's action
		// is not SEND_OUT): keep the Pokemon on the field, B out. While the
		// menu is open the party data is in battle order (the menu reorders
		// it): slot 0 (and slot 1 in an ordinary double) is on the field.
		if (rd8(p, symAddr("gPartyMenu") + 11) != 1) return (step % 30) < 2 ? 2 : 0;
		int first = (dbl && !multi) ? 2 : 1;
		bool usable = slot >= first && slot < (multi ? 3 : 6) && rd16(p, partyMon(0, slot) + 86) > 0;
		// Both battlers down in a double: the menu opens twice, and the
		// first choice still shows as a healthy slot the second time (the
		// game refuses it). Skip the slot sent a moment ago.
		if (usable && p->lastSentFrame && p->frame - p->lastSentFrame < 900 && slot == p->lastSent) usable = false;
		if (getenv("PARTY_DEBUG") && t == 0)
			out("[p%d party-menu] slot %d first %d multi %d hp %u action %u\n", p->id, slot, first, multi, rd16(p, partyMon(0, slot) + 86), rd8(p, symAddr("gPartyMenu") + 11));
		if (!usable) return t < 2 ? 0x80 : 0;
		if (t >= 90 && t < 92) { p->lastSent = slot; p->lastSentFrame = p->frame ? p->frame : 1; }
		return ((t >= 40 && t < 42) || (t >= 90 && t < 92)) ? 1 : 0;
	}
	// The summary screen's MainCB2 is static, and the symbol "MainCB2" may
	// be another file's: anything in pokemon_summary_screen.c's code (the
	// file is about 0x5C00 bytes around CB2_InitSummaryScreen) counts.
	bool summary = (summaryMain && cbNow == summaryMain) || (summaryInit && cbNow == summaryInit)
		|| (summaryInit && cbNow + 0x6000 > summaryInit && cbNow < summaryInit + 0x6000);
	if (battling && summary) {
		// In battle the summary screen only opens to pick a move to forget.
		// Without smart_moves: back out (the move is not learned). With
		// smart_moves: forget the weakest move (status moves count as 0
		// power, HM moves are kept) when the new move is stronger.
		if (!p->smartMoves) return (step % 24) < 2 ? 2 : 0;
		if (!p->learnPhase) { p->learnPhase = 1; p->learnStart = step; p->learnTarget = -1; return 0; }
		if (p->learnPhase == 1) {
			if (step - p->learnStart < 60) return 0;
			p->learnPhase = 2;
			unsigned info = symAddr("gMovesInfo");
			// The learner: the party mon whose personality and OT id the
			// summary screen holds (its copy of the mon). This covers the
			// battle's level-up and the evolution scene alike.
			unsigned slot = 6, sum = rd32(p, symAddr("sMonSummaryScreen"));
			for (unsigned o = 0; sum && o < 0x1000 && slot == 6; o += 4)
				for (unsigned m = 0; m < 6; m++) {
					unsigned pm = partyMon(0, m);
					if (rd32(p, pm) && rd32(p, sum + o) == rd32(p, pm) && rd32(p, sum + o + 4) == rd32(p, pm + 4)) { slot = m; break; }
				}
			if (slot == 6) slot = rd8(p, rd32(p, symAddr("gBattleStruct")) + symValue("harness.expGetterOffset"));
			unsigned mon = partyMon(0, slot < 6 ? slot : 0), personality = rd32(p, mon), key = personality ^ rd32(p, mon + 4);
			static const char* order[24] = {"GAEM","GAME","GEAM","GEMA","GMAE","GMEA","AGEM","AGME","AEGM","AEMG","AMGE","AMEG",
				"EGAM","EGMA","EAGM","EAMG","EMGA","EMAG","MGAE","MGEA","MAGE","MAEG","MEGA","MEAG"};
			int a = (int)(strchr(order[personality % 24], 'A') - order[personality % 24]);
			unsigned w0 = rd32(p, mon + 32 + a * 12) ^ key, w1 = rd32(p, mon + 32 + a * 12 + 4) ^ key;
			unsigned mv[4] = { w0 & 0x7FF, (w0 >> 16) & 0x7FF, w1 & 0x7FF, (w1 >> 16) & 0x7FF };
			unsigned learn = rd16(p, symAddr("gMoveToLearn"));
			if (!learn) { p->learnTarget = -1; return 0; }
			unsigned newPow = rd32(p, info + learn * 0x40 + 8) >> 23, minPow = 0xFFFF;
			for (int i = 0; i < 4; i++) {
				int hm = 0;
				for (int k = 0; k < 8; k++) { char nm[16]; snprintf(nm, sizeof(nm), "harness.hm%d", k); if (mv[i] == symValue(nm)) hm = 1; }
				if (hm || !mv[i]) continue;
				unsigned pow = rd32(p, info + mv[i] * 0x40 + 8) >> 23;
				if (pow < minPow) { minPow = pow; p->learnTarget = i; }
			}
			if (p->learnTarget >= 0 && newPow <= minPow) p->learnTarget = -1;
			out("[f%06d p%d learn] slot %u moves %u,%u,%u,%u new %u (power %u): %s %d\n", p->frame, p->id, slot,
			    mv[0], mv[1], mv[2], mv[3], learn, newPow, p->learnTarget >= 0 ? "forget slot" : "keep all", p->learnTarget);
		}
		unsigned dt = step - p->learnStart;
		if (p->learnTarget < 0 || dt > 600) return (step % 24) < 2 ? 2 : 0;
		if (dt < 90) return 0;
		unsigned k = (dt - 90) / 16, r = (dt - 90) % 16;
		if (k < (unsigned)p->learnTarget) return r < 2 ? 0x80 : 0;
		if (k == (unsigned)p->learnTarget + 1) return r < 2 ? 1 : 0;
		return 0;
	}
	if (!summary) p->learnPhase = 0;
	if (facingPartner(p))
		return 0;
	return (step % 24) < 2 ? 1 : 0;
}

// Pathfinding for `walk`: breadth-first search over the live map (the
// game's VMap, which includes the loaded strip of connected maps), honouring
// collision, elevation, ledges (one-way jumps), water (unless surfing),
// objects (the co-op partner and twin are passable), and warps (only the
// target may be a warp). Returns the key for the first step, 0 if already
// there, or -1 when no path exists right now.
// MetatileBehavior_Is{South,North,West,East}Blocked: side 0 S, 1 N, 2 W, 3 E.
static bool sideBlocked(unsigned beh, int side)
{
	switch (side) {
	case 0: return beh == 0x33 || beh == 0x36 || beh == 0x37 || beh == 0xC0;
	case 1: return beh == 0x32 || beh == 0x34 || beh == 0x35 || beh == 0xC0;
	case 2: return beh == 0x31 || beh == 0x35 || beh == 0x37 || beh == 0xC1;
	default: return beh == 0x30 || beh == 0x34 || beh == 0x36 || beh == 0xC1;
	}
}

static int walkStep(struct Player* p, int tx, int ty) {
	static unsigned layoutSym, headerSym;
	if (!layoutSym) { layoutSym = symAddr("gBackupMapLayout"); headerSym = symAddr("gMapHeader"); }
	int w = (int)rd32(p, layoutSym), h = (int)rd32(p, layoutSym + 4);
	unsigned map = rd32(p, layoutSym + 8);
	if (w <= 0 || h <= 0 || w * h > 10240) return -1;
	unsigned lay = rd32(p, headerSym), events = rd32(p, headerSym + 4);
	unsigned pattr = rd32(p, rd32(p, lay + 0x10) + 0x10), sattr = rd32(p, rd32(p, lay + 0x14) + 0x10);
	unsigned obj = sObjEvents + rd8(p, sAvatar + 5) * 0x24;
	int sx = (int16_t)rd16(p, obj + 0x10), sy = (int16_t)rd16(p, obj + 0x12);
	int gx = tx + 7, gy = ty + 7;
	int lw = (int)rd32(p, lay), lh = (int)rd32(p, lay + 4);
	bool goalInside = tx >= 0 && ty >= 0 && tx < lw && ty < lh;
	if (sx == gx && sy == gy) return 0;
	bool surfing = rd8(p, sAvatar) & 0x18;   // surfing or underwater (Dive)
	static __thread uint8_t blocked[10240];
	// Search nodes are (tile, carried elevation): a bridge tile (15) can be
	// reached at two levels, and only one of them may lead on.
	static __thread int32_t prev[10240 * 16];
	static __thread int32_t queue[10240 * 16];
	memset(blocked, 0, (size_t)(w * h));
	for (unsigned i = 0; i < 16; i++) {
		unsigned o = sObjEvents + i * 0x24;
		if (!(rd8(p, o) & 1) || o == obj) continue;
		unsigned lid = rd8(p, o + 8);
		if (lid == 0xFC || lid == 0xF6) continue;
		int ox = (int16_t)rd16(p, o + 0x10), oy = (int16_t)rd16(p, o + 0x12);
		if (ox >= 0 && oy >= 0 && ox < w && oy < h) blocked[oy * w + ox] = 1;
	}
	unsigned nwarps = rd8(p, events + 1), warps = rd32(p, events + 8);
	for (unsigned i = 0; i < nwarps && i < 64; i++) {
		int wx = (int16_t)rd16(p, warps + i * 8) + 7, wy = (int16_t)rd16(p, warps + i * 8 + 2) + 7;
		if (wx >= 0 && wy >= 0 && wx < w && wy < h && !(wx == gx && wy == gy)) blocked[wy * w + wx] = 2;
	}
	#define BLK(x, y) rd16(p, map + 2u * (unsigned)((y) * w + (x)))
	#define BEH(b) (((b) & 0x3FF) < 512 ? rd16(p, pattr + 2u * ((b) & 0x3FF)) : rd16(p, sattr + 2u * (((b) & 0x3FF) - 512))) & 0xFF
	#define ISWATER(beh) (((beh) >= 0x10 && (beh) <= 0x15 && (beh) != 0x13) || (beh) == 0x19 || (beh) == 0x22 || ((beh) >= 0x50 && (beh) <= 0x53))
	for (int i = 0; i < w * h * 16; i++) prev[i] = -2;
	int qh = 0, qt = 0, startTile = sy * w + sx;
	int start = startTile * 16 + ((BLK(sx, sy) >> 12) & 0xF);
	prev[start] = -1;
	queue[qt++] = start;
	static const int DX[4] = {0, 0, -1, 1}, DY[4] = {1, -1, 0, 0};
	static const unsigned JUMP[4] = {0x3B, 0x3A, 0x39, 0x38};
	int found = -1;
	while (qh < qt && found < 0) {
		int cur = queue[qh++], ct = cur / 16, ce0 = cur % 16, cx = ct % w, cy = ct / w;
		// On water means surfing (the start tile tells for the start).
		bool curSurf = ct == startTile ? surfing : ISWATER(BEH(BLK(cx, cy)));
		for (int d = 0; d < 4; d++) {
			int nx = cx + DX[d], ny = cy + DY[d], ce = ce0;
			if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
			unsigned b = BLK(nx, ny), beh = BEH(b);
			int ex = (b >> 12) & 0xF;
			if (beh >= 0x38 && beh <= 0x3B) {
				if (beh != JUMP[d] || curSurf) continue;
				nx += DX[d]; ny += DY[d];
				if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
				b = BLK(nx, ny); beh = BEH(b); ex = (b >> 12) & 0xF; ce = ex;
			}
			// A muddy slope only slides the player down (it needs the Mach
			// Bike to climb): enter it going south only.
			if ((beh == 0xD0 || beh == 0xD1) && d != 0) continue;   // muddy, bumpy (Acro Bike)
			if (beh == 0x13) continue;   // a waterfall: WATERFALL, not walked or surfed
			// One-way edges (MB_IMPASSABLE_*: cave ledges and walls): moving
			// in direction d is refused when this tile is blocked on its d
			// side or the next on the opposite side (event_object_movement.c,
			// IsMetatileDirectionallyImpassable).
			{
				static const int OPP[4] = {1, 0, 3, 2};   // S<->N, W<->E
				if (sideBlocked(BEH(BLK(cx, cy)), d) || sideBlocked(beh, OPP[d])) continue;
			}
			int ti = ny * w + nx;
			if (blocked[ti]) continue;
			// Stay on this map unless the target is off it (a connected map).
			if (goalInside && (nx < 7 || ny < 7 || nx >= 7 + lw || ny >= 7 + lh)) continue;
			bool goal = nx == gx && ny == gy; // a door warp tile has collision set
			if (((b >> 10) & 3) && !goal) continue;
			bool water = ISWATER(beh);
			// Surfing: on along the water, or off onto land (the game
			// dismounts there); once on land, land only (Surf is used
			// from the bank with A, not walked into).
			if (water && !curSurf) continue;
			if (!goal && !(ex == 0 || ex == 15 || ce == 0 || ce == 15 || ex == ce) && !(curSurf && !water)) continue;
			// As ObjectEventUpdateElevation: a transition tile (0) sets the
			// player's elevation to 0 (any level next); a bridge (15) keeps it.
			int ne = ex == 15 ? ce : ex;
			int ni = ti * 16 + ne;
			if (prev[ni] != -2) continue;
			prev[ni] = cur;
			queue[qt++] = ni;
			if (goal) { found = ni; break; }
		}
	}
	#undef BLK
	#undef BEH
	#undef ISWATER
	if (found < 0) {
		static int dbg = -1; if (dbg < 0) dbg = getenv("WALK_DEBUG") != NULL;
		if (dbg && (p->noPath % 600) == 0) {
			// The search's view around the player: reached tiles (*), blocked
			// objects (O), warps (W), collision (#), water (~), else elevation.
			for (int yy = sy - 8; yy <= sy + 12; yy++) {
				char row[64]; int o = 0;
				for (int xx = sx - 12; xx <= sx + 12 && o < 60; xx++) {
					char ch = ' ';
					if (xx >= 0 && yy >= 0 && xx < w && yy < h) {
						unsigned b = rd16(p, map + 2u * (unsigned)(yy * w + xx)); unsigned beh = ((b & 0x3FF) < 512 ? rd16(p, pattr + 2u * (b & 0x3FF)) : rd16(p, sattr + 2u * ((b & 0x3FF) - 512))) & 0xFF; int ti = yy * w + xx; bool seen = false;
						for (int e = 0; e < 16; e++) if (prev[ti * 16 + e] != -2) seen = true;
						ch = xx == sx && yy == sy ? '@' : seen ? '*' : blocked[ti] == 1 ? 'O' : blocked[ti] == 2 ? 'W' : ((b >> 10) & 3) ? '#' : (((beh) >= 0x10 && (beh) <= 0x15 && (beh) != 0x13) || (beh) == 0x19 || (beh) == 0x22 || ((beh) >= 0x50 && (beh) <= 0x53)) ? '~' : "0123456789abcdef"[(b >> 12) & 15];
					}
					row[o++] = ch;
				}
				row[o] = 0;
				out("[p%d walkdbg y%3d] %s\n", p->id, yy - 7, row);
			}
		}
		return -1;
	}
	int g = found;
	for (int guard = 0; prev[g] != start && prev[g] >= 0; guard++) {
		if (guard > w * h * 16) { out("[p%d] walk: path loop\n", p->id); return -1; }
		g = prev[g];
	}
	g /= 16;
	int fx = g % w, fy = g / w;
	if (fx > sx) return 1 << 4;
	if (fx < sx) return 1 << 5;
	if (fy < sy) return 1 << 6;
	return 1 << 7;
}

// Debug aid (SNAP_EVERY=N SNAP_DIR=dir): every N frames in battle, write
// this core's VRAM (96 KB), OAM, palette RAM and the first 0x60 I/O bytes
// to dir/p<id>_<frame>.bin, to inspect graphics state offline.
static void snapVideo(struct Player* p) {
	static int every = -1;
	static const char* dir;
	if (every < 0) {
		const char* e = getenv("SNAP_EVERY");
		every = e ? atoi(e) : 0;
		dir = getenv("SNAP_DIR");
		if (!dir) every = 0;
	}
	if (every <= 0 || p->frame % every) return;
	if (!(rd8(p, sMain + 0x439) & 2)) return;
	char path[600];
	snprintf(path, sizeof(path), "%s/p%d_%06u.bin", dir, p->id, p->frame);
	FILE* f = fopen(path, "wb");
	if (!f) return;
	static const struct { unsigned base, size; } regions[] = {
		{ 0x06000000, 0x18000 }, { 0x07000000, 0x400 }, { 0x05000000, 0x400 }, { 0x04000000, 0x60 },
	};
	for (unsigned r = 0; r < 4; r++)
		for (unsigned a = 0; a < regions[r].size; a += 4) {
			uint32_t v = p->core->busRead32(p->core, regions[r].base + a);
			fwrite(&v, 4, 1, f);
		}
	fclose(f);
	char name[64];
	snprintf(name, sizeof(name), "snap_%06u", p->frame);
	writePng(p, name);
}

// Debug aid (BATTLE_LOG=1): in battle, print every new battle message with
// each battler's species, HP and absent bit, so who acted on what can be
// read from the log instead of judged from the screen.
static void battleLog(struct Player* p) {
	static int on = -1;
	static char last[2][440];
	if (on < 0) on = getenv("BATTLE_LOG") ? (atoi(getenv("BATTLE_LOG")) == 2 ? 2 : 1) : 0;
	if (!on) return;
	if (!(rd8(p, sMain + 0x439) & 2)) { last[p->id & 1][0] = 0; return; }
	if (on == 2) {
		// BATTLE_LOG=2: also each controller function and move-animation
		// state change, per battler.
		static unsigned lastCtl[2][4], lastState[2][4], lastActive[2];
		unsigned funcs = symAddr("gBattlerControllerFuncs");
		unsigned hb = rd32(p, rd32(p, symAddr("gBattleSpritesDataPtr")) + 4);
		unsigned active = rd8(p, symAddr("gAnimScriptActive"));
		for (int b = 0; b < 4; b++) {
			unsigned f = rd32(p, funcs + b * 4), st = hb ? rd8(p, hb + b * 16 + 4) | (rd8(p, hb + b * 16) << 8) : 0;
			if (f != lastCtl[p->id & 1][b] || st != lastState[p->id & 1][b])
				out("[f%06d p%d ctl] b%d %s state=%u flags=%02x anim=%u exec=%08x\n", p->frame, p->id, b, symName(f), st & 0xFF, st >> 8,
				    active, rd32(p, symAddr("gBattleControllerExecFlags")));
			lastCtl[p->id & 1][b] = f; lastState[p->id & 1][b] = st;
		}
		if (active != lastActive[p->id & 1])
			out("[f%06d p%d ctl] gAnimScriptActive=%u\n", p->frame, p->id, active);
		// The live task list, whenever its functions change (in battle).
		{
			static char lastTasks[2][600];
			char cur[600]; int o = 0;
			for (int t = 0; t < 16 && o < 560; t++) {
				unsigned tb = symAddr("gTasks") + t * 40;
				if (rd8(p, tb + 4)) o += snprintf(cur + o, sizeof(cur) - o, " %d:%s@%08x/p%u", t, symName(rd32(p, tb)), rd32(p, tb), rd8(p, tb + 7));
			}
			cur[o] = 0;
			if (strcmp(cur, lastTasks[p->id & 1])) {
				out("[f%06d p%d tasks]%s\n", p->frame, p->id, cur);
				snprintf(lastTasks[p->id & 1], sizeof(lastTasks[0]), "%s", cur);
			}
		}
		// A move animation running 600 frames: what it waits on (the cry
		// player, its channel, and the live tasks).
		static unsigned animSince[2];
		if (!active) animSince[p->id & 1] = 0;
		else if (!animSince[p->id & 1]) animSince[p->id & 1] = p->frame;
		else if (p->frame - animSince[p->id & 1] == 600) {
			unsigned cry = rd32(p, symAddr("gMPlay_PokemonCry"));
			out("[f%06d p%d stuck-anim] gMPlay_PokemonCry=%08x", p->frame, p->id, cry);
			if (cry) {
				unsigned tr = rd32(p, cry + 44), ch = tr ? rd32(p, tr + 32) : 0;
				out(" status=%08x track0=%08x flags=%02x chan=%08x", rd32(p, cry + 4), tr, tr ? rd8(p, tr) : 0, ch);
				if (ch) out(" chanStatus=%02x chanTrack=%08x wav=%08x count=%08x", rd8(p, ch), rd32(p, ch + 44), rd32(p, ch + 36), rd32(p, ch + 24));
			}
			out(" visualTasks=%u soundTasks=%u\n", rd8(p, symAddr("gAnimVisualTaskCount")), rd8(p, symAddr("gAnimSoundTaskCount")));
			for (int t = 0; t < 16; t++) {
				unsigned tb = symAddr("gTasks") + t * 40;
				if (rd8(p, tb + 4)) out("[f%06d p%d stuck-anim] task %d %s data0=%04x data9=%04x\n", p->frame, p->id, t, symName(rd32(p, tb)),
				                        rd16(p, tb + 8), rd16(p, tb + 8 + 18));
			}
		}
		if (p->frame % 300 == 0) {
			struct ARMCore* cpu = ((struct GBA*)p->core->board)->cpu;
			char pc[120];
			snprintf(pc, sizeof(pc), "%s", symAt(cpu->gprs[15]));
			out("[f%06d p%d cpu] pc=%s lr=%s\n", p->frame, p->id, pc, symAt(cpu->gprs[14]));
		}
		lastActive[p->id & 1] = active;
	}
	char text[440];
	gameText(p, symAddr("gDisplayedStringBattle"), text, sizeof(text));
	if (!text[0] || !strcmp(text, last[p->id & 1])) return;
	snprintf(last[p->id & 1], sizeof(last[0]), "%s", text);
	unsigned mons = symAddr("gBattleMons"), absent = rd8(p, symAddr("gAbsentBattlerFlags"));
	unsigned count = rd8(p, symAddr("gBattlersCount"));
	char state[160]; int n = 0;
	for (unsigned b = 0; b < count && b < 4; b++)
		n += snprintf(state + n, sizeof(state) - n, " b%u=%u:%u/%u%s", b, rd16(p, mons + b * 0x8C),
		              rd16(p, mons + b * 0x8C + 42), rd16(p, mons + b * 0x8C + 46), (absent >> b) & 1 ? "(absent)" : "");
	out("[f%06d p%d battle] %s |%s\n", p->frame, p->id, text, state);
	// BATTLE_PARTY=1: the local player and partner party arrays (species:HP)
	// with each battle line, to follow switches and replacements.
	static int partyLog = -1;
	if (partyLog < 0) { const char* e = getenv("BATTLE_PARTY"); partyLog = e && e[0] == '1'; }
	if (partyLog) {
		char line[300]; int o = 0;
		for (int tr = 0; tr < 3; tr += 2) {
			o += snprintf(line + o, sizeof(line) - o, "%s[", tr ? " partner" : "player");
			for (int sl = 0; sl < 6; sl++) {
				unsigned mon = partyMon(tr, sl), pers = rd32(p, mon), key = pers ^ rd32(p, mon + 4);
				unsigned sp = (rd32(p, mon + 32 + sSub0[pers % 24] * 12) ^ key) & 0x7FF;
				o += snprintf(line + o, sizeof(line) - o, "%u:%u ", (rd8(p, mon + 19) & 2) ? sp : 0, rd16(p, mon + 86));
			}
			o += snprintf(line + o, sizeof(line) - o, "]");
		}
		unsigned ord = symAddr("gBattlePartyCurrentOrder");
		out("[f%06d p%d bparty] %s order=%02x%02x%02x idx=%u,%u,%u,%u\n", p->frame, p->id, line, rd8(p, ord), rd8(p, ord + 1), rd8(p, ord + 2),
		    rd16(p, symAddr("gBattlerPartyIndexes")), rd16(p, symAddr("gBattlerPartyIndexes") + 2),
		    rd16(p, symAddr("gBattlerPartyIndexes") + 4), rd16(p, symAddr("gBattlerPartyIndexes") + 6));
	}
}

// kill -USR1 <pid>: each core dumps its state (field, battle, screenshot)
// on its next frame, for a run that has stopped making progress.
static volatile sig_atomic_t sUsr1[2];
static void onUsr1(int sig) { (void)sig; sUsr1[0] = sUsr1[1] = 1; }


// Leak counters on this game: active tasks, sprites in use, sprite palette
// tags held, heap blocks allocated and their bytes (walking gHeap's chain).
static void leakCounts(struct Player* p, int* out)
{
	unsigned tasks = symAddr("gTasks"), spr = symAddr("gSprites"), tags = symAddr("sSpritePaletteTags");
	unsigned heap = symAddr("gHeap");
	int nt = 0, ns = 0, np = 0, nb = 0, bytes = 0;
	for (int i = 0; i < 16; i++) if (rd8(p, tasks + i * 0x28 + 4)) nt++;
	for (int i = 0; i < 64; i++) if (rd8(p, spr + i * 0x44 + 0x3E) & 1) ns++;
	for (int i = 0; i < 16; i++) if (rd16(p, tags + i * 2) != 0xFFFF) np++;
	unsigned b = heap;
	for (int k = 0; k < 400; k++) {
		unsigned w0 = rd32(p, b), w1 = rd32(p, b + 4);
		if ((w0 >> 16) != 0xA3A3) break;           // magic
		if (w0 & 1) { nb++; bytes += w1 & 0x3FFFF; }
		unsigned next = rd32(p, b + 12);
		if (next == heap || next == b) break;
		b = next;
	}
	out[0] = nt; out[1] = ns; out[2] = np; out[3] = nb; out[4] = bytes;
}

static bool taskActive(struct Player* p, const char* name) {
	// gTasks[16]: func at +0, isActive at +4, 0x28 bytes each.
	unsigned tasks = symAddr("gTasks"), fn = symAddr(name) & ~1u;
	for (int t = 0; t < 16; t++)
		if (rd8(p, tasks + t * 0x28 + 4) && (rd32(p, tasks + t * 0x28) & ~1u) == fn) return true;
	return false;
}

static void frameCallback(struct mCoreThread* t) {
	struct Player* p = t->userData;
	p->frame++;
	if (p->frame == 1 && !getenv("NO_RESET_TRAP")) installResetTrap(p);
	if (p->frame == 1) {
		if (!sCb2Addr) { sCb2Addr = sMain + 4; sCopyrightCb2 = symFind("CB2_InitCopyrightScreenAfterBootup") & ~1u; }
		installWatch(p);
	}
	// An unexpected reset (the boot screen again, or a reset instruction):
	// a failure, and the scenario stops after this dump.
	if (p->unexpectedReset == 1) {
		p->unexpectedReset = 2;
		out("[f%06d p%d] UNEXPECTED RESET (boot screen again)\n", p->frame, p->id);
		dump(p, "reset");
		sFailures++;
		snprintf(sAbortWhy, sizeof(sAbortWhy), "p%d unexpected reset", p->id);
		sAbortRun = 1;
	}
	if (sUsr1[p->id]) {
		sUsr1[p->id] = 0;
		out("[f%06d p%d usr1] cmd %d/%d step %d\n", p->frame, p->id, p->pc, p->ncmds, p->step);
		dump(p, "usr1");
		battleDump(p);
		for (int tr = 0; tr < 4; tr++) {
			char line[200]; int o = 0;
			for (int sl = 0; sl < 6; sl++) {
				unsigned mon = partyMon(tr, sl), pers = rd32(p, mon), key = pers ^ rd32(p, mon + 4);
				unsigned sp = (rd32(p, mon + 32 + sSub0[pers % 24] * 12) ^ key) & 0x7FF;
				o += snprintf(line + o, sizeof(line) - o, "%u:%u/%u ", (rd8(p, mon + 19) & 2) ? sp : 0, rd16(p, mon + 86), rd16(p, mon + 88));
			}
			out("[f%06d p%d usr1 party%d] %s\n", p->frame, p->id, tr, line);
		}
		out("[f%06d p%d usr1 idx] partyIdx=%04x %04x %04x %04x absent=%02x hitmarker=%08x\n", p->frame, p->id,
		    rd16(p, symAddr("gBattlerPartyIndexes")), rd16(p, symAddr("gBattlerPartyIndexes") + 2),
		    rd16(p, symAddr("gBattlerPartyIndexes") + 4), rd16(p, symAddr("gBattlerPartyIndexes") + 6),
		    rd8(p, symAddr("gAbsentBattlerFlags")), rd32(p, symAddr("gHitMarker")));
		writePng(p, "usr1");
	}
	snapVideo(p);
		{
			static unsigned sess;
			if (!sess) sess = symFind("Task_CoopSession") & ~1u;
			unsigned nt = 0;
			for (int t = 0; sess && t < 16; t++) {
				unsigned tb = symAddr("gTasks") + t * 40;
				if (rd8(p, tb + 4) && (rd32(p, tb) & ~1u) == sess) nt++;
			}
			if (nt > p->maxSessionTasks) p->maxSessionTasks = nt;
		}
	battleLog(p);
	// FIXTURE `no_wild 1`: hold wild encounters off (map loads reset it).
	if (p->noWild) p->core->busWrite8(p->core, symAddr("sWildEncountersDisabled"), 1);
	// FIXTURE: battle-only stat boost for this player's battlers (0 and 2):
	// attack..spDefense in gBattleMons, never written back to the party.
	// In a two-player link battle (no MULTI) the cable slave's own
	// battlers are 1 and 3; co-op battles put both humans on 0 and 2.
	unsigned bflags = rd32(p, sBattleFlags);
	int own = ((bflags & 2) && !(bflags & 0x40) && !(bflags & 4)) ? 1 : 0;
	// FIXTURE: weak_foes 1 - on the battle engine's game, the opposing
	// battlers (1 and 3) are held at 1 HP, so any hit knocks them out (a
	// flow test's win, not balance).
	if (p->weakFoes && (rd8(p, sMain + 0x439) & 2)) {
		unsigned bm = symAddr("gBattleMons"), bmSize = symValue("harness.battleMonSize"), bmHp = symValue("harness.battleMonHp");
		for (int b = 1; b < 4; b += 2)
			if (rd16(p, bm + b * bmSize) && rd16(p, bm + b * bmSize + bmHp) > 1)
				p->core->busWrite16(p->core, bm + b * bmSize + bmHp, 1);
	}
	if (p->strongBattlers && (rd8(p, sMain + 0x439) & 2)) {
		unsigned mons = symAddr("gBattleMons");
		for (int b = own; b < 4; b += 2)
			if (rd16(p, mons + b * 0x8C))
				for (int k = 0; k < 5; k++) p->core->busWrite16(p->core, mons + b * 0x8C + 2 + k * 2, p->strongBattlers);
	}
	// Test driver, not game state: in battle, keep the move cursor of this
	// player's battlers (0 and 2) on the strongest damaging move with PP, so
	// mashing A attacks instead of looping on a status move or "no PP left".
	if (p->smartMoves && p->forceMove < 0 && (rd8(p, sMain + 0x439) & 2)) {
		// The move menu's own data (ChooseMoveStruct at bufferA[b] + 4:
		// moves u16[4], currentPP u8[4] at +8). Unlike gBattleMons it is
		// filled on the guest too, which runs controllers only.
		unsigned res = rd32(p, symAddr("gBattleResources")), cur = symAddr("gMoveSelectionCursor"), info = symAddr("gMovesInfo");
		// Score = power x type effectiveness against the healthier-looking
		// foe x STAB (when gBattleMons is filled; the guest's may trail).
		unsigned bm = symAddr("gBattleMons"), bmSize = symValue("harness.battleMonSize"), bmTypes = symValue("harness.battleMonTypes");
		unsigned tsz = symValue("harness.typeSize"), ntypes = symValue("harness.numTypes"), bmHp = symValue("harness.battleMonHp");
		unsigned chart = symAddr("gTypeEffectivenessTable"), esz = symValue("harness.effSize");
		for (int b = own; res && b < 4; b += 2) {
			unsigned m = res + 0x10 + b * 0x200 + 4;
			int best = -1; unsigned bestPow = 0; int bestFoe = -1;
			for (int i = 0; i < 4; i++) {
				int foeHere = -1;
				unsigned mv = rd16(p, m + i * 2), pp = rd8(p, m + 8 + i);
				if (!mv || !pp || mv >= 0x0E9C0u / 0x40) continue;
				{
					// Conditional moves (Belch, Dream Eater, ...): the game refuses
					// or wastes them; never the driver's pick.
					static const char* cm[] = { "harness.condMove0", "harness.condMove1", "harness.condMove2", "harness.condMove3", "harness.condMove4" };
					int skip = 0;
					for (int k = 0; k < 5; k++) if (mv == symValue(cm[k])) skip = 1;
					if (skip) continue;
				}
				unsigned word = rd32(p, info + mv * 0x40 + 8);
				unsigned pow = word >> 23, mtype = (word >> 16) & 0x1F;
				if (pow && bmSize && chart) {
					unsigned bestEff = 0; int anyFoe = 0;
					for (int f = (b & 1) ^ 1; f < 4; f += 2) {
						unsigned fm = bm + f * bmSize;
						if (!rd16(p, fm) || !rd16(p, fm + bmHp)) continue;
						unsigned eff = 0x1000;
						for (int t = 0; t < 2; t++) {
							unsigned ft = tsz == 1 ? rd8(p, fm + bmTypes + t) : rd32(p, fm + bmTypes + t * tsz);
							if (t == 1 && ft == (tsz == 1 ? rd8(p, fm + bmTypes) : rd32(p, fm + bmTypes))) break;
							if (ft < ntypes && mtype < ntypes) eff = eff * rd16(p, chart + (mtype * ntypes + ft) * esz) / 0x1000;
						}
						anyFoe = 1;
						// Equal effect: aim at the weaker foe (finish it).
						if (eff > bestEff || (eff == bestEff && foeHere >= 0
						    && rd16(p, fm + bmHp) < rd16(p, bm + foeHere * bmSize + bmHp))) { bestEff = eff; foeHere = f; }
					}
					if (anyFoe) pow = pow * bestEff / 0x1000 * 4;
					else pow *= 4;
					unsigned am = bm + b * bmSize;
					for (int t = 0; t < 2 && rd16(p, am); t++) {
						unsigned at = tsz == 1 ? rd8(p, am + bmTypes + t) : rd32(p, am + bmTypes + t * tsz);
						if (at == mtype) { pow = pow * 3 / 2; break; }
					}
				}
				// smart_moves 2: a later move wins a tie (Ember over Scratch
				// against a Ghost); 1 keeps the first.
				if (best < 0 || pow > bestPow || (p->smartMoves == 2 && pow == bestPow)) { best = i; bestPow = pow; bestFoe = foeHere; }
			}
			if (best >= 0) p->core->busWrite8(p->core, cur + b, best);
			// Aim at the foe that move hurts most when the game asks for a
			// target (doubles).
			if (bestFoe >= 0 && !strcmp(symName(rd32(p, symAddr("gBattlerControllerFuncs") + b * 4)), "HandleInputChooseTarget"))
				p->core->busWrite8(p->core, symAddr("gMultiUsePlayerCursor"), bestFoe);
		}
	}
	// force_move SLOT [TARGET] (driver choice, not game state): this player's
	// battlers pick move SLOT; in the target menu, battler TARGET (4: the
	// ally, so support moves land on the partner's Pokemon).
	if (p->forceMove >= 0 && (rd8(p, sMain + 0x439) & 2)) {
		unsigned funcs = symAddr("gBattlerControllerFuncs");
		for (int b = own; b < 4; b += 2) {
			const char* fn = symName(rd32(p, funcs + b * 4));
			if (!strcmp(fn, "HandleInputChooseMove")) {
				if (getenv("FORCE_DEBUG") && rd8(p, symAddr("gMoveSelectionCursor") + b) != (unsigned)p->forceMove)
					out("[f%06d p%d force] battler %d cursor %u -> %d\n", p->frame, p->id, b, rd8(p, symAddr("gMoveSelectionCursor") + b), p->forceMove);
				p->core->busWrite8(p->core, symAddr("gMoveSelectionCursor") + b, p->forceMove);
			}
			else if (!strcmp(fn, "HandleInputChooseTarget") && p->forceTarget >= 0)
				p->core->busWrite8(p->core, symAddr("gMultiUsePlayerCursor"), p->forceTarget == 4 ? (b ^ 2) : p->forceTarget);
		}
	}
    // Controlled application-heartbeat outage: serial hardware keeps running.
    // Patch only this emulated core's callback, and always restore its opcode.
    if (p->muteFrames && --p->muteFrames == 0)
        p->core->rawWrite16(p->core, symAddr("Coop_LinkCallback"), -1, p->mutedOpcode);
    if (p->patchAddr && p->patchFrames && --p->patchFrames == 0) {
        p->core->rawWrite16(p->core, p->patchAddr, -1, p->patchOpcode);
        p->patchAddr = 0;
    }
	{
		// Co-op block traffic: each new co-op block in the send buffer, and
		// co-op blocks the battle code set aside (gCoopStrayBattleBlocks).
		static unsigned lastSend[2][4], lastStray[2];
		unsigned sb = symAddr("gBlockSendBuffer"), sa = symAddr("gCoopStrayBattleBlocks");
		if (sb && getenv("BLOCK_LOG")) {
			unsigned w[4];
			for (int k = 0; k < 4; k++) w[k] = rd32(p, sb + k * 4);
			if (memcmp(w, lastSend[p->id], sizeof(w))) {
				memcpy(lastSend[p->id], w, sizeof(w));
				if ((w[0] & 0xFF00) == 0xC000)
					out("[f%06d p%d SEND] %08x %08x %08x %08x inBattle=%u\n", p->frame, p->id, w[0], w[1], w[2], w[3], rd8(p, sMain ? sMain + 0x439 : 0) & 2);
			}
		}
		static unsigned lastRecv[2][2];
		unsigned rb = symAddr("gBlockRecvBuffer"), rs = symAddr("gBlockReceivedStatus");
		for (int k = 0; rb && getenv("BLOCK_LOG") && k < 2; k++) {
			unsigned w0 = rd32(p, rb + k * 256), w3 = rd32(p, rb + k * 256 + 12);
			unsigned sig = ((w0 & 0xFF00) == 0xC000) ? (w0 ^ w3) | 1 : 0;
			if (sig != lastRecv[p->id][k]) {
				lastRecv[p->id][k] = sig;
				if (sig) out("[f%06d p%d RECV] slot %d %08x .. %08x flag=%u\n", p->frame, p->id, k, w0, w3, rd8(p, rs + k));
			}
		}
		if (sa && rd16(p, sa) != lastStray[p->id]) {
			lastStray[p->id] = rd16(p, sa);
			out("[f%06d p%d STRAY-BLOCK] co-op blocks set aside by battle reads: %u\n", p->frame, p->id, lastStray[p->id]);
		}
	}
	if (sMain) {
		unsigned cb2 = rd32(p, sMain + 4);
		if (cb2 != sLastCb2[p->id]) {
			sLastCb2[p->id] = cb2;
			const char* nm = symName(cb2);
			if (sCb2Logs[p->id]++ < 30 || strstr(nm, "LinkError") || strstr(nm, "ErrorMessage") || strstr(nm, "SoftReset") || strstr(nm, "Copyright") || strstr(nm, "ContinueSavedGame") || strstr(nm, "CrashScreen"))
				out("[f%06d p%d] cb2 %08x %s\n", p->frame, p->id, cb2, nm);
			// A link error screen is never expected in co-op play: record it.
			if (strstr(nm, "CB2_LinkError")) {
				out("[f%06d p%d] LINK ERROR SCREEN: wireless=%u linkStatus=%08x errOccurred=%u suppress=%u hold=%u recvPlayers=%u\n", p->frame, p->id,
				    rd8(p, symAddr("gWirelessCommType")), rd32(p, symAddr("gLinkStatus")), rd8(p, symAddr("gLinkErrorOccurred")),
				    rd8(p, symAddr("gSuppressLinkErrorMessage")), rd8(p, symAddr("gCoopHoldLink")), rd8(p, symAddr("gReceivedRemoteLinkPlayers")));
				dump(p, "linkerror");
				battleDump(p);
				writePng(p, "linkerror");
			}
		}
	}
	// PC box watch: the first 12 slots of box 1 should only change when the
	// game deposits a Pokemon. Log every change with where the game was.
	if ((p->frame & 7) == 0 && p->frame > 600) {
		unsigned st = rd32(p, symAddr("gPokemonStoragePtr"));
		if (st >= 0x02000000 && st < 0x02040000) {
			unsigned sum = 0;
			for (int k = 0; k < 240; k++) sum = sum * 31 + rd32(p, st + 4 + k * 4);
			if (p->boxSumSet && sum != p->boxSum) {
				struct ARMCore* cpu = ((struct GBA*)p->core->board)->cpu;
				char diff[200]; int dn = 0, nd = 0;
				for (int k = 0; k < 240; k++) {
					unsigned w = rd32(p, st + 4 + k * 4);
					if (w != p->boxCopy[k]) { if (nd < 4 && dn < 180) dn += snprintf(diff + dn, sizeof(diff) - dn, " +%03x:%08x>%08x", k * 4, p->boxCopy[k], w); nd++; }
				}
				out("[f%06d p%d BOXWATCH] box 1 changed (%d words%s): cb1=%s cb2=%s pc=%s lr=%s\n", p->frame, p->id, nd, nd ? diff : "",
				    symName(rd32(p, sMain)), symName(rd32(p, sMain + 4)), symAt(cpu->gprs[15]), symAt(cpu->gprs[14]));
			}
			for (int k = 0; k < 240; k++) p->boxCopy[k] = rd32(p, st + 4 + k * 4);
			p->boxSum = sum; p->boxSumSet = 1;
			if (getenv("WATCH_BOX") && !sWatchQuiet[p->id]) { installWatch(p); sWatchLo[p->id] = st + 4; sWatchHi[p->id] = st + 4 + 960; }
		}
	}
	// An assert crash screen (debug builds) waits for START. Record it once
	// with the trainer-battle state, count a failure, then press past it.
	if ((p->frame & 15) == 0 && !p->crashStart) {
		static unsigned crashLo, crashHi;
		if (!crashLo) {
			crashLo = symAddr("CrashScreen");
			crashHi = 0xFFFFFFFFu;
			for (int i = 0; i < sNumSyms; i++)
				if ((sSyms[i].type == 'T' || sSyms[i].type == 't') && (sSyms[i].addr & ~1u) > crashLo && (sSyms[i].addr & ~1u) < crashHi)
					crashHi = sSyms[i].addr & ~1u;
		}
		struct ARMCore* cpu = ((struct GBA*)p->core->board)->cpu;
		// The crash screen busy-waits on the keypad inside CrashScreen.
		unsigned pc = cpu->gprs[15] & ~1u;
		bool inCrash = pc >= crashLo && pc < crashHi;
		if (inCrash) {
			unsigned tbp = symAddr("gTrainerBattleParameter"), at = symAddr("gApproachingTrainers");
			char hex[200]; int n = 0;
			for (int k = 0; k < 40 && n < 190; k++) n += snprintf(hex + n, sizeof(hex) - n, "%02x", rd8(p, tbp + k));
			out("[f%06d p%d] ASSERT CRASH SCREEN: trainerParams=%s approaching=%u lastTalked=%u selectedObj=%u battleFlags=%08x\n", p->frame, p->id, hex,
			    rd8(p, symAddr("gNoOfApproachingTrainers")), rd16(p, symAddr("gSpecialVar_LastTalked")), rd8(p, symAddr("gSelectedObjectEvent")), rd32(p, symAddr("gBattleTypeFlags")));
			for (int k = 0; k < 2; k++)
				out("[f%06d p%d] approaching[%d] obj=%u script=%s radius=%u\n", p->frame, p->id, k, rd8(p, at + k * 12), symAtAny(rd32(p, at + k * 12 + 4)), rd8(p, at + k * 12 + 1));
			for (int k = 0; k < 16; k++) {
				unsigned o = sObjEvents + k * 0x24;
				if (!(rd8(p, o) & 1)) continue;
				out("[f%06d p%d] obj%d localId=%u gfx=%u move=%u trainerType=%u range=%u at=(%d,%d) facing=%u map=%u.%u player=%u\n", p->frame, p->id, k,
				    rd8(p, o + 8), rd16(p, o + 4), rd8(p, o + 6), rd8(p, o + 7), rd8(p, o + 0x1D), (int16_t)rd16(p, o + 0x10) - 7, (int16_t)rd16(p, o + 0x12) - 7,
				    rd8(p, o + 0x18) & 15, rd8(p, o + 10), rd8(p, o + 9), (rd8(p, o + 2) & 1));
			}
			if (rd8(p, sMain + 0x439) & 2) {
				unsigned pos = symAddr("gBattlerPositions");
				out("[f%06d p%d] assert battle: attacker=%u target=%u positions=%u %u %u %u r0=%08x r1=%08x r2=%08x r3=%08x\n", p->frame, p->id,
				    rd8(p, symAddr("gBattlerAttacker")), rd8(p, symAddr("gBattlerTarget")), rd8(p, pos), rd8(p, pos + 1), rd8(p, pos + 2), rd8(p, pos + 3),
				    cpu->gprs[0], cpu->gprs[1], cpu->gprs[2], cpu->gprs[3]);
			}
			dump(p, "assert");
			battleDump(p);
			writePng(p, "assert");
			if (!p->crashSeen) sFailures++;
			p->crashSeen++;
			// Critical: the run stops once this screen has been pressed past
			// and dumped (no save from it is ever used: see quarantine).
			snprintf(sAbortWhy, sizeof(sAbortWhy), "p%d assert screen", p->id);
			sAbortRun = 2;
			p->crashStart = 8;
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

	// note_text: watch gStringVar4 every frame for the words (a message
	// that a later one replaces before any command could look).
	if (p->noteText[0] && !p->noteSeen) {
		char text[440];
		gameText(p, symAddr("gStringVar4"), text, sizeof(text));
		if (strstr(text, p->noteText)) { p->noteSeen = 1; out("[f%06d p%d] noted \"%s\"\n", p->frame, p->id, text); }
	}
	unsigned keys = 0;
	while (!p->done) {
		if (p->pc >= p->ncmds) { p->done = 1; break; }
		struct Cmd* c = &p->cmds[p->pc];
		bool advance = false;
		switch (c->kind) {
        case C_CROSS: {
            // Hold a direction (or nothing) until this save is on map
            // GROUP.NUM with field control.
            unsigned save = rd32(p, sSave1);
            unsigned mg = rd8(p, save + 4), mn = rd8(p, save + 5);
            if (mg == c->addr && mn == (unsigned)c->value && inField(p)) {
                if (++p->stable >= 20) { p->stable = 0; advance = true; }
                break;
            }
            p->stable = 0;
            if (inField(p)) keys = c->keys;
            else keys = autoKeys(p, (rd8(p, sMain + 0x439) & 2) ? p->offField : p->step);
            // A battle on the way (a hidden trainer, a wild encounter) does
            // not use up the map-change budget; it has its own cap.
            if ((rd8(p, sMain + 0x439) & 2) && p->offField < 60000u) { p->offField++; break; }
            p->offField = 0;
            if (++p->step >= c->frames) {
                out("[p%d] FAIL %s: on map %u.%u\n", p->id, c->text, mg, mn);
                dump(p, "fail");
                writePng(p, "fail");
                sFailures++;
                p->done = 1;
            }
            break;
        }
        case C_MIRROR: {
            // expect_mirror MINE THEIRS: a value on this game equals one on
            // the other game (use after a sync, at a quiet point).
            struct Player* o = c->args[0] ? p : &P[1 - p->id];
            unsigned mine = rdN(p, c->addr, c->size), theirs = rdN(o, (unsigned)c->value, c->size);
            if (mine != theirs) { out("[p%d] FAIL %s: %u, %s %u\n", p->id, c->text, mine, c->args[0] ? "expected (same game)" : "other game", theirs); sFailures++; }
            else out("[p%d] ok %s (%u)\n", p->id, c->text, mine);
            advance = true;
            break;
        }
        case C_STRONG:
            if (c->size) p->weakFoes = c->value;
            else p->strongBattlers = c->value;
            advance = true;
            break;
        case C_PARTY_SLOT: {
            // Only once the party menu is up and reading input.
            if ((rd32(p, sMain + 4) & ~1u) != (symAddr("CB2_UpdatePartyMenu") & ~1u) || !taskActive(p, "Task_HandleChooseMonInput")) {
                if (++p->step >= c->frames) { out("[p%d] FAIL party menu not ready for party_slot\n", p->id); sFailures++; advance = true; }
                break;
            }
            // In SWITCH (and Softboiled) mode the moving cursor is slotId2.
            unsigned act = rd8(p, symAddr("gPartyMenu") + 11);
            int slot = (signed char)rd8(p, symAddr("gPartyMenu") + ((act == 8 || act == 10) ? 10 : 9));
            if (slot == (int)c->value) { out("[p%d] party cursor at slot %d\n", p->id, slot); advance = true; break; }
            if (++p->step >= c->frames) { out("[p%d] FAIL party cursor at %d, wanted %u\n", p->id, slot, c->value); sFailures++; advance = true; break; }
            keys = (p->step % 16) == 0 ? (slot < (int)c->value ? (1u << 7) : (1u << 6)) : 0;
            break;
        }
        case C_HEAL_BELOW:
            p->healPct = c->value;
            p->healUses = c->size;
            p->healState = 0;
            p->healsLeft = -1;
            out("[p%d] heal_below %u%% (%d per battle)\n", p->id, c->value, c->size);
            advance = true;
            break;
        case C_SWITCH_LEAD:
            p->switchLead = c->value;
            p->switchState = 0;
            out("[p%d] switch_lead %u\n", p->id, c->value);
            advance = true;
            break;
        case C_SMART:
            p->smartMoves = c->value;
            advance = true;
            break;
        case C_NO_WILD:
            p->noWild = c->value;
            if (!c->value) p->core->busWrite8(p->core, symAddr("sWildEncountersDisabled"), 0);
            advance = true;
            break;
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
        case C_SPECIES: {
            // expect_species SLOT SPECIES (0 = empty slot)
            unsigned mon = partyMon(0, c->size), pid = rd32(p, mon), key = pid ^ rd32(p, mon + 4);
            unsigned w = rd32(p, mon + 32 + sSub0[pid % 24] * 12) ^ key;
            unsigned species = (rd8(p, mon + 19) & 2) ? (w & 0x7FF) : 0;
            if (species != c->value) { out("[p%d] FAIL slot %d species %u, expected %u\n", p->id, c->size, species, c->value); sFailures++; }
            else out("[p%d] ok slot %d species %u\n", p->id, c->size, species);
            advance = true;
            break;
        }
        case C_NOTE_PARTY:
        case C_SAME_PARTY: {
            // The whole local party, byte for byte (encrypted data, stats, HP, PP).
            unsigned char now[600];
            for (int i = 0; i < 600; i++) now[i] = rd8(p, partyMon(0, 0) + i);
            if (c->kind == C_NOTE_PARTY) { memcpy(p->partyNote, now, 600); out("[p%d] party noted\n", p->id); }
            else {
                int diff = -1;
                for (int i = 0; i < 600; i++) if (now[i] != p->partyNote[i]) { diff = i; break; }
                if (diff >= 0) { out("[p%d] FAIL party differs at byte %d (slot %d +%d)\n", p->id, diff, diff / 100, diff % 100); sFailures++; }
                else out("[p%d] ok party unchanged\n", p->id);
            }
            advance = true;
            break;
        }
        case C_PATCH_RET:
            // Make a function return at once for N frames (0: until the run
            // ends), on this core only: a controlled loss of one code path.
            if (p->patchAddr) p->core->rawWrite16(p->core, p->patchAddr, -1, p->patchOpcode);
            p->patchAddr = c->addr & ~1u;
            p->patchOpcode = rd16(p, p->patchAddr);
            p->core->rawWrite16(p->core, p->patchAddr, -1, 0x4770); // bx lr
            p->patchFrames = c->frames;
            out("[p%d] patched %s for %d frames\n", p->id, c->text, c->frames);
            advance = true;
            break;
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
		case C_SCREEN_SAME: {
			struct Player* o = &P[1 - p->id];
			unsigned x0 = c->args[0], y0 = c->args[1], w = c->args[2], h = c->args[3], diff = 0;
			for (unsigned y = y0; y < y0 + h && y < H; y++)
				for (unsigned x = x0; x < x0 + w && x < W; x++)
					if ((p->video[y * W + x] & 0xF8F8F8) != (o->video[y * W + x] & 0xF8F8F8)) diff++;
			if (diff > w * h / 50) { out("[p%d] FAIL %s: %u of %u pixels differ\n", p->id, c->text, diff, w * h); sFailures++; writePng(p, "screen-diff"); }
			else out("[p%d] ok %s (%u pixels differ)\n", p->id, c->text, diff);
			advance = true;
			break;
		}
		case C_MEGA_NEXT:
			p->megaPress = (int)c->value; p->megaPhase = 0; p->megaLeave = 0;
			out("[p%d] DRIVER mega_next %d\n", p->id, p->megaPress);
			advance = true;
			break;
		case C_FORCE_MOVE:
			p->forceMove = (int)c->value; p->forceTarget = (int)c->addr;
			out("[p%d] DRIVER force_move %d target %d\n", p->id, p->forceMove, p->forceTarget);
			advance = true;
			break;
		case C_TALK_OBJ: {
			// talk_obj LOCALID [T]: wait in place until that (moving) object
			// is next to the player, face it, press A; done once a script runs.
			if (!inField(p)) { if (++p->stable >= 2) { p->stable = 0; advance = true; } break; }
			p->stable = 0;
			unsigned me = sObjEvents + rd8(p, sAvatar + 5) * 0x24;
			int px = (int16_t)rd16(p, me + 0x10), py = (int16_t)rd16(p, me + 0x12);
			for (int k = 0; k < 16; k++) {
				unsigned o = sObjEvents + k * 0x24;
				if (!(rd8(p, o) & 1) || rd8(p, o + 8) != c->value || (rd8(p, o + 2) & 1)) continue;
				int ox = (int16_t)rd16(p, o + 0x10), oy = (int16_t)rd16(p, o + 0x12);
				int dx = ox - px, dy = oy - py;
				if (abs(dx) + abs(dy) != 1) break;
				unsigned dirKey = dx > 0 ? (1u << 4) : dx < 0 ? (1u << 5) : dy < 0 ? (1u << 6) : (1u << 7);
				unsigned want = dx > 0 ? 4 : dx < 0 ? 3 : dy < 0 ? 2 : 1; // DIR_EAST/WEST/NORTH/SOUTH
				if ((rd8(p, me + 0x18) & 15) != want) keys = (p->step & 7) < 2 ? dirKey : 0;
				else keys = (p->step & 7) == 4 ? 1 : 0;
				break;
			}
			if (++p->step >= c->frames) {
				out("[p%d] FAIL talk_obj %u: never next to it\n", p->id, c->value);
				dump(p, "fail"); writePng(p, "fail"); sFailures++; advance = true;
			}
			break;
		}
		case C_WALK_BATTLE:
			// walk_battle X Y T: walk toward X,Y until a battle starts.
			if (rd8(p, sMain + 0x439) & 2) { p->stable = 0; p->offField = 0; p->step = 0; advance = true; break; }
			if (p->offField > 1200u) {
				out("[p%d] FAIL walk_battle: off the field with no battle\n", p->id);
				dump(p, "fail"); writePng(p, "fail"); sFailures++; p->done = 1; break;
			}
			/* fall through */
		case C_WALK: {
			unsigned obj = sObjEvents + rd8(p, sAvatar + 5) * 0x24;
			int x = (int16_t)rd16(p, obj + 0x10) - 7, y = (int16_t)rd16(p, obj + 0x12) - 7;
			if (x == (int)c->addr && y == c->value) {
				if (++p->stable >= 16) { p->stable = 0; p->offField = 0; advance = true; }
				break;
			}
			p->stable = 0;
			bool field = inField(p);
			if (!field)
				keys = autoKeys(p, p->offField++); // a cutscene or a trainer's battle on the way
			if (field) {
				int k = walkStep(p, (int)c->addr, c->value);
				if (k > 0) keys = (unsigned)k;
				if (k < 0 && (p->step % 600) == 0)
					out("[f%06d p%d] walk: no path to (%d,%d) from (%d,%d)\n", p->frame, p->id, (int)c->addr, c->value, x, y);
				// No progress: no path for 30 s of field time ends the walk
				// now (with diagnostics) instead of at the end of its budget.
				p->noPath = k < 0 ? p->noPath + 1 : 0;
				if (p->noPath >= 1800) { p->noPath = 0; p->step = c->frames; }
			}
			// The budget counts overworld frames; battles and scenes on the
			// way have a separate, generous cap.
			if ((field ? ++p->step : 0) >= c->frames || p->offField >= 60000u) {
				p->offField = 0;
				if (c->size == 1) {
					// walk_try: exploration, not a failure.
					out("[f%06d p%d] walk_try (%d,%d) not reached, at (%d,%d)\n", p->frame, p->id, (int)c->addr, c->value, x, y);
					p->stable = 0; advance = true; break;
				}
				out("[p%d] FAIL walk (%d,%d) stuck at (%d,%d)\n", p->id, (int)c->addr, c->value, x, y);
				dump(p, "fail");
				writePng(p, "fail");
				sFailures++;
				p->done = 1;
			}
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
			unsigned base = c->value == 1 ? rd32(p, c->addr) + (unsigned)c->frames : c->addr; // hexp: through a pointer
			for (int i = 0; i < c->size && o < 580; i++)
				o += snprintf(line + o, sizeof(line) - o, "%02x%s", rd8(p, base + i), (i & 15) == 15 ? " | " : " ");
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
			// SYNC_PLAY=1 (the journey runner): waiting for the partner while
			// in a battle (a wild Pokemon on the last step, or the partner's
			// fight joined), play it out instead of standing at its first
			// message (a player would). Off for the regression scenarios,
			// which hold battles at a sync on purpose.
			if (!advance && getenv("SYNC_PLAY")) {
				static unsigned battleCb2;
				if (!battleCb2) battleCb2 = symAddr("BattleMainCB2") & ~1u;
				if ((rd32(p, sMain + 4) & ~1u) == battleCb2)
					keys = autoKeys(p, p->step++);
			}
			break;
		case C_UNTIL_TASK:
		case C_UNTIL_CB2:
		case C_UNTIL_FIELD:
		case C_UNTIL_MEM: {
			int nbytes = c->size < 0 ? -c->size : c->size;
			unsigned got = c->kind == C_UNTIL_MEM ? rdN(p, c->addr, nbytes) : 0;
			unsigned want = c->value;
			if ((want & 1u) && nbytes == 4) { got &= ~1u; want &= ~1u; }
			bool ok = false;
			if (c->kind == C_UNTIL_TASK) {
				// gTasks[16]: func at +0, isActive at +4, 0x28 bytes each.
				unsigned tasks = symAddr("gTasks");
				for (int t = 0; t < 16 && !ok; t++)
					ok = rd8(p, tasks + t * 0x28 + 4) && (rd32(p, tasks + t * 0x28) & ~1u) == c->addr;
			} else
			ok = c->kind == C_UNTIL_CB2 ? ((rd32(p, sMain + 4) & ~1u) == c->addr || cb2Seen(p, c->addr))
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
			bool inBattleNow = (rd8(p, sMain + 0x439) & 2) != 0;
			// mash_field with another key (B: NO to a nickname question
			// after the fight) still plays a battle on the way with the
			// battle keys; B alone backs out of every battle menu.
			if ((c->kind == C_UNTIL_FIELD || c->kind == C_UNTIL_CB2) && (c->keys == 1 || (c->kind == C_UNTIL_FIELD && c->keys && inBattleNow))) keys = autoKeys(p, c->kind == C_UNTIL_FIELD && inBattleNow ? p->offField : p->step);
			// (in a battle the budget's step stands still: its own count paces the keys)
			// mash_field: a battle on the way (wild, trainer) does not use up
			// the budget; it has its own generous cap.
			if (c->kind == C_UNTIL_FIELD && inBattleNow && p->offField < 60000u) { p->offField++; break; }
			if (!inBattleNow) p->offField = 0;
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
        case C_TINT: {
            unsigned faded = symAddr("gPlttBufferFaded"), unfaded = symAddr("gPlttBufferUnfaded");
            unsigned diff = 0;
            for (unsigned i = 2 * 16; i < 7 * 16; i++)
                if (rd16(p, faded + i * 2) != rd16(p, unfaded + i * 2)) diff++;
            unsigned tinted = diff > 8;
            if (tinted != c->value) { out("[p%d] FAIL tint %s: tinted=%u (%u colours differ), expected %u\n", p->id, c->text, tinted, diff, c->value); sFailures++; }
            else out("[p%d] ok tint %s: tinted=%u (%u colours differ)\n", p->id, c->text, tinted, diff);
            advance = true;
            break;
        }
        case C_LEAK: {
            int now[5];
            static const char* names[5] = { "tasks", "sprites", "sprite palettes", "heap blocks", "heap bytes" };
            leakCounts(p, now);
            {
                unsigned tags = symAddr("sSpritePaletteTags");
                char tb[160]; int o = 0;
                for (int i = 0; i < 16; i++) o += snprintf(tb + o, sizeof(tb) - o, "%s%04x", i ? " " : "", rd16(p, tags + i * 2));
                out("[p%d] leak %s palette tags: %s\n", p->id, c->text, tb);
                if (getenv("LEAK_SPRITES")) {
                    unsigned spr = symAddr("gSprites");
                    for (int i = 0; i < 64; i++) if (rd8(p, spr + i * 0x44 + 0x3E) & 1) {
                        unsigned t = rd32(p, spr + i * 0x44 + 0x14);
                        out("[p%d]   sprite %d pal %d tmplTag %04x cb %s\n", p->id, i, rd8(p, spr + i * 0x44 + 5) >> 4, t ? rd16(p, t + 2) : 0xFFFF, symName(rd32(p, spr + i * 0x44 + 0x1C)));
                    }
                }
            }
            if (!c->value) {
                memcpy(p->leakNote, now, sizeof(now)); p->leakNoted = true;
                out("[p%d] leak note %s: tasks %d sprites %d palettes %d heap %d blocks %d bytes\n", p->id, c->text, now[0], now[1], now[2], now[3], now[4]);
            } else {
                int bad = 0;
                for (int k = 0; k < 5; k++) if (now[k] != p->leakNote[k]) bad = 1;
                if (!p->leakNoted) { out("[p%d] FAIL leak_check without leak_note\n", p->id); sFailures++; }
                else if (bad) {
                    out("[p%d] FAIL leak %s:", p->id, c->text);
                    for (int k = 0; k < 5; k++) if (now[k] != p->leakNote[k]) out(" %s %d -> %d", names[k], p->leakNote[k], now[k]);
                    out("\n"); sFailures++;
                } else out("[p%d] ok no leak %s: tasks %d sprites %d palettes %d heap %d blocks %d bytes\n", p->id, c->text, now[0], now[1], now[2], now[3], now[4]);
            }
            advance = true;
            break;
        }
        case C_WRITEP: {
            unsigned addr = rd32(p, c->addr) + c->frames;
            if (c->args[1] == 3) {
                // expectp_same: the same value through the same pointer on
                // the other game (read byte by byte: may be unaligned).
                struct Player* o = &P[1 - p->id];
                unsigned oaddr = rd32(o, c->addr) + c->frames, mine = 0, theirs = 0;
                for (int k = c->size - 1; k >= 0; k--) {
                    mine = (mine << 8) | rd8(p, addr + k);
                    theirs = (theirs << 8) | rd8(o, oaddr + k);
                }
                if (mine != theirs) { out("[p%d] FAIL *%s+%u = %u, other game %u\n", p->id, c->text, c->frames, mine, theirs); sFailures++; }
                else out("[p%d] ok *%s+%u same on both (%u)\n", p->id, c->text, c->frames, mine);
                advance = true;
                break;
            }
            if (c->args[1] == 2) {
                // expectp: a value read through the pointer.
                unsigned v = rdN(p, addr, c->size);
                if (v != c->value) { out("[p%d] FAIL *%s+%u = %u, expected %u\n", p->id, c->text, c->frames, v, c->value); sFailures++; }
                else out("[p%d] ok *%s+%u == %u\n", p->id, c->text, c->frames, v);
                advance = true;
                break;
            }
            if (c->args[1] == 1) {
                // writep_other: the value comes from the other game.
                struct Player* o = &P[1 - p->id];
                // Byte reads: the source may be unaligned (the trainer id
                // sits at SaveBlock2+10, where a 32-bit bus read rotates).
                unsigned src = rd32(o, c->args[2]) + c->args[3];
                c->value = 0;
                for (int k = c->size - 1; k >= 0; k--) c->value = (c->value << 8) | rd8(o, src + k);
            }
            if (c->size == 1) p->core->busWrite8(p->core, addr, c->value);
            else if (c->size == 2) p->core->busWrite16(p->core, addr, c->value);
            else p->core->busWrite32(p->core, addr, c->value);
            out("[p%d] fixture write *%s+%u = %u\n", p->id, c->text, c->frames, c->value);
            advance = true;
            break;
        }
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
        case C_CATCH: {
            // catch N [timeout]: in a wild battle already started, throw the
            // first ball in the POKe BALLS pocket at each of this player's
            // action menus, up to N throws; then fight with A. Declines the
            // nickname (B). Done when back in the field.
            bool battling = rd8(p, sMain + 0x439) & 2;
            unsigned cb2 = rd32(p, sMain + 4) & ~1u;
            static unsigned bagCb, chooseAction;
            if (!bagCb) { bagCb = symAddr("CB2_BagMenuRun") & ~1u; chooseAction = symAddr("HandleInputChooseAction") & ~1u; }
            // An evolution scene at the battle's end (battle-end evolutions,
            // level-ups): A, never B, which would stop it.
            if (cb2 == (symFind("CB2_EvolutionSceneUpdate") & ~1u)) { ++p->step; keys = (p->step % 24) < 2 ? 1 : 0; break; }
            if (++p->step >= c->frames) {
                out("[p%d] FAIL catch timeout (%d throws)\n", p->id, p->grindCount);
                dump(p, "fail"); writePng(p, "fail"); sFailures++;
                p->grindCount = 0; p->grindInBattle = 0; advance = true; break;
            }
            if (!battling && cb2 != bagCb) {
                if (p->grindInBattle && inField(p)) {
                    out("[f%06d p%d catch] battle over after %d throws: outcome=%u party=%u\n", p->frame, p->id, p->grindCount,
                        rd8(p, symAddr("gBattleOutcome")), rd8(p, symAddr("gPartiesCount")));
                    p->grindCount = 0; p->grindInBattle = 0; advance = true; break;
                }
                keys = (p->step % 24) < 2 ? 2 : 0; // B: nickname NO, dex page, messages
                break;
            }
            if (!p->grindInBattle) { p->catchBallDowns = 0; p->catchFightLeft = c->addr; p->catchWasMenu = 0; }
            p->grindInBattle = 1;
            if (cb2 == bagCb) {
                unsigned t = p->step % 30;
                if (rd8(p, symAddr("gBagPosition") + 5) != 1) { keys = t < 2 ? (1u << 4) : 0; break; }
                // catch N T BALL: the first time in the pocket this battle,
                // move the cursor down to BALL (the bag keeps the place for
                // the next throws); without one, the first row.
                if (c->size && p->catchBallDowns == 0) {
                    unsigned pocket = symAddr("gBagPockets") + 8, slots = rd32(p, pocket), cap = rd16(p, pocket + 4) & 0x3FF;
                    int k = -1, n = 0;
                    for (unsigned i = 0; i < cap && i < 64; i++) {
                        unsigned id = rd16(p, slots + i * 4);
                        if (!id) continue;
                        if (id == (unsigned)c->size) { k = n; break; }
                        n++;
                    }
                    p->catchBallDowns = k > 0 ? k + 1 : -1;
                    if (k >= 0) out("[p%d] catch: ball %u at row %d\n", p->id, c->size, k);
                }
                if (p->catchBallDowns > 1) {
                    if (t == 10) p->catchBallDowns--;
                    keys = (t == 10 || t == 11) ? (1u << 7) : 0;
                    break;
                }
                p->catchBallDowns = -1;
                keys = t < 2 ? 1 : 0;
                break;
            }
            int atMenu = 0;
            unsigned funcs = symAddr("gBattlerControllerFuncs");
            // Several controllers have a static HandleInputChooseAction:
            // match by name, not by one address.
            for (int b = 0; b < 4; b++)
                if (!strcmp(symName(rd32(p, funcs + b * 4)), "HandleInputChooseAction")) atMenu = 1;
            (void)chooseAction;
            // catch N T BALL FIGHT: the first FIGHT action menus fight (to
            // weaken it), then the throws.
            if (p->catchWasMenu && !atMenu && p->catchFightLeft > 0 && p->catchFightLeft < 100) p->catchFightLeft--;
            p->catchWasMenu = atMenu;
            if (atMenu && p->catchFightLeft > 0 && p->catchFightLeft < 100) { keys = autoKeys(p, p->step); break; }
            // FIGHT 100+P: fight while the wild Pokemon (battler 1) has more
            // than P% HP, throw at or below it (a player's usual way).
            if (atMenu && c->addr >= 100) {
                unsigned bm = symAddr("gBattleMons") + symValue("harness.battleMonSize");
                unsigned hp = rd16(p, bm + symValue("harness.battleMonHp")), mx = rd16(p, bm + symValue("harness.battleMonMaxHp"));
                if (mx && hp * 100 > (c->addr - 100) * mx) { keys = autoKeys(p, p->step); break; }
            }
            if (atMenu && p->grindCount < (int)c->value) {
                // Steer the action cursor to BAG (1) by reading it, then A
                // (a battler just sent in starts on BATTLE).
                int me = 0;
                for (int b = 0; b < 4; b++)
                    if (!strcmp(symName(rd32(p, funcs + b * 4)), "HandleInputChooseAction")) me = b;
                unsigned t = p->step % 20, cur = rd8(p, symAddr("gActionSelectionCursor") + me);
                if (t != 0) { keys = 0; break; }
                if (cur & 2) keys = 1u << 6;               // UP
                else if (!(cur & 1)) keys = 1u << 4;       // RIGHT
                else { keys = 1; p->grindCount++; out("[p%d] catch: throw %d\n", p->id, p->grindCount); p->step += 30; }
                break;
            }
            if (atMenu) { keys = autoKeys(p, p->step); break; }
            // A fainted battler's forced replacement: the party menu.
            {
                static unsigned partyCbC;
                if (!partyCbC) partyCbC = symFind("CB2_UpdatePartyMenu") & ~1u;
                if (partyCbC && cb2 == partyCbC) { keys = autoKeys(p, p->step); break; }
            }
            // Doubles (a shared legend): the move and target menus are the
            // fight's, not text to close with B.
            for (int b = 0; b < 4; b++) {
                const char* fn = symName(rd32(p, funcs + b * 4));
                if (!strcmp(fn, "HandleInputChooseMove") || !strcmp(fn, "HandleInputChooseTarget")) atMenu = 1;
                // A move with no PP left: move the cursor once to the first
                // move that has some (smart_moves stays off in these runs).
                if (!strcmp(fn, "HandleInputChooseMove")) {
                    unsigned res = rd32(p, symAddr("gBattleResources")), cur = symAddr("gMoveSelectionCursor");
                    unsigned m = res + 0x10 + b * 0x200 + 4, at = rd8(p, cur + b);
                    if (res && at < 4 && rd8(p, m + 8 + at) == 0)
                        for (int i = 0; i < 4; i++)
                            if (rd16(p, m + i * 2) && rd8(p, m + 8 + i)) { p->core->busWrite8(p->core, cur + b, i); break; }
                }
            }
            if (atMenu) { keys = autoKeys(p, p->step); break; }
            keys = (p->step % 24) < 2 ? 2 : 0;
            break;
        }
        case C_HUNT: {
            // hunt THROWS SPECIES[,SPECIES...] [timeout]: pace in grass; in a
            // wild battle against a wanted species, throw balls (up to
            // THROWS per battle, then fight); against anything else, RUN.
            // Done when a wanted Pokemon is caught (gBattleOutcome 7).
            bool battling = rd8(p, sMain + 0x439) & 2;
            unsigned cb2 = rd32(p, sMain + 4) & ~1u;
            static unsigned bagCb;
            if (!bagCb) bagCb = symAddr("CB2_BagMenuRun") & ~1u;
            if (cb2 == (symFind("CB2_EvolutionSceneUpdate") & ~1u)) { ++p->step; keys = (p->step % 24) < 2 ? 1 : 0; break; }
            if (++p->step >= c->frames) {
                out("[p%d] FAIL hunt timeout\n", p->id);
                dump(p, "fail"); writePng(p, "fail"); sFailures++;
                p->grindCount = 0; p->grindInBattle = 0; advance = true; break;
            }
            // Field modes (c->size): 0 pace in grass, cave or water; 1 fish
            // with the registered rod (SELECT); 2 Rock Smash: warp to the
            // tile in front of a rock (map reload brings the rock back), face
            // it, A and YES; again after a smash with no encounter.
            static int huntSub[2], huntIdle[2], huntBattle[2];
            if (!battling && cb2 != bagCb) {
                if (p->grindInBattle) {
                    if (!inField(p)) { keys = (p->step % 24) < 2 ? 2 : 0; break; }
                    unsigned outcome = rd8(p, symAddr("gBattleOutcome"));
                    out("[f%06d p%d hunt] battle over: outcome=%u throws=%d party=%u\n", p->frame, p->id, outcome, p->grindCount, rd8(p, symAddr("gPartiesCount")));
                    p->grindInBattle = 0; p->grindCount = 0; huntSub[p->id] = 0; huntBattle[p->id] = 0;
                    if (outcome == 7) { advance = true; break; }
                }
                if (c->size == 1 && taskActive(p, "Task_Fishing")) {
                    unsigned tasks = symAddr("gTasks"), fn = symAddr("Task_Fishing") & ~1u; int st = -1;
                    for (int t = 0; t < 16; t++)
                        if (rd8(p, tasks + t * 0x28 + 4) && (rd32(p, tasks + t * 0x28) & ~1u) == fn) st = (int16_t)rd16(p, tasks + t * 0x28 + 8);
                    keys = (st >= 8 && (p->step & 1)) ? 1 : 0;
                    break;
                }
                if (!inField(p)) { huntIdle[p->id] = 0; keys = (p->step % 24) < 2 ? 1 : 0; if (c->size == 0) keys = (p->step % 24) < 2 ? 2 : 0; break; }
                if (c->size == 1) { keys = (p->step % 120) == 60 ? (1u << 2) : 0; break; }
                if (c->size == 2) {
                    if (huntSub[p->id] == 0) {
                        unsigned dest = symAddr("overworld.sWarpDestination"), save = rd32(p, sSave1);
                        unsigned g = c->args[0], m = c->args[1], x = c->args[2], y = c->args[3];
                        p->core->busWrite8(p->core, dest, g); p->core->busWrite8(p->core, dest + 1, m);
                        p->core->busWrite8(p->core, dest + 2, 0xFF);
                        p->core->busWrite16(p->core, dest + 4, x); p->core->busWrite16(p->core, dest + 6, y);
                        p->core->busWrite16(p->core, save + 0, x); p->core->busWrite16(p->core, save + 2, y);
                        p->core->busWrite8(p->core, save + 4, g); p->core->busWrite8(p->core, save + 5, m);
                        p->core->busWrite8(p->core, save + 6, 0xFF);
                        p->core->busWrite16(p->core, save + 8, x); p->core->busWrite16(p->core, save + 10, y);
                        p->core->busWrite32(p->core, sMain + 4, symAddr("CB2_LoadMap") | 1);
                        p->core->busWrite8(p->core, sMain + 0x438, 0);
                        huntSub[p->id] = 1; huntIdle[p->id] = 0; keys = 0;
                        p->grindCount = 0;
                        break;
                    }
                    huntIdle[p->id]++;
                    if (huntSub[p->id] == 1) {
                        // face the rock, then A (the YES/NO answer is the A above)
                        if (huntIdle[p->id] < 20) keys = 0;
                        else if (huntIdle[p->id] < 24) keys = c->keys;
                        else if (huntIdle[p->id] < 40) keys = 0;
                        else if (huntIdle[p->id] < 42) { keys = 1; huntSub[p->id] = 2; huntIdle[p->id] = 0; }
                        break;
                    }
                    // back in the field after the smash: no encounter, again
                    keys = 0;
                    if (huntIdle[p->id] > 90) huntSub[p->id] = 0;
                    break;
                }
                keys = ((p->step / 24) & 1) ? (1u << 7) : (1u << 6);
                break;
            }
            if (!p->grindInBattle) {
                p->grindInBattle = 1;
                // The wild Pokemon is enemy party slot 0 (gBattleMons is
                // not filled yet when the battle callback starts).
                unsigned mon = partyMon(1, 0), personality = rd32(p, mon), key = personality ^ rd32(p, mon + 4);
                unsigned sp = (rd32(p, mon + 32 + sSub0[personality % 24] * 12) ^ key) & 0x7FF;
                char list[96]; snprintf(list, sizeof(list), ",%s,", c->text + 16);
                char needle[16]; snprintf(needle, sizeof(needle), ",%u,", sp);
                p->stable = strstr(list, needle) != NULL;
                out("[f%06d p%d hunt] wild species %u: %s\n", p->frame, p->id, sp, p->stable ? "wanted" : "run");
            }
            if (cb2 == bagCb) {
                unsigned t = p->step % 30;
                if (rd8(p, symAddr("gBagPosition") + 5) != 1) keys = t < 2 ? (1u << 4) : 0;
                else keys = t < 2 ? 1 : 0;
                break;
            }
            // A trainer who saw the player while pacing: fight it out (no RUN).
            // So too a wild battle that RUN has not ended in 2500 frames
            // (ARENA TRAP, SHADOW TAG): the driver's moves.
            if (rd32(p, symAddr("gBattleTypeFlags")) & (1u << 3)) { p->stable = 0; keys = autoKeys(p, p->step); break; }
            if (!p->stable && ++huntBattle[p->id] > 2500) { keys = autoKeys(p, p->step); break; }
            int atMenu = 0;
            unsigned funcs = symAddr("gBattlerControllerFuncs");
            for (int b = 0; b < 4; b++)
                if (!strcmp(symName(rd32(p, funcs + b * 4)), "HandleInputChooseAction")) atMenu = 1;
            // Steer the action cursor (0 FIGHT, 1 BAG, 2 POKEMON, 3 RUN)
            // to RUN or BAG by reading it, then press A.
            if (atMenu && (!p->stable || p->grindCount < (int)c->value)) {
                // Safari Zone: BALL is the first entry (0), RUN the last.
                unsigned safari = rd32(p, symAddr("gBattleTypeFlags")) & (1u << 7);
                unsigned want = p->stable ? (safari ? 0 : 1) : 3, t = p->step % 20;
                unsigned cur = rd8(p, symAddr("gActionSelectionCursor"));
                if (t != 0) { keys = 0; break; }
                if ((cur & 1) != (want & 1)) keys = (want & 1) ? (1u << 4) : (1u << 5);
                else if ((cur & 2) != (want & 2)) keys = (want & 2) ? (1u << 7) : (1u << 6);
                else { keys = 1; if (p->stable) { p->grindCount++; out("[p%d] hunt: throw %d\n", p->id, p->grindCount); } p->step += 40; }
                break;
            }
            if (atMenu) { keys = autoKeys(p, p->step); break; }
            // Out of throws (or balls) the fight goes on: the move and
            // target menus are the driver's (B there backed out forever).
            for (int b = 0; b < 4; b++) {
                const char* fn = symName(rd32(p, funcs + b * 4));
                if (!strcmp(fn, "HandleInputChooseMove") || !strcmp(fn, "HandleInputChooseTarget")) atMenu = 1;
            }
            if (atMenu) { keys = autoKeys(p, p->step); break; }
            keys = (p->step % 24) < 2 ? 2 : 0;
            break;
        }
        case C_GRIND: {
            // grind N PCT [timeout]: in grass, pace for wild battles and fight
            // them (A presses, smart_moves if on) until N battles are done or
            // the lead's HP drops below PCT% (or it faints). Logs each battle.
            unsigned lead = partyMon(0, 0);
            unsigned hp = rd16(p, lead + 86), maxhp = rd16(p, lead + 88);
            bool battling = rd8(p, sMain + 0x439) & 2;
            if (++p->step >= c->frames) {
                out("[p%d] FAIL grind timeout after %d battles\n", p->id, p->grindCount);
                dump(p, "fail"); writePng(p, "fail"); sFailures++;
                p->grindCount = 0; p->grindInBattle = 0; advance = true; break;
            }
            if (battling || (c->addr && p->grindInBattle && (rd32(p, sMain + 4) & ~1u) == (symAddr("CB2_UpdatePartyMenu") & ~1u))) {
                if (!p->grindInBattle) p->grindSwitched = 0;
                p->grindInBattle = 1;
                // grind_sw: at the first action menu, POKEMON, the slot,
                // SHIFT; the trainee (lead) still shares the experience.
                // A wild double (two in front already): no switch.
                if (c->addr && !p->grindSwitched && (rd32(p, symAddr("gBattleTypeFlags")) & 1))
                    p->grindSwitched = 1;
                if (c->addr && !p->grindSwitched) {
                    unsigned cb2 = rd32(p, sMain + 4) & ~1u, t = p->step % 20;
                    if (cb2 == (symAddr("CB2_UpdatePartyMenu") & ~1u)) {
                        int slot = (signed char)rd8(p, symAddr("gPartyMenu") + 9);
                        if (t == 0) {
                            if (slot != (int)c->addr) keys = 1u << 7;
                            else { keys = 1; p->step += 60; p->grindSwitched = 2; }
                        }
                        break;
                    }
                    int atMenu = 0;
                    unsigned funcs = symAddr("gBattlerControllerFuncs");
                    for (int b = 0; b < 4; b++)
                        if (!strcmp(symName(rd32(p, funcs + b * 4)), "HandleInputChooseAction")) atMenu = 1;
                    if (atMenu) {
                        unsigned cur = rd8(p, symAddr("gActionSelectionCursor"));
                        if (t == 0) {
                            if (cur & 1) keys = 1u << 5;
                            else if (!(cur & 2)) keys = 1u << 7;
                            else keys = 1;
                        }
                        break;
                    }
                    keys = (p->step % 24) < 2 ? 1 : 0;
                    break;
                }
                if (p->grindSwitched == 2) {
                    // The SHIFT submenu: confirm, then normal fighting.
                    unsigned cb2 = rd32(p, sMain + 4) & ~1u;
                    if (cb2 == (symAddr("CB2_UpdatePartyMenu") & ~1u)) { keys = (p->step % 30) == 0 ? 1 : 0; break; }
                    p->grindSwitched = 1;
                }
                keys = autoKeys(p, p->step); break;
            }
            if (!inField(p)) { keys = autoKeys(p, p->step); break; }
            if (p->grindInBattle) {
                p->grindInBattle = 0;
                p->grindCount++;
                out("[f%06d p%d grind] battle %d done: lead lv=%u hp=%u/%u outcome=%u\n", p->frame, p->id, p->grindCount,
                    rd8(p, lead + 84), hp, maxhp, rd8(p, symAddr("gBattleOutcome")));
            }
            if (c->addr || p->switchLead) {
                unsigned sw = partyMon(0, c->addr ? c->addr : (unsigned)p->switchLead), shp = rd16(p, sw + 86), smax = rd16(p, sw + 88);
                if (shp * 100 < (unsigned)c->size * smax) { hp = shp; maxhp = smax; if (!hp) hp = 0; }
            }
            if (p->grindCount >= (int)c->value || hp == 0 || hp * 100 < (unsigned)c->size * maxhp) {
                out("[p%d] grind stop after %d battles (lead hp %u/%u)\n", p->id, p->grindCount, hp, maxhp);
                p->grindCount = 0; advance = true; break;
            }
            keys = ((p->step / 24) & 1) ? (1u << 7) : (1u << 6);
            break;
        }
        case C_PACE: {
            if ((rd32(p, sMain + 4) & ~1u) == (symAddr("BattleMainCB2") & ~1u)) { p->grindCount = 0; p->paceSettle = 0; advance = true; break; }
            // The stop condition was met mid-step: keys off until the player
            // stands still, so the next command (a START press) isn't lost
            // to the step still in progress.
            if (p->paceSettle) {
                unsigned av = symAddr("gPlayerAvatar");
                if (rd8(p, av + 2) == 0 && rd8(p, av + 3) != 1) p->paceSettle++;
                else p->paceSettle = 1;
                if (p->paceSettle > 8 || !inField(p)) { p->paceSettle = 0; advance = true; }
                break;
            }
            // pace_until_mem: stop when the value matches instead.
            if (c->size == -1) {
                if (((rd8(p, rd32(p, sSave1) + symValue("harness.saveFlagsOffset") + c->addr / 8) >> (c->addr % 8)) & 1) == c->value) {
                    out("[p%d] pace: flag %x = %u after %d frames\n", p->id, c->addr, c->value, p->step);
                    p->grindCount = 0; p->paceSettle = 1; break;
                }
            } else if (c->size == -2) {
                if ((rd32(p, sMain + 4) & ~1u) == c->addr) { out("[p%d] pace: callback reached after %d frames\n", p->id, p->step); p->grindCount = 0; advance = true; break; }
                if (!inField(p)) {   // the hatch scene starts from a script
                    keys = (p->step % 16) == 0 ? 1 : 0;
                    if (++p->step >= c->frames) { out("[p%d] FAIL pace: callback never reached (off the field)\n", p->id); dump(p, "fail"); writePng(p, "fail"); sFailures++; p->done = 1; }
                    break;
                }
            } else
            if (c->addr && ((c->size == 1 ? rd8(p, c->addr) : c->size == 2 ? rd16(p, c->addr) : rd32(p, c->addr)) == c->value)) { p->grindCount = 0; p->paceSettle = 1; break; }
            keys = ((p->step / 24) & 1) ? (1u << 7) : (1u << 6);
            // A PokeNav Match Call holds the field until its text is read.
            if (taskActive(p, "ExecuteMatchCall")) {
                if (!p->grindCount++) out("[f%06d p%d] pace: answering a Match Call\n", p->frame, p->id);
                keys = (p->step % 8) < 2 ? 1 : 0;
            } else p->grindCount = 0;
            if (++p->step >= c->frames) {
                out("[p%d] FAIL no battle while pacing\n", p->id);
                dump(p, "fail");
                sFailures++;
                p->done = 1; p->grindCount = 0;
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
        case C_MONEY_DELTA:
        case C_SET_MONEY: {
            // expect_money_change DELTA: since note_money. fixture_money N.
            unsigned key = rd32(p, rd32(p, symAddr("gSaveBlock2Ptr")) + symValue("harness.encryptionKeyOffset"));
            unsigned addr = rd32(p, sSave1) + symValue("harness.moneyOffset");
            unsigned money = rd32(p, addr) ^ key;
            if (c->kind == C_SET_MONEY) {
                p->core->busWrite32(p->core, addr, c->value ^ key);
                out("[p%d] FIXTURE money %u (was %u)\n", p->id, c->value, money);
            } else {
                int delta = (int)(money - p->moneyBase);
                if (delta != (int)c->value) { out("[p%d] FAIL money change %d, expected %d\n", p->id, delta, (int)c->value); sFailures++; }
                else out("[p%d] ok money change %d\n", p->id, delta);
            }
            advance = true;
            break;
        }
        case C_NOTE_MONEY:
        case C_MONEY_SAME:
        case C_MONEY_GAIN: {
            unsigned key = rd32(p, rd32(p, symAddr("gSaveBlock2Ptr")) + symValue("harness.encryptionKeyOffset"));
            unsigned money = rd32(p, rd32(p, sSave1) + symValue("harness.moneyOffset")) ^ key;
            if (c->kind == C_NOTE_MONEY) {
                p->moneyBase = money;
                out("[p%d] money %u\n", p->id, money);
            } else if (c->kind == C_MONEY_SAME) {
                if (money != p->moneyBase) { out("[p%d] FAIL money %u, was %u\n", p->id, money, p->moneyBase); sFailures++; }
                else out("[p%d] ok money unchanged (%u)\n", p->id, money);
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
            // No Pokemon on this save (party or PC) shares personality AND OT ID
            // with one on the other save: nothing was duplicated across. (A
            // copy keeps both; two saves booted in the same second can roll
            // the same personality for different Pokemon with different OTs.)
            struct Player* o = &P[1 - p->id];
            unsigned shared = 0, mine = 0;
            for (unsigned i = 0; i < 6 + 14 * 30; i++) {
                unsigned sm = rd32(p, symAddr("gPokemonStoragePtr")) + symValue("harness.storageBoxesOffset");
                unsigned a = i < 6 ? partyMon(0, i) : sm + (i - 6) * symValue("harness.boxMonSize");
                if (!(rd8(p, a + 19) & 2)) continue;
                mine++;
                unsigned pid = rd32(p, a), ot = rd32(p, a + 4);
                for (unsigned j = 0; j < 6 + 14 * 30; j++) {
                    unsigned so = rd32(o, symAddr("gPokemonStoragePtr")) + symValue("harness.storageBoxesOffset");
                    unsigned b = j < 6 ? partyMon(0, j) : so + (j - 6) * symValue("harness.boxMonSize");
                    if ((rd8(o, b + 19) & 2) && rd32(o, b) == pid && rd32(o, b + 4) == ot) { shared++; break; }
                }
            }
            if (shared) { out("[p%d] FAIL %u of %u Pokemon also on the other save\n", p->id, shared, mine); sFailures++; }
            else out("[p%d] ok none of %u Pokemon copied from the other save\n", p->id, mine);
            advance = true;
            break;
        }
        case C_ROMWRITE:
            p->core->rawWrite16(p->core, c->addr, -1, c->value);
            out("[p%d] fixture rom %s = %u (now %u)\n", p->id, c->text, c->value, rd16(p, c->addr));
            advance = true;
            break;
        case C_OBJBYTE: {
            // FIXTURE fixture_obj LOCALID OFF VALUE: one byte of the live object
            // with that local id (e.g. OFF 7 = trainerType: 0 hides its sight).
            int hit = 0;
            for (int k = 0; k < 16; k++) {
                unsigned o = sObjEvents + k * 0x24;
                if (!(rd8(p, o) & 1) || rd8(p, o + 8) != (c->addr & 0xFF)) continue;
                p->core->rawWrite8(p->core, o + (c->addr >> 8), -1, c->value);
                out("[f%06d p%d] fixture obj %u slot %d +%u = %u\n", p->frame, p->id, c->addr & 0xFF, k, c->addr >> 8, c->value);
                hit = 1;
            }
            if (!hit) out("[p%d] fixture obj %u: not present\n", p->id, c->addr & 0xFF);
            advance = true;
            break;
        }
        case C_BATTLE_ITEM: {
            // battle_item ITEM SLOT [MOVE]: at this player's next action menu,
            // BAG, the item, USE; in the party menu SLOT downs then A (MOVE
            // downs and A for a one-move item). Done back in the battle.
            static unsigned bagCb2, partyCb2, mainCb2;
            if (!bagCb2) { bagCb2 = symAddr("CB2_BagMenuRun") & ~1u; partyCb2 = symFind("CB2_UpdatePartyMenu") & ~1u; mainCb2 = symAddr("BattleMainCB2") & ~1u; }
            unsigned cb2 = rd32(p, sMain + 4) & ~1u, ph = (unsigned)p->step++;
            unsigned funcs = symAddr("gBattlerControllerFuncs");
            int item = c->value & 0xFFFF, slot = (c->value >> 16) & 0xFF, mv = (c->value >> 24) & 0xFF;
            if (ph >= (unsigned)c->frames) {
                out("[p%d] FAIL battle_item %d: timeout in phase %d\n", p->id, item, p->stable);
                dump(p, "fail"); writePng(p, "fail"); sFailures++; p->stable = 0; advance = true; break;
            }
            if (p->stable == 0) {
                int me = -1;
                for (int b = 0; b < 4; b++)
                    if (!strcmp(symName(rd32(p, funcs + b * 4)), "HandleInputChooseAction")) me = b;
                if (me < 0 || cb2 != mainCb2) { keys = 0; break; }
                unsigned t = ph % 20, cur = rd8(p, symAddr("gActionSelectionCursor") + me);
                if (t != 0) break;
                if (cur & 2) keys = 1u << 6; else if (!(cur & 1)) keys = 1u << 4;
                else { keys = 1; p->stable = 1; p->offField = 0; p->itemPhase = 0; }
                break;
            }
            if (p->stable == 1) {   // the bag: pocket, top, row, A (USE)
                if (cb2 != bagCb2) {
                    // Gone on to the party menu, or straight back to the battle.
                    if (p->itemPhase >= 3) { p->stable = 2; p->offField = 0; p->itemPhase = 0; }
                    break;
                }
                unsigned pockets = symAddr("gBagPockets");
                int pocket = -1, index = -1;
                for (int pk = 0; pk < 5 && pocket < 0; pk++) {
                    unsigned slots = rd32(p, pockets + pk * 8), cap = rd16(p, pockets + pk * 8 + 4) & 0x3FF;
                    if (slots < 0x02000000) continue;
                    for (unsigned i = 0, n = 0; i < cap && i < 400; i++) {
                        unsigned id = rd16(p, slots + i * 4);
                        if (!id) continue;
                        if ((int)id == item) { pocket = pk; index = n; break; }
                        n++;
                    }
                }
                if (pocket < 0) { out("[p%d] FAIL battle_item %d: not in the bag\n", p->id, item); sFailures++; p->stable = 0; advance = true; break; }
                unsigned q = p->offField++;
                unsigned curPocket = rd8(p, symAddr("gBagPosition") + 5);
                if (p->itemPhase == 0) {
                    if (curPocket != (unsigned)pocket) { keys = (q % 30) < 2 ? (1u << 4) : 0; break; }
                    if (q > 60) { p->itemPhase = 1; p->offField = 0; }
                    break;
                }
                if (p->itemPhase == 1) { if (q < 40 * 8) keys = (q % 8) < 2 ? (1u << 6) : 0; else { p->itemPhase = 2; p->offField = 0; } break; }
                if (p->itemPhase == 2) { if (q < (unsigned)index * 8) keys = (q % 8) < 2 ? (1u << 7) : 0; else if (q > (unsigned)index * 8 + 20) { p->itemPhase = 3; p->offField = 0; } break; }
                // A until the bag gives way (the party menu, or the battle for
                // an item with no Pokémon to choose).
                if (p->itemPhase == 3) { keys = (q % 40) < 2 ? 1 : 0; break; }
                break;
            }
            if (p->stable == 2) {   // the party menu (or straight back to the battle)
                unsigned q = p->offField++;
                if (cb2 == mainCb2 && q > 120) {
                    out("[f%06d p%d] battle_item %d used\n", p->frame, p->id, item);
                    p->stable = 0; p->itemPhase = 0; advance = true; break;
                }
                if (cb2 != partyCb2) break;
                // Steer by the menu's own cursor (gPartyMenu.slotId), then A.
                int cur = (int8_t)rd8(p, symAddr("gPartyMenu") + 9);
                if (p->itemPhase == 0) {
                    if (q < 40) break;
                    if (cur != slot) { keys = (q % 12) < 2 ? (1u << 7) : 0; if (q > 600) p->itemPhase = 9; break; }
                    p->itemPhase = 1; p->offField = 0; break;
                }
                if (p->itemPhase == 1) {
                    keys = q < 2 ? 1 : 0;
                    if (q > 30) { p->itemPhase = mv ? 2 : 3; p->offField = 0; out("[p%d] battle_item %d: party slot %d\n", p->id, item, cur); }
                    break;
                }
                if (p->itemPhase == 2) {   // the move list
                    if (q < 40) break;
                    if (q < 40 + (unsigned)(mv - 1) * 12) { keys = ((q - 40) % 12) < 2 ? (1u << 7) : 0; break; }
                    keys = q < 40 + (unsigned)(mv - 1) * 12 + 2 ? 1 : 0;
                    if (q > 40 + (unsigned)(mv - 1) * 12 + 30) { p->itemPhase = 3; p->offField = 0; }
                    break;
                }
                if (q > 240) keys = (q % 30) < 2 ? 1 : 0; // a message to close
                break;
            }
            break;
        }
        case C_FIELD_ITEM: {
            // field_item ITEM SLOT [MOVE]: from the open Bag (field), the item,
            // USE, the party slot (and move); then through the result (an
            // evolution, messages) with A, leaving bag and menus with B.
            // Done back in the field.
            static unsigned fBag, fParty, fOverworld;
            if (!fBag) { fBag = symAddr("CB2_BagMenuRun") & ~1u; fParty = symFind("CB2_UpdatePartyMenu") & ~1u; fOverworld = symAddr("CB2_Overworld") & ~1u; }
            unsigned cb2 = rd32(p, sMain + 4) & ~1u, ph = (unsigned)p->step++;
            int item = c->value & 0xFFFF, slot = (c->value >> 16) & 0xFF, mv = (c->value >> 24) & 0xFF;
            if (ph >= (unsigned)c->frames) {
                out("[p%d] FAIL field_item %d: timeout in phase %d.%d\n", p->id, item, p->stable, p->itemPhase);
                dump(p, "fail"); writePng(p, "fail"); sFailures++; p->stable = 0; p->itemPhase = 0; advance = true; break;
            }
            unsigned q = p->offField++;
            if (p->stable == 0) {   // the bag: pocket, top, row, A, A (USE)
                if (cb2 != fBag) { if (p->itemPhase >= 3) { p->stable = 1; p->offField = 0; p->itemPhase = 0; } break; }
                unsigned pockets = symAddr("gBagPockets");
                int pocket = -1, index = -1;
                for (int pk = 0; pk < 5 && pocket < 0; pk++) {
                    unsigned slots = rd32(p, pockets + pk * 8), cap = rd16(p, pockets + pk * 8 + 4) & 0x3FF;
                    if (slots < 0x02000000) continue;
                    for (unsigned i = 0, n = 0; i < cap && i < 400; i++) {
                        unsigned id = rd16(p, slots + i * 4);
                        if (!id) continue;
                        if ((int)id == item) { pocket = pk; index = n; break; }
                        n++;
                    }
                }
                if (pocket < 0) { out("[p%d] FAIL field_item %d: not in the bag\n", p->id, item); sFailures++; p->stable = 0; advance = true; break; }
                unsigned curPocket = rd8(p, symAddr("gBagPosition") + 5);
                if (p->itemPhase == 0) { if (curPocket != (unsigned)pocket) { keys = (q % 30) < 2 ? (1u << 4) : 0; break; } if (q > 60) { p->itemPhase = 1; p->offField = 0; } break; }
                if (p->itemPhase == 1) { if (q < 40 * 8) keys = (q % 8) < 2 ? (1u << 6) : 0; else { p->itemPhase = 2; p->offField = 0; } break; }
                if (p->itemPhase == 2) { if (q < (unsigned)index * 8) keys = (q % 8) < 2 ? (1u << 7) : 0; else if (q > (unsigned)index * 8 + 20) { p->itemPhase = 3; p->offField = 0; } break; }
                // A (the item's menu), then A again (USE, its first entry).
                keys = (q % 40) < 2 ? 1 : 0;
                break;
            }
            if (p->stable == 1) {   // the party menu: steer to SLOT, A (MOVE for a one-move item)
                if (cb2 != fParty) { if (q > 300) { p->stable = 2; p->offField = 0; } break; }
                int cur = (int8_t)rd8(p, symAddr("gPartyMenu") + 9);
                if (q < 40) break;
                if (p->itemPhase == 0) {
                    if (cur != slot) { keys = (q % 12) < 2 ? (1u << 7) : 0; break; }
                    p->itemPhase = 1; p->offField = 0; break;
                }
                if (p->itemPhase == 1) { keys = q < 2 ? 1 : 0; if (q > 30) { p->itemPhase = mv ? 2 : 3; p->offField = 0; } break; }
                if (p->itemPhase == 2) {
                    if (q < 40 + (unsigned)(mv - 1) * 12) { keys = (q >= 40 && ((q - 40) % 12) < 2) ? (1u << 7) : 0; break; }
                    keys = q < 40 + (unsigned)(mv - 1) * 12 + 2 ? 1 : 0;
                    if (q > 40 + (unsigned)(mv - 1) * 12 + 30) { p->itemPhase = 3; p->offField = 0; }
                    break;
                }
                p->stable = 2; p->offField = 0; break;
            }
            // Through the result: A for text and scenes; B out of the bag,
            // the party menu and the start menu. Done in the field.
            // (After USE the party menu only shows the result: messages,
            // stat boxes, a move to learn; A is safe there, while B would
            // cancel an evolution.)
            if (getenv("FIELD_ITEM_SHOTS") && q % 300 == 100) {
                char nm[48]; snprintf(nm, sizeof(nm), "fi%d-%u-%s", item, q, symName(cb2)); writePng(p, nm);
            }
            if (cb2 == fOverworld) {
                if (inField(p) && q > 60) { out("[f%06d p%d] field_item %d done\n", p->frame, p->id, item); p->stable = 0; p->itemPhase = 0; advance = true; }
                else keys = (q % 30) < 2 ? 2 : 0;   // the START menu
                break;
            }
            if (cb2 == fBag) { keys = (q % 30) < 2 ? 2 : 0; break; }
            // Back at "Use on which POKéMON?" (a Rare Candy stays for more):
            // B, not another use.
            if (cb2 == fParty) {
                static unsigned chooseMon;
                if (!chooseMon) chooseMon = symFind("Task_HandleChooseMonInput") & ~1u;
                int choosing = 0;
                for (int t = 0; t < 16 && chooseMon; t++) {
                    unsigned tb = symAddr("gTasks") + t * 40;
                    if (rd8(p, tb + 4) && (rd32(p, tb) & ~1u) == chooseMon) choosing = 1;
                }
                // Until the item's effect starts the menu still waits for a
                // choice; then it is back there once the effect is over.
                if (!choosing && p->itemPhase < 10) p->itemPhase = 10;
                if (choosing && (p->itemPhase >= 10 || q > 400)) { keys = (q % 30) < 2 ? 2 : 0; break; }
                if (choosing) { keys = (q % 40) == 20 ? 1 : 0; break; }   // the choice did not register yet
            }
            if (cb2 != fParty && p->itemPhase < 10) p->itemPhase = 10;
            keys = (q % 20) < 2 ? 1 : 0;
            break;
        }
        case C_FISH: {
            // fish [T]: the registered rod (SELECT) while facing water, again
            // after "Not even a nibble"; A for the bite and the messages.
            // Done when a battle starts.
            if (rd8(p, sMain + 0x439) & 2) { out("[f%06d p%d] fish: a bite (%d casts)\n", p->frame, p->id, p->grindCount); p->grindCount = 0; p->step = 0; advance = true; break; }
            if (++p->step >= c->frames) { out("[p%d] FAIL fish: no battle after %d casts\n", p->id, p->grindCount); sFailures++; p->grindCount = 0; p->step = 0; advance = true; break; }
            if (taskActive(p, "ExecuteMatchCall")) keys = (p->step % 8) < 2 ? 1 : 0;
            else if (taskActive(p, "Task_Fishing")) {
                // A only from the bite on (FISHING_WAIT_FOR_A = 8 and later):
                // an A during the dots reels in nothing.
                unsigned tasks = symAddr("gTasks"), fn = symAddr("Task_Fishing") & ~1u; int st = -1;
                for (int t = 0; t < 16; t++)
                    if (rd8(p, tasks + t * 0x28 + 4) && (rd32(p, tasks + t * 0x28) & ~1u) == fn) st = (int16_t)rd16(p, tasks + t * 0x28 + 8);
                if (st >= 8) keys = (p->step & 1) ? 1 : 0;
            } else if (inField(p)) { if ((p->step % 120) == 60) { keys = 1u << 2; p->grindCount++; } }
            else keys = (p->step % 8) < 2 ? 1 : 0;
            break;
        }
        case C_NPC_SYNC: {
            // npc_sync T [MAXLAG]: for T frames, compare every shared-NPC object
            // (random walkers and turners, not trainers) on both games by local
            // id. A pair counts as agreeing when either game's current tile
            // equals the other's current or previous tile (a step in flight).
            // FAIL when one stays apart for more than MAXLAG frames (default
            // 96), or jumps more than one tile between 8-frame samples.
            static unsigned apart[256], jumps[2], samples, objs, worst, facings; static int lastX[2][256], lastY[2][256];
            if (p->id == 1) { if (++p->step >= c->frames) { p->step = 0; advance = true; } break; }
            struct Player *q = &P[1];
            if (p->step == 0) { memset(apart, 0, sizeof(apart)); jumps[0] = jumps[1] = samples = objs = worst = facings = 0;
                memset(lastX, 0xFF, sizeof(lastX)); memset(lastY, 0xFF, sizeof(lastY)); }
            unsigned maxLag = c->value ? c->value : 96;
            if ((p->step & 7) == 0 && inField(p) && inField(q)) {
                samples++;
                for (int k = 0; k < 16; k++) {
                    unsigned o = sObjEvents + k * 0x24;
                    if (!(rd8(p, o) & 1) || (rd8(p, o + 2) & 1)) continue;
                    unsigned id = rd8(p, o + 8), mt = rd8(p, o + 6), tt = rd8(p, o + 7);
                    if (id == 0 || id >= 0xF0 || tt != 0) continue;
                    if (!((mt >= 1 && mt <= 6) || (mt >= 0xD && mt <= 0x34) || mt == 0x52)) continue;
                    unsigned o2 = 0; int found = 0;
                    for (int m = 0; m < 16; m++) {
                        o2 = sObjEvents + m * 0x24;
                        if ((rd8(q, o2) & 1) && rd8(q, o2 + 8) == id && !(rd8(q, o2 + 2) & 1)) { found = 1; break; }
                    }
                    if (!found) continue;
                    if (samples == 1) objs++;
                    int ax = (int16_t)rd16(p, o + 0x10), ay = (int16_t)rd16(p, o + 0x12);
                    int apx = (int16_t)rd16(p, o + 0x14), apy = (int16_t)rd16(p, o + 0x16);
                    int bx = (int16_t)rd16(q, o2 + 0x10), by = (int16_t)rd16(q, o2 + 0x12);
                    int bpx = (int16_t)rd16(q, o2 + 0x14), bpy = (int16_t)rd16(q, o2 + 0x16);
                    int same = (ax == bx && ay == by) || (ax == bpx && ay == bpy) || (apx == bx && apy == by);
                    // At rest on the same tile: the facing must agree too (turners).
                    unsigned fa = rd8(p, o), fb = rd8(q, o2);
                    int aRest = !(fa & 0x02) && (!(fa & 0x40) || (fa & 0x80)), bRest = !(fb & 0x02) && (!(fb & 0x40) || (fb & 0x80));
                    if (same && ax == bx && ay == by && aRest && bRest && (rd8(p, o + 0x18) & 15) != (rd8(q, o2 + 0x18) & 15))
                        { same = 0; facings++; }
                    if (same) apart[id] = 0;
                    else {
                        apart[id] += 8;
                        if (apart[id] > worst) worst = apart[id];
                        if (apart[id] == maxLag + 8)
                            out("[f%06d p0] npc %u apart: p0 (%d,%d) p1 (%d,%d)\n", p->frame, id, ax - 7, ay - 7, bx - 7, by - 7);
                    }
                    int pos[2][2] = { { ax, ay }, { bx, by } };
                    for (int side = 0; side < 2; side++) {
                        if (lastX[side][id] != -1 && abs(pos[side][0] - lastX[side][id]) + abs(pos[side][1] - lastY[side][id]) > 1) {
                            jumps[side]++;
                            out("[f%06d p%d] npc %u jumped (%d,%d) -> (%d,%d)\n", p->frame, side, id,
                                lastX[side][id] - 7, lastY[side][id] - 7, pos[side][0] - 7, pos[side][1] - 7);
                        }
                        lastX[side][id] = pos[side][0]; lastY[side][id] = pos[side][1];
                    }
                }
            }
            if (++p->step >= c->frames) {
                p->step = 0;
                int bad = worst > maxLag || jumps[0] || jumps[1] || objs == 0;
                unsigned st0 = symAddr("gCoopNpcStats");
                out("[p0] %s npc_sync: %u shared objects, %u samples, longest apart %u frames (limit %u), facing mismatches %u, jumps p0=%u p1=%u; replayed p0=%u p1=%u corrected %u/%u placed %u/%u\n",
                    bad ? "FAIL" : "ok", objs, samples, worst, maxLag, facings, jumps[0], jumps[1],
                    rd16(p, st0 + 8), rd16(q, st0 + 8), rd16(p, st0 + 10), rd16(q, st0 + 10), rd16(p, st0 + 12), rd16(q, st0 + 12));
                if (bad) sFailures++;
                advance = true;
            }
            break;
        }
        case C_ALLOW_RESET:
            p->resetAllowed = 1;
            out("[p%d] resets allowed from here (deliberate)\n", p->id);
            advance = true;
            break;
        case C_TASKS: {
            // expect_tasks FUNC N: exactly N live tasks run FUNC.
            // expect_tasks MAX N: never more than N co-op session tasks at once
            // so far in the run (counted every frame, battles included).
            if (!strcmp(c->text, "MAX")) {
                if (p->maxSessionTasks > c->value) { out("[p%d] FAIL session tasks peaked at %u, expected at most %u\n", p->id, p->maxSessionTasks, c->value); sFailures++; }
                else out("[p%d] ok session tasks peaked at %u\n", p->id, p->maxSessionTasks);
                advance = true;
                break;
            }
            unsigned n = 0, want = symAddr(c->text) & ~1u;
            for (int t = 0; t < 16; t++) {
                unsigned tb = symAddr("gTasks") + t * 40;
                if (rd8(p, tb + 4) && (rd32(p, tb) & ~1u) == want) n++;
            }
            if (n != c->value) { out("[p%d] FAIL tasks %s: %u live, expected %u\n", p->id, c->text, n, c->value); sFailures++; }
            else out("[p%d] ok tasks %s: %u\n", p->id, c->text, n);
            advance = true;
            break;
        }
        case C_PCFILL: {
            // fixture_fill_pc: every empty PC slot gets a copy of party slot
            // 0's BoxPokemon (encrypted as is), so nothing new fits.
            // fixture_unfill_pc: those copies (same personality) go again.
            unsigned storage = rd32(p, symAddr("gPokemonStoragePtr"));
            unsigned boxes = storage + symValue("harness.storageBoxesOffset"), size = symValue("harness.boxMonSize");
            unsigned src = partyMon(0, 0), pid = rd32(p, src), n = 0;
            for (unsigned i = 0; i < 14 * 30; i++) {
                unsigned m = boxes + i * size;
                if (c->value) {
                    if (rd8(p, m + 19) & 2) continue;
                    for (unsigned k = 0; k < size; k += 4) p->core->busWrite32(p->core, m + k, rd32(p, src + k));
                    n++;
                } else if ((rd8(p, m + 19) & 2) && rd32(p, m) == pid) {
                    for (unsigned k = 0; k < size; k += 4) p->core->busWrite32(p->core, m + k, 0);
                    n++;
                }
            }
            out("[p%d] fixture %s pc: %u slots\n", p->id, c->value ? "fill" : "unfill", n);
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
        case C_NICK: {
            // FIXTURE fixture_nick SLOT NAME: this player's party slot gets a
            // nickname (A-Z a-z 0-9 and '_' as a space, 9 at most). Only the
            // unencrypted 10-byte field is written; EOS ends it early.
            unsigned mon = partyMon(0, c->size) + 8;
            const char* name = c->text + 16;
            unsigned i;
            for (i = 0; i < 9 && name[i]; i++) {
                char ch = name[i];
                unsigned g = 0x00;
                if (ch >= 'A' && ch <= 'Z') g = 0xBB + (ch - 'A');
                else if (ch >= 'a' && ch <= 'z') g = 0xD5 + (ch - 'a');
                else if (ch >= '0' && ch <= '9') g = 0xA1 + (ch - '0');
                p->core->busWrite8(p->core, mon + i, g);
            }
            for (; i < 10; i++) p->core->busWrite8(p->core, mon + i, 0xFF);
            out("[p%d] fixture nickname slot %u = %s\n", p->id, c->size, name);
            advance = true;
            break;
        }
        case C_EXPECT_CB2: {
            unsigned cb2 = rd32(p, sMain + 4) & ~1u;
            if (cb2 == c->addr) out("[p%d] ok cb2 is %s\n", p->id, c->text);
            else { out("[p%d] FAIL cb2 is %08x, not %s\n", p->id, cb2, c->text); sFailures++; }
            advance = true;
            break;
        }
        case C_NOTE_TEXT:
            if (c->size == 1) { snprintf(p->noteText, sizeof(p->noteText), "%s", c->text + 16); p->noteSeen = 0; }
            else if (c->size == 2) {
                if (!p->noteSeen) out("[p%d] ok never shown \"%s\"\n", p->id, p->noteText);
                else { out("[p%d] FAIL \"%s\" was shown\n", p->id, p->noteText); sFailures++; }
                p->noteText[0] = 0; p->noteSeen = 0;
            }
            else {
                if (p->noteSeen) out("[p%d] ok noted \"%s\"\n", p->id, p->noteText);
                else { out("[p%d] FAIL \"%s\" was never in a message\n", p->id, p->noteText); sFailures++; }
                p->noteText[0] = 0; p->noteSeen = 0;
            }
            advance = true;
            break;
        case C_EXPECT_TEXT: {
            // expect_text SYM Words_here: a game string holds the words now.
            char text[440];
            gameText(p, c->addr, text, sizeof(text));
            if (strstr(text, c->text + 16)) out("[p%d] ok text \"%s\"\n", p->id, text);
            else { out("[p%d] FAIL text never had \"%s\" (now \"%s\")\n", p->id, c->text + 16, text); sFailures++; }
            advance = true;
            break;
        }
        case C_UNTIL_FTEXT: {
            // Tap A (every 24 frames) until a field message containing the
            // text is up; the message text is expanded into gStringVar4.
            char text[440];
            gameText(p, symAddr("gStringVar4"), text, sizeof(text));
            if (strstr(text, c->text + 16)) {
                out("[p%d] ok field text \"%s\"\n", p->id, text);
                advance = true;
            } else if (++p->step >= c->frames) {
                out("[p%d] FAIL field text never had \"%s\" (last \"%s\")\n", p->id, c->text + 16, text);
                dump(p, "fail");
                sFailures++;
                advance = true;
            } else if ((p->step % 24) == 0) {
                keys = 1; // A
            }
            break;
        }
        case C_UNTIL_TEXT: {
            // Wait for a battle message containing the text (spaces as '_').
            char text[440];
            gameText(p, symAddr("gDisplayedStringBattle"), text, sizeof(text));
            if (strstr(text, c->text + 16)) {
                out("[p%d] ok battle text \"%s\"\n", p->id, text);
                advance = true;
            } else if (c->value && (rd8(p, sMain + 0x439) & 2)) {
                keys = autoKeys(p, p->step);
            }
            if (!advance && ++p->step >= c->frames) {
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
            if (c->kind == C_ITEM && c->frames >= 0 && (unsigned)c->frames < cap) freeSlot = c->frames;
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
        case C_DEX: {
            // dex_caught NUM VALUE (set this save's own caught flag, a fixture)
            // expect_partner_dex NUM VALUE (the cached partner flag)
            unsigned n = c->addr - 1, got;
            if (c->size == 2) {
                // expect_dex_caught NUM VALUE: this save's own caught flag.
                got = (rd8(p, rd32(p, sSave1) + symValue("harness.dexCaughtOffset") + n / 8) >> (n % 8)) & 1;
                if (got != c->value) { out("[p%d] FAIL dex caught %u = %u, expected %u\n", p->id, c->addr, got, c->value); sFailures++; }
                else out("[p%d] ok dex caught %u = %u\n", p->id, c->addr, got);
            } else if (c->size == 1 || c->size == 3) {
                // dex_seen NUM VALUE (size 3): the seen flag, a fixture.
                unsigned a = rd32(p, sSave1) + symValue(c->size == 3 ? "harness.dexSeenOffset" : "harness.dexCaughtOffset") + n / 8;
                unsigned v = rd8(p, a);
                v = c->value ? (v | (1u << (n % 8))) : (v & ~(1u << (n % 8)));
                p->core->busWrite8(p->core, a, v);
                out("[p%d] fixture dex %s %u = %u\n", p->id, c->size == 3 ? "seen" : "caught", c->addr, c->value);
            } else {
                unsigned base = rd32(p, symAddr("gSaveBlock3Ptr"));
                got = (rd8(p, base + symValue("harness.partnerCaughtOffset") + n / 8) >> (n % 8)) & 1;
                unsigned valid = rd8(p, base + symValue("harness.partnerDexValidOffset"));
                if (got != c->value || !valid) { out("[p%d] FAIL partner dex %u = %u (valid %u), expected %u\n", p->id, c->addr, got, valid, c->value); sFailures++; }
                else out("[p%d] ok partner dex %u = %u\n", p->id, c->addr, got);
            }
            advance = true;
            break;
        }
        case C_TXN: {
            // expect_txn STATE: this save's escrow record (0 none, 1 prepared).
            unsigned rec = rd32(p, symAddr("gSaveBlock3Ptr")) + symValue("harness.dealOffset");
            unsigned st = rd8(p, rec);
            if (st != c->value) { out("[p%d] FAIL deal record state %u, expected %u (deal %04x, last %04x)\n", p->id, st, c->value, rd16(p, rec + 4), rd16(p, rec + 6)); sFailures++; }
            else out("[p%d] ok deal record state %u (deal %04x, last %04x)\n", p->id, st, rd16(p, rec + 4), rd16(p, rec + 6));
            advance = true;
            break;
        }
        case C_SLOT: {
            // fixture_slot SLOT SPECIES [HELD]: give an occupied party slot a
            // new species, held item and personality (re-encrypted, checksum
            // fixed), so a fixture party holds distinct Pokemon.
            static const char* order[24] = {"GAEM","GAME","GEAM","GEMA","GMAE","GMEA","AGEM","AGME","AEGM","AEMG","AMGE","AMEG",
                                             "EGAM","EGMA","EAGM","EAMG","EMGA","EMAG","MGAE","MGEA","MAGE","MAEG","MEGA","MEAG"};
            unsigned mon = partyMon(0, c->size), pid = rd32(p, mon), ot = rd32(p, mon + 4), key = pid ^ ot;
            unsigned w[12], sub[4][3];
            for (int i = 0; i < 12; i++) w[i] = rd32(p, mon + 32 + i * 4) ^ key;
            for (int t = 0; t < 4; t++) {
                int pos = (int)(strchr(order[pid % 24], "GAEM"[t]) - order[pid % 24]);
                for (int k = 0; k < 3; k++) sub[t][k] = w[pos * 3 + k];
            }
            sub[0][0] = (sub[0][0] & ~0x7FFu) | (c->value & 0x7FF);
            if (c->frames >= 0) sub[0][0] = (sub[0][0] & ~(0x3FFu << 16)) | (((unsigned)c->frames & 0x3FF) << 16);
            unsigned npid = pid ^ (0x00010001u * (c->size + 1)) ^ (c->value << 8);
            unsigned nkey = npid ^ ot, sum = 0;
            for (int t = 0; t < 4; t++) {
                int pos = (int)(strchr(order[npid % 24], "GAEM"[t]) - order[npid % 24]);
                for (int k = 0; k < 3; k++) {
                    sum += (sub[t][k] & 0xFFFF) + (sub[t][k] >> 16);
                    p->core->busWrite32(p->core, mon + 32 + (pos * 3 + k) * 4, sub[t][k] ^ nkey);
                }
            }
            p->core->busWrite32(p->core, mon, npid);
            p->core->busWrite16(p->core, mon + 28, sum & 0xFFFF);
            out("[p%d] fixture slot %d species %u held %d\n", p->id, c->size, c->value, c->frames);
            advance = true;
            break;
        }
        case C_SWAP:
            // fixture_swap A B: exchange two party slots (each Pokemon is
            // self-contained, so its personality and gender are kept).
            for (unsigned off = 0; off < 100; off++) {
                unsigned x = rd8(p, partyMon(0, c->size) + off), y = rd8(p, partyMon(0, c->value) + off);
                p->core->busWrite8(p->core, partyMon(0, c->size) + off, y);
                p->core->busWrite8(p->core, partyMon(0, c->value) + off, x);
            }
            out("[p%d] fixture swap slots %d and %u\n", p->id, c->size, c->value);
            advance = true;
            break;
        case C_FILL: {
            // fixture_fill_pocket POCKET [FIRST]: every empty slot gets one
            // of distinct item ids from FIRST up (default 100), so nothing
            // new fits. Test fixture for deferred rewards.
            unsigned pocket = symAddr("gBagPockets") + c->size * 8;
            unsigned slots = rd32(p, pocket), cap = rd16(p, pocket + 4) & 0x3FF;
            unsigned key = rd16(p, rd32(p, symAddr("gSaveBlock2Ptr")) + symValue("harness.encryptionKeyOffset"));
            unsigned i, next = c->value ? c->value : 100, n = 0;
            for (i = 0; i < cap; i++) {
                if (rd16(p, slots + i * 4) != 0)
                    continue;
                p->core->busWrite16(p->core, slots + i * 4, next++);
                p->core->busWrite16(p->core, slots + i * 4 + 2, (1 ^ key) & 0xFFFF);
                n++;
            }
            out("[p%d] fixture filled %u slots of pocket %d\n", p->id, n, c->size);
            advance = true;
            break;
        }
        case C_UNFILL: {
            // fixture_unfill_pocket POCKET [FIRST]: removes what
            // fixture_fill_pocket put there.
            unsigned pocket = symAddr("gBagPockets") + c->size * 8;
            unsigned slots = rd32(p, pocket), cap = rd16(p, pocket + 4) & 0x3FF;
            unsigned i, first = c->value ? c->value : 100, n = 0;
            for (i = 0; i < cap; i++) {
                unsigned id = rd16(p, slots + i * 4);
                if (id >= first && id < first + cap) {
                    p->core->busWrite16(p->core, slots + i * 4, 0);
                    p->core->busWrite16(p->core, slots + i * 4 + 2, 0);
                    n++;
                }
            }
            out("[p%d] fixture emptied %u slots of pocket %d\n", p->id, n, c->size);
            advance = true;
            break;
        }
        case C_GENDER: {
            // fixture_gender G: SaveBlock2.playerGender (offset 8). Test
            // fixture so two saves share a player base; the sprite follows
            // at the next map load.
            unsigned addr = rd32(p, symAddr("gSaveBlock2Ptr")) + 8;
            p->core->busWrite8(p->core, addr, c->value);
            out("[p%d] fixture gender %u\n", p->id, c->value);
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
		case C_TEST_HOOK: {
			unsigned mb = symAddr("gCoopTestMailbox");
			if (p->step == 0) {
				for (int k = 0; k < 8; k++) p->core->busWrite16(p->core, mb + 6 + k * 2, c->args[k]);
				p->core->busWrite16(p->core, mb + 4, c->value);
				p->core->busWrite16(p->core, mb + 22, 0);
				p->core->busWrite32(p->core, mb, 0x7E57C0DEu);
				if (c->value != 7 && c->value != 8 && c->value != 9) out(c->value == 5 ? "[f%06d p%d FIXTURE] %s %u %u %u %u %u %u %u\n" : "[f%06d p%d TEST HELPER] %s %u %u %u %u %u %u %u\n", p->frame, p->id, c->text,
				    c->args[0], c->args[1], c->args[2], c->args[3], c->args[4], c->args[5], c->args[6]);
			}
			if (rd32(p, mb) == 0) {
				unsigned r = rd16(p, mb + 22);
				if (r != 1) { out("[p%d] FAIL %s refused (result %u)\n", p->id, c->text, r); sFailures++; }
				else if (c->value == 7 || c->value == 8) {
					unsigned v = rd32(p, mb + 24);
					if (!c->size) out("[p%d] %s = %u (0x%x)\n", p->id, c->text, v, v);
					else if (c->size == 3) {
						unsigned k = c->addr;
						if (!p->monNoteSet[k]) { p->monNote[k] = v; p->monNoteSet[k] = 1; out("[p%d] %s = %u (0x%x) noted as @%u\n", p->id, c->text, v, v, k); }
						else if (p->monNote[k] == v) out("[p%d] ok %s = %u, as @%u\n", p->id, c->text, v, k);
						else { out("[p%d] FAIL %s = %u, @%u was %u\n", p->id, c->text, v, k, p->monNote[k]); sFailures++; }
					}
					else if (c->size == 4) {
						unsigned k = c->addr;
						if (p->monNoteSet[k] && v > p->monNote[k]) out("[p%d] ok %s = %u, more than @%u (%u)\n", p->id, c->text, v, k, p->monNote[k]);
						else { out("[p%d] FAIL %s = %u, not more than @%u (%u)\n", p->id, c->text, v, k, p->monNoteSet[k] ? p->monNote[k] : 0); sFailures++; }
					}
					else if (c->size == 2) {
						if (v >= c->args[6] && v <= c->args[7]) out("[p%d] ok %s = %u (in %u..%u)\n", p->id, c->text, v, c->args[6], c->args[7]);
						else { out("[p%d] FAIL %s = %u, expected %u..%u\n", p->id, c->text, v, c->args[6], c->args[7]); sFailures++; }
					}
					else if (v == c->addr) out("[p%d] ok %s = %u\n", p->id, c->text, v);
					else { out("[p%d] FAIL %s = %u, expected %u\n", p->id, c->text, v, c->addr); sFailures++; }
				}
				advance = true; break;
			}
			if (++p->step >= c->frames) { out("[p%d] FAIL %s: mailbox never served\n", p->id, c->text); sFailures++; advance = true; }
			break;
		}
		case C_WATCH:
			// watch SYM OFFSET LEN: log every store into that range (the
			// box watch is replaced); expect_writes N checks the count.
			installWatch(p);
			sWatchLo[p->id] = c->addr; sWatchHi[p->id] = c->addr + c->value;
			if (getenv("WATCH2")) {
				char w2[96]; snprintf(w2, sizeof(w2), "%s", getenv("WATCH2"));
				char* colon = strchr(w2, ':'); unsigned len = 4;
				if (colon) { len = strtoul(colon + 1, NULL, 0); *colon = 0; }
				sWatch2Lo = symAddr(w2); sWatch2Hi = sWatch2Lo + len;
			}
			sWatchHits[p->id] = 0; sWatchLogs[p->id] = 0; sWatchQuiet[p->id] = 1;
			out("[p%d] watch %08x..%08x\n", p->id, sWatchLo[p->id], sWatchHi[p->id]);
			advance = true;
			break;
		case C_EXPECT_WRITES:
			if (sWatchHits[p->id] != (int)c->value) { out("[p%d] FAIL %d stores in the watched range, expected %u\n", p->id, sWatchHits[p->id], c->value); sFailures++; }
			else out("[p%d] ok %d stores in the watched range\n", p->id, sWatchHits[p->id]);
			advance = true;
			break;
		case C_BAG_ITEM: {
			// bag_item ITEM: in the Bag, open the pocket holding ITEM and put
			// the cursor on it (the list's top first: the Bag remembers its
			// place and its saved cursor fields are not live).
			unsigned pockets = symAddr("gBagPockets");
			int pocket = -1, index = -1;
			for (int pk = 0; pk < 5 && pocket < 0; pk++) {
				unsigned slots = rd32(p, pockets + pk * 8);
				unsigned cap = rd16(p, pockets + pk * 8 + 4) & 0x3FF;   // capacity:10, id:6
				if (slots < 0x02000000) continue;
				for (unsigned i = 0, n = 0; i < cap && i < 400; i++) {
					unsigned id = rd16(p, slots + i * 4);
					if (!id) continue;
					if (id == c->value) { pocket = pk; index = n; break; }
					n++;
				}
			}
			if (pocket < 0) {
				out("[p%d] FAIL bag_item %u: not in the bag\n", p->id, c->value);
				dump(p, "fail"); sFailures++; p->done = 1; break;
			}
			unsigned ph = (unsigned)p->step++;
			unsigned cur = rd8(p, symAddr("gBagPosition") + 5);
			if (p->stable == 0) {            // the pocket
				if (cur == (unsigned)pocket) { if (++p->offField > 40) { p->stable = 1; p->step = 0; p->offField = 0; } }
				else {
					p->offField = 0; keys = (ph % 30) < 2 ? (1u << 4) : 0;
					if (ph > 600) { out("[p%d] FAIL bag_item: pocket %u, want %d\n", p->id, cur, pocket); dump(p, "fail"); sFailures++; p->done = 1; }
				}
			} else if (p->stable == 1) {     // the top of the list: 40 UPs
				if (ph < 40 * 8) keys = (ph % 8) < 2 ? (1u << 6) : 0;
				else { p->stable = 2; p->step = 0; }
			} else {                         // down to the item
				if (ph < (unsigned)index * 8) keys = (ph % 8) < 2 ? (1u << 7) : 0;
				else if (ph > (unsigned)index * 8 + 30) {
					out("[f%06d p%d bag_item] %u: pocket %d row %d\n", p->frame, p->id, c->value, pocket, index);
					p->stable = 0; advance = true;
				}
			}
			break;
		}
		case C_FLY_MAP: {
			// fly_map MAPSEC: on the Fly map, move the cursor to MAPSEC and
			// choose it; done once back in the field.
			static unsigned flyCb, regionEntries;
			if (!flyCb) { flyCb = symAddr("CB2_FlyMap") & ~1u; regionEntries = symAddr("gRegionMapEntries"); }
			unsigned cb = rd32(p, sMain + 4) & ~1u;
			unsigned ph = (unsigned)p->step++;
			if (cb != flyCb) {
				if (p->stable) {             // chosen: wait for the field
					if (inField(p)) { if (++p->stable > 30) { p->stable = 0; advance = true; } }
					break;
				}
				if (ph > 3000) { out("[p%d] FAIL fly_map: not on the Fly map\n", p->id); dump(p, "fail"); writePng(p, "fail"); sFailures++; p->done = 1; }
				break;
			}
			unsigned fm = rd32(p, symAddr("sFlyMap"));
			unsigned rm = fm + 8;
			unsigned sec = rd16(p, rm), cx = rd16(p, rm + 0x54), cy = rd16(p, rm + 0x56);
			unsigned tx = rd8(p, regionEntries + c->value * 8) + 1, ty = rd8(p, regionEntries + c->value * 8 + 1) + 2;
			if (sec == c->value) { keys = (ph % 20) < 2 ? 1 : 0; p->stable = 1; }
			else if (cx != tx) keys = (ph % 12) < 2 ? (cx < tx ? (1u << 4) : (1u << 5)) : 0;
			else if (cy != ty) keys = (ph % 12) < 2 ? (cy < ty ? (1u << 7) : (1u << 6)) : 0;
			if (ph > 6000) { out("[p%d] FAIL fly_map %u: cursor at (%u,%u) sec %u\n", p->id, c->value, cx, cy, sec); writePng(p, "fail"); sFailures++; p->done = 1; }
			break;
		}
		case C_LEGEND: {
			unsigned base = rd32(p, symAddr("gSaveBlock3Ptr"));
			static const char* const names[3] = {"claimed", "pending", "owed"};
			unsigned off = symValue(c->size == 0 ? "harness.legendClaimedOffset" : c->size == 1 ? "harness.legendPendingOffset" : "harness.legendOwedOffset");
			unsigned v = rd8(p, base + off);
			if (c->frames) {
				p->core->busWrite8(p->core, base + off, c->value);
				out("[p%d] fixture legend %s %02x (was %02x)\n", p->id, names[c->size], c->value, v);
			} else if (v != c->value) {
				out("[p%d] FAIL legend %s %02x, expected %02x\n", p->id, names[c->size], v, c->value);
				sFailures++;
			} else {
				out("[p%d] ok legend %s %02x\n", p->id, names[c->size], v);
			}
			advance = true;
			break;
		}
		case C_OWNED: {
			unsigned sb2 = rd32(p, symAddr("gSaveBlock2Ptr"));
			unsigned myId = rd16(p, sb2 + 10) | (rd16(p, sb2 + 12) << 16);   // playerTrainerId (SaveBlock2 +0x0A)
			unsigned st = rd32(p, symAddr("gPokemonStoragePtr")) + symValue("harness.storageBoxesOffset");
			unsigned bsz = symValue("harness.boxMonSize");
			int count = 0;
			for (int i = 0; i < 6 + 14 * 30; i++) {
				unsigned mon = i < 6 ? partyMon(0, i) : st + (i - 6) * bsz;
				unsigned pid = rd32(p, mon), ot = rd32(p, mon + 4), key = pid ^ ot;
				if (!pid && !ot) continue;
				unsigned g = rd32(p, mon + 32 + sSub0[pid % 24] * 12) ^ key;
				if ((g & 0x7FF) != c->value) continue;
				unsigned m0 = rd32(p, mon + 32 + sSub3[pid % 24] * 12) ^ key;
				unsigned m1 = rd32(p, mon + 32 + sSub3[pid % 24] * 12 + 4) ^ key;
				out("[p%d owned] %s%d species=%u pid=%08x ot=%08x%s metLoc=%u metLv=%u ivs=%u/%u/%u/%u/%u/%u\n", p->id,
				    i < 6 ? "party " : "box ", i < 6 ? i : i - 6, g & 0x7FF, pid, ot, ot == myId ? "(own)" : "(other)",
				    (m0 >> 8) & 0xFF, (m0 >> 16) & 0x7F, m1 & 31, (m1 >> 5) & 31, (m1 >> 10) & 31, (m1 >> 15) & 31, (m1 >> 20) & 31, (m1 >> 25) & 31);
				if (ot == myId) count++;
			}
			if (count < (int)c->addr || count > c->size) {
				out("[p%d] FAIL %s: %d own\n", p->id, c->text, count);
				sFailures++;
			} else {
				out("[p%d] ok %s: %d own\n", p->id, c->text, count);
			}
			advance = true;
			break;
		}
		case C_RIDE: {
			// ride DIRS (U/D/L/R letters): on the Mach Bike, hold each direction
			// until the player's tile has advanced one step that way, then the
			// next, without letting go (speed carries through turns; cracked
			// floors need full speed). Done when the moves are used up (the
			// last may enter a door: a map change also ends it).
			unsigned me = sObjEvents + rd8(p, sAvatar + 5) * 0x24;
			int x = (int16_t)rd16(p, me + 0x10), y = (int16_t)rd16(p, me + 0x12);
			unsigned sv = rd32(p, sSave1);
			unsigned mg = rd8(p, sv + 4) | (rd8(p, sv + 5) << 8) | 0x10000;
			const char* dirs = c->text + 5;   // "ride " prefix
			int n = (int)strlen(dirs);
			if (p->step == 0) { p->stable = 0; p->noPath = (unsigned)(x & 0xFFFF) | ((unsigned)(y & 0xFFFF) << 16); p->offField = mg; }
			int sx = (int16_t)(p->noPath & 0xFFFF), sy = (int16_t)(p->noPath >> 16);
			if (mg != p->offField) { out("[p%d] ok ride: map changed after %d moves\n", p->id, p->stable); p->stable = 0; p->noPath = 0; p->offField = 0; advance = true; break; }
			// The moves done so far: the expected tile after k moves.
			int ex = sx, ey = sy;
			for (int k = 0; k < p->stable; k++) { char d = dirs[k]; ex += d == 'R' ? 1 : d == 'L' ? -1 : 0; ey += d == 'D' ? 1 : d == 'U' ? -1 : 0; }
			// A lower-case letter is a bump: that direction held into a wall
			// for 24 frames, which stops the bike where it is.
			if (p->stable < n && dirs[p->stable] >= 'a') {
				char d = dirs[p->stable];
				keys = d == 'u' ? (1u << 6) : d == 'd' ? (1u << 7) : d == 'l' ? (1u << 5) : (1u << 4);
				if (++p->learnStart >= 24) { p->learnStart = 0; p->stable++; }
				p->step++;
				break;
			}
			if (p->stable < n) {
				char d = dirs[p->stable];
				int nx = ex + (d == 'R' ? 1 : d == 'L' ? -1 : 0), ny = ey + (d == 'D' ? 1 : d == 'U' ? -1 : 0);
				if (x == nx && y == ny) { p->stable++; d = p->stable < n ? dirs[p->stable] : 0; }
				else if (!(x == ex && y == ey)) {
					out("[p%d] FAIL ride: at (%d,%d) after %d moves, expected (%d,%d)\n", p->id, x - 7, y - 7, p->stable, ex - 7, ey - 7);
					dump(p, "fail"); writePng(p, "fail"); sFailures++; p->done = 1; break;
				}
				keys = d == 'U' ? (1u << 6) : d == 'D' ? (1u << 7) : d == 'L' ? (1u << 5) : d == 'R' ? (1u << 4) : 0;
			} else {
				if (++p->step > 120) { out("[p%d] ok ride: %d moves\n", p->id, n); p->stable = 0; p->noPath = 0; p->offField = 0; advance = true; }
				break;
			}
			if (++p->step > 6000) { out("[p%d] FAIL ride: timeout after %d moves\n", p->id, p->stable); dump(p, "fail"); sFailures++; p->done = 1; }
			break;
		}
		case C_DIVE: {
			// dive / emerge [T]: A (dive) or B (emerge) in the field, A to the
			// "use Dive?" question, until the player is underwater (flag 0x10)
			// or back on the surface, in the field.
			bool under = (rd8(p, sAvatar) & 0x10) != 0;
			bool fieldNow = inField(p);
			if (under == (c->value == 1) && fieldNow) {
				if (++p->stable >= 30) { p->stable = 0; advance = true; }
				break;
			}
			p->stable = 0;
			unsigned ph = (unsigned)p->step % 40;
			if (fieldNow) keys = ph < 2 ? (c->value == 1 ? 1u : 2u) : 0;
			else if (rd8(p, sMain + 0x439) & 2) { keys = autoKeys(p, p->offField++); break; }
			else keys = (ph % 20) < 2 ? 1 : 0;
			if (++p->step >= c->frames) {
				out("[p%d] FAIL %s: still %s\n", p->id, c->text, under ? "underwater" : "on the surface");
				dump(p, "fail"); writePng(p, "fail"); sFailures++; p->done = 1;
			}
			break;
		}
		case C_PARTY_ACTION: {
			// party_action SLOT ACTION: in the party menu, the cursor to SLOT,
			// A, then the submenu's cursor to ACTION (party_menu.c's MENU_*;
			// field moves are 28 + FIELD_MOVE_*: Strength 31, Surf 32, Fly 33,
			// Dive 34, Waterfall 35) and A. Done once A is pressed on it.
			static unsigned pmInt, pmenu, smenu;
			if (!pmInt) { pmInt = symAddr("sPartyMenuInternal"); pmenu = symAddr("gPartyMenu"); smenu = symAddr("menu.sMenu"); }
			unsigned ph = (unsigned)p->step++;
			unsigned in = rd32(p, pmInt);
			if (ph > 1500 || in < 0x02000000) {
				if (ph > 1500) { out("[p%d] FAIL party_action %u %u (phase %d)\n", p->id, c->addr, c->value, p->stable); dump(p, "fail"); writePng(p, "fail"); sFailures++; p->done = 1; }
				break;
			}
			if (p->stable == 0) {            // the slot
				unsigned slot = rd8(p, pmenu + 9);
				if (slot != c->addr) { keys = (ph % 16) < 2 ? (1u << 7) : 0; p->offField = 0; }
				else if (++p->offField > 20) { keys = 1; p->stable = 1; p->offField = 0; }
			} else if (p->stable == 1) {     // the submenu
				unsigned na = rd8(p, in + 23);
				int want = -1;
				for (unsigned i = 0; i < na && i < 8; i++) if (rd8(p, in + 15 + i) == c->value) want = (int)i;
				if (na == 0 || want < 0) { if (++p->offField > 300) { out("[p%d] FAIL party_action: action %u not in the submenu (%u actions)\n", p->id, c->value, na); dump(p, "fail"); writePng(p, "fail"); sFailures++; p->done = 1; } break; }
				int cur = (signed char)rd8(p, smenu + 2);
				if (cur != want) { keys = (ph % 16) < 2 ? (cur < want ? (1u << 7) : (1u << 6)) : 0; p->offField = 0; }
				else if (++p->offField > 20) { keys = 1; p->stable = 0; p->offField = 0; advance = true; }
			}
			break;
		}
		case C_STEP_TO: {
			// step_to DIR X Y [T]: press DIR in the field until the player
			// stands on X,Y (a warp pad on the same map, where cross cannot
			// tell the warp happened).
			unsigned me = sObjEvents + rd8(p, sAvatar + 5) * 0x24;
			int x = (int16_t)rd16(p, me + 0x10) - 7, y = (int16_t)rd16(p, me + 0x12) - 7;
			bool fieldNow = inField(p);
			if (x == (int)c->addr && y == c->value && fieldNow) {
				if (++p->stable >= 20) { p->stable = 0; advance = true; }
				break;
			}
			p->stable = 0;
			if (fieldNow) keys = ((unsigned)p->step % 16) < 8 ? c->keys : 0;
			else if (rd8(p, sMain + 0x439) & 2) { keys = autoKeys(p, p->offField++); break; }
			if (++p->step >= c->frames) {
				out("[p%d] FAIL step_to (%u,%u): at (%d,%d)\n", p->id, c->addr, c->value, x, y);
				dump(p, "fail"); writePng(p, "fail"); sFailures++; p->done = 1;
			}
			break;
		}
		case C_SURF: {
			if (rd8(p, sAvatar) & 8) {
				if (++p->stable >= 30) { p->stable = 0; p->step = 0; advance = true; }
				break;
			}
			p->stable = 0;
			// Face the water while in the field; A starts and answers the
			// question (YES is the default).
			unsigned ph = (unsigned)p->frame % 40;
			bool fieldNow = inField(p);
			if (fieldNow) keys = ph < 3 ? c->keys : (ph >= 20 && ph < 22) ? 1 : 0;
			else if (rd8(p, sMain + 0x439) & 2) keys = autoKeys(p, p->offField);
			else keys = (ph % 20) < 2 ? 1 : 0; // A only: B would answer NO
			// Scenes and battles on the way don't count against the budget.
			if (!fieldNow && p->offField < 60000u) { p->offField++; break; }
			p->offField = 0;
			if (getenv("CMD_TRACE") && p->step % 250 == 0) {
				unsigned me = sObjEvents + rd8(p, sAvatar + 5) * 0x24;
				out("[f%06d p%d surf] step %d keys %x facing %u script %u\n", p->frame, p->id, p->step, keys, rd8(p, me + 0x18) & 15, sScriptStatus ? rd8(p, sScriptStatus) : 9);
			}
			if (++p->step >= c->frames) {
				unsigned me = sObjEvents + rd8(p, sAvatar + 5) * 0x24;
				out("[p%d] FAIL %s: not surfing (avatar flags %02x, facing %u, obj bytes %02x %02x %02x %02x, lock %u)\n", p->id, c->text, rd8(p, sAvatar), rd8(p, me + 0x18) & 15, rd8(p, me), rd8(p, me + 1), rd8(p, me + 2), rd8(p, me + 3), rd8(p, sLockCtl));
				dump(p, "fail"); writePng(p, "fail"); sFailures++; p->done = 1;
			}
			break;
		}
		case C_BOXES: {
			// boxes: every occupied PC box slot (species and moves, decrypted).
			unsigned st = rd32(p, symAddr("gPokemonStoragePtr"));
			int n = 0;
			for (int bx = 0; bx < 14; bx++)
				for (int sl = 0; sl < 30; sl++) {
					unsigned mon = st + 4 + (bx * 30 + sl) * 80;
					if (!rd32(p, mon) && !rd32(p, mon + 4)) continue;
					showMonAddr(p, mon, 100 + bx, sl);
					n++;
				}
			out("[f%06d p%d boxes] %d occupied\n", p->frame, p->id, n);
			advance = true;
			break;
		}
		case C_PARTY:
			showParty(p);
			showPartySpecies(p);
			for (int k = 0; k < 6; k++)
				if (rd8(p, partyMon(0, k) + 19) & 2) showMonAt(p, 0, k);
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
			unsigned v = rdN(p, c->addr, c->size) & c->args[0];
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
		p->offField = 0;
		p->cb2Mark = p->cb2HistN;
		{ static int tr = -1; if (tr < 0) tr = getenv("CMD_TRACE") != NULL;
		  if (tr && p->pc < p->ncmds) out("[f%06d p%d cmd] %d %s\n", p->frame, p->id, p->pc, p->cmds[p->pc].text); }
		if (c->kind == C_PRESS || c->kind == C_WAIT) break;
	}
	// DRIVER mega_next N: the next N times one of this player's battlers is
	// at its move menu, press START once (the game's own Mega trigger) and
	// hold other keys off for a moment so the press registers before A.
	if (p->megaPress > 0 && (rd8(p, sMain + 0x439) & 2)) {
		unsigned bflags2 = rd32(p, sBattleFlags);
		int own2 = ((bflags2 & 2) && !(bflags2 & 0x40) && !(bflags2 & 4)) ? 1 : 0;
		int atMove = 0;
		for (int b = own2; b < 4; b += 2)
			if (!strcmp(symName(rd32(p, symAddr("gBattlerControllerFuncs") + b * 4)), "HandleInputChooseMove")) atMove = 1;
		if (!atMove) p->megaLeave = 0;
		if (atMove && !p->megaLeave) {
			if (p->megaPhase < 30) { p->megaPhase++; keys = 0; }
			else if (p->megaPhase == 30) { p->megaPhase++; keys = 8; out("[f%06d p%d] DRIVER START at the move menu (Mega trigger)\n", p->frame, p->id); }
			else if (p->megaPhase < 50) { p->megaPhase++; keys = 0; }
			else { p->megaPhase = 0; p->megaLeave = 1; p->megaPress--; }
		}
	}
	if (p->crashStart) { p->crashStart--; keys = p->crashStart >= 4 ? 8 /* START */ : 0; }
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
		} else if (!strcmp(cmd, "hexp")) {
			// hexp POINTER_SYM N OFFSET: N bytes at *POINTER_SYM + OFFSET
			c.kind = C_HEX; c.addr = symAddr(a); c.size = atoi(b); c.value = 1; c.frames = n >= 5 ? (int)strtoul(d, NULL, 0) : 0;
			snprintf(c.text, sizeof(c.text), "*%s+%s", a, n >= 5 ? d : "0");
		} else if (!strcmp(cmd, "hex")) {
			c.kind = C_HEX; c.addr = (a[0] == '0' && a[1] == 'x' ? strtoul(a, NULL, 16) : symAddr(a)) + (n >= 5 ? strtoul(d, NULL, 0) : 0); c.size = n >= 4 ? atoi(b) : 16;
			strncpy(c.text, a, sizeof(c.text) - 1);
		} else if (!strcmp(cmd, "dump")) {
			c.kind = C_DUMP; strncpy(c.text, n >= 3 ? a : "-", sizeof(c.text) - 1);
		} else if (!strcmp(cmd, "sync")) {
			if (w != 2) { fprintf(stderr, "sync needs b\n"); exit(1); }
			continue;
		} else if (!strcmp(cmd, "until_cb2") || !strcmp(cmd, "until_task")) {
			c.kind = cmd[6] == 'c' ? C_UNTIL_CB2 : C_UNTIL_TASK; c.addr = symAddr(a) & ~1u; c.frames = n >= 4 ? atoi(b) : 3000;
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
        } else if (!strcmp(cmd, "cross") || !strcmp(cmd, "until_map")) {
        	// cross DIR GROUP NUM [timeout] / until_map GROUP NUM [timeout]
        	int until = !strcmp(cmd, "until_map");
        	c.kind = C_CROSS;
        	c.keys = until ? 0 : parseKeys(a);
        	c.addr = strtoul(until ? a : b, NULL, 0);
        	c.value = strtoul(until ? b : d, NULL, 0);
        	c.frames = until ? (n >= 5 ? atoi(d) : 3000) : (n >= 6 ? atoi(e) : 3000);
        	snprintf(c.text, sizeof(c.text), "%s %s %s %s", cmd, a, b, until ? "" : d);
        } else if (!strcmp(cmd, "strong_battlers") || !strcmp(cmd, "weak_foes")) {
        	c.kind = C_STRONG;
        	c.value = strtoul(a, NULL, 0);
        	c.size = !strcmp(cmd, "weak_foes");
        } else if (!strcmp(cmd, "smart_moves")) {
        	c.kind = C_SMART;
        	c.value = strtoul(a, NULL, 0);
        } else if (!strcmp(cmd, "test_mon") || !strcmp(cmd, "test_level") || !strcmp(cmd, "test_heal") || !strcmp(cmd, "test_item") || !strcmp(cmd, "test_send_reply") || !strcmp(cmd, "test_friendship") || !strcmp(cmd, "test_held") || !strcmp(cmd, "test_data")) {
        	// TEST HELPER (combat assistance, recorded in the log): the ROM's
        	// harness-only mailbox, gCoopTestMailbox (src/coop_testhook.c).
        	c.kind = C_TEST_HOOK;
        	c.value = !strcmp(cmd, "test_mon") ? 1 : !strcmp(cmd, "test_level") ? 2 : !strcmp(cmd, "test_heal") ? 3 : !strcmp(cmd, "test_item") ? 4 : (!strcmp(cmd, "test_friendship") || !strcmp(cmd, "test_held") || !strcmp(cmd, "test_data")) ? 6 : 5;
        	memset(c.args, 0, sizeof(c.args));
        	sscanf(line, "%*s %*s %u %u %u %u %u %u %u %u", &c.args[0], &c.args[1], &c.args[2], &c.args[3], &c.args[4], &c.args[5], &c.args[6], &c.args[7]);
        	if (!strcmp(cmd, "test_held")) c.args[2] = 1;
        	// test_data SLOT hp|tracker VALUE: a hurt Pokemon, a move-use count.
        	if (!strcmp(cmd, "test_data")) {
        		char field[32] = ""; unsigned slot = 0, val = 0;
        		sscanf(line, "%*s %*s %u %31s %u", &slot, field, &val);
        		memset(c.args, 0, sizeof(c.args));
        		c.args[0] = slot; c.args[1] = val; c.args[2] = 2;
        		c.args[3] = !strcmp(field, "hp") ? 1 : !strcmp(field, "tracker") ? 2 : !strcmp(field, "evs") ? 3 : !strcmp(field, "ivs") ? 4 : !strcmp(field, "clear") ? 5 : !strcmp(field, "dcsteps") ? 6 : !strcmp(field, "hyper") ? 7 : 0;
        	}
        	snprintf(c.text, sizeof(c.text), "%s", cmd);
        	c.frames = 600;
        } else if (!strcmp(cmd, "test_battle_name")) {
        	// test_battle_name BATTLER: the ROM writes the trainer name a
        	// battle message gives that battler ({B_ATK_TRAINER_NAME}) into
        	// gStringVar4 (read-only; check it with expect_text).
        	c.kind = C_TEST_HOOK;
        	c.value = 9;
        	memset(c.args, 0, sizeof(c.args));
        	c.args[0] = (unsigned)atoi(a);
        	snprintf(c.text, sizeof(c.text), "%s", cmd);
        	c.frames = 600;
        } else if (!strcmp(cmd, "expect_mon")) {
        	// expect_mon SLOT FIELD VALUE [ARG] [daycare]: one field of a party
        	// (or Day Care) Pokemon read by the ROM's test hook; VALUE "-" only
        	// prints it. Read-only.
        	static const char *const fields[] = { "species", "level", "nature", "mint", "ability", "evtotal", "hyper", "iv",
        	    "held", "egg", "personality", "move", "friendship", "gender", "ev", "otid", "perfect", "exp", "hasmove", "stat", "dcsteps", "dexcaught", "hp" };
        	char slot[16] = "", field[24] = "", val[24] = "", arg[24] = "", src[24] = "";
        	sscanf(line, "%*s %*s %15s %23s %23s %23s %23s", slot, field, val, arg, src);
        	c.kind = C_TEST_HOOK;
        	c.value = 7;
        	memset(c.args, 0, sizeof(c.args));
        	c.args[0] = atoi(slot);
        	c.args[1] = 0xFFFF;
        	for (unsigned k = 0; k < sizeof(fields) / sizeof(fields[0]); k++) if (!strcmp(field, fields[k])) c.args[1] = k;
        	if (c.args[1] == 0xFFFF) { fprintf(stderr, "expect_mon: unknown field %s\n", field); exit(2); }
        	c.args[2] = strtoul(arg, NULL, 0);
        	c.args[3] = !strcmp(src, "daycare") || !strcmp(arg, "daycare");
        	if (!strcmp(src, "enemyA") || !strcmp(arg, "enemyA")) c.args[3] = 2;
        	if (!strcmp(src, "enemyB") || !strcmp(arg, "enemyB")) c.args[3] = 3;
        	if (!strcmp(arg, "daycare") || !strcmp(arg, "enemyA") || !strcmp(arg, "enemyB")) c.args[2] = 0;
        	c.size = strcmp(val, "-") != 0;
        	c.addr = strtoul(val, NULL, 0);
        	// VALUE LO..HI: a range (packed in args[6..7], unused by the hook)
        	{ char* dots = strstr(val, ".."); if (dots) { c.args[6] = strtoul(val, NULL, 0); c.args[7] = strtoul(dots + 2, NULL, 0); c.size = 2; } }
        	// VALUE @K: the first time, remember it as note K; later, the same
        	// value is expected (identity kept through a deposit, a reload).
        	if (val[0] == '@') { c.size = 3; c.addr = atoi(val + 1) & 7; }
        	// VALUE >@K: more than note K (EXP gained by a Pokemon that fought).
        	if (val[0] == '>' && val[1] == '@') { c.size = 4; c.addr = atoi(val + 2) & 7; }
        	snprintf(c.text, sizeof(c.text), "expect_mon %s %s%s", slot, field, c.args[3] == 1 ? " daycare" : c.args[3] == 2 ? " enemyA" : c.args[3] == 3 ? " enemyB" : "");
        	c.frames = 600;
        } else if (!strcmp(cmd, "expect_team")) {
        	// expect_team TRAINER KEY PARTY: the party id the ROM picks for a
        	// trainer id under a team key (src/coop_teams.c). Read-only.
        	c.kind = C_TEST_HOOK;
        	c.value = 8;
        	memset(c.args, 0, sizeof(c.args));
        	c.args[0] = strtoul(a, NULL, 0);
        	c.args[1] = strtoul(b, NULL, 0);
        	c.size = 1;
        	c.addr = strtoul(d, NULL, 0);
        	snprintf(c.text, sizeof(c.text), "expect_team %s key %s", a, b);
        	c.frames = 600;
        } else if (!strcmp(cmd, "watch")) {
        	c.kind = C_WATCH;
        	c.addr = symAddr(a) + strtoul(b, NULL, 0);
        	c.value = strtoul(d, NULL, 0);
        } else if (!strcmp(cmd, "expect_writes")) {
        	c.kind = C_EXPECT_WRITES;
        	c.value = strtoul(a, NULL, 0);
        } else if (!strcmp(cmd, "surf")) {
        	// surf DIR [T]: from the bank, face DIR and answer the game's
        	// "use SURF?" question until the player is surfing.
        	c.kind = C_SURF;
        	c.keys = parseKeys(a);
        	c.frames = n >= 4 ? atoi(b) : 4000;
        	snprintf(c.text, sizeof(c.text), "surf %s", a);
        } else if (!strcmp(cmd, "ride")) {
        	c.kind = C_RIDE;
        	snprintf(c.text, sizeof(c.text), "ride %s", a);
        } else if (!strcmp(cmd, "dive") || !strcmp(cmd, "emerge")) {
        	c.kind = C_DIVE;
        	c.value = !strcmp(cmd, "dive");
        	c.frames = n >= 3 ? atoi(a) : 4000;
        	snprintf(c.text, sizeof(c.text), "%s", cmd);
        } else if (!strcmp(cmd, "party_action")) {
        	c.kind = C_PARTY_ACTION;
        	c.addr = strtoul(a, NULL, 0);
        	c.value = strtoul(b, NULL, 0);
        	snprintf(c.text, sizeof(c.text), "party_action %s %s", a, b);
        } else if (!strcmp(cmd, "step_to")) {
        	c.kind = C_STEP_TO;
        	c.keys = parseKeys(a);
        	c.addr = strtoul(b, NULL, 0);
        	c.value = strtoul(d, NULL, 0);
        	c.frames = n >= 6 ? atoi(e) : 600;
        	snprintf(c.text, sizeof(c.text), "step_to %s %s %s", a, b, d);
        } else if (!strcmp(cmd, "bag_item")) {
        	c.kind = C_BAG_ITEM;
        	c.value = strtoul(a, NULL, 0);
        	snprintf(c.text, sizeof(c.text), "bag_item %s", a);
        } else if (!strcmp(cmd, "fly_map")) {
        	c.kind = C_FLY_MAP;
        	c.value = strtoul(a, NULL, 0);
        	snprintf(c.text, sizeof(c.text), "fly_map %s", a);
        } else if (!strcmp(cmd, "boxes")) {
        	c.kind = C_BOXES;
        } else if (!strcmp(cmd, "expect_screen_same")) {
        	// expect_screen_same X Y W H: that rectangle shows the same pixels on
        	// both screens (a battler picture both games should draw alike).
        	c.kind = C_SCREEN_SAME;
        	sscanf(line, "%*s %*s %u %u %u %u", &c.args[0], &c.args[1], &c.args[2], &c.args[3]);
        	snprintf(c.text, sizeof(c.text), "expect_screen_same %s %s %s %s", a, b, d, e);
        } else if (!strcmp(cmd, "mega_next")) {
        	c.kind = C_MEGA_NEXT;
        	c.value = (unsigned)atoi(a);
        	snprintf(c.text, sizeof(c.text), "mega_next %s", a);
        } else if (!strcmp(cmd, "force_move")) {
        	c.kind = C_FORCE_MOVE;
        	c.value = (unsigned)atoi(a);
        	c.addr = n >= 4 ? (!strcmp(b, "ally") ? 4u : strtoul(b, NULL, 0)) : 0xFFFFFFFFu;
        	snprintf(c.text, sizeof(c.text), "force_move %s", a);
        } else if (!strcmp(cmd, "talk_obj")) {
        	c.kind = C_TALK_OBJ;
        	c.value = strtoul(a, NULL, 0);
        	c.frames = n >= 4 ? atoi(b) : 3000;
        	snprintf(c.text, sizeof(c.text), "talk_obj %s", a);
        } else if (!strcmp(cmd, "no_wild")) {
        	c.kind = C_NO_WILD;
        	c.value = strtoul(a, NULL, 0);
        } else if (!strcmp(cmd, "walk") || !strcmp(cmd, "walk_battle") || !strcmp(cmd, "walk_try")) {
        	c.kind = !strcmp(cmd, "walk_battle") ? C_WALK_BATTLE : C_WALK;
        	c.size = !strcmp(cmd, "walk_try");
        	c.addr = strtoul(a, NULL, 0);
        	c.value = strtoul(b, NULL, 0);
        	c.frames = n >= 5 ? atoi(d) : 4000;
        	snprintf(c.text, sizeof(c.text), "walk %s %s", a, b);
        } else if (!strcmp(cmd, "force_encounter")) {
            c.kind = C_FORCE_ENC;
            c.value = strtoul(a, NULL, 0);
        } else if (!strcmp(cmd, "fish")) {
            c.kind = C_FISH;
            c.frames = n >= 3 ? atoi(a) : 6000;
            snprintf(c.text, sizeof(c.text), "fish");
        } else if (!strcmp(cmd, "field_item")) {
            c.kind = C_FIELD_ITEM;
            c.value = (strtoul(a, NULL, 0) & 0xFFFF) | ((n >= 4 ? strtoul(b, NULL, 0) : 0) << 16) | ((n >= 5 ? strtoul(d, NULL, 0) + 1 : 0) << 24);
            c.frames = 6000;
            snprintf(c.text, sizeof(c.text), "field_item %s", a);
        } else if (!strcmp(cmd, "battle_item")) {
            c.kind = C_BATTLE_ITEM;
            c.value = (strtoul(a, NULL, 0) & 0xFFFF) | ((n >= 4 ? strtoul(b, NULL, 0) : 0) << 16) | ((n >= 5 ? strtoul(d, NULL, 0) + 1 : 0) << 24);
            c.frames = 3000;
            snprintf(c.text, sizeof(c.text), "battle_item %s", a);
        } else if (!strcmp(cmd, "npc_sync")) {
            c.kind = C_NPC_SYNC;
            c.frames = atoi(a);
            c.value = n >= 4 ? strtoul(b, NULL, 0) : 0;
            snprintf(c.text, sizeof(c.text), "npc_sync %s", a);
        } else if (!strcmp(cmd, "fixture_obj")) {
            c.kind = C_OBJBYTE;
            c.addr = (strtoul(a, NULL, 0) & 0xFF) | (strtoul(b, NULL, 0) << 8);
            c.value = strtoul(d, NULL, 0);
            snprintf(c.text, sizeof(c.text), "fixture_obj %s %s %s", a, b, d);
        } else if (!strcmp(cmd, "fixture_rom16")) {
            // FIXTURE fixture_rom16 SYM+off VALUE: patch a halfword of ROM data
            // in this core (for example a route's wild species table).
            char sym[96]; unsigned off = 0;
            snprintf(sym, sizeof(sym), "%s", a);
            char* plus = strchr(sym, '+');
            if (plus) { off = strtoul(plus + 1, NULL, 0); *plus = 0; }
            c.kind = C_ROMWRITE; c.addr = symAddr(sym) + off; c.value = strtoul(b, NULL, 0);
            snprintf(c.text, sizeof(c.text), "%s", a);
        } else if (!strcmp(cmd, "allow_reset")) {
            // allow_reset: this scenario resets the game on purpose (the
            // credits, a soft reset): not a failure from here on.
            c.kind = C_ALLOW_RESET;
        } else if (!strcmp(cmd, "expect_tasks")) {
            c.kind = C_TASKS;
            snprintf(c.text, sizeof(c.text), "%s", a);
            c.value = strtoul(b, NULL, 0);
        } else if (!strcmp(cmd, "fixture_fill_pc") || !strcmp(cmd, "fixture_unfill_pc")) {
            c.kind = C_PCFILL;
            c.value = !strcmp(cmd, "fixture_fill_pc");
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
        } else if (!strcmp(cmd, "pace_until_flag") || !strcmp(cmd, "pace_until_cb2")) {
            // pace_until_flag FLAG VALUE [timeout]: walk in place until a save
            // flag has VALUE (e.g. the Day Care's pending egg).
            // pace_until_cb2 NAME [timeout]: until a callback runs (egg hatch).
            c.kind = C_PACE;
            if (!strcmp(cmd, "pace_until_flag")) {
                c.size = -1;
                c.addr = strtoul(a, NULL, 0);
                c.value = strtoul(b, NULL, 0);
                c.frames = n >= 5 ? atoi(d) : 6000;
            } else {
                c.size = -2;
                c.addr = symAddr(a) & ~1u;
                c.frames = n >= 4 ? atoi(b) : 6000;
            }
        } else if (!strcmp(cmd, "hunt")) {
            // hunt THROWS SPECIES[,SPECIES...] [timeout] (species numbers)
            // hunt THROWS LIST T fish | hunt THROWS LIST T smash G N X Y DIR
            c.kind = C_HUNT;
            c.value = strtoul(a, NULL, 0);
            snprintf(c.text, sizeof(c.text), "hunt");
            snprintf(c.text + 16, sizeof(c.text) - 16, "%s", b);
            c.frames = n >= 5 ? atoi(d) : 60000;
            c.size = 0;
            {
                char mode[16] = "", dir[16] = ""; unsigned g = 0, m = 0, x = 0, y = 0;
                int k = sscanf(line, "%*s %*s %*s %*s %*s %15s %u %u %u %u %15s", mode, &g, &m, &x, &y, dir);
                if (k >= 1 && !strcmp(mode, "fish")) c.size = 1;
                if (k >= 6 && !strcmp(mode, "smash")) { c.size = 2; c.args[0] = g; c.args[1] = m; c.args[2] = x; c.args[3] = y; c.keys = parseKeys(dir); }
            }
        } else if (!strcmp(cmd, "catch")) {
            // catch N [timeout] [BALL]
            c.kind = C_CATCH;
            c.value = strtoul(a, NULL, 0);
            c.frames = n >= 4 ? atoi(b) : 20000;
            c.size = n >= 5 ? atoi(d) : 0;   // the ball's item id (0: the first row)
            c.addr = n >= 6 ? (unsigned)atoi(e) : 0;   // action menus to fight first
        } else if (!strcmp(cmd, "grind")) {
            // grind N PCT [timeout]
            c.kind = C_GRIND;
            c.value = strtoul(a, NULL, 0);
            c.size = atoi(b);
            c.frames = n >= 5 ? atoi(d) : 60000;
        } else if (!strcmp(cmd, "heal_below")) {
            // heal_below PCT [USES]: driver aid, see autoKeys. 0 turns it off.
            c.kind = C_HEAL_BELOW;
            c.value = strtoul(a, NULL, 0);
            c.size = n >= 4 ? atoi(b) : 2;
        } else if (!strcmp(cmd, "party_slot")) {
            // party_slot N [timeout]: in the party menu, move the cursor to
            // slot N (the menu remembers the last slot when reopened).
            c.kind = C_PARTY_SLOT;
            c.value = strtoul(a, NULL, 0);
            c.frames = n >= 4 ? atoi(b) : 600;
        } else if (!strcmp(cmd, "switch_lead")) {
            // switch_lead SLOT: driver aid, see autoKeys. 0 turns it off.
            c.kind = C_SWITCH_LEAD;
            c.value = strtoul(a, NULL, 0);
        } else if (!strcmp(cmd, "grind_sw")) {
            // grind_sw N PCT SLOT [timeout]: as grind, but each battle
            // starts by switching the lead out for party slot SLOT (the
            // lead shares the experience; Gen 6+ full experience).
            c.kind = C_GRIND;
            c.value = strtoul(a, NULL, 0);
            c.size = atoi(b);
            c.addr = strtoul(d, NULL, 0);
            c.frames = n >= 6 ? atoi(e) : 60000;
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
        } else if (!strcmp(cmd, "expect_money_change") || !strcmp(cmd, "fixture_money")) {
            c.kind = !strcmp(cmd, "fixture_money") ? C_SET_MONEY : C_MONEY_DELTA;
            c.value = (unsigned)strtol(a, NULL, 0);
        } else if (!strcmp(cmd, "expect_money_same")) {
            c.kind = C_MONEY_SAME;
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
        } else if (!strcmp(cmd, "fixture_nick")) {
            c.kind = C_NICK;
            c.size = atoi(a);
            snprintf(c.text, sizeof(c.text), "fixture_nick");
            snprintf(c.text + 16, sizeof(c.text) - 16, "%s", b);
            for (char* q = c.text + 16; *q; q++) if (*q == '_') *q = ' ';
        } else if (!strcmp(cmd, "expect_text")) {
            // expect_text SYM Some_words: a game string (game charset) has them
            c.kind = C_EXPECT_TEXT;
            c.addr = symAddr(a);
            snprintf(c.text, sizeof(c.text), "expect_text");
            snprintf(c.text + 16, sizeof(c.text) - 16, "%s", b);
            for (char* q = c.text + 16; *q; q++) if (*q == '_') *q = ' ';
        } else if (!strcmp(cmd, "expect_cb2")) {
            // expect_cb2 FUNC: gMain.callback2 is FUNC now.
            c.kind = C_EXPECT_CB2;
            c.addr = symAddr(a) & ~1u;
            snprintf(c.text, sizeof(c.text), "%s", a);
        } else if (!strcmp(cmd, "note_text") || !strcmp(cmd, "expect_noted") || !strcmp(cmd, "expect_not_noted")) {
            // note_text Some_words: from now on, watch every frame's
            // gStringVar4 for them; expect_noted: they were seen;
            // expect_not_noted: they were not.
            c.kind = C_NOTE_TEXT;
            c.size = !strcmp(cmd, "note_text") ? 1 : !strcmp(cmd, "expect_not_noted") ? 2 : 0;
            snprintf(c.text, sizeof(c.text), "%s", cmd);
            snprintf(c.text + 16, sizeof(c.text) - 16, "%s", c.size == 1 ? a : "");
            for (char* q = c.text + 16; *q; q++) if (*q == '_') *q = ' ';
        } else if (!strcmp(cmd, "until_field_text")) {
            // until_field_text Some_words_here [timeout]: taps A meanwhile
            c.kind = C_UNTIL_FTEXT;
            c.frames = n >= 4 ? atoi(b) : 3000;
            snprintf(c.text, sizeof(c.text), "until_field_text");
            snprintf(c.text + 16, sizeof(c.text) - 16, "%s", a);
            for (char* q = c.text + 16; *q; q++) if (*q == '_') *q = ' ';
        } else if (!strcmp(cmd, "until_battle_text")) {
            // until_battle_text Some_words_here [timeout]
            c.kind = C_UNTIL_TEXT;
            c.frames = n >= 4 ? atoi(b) : 3000;
            snprintf(c.text, sizeof(c.text), "until_battle_text%s", "");
            c.value = n >= 5 && !strcmp(d, "auto"); // battle autopilot meanwhile
            snprintf(c.text + 16, sizeof(c.text) - 16, "%s", a);
            for (char* q = c.text + 16; *q; q++) if (*q == '_') *q = ' ';
        } else if (!strcmp(cmd, "fixture_item") || !strcmp(cmd, "expect_item")) {
            // POCKET (0 items, 1 balls, 2 TM/HM, 3 berries, 4 key) ITEM QTY [SLOT]
            // (fixture_item with SLOT: that pocket row, whatever it held)
            c.kind = !strcmp(cmd, "fixture_item") ? C_ITEM : C_EXPECT_ITEM;
            c.size = atoi(a);
            c.addr = strtoul(b, NULL, 0);
            c.value = strtoul(d, NULL, 0);
            c.frames = n >= 6 ? atoi(e) : -1;
        } else if (!strcmp(cmd, "dex_caught") || !strcmp(cmd, "dex_seen") || !strcmp(cmd, "expect_partner_dex") || !strcmp(cmd, "expect_dex_caught")) {
            c.kind = C_DEX;
            c.size = !strcmp(cmd, "dex_caught") ? 1 : !strcmp(cmd, "expect_dex_caught") ? 2 : !strcmp(cmd, "dex_seen") ? 3 : 0;
            c.addr = strtoul(a, NULL, 0);
            c.value = strtoul(b, NULL, 0);
        } else if (!strcmp(cmd, "expect_txn")) {
            c.kind = C_TXN;
            c.value = strtoul(a, NULL, 0);
        } else if (!strcmp(cmd, "fixture_slot")) {
            c.kind = C_SLOT;
            c.size = atoi(a);
            c.value = strtoul(b, NULL, 0);
            c.frames = n >= 5 ? (int)strtoul(d, NULL, 0) : -1;
        } else if (!strcmp(cmd, "expect_species")) {
            c.kind = C_SPECIES;
            c.size = atoi(a);
            c.value = strtoul(b, NULL, 0);
        } else if (!strcmp(cmd, "note_party") || !strcmp(cmd, "expect_party_same")) {
            c.kind = !strcmp(cmd, "note_party") ? C_NOTE_PARTY : C_SAME_PARTY;
        } else if (!strcmp(cmd, "patch_ret")) {
            c.kind = C_PATCH_RET;
            c.addr = symAddr(a);
            c.frames = n >= 4 ? atoi(b) : 0;
            snprintf(c.text, sizeof(c.text), "%s", a);
        } else if (!strcmp(cmd, "fixture_swap")) {
            c.kind = C_SWAP;
            c.size = atoi(a);
            c.value = strtoul(b, NULL, 0);
        } else if (!strcmp(cmd, "fixture_fill_pocket") || !strcmp(cmd, "fixture_unfill_pocket")) {
            c.kind = !strcmp(cmd, "fixture_fill_pocket") ? C_FILL : C_UNFILL;
            c.size = atoi(a);
            c.value = n >= 4 ? strtoul(b, NULL, 0) : 0;
        } else if (!strcmp(cmd, "fixture_gender")) {
            c.kind = C_GENDER;
            c.value = strtoul(a, NULL, 0);
        } else if (!strcmp(cmd, "expect_legend") || !strcmp(cmd, "fixture_legend")) {
            // expect_legend FIELD VALUE / fixture_legend FIELD VALUE: this save's
            // shared-legendary records (CoopSave layout 3), FIELD claimed,
            // pending or owed: a mask with one bit per COOP_ENC_* (bit id - 1).
            c.kind = C_LEGEND;
            c.size = !strcmp(a, "pending") ? 1 : !strcmp(a, "owed") ? 2 : 0;
            c.value = strtoul(b, NULL, 0);
            c.frames = !strcmp(cmd, "fixture_legend");
            snprintf(c.text, sizeof(c.text), "%s %s %s", cmd, a, b);
        } else if (!strcmp(cmd, "legend_owned")) {
            // legend_owned SPECIES MIN [MAX]: Pokemon of SPECIES in the party and
            // PC whose OT is this save's own trainer; each is printed (where,
            // personality, met location and level, IVs).
            c.kind = C_OWNED;
            c.value = strtoul(a, NULL, 0);
            c.addr = strtoul(b, NULL, 0);
            c.size = n >= 5 ? atoi(d) : (int)c.addr;
            snprintf(c.text, sizeof(c.text), "legend_owned %s %s", a, b);
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
			else if (!strcmp(a, "POKEDEX")) c.value = 0;
			else if (!strcmp(a, "OPTION")) c.value = 6;
			else { fprintf(stderr, "menu_to %s\n", a); exit(1); }
			snprintf(c.text, sizeof(c.text), "menu %s", a);
		} else if (!strcmp(cmd, "expect_battle")) {
			c.kind = C_BATTLE;
			c.value = n >= 3 && !strcmp(a, "moves") ? 1 : n >= 3 && !strcmp(a, "hp") ? 2 : 0;
			snprintf(c.text, sizeof(c.text), "battle %s", c.value ? "moves" : "roster");
		} else if (!strcmp(cmd, "expect_range")) {
			// expect_range SYM[+off]:size LOW HIGH [MASK] (inclusive; the
			// value ANDed with MASK first when one is given)
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
			c.args[0] = n >= 6 ? (unsigned)strtoul(e, NULL, 0) : 0xFFFFFFFFu;
			snprintf(c.text, sizeof(c.text), "%s in %s..%s%s%s", a, b, d, n >= 6 ? " mask " : "", n >= 6 ? e : "");
		} else if (!strcmp(cmd, "expect_mirror") || !strcmp(cmd, "expect_equal")) {
			// expect_mirror SYM[+off]:size SYM[+off] (this game vs the other)
			// expect_equal  SYM[+off]:size SYM[+off] (two places on this game)
			char s1[96], s2[96]; unsigned o1 = 0, o2 = 0; int size = 1;
			strncpy(s1, a, sizeof(s1) - 1); s1[sizeof(s1) - 1] = 0;
			strncpy(s2, b, sizeof(s2) - 1); s2[sizeof(s2) - 1] = 0;
			char* colon = strchr(s1, ':');
			if (colon) { size = atoi(colon + 1); *colon = 0; }
			char* plus = strchr(s1, '+');
			if (plus) { o1 = strtoul(plus + 1, NULL, 0); *plus = 0; }
			if ((colon = strchr(s2, ':'))) *colon = 0;
			if ((plus = strchr(s2, '+'))) { o2 = strtoul(plus + 1, NULL, 0); *plus = 0; }
			c.kind = C_MIRROR; c.size = size;
			c.addr = symAddr(s1) + o1; c.value = symAddr(s2) + o2;
			c.args[0] = !strcmp(cmd, "expect_equal");
			snprintf(c.text, sizeof(c.text), "mirror %s ~ %s", a, b);
		} else if (!strcmp(cmd, "expect_tint")) {
			// expect_tint 1|0 [LABEL]: whether the shown map palettes (BG 2-6)
			// differ from the unfaded ones, i.e. carry the time-of-day tint.
			c.kind = C_TINT; c.value = strtoul(a, NULL, 0);
			strncpy(c.text, n >= 3 ? b : "", sizeof(c.text) - 1);
		} else if (!strcmp(cmd, "leak_note") || !strcmp(cmd, "leak_check")) {
			// leak_note: remember this game's tasks, sprites, sprite palette
			// tags and heap blocks/bytes; leak_check: the same now (on the
			// field, at a quiet point).
			c.kind = C_LEAK;
			c.value = !strcmp(cmd, "leak_check");
			snprintf(c.text, sizeof(c.text), "%s", n >= 3 ? a : "");
		} else if (!strcmp(cmd, "writep_other") || !strcmp(cmd, "expectp") || !strcmp(cmd, "expectp_same")) {
			// FIXTURE writep_other PTRSYM+off:size SRCPTRSYM+off: write through
			// a pointer a value read through a pointer on the OTHER game
			// (e.g. the partner's trainer id). expectp PTRSYM+off:size VALUE:
			// check a value read through a pointer.
			char sym[96], src[96]; unsigned off = 0, soff = 0; int size = 1;
			strncpy(sym, a, sizeof(sym) - 1); sym[sizeof(sym) - 1] = 0;
			char* colon = strchr(sym, ':');
			if (colon) { size = atoi(colon + 1); *colon = 0; }
			char* plus = strchr(sym, '+');
			if (plus) { off = strtoul(plus + 1, NULL, 0); *plus = 0; }
			c.kind = C_WRITEP; c.addr = symAddr(sym); c.frames = off; c.size = size;
			if (!strcmp(cmd, "expectp_same")) {
				c.args[1] = 3;
			} else if (!strcmp(cmd, "expectp")) {
				c.args[1] = 2; c.value = symValue(b);
			} else {
				strncpy(src, b, sizeof(src) - 1); src[sizeof(src) - 1] = 0;
				if ((plus = strchr(src, '+'))) { soff = strtoul(plus + 1, NULL, 0); *plus = 0; }
				c.args[1] = 1; c.args[2] = symAddr(src); c.args[3] = soff;
			}
			strncpy(c.text, sym, sizeof(c.text) - 1);
		} else if (!strcmp(cmd, "writep")) {
			// FIXTURE writep PTRSYM+off:size value: write through a pointer
			// variable (gSaveBlock1Ptr, gSaveBlock2Ptr), resolved when run.
			char sym[96]; unsigned off = 0; int size = 1;
			strncpy(sym, a, sizeof(sym) - 1);
			char* colon = strchr(sym, ':');
			if (colon) { size = atoi(colon + 1); *colon = 0; }
			char* plus = strchr(sym, '+');
			if (plus) { off = strtoul(plus + 1, NULL, 0); *plus = 0; }
			c.kind = C_WRITEP; c.addr = symAddr(sym); c.frames = off; c.size = size; c.value = symValue(b);
			strncpy(c.text, sym, sizeof(c.text) - 1);
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
		if (!c.text[0]) snprintf(c.text, sizeof(c.text), "%s %s %s %s", cmd, a, b, d);
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
	signal(SIGUSR1, onUsr1);
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

	for (int i = 0; i < 2; i++) { P[i].id = i; P[i].forceMove = -1; P[i].forceTarget = -1; }
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
		// The cartridge clock runs with emulated time, so a run is the same
		// on any host and at any harness speed (1 emulated second = 60
		// frames, as on hardware). RTC_EPOCH<i>=unix seconds picks where it
		// starts (a test's time of day); otherwise 2026-10-29 10:00 UTC
		// (build 0xC017013E; before it the default was the host's clock).
		// RTC_WALL<i>=1 uses the host's wall clock instead, as an emulator
		// running faster than real time does (tests of real-time waits).
		{
			char name[32];
			snprintf(name, sizeof(name), "RTC_EPOCH%d", i);
			const char* epoch = getenv(name);
			snprintf(name, sizeof(name), "RTC_WALL%d", i);
			const char* wall = getenv(name);
			if (!(wall && wall[0] == '1')) {
				p->core->rtc.override = RTC_FAKE_EPOCH;
				p->core->rtc.value = (int64_t)strtoll(epoch && epoch[0] ? epoch : "1793268000", NULL, 0) * 1000;
			}
		}
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
		if (sAbortRun) {
			// Let the assert screen be pressed past and dumped first.
			static int grace = 0;
			if (sAbortRun == 1 || ++grace > 50) {
				out("ABORTED: %s (critical failure; the rest of the scenario was not run)\n", sAbortWhy);
				break;
			}
		}
		if (mCoreThreadHasCrashed(&P[0].thread) || mCoreThreadHasCrashed(&P[1].thread)) {
			fprintf(stderr, "core crashed\n");
			for (int i = 0; i < 2; i++) {
				if (!mCoreThreadHasCrashed(&P[i].thread)) continue;
				struct ARMCore* cpu = ((struct GBA*)P[i].core->board)->cpu;
				out("[p%d CRASH] pc=%08x lr=%s (%08x) sp=%08x r0=%08x r1=%08x r2=%08x r3=%08x cb2=%s\n", i, cpu->gprs[15],
				    symAt(cpu->gprs[14]), cpu->gprs[14], cpu->gprs[13], cpu->gprs[0], cpu->gprs[1], cpu->gprs[2], cpu->gprs[3],
				    symName(rd32(&P[i], sMain + 4)));
				for (int k = 0; k < 24; k++) {
					unsigned w = rd32(&P[i], cpu->gprs[13] + k * 4);
					if ((w >> 24) == 0x08) out("[p%d CRASH] stack+%02x %s\n", i, k * 4, symAt(w));
				}
			}
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
	// A failed run's saves are never a checkpoint: rename them so no later
	// step (a chained scenario, a journey) can continue from them.
	if (sFailures) {
		for (int i = 0; i < 2; i++) {
			char from[600], to[620];
			snprintf(from, sizeof(from), "%s/p%d.sav", sOutDir, i);
			snprintf(to, sizeof(to), "%s/p%d.sav.quarantined", sOutDir, i);
			rename(from, to);
		}
		char qf[600]; snprintf(qf, sizeof(qf), "%s/QUARANTINED", sOutDir);
		FILE* q = fopen(qf, "w");
		if (q) { fprintf(q, "failures=%d%s%s\n", sFailures, sAbortRun ? " aborted: " : "", sAbortRun ? sAbortWhy : ""); fclose(q); }
		out("saves quarantined (p0.sav.quarantined, p1.sav.quarantined)\n");
	}
	out("frames p0=%d p1=%d failures=%d\n", P[0].frame, P[1].frame, sFailures);
	_exit(sFailures ? 1 : 0);
}
