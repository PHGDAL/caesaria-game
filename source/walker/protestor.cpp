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

#include "protestor.hpp"
#include "objects/house.hpp"
#include "pathway/path_finding.hpp"
#include "constants.hpp"
#include "city/helper.hpp"
#include "objects/house_level.hpp"
#include "objects/constants.hpp"
#include "core/foreach.hpp"
#include "pathway/astarpathfinding.hpp"
#include "gfx/tilemap.hpp"
#include "objects/constants.hpp"
#include "pathway/pathway_helper.hpp"
#include "corpse.hpp"
#include "ability.hpp"
#include "game/resourcegroup.hpp"
#include "core/variant.hpp"

using namespace constants;

class Protestor::Impl
{
public:
  typedef enum { searchHouse=0, go2destination, searchAnyBuilding,
                 destroyConstruction, go2anyplace, gooutFromCity, wait } State;
  int houseLevel;
  State state;

  Pathway findTarget( PlayerCityPtr city, ConstructionList constructions, TilePos pos );
};

Protestor::Protestor(PlayerCityPtr city) : Walker( city ), _d( new Impl )
{    
  _setAnimation( gfx::protestor );
  _setType( walker::protestor );

  addAbility( Illness::create( 0.3, 4) );
}

void Protestor::_changeTile()
{
  Walker::_changeTile();
}

void Protestor::_reachedPathway()
{
  Walker::_reachedPathway();

  switch( _d->state )
  {
  case Impl::go2destination:
    _animationRef().clear();
    _animationRef().load( ResourceGroup::citizen2, 455, 8 );
    _animationRef().load( ResourceGroup::citizen2, 462, 8, Animation::reverse );
    _setAction( acFight );
    _d->state = Impl::destroyConstruction;
  break;

  default: break;
  }
}

void Protestor::timeStep(const unsigned long time)
{
  Walker::timeStep( time );

  switch( _d->state )
  {
  case Impl::searchHouse:
  {
    CityHelper helper( _getCity() );
    ConstructionList constructions = helper.find<Construction>( building::house );
    for( ConstructionList::iterator it=constructions.begin(); it != constructions.end(); )
    {
      HousePtr h = ptr_cast<House>( *it );
      if( h->getSpec().getLevel() <= _d->houseLevel ) { it=constructions.erase( it ); }
      else { it++; }
    }

    Pathway pathway = _d->findTarget( _getCity(), constructions, pos() );
    //find more expensive house, fire this!!!
    if( pathway.isValid() )
    {
      setPos( pathway.getStartPos() );
      setPathway( pathway );
      go();
      _d->state = Impl::go2destination;
    }
    else //not find house, try find any, that rioter can destroy
    {
      _d->state = Impl::searchAnyBuilding;
    }
  }
  break;

  case Impl::searchAnyBuilding:
  {
    CityHelper helper( _getCity() );
    ConstructionList constructions = helper.find<Construction>( building::house );
    constructions = helper.find<Construction>( building::any );

    for( ConstructionList::iterator it=constructions.begin(); it != constructions.end(); )
    {
      TileOverlay::Type type = (*it)->getType();
      TileOverlay::Group group = (*it)->getClass();
      if( type == building::house || type == construction::road
          || group == building::disasterGroup ) { it=constructions.erase( it ); }
      else { it++; }
    }

    Pathway pathway = _d->findTarget( _getCity(), constructions, pos() );
    if( pathway.isValid() )
    {
      setPos( pathway.getStartPos() );
      setPathway( pathway );
      go();
      _d->state = Impl::go2destination;
    }
    else
    {
      _d->state = Impl::go2anyplace;
    }
  }
  break;

  case Impl::go2anyplace:
  {
    Pathway pathway = PathwayHelper::randomWay( _getCity(), pos(), 10 );

    if( pathway.isValid() )
    {
      setPathway( pathway );
      go();
      _d->state = Impl::go2destination;
    }
    else
    {
      die();
      _d->state = Impl::wait;
    }
  }
  break;

  case Impl::go2destination:
  case Impl::wait:
  break;

  case Impl::destroyConstruction:
  {
    if( time % 16 == 1 )
    {

      CityHelper helper( _getCity() );
      ConstructionList constructions = helper.find<Construction>( building::any, pos() - TilePos( 1, 1), pos() + TilePos( 1, 1) );

      for( ConstructionList::iterator it=constructions.begin(); it != constructions.end(); )
      {
        if( (*it)->getType() == construction::road || (*it)->getClass() == building::disasterGroup )
        { it=constructions.erase( it ); }
        else { it++; }
      }

       if( constructions.empty() )
      {
        _animationRef().clear();
        _setAction( acMove );
        _d->state = Impl::searchHouse;
      }
      else
      {
        foreach( it, constructions )
        {
          ConstructionPtr c = *it;
          if( c->getClass() != building::disasterGroup && c->getType() != construction::road )
          {
            c->updateState( Construction::fire, 1 );
            c->updateState( Construction::damage, 1 );
            break;
          }
        }
      }
    }
  }
  break;

  default: break;
  }
}

ProtestorPtr Protestor::create(PlayerCityPtr city )
{ 
  ProtestorPtr ret( new Protestor( city ) );
  ret->drop();
  return ret;
}

Protestor::~Protestor()
{

}

void Protestor::send2City( HousePtr house )
{
  setPos( house->pos() );
  _d->houseLevel = house->getSpec().getLevel();
  _d->state = Impl::searchHouse;

  if( !isDeleted() )
  {
    _getCity()->addWalker( WalkerPtr( this ));
  }
}

void Protestor::die()
{
  Walker::die();

  Corpse::create( _getCity(), pos(), ResourceGroup::citizen2, 447, 454 );
}

void Protestor::save(VariantMap& stream) const
{
  Walker::save( stream );

  stream[ "houseLevel" ] = _d->houseLevel;
  stream[ "state" ] = (int)_d->state;
}

void Protestor::load(const VariantMap& stream)
{
  Walker::load( stream );

  _d->houseLevel = stream.get( "houseLevel" );
  _d->state = (Impl::State)stream.get( "state" ).toInt();
}

Pathway Protestor::Impl::findTarget(PlayerCityPtr city, ConstructionList constructions, TilePos pos )
{  
  if( !constructions.empty() )
  {
    Pathway pathway;
    for( int i=0; i<10; i++)
    {
      ConstructionList::iterator it = constructions.begin();
      std::advance( it, rand() % constructions.size() );

      pathway = PathwayHelper::create( pos, *it, PathwayHelper::allTerrain );
      if( pathway.isValid() )
      {
        return pathway;
      }
    }
  }

  return Pathway();
}
