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

		//! Class for describing supported video modes by the device.
		class VideoMode
		{
			public:
				//! Default constructor for choosing the video mode automatically.
				VideoMode() : mAutoVideoMode( true ) {}

				VideoMode( const ci::Vec2i &res, dc1394video_mode_t videoMode,
						   dc1394color_coding_t coding, float frameRate = 0.f ) :
					mResolution( res ), mVideoMode( videoMode ), mColorCoding( coding ),
					mFrameRate( frameRate ), mAutoVideoMode( false ) {}

				VideoMode & resolution( const ci::Vec2i &resolution ) { mResolution = resolution; return *this; }
				void setResolution( const ci::Vec2i &resolution ) { mResolution = resolution; }
				const ci::Vec2i & getResolution() const { return mResolution; }

				/** \a videoMode is one of DC1394_VIDEO_MODE_160x120_YUV444, DC1394_VIDEO_MODE_320x240_YUV422,
				  DC1394_VIDEO_MODE_640x480_YUV411, DC1394_VIDEO_MODE_640x480_YUV422, DC1394_VIDEO_MODE_640x480_RGB8,
				  DC1394_VIDEO_MODE_640x480_MONO8, DC1394_VIDEO_MODE_640x480_MONO16, DC1394_VIDEO_MODE_800x600_YUV422,
				  DC1394_VIDEO_MODE_800x600_RGB8, DC1394_VIDEO_MODE_800x600_MONO8, DC1394_VIDEO_MODE_1024x768_YUV422,
				  DC1394_VIDEO_MODE_1024x768_RGB8, DC1394_VIDEO_MODE_1024x768_MONO8, DC1394_VIDEO_MODE_800x600_MONO16,
				  DC1394_VIDEO_MODE_1024x768_MONO16, DC1394_VIDEO_MODE_1280x960_YUV422, DC1394_VIDEO_MODE_1280x960_RGB8,
				  DC1394_VIDEO_MODE_1280x960_MONO8, DC1394_VIDEO_MODE_1600x1200_YUV422, DC1394_VIDEO_MODE_1600x1200_RGB8,
				  DC1394_VIDEO_MODE_1600x1200_MONO8, DC1394_VIDEO_MODE_1280x960_MONO16, DC1394_VIDEO_MODE_1600x1200_MONO16,
				  DC1394_VIDEO_MODE_EXIF, DC1394_VIDEO_MODE_FORMAT7_0, DC1394_VIDEO_MODE_FORMAT7_1,
				  DC1394_VIDEO_MODE_FORMAT7_2, DC1394_VIDEO_MODE_FORMAT7_3, DC1394_VIDEO_MODE_FORMAT7_4,
				  DC1394_VIDEO_MODE_FORMAT7_5, DC1394_VIDEO_MODE_FORMAT7_6, DC1394_VIDEO_MODE_FORMAT7_7
				*/
				VideoMode & videoMode( dc1394video_mode_t videoMode ) { mVideoMode = videoMode; return *this; }
				void setVideoMode( dc1394video_mode_t videoMode ) { mVideoMode = videoMode; }
				dc1394video_mode_t getVideoMode() const { return mVideoMode; }

				/** \a coding is one of DC1394_COLOR_CODING_MONO8, DC1394_COLOR_CODING_YUV411,
				    DC1394_COLOR_CODING_YUV422, DC1394_COLOR_CODING_YUV444, DC1394_COLOR_CODING_RGB8,
					DC1394_COLOR_CODING_MONO16, DC1394_COLOR_CODING_RGB16, DC1394_COLOR_CODING_MONO16S,
					DC1394_COLOR_CODING_RGB16S, DC1394_COLOR_CODING_RAW8, DC1394_COLOR_CODING_RAW16
				*/
				VideoMode & colorCoding( dc1394color_coding_t coding ) { mColorCoding = coding; return *this; }
				void setColorCoding( dc1394color_coding_t coding ) { mColorCoding = coding; }
				dc1394color_coding_t getColorCoding() const { return mColorCoding; }

				VideoMode & frameRate( float frameRate ) { mFrameRate = frameRate; return *this; }
				void setFrameRate( float frameRate ) { mFrameRate = frameRate; }
				float getFrameRate() const { return mFrameRate; }

				VideoMode & autoVideoMode( bool autoMode ) { mAutoVideoMode = autoMode; return *this; }
				void setAutoVideoMode( bool autoMode ) { mAutoVideoMode = autoMode; }
				bool getAutoVideoMode() const { return mAutoVideoMode; }

			protected:
				ci::Vec2i mResolution;
				dc1394video_mode_t mVideoMode;
				dc1394color_coding_t mColorCoding;
				float mFrameRate;
				bool mAutoVideoMode;

			public:
				friend std::ostream & operator<<( std::ostream &lhs, const VideoMode &rhs )
				{
					const std::string modes[ DC1394_VIDEO_MODE_NUM ] = {
							"160x120_YUV444", "320x240_YUV422", "640x480_YUV411"
							"640x480_YUV422", "640x480_RGB8", "640x480_MONO8",
							"640x480_MONO16", "800x600_YUV422", "800x600_RGB8",
							"800x600_MONO8", "1024x768_YUV422", "1024x768_RGB8",
							"1024x768_MONO8", "800x600_MONO16", "1024x768_MONO16",
							"1280x960_YUV422", "1280x960_RGB8", "1280x960_MONO8",
							"1600x1200_YUV422", "1600x1200_RGB8", "1600x1200_MONO8",
							"1280x960_MONO16", "1600x1200_MONO16", "EXIF",
							"FORMAT7_0", "FORMAT7_1", "FORMAT7_2", "FORMAT7_3",
							"FORMAT7_4", "FORMAT7_5", "FORMAT7_6", "FORMAT7_7" };
					const std::string codings[ DC1394_COLOR_CODING_NUM ] = {
							"MONO8", "YUV411", "YUV422", "YUV444",
							"RGB8", "MONO16", "RGB16", "MONO16S",
							"RGB16S", "RAW8", "RAW16" };

					if ( rhs.mFrameRate == 0.f )
						lhs << "[" << rhs.mResolution.x << "x" << rhs.mResolution.y << " " <<
							modes[ rhs.mVideoMode - DC1394_VIDEO_MODE_MIN ] << " " <<
							codings[ rhs.mColorCoding - DC1394_COLOR_CODING_MIN ] << "]";
					else
						lhs << "[" << rhs.mResolution.x << "x" << rhs.mResolution.y <<
							"@" << rhs.mFrameRate << " " <<
							modes[ rhs.mVideoMode - DC1394_VIDEO_MODE_MIN ] << " " <<
							codings[ rhs.mColorCoding - DC1394_COLOR_CODING_MIN ] << "]";

					return lhs;
				}
		};

		//! Options for specifying Capture1394 parameters.
		class Options
		{
			public:
				Options() {}

				void setVideoMode( const VideoMode &videoMode ) { mVideoMode = videoMode; }

			private:
				VideoMode mVideoMode;
		};


		/*
		static CaptureRef create( const Options &options, const DeviceRef device = DeviceRef() ) { return CaptureRef( new Capture( width, height, device ) ); }
		*/

		Capture1394() {}
		//! \deprecated Call Capture1394::create() instead
		Capture1394( const Options &options = Options(), const DeviceRef device = DeviceRef() );
		~Capture1394() {}

		//! Returns a vector of all Devices connected to the system. If \a forceRefresh then the system will be polled for connected devices.
		static const std::vector< DeviceRef > & getDevices( bool forceRefresh = false );
		//! Finds a particular device based on its name
		static DeviceRef findDeviceByName( const std::string &name );
		//! Finds the first device whose name contains the string \a nameFragment
		static DeviceRef findDeviceByNameContains( const std::string &nameFragment );

		//! Class for implementing libdc1394 devices.
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

				//! Returns a vector of \a VideoMode's the device supports.
				const std::vector< VideoMode > & getSupportedVideoModes() const { return mSupportedVideoModes; }

				//! Returns a pointer to the libdc1394 device.
				dc1394camera_t * getNative() { return mCamera; }

			protected:
				Device() {}
				dc1394camera_t *mCamera;
				std::vector< VideoMode > mSupportedVideoModes;

				friend class Capture1394;
		};

	protected:
		static void checkError( dc1394error_t err );

		static bool sDevicesEnumerated;
		static std::vector< Capture1394::DeviceRef > sDevices;

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
};

class CaptureExc : public ci::Exception {};
class CaptureExcInitFail : public CaptureExc {};

}; // namespace mndl

