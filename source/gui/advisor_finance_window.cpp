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

#include "advisor_finance_window.hpp"
#include "gfx/decorator.hpp"
#include "core/gettext.hpp"
#include "gui/pushbutton.hpp"
#include "gui/label.hpp"
#include "city/statistic.hpp"
#include "game/resourcegroup.hpp"
#include "core/stringhelper.hpp"
#include "gfx/engine.hpp"
#include "core/gettext.hpp"
#include "game/enums.hpp"
#include "objects/construction.hpp"
#include "city/helper.hpp"
#include "objects/house.hpp"
#include "core/color.hpp"
#include "gui/texturedbutton.hpp"
#include "city/funds.hpp"
#include "objects/house_level.hpp"
#include "objects/constants.hpp"
#include "core/logger.hpp"
#include "city/statistic.hpp"
#include "widget_helper.hpp"

using namespace constants;
using namespace gfx;

namespace gui
{

namespace {
  const Point startPoint( 75, 145 );
  Point offset( 0, 17 );
}

class AdvisorFinanceWindow::Impl
{
public:
  PlayerCityPtr city;

  gui::Label* lbTaxRateNow;
  TexturedButton* btnHelp;

  void updateTaxRateNowLabel();
  void decreaseTax();
  void increaseTax();
  int calculateTaxValue();
};

AdvisorFinanceWindow::AdvisorFinanceWindow(PlayerCityPtr city, Widget* parent, int id )
: Window( parent, Rect( 0, 0, 640, 420 ), "", id ), _d( new Impl )
{
  _d->city = city;
  setupUI( ":/gui/financeadv.gui" );

  setPosition( Point( (parent->width() - 640 )/2, parent->height() / 2 - 242 ) );

  Label* lbCityHave;
  GET_WIDGET_FROM_UI( lbCityHave )
  if( lbCityHave ) lbCityHave->setText( StringHelper::format( 0xff, "%s %d %s", _("##city_have##"), city->funds().treasury(), _("##denaries##") ) );

  GET_DWIDGET_FROM_UI( _d, lbTaxRateNow )
  _d->updateTaxRateNowLabel();

  unsigned int regTaxPayers = city::Statistic::getTaxPayersPercent( city );
  std::string strRegPaeyrs = StringHelper::format( 0xff, "%d%% %s", regTaxPayers, _("##population_registered_as_taxpayers##") );
  Label* lbRegPayers;
  GET_WIDGET_FROM_UI( lbRegPayers )
  if( lbRegPayers ) lbRegPayers->setText( strRegPaeyrs );

  Point sp = startPoint;
  _drawReportRow( sp, _("##taxes##"), city::Funds::taxIncome );
  _drawReportRow( sp + offset, _("##trade##"), city::Funds::exportGoods );
  _drawReportRow( sp + offset * 2, _("##donations##"), city::Funds::donation );
  _drawReportRow( sp + offset * 3, _("##debet##"), city::Funds::debet );
  
  sp += Point( 0, 6 );
  _drawReportRow( sp + offset * 4, _("##import_fn##"), city::Funds::importGoods );
  _drawReportRow( sp + offset * 5, _("##wages##"), city::Funds::workersWages );
  _drawReportRow( sp + offset * 6, _("##buildings##"), city::Funds::buildConstruction );
  _drawReportRow( sp + offset * 7, _("##percents##"), city::Funds::creditPercents );
  _drawReportRow( sp + offset * 8, _("##pn_salary##"), city::Funds::playerSalary );
  _drawReportRow( sp + offset * 9, _("##other##"), city::Funds::sundries );
  _drawReportRow( sp + offset * 10, _("##empire_tax##"), city::Funds::empireTax );
  _drawReportRow( sp + offset * 11, _("##credit##"), city::Funds::credit );

  sp += Point( 0, 6 );
  _drawReportRow( sp + offset * 12, _("##profit##"), city::Funds::cityProfit );
  
  sp += Point( 0, 6 );
  _drawReportRow( sp + offset * 13, _("##balance##"), city::Funds::balance );

  _d->btnHelp = new TexturedButton( this, Point( 12, height() - 39), Size( 24 ), -1, ResourceMenu::helpInfBtnPicId );

  TexturedButton* btnDecreaseTax = new TexturedButton( this, Point( 185, 73 ), Size( 24 ), -1, 601 );
  TexturedButton* btnIncreaseTax = new TexturedButton( this, Point( 185+24, 73 ), Size( 24 ), -1, 605 );
  CONNECT( btnDecreaseTax, onClicked(), _d.data(), Impl::decreaseTax );
  CONNECT( btnIncreaseTax, onClicked(), _d.data(), Impl::increaseTax );
}

void AdvisorFinanceWindow::draw(gfx::Engine& painter )
{
  if( !visible() )
    return;

  Window::draw( painter );

  Rect p( startPoint + absoluteRect().lefttop() + offset * 3 + Point( 200, 0 ), Size( 72, 1) );
  painter.drawLine( 0xff000000, p.lefttop(), p.righttop() );

  p = Rect( startPoint + absoluteRect().lefttop() + offset * 3 + Point( 340, 0 ), Size( 72, 1) );
  painter.drawLine( 0xff000000, p.lefttop(), p.righttop() );

  p = Rect( startPoint + absoluteRect().lefttop() + offset * 11 + Point( 200, 10 ), Size( 72, 1) );
  painter.drawLine( 0xff000000, p.lefttop(), p.righttop() );

  p =  Rect( startPoint + absoluteRect().lefttop() + offset * 11 + Point( 340, 10 ), Size( 72, 1) );
  painter.drawLine( 0xff000000, p.lefttop(), p.righttop() );
}

void AdvisorFinanceWindow::_drawReportRow(const Point& pos, const std::string& title, int type)
{
  Font font = Font::create( FONT_1 );

  int lyvalue = _d->city->funds().getIssueValue( (city::Funds::IssueType)type, city::Funds::lastYear );
  int tyvalue = _d->city->funds().getIssueValue( (city::Funds::IssueType)type, city::Funds::thisYear );

  Size size( 100, 20 );
  Label* lb = new Label( this, Rect( pos, size), title );
  lb->setFont( font );

  lb = new Label( this, Rect( pos + Point( 215, 0), size), StringHelper::format( 0xff, "%d", lyvalue ) );
  lb->setFont( font );

  lb = new Label( this, Rect( pos + Point( 355, 0), size), StringHelper::format( 0xff, "%d", tyvalue ) );
  lb->setFont( font );
}

void AdvisorFinanceWindow::Impl::updateTaxRateNowLabel()
{
  if( !lbTaxRateNow )
    return;

  int taxValue = city::Statistic::getTaxValue( city );
  std::string strCurretnTax = StringHelper::format( 0xff, "%d%% %s %d %s",
                                                    city->funds().taxRate(), _("##may_collect_about##"),
                                                    taxValue, _("##denaries##") );
  lbTaxRateNow->setText( strCurretnTax );
}

void AdvisorFinanceWindow::Impl::decreaseTax()
{
  city->funds().setTaxRate( city->funds().taxRate() - 1 );
  updateTaxRateNowLabel();
}

void AdvisorFinanceWindow::Impl::increaseTax()
{
  city->funds().setTaxRate( city->funds().taxRate() + 1 );
  updateTaxRateNowLabel();
}

}//end namespace gui
