# Running Test Code
### Prerequisite
* You must change `// #define _DEBUG_TRUE` in `plugin/tracers/getliveprofiler.c` to `#define _DEBUG_TRUE`
### Before running test
* In `tests/mocks/`, run `gcc -DLINKTIME -c mockncurses.c`
* run this script on top directory
```
./autogen.sh --prefix=/usr/ --libdir=/usr/lib/x86_64-linux-gnu/ --enable-gcov --disable-graphviz
make clean; make
```
### Run test
run `make clean; make coverage`, then program will automatically test the code and make `index.html`(coverage report) in `tests/check/coverage/plugins/tracers/`.
