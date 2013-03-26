#pragma once

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/Surface.h"

class SurfaceCache
{
	public:
		SurfaceCache( int32_t width, int32_t height, ci::SurfaceChannelOrder sco, int numSurfaces );
		void resize( int32_t width, int32_t height );
		ci::Surface8u getNewSurface();
		static void surfaceDeallocator( void *refcon );

	private:
		std::vector< std::shared_ptr< uint8_t > > mSurfaceData;
		std::vector< bool > mSurfaceUsed;
		std::vector< std::pair< SurfaceCache *, int > > mDeallocatorRefcon;
		int32_t mWidth, mHeight;
		ci::SurfaceChannelOrder mSCO;
};

