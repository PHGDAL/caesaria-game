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

#ifndef __OPENCAESAR3_ALARM_EVENT_HOLDER_H_INCLUDED__
#define __OPENCAESAR3_ALARM_EVENT_HOLDER_H_INCLUDED__

#include "core/scopedptr.hpp"
#include "core/position.hpp"
#include "core/signals.hpp"

#include <string>

class AlarmEventHolder
{
public:
  AlarmEventHolder();

  ~AlarmEventHolder();

  void add( const TilePos& pos, const std::string& message );
  void next();
  
  bool haveAlarms() const;
  TilePos getCurrentPos() const;
  std::string getCurrentMessage() const;

oc3_signals public:
  Signal1<bool>& onAlarmChange();
  Signal1<const TilePos& >& onMoveToAlarm();

private:
  class Impl;
  ScopedPtr< Impl > _d;
};

#endif //__OPENCAESAR3_ALARM_EVENT_HOLDER_H_INCLUDED__