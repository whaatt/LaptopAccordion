LaptopAccordion
===============
The Laptop Accordion Code

### WTF
It's a laptop. Or an accordion. You decide. And for a decidedly academic take on
things, check out our [paper](http://www.gewang.com/publish/files/2016-nime-la.pdf)
in NIME. If you just want to play the accordion, click the releases tab at the top
of the page and download the correct release for your platform. If you want to try
the nightmare that is compiling the accordion, read on.

### Compilation
1. Download OpenFrameworks 0.9.0 for your platform.
2. Create a new project using the project generator.
3. Copy the appropriate directory in this repo on top of the project.
4. In the XCode or VS 2015 project, add all files in the `src` folder.
5. See below for instructions on how to compile and link FluidSynth.

### FluidSynth Windows
On Windows, simply copy the FluidSynth directory to be a sibling directory
of your OpenFrameworks installation. In VS 2015, add all files in the `include`
subdirectory to your project (or add that as a compiler search path), then
add the `lib` directory to the linker search path. Add `libfluidsynth.lib`
to the list of linkages. Hit build. When you get linker errors on your
first build, copy the DLL in `lib` to your application `bin` directory.

### FluidSynth OSX
On OSX, simply copy the FluidSynth directory to be a sibling directory
of your OpenFrameworks installation. In XCode, add all files in the `include`
subdirectory to your project. Now install [Homebrew](http://brew.sh/) and do `brew
install fluidsynth` before continuing. With FluidSynth installed, just add the file
`lib/libfluidsynth.1.dylib` to your project.
