// This file is part of openCaesar3.
//
// openCaesar3 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// openCaesar3 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with openCaesar3.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __OPENCAESAR3_ENEMYSOLDIER_PREFECT_H_INCLUDED__
#define __OPENCAESAR3_ENEMYSOLDIER_PREFECT_H_INCLUDED__

#include "soldier.hpp"

class EnemySoldier : public Soldier
{
public:
  static EnemySoldierPtr create( PlayerCityPtr city );

  virtual void timeStep(const unsigned long time);

  virtual void load( const VariantMap& stream );
  virtual void save( VariantMap& stream ) const;

  virtual void send2City( TilePos pos );
  virtual void die();

  ~EnemySoldier();

protected:
  virtual void _centerTile();
  virtual void _changeTile();
  virtual void _reachedPathway();

protected:
  EnemySoldier( PlayerCityPtr city );

  bool _looks4Protestor(TilePos& pos);
  //bool _looks4Fire( ReachedBuildings& buildings, TilePos& pos );
  //bool _checkPath2NearestFire( const ReachedBuildings& buildings );
  //void _serveBuildings( ReachedBuildings& reachedBuildings );
  void _back2Prefecture();
  void _back2Patrol();
  bool _findFire();
  virtual void _brokePathway(TilePos pos);

  class Impl;
  ScopedPtr< Impl > _d;
};

#endif //__OPENCAESAR3_ENEMYSOLDIER_PREFECT_H_INCLUDED__