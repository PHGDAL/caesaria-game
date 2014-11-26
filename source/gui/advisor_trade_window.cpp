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

#include "advisor_trade_window.hpp"
#include "gfx/picture.hpp"
#include "gfx/decorator.hpp"
#include "core/gettext.hpp"
#include "good/goodhelper.hpp"
#include "pushbutton.hpp"
#include "label.hpp"
#include "game/resourcegroup.hpp"
#include "core/stringhelper.hpp"
#include "gfx/engine.hpp"
#include "core/gettext.hpp"
#include "groupbox.hpp"
#include "objects/factory.hpp"
#include "city/helper.hpp"
#include "city/trade_options.hpp"
#include "objects/warehouse.hpp"
#include "good/goodstore.hpp"
#include "texturedbutton.hpp"
#include "core/event.hpp"
#include "core/foreach.hpp"
#include "core/logger.hpp"
#include "image.hpp"
#include "objects/constants.hpp"
#include "empireprices.hpp"
#include "goodordermanage.hpp"
#include "widget_helper.hpp"
#include "city/statistic.hpp"

using namespace constants;
using namespace gfx;

namespace gui
{

namespace advisorwnd
{

class TradeGoodInfo : public PushButton
{
public:
  TradeGoodInfo( Widget* parent, const Rect& rect, Good::Type good, int qty, bool enable,
                 city::TradeOptions::Order trade, int tradeQty )
    : PushButton( parent, rect, "", -1, false, PushButton::noBackground )
  {
    _type = good;
    _qty = qty;
    _enable = enable;
    _tradeOrder = trade;
    _tradeQty = tradeQty;
    _goodPicture = GoodHelper::picture( _type );
    _goodName = GoodHelper::name( _type );
    Decorator::draw( _border, Rect( 50, 0, width() - 50, height() ), Decorator::brownBorder );

    setFont( Font::create( FONT_2_WHITE ) );
  }

  virtual void draw(Engine &painter)
  {
    PushButton::draw( painter );

    painter.draw( _goodPicture, absoluteRect().lefttop() + Point( 15, 0) );
    painter.draw( _goodPicture, absoluteRect().righttop() - Point( 20 + _goodPicture.width(), 0 ) );

    if( _state() == stHovered )
      painter.draw( _border, absoluteRect().lefttop(), &absoluteClippingRectRef() );
  }

  virtual void _updateTextPic()
  {
    PushButton::_updateTextPic();

    if( _textPictureRef() != 0 )
    {
      Font f = font( _state() );
      PictureRef& textPic = _textPictureRef();
      f.draw( *textPic, _( _goodName ), 55, 0, true, false );
      f.draw( *textPic, StringHelper::format( 0xff, "%d", _qty), 190, 0, true, false );
      f.draw( *textPic, _enable ? "" : _("##disable##"), 260, 0, true, false );

      std::string ruleName[] = { "##import##", "", "##export##", "##stacking##" };
      std::string tradeStateText = ruleName[ _tradeOrder ];
      switch( _tradeOrder )
      {
      case city::TradeOptions::noTrade:
      case city::TradeOptions::stacking:
      case city::TradeOptions::importing:
        tradeStateText = _( ruleName[ _tradeOrder ] );
      break;

      case city::TradeOptions::exporting:
        tradeStateText = StringHelper::format( 0xff, "%s %d", _( ruleName[ _tradeOrder ] ), _tradeQty );
      break;

      default: break;
      }
      f.draw( *textPic, tradeStateText, 340, 0, true, false );
      textPic->update();
    }
  }

  Signal1<Good::Type>& onClickedA() { return _onClickedASignal; }

protected:
  virtual void _btnClicked()
  {
    PushButton::_btnClicked();

    emit _onClickedASignal( _type );
  }

private:
  int _qty;
  bool _enable;
  city::TradeOptions::Order _tradeOrder;
  int _tradeQty;
  Good::Type _type;
  std::string _goodName;
  Picture _goodPicture;
  Pictures _border;

signals private:
  Signal1<Good::Type> _onClickedASignal;
};

class Trade::Impl
{
public:
  PushButton* btnEmpireMap;
  PushButton* btnPrices; 
  GroupBox* gbInfo;
  PlayerCityPtr city;
  city::Statistic::GoodsMap allgoods;

