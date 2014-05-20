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
// Copyright 2012-2013 Dalerank, dalerankn8@gmail.com

#include "house.hpp"

#include "gfx/tile.hpp"
#include "objects/house_level.hpp"
#include "core/stringhelper.hpp"
#include "core/exception.hpp"
#include "walker/workerhunter.hpp"
#include "walker/immigrant.hpp"
#include "objects/market.hpp"
#include "objects/objects_factory.hpp"
#include "game/resourcegroup.hpp"
#include "core/variant.hpp"
#include "gfx/tilemap.hpp"
#include "game/gamedate.hpp"
#include "good/goodstore_simple.hpp"
#include "city/helper.hpp"
#include "core/foreach.hpp"
#include "constants.hpp"
#include "events/build.hpp"
#include "events/fireworkers.hpp"
#include "core/gettext.hpp"
#include "core/logger.hpp"
#include "city/funds.hpp"
#include "city/build_options.hpp"

using namespace constants;
using namespace gfx;

namespace {
  enum { maxNegativeStep=-2, maxPositiveStep=2 };
}

class House::Impl
{
public:
  typedef std::map< Service::Type, Service > Services;
  int picIdOffset;
  int houseId;  // pictureId
  int houseLevel;
  //float healthLevel;
  HouseSpecification spec;  // characteristics of the current house level
  Desirability desirability;
  SimpleGoodStore goodStore;
  Services services;  // value=access to the service (0=no access, 100=good access)
  int maxHabitants;
  DateTime taxCheckInterval;
  DateTime lastTaxationDate;
  std::string evolveInfo;
  CitizenGroup habitants;
  int currentYear;
  int changeCondition;

  void updateHealthLevel( HousePtr house );
  void initGoodStore( int size );
  void consumeServices();
  void consumeGoods(HousePtr house);
  void consumeFoods(HousePtr house);
  int getFoodLevel() const;
};

House::House(const int houseId) : Building( building::house ), _d( new Impl )
{
  _d->houseId = houseId;
  _d->taxCheckInterval = DateTime( -400, 1, 1 );
  _d->picIdOffset = ( math::random( 10 ) > 6 ? 1 : 0 );
  HouseSpecHelper& helper = HouseSpecHelper::instance();
  _d->houseLevel = helper.getHouseLevel( houseId );
  _d->spec = helper.getHouseLevelSpec( _d->houseLevel );
  setName( _d->spec.levelName() );
  _d->desirability.base = -3;
  _d->desirability.range = 3;
  _d->desirability.step = 1;
  _d->changeCondition = 0;
  _d->currentYear = GameDate::current().year();

  setState( House::health, 100 );
  setState( House::fire, 0 );
  setState( House::morale, 100 );

  _d->initGoodStore( 1 );

  // init the service access
  for( int i = 0; i<Service::srvCount; ++i )
  {
    // for every service type
    Service::Type service = Service::Type(i);
    _d->services[service] = Service();
  }
  _d->services[ Service::recruter ].setMax( 0 );
  _d->services[ Service::crime ] = 0;

  _update();
}

void House::_makeOldHabitants()
{
  CitizenGroup newHabitants = _d->habitants;
  newHabitants.makeOld();
  newHabitants[ CitizenGroup::newborn ] = 0; //birth+health function from mature habitants count
  newHabitants[ CitizenGroup::longliver ] = 0; //death-health function from oldest habitants count

  _updateHabitants( newHabitants );
}

void House::_updateHabitants( const CitizenGroup& group )
{
  int deltaWorkersNumber = group.count( CitizenGroup::mature ) - _d->habitants.count( CitizenGroup::mature );

  _d->habitants = group;

  _d->services[ Service::recruter ].setMax( _d->habitants.count( CitizenGroup::mature ) );

  int firedWorkersNumber = _d->services[ Service::recruter ] + deltaWorkersNumber;
  _d->services[ Service::recruter ] += deltaWorkersNumber;

  if( firedWorkersNumber < 0 )
  {
    events::GameEventPtr e = events::FireWorkers::create( pos(), abs( firedWorkersNumber ) );
    e->dispatch();
  }
}

