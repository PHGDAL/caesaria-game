// This file is part of CaesarIA.
//
// CaesarIA is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// CaesarIA is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CaesarIA.  If not, see <http://www.gnu.org/licenses/>.

#include "objects_factory.hpp"
#include "service.hpp"
#include "training.hpp"
#include "watersupply.hpp"
#include "warehouse.hpp"
#include "ruins.hpp"
#include "engineer_post.hpp"
#include "factory.hpp"
#include "house.hpp"
#include "senate.hpp"
#include "prefecture.hpp"
#include "entertainment.hpp"
#include "religion.hpp"
#include "aqueduct.hpp"
#include "barracks.hpp"
#include "road.hpp"
#include "market.hpp"
#include "granary.hpp"
#include "well.hpp"
#include "native.hpp"
#include "farm.hpp"
#include "pottery.hpp"
#include "low_bridge.hpp"
#include "high_bridge.hpp"
#include "clay_pit.hpp"
#include "marble_quarry.hpp"
#include "goverment.hpp"
#include "military.hpp"
#include "military_academy.hpp"
#include "forum.hpp"
#include "garden.hpp"
#include "health.hpp"
#include "metadata.hpp"
#include "education.hpp"
#include "amphitheater.hpp"
#include "walker/fish_place.hpp"
#include "wharf.hpp"
#include "constants.hpp"
#include "constants.hpp"
#include "core/logger.hpp"
#include "wall.hpp"
#include "fortification.hpp"
#include "gatehouse.hpp"
#include "shipyard.hpp"
#include "tower.hpp"
#include "elevation.hpp"
#include "dock.hpp"
#include <map>

using namespace constants;

template< class T > class BaseCreator : public TileOverlayConstructor
{
public:
  virtual TileOverlayPtr create()
  {
    TileOverlayPtr ret( new T() );
    ret->drop();

    return ret;
  }
};

template< class T > class ConstructionCreator : public BaseCreator<T>
{
public: 
  virtual TileOverlayPtr create()
  {
    TileOverlayPtr ret = BaseCreator<T>::create();

    const MetaData& info = MetaDataHolder::instance().getData( ret->getType() );
    init( ret, info );

    return ret;
  }

  virtual void init( TileOverlayPtr a, const MetaData& info )
  {
    ConstructionPtr construction = a.as<Construction>();
    if( construction == 0 )
      return;

    if( info.getBasePicture().isValid() )
    {
      construction->setPicture( info.getBasePicture() );  // default picture for build tool
    }

    VariantMap anMap = info.getOption( "animation" ).toMap();
    if( !anMap.empty() )
    {
      Animation anim;

      anim.load( anMap.get( "rc" ).toString(), anMap.get( "start" ).toInt(),
                 anMap.get( "count" ).toInt(), anMap.get( "reverse", false ).toBool(),
                 anMap.get( "step", 1 ).toInt() );

      Variant v_offset = anMap.get( "offset" );
      if( v_offset.isValid() )
      {
        anim.setOffset( v_offset.toPoint() );
      }

      anim.setDelay( anMap.get( "delay", 1 ).toInt() );

      construction->setAnimation( anim );
    }
  }
};

template< class T > class WorkingBuildingCreator : public ConstructionCreator<T>
{
public:
  void init( TileOverlayPtr a, const MetaData& info )
  {
    ConstructionCreator<T>::init( a, info );

    WorkingBuildingPtr wb = a.as<WorkingBuilding>();
    if( wb != 0 )
    {
      wb->setMaxWorkers( (int)info.getOption( "employers" ) );
    }
  }
};

template< class T > class FactoryCreator : public WorkingBuildingCreator<T>
{
public:
  void init( TileOverlayPtr a, const MetaData& info )
  {
    WorkingBuildingCreator<T>::init( a, info );

    FactoryPtr f = a.as<Factory>();
    if( f.isValid() )
    {
      f->setProductRate( (float)info.getOption( "productRate", 9.6 ) );
    }
  }
};

class TileOverlayFactory::Impl
{
public:
  typedef std::map< TileOverlay::Type, TileOverlayConstructor* > Constructors;
  std::map< std::string, TileOverlay::Type > name2typeMap;
  Constructors constructors;
};

