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

#ifndef _CAESARIA_WAREHOUSE_HPP_INCLUDE_
#define _CAESARIA_WAREHOUSE_HPP_INCLUDE_

#include "working.hpp"
#include "game/enums.hpp"
#include "good/good.hpp"
#include "core/position.hpp"

class GoodStore;

class Warehouse: public WorkingBuilding
{
  friend class WarehouseStore;

public:
  typedef enum { sellGoodsBuff, buyGoodsBuff } Buff;
  Warehouse();

  virtual void timeStep(const unsigned long time);
  void computePictures();
  GoodStore& store();
  const GoodStore& store() const;
  
  virtual void save(VariantMap& stream) const;
  virtual void load(const VariantMap& stream);

  bool onlyDispatchGoods() const;
  bool isGettingFull() const;
  float tradeBuff( Buff type ) const;

  virtual std::string troubleDesc() const;

private:
  void _resolveDevastationMode();
  void _resolveDeliverMode();

  class Impl;
  ScopedPtr< Impl > _d;
};

#endif //_CAESARIA_WAREHOUSE_HPP_INCLUDE_
