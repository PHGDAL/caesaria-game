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

#include "layerbuild.hpp"
#include "objects/aqueduct.hpp"
#include "layerconstants.hpp"
#include "walker/constants.hpp"
#include "game/roadbuild_helper.hpp"
#include "core/logger.hpp"
#include "events/build.hpp"
#include "core/foreach.hpp"
#include "city/city.hpp"
#include "core/event.hpp"
#include "sdl_engine.hpp"
#include "core/font.hpp"
#include "objects/fortification.hpp"
#include "core/stringhelper.hpp"
#include "camera.hpp"
#include "gui/dialogbox.hpp"
#include "renderermode.hpp"
#include "events/warningmessage.hpp"
#include "city/funds.hpp"
#include "walker/walker.hpp"
#include "city_renderer.hpp"

using namespace constants;
using namespace gui;

namespace gfx
{

namespace {
static const int frameCountLimiter=25;
CAESARIA_LITERALCONST(oc3_land)
}

class LayerBuild::Impl
{
public:
  bool multiBuilding;
  TilePos lastTilePos;
  TilePos startTilePos;
  bool kbShift, kbCtrl;
  bool borderBuilding;
  bool roadAssignment;
  bool drawTileBasicPicture;
  int drawLayerIndex;
  int frameCount;
  int money4Construction;
  Renderer* renderer;
  LayerPtr lastLayer;
  Font textFont;
  PictureRef textPic;
  TilesArray buildTiles;  // these tiles have draw over "normal" tilemap tiles!
};

void LayerBuild::_discardPreview()
{
  __D_IMPL(d,LayerBuild)
  foreach( tile, d->buildTiles )
  {
    if( (*tile)->overlay().isValid() )
    {
      (*tile)->overlay()->deleteLater();
    }

    delete *tile;
  }

  d->buildTiles.clear();
}

void LayerBuild::_checkPreviewBuild(TilePos pos)
{
  __D_IMPL(d,LayerBuild);
  BuildModePtr bldCommand = ptr_cast<BuildMode>( d->renderer->mode() );

  if (bldCommand.isNull())
    return;

  // TODO: do only when needed, when (i, j, _buildInstance) has changed
  ConstructionPtr overlay = bldCommand->getContruction();

  if( !overlay.isValid() )
  {
    return;
  }

  Size size = overlay->size();
  int cost = MetaDataHolder::getData( overlay->type() ).getOption( MetaDataOptions::cost );

  bool walkersOnTile = false;
  if( bldCommand->isCheckWalkers() )
  {
    TilesArray tiles = _city()->tilemap().getArea( pos, pos + TilePos( size.width()-1, size.height()-1 )  );
    foreach( t, tiles )
    {
      const WalkerList& walkers = _city()->walkers( (*t)->pos() );

      if( !walkers.empty() )
      {
        walkersOnTile = true;
        break;
      }
    }
  }

  if( !walkersOnTile && overlay->canBuild( _city(), pos, d->buildTiles ) )
  {
    //bldCommand->setCanBuild(true);
    Tilemap& tmap = _city()->tilemap();
    Tile *masterTile=0;
    d->money4Construction += cost;
    for (int dj = 0; dj < size.height(); ++dj)
    {
      for (int di = 0; di < size.width(); ++di)
      {
        Tile& basicTile =  tmap.at( pos + TilePos( di, dj ) );
        Tile* tile = new Tile( basicTile.pos() );  // make a copy of tile
        tile->setEPos( basicTile.epos() );

        if (di==0 && dj==0)
        {
          // this is the masterTile
          masterTile = tile;
        }
        tile->setPicture( tmap.at( pos + TilePos( di, dj ) ).picture() );
        tile->setMasterTile( masterTile );
        tile->setOverlay( ptr_cast<TileOverlay>( overlay ) );
        //tile->setFlag( Tile::tlRock, true );  //dirty hack that drawing this tile
        d->buildTiles.push_back( tile );
      }
    }
  }
  else
  {
    //bldCommand->setCanBuild(false);

    const Picture& grnPicture = Picture::load(lc_oc3_land, 1);
    const Picture& redPicture = Picture::load(lc_oc3_land, 2);

    //TilemapArea area = til
    Tilemap& tmap = _city()->tilemap();
    for (int dj = 0; dj < size.height(); ++dj)
    {
      for (int di = 0; di < size.width(); ++di)
      {
        TilePos rPos = pos + TilePos( di, dj );
        if( !tmap.isInside( rPos ) )
          continue;

        const Tile& basicTile = tmap.at( rPos );
        const bool isConstructible = basicTile.getFlag( Tile::isConstructible );
        Tile* tile = new Tile( basicTile.pos() );  // make a copy of tile
        tile->setEPos( basicTile.epos() );

        walkersOnTile = false;
        if( bldCommand->isCheckWalkers() )
        {
          walkersOnTile = !_city()->walkers( rPos ).empty();
        }

        tile->setPicture( (!walkersOnTile && isConstructible) ? grnPicture : redPicture );
        tile->setMasterTile( 0 );
        tile->setFlag( Tile::clearAll, true );
        tile->setOverlay( 0 );
        d->buildTiles.push_back( tile );
      }
    }
  }
}

void LayerBuild::_updatePreviewTiles( bool force )
{
  __D_IMPL(d,LayerBuild);
  Tile* curTile = _camera()->at( _lastCursorPos(), true );

  if( !curTile )
    return;

  if( !force && d->lastTilePos == curTile->epos() )
    return;

  if( !d->multiBuilding )
  {
    _setStartCursorPos( _lastCursorPos() );
    d->startTilePos = curTile->pos();
  }

  d->lastTilePos = curTile->epos();

  _discardPreview();
  d->money4Construction = 0;

  if( d->borderBuilding )
  {
    Tile* startTile = _camera()->at( d->startTilePos );  // tile under the cursor (or NULL)
    Tile* stopTile  = _camera()->at( _lastCursorPos(),  true );

    TilesArray pathWay = RoadPropagator::createPath( _city()->tilemap(),
                                                     startTile->epos(), stopTile->epos(),
                                                     d->roadAssignment, d->kbShift );
    Tilemap& tmap = _city()->tilemap();
    TilePos leftUpCorner = pathWay.leftUpCorner();
    TilePos rigthDownCorner = pathWay.rightDownCorner();
    TilePos leftDownCorner( leftUpCorner.i(), rigthDownCorner.j() );
    TilesArray ret;

    int mmapSize = std::max<int>( leftUpCorner.j() - rigthDownCorner.j() + 1,
                                  rigthDownCorner.i() - leftUpCorner.i() + 1 );
    for( int y=0; y < mmapSize; y++ )
    {
      for( int t=0; t <= y; t++ )
      {
        TilePos tpos = leftDownCorner + TilePos( t, mmapSize - 1 - ( y - t ) );
        if( pathWay.contain( tpos ) )
          ret.push_back( &tmap.at( tpos ) );
      }
    }

    for( int x=1; x < mmapSize; x++ )
    {
      for( int t=0; t < mmapSize-x; t++ )
      {
        TilePos tpos = leftDownCorner + TilePos( x + t, t );
        if( pathWay.contain( tpos ) )
          ret.push_back( &tmap.at( tpos ) );
      }
    }

    pathWay = ret;
    foreach( it, pathWay )
    {
      _checkPreviewBuild( (*it)->epos() );
    }
  }
  else
  {
    TilesArray tiles = _getSelectedArea( d->startTilePos );

    foreach( it, tiles ) { _checkPreviewBuild( (*it)->epos() ); }
  }  

  d->textPic->fill( 0x0, Rect() );
  d->textFont.setColor( 0xffff0000 );
  d->textFont.draw( *d->textPic, StringHelper::i2str( d->money4Construction ) + " Dn", Point() );
}

void LayerBuild::_buildAll()
{
  __D_IMPL(d,LayerBuild);
  BuildModePtr bldCommand = ptr_cast<BuildMode>( d->renderer->mode() );
  if( bldCommand.isNull() )
    return;

  ConstructionPtr cnstr = bldCommand->getContruction();

  if( !cnstr.isValid() )
  {
    Logger::warning( "LayerBuild: No construction for build" );
    return;
  }

  if( !_city()->funds().haveMoneyForAction( 1 ) )
  {
    events::GameEventPtr e = events::WarningMessageEvent::create( "##out_of_credit##" );
    e->dispatch();
    return;
  }

  bool buildOk = false;  
  foreach( it, d->buildTiles )
  {
    Tile* tile = *it;
    if( cnstr->canBuild( _city(), tile->epos(), TilesArray() ) && tile->isMasterTile())
    {
      events::GameEventPtr event = events::BuildEvent::create( tile->epos(), cnstr->type() );
      event->dispatch();
      buildOk = true;
    }
  }

  d->startTilePos = d->lastTilePos;

  if( !buildOk )
  {
    std::string errorStr = cnstr->errorDesc();

    events::GameEventPtr event = events::WarningMessageEvent::create( errorStr.empty()
                                                                      ? "##need_build_on_cleared_area##"
                                                                      : errorStr );
    event->dispatch();
  }
}

void LayerBuild::handleEvent(NEvent& event)
{
  __D_IMPL(_d,LayerBuild);
  _d->kbShift = false;
  _d->kbCtrl = false;
  if( event.EventType == sEventMouse )
  {
    _d->kbShift = event.mouse.shift;
    _d->kbCtrl = event.mouse.control;

    switch( event.mouse.type  )
    {
    case mouseMoved:
    {
      _setLastCursorPos( event.mouse.pos() );
      if( !event.mouse.isLeftPressed() || _startCursorPos().x() < 0 )
      {
        _setStartCursorPos( _lastCursorPos() );

        Tile* tile = _camera()->at( _lastCursorPos(), true );
        _d->startTilePos = tile ? tile->epos() : TilePos( -1, -1 );
      }

      _updatePreviewTiles( false );
    }
    break;

    case mouseLbtnPressed:
    {
      _updatePreviewTiles( false );
    }
    break;

    case mouseLbtnRelease:            // left button
    {
      Tile* tile = _camera()->at( event.mouse.pos(), false );  // tile under the cursor (or NULL)
      if( tile == 0 )
      {
        break;
      }

#ifndef CAESARIA_PLATFORM_ANDROID
      _finishBuild();
#endif
    }
    break;

    case mouseRbtnRelease:
    {
      int nextLayer = _d->lastLayer.isValid() ? _d->lastLayer->type() : citylayer::simple;
      _setNextLayer( nextLayer );
      _discardPreview();
    }
    break;

    default:
    break;
    }
  }

  if( event.EventType == sEventKeyboard )
  {
    bool pressed = event.keyboard.pressed;
    int moveValue = _camera()->scrollSpeed() * ( event.keyboard.shift ? 4 : 1 ) * (pressed ? 1 : 0);

    switch( event.keyboard.key )
    {
    case KEY_UP:    _camera()->moveUp   ( moveValue ); break;
    case KEY_DOWN:  _camera()->moveDown ( moveValue ); break;
    case KEY_RIGHT: _camera()->moveRight( moveValue ); break;
    case KEY_LEFT:  _camera()->moveLeft ( moveValue ); break;
    case KEY_ESCAPE: _setNextLayer( citylayer::simple ); _discardPreview(); break;
    case KEY_RETURN:
    {
      if( !event.keyboard.pressed )  //button was left up
      {
        _finishBuild();
      }
    }
    break;
    default: break;
    }
  }
}

void LayerBuild::_finishBuild()
{
  _buildAll();
  _setStartCursorPos( _lastCursorPos() );
  _updatePreviewTiles( true );
}

int LayerBuild::type() const {  return citylayer::build;}

void LayerBuild::_drawBuildTiles( Engine& engine)
{
  __D_IMPL(_d,LayerBuild);
  Point offset = _camera()->offset();
  foreach( it, _d->buildTiles )
  {
    Tile* postTile = *it;
    postTile->resetWasDrawn();

    if( postTile->masterTile() )
      postTile = postTile->masterTile();

    ConstructionPtr ptr_construction = ptr_cast<Construction>( postTile->overlay() );
    engine.resetColorMask();

    if( ptr_construction.isValid()
        && ptr_construction->canBuild( _city(), postTile->epos(), _d->buildTiles ) )
    {
      engine.setColorMask( 0x00000000, 0x0000ff00, 0, 0xff000000 );
    }

    drawProminentTile( engine, *postTile, offset, postTile->epos().z(), true );
  }

  engine.resetColorMask();
}

void LayerBuild::_handeLayerSwitch(int layer)
{
  __D_IMPL(_d,LayerBuild);
  if( layer != type() )
  {
    _d->lastLayer = LayerPtr();
    CityRenderer* cRenderer = safety_cast<CityRenderer*>( _d->renderer );
    if( cRenderer )
    {
      _d->lastLayer = cRenderer->currentLayer();
    }
  }
}

void LayerBuild::drawTile( Engine& engine, Tile& tile, const Point& offset )
{
  __D_IMPL(_d,LayerBuild);
  Point screenPos = tile.mappos() + offset;

  ConstructionPtr cntr = ptr_cast<Construction>( tile.overlay() );
  const TilesArray& postTiles = _d->buildTiles;

  if( _d->drawTileBasicPicture )
  {
    const Picture* picBasic = 0;
    const Picture* picOver = 0;
    if( cntr.isValid() && postTiles.size() > 0 )
    {
      picBasic = &cntr->picture();
      picOver = &cntr->picture( _city(), tile.epos(), postTiles );
    }

    if( picOver && picBasic != picOver )
    {
      drawPass( engine, tile, offset, Renderer::ground );
      engine.draw( *picOver, screenPos );
      drawPass( engine, tile, offset, Renderer::overlayAnimation );
    }
    else if( _d->lastLayer.isValid() )
    {
      _d->lastLayer->drawTile( engine, tile, offset );
    }
    else
    {
      Layer::drawTile( engine, tile, offset );
    }
  }
  else
  {
    if( cntr.isValid() )
    {
      const Picture& picOver = cntr->picture( _city(), tile.epos(), postTiles );
      engine.draw( picOver, screenPos );
      drawPass( engine, tile, offset, Renderer::overlayAnimation );
    }
    else
    {
      Layer::drawTile( engine, tile, offset );
    }
  }
}

void LayerBuild::render( Engine& engine)
{
  __D_IMPL(d,LayerBuild);
  d->drawTileBasicPicture = true;
  Layer::render( engine );

  if( ++d->frameCount >= frameCountLimiter)
  {
    _updatePreviewTiles( true );
  }

  d->frameCount %= frameCountLimiter;

  d->drawTileBasicPicture = false;
  _drawBuildTiles( engine );
  engine.draw( *d->textPic, engine.cursorPos() + Point( 10, 10 ));
}

void LayerBuild::init(Point cursor)
{
  __D_IMPL(_d,LayerBuild);
  Layer::init( cursor );

  _d->lastTilePos = TilePos(-1, -1);
  _d->startTilePos = TilePos(-1, -1);

  BuildModePtr command = ptr_cast<BuildMode>( _d->renderer->mode() );
  _d->multiBuilding = command.isValid() ? command->isMultiBuilding() : false;
  _d->roadAssignment = command.isValid() ? command->isRoadAssignment() : false;
  _d->borderBuilding = command.isValid() ? command->isBorderBuilding() : false;
}

void LayerBuild::beforeRender(Engine& engine)
{
  __D_IMPL(_d,LayerBuild);
  if( _d->lastLayer.isValid() )
    _d->lastLayer->beforeRender( engine );
  else
    Layer::beforeRender( engine );
}

void LayerBuild::afterRender(Engine& engine)
{
  __D_IMPL(_d,LayerBuild);
  if( _d->lastLayer.isValid() )
    _d->lastLayer->afterRender( engine );
  else
    Layer::afterRender( engine );
}

const Layer::WalkerTypes& LayerBuild::visibleTypes() const
{
  __D_IMPL_CONST(_d,LayerBuild);
  if( _d->lastLayer.isValid() )
    return _d->lastLayer->visibleTypes();

  return Layer::visibleTypes();
}

LayerPtr LayerBuild::create(Renderer* renderer, PlayerCityPtr city)
{
  LayerPtr ret( new LayerBuild( renderer, city ) );
  ret->drop();

  return ret;
}

LayerBuild::~LayerBuild() {}

LayerBuild::LayerBuild(Renderer* renderer, PlayerCityPtr city)
  : Layer( renderer->camera(), city ),
    __INIT_IMPL(LayerBuild)
{
  __D_IMPL(d,LayerBuild);
  d->renderer = renderer;
  d->frameCount = 0;
  d->startTilePos = TilePos( -1, -1 );
  d->textFont = Font::create( FONT_5 );
  d->textPic.init( Size( 100, 30 ) );
  _addWalkerType( walker::all );

  CityRenderer* cRenderer = safety_cast<CityRenderer*>( d->renderer );
  CONNECT( cRenderer, onLayerSwitch(), this, LayerBuild::_handeLayerSwitch )
}

}//end namespace gfx
