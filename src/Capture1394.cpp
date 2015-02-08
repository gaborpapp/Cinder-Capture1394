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

#include <cstring>

#include "Cinder/app/App.h"

#include <dc1394/capture.h>

#include "SurfaceCache.h"
#include "Capture1394.h"

using namespace std;

namespace mndl {

class ContextManager
{
	public:
		~ContextManager();

		static std::shared_ptr< ContextManager > instance();
		static dc1394_t * getContext();

	private:
		static dc1394_t * sContext;
		static std::shared_ptr< ContextManager > sInstance;
};

dc1394_t *ContextManager::sContext = NULL;
shared_ptr< ContextManager > ContextManager::sInstance;

ContextManager::~ContextManager()
{
	if ( sContext )
		dc1394_free( sContext );
}

shared_ptr< ContextManager > ContextManager::instance()
{
	if ( !sInstance )
		sInstance = shared_ptr< ContextManager >( new ContextManager );
	return sInstance;
}

dc1394_t * ContextManager::getContext()
{
	if ( !sContext )
	{
		sContext = dc1394_new();
		if ( sContext == NULL )
			throw Capture1394Exc( "Failed to initialize libdc1394." );
	}
	return sContext;
}

bool Capture1394::sDevicesEnumerated = false;
vector< Capture1394::DeviceRef > Capture1394::sDevices;

const vector< Capture1394::DeviceRef > & Capture1394::getDevices( bool forceRefresh )
{
	if ( sDevicesEnumerated && ( !forceRefresh ) )
		return sDevices;

	sDevices.clear();

	dc1394camera_list_t *cameraList;
	dc1394_t *context = ContextManager::instance()->getContext();
	checkError( dc1394_camera_enumerate ( context, &cameraList ) );

	for ( uint32_t i = 0; i < cameraList->num; i++ )
	{
		Capture1394::DeviceRef device = Capture1394::DeviceRef( new Capture1394::Device( cameraList->ids[ i ].guid ) );
		dc1394camera_t *camera = device->getNative();

		dc1394video_modes_t videoModes;
		checkError( dc1394_video_get_supported_modes( camera, &videoModes) );

		for ( int v = 0; v < videoModes.num; v++ )
		{
			dc1394video_mode_t videoMode = videoModes.modes[ v ];
			dc1394color_coding_t coding;
			dc1394_get_color_coding_from_video_mode( camera, videoMode, &coding );
			unsigned width, height;
			dc1394_get_image_size_from_video_mode( camera, videoMode, &width, &height );

			if ( !dc1394_is_video_mode_scalable( videoMode ) )
			{
				dc1394framerates_t framerates;
				checkError( dc1394_video_get_supported_framerates( camera, videoMode, &framerates ) );
				for ( int j = framerates.num - 1; j >= 0; j-- )
				{
					device->mSupportedVideoModes.push_back(
						Capture1394::VideoMode( ci::ivec2( width, height ), videoMode, coding,
							framerates.framerates[ j ] ) );
				}
			}
			else
			{
				// Modes corresponding for format6 and format7 do not have framerates
				device->mSupportedVideoModes.push_back( Capture1394::VideoMode( ci::ivec2( width, height ), videoMode, coding ) );
			}
		}

		// format 7
		dc1394format7modeset_t format7Modes;
		checkError( dc1394_format7_get_modeset( camera, &format7Modes ) );
		for ( int v = 0; v < DC1394_VIDEO_MODE_FORMAT7_NUM; v++ )
		{
			dc1394format7mode_t mode = format7Modes.mode[ v ];
			if ( ! mode.present )
				continue;

			device->mSupportedVideoModes.push_back(
				Capture1394::VideoMode(
					ci::ivec2( mode.max_size_x, mode.max_size_y ), dc1394video_mode_t( DC1394_VIDEO_MODE_FORMAT7_0 + v ), mode.color_coding ) );
		}

		sDevices.push_back( device );
	}

	sDevicesEnumerated = true;
	return sDevices;
}

Capture1394::DeviceRef Capture1394::findDeviceByName( const string &name )
{
	const vector< DeviceRef > &devices = getDevices();
	for ( auto deviceIt = devices.cbegin(); deviceIt != devices.cend(); ++deviceIt )
	{
		if ( ( *deviceIt )->getName() == name )
			return *deviceIt;
	}

	return DeviceRef(); // failed - return "null" device
}

Capture1394::DeviceRef Capture1394::findDeviceByNameContains( const string &nameFragment )
{
	const vector< DeviceRef > &devices = getDevices();
	for ( auto deviceIt = devices.cbegin(); deviceIt != devices.cend(); ++deviceIt )
	{
		if ( ( *deviceIt )->getName().find( nameFragment ) != std::string::npos )
			return *deviceIt;
	}

	return DeviceRef(); // failed - return "null" device
}

void Capture1394::checkError( dc1394error_t err )
{
	if ( err )
		throw Capture1394Exc( err );
}

Capture1394::Device::Device( uint64_t guid )
{
	dc1394_t *context = ContextManager::instance()->getContext();
	mCamera = dc1394_camera_new( context, guid );
}

Capture1394::Device::~Device()
{
	dc1394_video_set_transmission( mCamera, DC1394_OFF );
	dc1394_capture_stop( mCamera );
	dc1394_camera_free( mCamera );
}

Capture1394::Capture1394( const Options &options, const DeviceRef device ) :
	mObj( shared_ptr< Obj >( new Obj( options, device ) ) )
{}

Capture1394::Obj::Obj( const Options &options, const Capture1394::DeviceRef device ) :
	mOptions( options ), mDevice( device ), mHasNewFrame( false ),
	mIsCapturing( false )
{
	if ( !device )
	{
		mDevice = Capture1394::getDevices()[ 0 ];
	}
	dc1394camera_t *camera = mDevice->getNative();

	if ( mOptions.getVideoMode().getAutoVideoMode() )
	{
		Capture1394::VideoMode videoMode = mDevice->getSupportedVideoModes()[ 0 ];
		mOptions.setVideoMode( videoMode );
	}

	// allocate surface cache for the maximum resolution
	auto videoModes = mDevice->getSupportedVideoModes();
	ci::ivec2 maxRes( 0, 0 );
	for ( auto it = videoModes.cbegin(); it != videoModes.cend(); ++it )
	{
		ci::ivec2 res = it->getResolution();
		if ( maxRes.x * maxRes.y < res.x * res.y )
			maxRes = res;
	}
	mSurfaceCache = std::shared_ptr< SurfaceCache >( new SurfaceCache( maxRes.x, maxRes.y, ci::SurfaceChannelOrder::RGB, 8 ) );

	Capture1394::checkError( dc1394_video_set_operation_mode( camera, mOptions.getOperationMode() ) );

	Capture1394::checkError( dc1394_video_set_iso_speed( camera, DC1394_ISO_SPEED_400 ) );
	setVideoMode( mOptions.getVideoMode() );
}

Capture1394::Obj::~Obj()
{
	if ( mIsCapturing )
		stop();
}

void Capture1394::Obj::setVideoMode( const VideoMode &videoMode )
{
	bool wasCapturing = mIsCapturing;
	if ( mIsCapturing )
		stop();
	{
		lock_guard< mutex > lock( mMutex );
		mOptions.setVideoMode( videoMode );

		mWidth = mOptions.getVideoMode().getResolution().x;
		mHeight = mOptions.getVideoMode().getResolution().y;
		mSurfaceCache->resize( mWidth, mHeight );

		dc1394video_mode_t dcVideoMode = mOptions.getVideoMode().getVideoMode();
		if ( ( dcVideoMode < DC1394_VIDEO_MODE_FORMAT7_MIN ) || ( DC1394_VIDEO_MODE_FORMAT7_MAX < dcVideoMode ) )
		{
			Capture1394::checkError( dc1394_video_set_framerate( mDevice->getNative(), mOptions.getVideoMode().getFrameRate() ) );
		}
		else
		{
			Capture1394::checkError( dc1394_format7_set_roi( mDevice->getNative(),
						dcVideoMode,
						mOptions.getVideoMode().getColorCoding(),
						DC1394_USE_MAX_AVAIL,
						0, 0, mWidth, mHeight ) );
		}

		Capture1394::checkError( dc1394_video_set_mode( mDevice->getNative(), dcVideoMode ) );
		mHasNewFrame = false;
	}
	if ( wasCapturing )
		start();
}

void Capture1394::Obj::start()
{
	Capture1394::checkError( dc1394_video_set_transmission( mDevice->getNative(), DC1394_ON ) );
	Capture1394::checkError( dc1394_capture_setup( mDevice->getNative(), 8, DC1394_CAPTURE_FLAGS_DEFAULT ) );
	mThreadShouldQuit = false;
	mThread = shared_ptr< thread >( new thread( bind( &Capture1394::Obj::threadedFunc, this ) ) );
	mHasNewFrame = false;
	mIsCapturing = true;
}

void Capture1394::Obj::stop()
{
	if ( mThread )
	{
		mThreadShouldQuit = true;
		mThread->join();
		mThread.reset();
	}
	Capture1394::checkError( dc1394_video_set_transmission( mDevice->getNative(), DC1394_OFF ) );
	Capture1394::checkError( dc1394_capture_stop( mDevice->getNative() ) );
	mIsCapturing = false;
}

void Capture1394::Obj::threadedFunc()
{
	dc1394camera_t *camera = mDevice->getNative();
	dc1394video_frame_t *frame = NULL;

	while ( !mThreadShouldQuit )
	{
		// drain all frames
		if ( mOptions.getDiscardFrames() )
		{
			bool empty = false;
			while ( !empty )
			{
				if ( dc1394_capture_dequeue( camera, DC1394_CAPTURE_POLICY_POLL, &frame ) == DC1394_SUCCESS )
				{
					if ( frame != NULL )
						dc1394_capture_enqueue( camera, frame );
					else
						empty = true;
				}
				else
				{
					empty = true;
				}
			}
		}

		Capture1394::checkError( dc1394_capture_dequeue( camera, DC1394_CAPTURE_POLICY_WAIT, &frame ) );

		if ( frame )
		{
			if ( !dc1394_capture_is_frame_corrupt( camera, frame ) )
			{
				lock_guard< mutex > lock( mMutex );
				ci::Surface8uRef surface = mSurfaceCache->getNewSurface();
				dc1394video_mode_t videoMode = mOptions.getVideoMode().getVideoMode();
				if ( ( DC1394_VIDEO_MODE_FORMAT7_MIN <= videoMode ) &&
						( videoMode <= DC1394_VIDEO_MODE_FORMAT7_MAX ) )
				{
					Capture1394::checkError( dc1394_bayer_decoding_8bit(
								frame->image, surface->getData(), mWidth, mHeight,
								DC1394_COLOR_FILTER_RGGB, DC1394_BAYER_METHOD_BILINEAR ) );
				}
				else
				{
					dc1394color_coding_t colorCoding = mOptions.getVideoMode().getColorCoding();
					uint32_t bits; // TODO: is this calculation correct?
					switch ( colorCoding )
					{
						case DC1394_COLOR_CODING_RGB16:
						case DC1394_COLOR_CODING_MONO16:
						case DC1394_COLOR_CODING_RAW16:
							bits = 16;
							break;

						default:
							bits = 8;
							break;
					}
					Capture1394::checkError( dc1394_convert_to_RGB8(
								frame->image, surface->getData(), mWidth, mHeight,
								frame->yuv_byte_order, colorCoding, bits ) );
				}
				mCurrentSurface = surface;
				mHasNewFrame = true;
			}
			Capture1394::checkError( dc1394_capture_enqueue( camera, frame ) );
		}
	}
}

bool Capture1394::Obj::checkNewFrame() const
{
	lock_guard< mutex > lock( mMutex );
	return mHasNewFrame;
}

ci::Surface8uRef Capture1394::Obj::getSurface() const
{
	lock_guard< mutex > lock( mMutex );
	mHasNewFrame = false;
	return mCurrentSurface;
}

Capture1394Exc::Capture1394Exc( dc1394error_t err ) throw()
{
	strcpy( mMessage, "Capture1394: " );
	strncat( mMessage, dc1394_error_get_string( err ), 110 );
}

Capture1394Exc::Capture1394Exc( const string &log ) throw()
{
	strcpy( mMessage, "Capture1394: " );
	strncat( mMessage, log.c_str(), 110 );
}

}; // namespace mndl
