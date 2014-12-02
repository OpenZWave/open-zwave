## RF Related Issues

RF Issues related issues are usually a result of either Wireless Interference, or Collisions on your Z-Wave Network. 

Wireless Interference can come from many sources, such as other wireless devices (WIFI, cell phones etc) and we have even had reports of X10 Dongles interfering with Z-Wave if placed too close to the Z-Wave USB Controller. 

### Specific MessageID Results

#### MessageID 1

This message is triggered by collisions on your Z-Wave network. OpenZWave will automatically re-transmit messages so there usually is nothing to really worry about unless you receive a large amount of these Messages. 

If you are getting a large amount of these messages, it can mean that you are reaching the limit of the Z-Wave bandwidth. Most devices today support 40kbit/s, older devices support 9600/bit/s and only very recently have devices started supporting 100kbit/s. If possible, you should look at ways to reduce the amount of messages on your network. This can be in the form of:

 * Reduce the number of Values that are polled by OZW. Typically most devices support reports now days, so polling should only be used as last resort
 * Reduce the frequency of updates from devices (Reports). Devices like temperature sensors or light sensors typically have a configuration option that allows you to set how often they report updates.
 * If you have a large amount of slower devices, attempts to replace them with Z-Wave devices that support higher bandwidth (40 or 100kbit/s) often yield good results. 



