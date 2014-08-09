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
// Copyright 2012-2014 Dalerank, dalerankn8@gmail.com


#ifndef GFX_GL_ENGINE_HPP
#define GFX_GL_ENGINE_HPP

#include "engine.hpp"
#include "picture.hpp"
#include "vfs/path.hpp"
#include "core/predefinitions.hpp"
#include "core/smartlist.hpp"
#include "core/variant.hpp"

// This is the OpenGL engine
// It does a dumb drawing from back to front, in a 2D projection, with no depth buffer
namespace gfx
{

//#if defined(CAESARIA_PLATFORM_ANDROID) || defined(CAESARIA_PLATFORM_MACOSX) || defined(CAESARIA_PLATFORM_HAIKU)
#if 1
#define GlEngine SdlEngine
#else

PREDEFINE_CLASS_SMARTLIST(PostprocFilter,List)
typedef PostprocFilterList Effects;

class PostprocFilter : public ReferenceCounted
{
public:
  static PostprocFilterPtr create();
  void setVariables( const VariantMap& variables );
  void loadProgramm( vfs::Path fragmentShader );

  void setUniformVar( const std::string& name, const Variant& var );

  void begin();
  void bindTexture();
  void end();

private:
  PostprocFilter();
  void _log( unsigned int program );
  unsigned int _program;
  unsigned int _vertexShader, _fragmentShader;
  VariantMap _variables;
};

class EffectManager : public ReferenceCounted
{
public:
  EffectManager();
  void load( vfs::Path effectModel );

  Effects& effects();

private:
  Effects _effects;
};

class FrameBuffer : public ReferenceCounted
{
public:
  FrameBuffer();
  void initialize( Size size );

  void begin();
  void end();

  void draw();
  void draw( Effects& effects );

  bool isOk() const;

private:
  void _createFramebuffer( unsigned int& id );

  unsigned int _framebuffer, _framebuffer2;
  Size _size;
  bool _isOk;
};

class GlEngine : public Engine
{
public:
  GlEngine();
  virtual ~GlEngine();
  void init();
  void exit();
  void delay( const unsigned int msec );

  Picture* createPicture(const Size& size);
  virtual void loadPicture(Picture &ioPicture);
  virtual void unloadPicture(Picture &ioPicture);
  void deletePicture(Picture* pic);

  void startRenderFrame();
  void draw(const Picture &picture, const int dx, const int dy, Rect* clipRect=0);
  void draw(const Picture &picture, const Point& pos, Rect* clipRect=0 );
  void draw(const Pictures& pictures, const Point& pos, Rect* clipRect);
  void endRenderFrame();

  void setColorMask( int rmask, int gmask, int bmask, int amask );
  void resetColorMask();

  void createScreenshot( const std::string& filename );
  unsigned int fps() const;
  bool haveEvent( NEvent& event );

  Modes modes() const;

  Point cursorPos() const;
  Picture& screen();
private:
  void _createFramebuffer( unsigned int& id );
  void _initShaderProgramm(const char* vertSrc, const char* fragSrc,
                           unsigned int& vertexShader, unsigned int& fragmentShader, unsigned int& shaderProgram);

  class Impl;
  ScopedPtr<Impl> _d;

  Picture _screen;
  unsigned int _fps, _lastUpdateFps, _lastFps, _drawCall;
  float _rmask, _gmask, _bmask, _amask;  
};
#endif //#ifdef CAESARIA_PLATFORM_MACOSX

}
#endif