void House::_checkEvolve()
{
  bool validate = _d->spec.checkHouse( this, &_d->evolveInfo );
  if( !validate )
  {
    _d->changeCondition--;
    if( _d->changeCondition <= maxNegativeStep )
    {
      _d->changeCondition = 0;
      _levelDown();
    }
  }
  else
  {
    _d->evolveInfo = "";
    bool mayUpgrade =  _d->spec.next().checkHouse( this, &_d->evolveInfo );
    if( mayUpgrade )
    {
      _d->changeCondition++;
      if( _d->changeCondition >= maxPositiveStep )
      {
        _d->changeCondition = 0;
        _levelUp();
      }
    }
    else
    {
      _d->changeCondition = 0;
    }
  }

  if( _d->changeCondition < 0 )
  {
    std::string why;
    _d->spec.checkHouse( this, &why );
    if( !why.empty() )
    {
      why = why.substr( 0, why.size() - 2 );
      why += "_degrade##";
    }
    else
    {
      why = "##house_willbe_degrade##";
    }

    _d->evolveInfo = why;
  }
  else if( _d->changeCondition > 0 )
  {
    _d->evolveInfo = _("##house_evolves_at##");
  }
}

void House::_updateTax()
{
  _d->taxCheckInterval = GameDate::current();
  float cityTax = _city()->funds().taxRate() / 100.f;
  appendServiceValue( Service::forum, (cityTax * _d->spec.taxRate() * _d->habitants.count( CitizenGroup::mature ) / (float)DateTime::monthsInYear) );
}

void House::_updateMorale()
{
  const int currentHabtn = habitants().count();

  if( currentHabtn == 0 )
    return;

  const Service& srvc = _d->services[ Service::recruter ];

  int unemploymentPrc = srvc.value() / srvc.max() * 100;
  updateState( House::morale, unemploymentPrc > 10 ? (-unemploymentPrc / 5.f) : +1 );

  if( _d->spec.getMinFoodLevel() > 0 )
  {
    int foodStoreQty = 0;
    for( int k=Good::wheat; k <= Good::vegetable; k++ )
    {
      foodStoreQty += _d->goodStore.qty( (Good::Type)k );
    }
    const unsigned int habtnConsumeGoodQty = currentHabtn / 2;
    int monthWithFood = foodStoreQty / (habtnConsumeGoodQty+1);

    updateState( House::morale, math::clamp<double>( -4 + 1.25 * monthWithFood, -4., 4. ) );
  }

  appendServiceValue( Service::crime, _d->spec.crime() * ((100 - getState( House::morale )) / 100.f ) );
}

void House::_checkHomeless()
{
  int homelessCount = math::clamp( _d->habitants.count() - _d->maxHabitants, 0, 0xff );
  if( homelessCount > 0 )
  {
    homelessCount /= (homelessCount > 4 ? 2 : 1);
    CitizenGroup homeless = _d->habitants.retrieve( homelessCount );

    int workersFireCount = homeless.count( CitizenGroup::mature );
    if( workersFireCount > 0 )
    {
      events::GameEventPtr e = events::FireWorkers::create( pos(), workersFireCount );
      e->dispatch();
    }

    Immigrant::send2city( _city(), homeless, tile(), "##immigrant_no_home##" );
  }
}

void House::timeStep(const unsigned long time)
{
  if( _d->habitants.empty()  )
    return;

  if( _d->currentYear != GameDate::current().year() )
  {
    _d->currentYear = GameDate::current().year();
    _makeOldHabitants();    
  }

  if( time % spec().getServiceConsumptionInterval() == 0 )
  {
    _d->consumeServices();
    _d->updateHealthLevel( this );
    cancelService( Service::recruter );
  }

  if( time % spec().foodConsumptionInterval() == 0 )
  {
    _d->consumeFoods( this );
  }

  if( time % spec().getGoodConsumptionInterval() == 0 )
  {
    _d->consumeGoods( this );
  }

  if( _d->taxCheckInterval.month() != GameDate::current().month() )
  {
    _updateTax(); 
    _updateMorale();
    _checkHomeless();
  }

  if( GameDate::isWeekChanged() )
  {
    _checkEvolve();
  }

  Building::timeStep( time );
}

