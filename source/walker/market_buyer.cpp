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

#include "market_buyer.hpp"
#include "objects/metadata.hpp"
#include "core/exception.hpp"
#include "core/position.hpp"
#include "objects/market.hpp"
#include "objects/granary.hpp"
#include "gfx/tilemap.hpp"
#include "gfx/tile.hpp"
#include "core/variant.hpp"
#include "pathway/path_finding.hpp"
#include "market_kid.hpp"
#include "good/goodstore_simple.hpp"
#include "city/helper.hpp"
#include "name_generator.hpp"
#include "objects/constants.hpp"
#include "game/gamedate.hpp"

using namespace constants;
using namespace gfx;

class MarketBuyer::Impl
{
public:
  TilePos destBuildingPos;  // granary or warehouse
  Good::Type priorityGood;
  int maxDistance;
  MarketPtr market;
  SimpleGoodStore basket;
  int reservationID;
};

MarketBuyer::MarketBuyer(PlayerCityPtr city )
  : Human( city ), _d( new Impl )
{
   _setType( walker::marketBuyer );
   _d->maxDistance = 25;
   _d->basket.setCapacity(800);  // this is a big basket!

   _d->basket.setCapacity(Good::wheat, 800);
   _d->basket.setCapacity(Good::fruit, 800);
   _d->basket.setCapacity(Good::vegetable, 800);
   _d->basket.setCapacity(Good::meat, 800);
   _d->basket.setCapacity(Good::fish, 800);

   _d->basket.setCapacity(Good::pottery, 300);
   _d->basket.setCapacity(Good::furniture, 300);
   _d->basket.setCapacity(Good::oil, 300);
   _d->basket.setCapacity(Good::wine, 300);

   setName( NameGenerator::rand( NameGenerator::female ) );
}

MarketBuyer::~MarketBuyer(){}

template< class T >
TilePos getWalkerDestination2( Propagator &pathPropagator, const TileOverlay::Type type,
                               MarketPtr market, SimpleGoodStore& basket, const Good::Type what,
                               Pathway& oPathWay, int& reservId )
{
  SmartPtr< T > res;

  DirectPRoutes pathWayList = pathPropagator.getRoutes( type );

  int max_qty = 0;

  // select the warehouse with the max quantity of requested goods
  foreach( routeIt, pathWayList )
  {
    // for every warehouse within range
    ConstructionPtr construction = routeIt->first;
    //PathwayPtr pathWay= routeIt->second;

    SmartPtr< T > destBuilding = ptr_cast<T>( construction );
    int qty = destBuilding->store().getMaxRetrieve( what );
    if( qty > max_qty )
    {
      res = destBuilding;
      oPathWay = *routeIt->second.object();
      max_qty = qty;
    }
  }

  if( res.isValid() )
  {
    // a warehouse/granary has been found!
    // reserve some goods from that warehouse/granary
    int qty = std::min( max_qty, market->getGoodDemand( what ) );
    qty = std::min(qty, basket.capacity( what ) - basket.qty( what ));
    // std::cout << "MarketLady reserves from warehouse, qty=" << qty << std::endl;
    reservId = res->store().reserveRetrieval( what, qty, GameDate::current() );
    return res->pos();
  }

  return TilePos(-1, -1);
}

void MarketBuyer::computeWalkerDestination( MarketPtr market )
{
  _d->market = market;
  std::list<Good::Type> priorityGoods = _d->market->mostNeededGoods();

  _d->destBuildingPos = TilePos( -1, -1 );  // no destination yet

  if( priorityGoods.size() > 0 )
  {
    // we have something to buy!
    // get the list of buildings within reach
    Pathway pathWay;
    Propagator pathPropagator( _city() );
    pathPropagator.init( ptr_cast<Construction>( _d->market ) );
    pathPropagator.setAllDirections( false );
    pathPropagator.propagate( _d->maxDistance);

    // try to find the most needed good
    foreach( goodType, priorityGoods )
    {
      _d->priorityGood = *goodType;

      if( _d->priorityGood == Good::wheat || _d->priorityGood == Good::fish
          || _d->priorityGood == Good::meat || _d->priorityGood == Good::fruit
          || _d->priorityGood == Good::vegetable)
      {
        // try get that good from a granary
        _d->destBuildingPos = getWalkerDestination2<Granary>( pathPropagator, building::granary, _d->market,
                                                              _d->basket, _d->priorityGood, pathWay, _d->reservationID );

        if( _d->destBuildingPos.i() < 0 )
        {
          _d->destBuildingPos = getWalkerDestination2<Warehouse>( pathPropagator, building::warehouse, _d->market,
                                                                _d->basket, _d->priorityGood, pathWay, _d->reservationID );
        }
      }
      else
      {
        // try get that good from a warehouse
        _d->destBuildingPos = getWalkerDestination2<Warehouse>( pathPropagator, building::warehouse, _d->market,
                                                                _d->basket, _d->priorityGood, pathWay, _d->reservationID );
      }

      if( _d->destBuildingPos.i() >= 0 )
      {
        // we found a destination!
        setPos( pathWay.startPos() );
        setPathway( pathWay );
         break;
      }
    }
  }

  if( _d->destBuildingPos.i() < 0)
  {
    // we have nothing to buy, or cannot find what we need to buy
    deleteLater();
    return;
  }
}

std::string MarketBuyer::thoughts( Thought th) const
{
  switch( th )
  {
  case thCurrent:
    if( !pathway().isReverse() )
    {
      return "##marketBuyer_find_goods##";
    }
    else
    {
      return "##marketBuyer_return##";
    }
  break;

  default:
  break;
  }

  return "";
}

