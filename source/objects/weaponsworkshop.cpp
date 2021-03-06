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
// Copyright 2012-2013 Gregoire Athanase, gathanase@gmail.com
// Copyright 2012-2014 Dalerank, dalerankn8@gmail.com

#include "weaponsworkshop.hpp"
#include "constants.hpp"
#include "gfx/picture.hpp"
#include "game/resourcegroup.hpp"
#include "city/helper.hpp"

using namespace constants;
using namespace gfx;

WeaponsWorkshop::WeaponsWorkshop()
  : Factory(Good::iron, Good::weapon, building::weaponsWorkshop, Size(2) )
{
  setPicture( ResourceGroup::commerce, 108);

  _animationRef().load( ResourceGroup::commerce, 109, 6);
  _fgPicturesRef().resize(2);
}

bool WeaponsWorkshop::canBuild( PlayerCityPtr city, TilePos pos, const TilesArray& aroundTiles ) const
{
  return Factory::canBuild( city, pos, aroundTiles );
}

bool WeaponsWorkshop::build(PlayerCityPtr city, const TilePos& pos)
{
  Factory::build( city, pos );

  city::Helper helper( city );
  bool haveIronMine = !helper.find<Building>( building::ironMine ).empty();

  _setError( haveIronMine ? "" : "##need_iron_for_work##" );

  return true;
}

void WeaponsWorkshop::_storeChanged()
{
  _fgPicturesRef()[1] = inStockRef().empty() ? Picture() : Picture::load( ResourceGroup::commerce, 156 );
  _fgPicturesRef()[1].setOffset( 20, 15 );
}
