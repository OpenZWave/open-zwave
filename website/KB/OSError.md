## Operating System Errors

These errors are usually related to Operating System issues or incompatibilities. 

### Specific MessageID Results

#### MessageID 20, 21, 22, 23

These Messages indicate that OpenZWave could not set the Serial Port up correctly to communicate with the USB Controller. Please ensure you have specified the correct Serial Port and the application has permissions to access it.

#### MessageID 71

This message indicates that the Poll Thread has not been able to run for over 5 minutes. In most cases this indicates either a overloaded Z-Wave network, or the Operating System is close to its peak utilization, and cannot schedule time for the Thread to run. Please try to reduce the CPU utilization of the System (or check for other processes which maybe consuming a large amount of CPU time) and also review what type of Traffic is going on in your Z-Wave Network.


