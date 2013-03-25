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

#include <string>
#include <vector>

#include "cinder/params/Params.h"

#include "Capture1394.h"

namespace mndl {

typedef std::shared_ptr< class Capture1394Params > Capture1394ParamsRef;
class Capture1394Params
{
	public:
		static Capture1394ParamsRef create() { return Capture1394ParamsRef( new Capture1394Params() ); }
		~Capture1394Params() {}

		void update() { mObj->update(); }

		const Capture1394Ref getCurrentCaptureRef() const { return mObj->mCaptures[ mObj->mCurrentCapture ]; }

	protected:
		Capture1394Params();

		struct Feature
		{
			std::string mName;
			dc1394feature_t mId;
			bool mIsOn;
			int mMode;
			int mValue;
			int mBUValue;
			int mRVValue;

			bool operator==( const Feature &rhs )
			{
				return ( mId == rhs.mId ) &&
					   ( mIsOn == rhs.mIsOn ) &&
					   ( mMode == rhs.mMode ) &&
					   ( mValue == rhs.mValue ) &&
					   ( mBUValue == rhs.mBUValue ) &&
					   ( mRVValue == rhs.mRVValue );
			}
		};

		struct Obj
		{
			Obj();
			~Obj();

			void update();

			std::vector< Capture1394Ref > mCaptures;
			std::vector< std::string > mDeviceNames;
			int mCurrentCapture;

			void setupParams();
			ci::params::InterfaceGl mParams;

			dc1394featureset_t mFeatureSet;
			Capture1394Params::Feature mFeatures[ DC1394_FEATURE_NUM ];
			Capture1394Params::Feature mPrevFeatures[ DC1394_FEATURE_NUM ];
		};

		std::shared_ptr< Obj > mObj;
};

} // namespace mndl