void House::_tryEvolve_1_to_11_lvl( int level4grow, int startSmallPic, int startBigPic, const char desirability )
{
  city::Helper helper( _city() );

  if( size() == 1 )
  {
    Tilemap& tmap = _city()->tilemap();
    TilesArray area = tmap.getArea( tile().pos(), Size(2) );
    bool mayGrow = true;

    foreach( it, area )
    {
      if( *it == NULL )
      {
        mayGrow = false;   //some broken, can't grow
        break;
      }

      HousePtr house = ptr_cast<House>( (*it)->overlay() );
      if( house != NULL &&
          (house->spec().level() == level4grow || house->habitants().count() == 0) )
      {
        if( house->size().width() > 1 )  //bigger house near, can't grow
        {
          mayGrow = false;
          break;
        }            
      }
      else
      {
        mayGrow = false; //no house near, can't grow
        break;
      }
    }

    if( mayGrow )
    {
      CitizenGroup sumHabitants = habitants();
      int sumFreeWorkers = getServiceValue( Service::recruter );
      TilesArray::iterator delIt=area.begin();
      HousePtr selfHouse = ptr_cast<House>( (*delIt)->overlay() );

      _d->initGoodStore( Size( size().width() + 1 ).area() );

      ++delIt; //don't remove himself
      for( ; delIt != area.end(); ++delIt )
      {
        HousePtr house = ptr_cast<House>( (*delIt)->overlay() );
        if( house.isValid() )
        {
          house->deleteLater();
          house->_d->habitants.clear();

          sumHabitants += house->habitants();
          sumFreeWorkers += house->getServiceValue( Service::recruter );

          house->_d->services[ Service::recruter ].setMax( 0 );

          selfHouse->goodStore().storeAll( house->goodStore() );
        }
      }

      _d->habitants = sumHabitants;
      setServiceValue( Service::recruter, sumFreeWorkers );

      //reset desirability level with old house size
      helper.updateDesirability( this, false );
      _d->houseId = startBigPic;
      _d->picIdOffset = 0;
      _update();

      build( _city(), pos() );
      //set new desirability level
      helper.updateDesirability( this, true );
    }
  }

  //that this house will be upgrade, we need decrease current desirability level
  helper.updateDesirability( this, false );

  _d->desirability.base = desirability;
  _d->desirability.step = desirability < 0 ? 1 : -1;
  //now upgrade groud area to new desirability
  helper.updateDesirability( this, true );

  bool bigSize = size().width() > 1;
  _d->houseId = bigSize ? startBigPic : startSmallPic; 
  _d->picIdOffset = bigSize ? 0 : ( (rand() % 10 > 6) ? 1 : 0 );
}


void House::_levelUp()
{
  _d->houseLevel++;   
  _d->picIdOffset = 0;
     
  switch (_d->houseLevel)
  {
  case 1:
    _d->houseId = 1;
    _d->desirability.base = -3;
    _d->desirability.step = 1;
  break;

  case 2: _tryEvolve_1_to_11_lvl( 1, 1, 5, -3);
  break;
  
  case 3: _tryEvolve_1_to_11_lvl( 2, 3, 6, -3 );
  break;
  
  case 4: _tryEvolve_1_to_11_lvl( 3, 7, 11, -2 );
  break;
  
  case 5: _tryEvolve_1_to_11_lvl( 4, 9, 12, -2 );
  break;

  case 6: _tryEvolve_1_to_11_lvl( 5, 13, 17, -2 );
  break;

  case 7: _tryEvolve_1_to_11_lvl( 6, 15, 18, -2 );
  break;

  case 8: _tryEvolve_1_to_11_lvl( 7, 19, 23, -1 );
  break;

  case 9: _tryEvolve_1_to_11_lvl( 8, 21, 24, -1 );
  break;

  case 10: _tryEvolve_1_to_11_lvl( 9, 25, 29, 0 );
  break;

  case 11: _tryEvolve_1_to_11_lvl( 10, 27, 30, 0 );
  break;
  }

  _d->spec = HouseSpecHelper::instance().getHouseLevelSpec(_d->houseLevel);

  _update();
}

