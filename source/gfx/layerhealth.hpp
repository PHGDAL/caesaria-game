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

#ifndef __CAESARIA_LAYERHEALTH_H_INCLUDED__
#define __CAESARIA_LAYERHEALTH_H_INCLUDED__

#include "layerinfo.hpp"

namespace gfx
{

class LayerHealth : public LayerInfo
{
public:
  virtual int type() const;
  virtual void drawTile( Engine& engine, Tile& tile, const Point& offset );

  static LayerPtr create( TilemapCamera& camera, PlayerCityPtr city, int type );
  virtual void handleEvent(NEvent& event);

private:
  LayerHealth(Camera& camera, PlayerCityPtr city, int type );
  int _getLevelValue(HousePtr house);

  std::set<int> _flags;
  int _type;
};

}//end namespace gfx

#endif //__CAESARIA_LAYERHEALTH_H_INCLUDED__
