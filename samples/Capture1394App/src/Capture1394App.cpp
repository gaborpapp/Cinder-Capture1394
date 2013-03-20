/*
 Copyright (C) 2013 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/params/Params.h"

#include "Capture1394.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mndl;

class Capture1394App : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		float mFps;
		bool mVerticalSyncEnabled = false;

		Capture1394Ref mCapture1394;
		gl::Texture mTexture;
};

void Capture1394App::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void Capture1394App::setup()
{
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addParam( "Vertical sync", &mVerticalSyncEnabled );

	Capture1394::getDevices();
	Capture1394::DeviceRef dref = Capture1394::findDeviceByNameContains( "FMVU-03MTM" );
	const vector< Capture1394::VideoMode > &vmodes = dref->getSupportedVideoModes();
	for ( auto &mode : vmodes )
		console() << mode << endl;
	Capture1394::Options options;
	//options.setVideoMode( vmodes[ 8 ] );
	//options.setVideoMode( vmodes[ 0 ] );
	mCapture1394 = Capture1394::create( options );
	mCapture1394->start();
}

void Capture1394App::update()
{
	mFps = getAverageFps();

	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );

	Surface8u captureSurface;
	if ( mCapture1394->getSurface( &captureSurface ) )
	{
		mTexture = gl::Texture( captureSurface );
	}
}

void Capture1394App::draw()
{
	gl::clear();

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	if ( mTexture )
	{
		gl::draw( mTexture, getWindowBounds() );
	}

	params::InterfaceGl::draw();
}

void Capture1394App::shutdown()
{
	mCapture1394->stop();
}

void Capture1394App::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( !isFullScreen() )
			{
				setFullScreen( true );
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			else
			{
				setFullScreen( false );
				showCursor();
			}
			break;

		case KeyEvent::KEY_s:
			mParams.show( !mParams.isVisible() );
			if ( isFullScreen() )
			{
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			break;

		case KeyEvent::KEY_v:
			 mVerticalSyncEnabled = !mVerticalSyncEnabled;
			 break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP_BASIC( Capture1394App, RendererGl )

