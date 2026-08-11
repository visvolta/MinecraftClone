#include "game/GameBootstrap.h"
#include "worldgen/StructureTemplate.h"
#include "worldgen/StructurePrimitives.h"
#include "worldgen/Vanilla112State.h"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <vector>

int main()
{
    try
    {
        mc::game::GameBootstrap bootstrap("assets");
        bootstrap.loadContentModules();
        bootstrap.freezeRegistries();

        std::vector<std::filesystem::path> files;
        for(const auto& e:std::filesystem::recursive_directory_iterator("assets/minecraft/structures"))
            if(e.is_regular_file()&&e.path().extension()==".nbt") files.push_back(e.path());
        std::sort(files.begin(),files.end());

        std::size_t blocks=0,palette=0,markers=0;
        for(const auto& file:files)
        {
            const auto t=mc112::StructureTemplate::load(file);
            if(t.sizeX<=0||t.sizeY<=0||t.sizeZ<=0)
                throw std::runtime_error("Invalid template size: "+file.string());
            std::vector<mc::content::BlockState> states;
            for(const auto& p:t.palette)
            {
                auto state=mc112::tryVanilla112State(p.name,p.properties);
                if(!state) throw std::runtime_error("Unresolved state: "+file.string()+" :: "+p.name);
                states.push_back(*state); ++palette;
            }
            for(const auto& b:t.blocks)
            {
                ++blocks;
                if(b.palette>=states.size()) throw std::runtime_error("Palette index OOB: "+file.string());
                if(b.x<0||b.y<0||b.z<0||b.x>=t.sizeX||b.y>=t.sizeY||b.z>=t.sizeZ)
                    throw std::runtime_error("Block outside template bounds: "+file.string());
                if(mc112::named(states[b.palette],"structure_block")&&b.nbt) ++markers;
            }

            for(auto mirror:{mc112::Mirror::None,mc112::Mirror::LeftRight,mc112::Mirror::FrontBack})
            for(auto rotation:{mc112::Rotation::None,mc112::Rotation::Clockwise90,
                               mc112::Rotation::Clockwise180,mc112::Rotation::CounterClockwise90})
            {
                const auto box=t.transformedBox(31,70,-19,rotation,mirror);
                for(const auto& b:t.blocks)
                {
                    const auto p=t.transformedBlockPosition(b.x,b.y,b.z,rotation,mirror);
                    if(!box.contains(31+p[0],70+p[1],-19+p[2]))
                        throw std::runtime_error("Transform escaped bounding box: "+file.string());
                }
            }

            for(auto state:states)
            {
                auto r=state;
                for(int i=0;i<4;++i) r=mc112::rotateState(r,mc112::Rotation::Clockwise90);
                if(r!=state) throw std::runtime_error("State rotation is not cyclic: "+file.string());
            }
        }
        std::cout<<"Templates: "<<files.size()<<"\nPalette entries: "<<palette
                 <<"\nBlocks: "<<blocks<<"\nData markers: "<<markers
                 <<"\nPASS: structure palettes, bounds and rotations are consistent.\n";
        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr<<"Structure fidelity failure: "<<e.what()<<"\n";
        return 1;
    }
}
