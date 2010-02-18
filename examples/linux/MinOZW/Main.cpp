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

#include <unistd.h>
#include "Driver.h"
#include "Node.h"
#include "ValueStore.h"

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
	OpenZWave::Driver::Create( "/dev/ttyUSB0", "../../config/", OnNotification, NULL );
	
	// The driver is a singleton, so once created, you get a pointer to it
	// from anywhere in your code as follows
	OpenZWave::Driver* pDriver = OpenZWave::Driver::Get();

	// Now we just wait forever, while the Driver thread does all the 
	// initialisation and querying of the Z-Wave network.  In a normal app,
	// this is where you would go on to handle user input.
	while( true )
	{
		sleep(10000);
	}

	return 0;
}




