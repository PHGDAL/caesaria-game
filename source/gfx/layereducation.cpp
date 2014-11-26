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
// Copyright 2012-2013 Dalerank, dalerankn8@gmail.com

#include "layereducation.hpp"
#include "objects/constants.hpp"
#include "game/resourcegroup.hpp"
#include "objects/house.hpp"
#include "objects/house_level.hpp"
#include "layerconstants.hpp"
#include "tilemap_camera.hpp"
#include "city/helper.hpp"
#include "core/event.hpp"
#include "core/gettext.hpp"

using namespace constants;

namespace gfx
{

int LayerEducation::type() const {  return _type; }

int LayerEducation::_getLevelValue( HousePtr house ) const
{
  switch(_type)
  {
  case citylayer::education:
  {
    float acc = 0;
    int level = house->spec().minEducationLevel();
    switch(level)
    {
    case 3: acc += house->getServiceValue( Service::academy );
    case 2: acc += house->getServiceValue( Service::library );
    case 1: acc += house->getServiceValue( Service::school );
      return static_cast<int>(acc / level);
    default: return 0;
    }
  }

  case citylayer::school: return (int) house->getServiceValue( Service::school );
  case citylayer::library: return (int) house->getServiceValue( Service::library );
  case citylayer::academy: return (int) house->getServiceValue( Service::academy );
  }

  return 0;
}

void LayerEducation::drawTile(Engine& engine, Tile& tile, const Point& offset)
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

    int educationLevel = -1;
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

    case building::school:
    case building::library:
    case building::academy:
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

        educationLevel = _getLevelValue( house );

        needDrawAnimations = (house->spec().level() == 1) && (house->habitants().empty());

        city::Helper helper( _city() );
        drawArea( engine, helper.getArea( overlay ), offset, ResourceGroup::foodOverlay, OverlayPic::inHouseBase );
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
    else if( educationLevel > 0 )
    {
      _addColumn( screenPos, educationLevel );
    }
  }

  tile.setWasDrawn();
}

LayerPtr LayerEducation::create( Camera& camera, PlayerCityPtr city, int type )
{
  LayerPtr ret( new LayerEducation( camera, city, type ) );
  ret->drop();

  return ret;
}

std::string LayerEducation::_getAccessLevel( int lvlValue ) const
{
  if( lvlValue == 0 ) { return "##no_"; }
  else if( lvlValue < 20 ) { return "##warning_"; }
  else if( lvlValue < 40 ) { return "##bad_"; }
  else if( lvlValue < 60 ) { return "##simple_"; }
  else if( lvlValue < 80 ) { return "##good_"; }
  else { return "##awesome_"; }
}

void LayerEducation::handleEvent(NEvent& event)
{
  if( event.EventType == sEventMouse )
  {
    switch( event.mouse.type  )
    {
    case mouseMoved:
    {
      Tile* tile = _camera()->at( event.mouse.pos(), false );  // tile under the cursor (or NULL)
      std::string text = "";
      std::string levelName = "";      
      if( tile != 0 )
      {
        HousePtr house = ptr_cast<House>( tile->overlay() );
        if( house != 0 )
        {
          std::string typeName;
          int lvlValue = _getLevelValue( house );
          switch( _type )
          {
          case citylayer::education:
          {
            bool schoolAccess = house->hasServiceAccess( Service::school );
            bool libraryAccess = house->hasServiceAccess( Service::library );
            bool academyAccess = house->hasServiceAccess( Service::academy );

            if( schoolAccess && libraryAccess && academyAccess )
            {
              text = "##education_full_access##";
            }
            else
            {
              if( schoolAccess && libraryAccess ) { text = "##education_have_school_library_access##"; }
              else if( schoolAccess || libraryAccess ) { text = "##education_have_school_or_library_access##"; }
              else if( academyAccess ) { text = "##education_have_academy_access##"; }
              else { text = "##education_have_no_access##"; }
            }
          }
          break;
          case citylayer::school: typeName = "school";  break;
          case citylayer::library: typeName = "library"; break;
          case citylayer::academy: typeName = "academy"; break;
          }       

          if( text.empty() )
          {
            levelName = _getAccessLevel( lvlValue );
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

LayerEducation::LayerEducation( Camera& camera, PlayerCityPtr city, int type)
  : LayerInfo( camera, city, 9 )
{
  _type = type;

  switch( type )
  {
  case citylayer::education:
  case citylayer::school: _flags.insert( building::school ); _addWalkerType( walker::scholar ); break;
  case citylayer::library: _flags.insert( building::library ); _addWalkerType( walker::librarian ); break;
  case citylayer::academy: _flags.insert( building::academy ); _addWalkerType( walker::teacher ); break;
  }
}

}//end namespace gfx
