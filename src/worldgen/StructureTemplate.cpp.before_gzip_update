#include "worldgen/StructureTemplate.h"

#include "content/ContentCatalog.h"
#include "worldgen/Vanilla112State.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <bit>
#include <iterator>
#include <span>
#include <unordered_map>
#include <array>
#include <fstream>
#include <stdexcept>

namespace mc112
{
namespace
{
class Reader
{
public:
    explicit Reader(const std::filesystem::path& file)
    {
        std::ifstream in(file,std::ios::binary);
        if(!in)throw std::runtime_error("Missing 1.12 structure template: "+file.string());
        data_=std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in),{});
    }
    std::uint8_t u8(){need(1);return data_[p_++];}
    std::int8_t i8(){return static_cast<std::int8_t>(u8());}
    std::int16_t i16(){need(2);const auto v=static_cast<std::uint16_t>(data_[p_])<<8|data_[p_+1];p_+=2;return static_cast<std::int16_t>(v);}
    std::int32_t i32(){need(4);std::uint32_t v=0;for(int i=0;i<4;++i)v=(v<<8)|data_[p_++];return static_cast<std::int32_t>(v);}
    std::int64_t i64(){need(8);std::uint64_t v=0;for(int i=0;i<8;++i)v=(v<<8)|data_[p_++];return static_cast<std::int64_t>(v);}
    float f32(){const std::uint32_t bits=static_cast<std::uint32_t>(i32());return std::bit_cast<float>(bits);}
    double f64(){const std::uint64_t bits=static_cast<std::uint64_t>(i64());return std::bit_cast<double>(bits);}
    std::string str(){const auto n=static_cast<std::uint16_t>(i16());need(n);std::string s(reinterpret_cast<const char*>(data_.data()+p_),n);p_+=n;return s;}
    void bytes(std::size_t n){need(n);p_+=n;}
private:
    std::vector<std::uint8_t>data_;std::size_t p_=0;
    void need(std::size_t n){if(p_+n>data_.size())throw std::runtime_error("Truncated structure NBT");}
};

struct Node
{
    std::int8_t byte=0;std::int32_t integer=0;std::string text;
    std::vector<Node> list;std::unordered_map<std::string,Node> compound;
};

Node payload(Reader&r,int type);
Node list(Reader&r)
{
    const int child=r.u8();const int count=r.i32();if(count<0)throw std::runtime_error("Invalid NBT list length");
    Node out;out.list.reserve(static_cast<std::size_t>(count));for(int i=0;i<count;++i)out.list.push_back(payload(r,child));return out;
}
Node compound(Reader&r)
{
    Node out;for(;;){const int type=r.u8();if(type==0)break;const std::string name=r.str();out.compound.emplace(name,payload(r,type));}return out;
}
Node payload(Reader&r,int type)
{
    Node n;switch(type)
    {
        case 1:n.byte=r.i8();break;case 2:r.i16();break;case 3:n.integer=r.i32();break;case 4:r.i64();break;
        case 5:r.f32();break;case 6:r.f64();break;case 7:{const int c=r.i32();if(c<0)throw std::runtime_error("Invalid NBT byte array");r.bytes(static_cast<std::size_t>(c));break;}
        case 8:n.text=r.str();break;case 9:n=list(r);break;case 10:n=compound(r);break;
        case 11:{const int c=r.i32();if(c<0)throw std::runtime_error("Invalid NBT int array");for(int i=0;i<c;++i)r.i32();break;}
        case 12:{const int c=r.i32();if(c<0)throw std::runtime_error("Invalid NBT long array");for(int i=0;i<c;++i)r.i64();break;}
        default:throw std::runtime_error("Unsupported NBT tag type");
    }return n;
}
const Node& req(const Node&n,const char*key){auto f=n.compound.find(key);if(f==n.compound.end())throw std::runtime_error(std::string("Structure NBT missing ")+key);return f->second;}
int listInt(const Node&n,std::size_t i){if(i>=n.list.size())throw std::runtime_error("Structure NBT short int list");return n.list[i].integer;}
TemplateNbt flatNbt(const Node&n)
{
    TemplateNbt out;for(const auto&[k,v]:n.compound){if(!v.text.empty())out.strings.emplace(k,v.text);else{out.ints.emplace(k,v.integer);out.bytes.emplace(k,v.byte);}}return out;
}
std::array<int,3> transform(int x,int y,int z,Rotation rot,Mirror mirror=Mirror::None)
{
    // Template::transformedBlockPos(..., mirror, rotation), 1.12.2.
    if(mirror==Mirror::LeftRight) z=-z;
    else if(mirror==Mirror::FrontBack) x=-x;
    switch(rot)
    {
        case Rotation::CounterClockwise90:return{z,y,-x};
        case Rotation::Clockwise90:return{-z,y,x};
        case Rotation::Clockwise180:return{-x,y,-z};
        case Rotation::None:return{x,y,z};
    }
    return{x,y,z};
}

