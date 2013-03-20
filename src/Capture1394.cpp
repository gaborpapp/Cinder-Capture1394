#include "Cinder/app/App.h"

#include "Capture1394.h"

using namespace std;

namespace mndl {

class ContextManager
{
	public:
		static std::shared_ptr< ContextManager > instance();
		static dc1394_t * getContext();

	private:
		static dc1394_t * sContext;
		static std::shared_ptr< ContextManager > sInstance;
};

dc1394_t * ContextManager::sContext;
shared_ptr< ContextManager > ContextManager::sInstance;

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
			throw CaptureExcInitFail();
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

		checkError( dc1394_video_set_operation_mode( camera, DC1394_OPERATION_MODE_1394B ) );

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
					float framerate;
					dc1394_framerate_as_float( framerates.framerates[ j ], &framerate );
					device->mSupportedVideoModes.push_back(
						Capture1394::VideoMode( ci::Vec2i( width, height ), videoMode, coding, framerate ) );
				}
			}
			else
			{
				// Modes corresponding for format6 and format7 do not have framerates
				device->mSupportedVideoModes.push_back( Capture1394::VideoMode( ci::Vec2i( width, height ), videoMode, coding ) );
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
					ci::Vec2i( mode.max_size_x, mode.max_size_y ), dc1394video_mode_t( DC1394_VIDEO_MODE_FORMAT7_0 + v ), mode.color_coding ) );
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
	// TODO
	if ( err )
		throw CaptureExcInitFail();
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

}; // namespace mndl
