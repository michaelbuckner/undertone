#!/usr/bin/env python3
"""Collect the pinned dependency's original notices without installing its SDK."""
from pathlib import Path
import argparse
import hashlib
import re
import shutil


def prepare(juce: Path, output: Path) -> None:
    output.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(juce / "LICENSE.md", output / "JUCE-LICENSE.md")
    shutil.copyfile(juce / "JUCE.spdx.json", output / "JUCE.spdx.json")
    notices = [
        "JUCE 9.0.4 and bundled dependency notices\n",
        "Original upstream notices are reproduced below. This collection intentionally\n"
        "includes notices for optional JUCE modules as well as Undertone's dependencies.\n"
        "Inclusion is not a claim that every optional component is compiled into Undertone.\n"
        "See JUCE.spdx.json and the matching source archive for provenance and full sources.\n",
    ]
    seen = set()
    for path in sorted(juce.rglob("*")):
        if not path.is_file() or ".git" in path.relative_to(juce).parts:
            continue
        # Full notice files preserve multi-part terms (e.g. JPEG's README).
        name = path.name.upper()
        full_notice = name.startswith(("LICENSE", "LICENCE", "COPYING", "COPYRIGHT", "NOTICE", "README"))
        if not full_notice and path.suffix.lower() not in {".h", ".hpp", ".c", ".cpp", ".cc", ".mm", ".m", ".inc"}:
            continue
        raw = path.read_bytes()
        if b"\0" in raw:
            continue
        text = raw.decode("utf-8", errors="replace")
        if full_notice:
            blocks = [text]
        else:
            blocks = re.findall(r"/\*[\s\S]*?\*/|(?:(?:^|\n)[ \t]*//[^\n]*)+", text)
            blocks = [b for b in blocks if re.search(r"copyright|redistribution|permission is hereby granted", b, re.I)]
        for block in blocks:
            key = hashlib.sha256(block.encode("utf-8")).hexdigest()
            if key in seen:
                continue
            seen.add(key)
            notices.extend(["\n" + "=" * 72 + "\n", path.relative_to(juce).as_posix() + "\n\n", block, "\n"])
    (output / "JUCE-THIRD-PARTY-NOTICES.txt").write_text("".join(notices), encoding="utf-8", newline="\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--juce-source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    prepare(args.juce_source.resolve(), args.output.resolve())
