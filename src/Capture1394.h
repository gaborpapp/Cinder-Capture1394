#pragma once

#include <string>
#include <vector>

#include "cinder/Cinder.h"
#include "cinder/Exception.h"
#include "cinder/Surface.h"

#include <dc1394/dc1394.h>
#include <dc1394/capture.h>

namespace mndl {

typedef std::shared_ptr< class Capture1394 >  Capture1394Ref;

class Capture1394
{
	public:
		class Device;
		typedef std::shared_ptr< Device > DeviceRef;
		typedef uint64_t DeviceIdentifier;

		//! Returns a vector of all Devices connected to the system. If \a forceRefresh then the system will be polled for connected devices.
		static const std::vector< DeviceRef > & getDevices( bool forceRefresh = false );

		class Device
		{
			public:
				Device( uint64_t guid );
				~Device();

				//! Returns the human-readable name of the device.
				const std::string getName() const { return mCamera->model; }
				//! Returns whether the device is available for use.
				bool checkAvailable();
				//! Returns whether the device is currently connected.
				bool isConnected();
				//! Returns the unique identifier.
				DeviceIdentifier getUniqueId() { return mCamera->guid; };

				//! Returns a pointer to the libdc1394 device.
				dc1394camera_t * getNative();

			protected:
				Device() {}
				dc1394camera_t *mCamera;
		};

	protected:
		static void checkError( dc1394error_t err );

		// this maintains a reference to the context manager so that we don't destroy
		// it before the last Capture1394 is destroyed
		std::shared_ptr< class ContextManager > mContextManagerPtr;

#if 0
		struct Obj {
			Obj( int32_t width, int32_t height, const Capture1394::DeviceRef device );
			virtual ~Obj();

		};

		std::shared_ptr<Obj> mObj;

	public:
		//@{
		//! Emulates shared_ptr-like behavior
		typedef std::shared_ptr<Obj> Capture::*unspecified_bool_type;
		operator unspecified_bool_type() const { return ( mObj.get() == 0 ) ? 0 : &Capture::mObj; }
		void reset() { mObj.reset(); }
		//@}
#endif
		static bool sDevicesEnumerated;
		static std::vector< Capture1394::DeviceRef > sDevices;
};

class CaptureExc : public ci::Exception {};
class CaptureExcInitFail : public CaptureExc {};

}; // namespace mndl

