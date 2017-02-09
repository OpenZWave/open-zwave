Open-ZWave Library
==================

Our goal is to create free software library that interfaces with selected Z-Wave PC controllers, allowing anyone to create applications that manipulate and respond to devices on a Z-Wave network, without requiring in-depth knowledge of the Z-Wave protocol.

This software is currently aimed at Application Developers who wish to incorporate Z-Wave functionality into their applications.

Our Homepage is at http://www.openzwave.net/ and our Github Page is located at https://github.com/OpenZWave/. If you wish to participate on our Mailing List, please visit https://groups.google.com/forum/#!forum/openzwave

## Getting Started
There is a example application on how to interface with the library called MinOZW. This currently offers a minimal example on how to monitor for changes and enable Polling of values. There are also plenty of other examples on the Internet that you can research. 

## Getting Help
If you are using a Application that uses OZW, you should first check with the Application Developers for support. As OZW is a library used by dozens of large opensource home automation applications, we can not support application related issues here. If your application developer indicates the problem may be with OZW, please follow the guidelines below. 

First thing you should do if you are running into troubles is search our [mailing list](https://groups.google.com/forum/#!forum/openzwave). There is a high chance that your problem may have already been discussed on the list. 
If your still having problems, you should Generate a Log file and upload your Log to the [Log Analyzer on our homepage](http://www.openzwave.com/log-analyzer), which will check for common issues, and provide some basic recommendations. Please follow the instructions on that page to ensure your log file contains the relevant information. 

If you think you have found a bug, please see the next section. 

For General Questions/How Do I etc, please post a message to our [mailing list](https://groups.google.com/forum/#!forum/openzwave)

## Opening or Contributing to a Issue on Github
Z-Wave is a reasonably complex protocol, and thus, it almost always necessary for us to review a OZW Log file to determine what’s going on. As GitHub does not support attachments on Issues, please upload a [LogFile](http://www.openzwave.com/log-analyzer) and then create a issue on [GitHub](https://github.com/OpenZWave/open-zwave/issues). Please include a link to the LogFile Scan results (check the "Public Results" button on the results page and copy the URL it links to). 
Not uploading a Log file will often mean that we are unable to provide any further help without seeing what’s going on, so please ensure you do this. 

## Supporting New Devices
If your device is not recognized by OZW, it should still work as long as its compliant with the Z-Wave specifications. Our Device database is mainly community contributions, so please consult [this page](https://github.com/OpenZWave/open-zwave/wiki/Adding-Devices) on how to add the Device to the manufacturer_specific.xml file and generate a Configuration file for it. 

## Contributing to OZW
We are happy to accept Pull Requests via GitHub. A few things to follow when preparing a Pull Request. 

1. If you have added or changed any of the configuration files (eg, adding a new device) please run "make xmltest" from the root directory to ensure your XML is well formed. 
2. Please add a entry to the ChangeLog describing the change you made. 
3. If you are changing some internal code paths in OZW, please discuss on the mailing list prior to starting your work. Z-Wave is complex and there are lots of corner cases as far as proper support of the various (non-standard) devices out there. While it might sometimes seem overcomplicated, it most likely is there for a reason. 
4. Finally, if you have signed a NDA with Sigma Designs, we are unable to accept your changes. OZW is developed by reverse engineering and consulting publically available information. We have not signed a NDA with Sigma, and do not want users to infringe upon their agreement with Sigma either. 

## Final Words
As of Sept, 2016, Sigma has released a large portion (not all) of the Z-Wave Protocol Specifications into the public domain. Prior to Sept, 2016, OpenZWave was developed by mainly reverse engineering the protocol as well as consulting various public information on the Internet. 

The Z-Wave Specifications can now be found at http://zwavepublic.com/ and we welcome contributions or
reviews of the specifications against our code base. 

OpenZWave is a opensource program that is LGPL licensed. This does allow commercial applications to utilize the openzwave libary, but we ask that you support us by either contributing any changes back to the community, or consider a donation of Z-Wave hardware to the developers so we can continue to ensure openzwave works well with the z-wave ecosystem. (You should also be aware that of the Z-Wave restrictions about selling software that implements this public standard. Please consult Sigma, or the Z-Wave Alliance for further info)

Of course, for Open Source applications, as long as you abide by our License (LGPL) we would love to welcome you the OZW community!

And for those that are wondering, you cannot use OZW to create Z-Wave Nodes. This is not the goal of the library. We allow applications to talk to Z-Wave nodes on the network via a Z-Wave Controller. 

<p align="center">
<img src="https://github.com/OpenZWave/open-zwave/raw/master/docs/images%2Bcss/zwalliance_250x100.jpg"><br>
OpenZWave is a Affiliate Member of the Z-Wave Alliance
</p>
