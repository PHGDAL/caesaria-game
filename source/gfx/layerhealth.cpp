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

#include "layerhealth.hpp"
#include "objects/constants.hpp"
#include "game/resourcegroup.hpp"
#include "objects/house.hpp"
#include "objects/house_level.hpp"
#include "layerconstants.hpp"
#include "tilemap_camera.hpp"
#include "city/helper.hpp"
#include "core/gettext.hpp"
#include "core/event.hpp"

using namespace constants;

namespace gfx
{

int LayerHealth::type() const {  return _type; }

int LayerHealth::_getLevelValue( HousePtr house )
{
  switch(_type)
  {
  case citylayer::health: return (int) house->state( House::health );
  case citylayer::hospital: return (int) house->getServiceValue( Service::hospital );
  case citylayer::barber: return (int) house->getServiceValue( Service::barber );
  case citylayer::baths: return (int) house->getServiceValue( Service::baths );
  }

  return 0;
}

void LayerHealth::drawTile(Engine& engine, Tile& tile, const Point& offset)
{
  Point screenPos = tile.mappos() + offset;

  if( tile.overlay().isNull() )
  {
    //draw background
    engine.draw( tile.picture(), screenPos );
  }
  else
  {
    bool needDrawAnimations = false;
    TileOverlayPtr overlay = tile.overlay();

    int healthLevel = -1;
    switch( overlay->type() )
    {
    // Base set of visible objects
    case construction::road:
    case construction::plaza:
    case construction::garden:

    case building::burnedRuins:
    case building::collapsedRuins:

    case building::lowBridge:
    case building::highBridge:

    case building::elevation:
    case building::rift:
      needDrawAnimations = true;
    break;

    case building::doctor:
    case building::hospital:
    case building::barber:
    case building::baths:
      needDrawAnimations = _flags.count( overlay->type() ) > 0;
      if( !needDrawAnimations )
      {
        city::Helper helper( _city() );
        drawArea( engine, helper.getArea( overlay ), offset, ResourceGroup::foodOverlay, OverlayPic::base );
      }
    break;

      //houses
    case building::house:
      {
        HousePtr house = ptr_cast<House>( overlay );

        healthLevel = _getLevelValue( house );

        needDrawAnimations = (house->spec().level() == 1) && (house->habitants().empty());

        if( !needDrawAnimations )
        {
          city::Helper helper( _city() );
          drawArea( engine, helper.getArea( overlay ), offset, ResourceGroup::foodOverlay, OverlayPic::inHouseBase );
        }
      }
    break;

      //other buildings
    default:
      {
        city::Helper helper( _city() );
        drawArea( engine, helper.getArea( overlay ), offset, ResourceGroup::foodOverlay, OverlayPic::base );
      }
    break;
    }

    if( needDrawAnimations )
    {
      Layer::drawTile( engine, tile, offset );
      registerTileForRendering( tile );
    }
    else if( healthLevel > 0 )
    {
      _addColumn( screenPos, healthLevel );
      //drawColumn( engine, screenPos, healthLevel );
    }
  }

  tile.setWasDrawn();
}

LayerPtr LayerHealth::create(TilemapCamera& camera, PlayerCityPtr city, int type )
{
  LayerPtr ret( new LayerHealth( camera, city, type ) );
  ret->drop();

  return ret;
}

void LayerHealth::handleEvent(NEvent& event)
{
  if( event.EventType == sEventMouse )
  {
    switch( event.mouse.type  )
    {
    case mouseMoved:
    {
      Tile* tile = _camera()->at( event.mouse.pos(), false );  // tile under the cursor (or NULL)
      std::string text = "";
      if( tile != 0 )
      {
        HousePtr house = ptr_cast<House>( tile->overlay() );
        if( house != 0 )
        {
          std::string typeName;
          switch( _type )
          {
          case citylayer::health: typeName = "health"; break;
          case citylayer::doctor: typeName = "doctor"; break;
          case citylayer::hospital: typeName = "hospital"; break;
          case citylayer::barber: typeName = "barber"; break;
          case citylayer::baths: typeName = "baths"; break;
          }

          int lvlValue = _getLevelValue( house );
          std::string levelName;
          if( lvlValue > 0 )
          {
            if( lvlValue < 20 ) { levelName = "##warning_"; }
            else if( lvlValue < 40 ) { levelName = "##bad_"; }
            else if( lvlValue < 60 ) { levelName = "##simple_"; }
            else if( lvlValue < 80 ) { levelName = "##good_"; }
            else { levelName = "##awesome_"; }

            text = levelName + typeName + "_access##";
          }
        }
      }

      _setTooltipText( _(text) );
    }
    break;

    default: break;
    }
  }

  Layer::handleEvent( event );
}

LayerHealth::LayerHealth(Camera& camera, PlayerCityPtr city, int type)
  : LayerInfo( camera, city, 9 )
{
  _type = type;

  switch( type )
  {
  case citylayer::health:
  case citylayer::doctor:
    _flags.insert( building::doctor );
    _addWalkerType( walker::doctor );
  break;

  case citylayer::hospital:
    _flags.insert( building::hospital );
    _addWalkerType( walker::surgeon );
  break;

  case citylayer::barber:
    _flags.insert( building::barber );
    _addWalkerType( walker::barber );
  break;

  case citylayer::baths:
    _flags.insert( building::baths );
    _addWalkerType( walker::bathlady );
  break;
  }
}

}//end namespace gfx
