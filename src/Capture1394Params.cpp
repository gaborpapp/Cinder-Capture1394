/*
 Copyright (C) 2013-2014 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published
 by the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <sstream>

#include "cinder/app/App.h"
#include "cinder/Xml.h"

#include "Capture1394Params.h"

using namespace std;

namespace mndl {

Capture1394Params::Capture1394Params() :
	mObj( shared_ptr< Obj >( new Capture1394Params::Obj( ci::app::App::get()->getWindow() ) ) )
{}

Capture1394Params::Capture1394Params( const ci::app::WindowRef &window ) :
	mObj( shared_ptr< Obj >( new Capture1394Params::Obj( window ) ) )
{}

void Capture1394Params::read( const ci::DataSourceRef &source )
{
	ci::XmlTree doc = ci::XmlTree( source );

	ci::XmlTree cameraId = doc.getChild( "cameraId" );
	mObj->mCurrentCapture = cameraId.getAttributeValue( "id", 0 );
	ci::XmlTree videoMode = doc.getChild( "videoMode" );
	mObj->mVideoMode = videoMode.getAttributeValue( "id", 0 );

	ci::XmlTree features = doc.getChild( "features" );
	for ( ci::XmlTree::ConstIter child = features.begin(); child != features.end(); ++child )
	{
		int id = child->getAttributeValue( "id", 0 );
		if ( id == 0 )
		{
			continue;
		}
		int i = id - DC1394_FEATURE_MIN;
		mObj->mFeatures[ i ].mId = (dc1394feature_t)id;
		mObj->mFeatures[ i ].mName = child->getAttributeValue( "name", std::string() );
		mObj->mFeatures[ i ].mIsOn = child->getAttributeValue( "isOn", false );
		mObj->mFeatures[ i ].mMode = child->getAttributeValue( "mode", 0 );
		mObj->mFeatures[ i ].mValue = child->getAttributeValue( "value", 0 );
		mObj->mFeatures[ i ].mBUValue = child->getAttributeValue( "BUValue", 0 );
		mObj->mFeatures[ i ].mRVValue = child->getAttributeValue( "RVValue", 0 );
	}
}

void Capture1394Params::write( const ci::DataTargetRef &target )
{
	ci::XmlTree doc = ci::XmlTree::createDoc();

	ci::XmlTree cameraId = ci::XmlTree( "cameraId", "" );
	cameraId.setAttribute( "id", mObj->mCurrentCapture );
	doc.push_back( cameraId );

	ci::XmlTree videoMode = ci::XmlTree( "videoMode", "" );
	videoMode.setAttribute( "id", mObj->mVideoMode );
	doc.push_back( videoMode );

	ci::XmlTree features = ci::XmlTree( "features", "" );
	for ( int i = 0; i < DC1394_FEATURE_NUM; i++ )
	{
		if ( mObj->mFeatures[ i ].mId == 0 )
		{
			continue;
		}

		ci::XmlTree feature = ci::XmlTree( "feature", "" );
		feature.setAttribute( "id", mObj->mFeatures[ i ].mId );
		feature.setAttribute( "name", mObj->mFeatures[ i ].mName );
		feature.setAttribute( "isOn", mObj->mFeatures[ i ].mIsOn );
		feature.setAttribute( "mode", mObj->mFeatures[ i ].mMode );
		feature.setAttribute( "value", mObj->mFeatures[ i ].mValue );
		feature.setAttribute( "BUValue", mObj->mFeatures[ i ].mBUValue );
		feature.setAttribute( "RVValue", mObj->mFeatures[ i ].mRVValue );

		features.push_back( feature );
	}
	doc.push_back( features );
	doc.write( target );
}

Capture1394Params::Obj::Obj( const ci::app::WindowRef &window ) :
	mCurrentCapture( 0 ), mVideoMode( 0 )
{
	const vector< Capture1394::DeviceRef > &devices = Capture1394::getDevices();
	for ( auto it = devices.cbegin(); it != devices.end(); ++it )
	{
		Capture1394::DeviceRef device = *it;

		try
		{
			mCaptures.push_back( Capture1394::create( Capture1394::Options(), device ) );
			mDeviceNames.push_back( device->getName() );
		}
		catch ( const Capture1394Exc &exc )
		{
			ci::app::console() << "Unable to initialize device: " << device->getName() << " " << exc.what() << endl;
			ci::app::App::get()->quit();
		}
	}

    if ( devices.empty() )
    {
        mCaptures.push_back( Capture1394Ref() );
		mDeviceNames.push_back( "Camera not available" );
	}
	mParams = ci::params::InterfaceGl::create( window, "Capture1394", ci::Vec2i( 350, 550 ) );
	setupParams();
}

Capture1394Params::Obj::~Obj()
{
}

void Capture1394Params::Obj::setupParams()
{
	mParams->clear();
	mParams->addParam( "Capture", mDeviceNames, &mCurrentCapture );

	if ( !mCaptures[ mCurrentCapture ] )
		return;

	// video modes
	vector< string > videoModeNames;
	const vector< Capture1394::VideoMode > &videoModes = mCaptures[ mCurrentCapture ]->getDevice()->getSupportedVideoModes();
	for ( auto it = videoModes.cbegin(); it != videoModes.cend(); ++it )
	{
		stringstream videoMode;
		videoMode << *it;
		videoModeNames.push_back( videoMode.str() );
	}
	mParams->addParam( "Video modes", videoModeNames, &mVideoMode );
	if ( mVideoMode >= videoModeNames.size() )
	{
		mVideoMode = 0;
	}

	// features
	dc1394camera_t *camera = mCaptures[ mCurrentCapture ]->getDevice()->getNative();
	Capture1394::checkError( dc1394_feature_get_all( camera, &mFeatureSet ) );
	for ( int i = 0; i < DC1394_FEATURE_NUM; i++ )
	{
		const dc1394feature_info_t &feature = mFeatureSet.feature[ i ];
		if ( !feature.available )
			continue;

		// TODO: support trigger
		if ( ( feature.id == DC1394_FEATURE_TRIGGER ) ||
			 ( feature.id == DC1394_FEATURE_TRIGGER_DELAY ) )
			continue;

		mFeatures[ i ].mId = feature.id;
		string name = dc1394_feature_get_string( feature.id );
		mFeatures[ i ].mName = name;
		mParams->addText( name );
		if ( feature.on_off_capable )
		{
			mFeatures[ i ].mIsOn = feature.is_on;
			mParams->addParam( name + " ON/OFF", &mFeatures[ i ].mIsOn );
		}

		// modes
		vector< string > modeNames;
		string modes[] = { "manual", "auto", "one push auto" };
		for ( int j = 0; j < feature.modes.num; j++ )
		{
			dc1394feature_mode_t mode = feature.modes.modes[ j ];
			modeNames.push_back( modes[ mode - DC1394_FEATURE_MODE_MIN ] );
			if ( mode == feature.current_mode )
				mFeatures[ i ].mMode = j;
		}
		mParams->addParam( name + " mode", modeNames, &mFeatures[ i ].mMode );

		// values
		mFeatures[ i ].mValue = feature.value;
		bool readonly = feature.current_mode == DC1394_FEATURE_MODE_AUTO;
		if ( feature.id == DC1394_FEATURE_WHITE_BALANCE )
		{
			mFeatures[ i ].mBUValue = feature.BU_value;
			mParams->addParam( name + " B/U", &mFeatures[ i ].mBUValue, readonly ).min( feature.min ).max( feature.max );
			mFeatures[ i ].mRVValue = feature.RV_value;
			mParams->addParam( name + " R/V", &mFeatures[ i ].mRVValue, readonly ).min( feature.min ).max( feature.max );
		}
		else
		{
			mParams->addParam( name + " value", &mFeatures[ i ].mValue ).min( feature.min ).max( feature.max );
		}
		mPrevFeatures[ i ] = mFeatures[ i ];
	}
}

void Capture1394Params::Obj::update()
{
	static int prevCapture = -1;
	static int prevVideoMode = 0;

	// check active capture device
	if ( prevCapture != mCurrentCapture )
	{
		if ( ( prevCapture >= 0 ) && mCaptures[ prevCapture ] && mCaptures[ prevCapture ]->isCapturing() )
		{
			mCaptures[ prevCapture ]->stop();
		}

		if ( mCaptures[ mCurrentCapture ] && ( ! mCaptures[ mCurrentCapture ]->isCapturing() ) )
		{
			setupParams();
			mCaptures[ mCurrentCapture ]->start();
		}

		prevCapture = mCurrentCapture;
	}

	if ( !mCaptures[ mCurrentCapture ] )
		return;

	dc1394camera_t *camera = mCaptures[ mCurrentCapture ]->getDevice()->getNative();

	// video mode
	if ( prevVideoMode != mVideoMode )
	{
		const vector< Capture1394::VideoMode > &videoModes = mCaptures[ mCurrentCapture ]->getDevice()->getSupportedVideoModes();
		mCaptures[ mCurrentCapture ]->setVideoMode( videoModes[ mVideoMode ] );
		prevVideoMode = mVideoMode;
	}

	// iterate features looking for changes, since params has no change callbacks
	for ( int i = 0; i < DC1394_FEATURE_NUM; i++ )
	{
		if ( mFeatures[ i ] == mPrevFeatures[ i ] )
			continue;

		if ( mFeatures[ i ].mIsOn != mPrevFeatures[ i ].mIsOn )
		{
			Capture1394::checkError( dc1394_feature_set_power( camera,
						mFeatures[ i ].mId, static_cast< dc1394switch_t >( mFeatures[ i ].mIsOn ) ) );
			mPrevFeatures[ i ].mIsOn = mFeatures[ i ].mIsOn;
		}
		if ( mFeatures[ i ].mMode != mPrevFeatures[ i ].mMode )
		{
			dc1394feature_mode_t mode = mFeatureSet.feature[ i ].modes.modes[ mFeatures[ i ].mMode ];
			Capture1394::checkError( dc1394_feature_set_mode( camera,
						mFeatures[ i ].mId, mode ) );
			string readonly = ( mode == DC1394_FEATURE_MODE_AUTO ) ? "readonly=true" : "readonly=false";
			if ( mFeatures[ i ].mId == DC1394_FEATURE_WHITE_BALANCE )
			{
				mParams->setOptions( mFeatures[ i ].mName + " B/U", readonly );
				mParams->setOptions( mFeatures[ i ].mName + " R/V", readonly );
			}
			else
			{
				mParams->setOptions( mFeatures[ i ].mName + " value", readonly );
			}
			mPrevFeatures[ i ].mMode = mFeatures[ i ].mMode;
		}
		if ( mFeatures[ i ].mValue != mPrevFeatures[ i ].mValue )
		{
			Capture1394::checkError( dc1394_feature_set_value( camera,
						mFeatures[ i ].mId, static_cast< uint32_t >( mFeatures[ i ].mValue ) ) );
			mPrevFeatures[ i ].mValue = mFeatures[ i ].mValue;
		}
		if ( ( mFeatures[ i ].mBUValue != mPrevFeatures[ i ].mBUValue ) ||
			 ( mFeatures[ i ].mRVValue != mPrevFeatures[ i ].mRVValue ) )
		{
			Capture1394::checkError( dc1394_feature_whitebalance_set_value( camera,
					static_cast< uint32_t >( mFeatures[ i ].mBUValue ),
					static_cast< uint32_t >( mFeatures[ i ].mRVValue ) ) );
			mPrevFeatures[ i ].mBUValue = mFeatures[ i ].mBUValue;
			mPrevFeatures[ i ].mRVValue = mFeatures[ i ].mRVValue;
		}
	}
}

} // namespace mndl

