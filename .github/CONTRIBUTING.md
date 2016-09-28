Contributing to OpenZWave
=========================

Thanks for considering to contribute to the OpenZWave Development!
Here are a few tips to help you get started:

### Contributing New Devices to OZW
Please consult https://github.com/OpenZWave/open-zwave/wiki/Adding-Devices
for the steps and instructions on how to add new devices to OZW

### Fixing Bugs
Please base your bug fixes against the master branch. The master branch is
considered the stable and is used for our releases. 

Please also use a descriptive commit message

### Contributing new Features
Our advise before starting work on new features for OZW, is that you discuss
your ideas on the mailing list. This helps ensure that your enhancements
will fits in with our direction for OZW, as well as meets a few requirements
around our API and designs of the library interface. 

All changes should be based against the dev branch, unless advised by a
Maintainer to use a different branch. 

For new command classes, we generally want to evaluate how to expose the new
functionality to applications via ValueID's properly as we do not want to
alter this portion of the API in the future as new revisions of
CommandClasses are released by Sigma. This often requires some in depth
discussions. 

For new platform support, please use the platform abstraction where
possible. try to avoid as much as possible #ifdef statements in the main
code base. 

