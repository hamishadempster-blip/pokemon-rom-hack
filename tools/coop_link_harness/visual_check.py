#!/usr/bin/env python3
"""Visual checks on a harness run (tools/coop_link_harness).

    BATTLE_LOG=1 [SNAP_EVERY=N SNAP_DIR=dir] run_scenario.sh scen.txt out
    visual_check.py out.log [snapdir]

Reads the BATTLE_LOG lines ("[fNNN pX battle] text | b0=species:hp/max ...")
that the harness prints for every new battle message on each screen, and,
when given, the SNAP_EVERY dumps (p<id>_<frame>.bin: VRAM, OAM, palette,
I/O). Each check prints PASS or FAIL with the evidence; the exit status is the
number of failures. These are checks of what the game shows, not of the
emulator's pixels: they read the same text and registers the screen is built
from. A PASS here does not replace looking at the recordings.

Checks:
  victory-text     no "You defeated X and !" (a second trainer that isn't there)
  send-out-match   in each shared battle, both screens announce the same
                   Pokemon for each send-out
  no-phantom       at each "What will ... do?" on the engine's screen, every
                   battler slot is a real Pokemon or marked absent (no
                   species-0 battler that is still "present": the Lv0 box)
  no-blank-name    no message about a Pokemon with no name (the phantom
                   second trainer's empty slot announced at send-out)
  prompt-overlap   "Partner choosing" is never drawn into "What will ... do?"
  learn-prompt     the move-forget question shows on one screen only; the
                   other screen says who is choosing
  bg1-layout       (with snaps) BG1 never keeps the VS intro's 512x256 layout
                   once the battle's own windows use it (BG1CNT 0x5C00/01:
                   the stale VS tilemap seen as blocks at level-ups)
"""
import glob
import os
import re
import struct
import sys

LINE = re.compile(r'^\[f(\d+) p(\d) battle\] (.*?) \|(.*)$')
STATE = re.compile(r'b(\d)=(\d+):(\d+)/(\d+)(\(absent\))?')


def load(path):
    msgs = {0: [], 1: []}
    for raw in open(path, errors='replace'):
        m = LINE.match(raw.rstrip('\n'))
        if not m:
            continue
        frame, pid, raw, state = int(m.group(1)), int(m.group(2)), m.group(3), m.group(4)
        battlers = {int(b): (int(sp), int(hp), int(mx), bool(ab)) for b, sp, hp, mx, ab in STATE.findall(state)}
        # A name with nothing in it, followed by the box's two control
        # characters: the battle announcing a Pokemon that has no name.
        if raw.strip() == '~~' and any(v[0] for v in battlers.values()):
            raw = '<no name> ~~'
        msgs[pid].append((frame, raw.replace('~', ' ').strip(), battlers))
    return msgs


def battles(seq):
    """Split one screen's messages into battles."""
    out, cur = [], []
    for item in seq:
        # A new opening line, or a long quiet spell (back on the field).
        if cur and (re.search(r'challenged by|appeared!|wants to battle', item[1]) or item[0] - cur[-1][0] > 600):
            out.append(cur)
            cur = []
        cur.append(item)
    if cur:
        out.append(cur)
    return out


def sendouts(battle):
    names = []
    for _, text, _ in battle:
        for m in re.finditer(r'sent out ([^!]+)!|Go,? ([^!]+)!|Go! ([^!]+)!', text):
            names.append((m.group(1) or m.group(2) or m.group(3)).strip())
    return sorted(names)


