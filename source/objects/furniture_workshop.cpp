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

#include "furniture_workshop.hpp"

#include "city/helper.hpp"
#include "core/gettext.hpp"
#include "game/resourcegroup.hpp"
#include "timber_logger.hpp"

using namespace constants;
using namespace gfx;

bool FurnitureWorkshop::canBuild(PlayerCityPtr city, TilePos pos, const TilesArray& aroundTiles) const
{
  return Factory::canBuild( city, pos, aroundTiles );
}

bool FurnitureWorkshop::build(PlayerCityPtr city, const TilePos& pos)
{
  Factory::build( city, pos );

  city::Helper helper( city );
  bool haveTimberLogger = !helper.find<TimberLogger>( building::timberLogger ).empty();

  _setError( haveTimberLogger ? "" : _("##need_timber_for_work##") );

  return true;
}

FurnitureWorkshop::FurnitureWorkshop() : Factory(Good::timber, Good::furniture, building::furnitureWorkshop, Size(2) )
{
  setPicture( ResourceGroup::commerce, 117 );
  _fgPicturesRef().resize( 3 );
}

void FurnitureWorkshop::_storeChanged()
{
  _fgPicturesRef()[1] = inStockRef().empty() ? Picture() : Picture::load( ResourceGroup::commerce, 155 );
  _fgPicturesRef()[1].setOffset( 47, 0 );
}
