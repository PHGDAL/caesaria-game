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

#include "market.hpp"
#include "gfx/picture.hpp"
#include "game/resourcegroup.hpp"
#include "walker/market_buyer.hpp"
#include "core/variant.hpp"
#include "good/goodstore_simple.hpp"
#include "city/city.hpp"
#include "walker/serviceman.hpp"
#include "objects/constants.hpp"
#include "game/gamedate.hpp"
#include "walker/helper.hpp"

using namespace gfx;

class Market::Impl
{
public:
  SimpleGoodStore goodStore;

  bool isAnyGoodStored()
  {
    bool anyGoodStored = false;
    for( int i = 0; i < Good::goodCount; ++i)
    {
      anyGoodStored |= ( goodStore.qty( Good::Type(i) ) >= 100 );
    }

    return anyGoodStored;
  }

  void initStore()
  {
    goodStore.setCapacity(5000);
    goodStore.setCapacity(Good::wheat, 800);
    goodStore.setCapacity(Good::fish, 600);
    goodStore.setCapacity(Good::fruit, 600);
    goodStore.setCapacity(Good::meat, 600);
    goodStore.setCapacity(Good::vegetable, 600);
    goodStore.setCapacity(Good::pottery, 250);
    goodStore.setCapacity(Good::furniture, 250);
    goodStore.setCapacity(Good::oil, 250);
    goodStore.setCapacity(Good::wine, 250);
  }
};

Market::Market() : ServiceBuilding(Service::market, constants::building::market, Size(2) ),
  _d( new Impl )
{
  _fgPicturesRef().resize(1);  // animation
  _d->initStore();

  _animationRef().load( ResourceGroup::commerce, 2, 10 );
  _animationRef().setDelay( 4 );
}

void Market::deliverService()
{
  if( numberWorkers() > 0 && walkers().size() == 0 )
  {
    // the marketBuyer is ready to buy something!
    MarketBuyerPtr buyer = MarketBuyer::create( _city() );
    buyer->send2City( this );

    if( !buyer->isDeleted() )
    {
      addWalker( buyer.object() );
    }
    else if( _d->isAnyGoodStored() )
    {
      ServiceBuilding::deliverService();
    }
  }
}

unsigned int Market::walkerDistance() const {  return 26; }
GoodStore& Market::goodStore(){  return _d->goodStore; }

std::list<Good::Type> Market::mostNeededGoods()
{
  std::list<Good::Type> res;

  std::multimap<float, Good::Type> mapGoods;  // ordered by demand

  for (int n = 0; n < Good::goodCount; ++n)
  {
    // for all types of good
    Good::Type goodType = (Good::Type) n;
    GoodStock &stock = _d->goodStore.getStock(goodType);
    int demand = stock.capacity() - stock.qty();
    if (demand > 200)
    {
      mapGoods.insert( std::make_pair(float(stock.qty())/float(stock.capacity()), goodType));
    }
  }

  for( std::multimap<float, Good::Type>::iterator itMap = mapGoods.begin(); itMap != mapGoods.end(); ++itMap)
  {
    Good::Type goodType = itMap->second;
    res.push_back(goodType);
  }

  return res;
}


int Market::getGoodDemand(const Good::Type &goodType)
{
  int res = 0;
  GoodStock &stock = _d->goodStore.getStock(goodType);
  res = stock.capacity() - stock.qty();
  res = (res/100)*100;  // round at the lowest century
  return res;
}

void Market::save( VariantMap& stream) const 
{
  ServiceBuilding::save( stream );
  stream[ "goodStore" ] = _d->goodStore.save();
}

void Market::load( const VariantMap& stream)
{
  ServiceBuilding::load( stream );

  _d->goodStore.load( stream.get( "goodStore" ).toMap() );

  _d->initStore();
}

void Market::timeStep(const unsigned long time)
{
  if( GameDate::isWeekChanged() )
  {
    ServiceWalkerList servicemen;
    servicemen << walkers();
    if( servicemen.size() > 0 && _d->goodStore.qty() == 0 )
    {
      servicemen.front()->return2Base();
    }
  }

  ServiceBuilding::timeStep( time );
}
