## Device Database Configuration Errors

These types of errors usually indicate that the Device Configuration database (manufacturer_specific.xml and associated files) has issues with your device and possibly requires further fine tuning for your device to operate correctly. 

In 9 times out of 10, Most Nodes will work out of the box with OpenZWave without us having to add it to the DeviceDatabase. Sometimes though there are quirks or bugs in the nodes firmware which means that a "advertised" feature of the node either does not work like other similar nodes, or is not supported at all despite the node saying it is. For this reason, we maintain a database (sometimes referred to the manufacturer_specific.xml file and associated files that live in the config directory of OpenZWave) that allows us to deal with these devices that behave differently. 

In the Z-Wave protocol, there is no way to distinguish between us sending a message to a node that is not currently alive, or a sending a command to a node that it does not (correctly) support. The node just simply does not acknowledge the message, hence a bit of investigation is required to determine the real root cause of Dropped Messages. There are a few things you can try:

 * Update your copy of OpenZWave, or if that's not possible, at least download a updated version of the manufacturer_specific.xml file and associated sub-directories. 
 * If its a relatively new device, confirm its listed in the manufacturer_specific.xml file and if not, send some details, including a link to any online manual or reference to the [Google Groups List][1] so someone can add it to the device database and get you to confirm it works after that. 
 * Search the [Google Groups list][1] list to see if other users have experienced the same issue with the device and if any solution has be offered. 

### Specific MessageID Results

#### MessageID 15

This message relates to a specific firmware bug that is present in Vitrum Devices. If you are receiving this message, its best to report this issue to the OpenZWave Developers

#### MessageID 17, 18

This message means that for MultiInstance Devices, we could not map a EndPoint to a CommandClass. This usually requires a specific configuration and should be reported to the [Google Groups list][1] or [BugTracker][2]

#### MessageID 84, 85

The TriggerRefreshValue attribute in one of the DeviceConfig Files is not setup correctly. Please ensure you have the latest Device Database files, and if so, report this to the developers.

#### MessageID 20000

The Device did not acknowledge a message sent to it. This could indicate that the device is incorrectly advertising some CommandClasses it doesn't support, a error/bug with the device, a Incorrect Configuration or a [RFIssue](rfissue). 

You should consult the error message try to determine which CommandClass the message originated from and consult the device manual, [Google Groups list][1] or other public sources such as [Pepper1db](http://www.pepper1.net/zwavedb/) to determine if the device really does support the CommandClass. 

If the device does not appear to correctly support the CommandClass, please consult the [Google Groups list][1] for further information.



[1]: https://groups.google.com/forum/#!forum/openzwave "OpenZWave Google Groups"
[2]: http://code.google.com/p/open-zwave/issues/list "OpenZWave Issue Tracker"
