#!/usr/bin/env python3
"""Import Minecraft 1.12.2 vanilla structure templates from an MCP 9.40 checkout.

MCP stores the bundled .nbt resources gzip-compressed. MinecraftClone's
StructureTemplate loader consumes uncompressed NBT, so this copies every
structure template and transparently gunzips it.
"""
from __future__ import annotations
import argparse
import gzip
from pathlib import Path
import shutil
import sys


def import_templates(mcp_root: Path, clone_root: Path) -> int:
    src = mcp_root / "src" / "minecraft" / "assets" / "minecraft" / "structures"
    dst = clone_root / "assets" / "minecraft" / "structures"
    if not src.is_dir():
        print(f"error: MCP structure directory not found: {src}", file=sys.stderr)
        return 2
    dst.mkdir(parents=True, exist_ok=True)
    count = 0
    for source in sorted(src.rglob("*.nbt")):
        relative = source.relative_to(src)
        target = dst / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        data = source.read_bytes()
        if data[:2] == b"\x1f\x8b":
            data = gzip.decompress(data)
        target.write_bytes(data)
        count += 1
    print(f"Imported {count} vanilla 1.12.2 structure templates into {dst}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("mcp_root", type=Path, help="Path to the mcp940 checkout")
    parser.add_argument("--clone-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    return import_templates(args.mcp_root.resolve(), args.clone_root.resolve())

if __name__ == "__main__":
    raise SystemExit(main())