TileOverlayPtr TileOverlayFactory::create(const TileOverlay::Type type) const
{
  Impl::Constructors::iterator findConstructor = _d->constructors.find( type );

  if( findConstructor != _d->constructors.end() )
  {
    return findConstructor->second->create();
  }

  return TileOverlayPtr();
}

TileOverlayPtr TileOverlayFactory::create( const std::string& typeName ) const
{
  return TileOverlayPtr();
}

TileOverlayFactory& TileOverlayFactory::getInstance()
{
  static TileOverlayFactory inst;
  return inst;
}

TileOverlayFactory::TileOverlayFactory() : _d( new Impl )
{
#define ADD_CREATOR(type,classObject,creator) addCreator(type,CAESARIA_STR_EXT(classObject), new creator<classObject>() )
  // entertainment
  ADD_CREATOR(building::theater,      Theater,      WorkingBuildingCreator );
  ADD_CREATOR(building::amphitheater, Amphitheater, WorkingBuildingCreator );
  ADD_CREATOR(building::colloseum,    Collosseum,   WorkingBuildingCreator );
  ADD_CREATOR(building::actorColony,  ActorColony,  WorkingBuildingCreator );
  ADD_CREATOR(building::gladiatorSchool, GladiatorSchool, WorkingBuildingCreator );
  ADD_CREATOR(building::lionsNursery, LionsNursery, WorkingBuildingCreator );
  ADD_CREATOR(building::chariotSchool,WorkshopChariot, WorkingBuildingCreator );
  ADD_CREATOR(building::hippodrome,   Hippodrome, WorkingBuildingCreator );

  // road&house
  ADD_CREATOR(building::house,        House, ConstructionCreator );
  ADD_CREATOR(construction::road,     Road, ConstructionCreator );

  // administration
  addCreator(building::forum,        CAESARIA_STR_EXT(Forum) , new WorkingBuildingCreator<Forum>() );
  addCreator(building::senate,       CAESARIA_STR_EXT(Senate), new WorkingBuildingCreator<Senate>() );
  addCreator(building::governorHouse,CAESARIA_STR_EXT(GovernorsHouse) , new ConstructionCreator<GovernorsHouse>() );
  addCreator(building::governorVilla,CAESARIA_STR_EXT(GovernorsVilla) , new ConstructionCreator<GovernorsVilla>() );
  addCreator(building::governorPalace, CAESARIA_STR_EXT(GovernorsPalace), new ConstructionCreator<GovernorsPalace>() );
  addCreator(building::smallStatue,    CAESARIA_STR_EXT(SmallStatue), new ConstructionCreator<SmallStatue>() );
  addCreator(building::middleStatue,    CAESARIA_STR_EXT(MediumStatue), new ConstructionCreator<MediumStatue>() );
  addCreator(building::bigStatue,    CAESARIA_STR_EXT(BigStatue), new ConstructionCreator<BigStatue>() );
  addCreator(construction::garden, CAESARIA_STR_EXT(Garden) , new ConstructionCreator<Garden>() );
  addCreator(construction::plaza,  CAESARIA_STR_EXT(Plaza)  , new ConstructionCreator<Plaza>() );

  // water
  addCreator(building::well,       CAESARIA_STR_EXT(Well)     , new WorkingBuildingCreator<Well>() );
  addCreator(building::fountain,   CAESARIA_STR_EXT(Fountain) , new WorkingBuildingCreator<Fountain>() );
  addCreator(building::aqueduct,   CAESARIA_STR_EXT(Aqueduct), new ConstructionCreator<Aqueduct>() );
  addCreator(building::reservoir,  CAESARIA_STR_EXT(Reservoir), new ConstructionCreator<Reservoir>() );

  // security
  addCreator(building::prefecture,   CAESARIA_STR_EXT(Prefecture)  , new WorkingBuildingCreator<Prefecture>() );
  addCreator(building::fortLegionaire, CAESARIA_STR_EXT(FortLegionnaire), new WorkingBuildingCreator<FortLegionnaire>() );
  addCreator(building::fortJavelin, CAESARIA_STR_EXT(FortJaveline)   , new WorkingBuildingCreator<FortJaveline>() );
  addCreator(building::fortMounted, CAESARIA_STR_EXT(FortMounted)  , new WorkingBuildingCreator<FortMounted>() );
  addCreator(building::militaryAcademy, CAESARIA_STR_EXT(MilitaryAcademy), new WorkingBuildingCreator<MilitaryAcademy>() );
  addCreator(building::barracks,   CAESARIA_STR_EXT(Barracks)        , new WorkingBuildingCreator<Barracks>() );
  ADD_CREATOR( building::wall,          Wall, ConstructionCreator );
  ADD_CREATOR( building::fortification, Fortification, ConstructionCreator );
  ADD_CREATOR( building::gatehouse, Gatehouse, ConstructionCreator );
  ADD_CREATOR( building::tower,     Tower, WorkingBuildingCreator );


  // commerce
  addCreator(building::market,     CAESARIA_STR_EXT(Market)  , new WorkingBuildingCreator<Market>() );
  addCreator(building::warehouse,  CAESARIA_STR_EXT(Warehouse), new WorkingBuildingCreator<Warehouse>() );
  addCreator(building::granary,      CAESARIA_STR_EXT(Granary)  , new WorkingBuildingCreator<Granary>() );

  // farms
  addCreator(building::wheatFarm,    CAESARIA_STR_EXT(FarmWheat) , new FactoryCreator<FarmWheat>() );
  addCreator(building::oliveFarm, CAESARIA_STR_EXT(FarmOlive) , new FactoryCreator<FarmOlive>() );
  addCreator(building::grapeFarm,    CAESARIA_STR_EXT(FarmGrape) , new FactoryCreator<FarmGrape>() );
  addCreator(building::pigFarm,   CAESARIA_STR_EXT(FarmMeat)     , new FactoryCreator<FarmMeat>() );
  addCreator(building::fruitFarm, CAESARIA_STR_EXT(FarmFruit)    , new FactoryCreator<FarmFruit>() );
  addCreator(building::vegetableFarm, CAESARIA_STR_EXT(FarmVegetable), new FactoryCreator<FarmVegetable>() );

  // raw materials
  addCreator(building::ironMine,     CAESARIA_STR_EXT(IronMine)  , new FactoryCreator<IronMine>() );
  addCreator(building::timberLogger, CAESARIA_STR_EXT(TimberLogger), new FactoryCreator<TimberLogger>() );
  addCreator(building::clayPit,      CAESARIA_STR_EXT(ClayPit)  , new FactoryCreator<ClayPit>() );
  addCreator(building::marbleQuarry, CAESARIA_STR_EXT(MarbleQuarry), new FactoryCreator<MarbleQuarry>() );

  // factories
  ADD_CREATOR(building::weaponsWorkshop, WeaponsWorkshop, FactoryCreator );
  ADD_CREATOR(building::furnitureWorkshop,  FurnitureWorkshop, FactoryCreator );
  ADD_CREATOR(building::winery, Winery, FactoryCreator );
  ADD_CREATOR(building::creamery, Creamery, FactoryCreator );
  ADD_CREATOR(building::pottery,  Pottery, FactoryCreator );

  // utility
  addCreator(building::engineerPost, CAESARIA_STR_EXT(EngineerPost), new WorkingBuildingCreator<EngineerPost>() );
  addCreator(building::lowBridge,    CAESARIA_STR_EXT(LowBridge), new ConstructionCreator<LowBridge>() );
  ADD_CREATOR(building::highBridge,   HighBridge, ConstructionCreator );
  ADD_CREATOR(building::dock,       Dock    , WorkingBuildingCreator );
  ADD_CREATOR(building::shipyard,   Shipyard, FactoryCreator );
  ADD_CREATOR(building::wharf,      Wharf   , FactoryCreator );
  ADD_CREATOR(building::triumphalArch, TriumphalArch, ConstructionCreator );

  // religion
  addCreator(building::templeCeres,  CAESARIA_STR_EXT(TempleCeres)  , new WorkingBuildingCreator<TempleCeres>() );
  addCreator(building::templeNeptune, CAESARIA_STR_EXT(TempleNeptune), new WorkingBuildingCreator<TempleNeptune>() );
  addCreator(building::templeMars,CAESARIA_STR_EXT(TempleMars)   , new WorkingBuildingCreator<TempleMars>() );
  addCreator(building::templeVenus, CAESARIA_STR_EXT(TempleVenus)  , new WorkingBuildingCreator<TempleVenus>() );
  addCreator(building::templeMercury, CAESARIA_STR_EXT(TempleMercure), new WorkingBuildingCreator<TempleMercure>() );
  addCreator(building::cathedralCeres, CAESARIA_STR_EXT(BigTempleCeres)  , new WorkingBuildingCreator<BigTempleCeres>() );
  addCreator(building::cathedralNeptune, CAESARIA_STR_EXT(BigTempleNeptune), new WorkingBuildingCreator<BigTempleNeptune>() );
  addCreator(building::cathedralMars, CAESARIA_STR_EXT(BigTempleMars)   , new WorkingBuildingCreator<BigTempleMars>() );
  addCreator(building::cathedralVenus, CAESARIA_STR_EXT(BigTempleVenus)  , new WorkingBuildingCreator<BigTempleVenus>() );
  addCreator(building::cathedralMercury, CAESARIA_STR_EXT(BigTempleMercure), new WorkingBuildingCreator<BigTempleMercure>() );
  addCreator(building::oracle, CAESARIA_STR_EXT(TempleOracle) , new WorkingBuildingCreator<TempleOracle>() );

  // health
  addCreator(building::baths,      CAESARIA_STR_EXT(Baths)   , new WorkingBuildingCreator<Baths>() );
  addCreator(building::barber,     CAESARIA_STR_EXT(Barber)  , new WorkingBuildingCreator<Barber>() );
  addCreator(building::doctor,     CAESARIA_STR_EXT(Doctor)  , new WorkingBuildingCreator<Doctor>() );
  addCreator(building::hospital,   CAESARIA_STR_EXT(Hospital), new WorkingBuildingCreator<Hospital>() );

  // education
  addCreator(building::school,     CAESARIA_STR_EXT(School) , new WorkingBuildingCreator<School>() );
  addCreator(building::library,    CAESARIA_STR_EXT(Library), new WorkingBuildingCreator<Library>() );
  addCreator(building::academy,    CAESARIA_STR_EXT(Academy), new WorkingBuildingCreator<Academy>() );
  addCreator(building::missionaryPost, CAESARIA_STR_EXT(MissionaryPost), new ConstructionCreator<MissionaryPost>() );

  // natives
  addCreator(building::nativeHut, CAESARIA_STR_EXT(NativeHut)   , new ConstructionCreator<NativeHut>() );
  addCreator(building::nativeCenter, CAESARIA_STR_EXT(NativeCenter), new ConstructionCreator<NativeCenter>() );
  addCreator(building::nativeField, CAESARIA_STR_EXT(NativeField) , new ConstructionCreator<NativeField>() );

  //damages
  ADD_CREATOR(building::burningRuins, BurningRuins, ConstructionCreator );
  ADD_CREATOR(building::burnedRuins, BurnedRuins, ConstructionCreator );
  ADD_CREATOR(building::collapsedRuins, CollapsedRuins, ConstructionCreator );
  ADD_CREATOR(building::plagueRuins, PlagueRuins, ConstructionCreator);

  //places
  ADD_CREATOR( building::elevation, Elevation, BaseCreator );
}

void TileOverlayFactory::addCreator( const TileOverlay::Type type, const std::string& typeName, TileOverlayConstructor* ctor )
{
  bool alreadyHaveConstructor = _d->name2typeMap.find( typeName ) != _d->name2typeMap.end();

  if( !alreadyHaveConstructor )
  {
    _d->name2typeMap[ typeName ] = type;
    _d->constructors[ type ] = ctor;
  }
  else
  {
    Logger::warning( "TileOverlayFactory already have constructor for %s", typeName.c_str() );
  }
}

bool TileOverlayFactory::canCreate( const TileOverlay::Type type ) const
{
  return _d->constructors.find( type ) != _d->constructors.end();   
}