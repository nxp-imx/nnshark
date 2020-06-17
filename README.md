# Live Profiler for Gstreamer
Real-time profiler plugin for Gstreamer

## Open Source License
- GstShark

https://github.com/RidgeRun/gst-shark

- For further information, visit GstShark wiki

https://developer.ridgerun.com/wiki/index.php?title=GstShark

## How To Install
- On Ubuntu x64
```console
$ sudo apt install gtk-doc-tools libgraphviz-dev libncurses5-dev libncursesw5-dev
$ ./autogen_ubuntux64.sh
$ make
$ sudo make install
```
For other environment, visit GstShark wiki

## For contribution
Since gst-shark have gst-indent, you should run gst-indent before commit
```
common/gst-indent <file name>
```

## How To Use Live Profiler
Set environment variable as below.
- GST_DEBUG = "GST_TRACER:7"
- GST_TRACERS = "live"
- LOG_ENABLED = TRUE (if you need)
```console
$ GST_DEBUG="GST_TRACER:7" GST_TRACERS="live"\
     gst-launch-1.0 videotestsrc ! videorate max-rate=15 ! fakesink
```

## How To Use Log Visualizer
You should know directory which have log (default is /nnstreamer/bin)
```
python3 scripts/graphics/log_visualizer.py --dir=<log directory>
```
It will open 3 browsers.
