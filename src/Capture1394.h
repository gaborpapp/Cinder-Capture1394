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

#pragma once

#include <exception>
#include <string>
#include <vector>

#include "cinder/Cinder.h"
#include "cinder/Surface.h"
#include "cinder/Thread.h"

#include <dc1394/dc1394.h>

namespace mndl {

typedef std::shared_ptr< class Capture1394 > Capture1394Ref;

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
						   dc1394color_coding_t coding, dc1394framerate_t frameRate = (dc1394framerate_t)0 ) :
					mResolution( res ), mVideoMode( videoMode ), mColorCoding( coding ),
					mFrameRate( frameRate ), mAutoVideoMode( false ) {}

				//! TODO: the resolution is set by the videomode, this has no effect yet
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

				VideoMode & frameRate( dc1394framerate_t frameRate ) { mFrameRate = frameRate; return *this; }
				void setFrameRate( dc1394framerate_t frameRate ) { mFrameRate = frameRate; }
				dc1394framerate_t getFrameRate() const { return mFrameRate; }

				VideoMode & autoVideoMode( bool autoMode ) { mAutoVideoMode = autoMode; return *this; };
				void setAutoVideoMode( bool autoMode ) { mAutoVideoMode = autoMode; }
				bool getAutoVideoMode() const { return mAutoVideoMode; }

			protected:
				ci::Vec2i mResolution;
				dc1394video_mode_t mVideoMode;
				dc1394color_coding_t mColorCoding;
				dc1394framerate_t mFrameRate;
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
					const float framerates[ DC1394_FRAMERATE_NUM ] = {
						1.85f, 3.75f, 7.5f, 15.f, 30.f, 60.f, 120.f, 240.f };

					if ( rhs.mFrameRate == 0 )
						lhs << "[" << rhs.mResolution.x << "x" << rhs.mResolution.y << " " <<
							modes[ rhs.mVideoMode - DC1394_VIDEO_MODE_MIN ] << " " <<
							codings[ rhs.mColorCoding - DC1394_COLOR_CODING_MIN ] << "]";
					else
						lhs << "[" << rhs.mResolution.x << "x" << rhs.mResolution.y <<
							"@" << framerates[ rhs.mFrameRate - DC1394_FRAMERATE_MIN ] << " " <<
							modes[ rhs.mVideoMode - DC1394_VIDEO_MODE_MIN ] << " " <<
							codings[ rhs.mColorCoding - DC1394_COLOR_CODING_MIN ] << "]";
					return lhs;
				}
		};

		//! Options for specifying Capture1394 parameters.
		class Options
		{
			public:
				Options() : mOperationMode( DC1394_OPERATION_MODE_LEGACY ), mDiscardFrames( true ) {}

				//! Sets video mode. Default is automatic.
				Options &videoMode( const VideoMode &videoMode ) { mVideoMode = videoMode; return *this; }
				void setVideoMode( const VideoMode &videoMode ) { mVideoMode = videoMode; }
				const VideoMode & getVideoMode() const { return mVideoMode; }

				/** Sets operation mode, \a operationMode is one of DC1394_OPERATION_MODE_LEGACY, DC1394_OPERATION_MODE_1394B.
				 *  Default is DC1394_OPERATION_MODE_LEGACY
				 */
				Options &operationMode( dc1394operation_mode_t operationMode ) { mOperationMode = operationMode; return *this; }
				void setOperationMode( dc1394operation_mode_t operationMode ) { mOperationMode = operationMode; }
				dc1394operation_mode_t getOperationMode() { return mOperationMode; }

				//! Enables frame discarding. Default is on.
				Options &discardFrames( bool discard ) { mDiscardFrames = discard; return *this; }
				void setDiscardFrames( bool discard ) { mDiscardFrames = discard; }
				bool getDiscardFrames() { return mDiscardFrames; }

			private:
				VideoMode mVideoMode;
				dc1394operation_mode_t mOperationMode;
				bool mDiscardFrames;
		};


		static Capture1394Ref create( const Options &options = Options(), const DeviceRef device = DeviceRef() ) { return Capture1394Ref( new Capture1394( options, device ) ); }

		Capture1394() {}
		~Capture1394() {}

		//! Begin capturing video.
		void start() { mObj->start(); }
		//! Stop capturing video.
		void stop() { mObj->stop(); }
		//! Is the device capturing video
		bool isCapturing() const { return mObj->mIsCapturing; }

		//! Returns the width of the captured image in pixels.
		int32_t getWidth() const { return mObj->getWidth(); }
		//! Returns the height of the captured image in pixels.
		int32_t getHeight() const { return mObj->getHeight(); }
		//! Returns the size of the captured image in pixels.
		ci::Vec2i getSize() const { return ci::Vec2i( getWidth(), getHeight() ); }
		//! Returns the aspect ratio of the capture imagee, which is its width / height
		float getAspectRatio() const { return getWidth() / (float)getHeight(); }
		//! Returns the bounding rectangle of the capture imagee, which is Area( 0, 0, width, height )
		ci::Area getBounds() const { return ci::Area( 0, 0, getWidth(), getHeight() ); }

		//! Returns whether there is a new video frame available since the last call to checkNewFrame().
		bool checkNewFrame() const { return mObj->checkNewFrame(); }

		//! Returns a Surface representing the current captured frame.
		ci::Surface8u getSurface() const { return mObj->getSurface(); }

		//! Returns the associated Device for this instance of Capture1394
		const DeviceRef getDevice() const { return mObj->mDevice; }

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

		static void checkError( dc1394error_t err );

	protected:
		Capture1394( const Options &options = Options(), const DeviceRef device = DeviceRef() );

		static bool sDevicesEnumerated;
		static std::vector< Capture1394::DeviceRef > sDevices;

		struct Obj
		{
			Obj( const Options &options, const Capture1394::DeviceRef device );
			~Obj();

			void start();
			void stop();

			int32_t getWidth() const { return mWidth; };
			int32_t getHeight() const { return mHeight; };

			bool checkNewFrame() const;
			ci::Surface8u getSurface() const;

			Options mOptions;
			DeviceRef mDevice;
			int32_t mWidth, mHeight;

			std::shared_ptr< std::thread > mThread;
			mutable std::mutex mMutex;
			bool mThreadShouldQuit;

			void threadedFunc();

			std::shared_ptr< class SurfaceCache > mSurfaceCache;
			ci::Surface8u mCurrentSurface;
			mutable bool mHasNewFrame;
			bool mIsCapturing;
		};

		std::shared_ptr< Obj > mObj;

	public:
		//@{
		//! Emulates shared_ptr-like behavior
		typedef std::shared_ptr<Obj> Capture1394::*unspecified_bool_type;
		operator unspecified_bool_type() const { return ( mObj.get() == 0 ) ? 0 : &Capture1394::mObj; }
		void reset() { mObj.reset(); }
		//@}
};

class Capture1394Exc : public std::exception
{
	public:
		Capture1394Exc( dc1394error_t err ) throw();
		Capture1394Exc( const std::string &log ) throw();

		virtual const char * what() const throw()
		{
			return mMessage;
		}

	private:
		char mMessage[ 129 ];
};

}; // namespace mndl

