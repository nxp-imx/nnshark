# Live Profiler for NNStreamer

[![License](https://img.shields.io/github/license/nnstreamer/nnshark?style=plastic)](https://github.com/nnstreamer/nnshark/blob/master/COPYING.LESSER)

A GStreamer plugin to support real-time performance profiling of NNStreamer pipelines.
This project is a fork of [GstShark](https://github.com/RidgeRun/gst-shark) and initiated as a term project of the Creative Integrated Design courses between 2019 and 2020 in SNU.

## Getting Started

#### Prerequisites

To build NNShark in the Ubuntu/Debian environment, the following packages are needed.

```console
$ sudo apt install gtk-doc-tools libgraphviz-dev libncurses5-dev libncursesw5-dev
...
$ git clone https://github.com/nnstreamer/nnshark.git
$ cd nnshark
```

#### How to build and install

In this example, ```/usr``` and ```/usr/lib/x86_64-linux-gnu/``` are used as @prefix@ and @libdir@, respectively.

```console
$ ./autogen.sh --prefix /usr/ --libdir /usr/lib/x86_64-linux-gnu/
$ make
$ sudo make install
```

Note that, in the case of AMD64 Ubuntu, the ```sudo make install``` command places static and shared libraries in your @libdir@ (i.e., ```/usr/lib/x86_64-linux-gnu/```) and the NNShark gstreamer plugin in your default gstreamer plugin directory (i.e., ```/usr/lib/x86_64-linux-gnu/gstreamer-1.0/```).

## NNStreamer Pipeline Profiling

#### Real-time profiling with interactive user interface

NNShark supports real-time performance profiling of each GstElementts in the target pipeline with ncurse-based interactive user interface.

In this example, an object detection pipeline using the TensorFlow Lite model, ```ssd_mobinet_v2_coco.tflite```, from the [NNStreamer example repository](https://github.com/nnstreamer/nnstreamer-example/tree/master/bash_script/example_object_detection_tensorflow_lite) is chosen.

Before starting the following tutorials, please make sure the [NNStreamer PPA](https://launchpad.net/~nnstreamer/+archive/ubuntu/ppa) is added as one of your APT repositories.

```console
$ sudo add-apt-repository ppa:nnstreamer/ppa
$ sudo apt update
```

1. Install ```nnstreamer``` and ```nnstreamer-tensorflow-lite```

```console
$ sudo apt install nnstreamer nnstreamer-tensorflow-lite
```

2. Clone the [NNStreamer example repository](https://github.com/nnstreamer/nnstreamer-example/tree/master/bash_script/example_object_detection_tensorflow_lite) and prepare the the ```ssd_mobinet_v2_coco.tflite``` TensorFlow Lite model

```console
$ git clone https://github.com/nnstreamer/nnstreamer-example.git
$ cd nnstreamer-example/bash_script/example_models
$ ./get-model.sh ./get-model.sh object-detection-tflite
$ ls tflite_model -1
box_priors.txt
coco_labels_list.txt
ssd_mobilenet_v2_coco.tflite
$ cd ../example_object_detection_tensorflow_lite
$ ln -s ../example_models/tflite_model
```

3. Profile the pipeline at the runtime

NNShark requires two system environment variables, ```GST_DEBUG``` and ```GST_TRACERS```. For the real-time profiling, those two variables should be exported as follows:

```console
$ export GST_DEBUG="GST_TRACER:7"
$ export GST_TRACERS="live"
```

Then, you can launch the pipeline by using the ```gst-launch-object-detection-tflite.sh``` script in the current directory. Note that the source of the pipeline is ```v4l2src``` so that you need v4l2 supported camera devices to launch this pipeline. Otherwise, you can modify the pipeline string in ```gst-launch-object-detection-tflite.sh``` to fit your environment.

```console
$ ./gst-launch-object-detection-tflite.sh
```

You could see the following ncurse-based user interface containing the performance information of the pipeline.

![nnshark_realtime](https://user-images.githubusercontent.com/2772376/90096307-e3213700-dd6d-11ea-8eea-e939b34b7319.png)

#### Profiling based on recorded data with [Log Visualizer](https://github.com/nnstreamer/nnshark/blob/master/scripts/graphics/log_visualizer.py)

While profiling the pipeline at the runtime, you can record raw profile data and visualize them using the provided tool, [Log Visualizer](https://github.com/nnstreamer/nnshark/blob/master/scripts/graphics/log_visualizer.py).

To enable the log-recording mode, export an extra environment variable, ```LOG_ENABLED```, in addition to ```GST_DEBUG``` and ```GST_TRACERS``` as follows:

```console
$ export GST_DEBUG="GST_TRACER:7"
$ export GST_TRACERS="live"
$ export LOG_ENABLED="TRUE"
```

Then, launch the target pipeline as mentioned above and the raw profile data will be placed in ```gstshark_<timestamp>``` of the current directory.

[Log Visualizer](https://github.com/nnstreamer/nnshark/blob/master/scripts/graphics/log_visualizer.py) is a python3 script and depends on the following python modules.

- plotly
- numpy

To visualize the raw data via the Log Visualizer tool, run the following command.

```console
$ python3 scripts/graphics/log_visualizer.py --dir=<log directory>
```

Then, three browser instances showing the CPU usage of each element over time, the processing time for each element over time, and the buffer rate of each pad over time.

The following images show examples for those results on the browser instances.

1. CPU Usage
![cpuusage](https://user-images.githubusercontent.com/44594966/85497519-07d41a80-b619-11ea-810e-6d15661206a4.png)

2. Processing Time
![proctime](https://user-images.githubusercontent.com/44594966/85497520-07d41a80-b619-11ea-9880-bc135c008867.png)

3. Buffer Rate
![bufrate](https://user-images.githubusercontent.com/44594966/85497513-06a2ed80-b619-11ea-8bfa-c3e4c27dae3b.png)
