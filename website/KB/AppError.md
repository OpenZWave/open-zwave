## Application Integration Errors

These Messages typically indicate that a application is incorrectly using the OpenZWave API. These messages should typically be reported to your application developer rather than the OpenZWave Developers. 

### Specific MessageID Results

#### MessageID 6

This message indicates that the Options Class has not been locked prior to starting the Driver. You should ensure you lock the options class as follows:

	Options::Create( "../../../config/", "", "" );
	...
	Options::Get()->Lock();

#### MessageID 7, 78

The application has requested a invalid HomeID when calling one of the OpenZWave API's. You should confirm that the application is passing the correct HomeID's to the OpenZWave API.

#### MessageID 8

The application has requested to poll a ValueID with a incorrect HomeID. You should confirm the HomeID you are passing to the Polling API methods.

#### MessageID 13, 14

You can not modify any options once a Driver is loaded. If you wish to modify the options, you need to unload the driver and then unlock the options, make your changes, and the relock the options class before starting the driver. 

#### MessageID 15, 24, 27

OpenZWave can not open the Serial Port or HID device specified when calling the `Manager::Get()->AddDriver` method. Please confirm you have specified the correct serial port or VID/PID and the application has the necessary permissions to open that serial port/Device

#### MessageID 19

This message indicates that another application could be using the serial port when OpenZWave attempted to open it. You should verify no other applications are using the serial port.

#### MessageID 26

This message indicates that OpenZWave could not find a USB device with the specified VID and PID values. Please double check them.

#### MessageID 41

This message usually indicates that the Application requested a WriteConfig, but OpenZWave either was not fully initialized, or had failed to start correctly. Please only issue the WriteConfig once OpenZWave indicates it is operational via the Notification Callbacks. 

Best Practices is to call WriteConfig after receiving a "AllNodesQueried" or a "AllNodesQueriedSomeDead" notification and again and application shutdown time.

#### MessageID 48

This message usually indicates that a incorrect serial port was specified when OpenZWave was started and we are instead talking to another device, and not a ZWave Controller. Please confirm the serial port is correct.

If you occasionally receive this message but OpenZWave is functioning correctly, it could indicate other problems and you should seek help on the [Google Groups list][1]

#### MessageID 79

A Option was set by the application that OpenZWave does not support. You should confirm the options passed during the initialization phase of OpenZWave (Refer to MessageID 6 above)

#### MessageID 89, 90

The application sent a invalid System_Config Variable to the DoorLock CC. You should contact the developers of your application to investigate this issue. 

#### MessageID 91

The application sent a invalid request to the EnergyProduction CC. You should contact the developers of your application to investigate this issue.

#### MessageID 112

The Application requested UserCode at position 0, which is invalid. Please report this issue to the developers of your application. 


[1]: https://groups.google.com/forum/#!forum/openzwave "OpenZWave Google Groups"
