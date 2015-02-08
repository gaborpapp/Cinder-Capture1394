/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
    the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
    the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "SurfaceCache.h"

SurfaceCache::SurfaceCache( int32_t width, int32_t height, ci::SurfaceChannelOrder sco, int numSurfaces )
        : mWidth( width ), mHeight( height ), mSCO( sco )
{
	for ( int i = 0; i < numSurfaces; ++i )
	{
		mSurfaceData.push_back( std::shared_ptr<uint8_t>( new uint8_t[ width * height * sco.getPixelInc()], std::default_delete<uint8_t[]>() ) );
		mSurfaceUsed.push_back( false );
	}
}

void SurfaceCache::resize( int32_t width, int32_t height )
{
	mWidth = width;
	mHeight = height;
}

ci::Surface8uRef SurfaceCache::getNewSurface()
{
	// try to find an available block of pixel data to wrap a surface around
	for ( size_t i = 0; i < mSurfaceData.size(); ++i )
	{
		if ( ! mSurfaceUsed[i] )
		{
			mSurfaceUsed[i] = true;
			auto newSurface = new ci::Surface( mSurfaceData[i].get(), mWidth, mHeight, mWidth * mSCO.getPixelInc(), mSCO );
			ci::Surface8uRef result = std::shared_ptr<ci::Surface8u>( newSurface, [=] ( ci::Surface8u* s ) { mSurfaceUsed[i] = false; } );
			return result;
		}
	}

	// we couldn't find an available surface, so we'll need to allocate one
	return ci::Surface8u::create( mWidth, mHeight, mSCO.hasAlpha(), mSCO );
}
