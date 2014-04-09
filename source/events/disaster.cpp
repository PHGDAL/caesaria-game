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

#include "disaster.hpp"
#include "game/game.hpp"
#include "gfx/tilemap.hpp"
#include "city/city.hpp"
#include "playsound.hpp"
#include "objects/objects_factory.hpp"
#include "dispatcher.hpp"
#include "core/gettext.hpp"
#include "objects/house_level.hpp"
#include "objects/house.hpp"
#include "objects/ruins.hpp"
#include "core/stringhelper.hpp"
#include "gfx/tilesarray.hpp"
#include "build.hpp"
#include "core/foreach.hpp"

using namespace constants;
using namespace gfx;

namespace events
{

GameEventPtr DisasterEvent::create( const Tile& tile, Type type )
{
  DisasterEvent* event = new DisasterEvent();
  event->_pos = tile.pos();
  event->_type = type;
  event->_infoType = 0;

  TileOverlayPtr overlay = tile.overlay();
  if( overlay.isValid() )
  {
    overlay->deleteLater();
    HousePtr house = ptr_cast< House >( overlay );
    if( house.isValid() )
    {
      event->_infoType = 1000 + house->getSpec().level();
    }
    else
    {
      event->_infoType = overlay->type();
    }
  }

  GameEventPtr ret( event );
  ret->drop();

  return ret;
}

void DisasterEvent::_exec( Game& game, unsigned int )
{
  Tilemap& tmap = game.city()->tilemap();
  Tile& tile = tmap.at( _pos );
  TilePos rPos = _pos;

  if( tile.getFlag( Tile::isDestructible ) )
  {
    Size size( 1 );

    TileOverlayPtr overlay = tile.overlay();
    if( overlay.isValid() )
    {
      overlay->deleteLater();
      rPos = overlay->pos();
      size = overlay->size();
    }

    switch( _type )
    {
    case DisasterEvent::collapse:
    {
      GameEventPtr e = PlaySound::create( "explode", rand() % 2, 100 );
      e->dispatch();
    }
    break;

    default:
    break;
    }

    TilesArray clearedTiles = tmap.getArea( rPos, size );
    foreach( tile, clearedTiles )
    {
      TileOverlay::Type dstr2constr[] = { building::burningRuins, building::collapsedRuins, building::plagueRuins };
      TileOverlayPtr ov = TileOverlayFactory::getInstance().create( dstr2constr[_type] );
      if( ov.isValid() )
      {
        SmartPtr< Ruins > ruins = ptr_cast< Ruins >( ov );
        if( ruins.isValid() )
        {
          std::string typev = _infoType > 1000
                                ? StringHelper::format( 0xff, "house%02d", _infoType - 1000 )
                                : MetaDataHolder::getTypename( _infoType );
          ruins->setInfo( StringHelper::format( 0xff, "##ruins_%04d_text##", typev.c_str() ) );
        }

        Dispatcher::instance().append( BuildEvent::create( (*tile)->pos(), ov ) );
      }
    }

    std::string dstr2string[] = { _("##alarm_fire_in_city##"), _("##alarm_building_collapsed##"),
                                  _("##alarm_plague_in_city##") };
    game.city()->onDisasterEvent().emit( _pos, dstr2string[_type] );
  }
}

bool DisasterEvent::_mayExec(Game&, unsigned int) const{  return true;}

DisasterEvent::DisasterEvent() : _type( count ),_infoType( 0 )
{}

} //end namespace events