mc::content::BlockState mirrorState(mc::content::BlockState value,Mirror mirror)
{
    if(mirror==Mirror::None) return value;
    const auto* active=mc::content::ContentCatalog::active();
    if(active==nullptr) return value;
    const auto* name=active->blockName(value);
    if(name==nullptr) return value;
    auto props=active->serializeStateProperties(value);
    for(auto& [key,val]:props)
    {
        if(key=="facing")
        {
            if(mirror==Mirror::LeftRight)
            {
                if(val=="north") val="south"; else if(val=="south") val="north";
            }
            else
            {
                if(val=="east") val="west"; else if(val=="west") val="east";
            }
        }
        else if(key=="rotation")
        {
            try
            {
                int r=std::stoi(val)&15;
                r=(mirror==Mirror::LeftRight)?((8-r)&15):((16-r)&15);
                val=std::to_string(r);
            }
            catch(...){}
        }
        else if(key=="shape")
        {
            // Stair mirrors swap handedness. Rail shapes encode compass
            // directions and are reflected explicitly below.
            if(val=="inner_left") val="inner_right";
            else if(val=="inner_right") val="inner_left";
            else if(val=="outer_left") val="outer_right";
            else if(val=="outer_right") val="outer_left";
            else if(mirror==Mirror::LeftRight)
            {
                if(val=="ascending_north") val="ascending_south";
                else if(val=="ascending_south") val="ascending_north";
                else if(val=="north_east") val="south_east";
                else if(val=="north_west") val="south_west";
                else if(val=="south_east") val="north_east";
                else if(val=="south_west") val="north_west";
            }
            else
            {
                if(val=="ascending_east") val="ascending_west";
                else if(val=="ascending_west") val="ascending_east";
                else if(val=="north_east") val="north_west";
                else if(val=="north_west") val="north_east";
                else if(val=="south_east") val="south_west";
                else if(val=="south_west") val="south_east";
            }
        }
    }
    // Multipart directional properties (vines, fences/walls in imported
    // palettes) carry directions as property keys.
    auto swapProp=[&](std::string_view a,std::string_view b)
    {
        auto ia=std::find_if(props.begin(),props.end(),[&](const auto&p){return p.first==a;});
        auto ib=std::find_if(props.begin(),props.end(),[&](const auto&p){return p.first==b;});
        if(ia!=props.end()&&ib!=props.end()) std::swap(ia->second,ib->second);
    };
    if(mirror==Mirror::LeftRight) swapProp("north","south"); else swapProp("east","west");
    const auto result=active->state(*name,std::span<const std::pair<std::string,std::string>>(props));
    return result.value_or(value);
}
}

std::string TemplateNbt::string(std::string_view key)const{auto f=strings.find(std::string(key));return f==strings.end()?std::string{}:f->second;}
int TemplateNbt::integer(std::string_view key,int fallback)const noexcept{auto f=ints.find(std::string(key));return f==ints.end()?fallback:f->second;}

StructureTemplate StructureTemplate::loadUncompressed(const std::filesystem::path&file)
{
    Reader r(file);const int rootType=r.u8();if(rootType!=10)throw std::runtime_error("Structure root must be a compound");static_cast<void>(r.str());const Node root=compound(r);
    StructureTemplate out;const Node&size=req(root,"size");out.sizeX=listInt(size,0);out.sizeY=listInt(size,1);out.sizeZ=listInt(size,2);
    const Node&pal=req(root,"palette");out.palette.reserve(pal.list.size());for(const Node&e:pal.list){TemplatePaletteEntry p;p.name=req(e,"Name").text;auto it=e.compound.find("Properties");if(it!=e.compound.end())for(const auto&[k,v]:it->second.compound)p.properties.emplace_back(k,v.text);std::sort(p.properties.begin(),p.properties.end());out.palette.push_back(std::move(p));}
    const Node&blocks=req(root,"blocks");out.blocks.reserve(blocks.list.size());for(const Node&e:blocks.list){TemplateBlock b;const Node&pos=req(e,"pos");b.x=listInt(pos,0);b.y=listInt(pos,1);b.z=listInt(pos,2);b.palette=static_cast<std::uint32_t>(req(e,"state").integer);auto it=e.compound.find("nbt");if(it!=e.compound.end())b.nbt=flatNbt(it->second);out.blocks.push_back(std::move(b));}
    return out;
}

