#include "worldgen/EndCityStructure.h"
#include "worldgen/StructureTemplate.h"
#include "worldgen/WorldGenerationContext.h"
#include <algorithm>
#include <array>
#include <bit>
#include <string>
#include <string_view>

namespace mc112::EndCityStructure { namespace {
int floorDiv(int v,int d){int q=v/d,r=v%d;if(r&&((r<0)!=(d<0)))--q;return q;}
std::int64_t addWrap(std::int64_t a,std::int64_t b){return std::bit_cast<std::int64_t>(std::uint64_t(a)+std::uint64_t(b));}
std::int64_t mulWrap(std::int64_t a,std::int64_t b){return std::bit_cast<std::int64_t>(std::uint64_t(a)*std::uint64_t(b));}
Rotation addRot(Rotation a,Rotation b){return Rotation((int(a)+int(b))&3);}
std::array<int,2> rotXZ(int x,int z,Rotation r){switch(r){case Rotation::Clockwise90:return{-z,x};case Rotation::Clockwise180:return{-x,-z};case Rotation::CounterClockwise90:return{z,-x};default:return{x,z};}}

struct CityPiece final:Piece{
    std::string name; Rotation rotation=Rotation::None; bool overwrite=true; int ox=0,oy=0,oz=0;
    CityPiece(std::string n,int x,int y,int z,Rotation r,bool ow):name(std::move(n)),rotation(r),overwrite(ow),ox(x),oy(y),oz(z){
        const auto& t=templates().get("endcity/"+name); box=t.transformedBox(ox,oy,oz,rotation); componentType=0;
    }
    static StructureTemplateLibrary& templates(){static StructureTemplateLibrary lib;return lib;}
    void offset(int x,int y,int z) override{Piece::offset(x,y,z);ox+=x;oy+=y;oz+=z;}
    bool place(WorldGenerationContext& c,JavaRandom& r,const Box& clip) override{
        const auto& t=templates().get("endcity/"+name);
        t.place(c,ox,oy,oz,rotation,clip,1.0f,nullptr,true,[&](int x,int y,int z,const TemplateNbt& nbt,Rotation){
            const auto m=nbt.string("metadata");
            if(m.rfind("Chest",0)==0){const int cy=y-1;if(clip.contains(x,cy,z)){const auto seed=r.nextLong();c.assignStructureLoot(x,cy,z,"minecraft:chests/end_city_treasure",seed);}}
            else if(m.rfind("Sentry",0)==0){if(clip.contains(x,y,z))c.spawnStructureMob("minecraft:shulker",x,y,z);}
            else if(m.rfind("Elytra",0)==0){if(clip.contains(x,y,z))c.spawnStructureMob("minecraft:item_frame_elytra",x,y,z);}
        }); return true;
    }
};

CityPiece* add(std::vector<std::unique_ptr<Piece>>& ps,std::unique_ptr<CityPiece> p){auto* q=p.get();ps.push_back(std::move(p));return q;}
CityPiece* child(std::vector<std::unique_ptr<Piece>>& ps,CityPiece& parent,int x,int y,int z,std::string name,Rotation r,bool overwrite){
    auto d=rotXZ(x,z,parent.rotation);return add(ps,std::make_unique<CityPiece>(std::move(name),parent.ox+d[0],parent.oy+y,parent.oz+d[1],r,overwrite));
}

enum class Gen{House,Tower,Bridge,Fat};
struct Build{ bool ship=false; };

bool recurse(Gen gen,int depth,CityPiece& parent,std::vector<std::unique_ptr<Piece>>& all,JavaRandom& rnd,Build& b);

bool generatedAccept(std::vector<std::unique_ptr<Piece>>& all,std::vector<std::unique_ptr<Piece>>& tmp,CityPiece& parent,JavaRandom& rnd){
    const int id=rnd.nextInt();
    for(auto& p:tmp){p->componentType=id; if(auto* hit=findIntersecting(all,p->box);hit && hit->componentType!=parent.componentType)return false;}
    for(auto& p:tmp)all.push_back(std::move(p)); return true;
}

bool genHouse(int depth,CityPiece& p,std::vector<std::unique_ptr<Piece>>& out,JavaRandom& r,Build& b){
    if(depth>8)return false; auto* q=child(out,p,-1,4,-1,"base_floor",p.rotation,true); int i=r.nextInt(3);
    if(i==0){child(out,*q,-1,4,-1,"base_roof",p.rotation,true);}
    else if(i==1){q=child(out,*q,-1,0,-1,"second_floor_2",p.rotation,false);q=child(out,*q,-1,8,-1,"second_roof",p.rotation,false);recurse(Gen::Tower,depth+1,*q,out,r,b);}
    else{q=child(out,*q,-1,0,-1,"second_floor_2",p.rotation,false);q=child(out,*q,-1,4,-1,"third_floor_c",p.rotation,false);q=child(out,*q,-1,8,-1,"third_roof",p.rotation,true);recurse(Gen::Tower,depth+1,*q,out,r,b);} return true;
}

bool genTower(int depth,CityPiece& p,std::vector<std::unique_ptr<Piece>>& out,JavaRandom& r,Build& b){
    auto* q=child(out,p,3+r.nextInt(2),-3,3+r.nextInt(2),"tower_base",p.rotation,true);q=child(out,*q,0,7,0,"tower_piece",p.rotation,true);CityPiece* bridge=r.nextInt(3)==0?q:nullptr;int n=1+r.nextInt(3);for(int j=0;j<n;++j){q=child(out,*q,0,4,0,"tower_piece",p.rotation,true);if(j<n-1&&r.nextBoolean())bridge=q;}
    if(bridge){constexpr std::array<std::array<int,4>,4> br{{{{0,1,-1,0}},{{1,6,-1,1}},{{3,0,-1,5}},{{2,5,-1,6}}}};for(auto a:br)if(r.nextBoolean()){auto* e=child(out,*bridge,a[1],a[2],a[3],"bridge_end",addRot(p.rotation,Rotation(a[0])),true);recurse(Gen::Bridge,depth+1,*e,out,r,b);}child(out,*q,-1,4,-1,"tower_top",p.rotation,true);}
    else if(depth!=7){return recurse(Gen::Fat,depth+1,*q,out,r,b);}else child(out,*q,-1,4,-1,"tower_top",p.rotation,true);return true;
}

bool genBridge(int depth,CityPiece& p,std::vector<std::unique_ptr<Piece>>& out,JavaRandom& r,Build& b){
    int n=r.nextInt(4)+1;auto* q=child(out,p,0,0,-4,"bridge_piece",p.rotation,true);q->componentType=-1;int dy=0;for(int k=0;k<n;++k){if(r.nextBoolean()){q=child(out,*q,0,dy,-4,"bridge_piece",p.rotation,true);dy=0;}else{if(r.nextBoolean())q=child(out,*q,0,dy,-4,"bridge_steep_stairs",p.rotation,true);else q=child(out,*q,0,dy,-8,"bridge_gentle_stairs",p.rotation,true);dy=4;}}
    if(!b.ship && r.nextInt(10-depth)==0){child(out,*q,-8+r.nextInt(8),dy,-70+r.nextInt(10),"ship",p.rotation,true);b.ship=true;}
    else if(!recurse(Gen::House,depth+1,*q,out,r,b))return false;
    q=child(out,*q,4,dy,0,"bridge_end",addRot(p.rotation,Rotation::Clockwise180),true);q->componentType=-1;return true;
}

bool genFat(int depth,CityPiece& p,std::vector<std::unique_ptr<Piece>>& out,JavaRandom& r,Build& b){
    auto* q=child(out,p,-3,4,-3,"fat_tower_base",p.rotation,true);q=child(out,*q,0,4,0,"fat_tower_middle",p.rotation,true);
    constexpr std::array<std::array<int,4>,4> br{{{{0,4,-1,0}},{{1,12,-1,4}},{{3,0,-1,8}},{{2,8,-1,12}}}};
    for(int i=0;i<2&&r.nextInt(3)!=0;++i){q=child(out,*q,0,8,0,"fat_tower_middle",p.rotation,true);for(auto a:br)if(r.nextBoolean()){auto* e=child(out,*q,a[1],a[2],a[3],"bridge_end",addRot(p.rotation,Rotation(a[0])),true);recurse(Gen::Bridge,depth+1,*e,out,r,b);}}child(out,*q,-2,8,-2,"fat_tower_top",p.rotation,true);return true;
}

bool recurse(Gen gen,int depth,CityPiece& parent,std::vector<std::unique_ptr<Piece>>& all,JavaRandom& rnd,Build& b){if(depth>8)return false;std::vector<std::unique_ptr<Piece>> tmp;bool ok=false;switch(gen){case Gen::House:ok=genHouse(depth,parent,tmp,rnd,b);break;case Gen::Tower:ok=genTower(depth,parent,tmp,rnd,b);break;case Gen::Bridge:ok=genBridge(depth,parent,tmp,rnd,b);break;case Gen::Fat:ok=genFat(depth,parent,tmp,rnd,b);break;}return ok&&generatedAccept(all,tmp,parent,rnd);}

Rotation startRotation(int cx,int cz){JavaRandom r(std::int64_t(cx)+std::int64_t(cz)*10387313LL);return Rotation(r.nextInt(4));}
int yFor(int cx,int cz,Rotation rot,const HeightSampler& h){int dx=5,dz=5;if(rot==Rotation::Clockwise90)dx=-5;else if(rot==Rotation::Clockwise180){dx=-5;dz=-5;}else if(rot==Rotation::CounterClockwise90)dz=-5;const int ox=cx*16,oz=cz*16;return std::min({h(ox+7,oz+7),h(ox+7,oz+7+dz),h(ox+7+dx,oz+7),h(ox+7+dx,oz+7+dz)});}
}

bool isCandidate(std::int64_t seed,int cx,int cz,const IslandPredicate& island){int ix=cx,iz=cz;if(cx<0)cx-=19;if(cz<0)cz-=19;int rx=floorDiv(cx,20),rz=floorDiv(cz,20);std::int64_t s=addWrap(addWrap(mulWrap(rx,341873128712LL),mulWrap(rz,132897987541LL)),seed);s=addWrap(s,10387313LL);JavaRandom r(s);int x=rx*20+(r.nextInt(9)+r.nextInt(9))/2,z=rz*20+(r.nextInt(9)+r.nextInt(9))/2;return ix==x&&iz==z&&(!island||island(ix,iz));}
Start create(std::int64_t seed,int cx,int cz,JavaRandom& pop,const HeightSampler& h){Start s;s.chunkX=cx;s.chunkZ=cz;const Rotation rot=startRotation(cx,cz);const int y=yFor(cx,cz,rot,h);if(y<60)return s;auto* p=add(s.pieces,std::make_unique<CityPiece>("base_floor",cx*16+8,y,cz*16+8,rot,true));p=child(s.pieces,*p,-1,0,-1,"second_floor",rot,false);p=child(s.pieces,*p,-1,4,-1,"third_floor",rot,false);p=child(s.pieces,*p,-1,8,-1,"third_roof",rot,true);Build b;recurse(Gen::Tower,1,*p,s.pieces,pop,b);s.bounds=boundsOf(s.pieces);s.sizeable=true;return s;}
bool Start::place(WorldGenerationContext& c,JavaRandom& r,const Box& clip)const{if(!sizeable)return false;bool any=false;for(auto& p:pieces)if(p->box.intersects(clip)){p->place(c,r,clip);any=true;}return any;}
}
