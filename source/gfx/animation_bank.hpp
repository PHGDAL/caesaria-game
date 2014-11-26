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

#ifndef __CAESARIA_ANIMATION_BANK_H_INCLUDED__
#define __CAESARIA_ANIMATION_BANK_H_INCLUDED__

#include "animation.hpp"
#include "walker/action.hpp"
#include "good/good.hpp"
#include "core/direction.hpp"
#include "vfs/path.hpp"

#include <map>

namespace gfx
{

class AnimationBank
{
public:
  typedef enum {
    simpleCart=100,
    bigCart = 200,
    megaCart = 300,
    imigrantCart = 400,
    circusCart = 500
  } CartCapacity;

  typedef std::map< DirectedAction, Animation > MovementAnimation;

  static AnimationBank& instance();

  // loads all cart graphics
  void loadCarts();

  void loadAnimation( vfs::Path model );

  static const Picture& getCart(int good, int capacity, constants::Direction direction );

  static const MovementAnimation& find( int type );
private:
  AnimationBank();

  class Impl;
  ScopedPtr< Impl > _d;
};

}//end namespace gfx

#endif  //__CAESARIA_ANIMATION_BANK_H_INCLUDED__
