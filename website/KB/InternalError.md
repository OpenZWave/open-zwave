## Internal OpenZWave Errors

These types of messages typically indicate some sort of corruption of state in OpenZWave. Please consult the list of MessageID's below to determine if you should report it, or safely ignore them. 


### Specific MessageID Results

#### MessageID 5

This error indicates corruption on our Polling List. Please report this error initially to Developers of your application (hint - that's not OpenZWave) and refer them to your LogFile and this page.

This error usually indicates some sort of memory corruption in the application that's affecting OpenZWave. You should first confirm your application is not overwriting memory it shouldn't (on Linux, Valgrind is a handy tool) before seeking assistance from us. 

You should also confirm that you are interacting with the Polling API correctly. 

(Error Message for reference is "IsPolled setting for valueId (.*) is not consistent with the poll list")

#### MessageID 25

This indicates that the Serial Port used to communicate with the USB Controller is not opened correctly. Please restart your application and verify you are selecting the correct serial port and only report this to the OpenZWave Developers if you are confident you have correctly configured OpenZWave.

(Error Message for reference is "ERROR: Serial port must be opened before writing")

#### MessageID 28

Depending upon the contents of the error message, see if you can diagnose what is wrong here. (eg, specifying the incorrect VID or PID of the Z-Wave Controller, otherwise, seek help on the [Google Groups List][1]

(Error Message for reference is "HIDAPI ERROR STRING (if any): (.*)")

#### MessageID 29, 30, 31, 32, 33, 34, 46, 58, 64, 65, 66, 75, 86

These messages should not occur during normal operations in OpenZWave. Please attempt to restart your application and if the errors persist, seek help on the [Google Groups List][1]

#### MessageID 35

This message means that we were unable to correctly initialize the USB Controller. If this is a new controller on the market, we *might* need to update our code to handle it. If its a well known and deployed controller (such as the Aeon Z-Stick) it could indicate that the controller has become corrupted. You should attempt to perform a factory reset, or reload the firmware (if possible). You could also seek help on the [Google Groups List][1]

#### MessageID 47

This message typically indicates that a message sent to the controller was corrupted (its checksum failed). If you are receiving a large amount of these messages please seek help on the [Google Groups List][1]. 
If you infrequently get this message there should not be anything to be concerned about. 

#### MessageID 70

This message can be ignored for now. There is no need to report it as its a known issue with little too no side effects. 

#### MessageID 72, 74

This message indicates that one of the OpenZWave State Files is corrupted (State Files are used to store information about your network so that upon restarts we do not have to rediscover everything). In this case, the zwbutton.xml file is corrupt. You can safely remove this file and restart your application. The file will be automatically recreated. 

#### MessageID 95, 96, 97, 98, 99, 100, 101, 107, 108, 109

These messages indicate failures when setting up or encrypting messages to devices. Please report this to the Developers. 

#### MessageID 103

These messages indicate that the Device Sent a SecuritySchema Report more than once. If you receive lots of these messages, please seek help on the [Google Groups List][1]

#### MessageID 113

This message indicates the UserCode Length Sent to OZW by the device is larger than was OZW can handle. Please report this to the developers. 

[1]: https://groups.google.com/forum/#!forum/openzwave "OpenZWave Google Groups"


