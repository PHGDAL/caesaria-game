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

#ifndef _CAESARIA_CONSTRUCTION_H_INCLUDE_
#define _CAESARIA_CONSTRUCTION_H_INCLUDE_

#include "gfx/tileoverlay.hpp"
#include "core/scopedptr.hpp"
#include "core/referencecounted.hpp"
#include "predefinitions.hpp"
#include "game/service.hpp"
#include "objects/metadata.hpp"
#include "gfx/tilesarray.hpp"

class Construction : public gfx::TileOverlay
{
public:
  typedef int ParameterType;
  typedef enum { fire=0, damage, inflammability, collapsibility, count } Param;
  Construction( const TileOverlay::Type type, const Size& size );
  virtual ~Construction();

  virtual bool canBuild( PlayerCityPtr city, TilePos pos, const gfx::TilesArray& aroundTiles ) const;  // returns true if it can be built there
  virtual std::string getError() const;
  virtual std::string troubleDesc() const;
  virtual void build( PlayerCityPtr city, const TilePos& pos );
  virtual void burn();
  virtual void collapse();

  virtual bool isNeedRoadAccess() const;
  virtual gfx::TilesArray getAccessRoads() const;  // return all road tiles adjacent to the construction
  virtual void computeAccessRoads();  
  virtual int  getRoadAccessDistance() const; // virtual because HOUSE has different behavior
  virtual gfx::TilesArray getEnterArea() const;

  virtual Desirability getDesirability() const;
  virtual bool canDestroy() const;
  virtual void destroy();

  virtual void updateState( ParameterType name, double value );
  virtual void setState( ParameterType name, double value );
  virtual double getState( ParameterType name ) const;

  virtual void timeStep(const unsigned long time);
  virtual const gfx::Picture& getPicture() const;
  virtual const gfx::Picture& getPicture( PlayerCityPtr city, TilePos pos,
                                          const gfx::TilesArray& aroundTiles ) const;

  virtual void save(VariantMap& stream) const;
  virtual void load(const VariantMap& stream);
protected:
  class Impl;
  ScopedPtr< Impl > _d;
};

#endif//_CAESARIA_CONSTRUCTION_H_INCLUDE_
