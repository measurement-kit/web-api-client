# Web API Client

This contains client implementation to speak to web service, such as:

* OONI Orchestra
* GeoIP Lookup services
etc.

## Build instructions

### Unix

```
cmake -GNinja .
ninja -v
ctest -a -j8
```
### Msys2

```
cmake -G'Unix Makefiles' .
make
ctest -a -j8
```

### Windows

Open `CMakeLists.txt` with Visual Studio. Make sure you select the
`x64-Release` build, since it's the only supported build for now.
