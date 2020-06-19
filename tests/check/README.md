# Running Test Code
### Prerequisite
* You must change `// #define _DEBUG_TRUE` in `plugin/tracers/gstliveprofiler.c` to `#define _DEBUG_TRUE`
### Before running test
* In `tests/mocks/`, run `gcc -DLINKTIME -c mockncurses.c`
* run this script on top directory
```
./autogen.sh --prefix=/usr/ --libdir=/usr/lib/x86_64-linux-gnu/ --enable-gcov --disable-graphviz
make
```
### Run test
run 
```
mkdir coverage
make clean; make coverage
``` 
on `tests/check` directory, then program will automatically test the code and make `index.html`(coverage report) in `tests/check/coverage/plugins/tracers/`.
