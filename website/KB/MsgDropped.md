## Dropped Messages

Dropped Messages usually indicate either:

 * A Failed Node (eg, dead battery)
 * A Incorrectly configured Node
 * RF Interference or Coverage issues
 * FLiRS compatibility issues.

### A Failed Node

Failed Nodes obviously will not respond to anything sent by OpenZWave. Failed nodes can be a result of a dead battery, a Node that was "locally" removed from the network (via say a Factory Reset on the device) but never excluded by OpenZWave or the application. If its is a true "failed node" you should remove the node via the RemoveFailedNode Command (if your application supports it). If your application does not support it, ask the developers of your application for assistance, or temporarily switch to another application that does support it. As the NodeList is stored on the USB Controller, as long as any application can remove a failed Node, OZW will also forget about it in subsequent attempts to run your application. 

Should the node not actually be dead, please consult [DeadNode](deadnode) for further information. 

### A Incorrectly Configured Node

Please refer to [DeviceConfigError](deviceconfigerror) for more details. 

### RF Interference and Coverage issues

RF Interference is when there is other devices in range of your Z-Wave network or devices that are interfering with the same frequency that Z-Wave uses. 

Coverage issues are related to devices that are on the limit of the Z-Wave Mesh Network Coverage (eg, they are far away or have walls between them and other devices in the network). 

Please consult [RFIssues](rfissues) for more information.

### FLiRS Compatibility Issues

FLiRS stands for Frequently Listening Routing Slave and is a newer technology deployed for Battery Powered Devices. It is a superior technology to traditional sleeping devices and FLiRS devices should not be confused with sleeping devices. 

As the name implies, FLiRS devices check for new messages typically every 100ms and go straight to sleep. They only listen and do not transmit, so they still are very friendly to battery life. In order to communicate with a FLiRS device, the device that is the last hop of the message to the final destination must support a technology known as beaming. beaming is where the device sends a short burst over the network saying "I have something for you" and hope that the FLiRS device is listening at that time. 

Right now, we have only really seen FLiRS deployed on Door Locks, so its deployment is still quite limited. 

The issue you may face here is if your device Support's FLiRS but not all the neighbors of that device support beaming, then messages sent via a Non-Beaming device will not actually signal the FLiRS device that there is a message pending and in the end the message will timeout. 

Even in cases where the Controller (Typically Node 1) is a Neighbor of a FLiRS device, messages can sometimes still travel via another device to the destination. 

You should do your best to ensure that the neighbors of FLiRS devices support beaming. Several older devices do not support beaming, and you can check if Beaming is supported on your nodes by checking the Node Information Panel in the Results tab of a [LogAnalyzer](/log-analyzer/previous-scan) session. There is also some speculation that all nodes in a messages path to a FLiRS device must support beaming, but we have not been able to confirm this. 

(Note: OpenZWave cannot control the path a message takes through a network as far as we know, hence unlike more traditional "routing" protocols like TCP/IP you cannot influence what devices a Node picks as its Neighbors. It appears to be as long as the Device can see another device, it picks that as a potential neighbor and its upto the protocol to decide how to send that message)

### Specific MessageID Results

#### MessageID 3

We have dropped a message to the Node after attempting retries to send it over. Consult the above topics to investigate possible causes. 







[1]: https://groups.google.com/forum/#!forum/openzwave "OpenZWave Google Groups"
