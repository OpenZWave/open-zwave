## Dealing with Dead Nodes

Dead Node Detection in OpenZWave is not perfect. There can be instances where devices do come back alive but OpenZWave still believes they are dead. The reason is a combination of the USB Controller Firmware and how OpenZWave treats dead nodes. 

A Bit of background:

The Firmware in the Z-Wave controller has a list of nodes and also contains another list called "Failed Nodes". When everything on your network is working fine, there are no nodes in the "Failed Node List" and everything works as expected. Should a device stop responding though (via, say a dead battery), after a unknown amount of attempts the firmware in the controller will place the node on the Failed Node List. Typically the only way to recover from a failed node after that is to get the Device to talk back to the controller directly. While most nodes offer a button that will initiate a Node Information Frame, this does not appear to be enough to get the Device of the Failed Node List (maybe because the NIF is usually a broadcast, rather than unicast transmission). 

From the OpenZWave side of things, once a Node ends up on the Failed Node List in the controller, the controller will reject any attempt to communicate with it. There may be a command that be issued to the controller to clear the failed node list, or retry communications with failed nodes, but we are not aware of it. 

To try to overcome this issue, OpenZWave implements its own "Failed Node List" and tries to stop nodes ending up on the controllers failed node list, but this has shortcomings as well. We are currently exploring different options to address this though, so a solution might be available soon. 

Usually the best options for recovery are:

 * Try to initiate a NIF frame from the device. (Usually by the inclusion button located on the device) or get some traffic from the device to the Controller. 

   If its a battery powered device and it looses its setting when the battery dies, this can be problematic though, as the Association settings would have also been lost. 
 * Restart the application. As long as the node is only on OpenZWave's failed node list, this should recover the device.
 * Exclude the device from the network and readd it back. 
 * Worst case scenarios are to do a Hard Reset on the USB Controller. 

### Specific MessageID Results

#### MessageID 2 and 10

This message is emitted when OpenZWave has placed the Node on its own internal Failed Node List. Try the above steps to see if you can recover the node. 

#### MessageID 9

This message is emitted when a Node that OpenZWave had placed on its internal failed node list communicates with us. If you are receiving this message a lot in conjunction with MessageID 2, and are NOT doing anything to rectify the problem, it could mean that you have a device that's on the limit of the Z-Wave Network Coverage. Try moving the node closer to other nodes, or investigate any potential sources of RF interference between the affected node and the neighbors that are close to it.

