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
		sDevices.push_back( Capture1394::DeviceRef( new Capture1394::Device( cameraList->ids[ i ].guid ) ) );
		ci::app::console() << i << " " << hex << cameraList->ids[ i ].guid << " " << sDevices.back()->getName() << endl;
	}

	sDevicesEnumerated = true;
	return sDevices;
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
