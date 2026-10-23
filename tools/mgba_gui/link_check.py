#!/usr/bin/env python3
"""Prove two linked mGBA Qt windows on this machine.

Copies the ROM to disposable names, starts one mGBA process, opens its
multiplayer window, and checks that both cores are attached and drawing.
Saves go under the work directory, never next to the source ROM.

This does not play through the story. The headless harness in
tools/coop_link_harness does that. mGBA only links windows that share a
process; two separate mgba-qt processes are not a link session.
"""

import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

from PIL import Image

DISPLAY = os.environ.get("DISPLAY", ":1")
MGBA = os.environ.get("MGBA_QT", "/usr/games/mgba-qt")


def run(args, **kw):
    env = os.environ.copy()
    env["DISPLAY"] = DISPLAY
    return subprocess.run(args, env=env, check=False, text=True, capture_output=True, **kw)


def xdotool(*args):
    result = run(["xdotool", *args])
    if result.returncode != 0:
        raise RuntimeError(f"xdotool {' '.join(args)} failed: {result.stderr.strip()}")
    return result.stdout.strip()


def window_names():
    listed = run(["xwininfo", "-root", "-tree"])
    names = []
    for line in listed.stdout.splitlines():
        if "mGBA -" not in line or '"' not in line:
            continue
        wid, _, rest = line.strip().partition(" ")
        title = rest.split('"', 2)[1]
        names.append((wid, title))
    return names


def wait_until(predicate, timeout, what):
    deadline = time.time() + timeout
    last = None
    while time.time() < deadline:
        last = predicate()
        if last:
            return last
        time.sleep(0.25)
    raise TimeoutError(f"timed out waiting for {what}; last={last!r}")


def write_config(cfg_dir: Path, player1: Path):
    qt = cfg_dir / "mgba" / "qt.ini"
    qt.parent.mkdir(parents=True, exist_ok=True)
    qt.write_text(
        "\n".join([
            "[General]",
            "displayDriver=0",
            "",
            "[shortcutKey]",
            "multiWindow=Ctrl+Shift+M",
            "",
            "[mru]",
            f"0={player1}",
            "",
        ]) + "\n"
    )


def colorful(path: Path) -> bool:
    if not path.is_file() or path.stat().st_size < 1000:
        return False
    im = Image.open(path).convert("RGB").resize((80, 60))
    return len(set(im.get_flattened_data())) >= 12


def core_shots(rom_dir: Path, windows):
    for old in rom_dir.glob("*.png"):
        old.unlink()
    for wid, _title in windows:
        xdotool("windowactivate", "--sync", wid)
        xdotool("key", "--window", wid, "F12")
        time.sleep(0.25)


def wid_int(value: str) -> int:
    return int(value, 0)