def main():
    log = sys.argv[1]
    snapdir = sys.argv[2] if len(sys.argv) > 2 else None
    msgs = load(log)
    fails = 0

    def report(name, ok, detail=''):
        nonlocal fails
        print(('PASS' if ok else 'FAIL') + ' ' + name + (': ' + detail if detail else ''))
        if not ok:
            fails += 1

    if not msgs[0] and not msgs[1]:
        print('SKIP no BATTLE_LOG lines in ' + log)
        return 0

    bad = [(p, f, t) for p in (0, 1) for f, t, _ in msgs[p] if re.search(r'You defeated .* and\s*!', t)]
    report('victory-text', not bad, '; '.join('p%d f%d "%s"' % b for b in bad[:3]))

    # Each send-out line of a four-battler (co-op) battle on one screen needs
    # a send-out of the same kind on the other screen within 1000 frames
    # naming the same Pokemon. The players' own line differs by perspective
    # ("BLUE sent out A! Go, B!" / "MISTY sent out B! Go, A!"), so names are
    # compared as a set. Solo battles (two battlers) are left out.
    players = set(m.group(1) for p in (0, 1) for _, t, _ in msgs[p] for m in [re.search(r'Waiting for (\S+?)\W*$', t)] if m)

    def own(t):
        return 'Go' in t or any(t.startswith(n + ' sent out') for n in players)

    def outs(p):
        return [(f, t, own(t)) for f, t, st in msgs[p] if len(st) >= 4 and re.search(r'sent out|Go[,!] ', t)]
    o0, o1 = outs(0), outs(1)
    mism, pairs = [], 0
    for f, t, own in o0:
        near = [u for g, u, k in o1 if abs(g - f) < 1000 and k == own]
        if not near:
            continue
        pairs += 1
        names = sendouts([(0, t, {})])
        if not any(sendouts([(0, u, {})]) == names for u in near):
            mism.append('f%d p0 "%s" / p1 "%s"' % (f, t[:60], near[0][:60]))
    report('send-out-match', not mism, '; '.join(mism[:3]) or '%d co-op send-outs seen on both screens' % pairs)

    # The engine's screen is the one whose battler table is filled at the
    # challenge; the other screen fills it in later.
    phantom = []
    for p in (0, 1):
        for f, t, st in msgs[p]:
            if not t.startswith('What will'):
                continue
            filled = [b for b, (sp, hp, mx, ab) in st.items() if sp]
            if not filled:
                continue
            for b, (sp, hp, mx, ab) in st.items():
                if sp == 0 and not ab:
                    phantom.append('p%d f%d battler %d' % (p, f, b))
    report('no-phantom', not phantom, '; '.join(phantom[:3]))

    blank = [(p, f, t) for p in (0, 1) for f, t, _ in msgs[p]
             if t.startswith('<no name>') or re.search(r'sent out\s*!|Go[,!]\s*!', t)]
    report('no-blank-name', not blank, '; '.join('p%d f%d "%s"' % b for b in blank[:3]))

    overlap = [(p, f, t) for p in (0, 1) for f, t, _ in msgs[p] if 'choosing' in t and 'What will' in t]
    report('prompt-overlap', not overlap, '; '.join('p%d f%d "%s"' % b for b in overlap[:3]))

    asks = {p: [f for f, t, _ in msgs[p] if t.startswith('Should another move be forgotten')] for p in (0, 1)}
    told = {p: [f for f, t, _ in msgs[p] if ' is choosing whether ' in t] for p in (0, 1)}
    both = [(f0, f1) for f0 in asks[0] for f1 in asks[1] if abs(f0 - f1) < 600]
    detail = 'questions p0 %d p1 %d, "is choosing" p0 %d p1 %d' % (len(asks[0]), len(asks[1]), len(told[0]), len(told[1]))
    report('learn-prompt', not both, detail if both or asks[0] or asks[1] else 'no move-learning prompt in this run')

    if snapdir:
        hits = []
        files = sorted(glob.glob(os.path.join(snapdir, 'p*_*.bin')))
        for path in files:
            data = open(path, 'rb').read()
            if len(data) < 0x18800 + 0x60:
                continue
            bg1cnt = struct.unpack_from('<H', data, 0x18800 + 0x0A)[0]
            # The VS intro legitimately uses 512x256 with character base 1
            # (0x5C04); the stale layout under the battle's own windows had
            # character base 0 (0x5C00/0x5C01).
            if (bg1cnt & 0xDF0C) == 0x5C00:
                hits.append('%s BG1CNT=%04x' % (os.path.basename(path), bg1cnt))
        report('bg1-layout', not hits, ('%d of %d snaps: ' % (len(hits), len(files)) + '; '.join(hits[:3])) if hits else '%d snaps' % len(files))
    return fails


if __name__ == '__main__':
    sys.exit(main())
