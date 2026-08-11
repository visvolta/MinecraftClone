#!/usr/bin/env python3
"""Install this source overlay into a MinecraftClone checkout and update CMake."""
from __future__ import annotations
import argparse
from pathlib import Path
import shutil
import sys

WORLD_SOURCES = [
    "src/worldgen/EndCityStructure.cpp",
    "src/worldgen/FossilGenerator.cpp",
    "src/worldgen/MineshaftStructure.cpp",
    "src/worldgen/OceanMonumentStructure.cpp",
    "src/worldgen/ScatteredFeatureStructure.cpp",
    "src/worldgen/StrongholdStructure.cpp",
    "src/worldgen/StructurePrimitives.cpp",
    "src/worldgen/StructureTemplate.cpp",
    "src/worldgen/Vanilla112State.cpp",
    "src/worldgen/VillageStructure.cpp",
    "src/worldgen/WoodlandMansionStructure.cpp",
]

SKIP_NAMES = {"STATUS.md", "CMakeLists.txt.patch"}
SKIP_SUFFIXES = {".patch", ".md"}


def copy_overlay(overlay: Path, target: Path) -> None:
    for source in overlay.rglob("*"):
        if not source.is_file():
            continue
        rel = source.relative_to(overlay)
        if rel.parts[0] == "tests" or rel.parts[0] == "tools":
            continue
        if source.name in SKIP_NAMES or source.suffix.lower() in SKIP_SUFFIXES:
            continue
        dest = target / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, dest)


def update_cmake(target: Path) -> None:
    path = target / "CMakeLists.txt"
    text = path.read_text(encoding="utf-8")
    missing = [s for s in WORLD_SOURCES if s not in text]
    if not missing:
        return
    anchor = "    src/worldgen/WorldGenerationContext.cpp\n"
    if anchor not in text:
        raise RuntimeError("Could not find minecraft_world source-list anchor in CMakeLists.txt")
    insertion = "".join(f"    {s}\n" for s in missing)
    text = text.replace(anchor, insertion + anchor, 1)
    path.write_text(text, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("clone_root", type=Path, help="Path to the MinecraftClone checkout")
    parser.add_argument("--mcp-root", type=Path, help="Optional path to mcp940; imports vanilla .nbt templates")
    args = parser.parse_args()
    target = args.clone_root.resolve()
    if not (target / "CMakeLists.txt").is_file() or not (target / "src").is_dir():
        print(f"error: not a MinecraftClone checkout: {target}", file=sys.stderr)
        return 2
    overlay = Path(__file__).resolve().parents[1]
    copy_overlay(overlay, target)
    update_cmake(target)
    if args.mcp_root:
        sys.path.insert(0, str(Path(__file__).parent))
        from import_mcp112_templates import import_templates
        code = import_templates(args.mcp_root.resolve(), target)
        if code:
            return code
    print("Minecraft 1.12.2 worldgen/structure port installed.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