void House::_tryDegrage_11_to_2_lvl( int smallPic, int bigPic, const char desirability )
{
  bool bigSize = size().width() > 1;
  _d->houseId = bigSize ? bigPic : smallPic;
  _d->picIdOffset = bigSize ? 0 : ( rand() % 10 > 6 ? 1 : 0 );

  city::Helper helper( _city() );
  //clear current desirability influence
  helper.updateDesirability( this, false );

  _d->desirability.base = desirability;
  _d->desirability.step = desirability < 0 ? 1 : -1;
  //set new desirability level
  helper.updateDesirability( this, true );
}

void House::_levelDown()
{
  if( _d->houseLevel == HouseLevel::smallHovel )
    return;

  _d->houseLevel--;
  _d->spec = HouseSpecHelper::instance().getHouseLevelSpec(_d->houseLevel);

  switch (_d->houseLevel)
  {
  case 1:
  {
    _d->houseId = 1;
    _d->picIdOffset = ( rand() % 10 > 6 ? 1 : 0 );

    Tilemap& tmap = _city()->tilemap();

    if( size().area() > 1 )
    {
      TilesArray perimetr = tmap.getArea( pos(), Size(2) );
      int peoplesPerHouse = habitants().count() / 4;
      foreach( tile, perimetr )
      {
        HousePtr house = ptr_cast<House>( TileOverlayFactory::instance().create( building::house ) );
        house->_d->habitants = _d->habitants.retrieve( peoplesPerHouse );
        house->_d->houseId = HouseLevel::smallHovel;
        house->_update();

        events::GameEventPtr event = events::BuildEvent::create( (*tile)->pos(), house.object() );
        event->dispatch();
      }

      _d->services[ Service::recruter ].setMax( 0 );
      deleteLater();
    }
  }
  break;

  case 2: _tryDegrage_11_to_2_lvl( 1, 5, -3 );
  break;

  case 3: _tryDegrage_11_to_2_lvl( 3, 6, -3 );
  break;

  case 4: _tryDegrage_11_to_2_lvl( 7, 11, -2 );
  break;

  case 5: _tryDegrage_11_to_2_lvl( 9, 12, -2 );
  break;

  case 6: _tryDegrage_11_to_2_lvl( 13, 17, -2 );
  break;

  case 7: _tryDegrage_11_to_2_lvl( 15, 18, -2 );
  break;

  case 8: _tryDegrage_11_to_2_lvl( 19, 23, -1 );
  break;

  case 9: _tryDegrage_11_to_2_lvl( 21, 23, -1 );
  break;

  case 10: _tryDegrage_11_to_2_lvl( 25, 29, 0 );
  break;

  case 11: _tryDegrage_11_to_2_lvl( 27, 30, 0 );
  break;
  }

  _update();
}

