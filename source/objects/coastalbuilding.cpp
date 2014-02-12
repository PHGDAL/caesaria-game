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

#include "coastalbuilding.hpp"
#include "gfx/tile.hpp"
#include "game/resourcegroup.hpp"
#include "city/helper.hpp"
#include "gfx/tilemap.hpp"
#include "core/foreach.hpp"
#include "walker/fishing_boat.hpp"
#include "core/foreach.hpp"
#include "good/goodstore.hpp"
#include "constants.hpp"

using namespace constants;

class CoastalFactory::Impl
{
public:
  std::vector<int> saveTileInfo;
  Direction direction;

  bool isFlatCoast( const Tile& tile ) const
  {
    int imgId = tile.getOriginalImgId();
    return (imgId >= 372 && imgId <= 387);
  }

  Direction getDirection(PlayerCityPtr city, TilePos pos);
};

CoastalFactory::CoastalFactory(const Good::Type consume, const Good::Type produce,
                               const TileOverlay::Type type, Size size) : Factory(consume, produce, type, size),
  _d( new Impl )
{
}

bool CoastalFactory::canBuild( PlayerCityPtr city, TilePos pos, const TilesArray& aroundTiles ) const
{
  bool is_constructible = true;//Construction::canBuild( city, pos );

  Direction direction = _d->getDirection( city, pos );

  const_cast< CoastalFactory* >( this )->_setDirection( direction );

  return (is_constructible && direction != noneDirection );
}

void CoastalFactory::build(PlayerCityPtr city, const TilePos& pos)
{
  _setDirection( _d->getDirection( city, pos ) );

  TilesArray area = city->getTilemap().getArea( pos, getSize() );

  foreach( tile, area ) { _d->saveTileInfo.push_back( TileHelper::encode( *(*tile) ) ); }

  Factory::build( city, pos );
}

void CoastalFactory::destroy()
{
  CityHelper helper( _getCity() );

  TilesArray area = helper.getArea( this );

  int index=0;
  foreach( tile, area ) { TileHelper::decode( *(*tile), _d->saveTileInfo[ index++ ] ); }

  Factory::destroy();
}

void CoastalFactory::save(VariantMap& stream) const
{
  Factory::save( stream );

  stream[ "direction" ] = (int)_d->direction;
  stream[ "saved_tile"] = VariantList( _d->saveTileInfo );
}

void CoastalFactory::load(const VariantMap& stream)
{
  Factory::load( stream );

  _d->direction = (Direction)stream.get( "direction", (int)southWest ).toInt();
  _d->saveTileInfo << stream.get( "saved_tile" ).toList();
}

void CoastalFactory::assignBoat(ShipPtr)
{

}

const Tile& CoastalFactory::getLandingTile() const
{
  Tilemap& tmap = _getCity()->getTilemap();
  TilePos offset( -999, -999 );
  switch( _d->direction )
  {
  case south: offset = TilePos( 0, -1 ); break;
  case west: offset = TilePos( -1, 0 ); break;
  case north: offset = TilePos( 0, 2 ); break;
  case east: offset = TilePos( 2, 0 ); break;

  default: break;
  }

  return tmap.at( pos() + offset );
}

CoastalFactory::~CoastalFactory()
{

}

void CoastalFactory::_setDirection(Direction direction)
{
  _d->direction = direction;
  _updatePicture( direction );
}

Direction CoastalFactory::Impl::getDirection(PlayerCityPtr city, TilePos pos)
{
  Tilemap& tilemap = city->getTilemap();

  const Tile& t00 = tilemap.at( pos );
  const Tile& t10 = tilemap.at( pos + TilePos( 1, 0 ) );
  const Tile& t01 = tilemap.at( pos + TilePos( 0, 1 ) );
  const Tile& t11 = tilemap.at( pos + TilePos( 1, 1 ) );

  if( t00.getFlag( Tile::tlWater ) && isFlatCoast( t00 )
      && t10.getFlag( Tile::tlWater ) && isFlatCoast( t10 )
      && t01.getFlag( Tile::isConstructible ) && t11.getFlag( Tile::isConstructible ) )
  {
    return south;
  }

  if( t01.getFlag( Tile::tlWater ) && isFlatCoast( t01 )
      && t11.getFlag( Tile::tlWater ) && isFlatCoast( t11 )
      && t00.getFlag( Tile::isConstructible ) && t10.getFlag( Tile::isConstructible ) )
  {
    return north;
  }

  if( t00.getFlag( Tile::tlWater ) && isFlatCoast( t00 )
      && t01.getFlag( Tile::tlWater ) && isFlatCoast( t01 )
      && t10.getFlag( Tile::isConstructible ) && t11.getFlag( Tile::isConstructible ) )
  {
    return west;
  }

  if( t10.getFlag( Tile::tlWater ) && isFlatCoast( t10 )
      && t11.getFlag( Tile::tlWater ) && isFlatCoast( t11 )
      && t00.getFlag( Tile::isConstructible ) && t01.getFlag( Tile::isConstructible ) )
  {
    return east;
  }

  return noneDirection;
}
