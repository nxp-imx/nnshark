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
```console
$ GST_DEBUG="GST_TRACER:7" GST_TRACERS="live"\
     gst-launch-1.0 videotestsrc ! videorate max-rate=15 ! fakesink
```

## Log Visualizer
### Requirement
- python 3.x
- plotly
- numpy

### Usage
If you want to record log, add `LOG_ENABLED` environment like below
```console
$ GST_DEBUG="GST_TRACER:7" GST_TRACERS="live" LOG_ENABLED=TRUE\
     gst-launch-1.0 videotestsrc ! videorate max-rate=15 ! fakesink
```

You should know directory which have log (default path is `/nnstreamer/bin/gstshark_<timestamp>`)
```
python3 scripts/graphics/log_visualizer.py --dir=<log directory>
```
It will open 3 browsers.

### Description

Each browser have Two graph and below one is Buffer Timeline graph.
User can see which buffers each pad is handling at a specific time.

#### (1) CPU Usage 

![cpuusage](https://user-images.githubusercontent.com/44594966/85497519-07d41a80-b619-11ea-810e-6d15661206a4.png)

User can check CPU usage over time.

#### (2) Processing time

![proctime](https://user-images.githubusercontent.com/44594966/85497520-07d41a80-b619-11ea-9880-bc135c008867.png)

User can check processing time for each element.

#### (3) Buffer rate

![bufrate](https://user-images.githubusercontent.com/44594966/85497513-06a2ed80-b619-11ea-8bfa-c3e4c27dae3b.png)

User can check buffer rate for each pad
