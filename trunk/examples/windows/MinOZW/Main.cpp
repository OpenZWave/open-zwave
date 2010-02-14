//-----------------------------------------------------------------------------
//
//	Main.cpp
//
//	Minimal application to test OpenZWave.
//
//	Creates an OpenZWave::Driver and the waits.  In Debug builds
//	you should see verbose logging to the console, which will
//	indicate that communications with the Z-Wave network are working.
//
//	Copyright (c) 2010 Mal Lansell <mal@openzwave.com>
//
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "Windows.h"
#include "Driver.h"
#include "Node.h"
#include "Group.h"
#include "ValueStore.h"
#include "Value.h"
#include "ValueBool.h"

using namespace OpenZWave;


//-----------------------------------------------------------------------------
// <OnNotification>
// Callback that is triggered when a value, group or node changes
//-----------------------------------------------------------------------------
void OnNotification
(
	Driver::Notification const* _notification,
	void* _context
)
{
	int breakhere = 1;
}

//-----------------------------------------------------------------------------
// <main>
// Create the driver and then wait
//-----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	// Create a Z-Wave Driver

	// Modify this line to set the correct serial port for your PC interface.
	// The second argument is a path for the log file.  If you leave it 
	// blank, the log file will appear in the program's working directory.
	OpenZWave::Driver::Create( "\\\\.\\COM2", "", OnNotification, NULL );
	
	// The driver is a singleton, so once created, you get a pointer to it
	// from anywhere in your code as follows
	OpenZWave::Driver* pDriver = OpenZWave::Driver::Get();

	Sleep(10000);

//	pDriver->BeginAddController();
//	pDriver->BeginRemoveNode();
//	pDriver->BeginAddNode();
//	pDriver->ResetController();

//	if( Node* node = pDriver->GetNode(2) )
	{
		// Polling - enable this for devices that do not report state changes
//		node->SetPolled( true );
		
		// Associations
//		if( Group* group = node->GetGroup(1) )
//		{
			// Add node 3 to group 1 of node 2.
//			group->AddNode(3);
//		}
		
		// Values
//		if( ValueStore* store = node->GetValueStore() )
//		{
//			for( ValueStore::Iterator it = store->Begin(); it != store->End(); ++it )
//			{
//				Value* value = it->second;
				
				// Here we select a value by its label.  In a real app, the user should be presented with
				// a UI displaying all the enumerated values from the store (probably filtered by their 
				// genre - user, system or config).  Each value has a unique ID, so whichever value is
				// modfied by the user, it should be a simple case to get a pointer to the value object
				// using Driver::Get()->GetValue( id );  The value pointer can then be cast to the correct
				// type according to value->GetValueTypeId()

//				if( value->GetLabel() == "Switch" )
//				{
//					ValueBool* valueBool = static_cast<ValueBool*>(value);
//					valueBool->Set( true );
//					Sleep( 5000 );
//					valueBool->Set( false );
//				}
//			}
//		}
	}

	
	// Now we just wait forever, while the Driver thread does all the 
	// initialisation and querying of the Z-Wave network.  In a normal app,
	// this is where you would go on to handle user input.
	while( true )
	{
		Sleep(10000);
	}

	return 0;
}




