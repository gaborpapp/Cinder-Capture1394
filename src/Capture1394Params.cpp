/*
 Copyright (C) 2013 Gabor Papp

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

#include "Capture1394Params.h"

using namespace std;

namespace mndl {

Capture1394Params::Capture1394Params() :
	mObj( shared_ptr< Obj >( new Capture1394Params::Obj() ) )
{}

Capture1394Params::Obj::Obj() :
	mCurrentCapture( 0 )
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
	mParams = ci::params::InterfaceGl( "Capture1394", ci::Vec2i( 350, 550 ) );
	setupParams();
}

Capture1394Params::Obj::~Obj()
{
}

void Capture1394Params::Obj::setupParams()
{
	mParams.clear();
	mParams.addParam( "Capture", mDeviceNames, &mCurrentCapture );

	if ( !mCaptures[ mCurrentCapture ] )
		return;

	dc1394camera_t *camera = mCaptures[ mCurrentCapture ]->getDevice()->getNative();
	Capture1394::checkError( dc1394_feature_get_all( camera, &mFeatureSet ) );
	for ( int i = 0; i < DC1394_FEATURE_NUM; i++ )
	{
		const dc1394feature_info_t &feature = mFeatureSet.feature[ i ];
		if ( !feature.available )
			continue;

		mFeatures[ i ].mId = feature.id;
		string name = dc1394_feature_get_string( feature.id );
		mFeatures[ i ].mName = name;
		mParams.addText( name );
		if ( feature.on_off_capable )
		{
			mFeatures[ i ].mIsOn = feature.is_on;
			mParams.addParam( name + " ON/OFF", &mFeatures[ i ].mIsOn );
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
		mParams.addParam( name + " mode", modeNames, &mFeatures[ i ].mMode );

		// values
		stringstream ss;
		mFeatures[ i ].mValue = feature.value;
		ss << "min=" << feature.min << " max=" << feature.max;
		bool readonly = feature.current_mode == DC1394_FEATURE_MODE_AUTO;
		if ( feature.id == DC1394_FEATURE_WHITE_BALANCE )
		{
			mFeatures[ i ].mBUValue = feature.BU_value;
			mParams.addParam( name + " B/U", &mFeatures[ i ].mBUValue, ss.str(), readonly );
			mFeatures[ i ].mRVValue = feature.RV_value;
			mParams.addParam( name + " R/V", &mFeatures[ i ].mRVValue, ss.str(), readonly );
		}
		else
		{
			mParams.addParam( name + " value", &mFeatures[ i ].mValue, ss.str(), readonly );
		}
		mPrevFeatures[ i ] = mFeatures[ i ];
	}
}

void Capture1394Params::Obj::update()
{
	static int prevCapture = -1;

	// check active capture device
	if ( prevCapture != mCurrentCapture )
	{
		if ( ( prevCapture >= 0 ) && ( mCaptures[ prevCapture ] ) )
			mCaptures[ prevCapture ]->stop();

		if ( mCaptures[ mCurrentCapture ] )
		{
			setupParams();
			mCaptures[ mCurrentCapture ]->start();
		}

		prevCapture = mCurrentCapture;
	}

	if ( !mCaptures[ mCurrentCapture ] )
		return;

	// iterate features looking for changes, since params has no change callbacks
	dc1394camera_t *camera = mCaptures[ mCurrentCapture ]->getDevice()->getNative();
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
				mParams.setOptions( mFeatures[ i ].mName + " B/U", readonly );
				mParams.setOptions( mFeatures[ i ].mName + " R/V", readonly );
			}
			else
			{
				mParams.setOptions( mFeatures[ i ].mName + " value", readonly );
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

