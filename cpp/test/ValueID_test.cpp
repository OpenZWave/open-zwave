//-----------------------------------------------------------------------------
//
//	ValueID_test.cpp
//
//	Test Framework for ValueID's
//
//	Copyright (c) 2017 Justin Hammond <justin@dynam.ac>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "value_classes/ValueID.h"
using namespace OpenZWave;

TEST(ValueID, Constructor) {
	ValueID *vid = new ValueID(0xFFFF, 0x1, ValueID::ValueGenre_Basic, 0xCC, 0x02, 0x04, ValueID::ValueType_BitSet);
	EXPECT_EQ(vid->GetCommandClassId(), 0xCC);
	EXPECT_EQ(vid->GetGenre(), ValueID::ValueGenre_Basic);
	EXPECT_EQ(vid->GetHomeId(), 0xFFFF);
	EXPECT_EQ(vid->GetIndex(), 0x04);
	EXPECT_EQ(vid->GetInstance(), 0x02);
	EXPECT_EQ(vid->GetNodeId(), 0x01);
	EXPECT_EQ(vid->GetType(), ValueID::ValueType_BitSet);
	EXPECT_EQ(vid->GetId(), 0x400000133002A);
	delete vid;
}
TEST(ValueID, KeyConstructor) {
	ValueID *vid = new ValueID(0xFFFF, (uint64)0x400000133002A);
	EXPECT_EQ(vid->GetCommandClassId(), 0xCC);
	EXPECT_EQ(vid->GetGenre(), ValueID::ValueGenre_Basic);
	EXPECT_EQ(vid->GetHomeId(), 0xFFFF);
	EXPECT_EQ(vid->GetIndex(), 0x04);
	EXPECT_EQ(vid->GetInstance(), 0x02);
	EXPECT_EQ(vid->GetNodeId(), 0x01);
	EXPECT_EQ(vid->GetType(), ValueID::ValueType_BitSet);
	delete vid;
}
TEST(ValueID, Comparision) {
	ValueID *vid1 = new ValueID(0xFFFF, (uint64)0x400000133002A);
	ValueID *vid2 = new ValueID(0xFFFF, 0x1, ValueID::ValueGenre_Basic, 0xCC, 0x02, 0x04, ValueID::ValueType_BitSet);
	ValueID *vid3 = new ValueID(0xFFFF, (uint64)0x01);
//	EXPECT_TRUE(vid1 == vid2);
//	EXPECT_TRUE(vid1 != vid3);
//	EXPECT_FALSE(vid1 == vid3);
	delete vid1;
	delete vid2;
	delete vid3;
}




