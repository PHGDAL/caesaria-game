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
//
// Copyright 2012-2014 Dalerank, dalerankn8@gmail.com

#include "build_options.hpp"
#include "objects/metadata.hpp"
#include "core/foreach.hpp"
#include "objects/constants.hpp"
#include <map>

using namespace constants;
using namespace gfx;

namespace city
{

static const char* disable_all = "disable_all";

struct BuildingRule
{
  TileOverlay::Type type;
  bool mayBuild;
  unsigned int quotes;
};

class BuildOptions::Impl
{
public:
  typedef std::map< TileOverlay::Type, BuildingRule > BuildingRules;
  typedef std::vector< TilePos > MemPoints;

  BuildingRules rules;
  MemPoints memPoints;

  bool checkDesirability;
  unsigned int maximumForts;
};

BuildOptions::BuildOptions() : _d( new Impl )
{
  _d->checkDesirability = true;
  _d->maximumForts = 999;
  _d->memPoints.resize( 10 );
}

BuildOptions::~BuildOptions() {}

void BuildOptions::setBuildingAvailble( const TileOverlay::Type type, bool mayBuild )
{
  _d->rules[ type ].mayBuild = mayBuild;
}

void BuildOptions::setBuildingAvailble( const TileOverlay::Type start, const TileOverlay::Type stop, bool mayBuild )
{
  for( int i=start; i <= stop; i++ )
    _d->rules[ (TileOverlay::Type)i ].mayBuild = mayBuild;
}

bool BuildOptions::isBuildingsAvailble( const TileOverlay::Type start, const TileOverlay::Type stop ) const
{
  bool mayBuild = false;
  for( int i=start; i <= stop; i++ )
    mayBuild |= _d->rules[ (TileOverlay::Type)i ].mayBuild;

  return mayBuild;
}

bool BuildOptions::isCheckDesirability() const {  return _d->checkDesirability; }
unsigned int BuildOptions::getMaximumForts() const { return _d->maximumForts; }

void BuildOptions::setGroupAvailable( const BuildMenuType type, Variant vmb )
{
  if( vmb.isNull() )
    return;

  bool mayBuild = (vmb.toString() != disable_all);
  switch( type )
  {
  case BM_FARM: setBuildingAvailble( building::wheatFarm, building::pigFarm, mayBuild ); break;
  case BM_WATER: setBuildingAvailble( building::reservoir, building::well, mayBuild ); break;
  case BM_HEALTH: setBuildingAvailble( building::doctor, building::barber, mayBuild ); break;
  case BM_RAW_MATERIAL: setBuildingAvailble( building::marbleQuarry, building::clayPit, mayBuild ); break;
  case BM_RELIGION: setBuildingAvailble( building::templeCeres, building::oracle, mayBuild ); break;
  case BM_FACTORY: setBuildingAvailble( building::winery, building::pottery, mayBuild ); break;
  case BM_EDUCATION: setBuildingAvailble( building::school, building::library, mayBuild ); break;
  case BM_ENTERTAINMENT: setBuildingAvailble( building::amphitheater, building::chariotSchool, mayBuild ); break;
  case BM_ADMINISTRATION: setBuildingAvailble( building::senate, building::governorPalace, mayBuild ); break;
  case BM_ENGINEERING:
    setBuildingAvailble( building::engineerPost, building::wharf, mayBuild );
    setBuildingAvailble( construction::plaza, mayBuild );
    setBuildingAvailble( construction::garden, mayBuild );
  break;
  case BM_SECURITY: setBuildingAvailble( building::prefecture, building::fortArea, mayBuild ); break;
  case BM_COMMERCE: setBuildingAvailble( building::market, building::warehouse, mayBuild ); break;
  case BM_TEMPLE: setBuildingAvailble( building::templeCeres, building::templeVenus, mayBuild ); break;
  case BM_BIGTEMPLE: setBuildingAvailble( building::cathedralCeres, building::cathedralVenus, mayBuild ); break;
  case BM_MAX: setBuildingAvailble( construction::unknown, building::typeCount, mayBuild );

  default:
  break;
  }
}

bool BuildOptions::isGroupAvailable(const BuildMenuType type) const
{
  switch( type )
  {
  case BM_FARM:         return isBuildingsAvailble( building::wheatFarm, building::pigFarm ); break;
  case BM_WATER:        return isBuildingsAvailble( building::reservoir, building::well ); break;
  case BM_HEALTH:       return isBuildingsAvailble( building::doctor, building::barber ); break;
  case BM_RAW_MATERIAL: return isBuildingsAvailble( building::marbleQuarry, building::clayPit ); break;
  case BM_RELIGION:     return isBuildingsAvailble( building::templeCeres, building::oracle ); break;
  case BM_FACTORY:      return isBuildingsAvailble( building::winery, building::pottery ); break;
  case BM_EDUCATION:    return isBuildingsAvailble( building::school, building::library ); break;
  case BM_ENTERTAINMENT: return isBuildingsAvailble( building::amphitheater, building::chariotSchool ); break;
  case BM_ADMINISTRATION: return isBuildingsAvailble( building::senate, building::governorPalace ); break;
  case BM_ENGINEERING:  return isBuildingsAvailble( building::engineerPost, building::wharf ); break;
  case BM_SECURITY:     return isBuildingsAvailble( building::prefecture, building::fortArea ); break;
  case BM_COMMERCE:     return isBuildingsAvailble( building::market, building::warehouse ); break;
  case BM_TEMPLE:       return isBuildingsAvailble( building::templeCeres, building::templeVenus ); break;
  case BM_BIGTEMPLE:    return isBuildingsAvailble( building::cathedralCeres, building::cathedralVenus ); break;
  default:
  break;
  }

  return false;
}

unsigned int BuildOptions::getBuildingsQuote(const TileOverlay::Type type) const
{
  Impl::BuildingRules::const_iterator it = _d->rules.find( type );
  return it != _d->rules.end() ? it->second.quotes : 999;
}

TilePos BuildOptions::memPoint(unsigned int index) const
{
  index = math::clamp<unsigned int>( index, 0, _d->memPoints.size()-1 );
  return _d->memPoints[ index ];
}

void BuildOptions::setMemPoint(unsigned int index, TilePos point)
{
  index = math::clamp<unsigned int>( index, 0, _d->memPoints.size()-1 );
  _d->memPoints[ index ] = point;
}

void BuildOptions::clear() {  _d->rules.clear(); }

void BuildOptions::load(const VariantMap& options)
{
  setGroupAvailable( BM_FARM, options.get( "farm" ) );
  setGroupAvailable( BM_RAW_MATERIAL, options.get( "raw_material" ) );
  setGroupAvailable( BM_FACTORY, options.get( "factory" ) );
  setGroupAvailable( BM_WATER, options.get( "water" ) );
  setGroupAvailable( BM_HEALTH, options.get( "health" ) );
  setGroupAvailable( BM_RELIGION, options.get( "religion" ) );
  setGroupAvailable( BM_EDUCATION, options.get( "education" ) );
  setGroupAvailable( BM_ENTERTAINMENT, options.get( "entertainment" ) );
  setGroupAvailable( BM_ADMINISTRATION, options.get( "govt" ) );
  setGroupAvailable( BM_ENGINEERING, options.get( "engineering" ) );
  setGroupAvailable( BM_SECURITY, options.get( "security" ) );
  setGroupAvailable( BM_COMMERCE, options.get( "commerce" ) );

  VariantMap buildings = options.get( "buildings" ).toMap();
  foreach( item, buildings )
  {
    TileOverlay::Type btype = MetaDataHolder::findType( item->first );
    setBuildingAvailble( btype, item->second.toBool() );
  }

  VariantList points = options.get("points").toList();
  unsigned int index=0;
  foreach( it, points )
  {
    setMemPoint( index, it->toTilePos() );
    index++;
  }

  _d->checkDesirability = options.get( "check_desirability", _d->checkDesirability );
  _d->maximumForts = options.get( "maximumForts", _d->maximumForts );
}

VariantMap BuildOptions::save() const
{
  VariantMap blds;
  VariantMap quotes;
  foreach( it, _d->rules )
  {
    std::string typeName = MetaDataHolder::findTypename( it->first );
    blds[ typeName ] = it->second.mayBuild;
    quotes[ typeName ] = it->second.quotes;
  }

  VariantList points;
  foreach( it, _d->memPoints )
  {
    points.push_back( *it );
  }

  VariantMap ret;
  ret[ "buildings" ] = blds;
  ret[ "quotes" ] = quotes;
  ret[ "maximumForts" ] = _d->maximumForts;
  ret[ "check_desirability" ] = _d->checkDesirability;
  ret[ "points" ] = points;
  return ret;
}

BuildOptions& BuildOptions::operator=(const city::BuildOptions& a)
{
  _d->rules = a._d->rules;
  _d->checkDesirability = a._d->checkDesirability;
  _d->maximumForts = a._d->maximumForts;
  _d->memPoints = a._d->memPoints;

  return *this;
}

bool BuildOptions::isBuildingAvailble(const TileOverlay::Type type ) const
{
  Impl::BuildingRules::iterator it = _d->rules.find( type );
  return (it != _d->rules.end() ? (*it).second.mayBuild : true);
}

}//end namespace city
