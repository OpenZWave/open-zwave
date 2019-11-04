//-----------------------------------------------------------------------------
//
//	Manager_test.cpp
//
//	Test Framework for Manager Stuff
//
//	Copyright (c) 2019 Peter Gebruers <peter.gebruers@gmail.com>
//
//	Based on work Copyrighted (c) 2017 Justin Hammond <justin@dynam.ac>
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

#include "Manager.h"
#include "TestHelper.h"
#include "gtest/gtest.h"

namespace OpenZWave
{
namespace Testing
{

TEST_F(TestHelper, GetValueEndPoint)
{
	// Test valid data
	EXPECT_EQ(
		Manager::Get()->GetValueEndPoint(ValueID(FakeHomeId, FakeNode2Id, ValueID::ValueGenre_Basic, FakeCommandClass, Instance1, FakeValueIndex, ValueID::ValueType_BitSet)),
		0);

	EXPECT_EQ(
		Manager::Get()->GetValueEndPoint(ValueID(FakeHomeId, FakeNode2Id, ValueID::ValueGenre_Basic, FakeCommandClass, Instance2, FakeValueIndex, ValueID::ValueType_BitSet)),
		1);

	EXPECT_EQ(
		Manager::Get()->GetValueEndPoint(ValueID(FakeHomeId, FakeNode2Id, ValueID::ValueGenre_Basic, FakeCommandClass, Instance3, FakeValueIndex, ValueID::ValueType_BitSet)),
		127);

	// Test *unset* Instance i.e. device does not have Instance

	EXPECT_EQ(
		Manager::Get()->GetValueEndPoint(ValueID(FakeHomeId, FakeNode2Id, ValueID::ValueGenre_Basic, FakeCommandClass, Instance4, FakeValueIndex, ValueID::ValueType_BitSet)),
		0);

	// Test exceptions

	// Could compare "full" message but might prove "unstable" because it contains source code and line number
	// EXPECT_STREQ(e.what(), "Manager.cpp:403 - InvalidHomeIDError (100) Msg: Invalid HomeId passed to GetDriver");

	// Cannot use EXPECT_THROW of googletest here because we need to test properties of the exception.
	// https://github.com/google/googletest/issues/952

	try
	{
		Manager::Get()->GetValueEndPoint(ValueID(0, static_cast<uint64>(0x01)));
		ADD_FAILURE() << "GetValueEndPoint should throw an error, but it did not...";
	}
	catch (OZWException &e)
	{
		EXPECT_EQ(e.GetMsg(), "Invalid HomeId passed to GetDriver");
		EXPECT_EQ(e.GetType(), OZWException::ExceptionType::OZWEXCEPTION_INVALID_HOMEID);
	}
	catch (...)
	{
		ADD_FAILURE() << "GetValueEndPoint should throw OZWException but got a different type.\n ";
	}

	try
	{
		Manager::Get()->GetValueEndPoint(ValueID(FakeHomeId, static_cast<uint64>(0x01)));
		ADD_FAILURE() << "GetValueEndPoint should throw an error, but it did not...";
	}
	catch (OZWException &e)
	{
		EXPECT_EQ(e.GetMsg(), "Invalid ValueID passed to GetValueEndPoint");
		EXPECT_EQ(e.GetType(), OZWException::ExceptionType::OZWEXCEPTION_INVALID_VALUEID);
	}
	catch (...)
	{
		ADD_FAILURE() << "GetValueEndPoint should throw OZWException but got a different type.\n ";
	}
}

} // namespace Testing
} // namespace OpenZWave