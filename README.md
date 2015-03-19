##Introduction

PSim-cpp is a C++11 port of Prof. Massimo Di Pierro's parallel simulator psim.py. Like the Python variant, Psim-cpp uses anonymous OS pipes with serialization to communicate between processors. Psim-cpp also takes advantage of C++11's anonymous functions, storing topology lambdas and binary operator lambdas in functor objects that can be passed as parameters to e.g. global reduction functions. Psim-cpp also attempt to emulate the simplicity of the Python version by using `accumulate()` with functor objects to mimick the functional style of Python's `reduce()`.    

##Building and Running

Psim-cpp uses Boost .dylibs, specifically the `Boost.Serialization` and `Boost.IOStreams` libraries, which were manually added to the Xcode project under `Build Phases`.

To install Boost from MacPorts, run:

```
sudo port install boost
```
The include headers are (depending on your machine) found in `/opt/local/include/` while the .a and .dylib binaries are found in `/opt/local/lib`.

Under PSIM target --> Build Settings --> Search Paths, add `/opt/local/inlcude` to Header Search Paths and `/opt/local/lib` to Library Search Paths. The Boost libraries can now be included in the project. 
