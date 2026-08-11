#!/usr/bin/env python3
from __future__ import annotations

import argparse
import gzip
import io
import json
from pathlib import Path
import sys
import urllib.request
import zipfile

VERSION = "1.12.2"
VERSION_MANIFEST = "https://piston-meta.mojang.com/mc/game/version_manifest_v2.json"
WANTED_PREFIXES = (
    "assets/minecraft/structures/mansion/",
    "assets/minecraft/structures/endcity/",
)

def get_json(url: str) -> dict:
    req = urllib.request.Request(url, headers={"User-Agent": "MinecraftClone-1.12.2-AssetInstaller/1.0"})
    with urllib.request.urlopen(req, timeout=60) as response:
        return json.load(response)

def get_bytes(url: str) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": "MinecraftClone-1.12.2-AssetInstaller/1.0"})
    with urllib.request.urlopen(req, timeout=120) as response:
        return response.read()

def resolve_client_url() -> str:
    manifest = get_json(VERSION_MANIFEST)
    version_info_url = None

    for version in manifest.get("versions", []):
        if version.get("id") == VERSION:
            version_info_url = version.get("url")
            break

    if not version_info_url:
        raise RuntimeError(f"Minecraft {VERSION} was not found in Mojang's version manifest.")

    version_info = get_json(version_info_url)
    client = version_info.get("downloads", {}).get("client", {})
    url = client.get("url")

    if not url:
        raise RuntimeError(f"Could not find the official Minecraft {VERSION} client download URL.")

    return url

def install(project_root: Path) -> int:
    project_root = project_root.resolve()
    destination_root = project_root / "assets" / "minecraft" / "structures"

    if not project_root.is_dir():
        print(f"ERROR: Project folder does not exist: {project_root}", file=sys.stderr)
        return 2

    print(f"Project: {project_root}")
    print(f"Resolving official Minecraft {VERSION} client...")
    client_url = resolve_client_url()

    print(f"Downloading official Minecraft {VERSION} client JAR...")
    jar_data = get_bytes(client_url)

    mansion_count = 0
    endcity_count = 0

    print("Extracting Woodland Mansion and End City templates...")
    with zipfile.ZipFile(io.BytesIO(jar_data), "r") as jar:
        members = [
            name for name in jar.namelist()
            if name.endswith(".nbt") and name.startswith(WANTED_PREFIXES)
        ]

        if not members:
            raise RuntimeError("No mansion/endcity structure templates were found in the 1.12.2 client JAR.")

        for member in sorted(members):
            raw = jar.read(member)

            # Vanilla 1.12.2 bundled structure NBT resources are gzip streams.
            # MinecraftClone's StructureTemplate loader expects uncompressed NBT.
            if raw[:2] == b"\x1f\x8b":
                raw = gzip.decompress(raw)

            relative = Path(member).relative_to("assets/minecraft/structures")
            target = destination_root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(raw)

            if relative.parts[0] == "mansion":
                mansion_count += 1
            elif relative.parts[0] == "endcity":
                endcity_count += 1

    print()
    print("Installed successfully.")
    print(f"  Mansion templates: {mansion_count}")
    print(f"  End City templates: {endcity_count}")
    print(f"  Destination: {destination_root}")

    if mansion_count == 0 or endcity_count == 0:
        print("WARNING: One of the expected structure folders was empty.", file=sys.stderr)
        return 3

    return 0

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Install exact vanilla Minecraft 1.12.2 mansion/endcity NBT templates into MinecraftClone."
    )
    parser.add_argument(
        "project_root",
        type=Path,
        help="Path to your MinecraftClone project root",
    )
    args = parser.parse_args()

    try:
        return install(args.project_root)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

if __name__ == "__main__":
    raise SystemExit(main())
