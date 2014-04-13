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

#include "ceres.hpp"
#include "game/gamedate.hpp"
#include "objects/farm.hpp"
#include "events/showinfobox.hpp"
#include "objects/extension.hpp"
#include "core/gettext.hpp"
#include "city/city.hpp"

using namespace constants;
using namespace gfx;

namespace religion
{

namespace rome
{

DivinityPtr Ceres::create()
{
  DivinityPtr ret( new Ceres() );
  ret->setInternalName( baseDivinityNames[ romeDivCeres ] );
  ret->drop();

  return ret;
}

void Ceres::updateRelation(float income, PlayerCityPtr city)
{
  RomeDivinity::updateRelation( income, city );

  int monthFromLastAction = _lastActionDate.monthsTo( GameDate::current() );
  if( relation() < 1 && monthFromLastAction > 10 )
  {
    _lastActionDate = GameDate::current();
    _wrath( city );
  }

  if( relation() > 98 && monthFromLastAction > 8 )
  {
    _lastActionDate = GameDate::current();

    _blessing( city );
  }
}

void Ceres::_wrath( PlayerCityPtr city )
{
  events::GameEventPtr event = events::ShowInfobox::create( _("##wrath_of_ceres_title##"),
                                                            _("##wrath_of_ceres_description##"),
                                                            events::ShowInfobox::send2scribe );
  event->dispatch();

  FarmList farms;
  farms << city->overlays();

  foreach( farm, farms )
  {
    (*farm)->updateProgress( -(*farm)->getProgress() );
  }
}

void Ceres::_blessing(PlayerCityPtr city)
{
  events::GameEventPtr event = events::ShowInfobox::create( _("##blessing_of_ceres_title##"),
                                                            _("##blessing_of_ceres_description##") );
  event->dispatch();

  FarmList farms;
  farms << city->overlays();

  foreach( farm, farms )
  {
    FactoryProgressUpdater::assignTo( ptr_cast<Factory>( *farm ), 0.1, GameDate::days2ticks( 60 ) );
  }
}

}//end namespace rome

}//end namespace religion