void House::buyMarket( ServiceWalkerPtr walker )
{
  // std::cout << "House buyMarket" << std::endl;
  MarketPtr market = ptr_cast<Market>( walker->base() );
  if( market.isNull() )
    return;

  GoodStore& marketStore = market->getGoodStore();

  GoodStore &houseStore = goodStore();
  for (int i = 0; i < Good::goodCount; ++i)
  {
    Good::Type goodType = (Good::Type) i;
    int houseQty = houseStore.qty(goodType);
    int houseSafeQty = _d->spec.computeMonthlyGoodConsumption( this, goodType, false )
                       + _d->spec.next().computeMonthlyGoodConsumption( this, goodType, false );
    houseSafeQty *= 6;

    int marketQty = marketStore.qty(goodType);
    if( houseQty < houseSafeQty && marketQty > 0  )
    {
       int qty = std::min( houseSafeQty - houseQty, marketQty);
       qty = math::clamp( qty, 0, houseStore.freeQty( goodType ) );

       if( qty > 0 )
       {
         GoodStock stock(goodType, qty);
         marketStore.retrieve(stock, qty);

         stock.setCapacity( qty );
         stock.setQty( stock.capacity() );

         houseStore.store(stock, stock.qty() );
       }
    }
  }
}

void House::applyService( ServiceWalkerPtr walker )
{
  Building::applyService(walker);  // handles basic services, and remove service reservation

  Service::Type service = walker->getService();
  switch (service)
  {
  case Service::well:
  case Service::fountain:
  case Service::religionNeptune:
  case Service::religionCeres:
  case Service::religionVenus:
  case Service::religionMars:
  case Service::religionMercury:
  case Service::barber:
  case Service::baths:
  case Service::school:
  case Service::library:
  case Service::academy:
  case Service::theater:
  case Service::amphitheater:
  case Service::colloseum:
  case Service::hippodrome:
    setServiceValue(service, 100);
  break;

  case Service::hospital:
  case Service::doctor:
    updateState( (Construction::Param)House::health, 10 );
    setServiceValue(service, 100);
  break;
  
  case Service::market:
    setServiceValue( Service::market, 100 );
    buyMarket(walker);
  break;
 
  case Service::prefect:
    appendServiceValue(Service::crime, -25);
  break;

  case Service::oracle:
  case Service::engineer:
  case Service::srvCount:
  break;

  case Service::recruter:
  {
    int svalue = getServiceValue( service );
    if( !svalue )
      break;

    RecruterPtr hunter = ptr_cast<Recruter>( walker );
    if( hunter.isValid() )
    {
      int hiredWorkers = math::clamp( svalue, 0, hunter->getWorkersNeeded() );
      appendServiceValue( service, -hiredWorkers );
      hunter->hireWorkers( hiredWorkers );
    }
  }
  break;

  default:
  break;
  }
}

float House::evaluateService(ServiceWalkerPtr walker)
{
  float res = 0.0;
  Service::Type service = walker->getService();
  if( _reservedServices.count(service) == 1 )
  {
     // service is already reserved
    return 0.0;
  }

  switch(service)
  {
  case Service::engineer: res = getState( Construction::damage ); break;
  case Service::prefect: res = getState( Construction::fire ); break;

  // this house pays taxes
  case Service::forum:
  case Service::senate:
    res = _d->services[ Service::forum ];
  break;

  case Service::market:
  {
    MarketPtr market = ptr_cast<Market>( walker->base() );
    GoodStore &marketStore = market->getGoodStore();
    GoodStore &houseStore = goodStore();
    for (int i = 0; i < Good::goodCount; ++i)
    {
      Good::Type goodType = (Good::Type) i;
      int houseQty = houseStore.qty(goodType) / 10;
      int houseSafeQty = _d->spec.computeMonthlyGoodConsumption( this, goodType, false)
                         + _d->spec.next().computeMonthlyGoodConsumption( this, goodType, false );
      int marketQty = marketStore.qty(goodType);
      if( houseQty < houseSafeQty && marketQty > 0)
      {
         res += std::min( houseSafeQty - houseQty, marketQty);
      }
    }
  }
  break;

  case Service::recruter:
  {
    res = (float)getServiceValue( service );
  }
  break;   

  default:
  {
    return _d->spec.evaluateServiceNeed( this, service);
  }
  break;
  }

  // std::cout << "House evaluateService " << service << "=" << res << std::endl;

  return res;
}