std::array<int,3> StructureTemplate::transformedSize(Rotation rotation) const noexcept
{
    if(rotation==Rotation::Clockwise90||rotation==Rotation::CounterClockwise90)
        return{sizeZ,sizeY,sizeX};
    return{sizeX,sizeY,sizeZ};
}

std::array<int,3> StructureTemplate::transformedBlockPosition(
    int x,int y,int z,Rotation rotation,Mirror mirror) const noexcept
{
    return transform(x,y,z,rotation,mirror);
}

std::array<int,3> StructureTemplate::getZeroPositionWithTransform(
    int x,int y,int z,Rotation rotation,Mirror mirror) const noexcept
{
    // Template::func_191157_a with the template's actual dimensions.
    int i=sizeX-1,j=sizeZ-1;
    const bool frontBack=mirror==Mirror::FrontBack;
    const bool leftRight=mirror==Mirror::LeftRight;
    if(rotation==Rotation::CounterClockwise90)
        return{leftRight?x:x+j,y,frontBack?z+i:z};
    if(rotation==Rotation::Clockwise90)
        return{leftRight?x+i:x,y,frontBack?z:z+j};
    if(rotation==Rotation::Clockwise180)
        return{frontBack?x:x+i,y,leftRight?z:z+j};
    return{frontBack?x+i:x,y,leftRight?z+j:z};
}

Box StructureTemplate::transformedBox(int ox,int oy,int oz,Rotation rot,Mirror mirror)const noexcept
{
    const std::array<std::array<int,3>,4> corners{{
        transform(0,0,0,rot,mirror),
        transform(sizeX-1,0,0,rot,mirror),
        transform(0,0,sizeZ-1,rot,mirror),
        transform(sizeX-1,0,sizeZ-1,rot,mirror)}};
    int minX=corners[0][0],maxX=corners[0][0];
    int minZ=corners[0][2],maxZ=corners[0][2];
    for(const auto& c:corners)
    {
        minX=std::min(minX,c[0]);maxX=std::max(maxX,c[0]);
        minZ=std::min(minZ,c[2]);maxZ=std::max(maxZ,c[2]);
    }
    return{ox+minX,oy,oz+minZ,ox+maxX,oy+sizeY-1,oz+maxZ};
}

void StructureTemplate::place(WorldGenerationContext&c,int ox,int oy,int oz,Rotation rot,const Box&clip,float integrity,JavaRandom*random,bool ignoreStructureBlocks,const TemplateMarkerHandler& markerHandler,Mirror mirror)const
{
    const auto*catalog=mc::content::ContentCatalog::active();if(catalog==nullptr)throw std::logic_error("Content catalog inactive during structure placement");
    std::vector<mc::content::BlockState> resolved;resolved.reserve(palette.size());
    for(const auto&p:palette){resolved.push_back(rotateState(mirrorState(vanilla112State(p.name,std::span<const Property>(p.properties)),mirror),rot));}
    for(const TemplateBlock&b:blocks)
    {
        if(b.palette>=resolved.size())throw std::runtime_error("Structure palette index out of range");
        if(integrity<1.0f&&random!=nullptr&&random->nextFloat()>integrity)continue;
        const auto local=transform(b.x,b.y,b.z,rot,mirror);const int x=ox+local[0],y=oy+local[1],z=oz+local[2];if(!clip.contains(x,y,z))continue;
        const auto s=resolved[b.palette];
        if(named(s,"structure_block"))
        {
            if(markerHandler && b.nbt) markerHandler(x,y,z,*b.nbt,rot);
            if(ignoreStructureBlocks) continue;
        }
        if(named(s,"structure_void"))continue;
        c.setBlockState(x,y,z,s);
    }
}

StructureTemplateLibrary::StructureTemplateLibrary(std::filesystem::path root):root_(std::move(root)){}
void StructureTemplateLibrary::setRoot(std::filesystem::path root){root_=std::move(root);cache_.clear();}
const StructureTemplate&StructureTemplateLibrary::get(const std::string&path)const{auto it=cache_.find(path);if(it!=cache_.end())return it->second;const auto file=root_/(path+".nbt");auto [inserted,_]=cache_.emplace(path,StructureTemplate::loadUncompressed(file));return inserted->second;}
}
