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

#ifndef _CAESARIA_INFOBOXLAND_H_INCLUDE_
#define _CAESARIA_INFOBOXLAND_H_INCLUDE_

#include "info_box.hpp"

namespace gui
{

namespace infobox
{

class AboutLand : public Simple
{
  static const int lbTextId=2;
  std::string _helpUri;
public:
  AboutLand( Widget* parent, PlayerCityPtr city, const gfx::Tile& tile );

  virtual void setText( const std::string& text );
  virtual void showDescription();
};

class AboutFreeHouse : public AboutLand
{
public:
  AboutFreeHouse( Widget* parent, PlayerCityPtr city, const gfx::Tile& tile );
  virtual void showDescription();
};

}

}//end namespace gui
#endif //_CAESARIA_INFOBOXLAND_H_INCLUDE_
