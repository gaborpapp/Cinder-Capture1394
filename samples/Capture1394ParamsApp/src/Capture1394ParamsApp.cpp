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

#include "Capture1394Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mndl;

class Capture1394ParamsApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		bool mVerticalSyncEnabled = false;

		Capture1394ParamsRef mCapture1394Params;
		gl::Texture mTexture;
};

void Capture1394ParamsApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void Capture1394ParamsApp::setup()
{
	setFrameRate( 1000 );
	mCapture1394Params = Capture1394Params::create();
}

void Capture1394ParamsApp::update()
{
	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );

	mCapture1394Params->update();

	const Capture1394Ref captureRef = mCapture1394Params->getCurrentCaptureRef();
	if ( captureRef && captureRef->checkNewFrame() )
	{
		mTexture = gl::Texture( captureRef->getSurface() );
	}
}

void Capture1394ParamsApp::draw()
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

void Capture1394ParamsApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			setFullScreen( !isFullScreen() );
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

CINDER_APP_BASIC( Capture1394ParamsApp, RendererGl )

