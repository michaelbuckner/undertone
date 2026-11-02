#!/usr/bin/env python3
"""Package checked runtime files and complete source from the same Git revisions."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import zipfile

JUCE_COMMIT = "3fb5cbc0340758596dac39f7c8de47d562b4896e"


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args])


def add_git_tree(z, repo, revision, prefix):
    """Read Git blobs, never untracked files, local settings, or credentials."""
    for row in git(repo, "ls-tree", "-rz", revision).split(b"\0"):
        if not row:
            continue
        metadata, name = row.split(b"\t", 1)
        mode, kind, sha = metadata.decode().split()
        if kind != "blob":
            raise RuntimeError("Source package contains an unsupported Git entry")
        info = zipfile.ZipInfo(prefix + name.decode("utf-8"))
        info.create_system = 3
        info.external_attr = int(mode, 8) << 16
        info.compress_type = zipfile.ZIP_DEFLATED
        z.writestr(info, git(repo, "cat-file", "blob", sha))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", choices=["Windows-x64", "macOS-universal"], required=True)
    parser.add_argument("--stage", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--juce-source", type=Path, required=True)
    parser.add_argument("--include-source", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    version = re.search(r"project\(Undertone VERSION ([0-9.]+)", (root / "CMakeLists.txt").read_text()).group(1)
    head = git(root, "rev-parse", "HEAD").decode().strip()
    dependency = git(args.juce_source, "rev-parse", "HEAD").decode().strip()
    if dependency != JUCE_COMMIT:
        raise RuntimeError("JUCE revision does not match the pinned source")
    # The source archive is made from Git; prevent stale source with newer binaries.
    if git(root, "status", "--porcelain", "--untracked-files=no").strip():
        raise RuntimeError("Commit tracked changes before packaging a release")
    stage = args.stage.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    for forbidden in ("bin", "include", "lib"):
        if (stage / forbidden).exists():
            raise RuntimeError("JUCE development files found in runtime package: " + forbidden)
    for required in ("LICENSE", "COPYRIGHT", "THIRD-PARTY-NOTICES.md", "docs/source-and-license.md", "licenses/JUCE-LICENSE.md", "licenses/JUCE.spdx.json", "licenses/JUCE-THIRD-PARTY-NOTICES.txt"):
        if not (stage / required).is_file():
            raise RuntimeError("Missing distribution notice: " + required)
    binary = stage / ("Undertone.exe" if args.platform == "Windows-x64" else "Undertone.app/Contents/MacOS/Undertone")
    if not binary.is_file():
        raise RuntimeError("Missing app executable")
    if args.platform == "macOS-universal":
        subprocess.run(["lipo", str(binary), "-verify_arch", "arm64", "x86_64"], check=True)
        if not os.access(binary, os.X_OK):
            raise RuntimeError("Mac executable permission is missing")
    source_name = f"Undertone-{version}-source.zip"
    manifest = {
        "version": version, "platform": args.platform, "undertone_commit": head,
        "juce_commit": JUCE_COMMIT,
        "license": "AGPL-3.0-only",
        "source_archive": source_name,
        "source_url": f"https://github.com/michaelbuckner/undertone/releases/download/v{version}/{source_name}",
        "files": {p.relative_to(stage).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in sorted(stage.rglob("*")) if p.is_file() and p.name != "BUILD-MANIFEST.json"},
    }
    (stage / "BUILD-MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    runtime = args.output / f"Undertone-{version}-{args.platform}.zip"
    with zipfile.ZipFile(runtime, "w", zipfile.ZIP_DEFLATED) as z:
        for path in sorted(stage.rglob("*")):
            if path.is_file():
                if path.is_symlink():
                    raise RuntimeError("Unexpected runtime symlink: " + path.name)
                z.write(path, "Undertone/" + path.relative_to(stage).as_posix())
    archives = [runtime]
    if args.include_source:
        source = args.output / source_name
        prefix = f"Undertone-{version}-source/"
        with zipfile.ZipFile(source, "w", zipfile.ZIP_DEFLATED) as z:
            add_git_tree(z, root, head, prefix)
            add_git_tree(z, args.juce_source, JUCE_COMMIT, prefix + "vendor/JUCE/")
            z.writestr(prefix + "SOURCE-MANIFEST.json", json.dumps({k: v for k, v in manifest.items() if k != "files"}, indent=2) + "\n")
        archives.append(source)
    for archive in archives:
        with zipfile.ZipFile(archive) as z:
            if z.testzip() is not None:
                raise RuntimeError("Archive CRC failure")
        print(hashlib.sha256(archive.read_bytes()).hexdigest(), archive.name)


if __name__ == "__main__":
    main()
