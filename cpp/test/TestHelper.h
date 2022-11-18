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
#ifndef TESTING_TESTHELPER_H
#define TESTING_TESTHELPER_H

#include <cstdint>
#include "gtest/gtest.h"

namespace OpenZWave
{
namespace Testing
{

// FakeHomeId and FakeNode2Id are used to create "Fake" objects
// when SetUp() is called by the test framework.
// Using these constants improves readability
constexpr uint32_t FakeHomeId = 0x99999999U;
constexpr uint8_t FakeNode2Id = 0x02;
constexpr uint16_t FakeValueIndex = 0x02;
constexpr uint8_t FakeCommandClass = 0x20;

// We'll often use "instance" in a function call with many parameters
// Using these names improves readability
constexpr uint8_t Instance1 = 0x01;
constexpr uint8_t Instance2 = 0x02;
constexpr uint8_t Instance3 = 0x03;
constexpr uint8_t Instance4 = 0x04;
class TestHelper : public testing::Test
{
protected:
	void SetUp() override;
	void TearDown() override;
};
} // namespace Testing
} // namespace OpenZWave
#endif