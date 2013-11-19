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

#ifndef __OPENCAESAR3_GAME_PREDEFINITIONS_H_INCLUDED__
#define __OPENCAESAR3_GAME_PREDEFINITIONS_H_INCLUDED__

#include "gfx/predefinitions.hpp"
#include "building/predefinitions.hpp"
#include "walker/predefinitions.hpp"
#include "world/predefinitions.hpp"
#include "core/predefinitions.hpp"
#include "events/predefinitions.hpp"
#include "gui/predefinitions.hpp"

PREDEFINE_CLASS_SMARTPOINTER_LIST(Road,List)
PREDEFINE_CLASS_SMARTPOINTER_LIST(FishPlace,List)
PREDEFINE_CLASS_SMARTPOINTER(Player)
PREDEFINE_CLASS_SMARTPOINTER(PLayerCity)
PREDEFINE_CLASS_SMARTPOINTER(CityService)

class Tilemap;
class Pathway;

#endif