TilePos MarketBuyer::places(Walker::Place type) const
{
  switch( type )
  {
  case plOrigin: return _d->market.isValid() ? _d->market->pos() : TilePos( -1, -1 );
  case plDestination: return _d->destBuildingPos;
  default: break;
  }

  return Human::places( type );
}


void MarketBuyer::_reachedPathway()
{
   Walker::_reachedPathway();
   if( _pathwayRef().isReverse() )
   {
     // walker is back in the market
     deleteLater();
     if( _d->market.isValid() )
     {
       // put the content of the basket in the market
       _d->market->goodStore().storeAll( _d->basket );
     }
   }
   else
   {
      // get goods from destination building
      TileOverlayPtr building = _city()->tilemap().at( _d->destBuildingPos ).overlay();
      
      if( is_kind_of<Granary>( building ) )
      {
        GranaryPtr granary = ptr_cast<Granary>( building );
        // this is a granary!
        // std::cout << "MarketLady arrives at granary, res=" << _reservationID << std::endl;
        granary->store().applyRetrieveReservation(_d->basket, _d->reservationID);

        // take other goods if possible
        for (int n = Good::wheat; n<=Good::vegetable; ++n)
        {
          // for all types of good (except G_NONE)
          Good::Type goodType = (Good::Type) n;
          int qty = _d->market->getGoodDemand(goodType) - _d->basket.qty(goodType);
          if (qty > 0)
          {
            qty = std::min(qty, granary->store().getMaxRetrieve(goodType));
            qty = std::min(qty, _d->basket.capacity(_d->priorityGood) - _d->basket.qty(_d->priorityGood));
            if (qty > 0)
            {
              // std::cout << "extra retrieve qty=" << qty << " basket=" << _basket.getStock(goodType)._currentQty << std::endl;
              GoodStock& stock = _d->basket.getStock(goodType);
              granary->store().retrieve(stock, qty);
            }
          }
        }
      }
      else if( is_kind_of<Warehouse>( building ) )
      {
        WarehousePtr warehouse = ptr_cast<Warehouse>( building );
        // this is a warehouse!
        warehouse->store().applyRetrieveReservation(_d->basket, _d->reservationID);

        // take other goods if possible
        for (int n = Good::wheat; n<Good::goodCount; ++n)
        {
          // for all types of good (except G_NONE)
          Good::Type goodType = (Good::Type) n;
          int qty = _d->market->getGoodDemand(goodType) - _d->basket.qty(goodType);
          if (qty > 0)
          {
            qty = std::min(qty, warehouse->store().getMaxRetrieve(goodType));
            qty = std::min(qty, _d->basket.capacity(_d->priorityGood) - _d->basket.qty(_d->priorityGood));
            if (qty > 0)
            {
              // std::cout << "extra retrieve qty=" << qty << " basket=" << _basket.getStock(goodType)._currentQty << std::endl;
              GoodStock& stock = _d->basket.getStock(goodType);
              warehouse->store().retrieve(stock, qty);
            }
          }
        }
      }

      unsigned long delay = 20;

      while( _d->basket.qty() > 100 )
      {
        for( int gtype=Good::wheat; gtype < Good::goodCount ; gtype++ )
        {
          GoodStock& currentStock = _d->basket.getStock( (Good::Type)gtype );
          if( currentStock.qty() > 0 )
          {
            MarketKidPtr boy = MarketKid::create( _city(), this );
            GoodStock& boyBasket =  boy->getBasket();
            boyBasket.setType( (Good::Type)gtype );
            boyBasket.setCapacity( 100 );
            _d->basket.retrieve( boyBasket, math::clamp( currentStock.qty(), 0, 100 ) );
            boy->setDelay( delay );
            delay += 20;
            boy->send2City( _d->market );
            _d->market->addWalker( ptr_cast<Walker>( boy ) );
          }
        }
      }

      // walker is near the granary/warehouse
      _pathwayRef().move( Pathway::reverse );
      _centerTile();
      go();
   }
}

void MarketBuyer::send2City( MarketPtr market )
{
  computeWalkerDestination( market );

  if( !isDeleted() )
  {
    _city()->addWalker( WalkerPtr( this ) );
  }
}

void MarketBuyer::save( VariantMap& stream ) const
{
  Walker::save( stream );
  VARIANT_SAVE_ANY_D( stream, _d, destBuildingPos );
  stream[ "priorityGood" ] = (int)_d->priorityGood;
  stream[ "marketPos" ] = _d->market->pos();

  stream[ "basket" ] = _d->basket.save();
  VARIANT_SAVE_ANY_D( stream, _d, maxDistance );
  VARIANT_SAVE_ANY_D( stream, _d, reservationID );
}

void MarketBuyer::load( const VariantMap& stream)
{
  Walker::load( stream );
  VARIANT_LOAD_ANY_D( _d, destBuildingPos, stream );
  _d->priorityGood = (Good::Type)stream.get( "priorityGood" ).toInt();

  TilePos tpos = stream.get( "marketPos" ).toTilePos();

  _d->market << _city()->getOverlay( tpos );

  _d->basket.load( stream.get( "basket" ).toMap() );
  VARIANT_LOAD_ANY_D( _d, maxDistance, stream );
  VARIANT_LOAD_ANY_D( _d, reservationID, stream );
}

MarketBuyerPtr MarketBuyer::create( PlayerCityPtr city )
{
  MarketBuyerPtr ret( new MarketBuyer( city ) );
  ret->drop();

  return ret;
}
