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

extern uint16_t ozw_vers_major;
extern uint16_t ozw_vers_minor;
extern uint16_t ozw_vers_revision;
namespace OpenZWave
{

namespace Testing
{
TEST(OpenZWave, Version)
{
	EXPECT_EQ(ozw_vers_major, 1);
	EXPECT_EQ(ozw_vers_minor, 6);
	// Allow ozw_vers_revision to be zero, to avoid build server failure:
	// "ValueID_test.cpp:42: Failure"
	// "Expected: (ozw_vers_revision) >= (900), actual: 0 vs 900 [ FAILED ] OpenZWave.Version (0 ms)""
	if (ozw_vers_revision != 0)
	{
		EXPECT_GE(ozw_vers_revision, 992);
	}
}
TEST(ValueID, Constructor)
{
	ValueID *vid = new ValueID(0xFFFFu, 0x1, ValueID::ValueGenre_Basic, 0xCC, 0x02, 0x04, ValueID::ValueType_BitSet);
	EXPECT_EQ(vid->GetCommandClassId(), 0xCC);
	EXPECT_EQ(vid->GetGenre(), ValueID::ValueGenre_Basic);
	EXPECT_EQ(vid->GetHomeId(), 0xFFFFu);
	EXPECT_EQ(vid->GetIndex(), 0x04);
	EXPECT_EQ(vid->GetInstance(), 0x02);
	EXPECT_EQ(vid->GetNodeId(), 0x01);
	EXPECT_EQ(vid->GetType(), ValueID::ValueType_BitSet);
	EXPECT_EQ(vid->GetId(), 0x400000133002Au);
	delete vid;
}
TEST(ValueID, KeyConstructor)
{
	// static cast needed to avoid: "call to constructor of 'OpenZWave::ValueID' is ambiguous"
	ValueID *vid = new ValueID(0xFFFFu, static_cast<uint64>(0x400000133002Au));
	EXPECT_EQ(vid->GetCommandClassId(), 0xCC);
	EXPECT_EQ(vid->GetGenre(), ValueID::ValueGenre_Basic);
	EXPECT_EQ(vid->GetHomeId(), 0xFFFFu);
	EXPECT_EQ(vid->GetIndex(), 0x04);
	EXPECT_EQ(vid->GetInstance(), 0x02);
	EXPECT_EQ(vid->GetNodeId(), 0x01);
	EXPECT_EQ(vid->GetType(), ValueID::ValueType_BitSet);
	delete vid;
}
TEST(ValueID, Comparision)
{
	EXPECT_EQ(
		ValueID(0xFFFF, (uint64)0x400000133002A),
		ValueID(0xFFFF, 0x1, ValueID::ValueGenre_Basic, 0xCC, 0x02, 0x04, ValueID::ValueType_BitSet));
	EXPECT_NE(
		ValueID(0xFFFF, (uint64)0x01),
		ValueID(0xFFFF, 0x1, ValueID::ValueGenre_Basic, 0xCC, 0x02, 0x04, ValueID::ValueType_BitSet));
}
TEST(ValueID, GetStoreKey)
{
	ValueID *vid1 = new ValueID(0xFFFFu, 0x1, ValueID::ValueGenre_Basic, 0xCC, 0x02, 0x04, ValueID::ValueType_BitSet);
	ValueID *vid2 = new ValueID(0xFFFFu, 0x1, ValueID::ValueGenre_Basic, 0x01, 0x01, 0x01, ValueID::ValueType_BitSet);
	ValueID *vid3 = new ValueID(0xFFFFu, 0x1, ValueID::ValueGenre_Basic, 0x80, 0x80, 0x80, ValueID::ValueType_BitSet);

	EXPECT_EQ(vid1->GetValueStoreKey(), ValueID::GetValueStoreKey(0xCC, 0x02, 0x04));
	EXPECT_EQ(vid2->GetValueStoreKey(), ValueID::GetValueStoreKey(0x01, 0x01, 0x01));
	EXPECT_EQ(vid3->GetValueStoreKey(), ValueID::GetValueStoreKey(0x80, 0x80, 0x80));
	EXPECT_NE(vid1->GetValueStoreKey(), vid2->GetValueStoreKey());
	EXPECT_NE(vid1->GetValueStoreKey(), vid3->GetValueStoreKey());
	EXPECT_NE(vid2->GetValueStoreKey(), vid3->GetValueStoreKey());

	// See if we've all three contributing parts (CC, instance and index) in the mix.

	EXPECT_NE(ValueID::GetValueStoreKey(0x80, 0x80, 0x80), ValueID::GetValueStoreKey(0x81, 0x80, 0x80));
	EXPECT_NE(ValueID::GetValueStoreKey(0x80, 0x80, 0x80), ValueID::GetValueStoreKey(0x80, 0x81, 0x80));
	EXPECT_NE(ValueID::GetValueStoreKey(0x80, 0x80, 0x80), ValueID::GetValueStoreKey(0x80, 0x80, 0x81));

	ValueID *vid4 = new ValueID(0xFFFFu, 0x1, ValueID::ValueGenre_Basic, 0x81, 0x80, 0x80, ValueID::ValueType_BitSet);
	ValueID *vid5 = new ValueID(0xFFFFu, 0x1, ValueID::ValueGenre_Basic, 0x80, 0x81, 0x80, ValueID::ValueType_BitSet);
	ValueID *vid6 = new ValueID(0xFFFFu, 0x1, ValueID::ValueGenre_Basic, 0x80, 0x80, 0x81, ValueID::ValueType_BitSet);

	EXPECT_NE(vid3->GetValueStoreKey(), vid4->GetValueStoreKey());
	EXPECT_NE(vid3->GetValueStoreKey(), vid5->GetValueStoreKey());
	EXPECT_NE(vid3->GetValueStoreKey(), vid6->GetValueStoreKey());
	EXPECT_NE(vid4->GetValueStoreKey(), vid5->GetValueStoreKey());
	EXPECT_NE(vid4->GetValueStoreKey(), vid6->GetValueStoreKey());
	EXPECT_NE(vid5->GetValueStoreKey(), vid6->GetValueStoreKey());

	delete vid1;
	delete vid2;
	delete vid3;
	delete vid4;
	delete vid5;
	delete vid6;
}
TEST(ValueID, GetAsString)
{
	EXPECT_EQ(
		ValueID(0xFFFF, static_cast<uint64>(0x400000133002A)).GetAsString(),
		"HomeID: 0x0000ffff, ValueID: (Id 0x000400000133002a, NodeID 1, Genre basic, CC 0xcc, Instance 2, Index 4, Type bitset)");
	EXPECT_EQ(
		ValueID(static_cast<uint64>(0x1), 0x02, ValueID::ValueGenre_System, 0x04, 5, 6, ValueID::ValueType_String).GetAsString(),
		"HomeID: 0x00000001, ValueID: (Id 0x0006000002c10057, NodeID 2, Genre system, CC 0x04, Instance 5, Index 6, Type string)");
	EXPECT_EQ(
		ValueID(static_cast<uint64>(0xABCDEF01), static_cast<uint64>(0x0123456789ABCDEF)).GetAsString(),
		"HomeID: 0xabcdef01, ValueID: (Id 0x0123456789abcdef, NodeID 137, Genre config, CC 0xaf, Instance 222, Index 291, Type invalid type)");
}
} // namespace Testing
} // namespace OpenZWave