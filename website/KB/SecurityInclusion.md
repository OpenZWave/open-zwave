## Inclusion for Security Enabled Devices

Any device that supports the Security CC needs to be included into your network via your application or OZW. Including these devices via say the inclusion button on a Aeon Labs Z-Stick will not attempt to exchange the network keys necessary to enable encrypted communications. You can consult the [Adding_Security_Devices_to_OZW](http://code.google.com/p/open-zwave/wiki/Adding_Security_Devices_to_OZW) wiki page for further information. If you receive any messages below, it indicates that the Inclusion was not successfully and you should re-attempt it after following the recommendations. 

As noted in the Wiki article, there are some timeouts that must be avoided in order to have a device successfully included in the network. You may have to retry the inclusion several times before you get a successfully inclusion.

### Specific MessageID Results

#### MessageID 20002

The first step in setting up the Encryption with a new device to ensure that both OZW and the device support the same encryption parameters. This is the SecurityScheme. If your Device fails to respond to the SecurityScheme Message, it might indicate that it believes it is already included in (another) network and ignores this message. A factory reset on the device should correct this. 

#### MessageID 20003, 20004

These messages indicate that the time taken to communicate the Security Keys with a device is either close to a timeout, or has exceeded the timeout and may result in the device not enabling the Securiy CC. You should reattempt the inclusion (after excluding the device) in the hope that you don't receive this message. 


