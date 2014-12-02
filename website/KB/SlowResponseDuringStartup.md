## Devices are Slow to respond after restarting OZW enabled applications

When OZW is starting up, it goes through several phases known as QueryStages. These stages discover things like the capabilities of the device, its configuration and of course, the current values of the device (for instance temperature). 

The first time a device is added to your network, OZW will conduct a deep inspection of the device, exchanging lots of messages with it, and a lot of "Static" information is saved in the zwcfg_*.xml file. On future restarts OZW will reload the information saved in the zwcfg_*.xml file, and only query for dynamic variables (like the current temperature again). 

Regardless, during startup, on a "healthy" Z-Wave Network, where all devices are configured and operate correctly, it can be a couple of minutes for this phase to complete. (remember, Z-Wave is a low bandwidth protocol unlike wifi etc). On Networks where devices are poorly supported or configured, this can be considerably longer.

In the case of Sleeping devices, when the first message to the device does not get acknowledged (as its sleeping) all messages are moved to a "Wakeup" queue, where they are stored, only to be sent once the device does wake up.

Many users complain that immediately after restarting their application, they make a change and it takes anywhere from 10's of seconds to several minutes for the device to respond. This is due to the fact that there would be many messages queued up ahead of the message that the user has initiated and it takes a while to get to it. Application developers should probably disable the ability to set new values on a ValueID prior to receiving at AllNodesQueried* Notification, or at the very least, warn the user the device might not respond straight away. 

The below messages indicate that a User has attempted to set a value prior to this notification, and hence the message might be delayed. 

### Specific MessageID Results

#### MessageID 20005, 20006, 20007, 20008, 20009, 20011, 20012, 20013, 20014, 20015, 20016, 20017, 20018, 20019

A ValueID was set when OZW was still starting up. You should wait till OZW has fully started up before attempting to set a value. 

