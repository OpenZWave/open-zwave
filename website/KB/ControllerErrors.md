## Controller Errors

These errors are related to communicating with the Controller and can indicate a controller that is not operating correctly.

### Specific MessageID Results

#### MessageID 43, 44

When attempting to read a packet from the Controller, we timed out. This could indicate either a overloaded system or a controller in a corrupt state. Please stop your application, power cycle the controller and try again.

#### MessageID 45

When reading a packet from the Controller, the check-sum failed, indicating a corrupt packet. If you receive this message occasionally, it is usually OK, but if you receive a large amount of these messages, please stop your application, power-cycle the controller and try again. 

#### MessageID 20001

Your Controller does not support SUC/SIS modes used in Z-Wave networks, but SIS was enabled in the Options.xml file. You should set EnableSIS to false in the Options.xml file and restart your application.

