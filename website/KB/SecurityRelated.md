## Security CC Errors

These messages are related to the Security CC and usually indicate some sort of failure when trying to talk encrypted to a Security enabled Device. You should ensure that you have followed the instructions on the [OpenZWave Wiki](http://code.google.com/p/open-zwave/wiki/Adding_Security_Devices_to_OZW) about adding Security Devices to OZW when setting up your device. 

### Specific MessageID Results

#### MessageID 102, 105, 106

This message indicates that we could not encrypt or decrypt a packet from a device. It could indicate that the Network Key specified by the application or Options.xml is different to what was used when including the device. You should ensure that the Network key is the same by possibly factory resetting the device and including it in your network again. If not, please seek help on the [Google Groups list][1]

#### MessageID 104

SecurityScheme's are pre-determined authentication paramaters defined by the Z-Wave Protocol. Currently OZW implements Scheme 0 which is the only known Scheme at this time. If you recieve this message, it indicates that there is a new Scheme that needs to be investigated and implemented. Please report this to the Developers. 


[1]: https://groups.google.com/forum/#!forum/openzwave "OpenZWave Google Groups"