TilesArray House::getEnterArea() const
{
  if( isWalkable() )
  {
    TilesArray ret;
    ret.push_back( &tile() );
    return ret;
  }
  else
  {
    return Building::getEnterArea();
  }
}

double House::getState( ParameterType param) const
{
  switch( (int)param )
  {
  case House::food: return _d->getFoodLevel();

  default: return Building::getState( param );
  }
}

void House::_update()
{
  int picId = ( _d->houseId == HouseLevel::smallHovel && _d->habitants.count() == 0 ) ? 45 : (_d->houseId + _d->picIdOffset);
  Picture pic = Picture::load( ResourceGroup::housing, picId );
  setPicture( pic );
  setSize( Size( (pic.width() + 2 ) / 60 ) );
  _d->maxHabitants = _d->spec.getMaxHabitantsByTile() * size().area();
  _d->services[ Service::forum ].setMax( _d->spec.taxRate() * _d->maxHabitants );
  _d->initGoodStore( size().area() );
}

int House::getRoadAccessDistance() const {  return 2; }

void House::addHabitants( CitizenGroup& habitants )
{
  int peoplesCount = math::clamp(  _d->maxHabitants - _d->habitants.count(), 0, _d->maxHabitants );
  CitizenGroup newState = _d->habitants;
  newState += habitants.retrieve( peoplesCount );

  _updateHabitants( newState );
  _update();
}

CitizenGroup House::remHabitants(int count)
{
  count = math::clamp<int>( count, 0, _d->habitants.count() );
  CitizenGroup hb = _d->habitants.retrieve( count );

  _updateHabitants( _d->habitants );

  return hb;
}

void House::destroy()
{
  _d->maxHabitants = 0;

  const int maxCitizenInGroup = 8;
  do
  {
    CitizenGroup homeless = _d->habitants.retrieve( std::min<int>( _d->habitants.count(), maxCitizenInGroup ) );
    Immigrant::send2city( _city(), homeless, tile(), "##immigrant_thrown_from_house##" );
  }
  while( _d->habitants.count() >= maxCitizenInGroup );

  if( workersCount() > 0 )
  {
    events::GameEventPtr e = events::FireWorkers::create( pos(), workersCount() );
    e->dispatch();
  }

  _d->habitants.clear();

  Building::destroy();
}

std::string House::sound() const
{
  if( !_d->habitants.count() )
    return "";

  return StringHelper::format( 0xff, "house_%05d.wav", _d->houseLevel*10+1 );
}

std::string House::troubleDesc() const
{
  std::string ret = Building::troubleDesc();

  if( ret.empty() )
  {
    ret = _d->evolveInfo;
  }

  return ret;
}

bool House::isCheckedDesirability() const {  return _city()->buildOptions().isCheckDesirability(); }

void House::save( VariantMap& stream ) const
{
  Building::save( stream );

  stream[ "picIdOffset" ] = _d->picIdOffset;
  stream[ "houseId" ] = _d->houseId;
  stream[ "houseLevel" ] = _d->houseLevel;
  stream[ "desirability" ] = _d->desirability.base;
  stream[ "currentHubitants" ] = _d->habitants.save();
  stream[ "maxHubitants" ] = _d->maxHabitants;
  stream[ "goodstore" ] = _d->goodStore.save();
  stream[ "healthLevel" ] = getState( (Construction::Param)House::health );
  stream[ "changeCondition" ] = _d->changeCondition;

  VariantList vl_services;
  foreach( mapItem, _d->services )
  {
    vl_services.push_back( Variant( (int)mapItem->first) );
    vl_services.push_back( Variant( mapItem->second ) );
  }

  stream[ "services" ] = vl_services;
} 

