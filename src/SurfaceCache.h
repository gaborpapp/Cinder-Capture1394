#pragma once

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/Surface.h"

class SurfaceCache
{
	public:
		SurfaceCache( int32_t width, int32_t height, ci::SurfaceChannelOrder sco, int numSurfaces );
		void resize( int32_t width, int32_t height );
		ci::Surface8uRef getNewSurface();

	private:
		std::vector< std::shared_ptr< uint8_t > > mSurfaceData;
		std::vector< bool > mSurfaceUsed;
		int32_t mWidth, mHeight;
		ci::SurfaceChannelOrder mSCO;
};

