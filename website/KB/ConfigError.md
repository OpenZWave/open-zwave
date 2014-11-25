## Configuration Related Errors

Configuration Errors usually relate to corrupt or missing files that are needed by OpenZWave to operate correctly. 

### Specific MessageID Results

#### MessageID 11, 12

This message indicates that a attempt to open a Configuration or State File failed at the specified location. OpenZWave will typically retry a few different paths (System Path, Local Path and application defined paths) before giving up. If you recieve MessageID 12, it indicates that OpenZWave could not find the file, and has given up, and thus you should ensure that the files specified in the message are located in one of the paths attempted.

#### MessageID 36, 38, 39, 40, 73

This indicates that the file specified in the message was from a older version of OpenZWave and cannot be loaded. You should backup and then remove the file and attempt to restart your application.

#### MessageID 37

This error indicates that the file is for a different Z-Wave Network (each network is assigned a random ID when the controller is hard reset). You can safely remove this file and restart your application

#### MessageID 76, 77

This error indicates that the Network Key specified in the Options.xml file does not contain exactly 16 Bytes, or was malformed. The Network Key is used for encryption and should be specified by the user before adding any devices to your network that support the Security CommandClass. You should refer to [Adding Security Devices to OZW](http://code.google.com/p/open-zwave/wiki/Adding_Security_Devices_to_OZW) for more information about correctly setting up and including Security Enabled Devices. 

#### MessageID 80

This indicates that one of the Configuration Files or State Files was corrupted and could not be parsed. You remove the file, and in the case of Configuration files, replace it with a known good copy of the file (eg, from our [Source Repository](http://code.google.com/p/open-zwave/source/browse/#svn%2Ftrunk%2Fconfig))