void House::load( const VariantMap& stream )
{
  Building::load( stream );

  _d->picIdOffset = (int)stream.get( "picIdOffset", 0 );
  _d->houseId = (int)stream.get( "houseId", 0 );
  _d->houseLevel = (int)stream.get( "houseLevel", 0 );
  //_d->healthLevel = (float)stream.get( "healthLevel", 0 );
  _d->spec = HouseSpecHelper::instance().getHouseLevelSpec(_d->houseLevel);

  _d->desirability.base = (int)stream.get( "desirability", 0 );
  _d->desirability.step = _d->desirability.base < 0 ? 1 : -1;

  _d->habitants.load( stream.get( "currentHubitants" ).toList() );
  _d->maxHabitants = (int)stream.get( "maxHubitants", 0 );
  _d->changeCondition = stream.get( "changeCondition", 0 );
  _d->goodStore.load( stream.get( "goodstore" ).toMap() );
  _d->currentYear = GameDate::current().year();

  _d->initGoodStore( size().area() );

  _d->services[ Service::recruter ].setMax( _d->habitants.count( CitizenGroup::mature ) );
  VariantList vl_services = stream.get( "services" ).toList();

  for( unsigned int i=0; i < vl_services.size(); i++ )
  {
    Service::Type type = Service::Type( vl_services.get( i ).toInt() );
    _d->services[ type ] = vl_services.get( i+1 ).toFloat(); //serviceValue
  }

  Building::build( _city(), pos() );
  _update();
}

int House::Impl::getFoodLevel() const
{
  const Good::Type f[] = { Good::wheat, Good::fish, Good::meat, Good::fruit, Good::vegetable };
  std::set<Good::Type> foods( f, f+5 );

  int ret = 0;
  int foodLevel = spec.getMinFoodLevel();
  if( foodLevel == 0 )
    return 0;

  while( foodLevel > 0 )
  {
    Good::Type maxFtype = Good::none;
    int maxFoodQty = 0;
    foreach( ft, foods )
    {
      int tmpQty = goodStore.qty( *ft );
      if( tmpQty > maxFoodQty )
      {
        maxFoodQty = tmpQty;
        maxFtype = *ft;
      }
    }

    ret += maxFoodQty * 100 / goodStore.capacity( maxFtype );
    foods.erase( maxFtype );
    foodLevel--;
  }

  ret /= spec.getMinFoodLevel();
  return ret;
}

int House::workersCount() const
{
  const Service& srvc = _d->services[ Service::recruter ];
  return srvc.max() - srvc.value();
}

bool House::isEducationNeed(Service::Type type) const
{
  int lvl = _d->spec.getMinEducationLevel();
  switch( type )
  {
  case Service::school: return (lvl>0);
  case Service::academy: return (lvl>1);
  case Service::library: return (lvl>2);
  default: break;
  }

  return false;
}

bool House::isEntertainmentNeed(Service::Type type) const
{
  int lvl = _d->spec.getMinEntertainmentLevel();
  switch( type )
  {
  case Service::theater: return (lvl>=10);
  case Service::amphitheater: return (lvl>=30);
  case Service::colloseum: return (lvl>=60);
  case Service::hippodrome: return (lvl>=80);
  default: break;
  }

  return false;
}

float House::collectTaxes()
{
  float tax = getServiceValue( Service::forum );
  setServiceValue( Service::forum, 0 );
  _d->lastTaxationDate = GameDate::current();
  return tax;
}

DateTime House::getLastTaxation() const{  return _d->lastTaxationDate;}
std::string House::getEvolveInfo() const{  return _d->evolveInfo;}
Desirability House::getDesirability() const {  return _d->desirability; }
bool House::isWalkable() const{  return (_d->houseId == HouseLevel::smallHovel && _d->habitants.count() == 0); }
bool House::isFlat() const { return false; }//isWalkable(); }
const CitizenGroup& House::habitants() const  {  return _d->habitants; }
GoodStore& House::goodStore(){   return _d->goodStore;}
const HouseSpecification& House::spec() const{   return _d->spec; }
bool House::hasServiceAccess( Service::Type service) {  return (_d->services[service] > 0); }
float House::getServiceValue( Service::Type service){  return _d->services[service]; }
void House::setServiceValue( Service::Type service, float value) {  _d->services[service] = value; }
int House::maxHabitants() {  return _d->maxHabitants; }
void House::appendServiceValue( Service::Type srvc, float value){  setServiceValue( srvc, getServiceValue( srvc ) + value ); }


