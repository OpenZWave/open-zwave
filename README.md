![Open-ZWave Library](https://github.com/OpenZWave/open-zwave-web/raw/master/gfx/OZW_SF.png)
==================

*Last Updated: 4 October 2019*

Our goal is to create free software library that interfaces with Z-Wave controllers, allowing anyone to create applications to control devices on a Z-Wave network, without requiring in-depth knowledge of the Z-Wave protocol. OpenZWave is not about creating Z-Wave devices (nodes). The project consists of the main library, written in C++ and wrappers and supporting projects, to interface different languages and protocol(s).

This software is currently aimed at Application Developers who wish to incorporate Z-Wave functionality into their applications.

Our Homepage is at [http://www.openzwave.net](http://www.openzwave.net/) and our Github Organization is located at [https://github.com/OpenZWave/](https://github.com/OpenZWave/). There are several repositories at that location. This is the README of the main project (often called "the library" or "OpenZWave" or "open-zwave"), located at [https://github.com/OpenZWave/open-zwave](https://github.com/OpenZWave/open-zwave).

The current version of the main library is called OpenZWave 1.6, it is the *master* branch of the open-zwave repository and tagged "1.6". The previous version was 1.4 (same branch name and tag), and 1.5 was a development branch. The older versions are no longer maintained. The Dev branch is experimental and is not recommend unless someone specifically asks you to try it.

OpenZWave 1.6 was released on March the 5th, 2019, when the Dev branch was merged into master and the 1.6 tag was created. The [OpenZWave Wiki 1.6 Release Notes](https://github.com/OpenZWave/open-zwave/wiki/OpenZWave-1.6-Release-Notes) summarizes changes.

## Getting Started

There is an example application based on the library, it is called MinOZW and it is part of the [open-zwave repository](https://github.com/OpenZWave/open-zwave). This currently offers a minimal example. There are also plenty of other examples on the Internet that you can research.

On Linux, macOS, BSD run "make" to build the library and MinOZW. Optionally, run "make install" to install the library, header and config files system-wide. Run "BUILD=DEBUG make" to make a debug build.

Online [Developers documentation](http://www.openzwave.com/dev/) is generated from source comments by Doxygen. If you do "make install" and have Doxygen on your system, a description of the api will be in open-zwave/docs/api/html/index.html

On Windows, install "Visual Studio" then open "open-zwave/cpp/build/windows/vs2010/OpenZWave.sln" to build the library or "open-zwave/cpp/examples/windows/MinOZW/vs2010/MinOZW.sln" to build both library and sample application.

## Getting Help

If you are using a Application based on OpenZWave, and you have an issue, you should check:

1. If you are an end user, check the Forum and Issue Tracker of your product based on OpenZWave. As this is a library used by dozens of large open source home automation applications, we can not support application related issues here.
2. The [OpenZWave Mailing List](https://groups.google.com/forum/#!forum/openzwave) for general discussion of issues and contacting peers and developers.
3. The [OpenZWave Wiki](https://github.com/OpenZWave/open-zwave/wiki) for main library information.
4. The Issue Tracker of the sub-project, for example the [Zwave2Mqtt Issue Tracker](https://github.com/OpenZWave/Zwave2Mqtt/issues)
5. The [OpenZWave Issue Tracker](https://github.com/OpenZWave/open-zwave/issues) for main library issues and "things being worked on". See "Opening or Contributing to an Issue on Github" if you think the problem is related to the OpenZWave main library.

## Opening or Contributing to an Issue on Github

Please check *both closed and open Issues* before reporting. If you're still having problems, you should generate a (debug) log file and upload that to the [OpenZWave Log Analyzer on our homepage](http://www.openzwave.com/log-analyzer), which will check for common issues, and provide some basic recommendations. Please follow the instructions on that page to ensure your log file contains the relevant information.

If you think you have found a bug, ZIP the OZW_log.txt and your ozwcache*xml file and "drag and drop" the resulting archive on the github issue or comment you create on GitHub. Please do not paste as text.

Please always provide an OZW_Log.txt file. Z-Wave is a reasonably complex protocol, and thus, it is almost always necessary for us to review that log file to understand what is going on.

## Language Wrappers

A number of members of the community have developed wrappers for OpenZWave for other languages. Those actively maintained wrappers can be found at the [OpenZWave Github Organization](https://github.com/OpenZWave)

If you have developed a wrapper for another language, and would like to publish it under the OpenZWave Organization (and gain access to things like CI, Distribution Build Servers etc) please contact Fishwaldo on our mailing list.

## Device Database and Supporting New Devices

If your device is not recognized by OpenZWave, it should still work as long as its compliant with the Z-Wave specifications. Our Device database is mainly based on community contributions, so please consult the [wiki page on Adding Devices](https://github.com/OpenZWave/open-zwave/wiki/Adding-Devices) on how to add the Device to the manufacturer_specific.xml file and generate a Configuration xml file for it.

When OpenZWave 1.6 starts, it checks the version of several related files configuration files and downloads the latest revision. This behavior can be changed by changing the AutoUpdateConfigFile configuration in options.xml. Config options are explained on the  [wiki page on Config-Options](https://github.com/OpenZWave/open-zwave/wiki/Config-Options)

You can have a look at the current device database for OpenZWave 1.6 by browsing to the [Online Device Database](http://openzwave.com/device-database)

Please note device configuration files for 1.6 and 1.4 are different. If your application is based on OpenZWave version 1.4, the older database is still available but unmaintained at: [OpenZWave 1.4 branch, config folder](https://github.com/OpenZWave/open-zwave/tree/1.4/config)

## Contributing to OpenZWave

We are happy to accept Pull Requests via GitHub. A few things to follow when preparing a Pull Request.

1. If you have added or changed any of the configuration files (eg, adding a new device) please run "make xmltest" from the root directory to ensure your XML is well formed. Also run "make dist-update" if you have added files.
2. Please add a entry to the ChangeLog describing the change you made.
3. If you want to change OpenZWave code, please discuss your plan through the Mailing List or on Github *prior to starting your work*. Z-Wave is complex and there are lots of corner cases as far as proper support of the various (non-standard) devices out there. While it might sometimes seem (overly) complicated, it most likely is there for a reason.
4. Finally, if you have signed a NDA with Sigma Designs, and your proposed changes are covered by that NDA, we are unable to accept your changes. OZW is developed by reverse engineering and consulting publicly available information. We do not want users to infringe upon their agreements. The OpenZWave organization has not signed a NDA with Sigma.
5. Parts of the spec have been released as the [Public Z-Wave Specification](https://www.silabs.com/products/wireless/mesh-networking/z-wave/specification), and the radio protocol is know as [G.9959](https://www.itu.int/rec/T-REC-G.9959-201501-I/en). There may be more public resources, but please do check the terms and conditions of every part before posting for compatibility with "Open Source Software"

## Trivia

Prior to Sept, 2016, OpenZWave was developed by mainly reverse engineering the protocol as well as consulting various public information on the Internet.

In September 2016, Sigma Designs released a large portion (not all) of the Z-Wave Protocol Specifications into the public domain.

On April 18, 2018, [Silabs acquired Sigma Design's Z-Wave business](https://news.silabs.com/2018-04-18-Silicon-Labs-Completes-Acquisition-of-Sigma-Designs-Z-Wave-Business), they now host the official [Z-Wave Support Resources](https://www.silabs.com/support/z-wave)

In 2005 the [Z-Wave Alliance](https://z-wavealliance.org) was established. OpenZWave is an Affiliate Member of the Z-Wave Alliance.

<p align="center">
<img src="https://github.com/OpenZWave/open-zwave/raw/master/docs/images%2Bcss/zwalliance_250x100.jpg">
</p>

## License

OpenZWave is an open source program that is LGPL licensed. This does allow commercial applications to utilize the OpenZWave library, but we ask that you support us by either contributing any changes back to the community, or consider a donation of Z-Wave hardware to the developers so we can continue to ensure openzwave works well with the z-wave ecosystem. (You should also be aware that of the Z-Wave restrictions about selling software that implements this public standard. Please consult the [Z-Wave Alliance](https://z-wavealliance.org) and [Z-Wave Support Resources](https://www.silabs.com/support/z-wave) for further info.

Of course, for Open Source applications, as long as you abide by our License (LGPL) we would love to welcome you the OpenZWave community!

## Contributors

### Code Contributors

This project exists thanks to all the people who contribute. [[Contribute](CONTRIBUTING.md)].
<a href="https://github.com/OpenZWave/open-zwave/graphs/contributors"><img src="https://opencollective.com/ozw/contributors.svg?width=890&button=false" /></a>

### Financial Contributors

Become a financial contributor and help us sustain our community. [[Contribute](https://opencollective.com/ozw/contribute)]

#### Individuals

<a href="https://opencollective.com/ozw"><img src="https://opencollective.com/ozw/individuals.svg?width=890"></a>

#### Organizations

Support this project with your organization. Your logo will show up here with a link to your website. [[Contribute](https://opencollective.com/ozw/contribute)]

<a href="https://opencollective.com/ozw/organization/0/website"><img src="https://opencollective.com/ozw/organization/0/avatar.svg"></a>
<a href="https://opencollective.com/ozw/organization/1/website"><img src="https://opencollective.com/ozw/organization/1/avatar.svg"></a>
<a href="https://opencollective.com/ozw/organization/2/website"><img src="https://opencollective.com/ozw/organization/2/avatar.svg"></a>
<a href="https://opencollective.com/ozw/organization/3/website"><img src="https://opencollective.com/ozw/organization/3/avatar.svg"></a>
<a href="https://opencollective.com/ozw/organization/4/website"><img src="https://opencollective.com/ozw/organization/4/avatar.svg"></a>
<a href="https://opencollective.com/ozw/organization/5/website"><img src="https://opencollective.com/ozw/organization/5/avatar.svg"></a>
<a href="https://opencollective.com/ozw/organization/6/website"><img src="https://opencollective.com/ozw/organization/6/avatar.svg"></a>
<a href="https://opencollective.com/ozw/organization/7/website"><img src="https://opencollective.com/ozw/organization/7/avatar.svg"></a>
<a href="https://opencollective.com/ozw/organization/8/website"><img src="https://opencollective.com/ozw/organization/8/avatar.svg"></a>
<a href="https://opencollective.com/ozw/organization/9/website"><img src="https://opencollective.com/ozw/organization/9/avatar.svg"></a>
