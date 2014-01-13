// This file is part of openCaesar3.
//
// openCaesar3 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// openCaesar3 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with openCaesar3.  If not, see <http://www.gnu.org/licenses/>.

#include "native.hpp"
#include "game/resourcegroup.hpp"
#include "gfx/tile.hpp"
#include "city/city.hpp"
#include "constants.hpp"

using namespace constants;

NativeBuilding::NativeBuilding(const Type type, const Size& size )
: Building( type, size )
{
}

void NativeBuilding::save( VariantMap& stream) const 
{
  Building::save(stream);
}

void NativeBuilding::load( const VariantMap& stream) {Building::load(stream);}

void NativeBuilding::build(PlayerCityPtr city, const TilePos& pos )
{
  Building::build( city, pos );
  getTile().setFlag( Tile::tlRock, true );
  getTile().setFlag( Tile::tlBuilding, false );
}

NativeHut::NativeHut() : NativeBuilding( building::nativeHut, Size(1) )
{
  setPicture( ResourceGroup::housing, 49 );
  //setPicture(PicLoader::instance().get_picture("housng1a", 50));
}

void NativeHut::save( VariantMap& stream) const 
{
  Building::save(stream);
}

void NativeHut::load( const VariantMap& stream) {Building::load(stream);}

NativeCenter::NativeCenter() : NativeBuilding( building::nativeCenter, Size(2) )
{
  setPicture( ResourceGroup::housing, 51 );
}

void NativeCenter::save( VariantMap&stream) const 
{
  Building::save(stream);
}

void NativeCenter::load( const VariantMap& stream) {Building::load(stream);}

NativeField::NativeField() : NativeBuilding( building::nativeField, Size(1) )
{
  setPicture( ResourceGroup::commerce, 13 );
}

void NativeField::save( VariantMap&stream) const 
{
  Building::save(stream);
}

void NativeField::load( const VariantMap& stream) {Building::load(stream);}