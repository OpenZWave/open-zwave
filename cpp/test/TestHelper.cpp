//-----------------------------------------------------------------------------
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

/*
	To get access to private members of Options, Manager and Driver,
	this has to be in their header files

	namespace Testing
	{
		class TestHelper;
	}

	friend class Testing::TestHelper;
*/

#include "TestHelper.h"
#include "Manager.h"
#include "Options.h"
#include "command_classes/CommandClass.h"

namespace OpenZWave
{
namespace Testing
{

void TestHelper::SetUp()
{
	// Doing options = new Options then "fix up" the internal
	// state of the object creates a "fake object". It is
	// much faster, and lighter than calling Options::Create
	// because the latter does a lot more. For testing purposes
	// we don't need need everything done by Create.

	// The constructor of Options does not seem to set "the singleton"
	// while the constructor of Manager does set it... Do this here...
	Options::s_instance = new Options("", "", "");

	// Doing "new Manager" creates a "fake Manager object".
	// It is much faster, and lighter than calling Manager::Create
	// because the latter does a lot more... Like logging, doing http, load
	// config files... Last time I checked, Create took > 1000 ms.
	// doing "new Manager" takes a fraction of that.
	// A call to Manager::Destroy(); is needed to free the memory
	// Manager is a singleton.
	new Manager();

	// You would expect to call Manager::AddDriver("") here but that will start up many
	// things we do not need and will ultimately fail to open the port because we do
	// not have a port... Instead create an instance with fake controllerPath.

	Driver *driver = new Driver("dummy", Driver::ControllerInterface::ControllerInterface_Serial);

	// Pretend the Manager knows about a certain HomeID by setting m_readyDrivers to this fake driver
	Manager::Get()->m_readyDrivers[FakeHomeId] = driver;

	// Pretend node 2 exists
	Node *node = new Node(FakeHomeId, FakeNode2Id);
	driver->m_nodes[node->GetNodeId()] = node;

	auto cc = node->AddCommandClass(FakeCommandClass);

	if (cc == nullptr)
	{
		throw std::runtime_error("auto cc = node->AddCommandClass(test_cc) returned a nullptr");
	}

	// Real devices will usually have either Instance 1 ->  End Point 1 or 0
	// But for sake of testing we can set anything we like.
	cc->SetEndPoint(Instance1, 0);
	node->CreateValueString(ValueID::ValueGenre_User, FakeCommandClass, Instance1, FakeValueIndex, "label", "units", false, false, "default", 0);
	cc->SetEndPoint(Instance2, 1);
	node->CreateValueString(ValueID::ValueGenre_User, FakeCommandClass, Instance2, FakeValueIndex, "label", "units", false, false, "default", 0);
	cc->SetEndPoint(Instance3, 127);
	node->CreateValueString(ValueID::ValueGenre_User, FakeCommandClass, Instance3, FakeValueIndex, "label", "units", false, false, "default", 0);

	// Set a value, but do not set an endpoint, to test if the map properly initializes to zero
	node->CreateValueString(ValueID::ValueGenre_User, FakeCommandClass, Instance4, FakeValueIndex, "label", "units", false, false, "default", 0);
}

// virtual void TearDown() will be called after each test is run.

void TestHelper::TearDown()
{
	// Do a reasonable job of cleaning up
	// Manager::Get()->RemoveDriver calls the driver's destructor
	// That destructor is pretty long and destroys a truckload of objects
	Manager::Get()->RemoveDriver("dummy");
	Manager::Destroy();
	Options::Destroy();
};

} // namespace Testing
} // namespace OpenZWave