def fps_of(title: str):
    if " fps)" not in title:
        return None
    try:
        return float(title.split("(")[-1].split(" fps)")[0])
    except ValueError:
        return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=Path("/workspace/pokeemerald.gba"))
    parser.add_argument("--work", type=Path, default=Path("/tmp/mgba-coop"))
    parser.add_argument("--keep", action="store_true", help="leave mGBA running")
    args = parser.parse_args()

    if not args.rom.is_file():
        sys.exit(f"ROM not found: {args.rom}")
    if not Path(MGBA).is_file():
        sys.exit(f"mgba-qt not found: {MGBA}")

    work = args.work
    roms = work / "roms"
    saves = work / "saves"
    shots = work / "shots"
    cfg = work / "config"
    runtime = work / "runtime"
    for path in (roms, saves, shots, runtime):
        path.mkdir(parents=True, exist_ok=True)
    os.chmod(runtime, 0o700)
    for old in saves.glob("*"):
        old.unlink()
    for old in roms.glob("*.png"):
        old.unlink()

    player0 = roms / "player0.gba"
    player1 = roms / "player1.gba"
    shutil.copyfile(args.rom, player0)
    shutil.copyfile(args.rom, player1)
    write_config(cfg, player1.resolve())

    run(["pkill", "-f", MGBA])
    wait_until(lambda: run(["pgrep", "-f", MGBA]).returncode != 0, 8, "old mGBA to exit")

    env = os.environ.copy()
    env.update({
        "DISPLAY": DISPLAY,
        "XDG_CONFIG_HOME": str(cfg),
        "XDG_RUNTIME_DIR": str(runtime),
        "SDL_AUDIODRIVER": "dummy",
        "QT_QPA_PLATFORM": "xcb",
    })
    log = open(work / "mgba.log", "w")
    proc = subprocess.Popen(
        [MGBA, "-2", "-C", "useBios=0", "-C", "audioSync=0", "-C", "videoSync=1",
         "-C", "fpsTarget=60", "-C", f"savegamePath={saves}", str(player0)],
        env=env, stdout=log, stderr=subprocess.STDOUT,
    )
    report = []
    try:
        def primary_ready():
            item = next((item for item in window_names() if "POKEMON" in item[1] and "fps" in item[1]), None)
            return item

        primary = wait_until(primary_ready, 20, "the first mGBA window to start drawing")
        report.append(f"window0 {primary[0]} {primary[1]}")

        def open_second():
            xdotool("windowactivate", "--sync", primary[0])
            time.sleep(0.2)
            xdotool("key", "ctrl+shift+m")
            time.sleep(0.6)
            return next((item for item in window_names() if item[0] != primary[0] and item[1].startswith("mGBA -")), None)

        second = wait_until(open_second, 15, "the multiplayer window")
        xdotool("windowmove", second[0], "60", "50")
        xdotool("windowactivate", "--sync", second[0])
        time.sleep(0.3)
        xdotool("key", "--window", second[0], "ctrl+1")

        def linked():
            names = window_names()
            titles = [title for _, title in names]
            if any("Player 1 of 2" in t for t in titles) and any("Player 2 of 2" in t for t in titles):
                rates = [fps_of(t) for t in titles]
                if all(rate is not None and rate > 1 for rate in rates):
                    return names
            return None

        names = wait_until(linked, 20, "both players drawing")
        for wid, title in names:
            report.append(f"linked {wid} {title}")

        focused = next(wid for wid, title in names if "Player 2 of 2" in title)
        other = next(wid for wid, title in names if "Player 1 of 2" in title)

        def both_drawn():
            core_shots(roms, names)
            paths = [roms / "player0-0.png", roms / "player1-0.png"]
            if all(colorful(path) for path in paths):
                return [path.read_bytes() for path in paths]
            return None

        before = wait_until(both_drawn, 25, "both cores to draw a real frame")
        report.append("both cores drew a non-blank frame")

        # Freeze player 1, then press Start only on player 2.
        xdotool("windowactivate", "--sync", other)
        xdotool("key", "--window", other, "ctrl+p")
        time.sleep(0.3)
        core_shots(roms, names)
        frozen = (roms / "player0-0.png").read_bytes()
        xdotool("windowactivate", "--sync", focused)
        active = xdotool("getactivewindow")
        report.append(f"focus {active} wanted {focused}")
        if wid_int(active) != wid_int(focused):
            raise RuntimeError("player 2 did not take keyboard focus")
        for _ in range(4):
            xdotool("key", "Return")
            time.sleep(0.2)
        time.sleep(0.4)
        core_shots(roms, names)
        stayed = (roms / "player0-0.png").read_bytes()
        moved = (roms / "player1-0.png").read_bytes()
        if stayed != frozen:
            raise RuntimeError("player 1 changed while it was paused")
        if moved == before[1]:
            raise RuntimeError("player 2 did not change after Start")
        report.append(f"player 1 stayed frozen ({len(stayed)} bytes)")
        report.append(f"player 2 advanced ({len(before[1])} -> {len(moved)} bytes)")

        workspace_sav = list(args.rom.parent.glob("*.sav"))
        report.append(f"workspace sav files {len(workspace_sav)}")
        report.append(f"disposable saves {[p.name for p in sorted(saves.iterdir())]}")
        print("\n".join(report))
        print("PASS two linked mGBA windows")
        return 0
    except Exception as exc:
        report.append(f"FAIL {exc}")
        print("\n".join(report), file=sys.stderr)
        print("windows:", window_names(), file=sys.stderr)
        print((work / "mgba.log").read_text()[-2000:], file=sys.stderr)
        return 1
    finally:
        log.close()
        if not args.keep:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()


if __name__ == "__main__":
    sys.exit(main())
