#include "game/GameBootstrap.h"
#include "worldgen/StructureTemplate.h"
#include "worldgen/Vanilla112State.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
std::string describe(
    const std::string& name,
    const std::vector<mc112::Property>& properties)
{
    std::ostringstream out;
    out << name;
    if (!properties.empty())
    {
        out << '[';
        for (std::size_t i = 0; i < properties.size(); ++i)
        {
            if (i != 0) out << ',';
            out << properties[i].first << '=' << properties[i].second;
        }
        out << ']';
    }
    return out.str();
}

bool expectState(
    std::string_view name,
    std::initializer_list<mc112::Property> properties)
{
    const std::vector<mc112::Property> props(properties);
    if (mc112::tryVanilla112State(name, props))
        return true;

    std::cerr << "Regression mapping failed: "
              << describe(std::string(name), props) << '\n';
    return false;
}
}

int main()
{
    try
    {
        mc::game::GameBootstrap bootstrap("assets");
        bootstrap.loadContentModules();
        bootstrap.freezeRegistries();

        bool ok = true;

        for (std::string_view half : {"lower", "upper"})
        {
            ok = expectState("minecraft:double_plant",
                {{"variant","sunflower"},{"half",std::string(half)}}) && ok;
            ok = expectState("minecraft:double_plant",
                {{"variant","syringa"},{"half",std::string(half)}}) && ok;
            ok = expectState("minecraft:double_plant",
                {{"variant","double_grass"},{"half",std::string(half)}}) && ok;
            ok = expectState("minecraft:double_plant",
                {{"variant","double_fern"},{"half",std::string(half)}}) && ok;
            ok = expectState("minecraft:double_plant",
                {{"variant","double_rose"},{"half",std::string(half)}}) && ok;
            ok = expectState("minecraft:double_plant",
                {{"variant","paeonia"},{"half",std::string(half)}}) && ok;
        }

        ok = expectState(
            "minecraft:red_flower", {{"type","houstonia"}}) && ok;
        ok = expectState(
            "minecraft:tallgrass", {{"type","tall_grass"}}) && ok;
        ok = expectState(
            "minecraft:tallgrass", {{"type","fern"}}) && ok;
        ok = expectState(
            "minecraft:tallgrass", {{"type","dead_bush"}}) && ok;

        const std::filesystem::path structureRoot =
            "assets/minecraft/structures";

        if (!std::filesystem::is_directory(structureRoot))
        {
            std::cerr << "Missing structure asset directory: "
                      << structureRoot.string() << '\n';
            return 2;
        }

        std::vector<std::filesystem::path> files;
        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(structureRoot))
        {
            if (entry.is_regular_file() &&
                entry.path().extension() == ".nbt")
                files.push_back(entry.path());
        }
        std::sort(files.begin(), files.end());

        if (files.empty())
        {
            std::cerr << "No structure NBT files found under "
                      << structureRoot.string() << '\n';
            return 2;
        }

        std::set<std::string> unresolved;
        std::size_t paletteEntries = 0;

        for (const auto& file : files)
        {
            mc112::StructureTemplate structure;
            try
            {
                structure = mc112::StructureTemplate::load(file);
            }
            catch (const std::exception& error)
            {
                unresolved.insert(
                    file.generic_string() +
                    " :: TEMPLATE_LOAD_ERROR :: " + error.what());
                continue;
            }

            for (const auto& entry : structure.palette)
            {
                ++paletteEntries;
                if (!mc112::tryVanilla112State(entry.name, entry.properties))
                {
                    unresolved.insert(
                        file.generic_string() + " :: " +
                        describe(entry.name, entry.properties));
                }
            }
        }

        std::cout << "Structure templates scanned: " << files.size() << '\n';
        std::cout << "Palette entries checked: " << paletteEntries << '\n';

        if (!unresolved.empty())
        {
            std::cerr << "\nUnresolved Minecraft 1.12.2 structure states ("
                      << unresolved.size() << "):\n";
            for (const std::string& item : unresolved)
                std::cerr << "  " << item << '\n';

            std::cerr
                << "\nFix every entry above in Vanilla112State/resource "
                   "registration; do not silently discard stored properties.\n";
            return 1;
        }

        if (!ok)
            return 1;

        std::cout
            << "PASS: every installed Minecraft 1.12.2 structure palette "
               "state resolves exactly.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Structure palette audit failed: "
                  << error.what() << '\n';
        return 2;
    }
}
