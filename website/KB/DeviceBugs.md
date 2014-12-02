## Device Bugs or Quirks

These types of errors usually indicate a bug in the Device Firmware or a "Quirk" of the way the device is implemented. Usually we can work around these issues. If you encounter any Messages below for your device, you should alert the developers so we can investigate it further. 

### Specific MessageID Results

#### MessageID 81

The Thermostat returned a Day that was not in a valid range. Please alert the developers

#### MessageID 82

The overrideState value return from the device was not a known state. Please report to the developers

#### MessageID 87, 88

The LockState Variable sent by a device is not known to us. Please report to the Developers.

#### MessageID 92, 110, 111

A device sent a value that is not known to use currently. This could indicate a newer version of the Command Classes on this device, or a Bug. Please report this to the developers.

#### MessageID 93

A device EndPointIndex is higher than what was reported during the QueryStages. This is usually a firmware bug and needs to be worked around in OZW. Please report this to the developers.

#### MessageID 114

This message indicates we received a invalid Wakeup Interval Report from the device. Please report this to the developers. 