  bool getWorkState( Good::Type gtype );
  void updateGoodsInfo();
  void showGoodOrderManageWindow( Good::Type type );
  void showGoodsPriceWindow();
};

void Trade::Impl::updateGoodsInfo()
{
  if( !gbInfo )
    return;

  Widget::Widgets children = gbInfo->children();

  foreach( child, children ) { (*child)->deleteLater(); }

  Point startDraw( 0, 5 );
  Size btnSize( gbInfo->width(), 20 );
  city::TradeOptions& copt = city->tradeOptions();
  for( int i=Good::wheat, indexOffset=0; i < Good::goodCount; i++ )
  {
    Good::Type gtype = Good::Type( i );

    city::TradeOptions::Order tradeState = copt.getOrder( gtype );
    if( tradeState == city::TradeOptions::disabled )
    {
      continue;
    }

    bool workState = getWorkState( gtype );
    int tradeQty = copt.exportLimit( gtype );
    
    TradeGoodInfo* btn = new TradeGoodInfo( gbInfo, Rect( startDraw + Point( 0, btnSize.height()) * indexOffset, btnSize ),
                                            gtype, allgoods[ gtype ], workState, tradeState, tradeQty );
    indexOffset++;
    CONNECT( btn, onClickedA(), this, Impl::showGoodOrderManageWindow );
  }
}

bool Trade::Impl::getWorkState(Good::Type gtype )
{
  city::Helper helper( city );

  bool industryActive = false;
  FactoryList producers = helper.getProducers<Factory>( gtype );

  foreach( it, producers ) { industryActive |= (*it)->isActive(); }

  return producers.empty() ? true : industryActive;
}

void Trade::Impl::showGoodOrderManageWindow(Good::Type type )
{
  Widget* parent = gbInfo->parent();
  int gmode = GoodOrderManageWindow::gmUnknown;
  gmode |= (city::Statistic::canImport( city, type ) ? GoodOrderManageWindow::gmImport : 0);
  gmode |= (city::Statistic::canProduce( city, type ) ? GoodOrderManageWindow::gmProduce : 0);

  GoodOrderManageWindow* wnd = new GoodOrderManageWindow( parent, Rect( 50, 130, parent->width() - 45, parent->height() -60 ), 
                                                          city, type, allgoods[ type ], (GoodOrderManageWindow::GoodMode)gmode );

  CONNECT( wnd, onOrderChanged(), this, Impl::updateGoodsInfo );
}

void Trade::Impl::showGoodsPriceWindow()
{
  Widget* parent = gbInfo->parent();
  Size size( 610, 180 );
  new EmpirePricesWindow( parent, -1, Rect( Point( ( parent->width() - size.width() ) / 2,
                                                   ( parent->height() - size.height() ) / 2), size ), city );
}

Trade::Trade(PlayerCityPtr city, Widget* parent, int id )
: Window( parent, Rect( 0, 0, 640, 432 ), "", id ), _d( new Impl )
{
  setupUI( ":/gui/tradeadv.gui" );
  setPosition( Point( (parent->width() - 640 )/2, parent->height() / 2 - 242 ) );

  _d->city = city;
  _d->allgoods = city::Statistic::getGoodsMap( city, false );

  GET_DWIDGET_FROM_UI( _d, btnEmpireMap  )
  GET_DWIDGET_FROM_UI( _d, btnPrices )
  GET_DWIDGET_FROM_UI( _d, gbInfo )

  CONNECT( _d->btnEmpireMap, onClicked(), this, Trade::deleteLater );
  CONNECT( _d->btnPrices, onClicked(), _d.data(), Impl::showGoodsPriceWindow );

  _d->updateGoodsInfo();
}

void Trade::draw(gfx::Engine& painter )
{
  if( !visible() )
    return;

  Window::draw( painter );
}

Signal0<>& Trade::onEmpireMapRequest() { return _d->btnEmpireMap->onClicked(); }

}

}//end namespace gui
