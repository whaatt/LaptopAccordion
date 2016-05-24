LaptopAccordion
===============
The Laptop Accordion Code

### WTF
It's a laptop. Or an accordion. You decide. Or, if you want our academic take on
things, check out our [paper](http://www.gewang.com/publish/files/2016-nime-la.pdf)
in NIME. But to get started, simply grab a fresh installation of OpenFrameworks 0.9.0
for your platform, clone the repo, copy the top-level directory corresponding to your
platform into the apps/myApps directory, rename it to something sensible, and open
up XCode or Visual Studio 2015. Then you have to link FluidSynth, which is kind of
a pain in the ass, so I have instructions for that below.

### FluidSynth
On Windows, simply copy the FluidSynth directory to be a sibling directory
of your OpenFrameworks installation, and then when you get linker errors on your
first build, copy the DLL in FluidSynth/lib to your application bin directory.
