// Two linked mGBA cores, headless, driven by a script. See README.md.
#include <mgba/core/core.h>
#include <mgba/core/thread.h>
#include <mgba/core/log.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/sio/lockstep.h>
#include <mgba-util/vfs.h>

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

enum { C_PARTNER, C_GOTO, C_HEX, C_PRESS, C_WAIT, C_SHOT, C_DUMP, C_BARRIER, C_UNTIL_CB2, C_UNTIL_FIELD, C_EXPECT, C_UNTIL_MEM };

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

static unsigned coopStatic(const char* name) {
	char full[80];
	snprintf(full, sizeof(full), "coop.%s", name);
	return symAddr(full);
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
	out("[f%06d p%d %s] cb1=%s cb2=%s map=%d.%d pos=(%d,%d) partner=%s lock=%u script=%u battle=%08x linkErr=%u"
	    " linkId=%u remote=%u.%u@(%u,%u) coop{state=%u req=%u wait=%u slot=%u kind=%u inBattle=%u valid=%u guest=%u hold=%u}\n",
	    p->frame, p->id, tag, symName(cb1), symName(cb2), mg, mn, px, py, partner, rd8(p, sLockCtl),
	    sScriptStatus ? rd8(p, sScriptStatus) : 0, rd32(p, sBattleFlags), rd8(p, sLinkErr),
	    rd8(p, symAddr("gLink") + 2),
	    rd8(p, remote + 2), rd8(p, remote + 3), rd8(p, remote + 4), rd8(p, remote + 5),
	    rd8(p, coopStatic("sLinkState")), rd8(p, coopStatic("sRequest")), rd8(p, coopStatic("sWaitKind")),
	    rd8(p, coopStatic("sSlot")), rd8(p, coopStatic("sBattleKind")), rd8(p, coopStatic("sInBattle")),
	    rd8(p, coopStatic("sPartnerValid")), rd8(p, coopStatic("sIsGuest")), rd8(p, symAddr("gCoopHoldLink")));
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
	if (sScriptStatus && rd8(p, sScriptStatus) == 0) return false;
	return true;
}

static void frameCallback(struct mCoreThread* t) {
	struct Player* p = t->userData;
	p->frame++;
	if (p->frame > sMaxFrames && !p->done) {
		out("[p%d] global frame limit hit at pc=%d\n", p->id, p->pc);
		dump(p, "timeout");
		sFailures++;
		p->done = 1;
	}
	unsigned keys = 0;
	while (!p->done) {
		if (p->pc >= p->ncmds) { p->done = 1; break; }
		struct Cmd* c = &p->cmds[p->pc];
		bool advance = false;
		switch (c->kind) {
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
			int found = -1;
			for (int i = 0; i < 16; i++) {
				unsigned ob = sObjEvents + i * 0x24;
				if ((rd8(p, ob) & 1) && rd8(p, ob + 8) == 0xFC) found = i;
			}
			char why[160] = "";
			if (c->value == 0) {
				if (found >= 0) snprintf(why, sizeof(why), "partner sprite shown on another map");
			} else if (!want) {
				snprintf(why, sizeof(why), "players not on the same map (%d.%d vs %d.%d)", mg, mn, omg, omn);
			} else if (found < 0) {
				snprintf(why, sizeof(why), "no partner sprite");
			} else {
				unsigned ob = sObjEvents + found * 0x24;
				unsigned spr = symAddr("gSprites") + rd8(p, ob + 0x23) * 0x44;
				int sx = (int16_t)rd16(p, ob + 0x10), sy = (int16_t)rd16(p, ob + 0x12);
				unsigned objFlags = rd16(p, ob), sprFlags = rd16(p, spr + 0x3E);
				int dist = abs(sx - ox) + abs(sy - oy);
				if (objFlags & (1u << 13)) snprintf(why, sizeof(why), "partner object invisible");
				else if (!(sprFlags & 1) || ((sprFlags & 4) && !(objFlags & (1u << 14)))) snprintf(why, sizeof(why), "partner sprite hidden (flags %04x)", sprFlags);
				else if (dist > 1) snprintf(why, sizeof(why), "partner drawn at (%d,%d), really at (%d,%d)", sx - 7, sy - 7, ox - 7, oy - 7);
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
			bool ok = c->kind == C_UNTIL_CB2 ? (rd32(p, sMain + 4) & ~1u) == c->addr
			        : c->kind == C_UNTIL_FIELD ? inField(p)
			        : rdN(p, c->addr, c->size) == c->value;
			if (c->kind == C_UNTIL_FIELD) {
				p->stable = ok ? p->stable + 1 : 0;
				ok = p->stable >= 20;
			}
			if (ok) { p->stable = 0; advance = true; break; }
			if (c->keys && (p->step % 24) < 2) keys = c->keys;
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
		case C_EXPECT: {
			unsigned v = rdN(p, c->addr, c->size);
			if (v != c->value) {
				out("[p%d] FAIL expect %s: got %u want %u\n", p->id, c->text, v, c->value);
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
		char who[8], cmd[32], a[96] = "", b[96] = "", d[96] = "";
		int n = sscanf(line, "%7s %31s %95s %95s %95s", who, cmd, a, b, d);
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
			snprintf(c.text, sizeof(c.text), "cb2 %s", a);
		} else if (!strcmp(cmd, "until_field") || !strcmp(cmd, "mash_field")) {
			c.kind = C_UNTIL_FIELD; c.frames = n >= 3 ? atoi(a) : 3000;
			if (cmd[0] == 'm') c.keys = n >= 4 ? parseKeys(b) : 1;
			strcpy(c.text, "field control");
		} else if (!strcmp(cmd, "until_mem") || !strcmp(cmd, "expect")) {
			// until_mem SYM[+off]:size value [timeout]
			char sym[96]; unsigned off = 0; int size = 1;
			strncpy(sym, a, sizeof(sym) - 1);
			char* colon = strchr(sym, ':');
			if (colon) { size = atoi(colon + 1); *colon = 0; }
			char* plus = strchr(sym, '+');
			if (plus) { off = strtoul(plus + 1, NULL, 0); *plus = 0; }
			c.kind = !strcmp(cmd, "expect") ? C_EXPECT : C_UNTIL_MEM;
			c.addr = (!strncmp(sym, "coop.", 5) ? symAddr(sym) : symAddr(sym)) + off;
			c.size = size;
			c.value = strtoul(b, NULL, 0);
			c.frames = n >= 5 ? atoi(d) : 3000;
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
	// Tearing down a live lockstep pair can deadlock, so flush the saves and leave.
	for (int i = 0; i < 2; i++) mCoreThreadInterrupt(&P[i].thread);
	out("frames p0=%d p1=%d failures=%d\n", P[0].frame, P[1].frame, sFailures);
	_exit(sFailures ? 1 : 0);
}