void House::Impl::updateHealthLevel( HousePtr house )
{
  float delim = 1 + (((services[Service::well] > 0 || services[Service::fountain] > 0) ? 1 : 0))
      + ((services[Service::doctor] > 0 || services[Service::hospital] > 0) ? 1 : 0)
      + (services[Service::baths] > 0 ? 0.7 : 0)
      + (services[Service::barber] > 0 ? 0.3 : 0);

  float decrease = 0.3f / delim;

  house->updateState( (Construction::Param)House::health, -decrease );
}

void House::Impl::initGoodStore(int size)
{
  int rsize = 25 * size * houseLevel;
  goodStore.setCapacity( rsize * 10 );  // no limit
  goodStore.setCapacity(Good::wheat, rsize );
  goodStore.setCapacity(Good::fish, rsize );
  goodStore.setCapacity(Good::meat, rsize );
  goodStore.setCapacity(Good::fruit, rsize );
  goodStore.setCapacity(Good::vegetable, rsize );
  goodStore.setCapacity(Good::pottery, rsize );
  goodStore.setCapacity(Good::furniture, rsize);
  goodStore.setCapacity(Good::oil, rsize );
  goodStore.setCapacity(Good::wine, rsize );
}

void House::Impl::consumeServices()
{
  int currentWorkersPower = services[ Service::recruter ];       //save available workers number
  float tax = services[ Service::forum ];

  foreach( s, services ) { s->second -= 1; } //consume services

  services[ Service::recruter ] = currentWorkersPower;     //restore available workers number
  services[ Service::forum ] = tax;
}

void House::Impl::consumeGoods( HousePtr house )
{
  for( int i = Good::olive; i < Good::goodCount; ++i)
  {
     Good::Type goodType = (Good::Type) i;
     int montlyGoodsQty = spec.computeMonthlyGoodConsumption( house, goodType, true );
     goodStore.setQty( goodType, std::max( goodStore.qty(goodType) - montlyGoodsQty, 0) );
  }
}

void House::Impl::consumeFoods(HousePtr house)
{
  const int foodLevel = spec.getMinFoodLevel();
  if( foodLevel == 0 )
    return;


  const int needFoodQty = spec.computeMonthlyFoodConsumption( house ) * spec.foodConsumptionInterval() / GameDate::days2ticks( 30 );

  int availableFoodLevel = 0;
  for( int afl=Good::wheat; afl <= Good::vegetable; afl++ )
  {
    availableFoodLevel += ( goodStore.qty( (Good::Type)afl ) > 0 ? 1 : 0 );
  }
  availableFoodLevel = std::min( availableFoodLevel, foodLevel );
  bool haveFoods4Eating = ( availableFoodLevel > 0 );

  if( haveFoods4Eating )
  {
    int alsoNeedFood = needFoodQty;
    while( alsoNeedFood > 0 )
    {
      int realConsumedQty = 0;
      for( int k=Good::wheat; k <= Good::vegetable; k++ )
      {
        Good::Type gType = (Good::Type)k;
        int vQty = std::min( goodStore.qty( gType ), needFoodQty / availableFoodLevel );
        vQty = std::min( vQty, alsoNeedFood );
        if( vQty > 0 )
        {
          realConsumedQty += vQty;
          alsoNeedFood -= vQty;
          goodStore.setQty( gType, std::max( goodStore.qty( gType ) - vQty, 0) );
        }
      }

      if( realConsumedQty == 0 )
      {
        haveFoods4Eating = false;
        break;
      }
    }
  }

  if( !haveFoods4Eating )
  {
    Logger::warning( "House: [%dx%d] have no food for habitants", house->pos().i(), house->pos().j() );
  }
}
