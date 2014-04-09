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

#include "layer.hpp"

namespace gfx
{

class LayerHealth : public Layer
{
public:
  virtual int getType() const;
  virtual VisibleWalkers getVisibleWalkers() const;
  virtual void drawTile( Engine& engine, Tile& tile, Point offset );

  static LayerPtr create( TilemapCamera& camera, PlayerCityPtr city, int type );
  virtual void handleEvent(NEvent& event);

private:
  LayerHealth(Camera& camera, PlayerCityPtr city, int type );
  int _getLevelValue(HousePtr house);

  std::set<int> _flags;
  std::set<int> _walkers;
  int _type;
};

}//end namespace gfx

#endif //__CAESARIA_LAYERHEALTH_H_INCLUDED__
