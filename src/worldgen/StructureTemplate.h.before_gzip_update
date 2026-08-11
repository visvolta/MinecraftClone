#pragma once

#include "content/BlockState.h"
#include "worldgen/StructurePrimitives.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class WorldGenerationContext;

namespace mc112
{
struct TemplateNbt
{
    std::unordered_map<std::string,std::string> strings;
    std::unordered_map<std::string,std::int32_t> ints;
    std::unordered_map<std::string,std::int8_t> bytes;
    [[nodiscard]] std::string string(std::string_view key) const;
    [[nodiscard]] int integer(std::string_view key,int fallback=0) const noexcept;
};

struct TemplateBlock
{
    int x=0,y=0,z=0;
    std::uint32_t palette=0;
    std::optional<TemplateNbt> nbt;
};

struct TemplatePaletteEntry
{
    std::string name;
    std::vector<std::pair<std::string,std::string>> properties;
};

enum class Mirror : std::uint8_t { None, LeftRight, FrontBack };

using TemplateMarkerHandler = std::function<void(
    int, int, int, const TemplateNbt&, Rotation)>;

class StructureTemplate
{
public:
    int sizeX=0,sizeY=0,sizeZ=0;
    std::vector<TemplatePaletteEntry> palette;
    std::vector<TemplateBlock> blocks;

    [[nodiscard]] static StructureTemplate loadUncompressed(const std::filesystem::path& file);

    // Exact Template 1.12.2 coordinate helpers (Mirror.NONE). Template block
    // transforms rotate around local (0,0,0), so CW/CCW rotations can produce
    // negative local coordinates. getZeroPositionWithTransform performs the
    // vanilla origin shift used by fossils and other template structures.
    [[nodiscard]] std::array<int,3> transformedSize(Rotation rotation) const noexcept;
    [[nodiscard]] std::array<int,3> transformedBlockPosition(
        int x,int y,int z,Rotation rotation, Mirror mirror=Mirror::None) const noexcept;
    [[nodiscard]] std::array<int,3> getZeroPositionWithTransform(
        int x,int y,int z,Rotation rotation, Mirror mirror=Mirror::None) const noexcept;
    [[nodiscard]] Box transformedBox(int originX,int originY,int originZ,Rotation rotation, Mirror mirror=Mirror::None) const noexcept;

    void place(WorldGenerationContext&,int originX,int originY,int originZ,Rotation rotation,
               const Box& clip,float integrity=1.0f,JavaRandom* random=nullptr,
               bool ignoreStructureBlocks=true,
               const TemplateMarkerHandler& markerHandler={},
               Mirror mirror=Mirror::None) const;
};

class StructureTemplateLibrary
{
public:
    explicit StructureTemplateLibrary(std::filesystem::path root="assets/minecraft/structures");
    [[nodiscard]] const StructureTemplate& get(const std::string& path) const;
    void setRoot(std::filesystem::path root);
private:
    std::filesystem::path root_;
    mutable std::unordered_map<std::string,StructureTemplate> cache_;
};
}
