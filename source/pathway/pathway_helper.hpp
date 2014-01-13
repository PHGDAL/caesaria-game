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

#ifndef __CAESARIA_PATHWAYHELPER_H_INCLUDED__
#define __CAESARIA_PATHWAYHELPER_H_INCLUDED__

#include "pathway.hpp"
#include "predefinitions.hpp"
#include "city/predefinitions.hpp"
#include "objects/predefinitions.hpp"

class PathwayHelper
{
public:
  typedef enum { roadOnly=0, allTerrain, water, roadFirst } WayType;
  static Pathway create(TilePos startPos, TilePos stopPos,
                        WayType type=roadOnly );

  static Pathway create(TilePos startPos, ConstructionPtr construction,
                        WayType type);

  static Pathway create(TilePos statrPos, TilePos stopPos,
                        const TilePossibleCondition& condition );

  static Pathway randomWay( PlayerCityPtr city, TilePos startPos, int walkRadius );
};

#endif