import argparse
import os
import plotly.graph_objects as go
import copy
import numpy as np

logging_period = 0.1
cpu_num = 8

ts = 0

cpu_data = []
element_data = {}
pad_data = {}

element_name = []
def parse_log_file():
    metadata_file = args.dir + "/log_metadata"
    log_file = args.dir + "/log"

    with open(metadata_file, "r") as f:
        ts = float(f.readline())
        for i in f.readlines():
            element_name.append(i.split(" ")[1].split("\n")[0])

    for i in range(cpu_num):
        cpu_data.append([])
    
    prev_cpuusage = np.zeros(cpu_num)
    prev_proctime = np.zeros(len(element_name))
    prev_queuelevel = np.zeros(len(element_name))
    prev_maxqueue = np.zeros(len(element_name))
    prev_bufrate = np.zeros(len(element_name))

    for i in range(len(element_name)):
        # Add -1 for Enable value (element don't need bufrate)
        if(len(element_name[i].split("-")) == 1):
            element_data[i] = {"proctime" : [], "queuelevel" : [], "maxqueue" : []}
            prev_bufrate[i] = -1
        else:
            pad_data[i] = {"bufrate" : []}
            prev_proctime[i] = -1
            prev_queuelevel[i] = -1
            prev_maxqueue[i] = -1
    
    with open(log_file, "r") as f:
        data = f.readlines()
        idx = 0
        # Parse Initial data
        while(data[idx].split("\n")[0] == "t"):
            ts += logging_period
            idx += 1

        # Parsing remain data
        for i in range(idx, len(data)):
            parsed_data = data[i].split("\n")[0].split(" ")

            # Next data
            if(parsed_data[0] == "t"):
                for i in range(cpu_num):
                    cpu_data[i].append(prev_cpuusage[i])

                for i in range(len(element_name)):
                    if(prev_proctime[i] != -1):
                        element_data[i]["proctime"].append(prev_proctime[i])
                    if(prev_queuelevel[i] != -1):
                        element_data[i]["queuelevel"].append(prev_queuelevel[i])
                    if(prev_maxqueue[i] != -1):
                        element_data[i]["maxqueue"].append(prev_maxqueue[i])
                    if(prev_bufrate[i] != -1):
                        pad_data[i]["bufrate"].append(prev_bufrate[i])

            # CPU Usage data
            elif(parsed_data[0] == "c"):
                for i in range(cpu_num):
                    prev_cpuusage[i] = float(parsed_data[i+1]) / 10
            # pad data
            elif(parsed_data[0] == "p"):
                prev_bufrate[int(parsed_data[1])] += float(parsed_data[2]) / 100
            # element data
            else:
                if(parsed_data[1] != '.'):
                    prev_proctime[int(parsed_data[0])] += float(parsed_data[1])
                if(parsed_data[2] != '.'):
                    prev_queuelevel[int(parsed_data[0])] += int(parsed_data[2])
                if(parsed_data[3] != '.'):
                    prev_maxqueue[int(parsed_data[0])] += int(parsed_data[3])

        # append last data

        for i in range(cpu_num):
            cpu_data[i].append(prev_cpuusage[i])

        for i in range(len(element_name)):
            if(prev_proctime[i] != -1):
                element_data[i]["proctime"].append(prev_proctime[i])
            if(prev_queuelevel[i] != -1):
                element_data[i]["queuelevel"].append(prev_queuelevel[i])
            if(prev_maxqueue[i] != -1):
                element_data[i]["maxqueue"].append(prev_maxqueue[i])
            if(prev_bufrate[i] != -1):
                pad_data[i]["bufrate"].append(prev_bufrate[i])

def visualize():
    x = np.arange(len(cpu_data[0]))

    config = dict({'scrollZoom': True})

    # For CPU Usage
    fig = go.Figure()
    for i in range(len(cpu_data)):
        fig.add_trace(go.Scatter(
            x=x,
            y=cpu_data[i],
            name = 'CPU ' + str(i),
            connectgaps=True
        ))

    # fig.show(config=config)

    # For proctime
    fig = go.Figure()
    for idx in element_data:
        fig.add_trace(go.Scatter(
            x=x,
            y=element_data[idx]["proctime"],
            name = element_name[idx],
            connectgaps=True
        ))

    # fig.show(config=config)

    # For bufrate
    fig = go.Figure()
    for idx in pad_data:
        fig.add_trace(go.Scatter(
            x=x,
            y=pad_data[idx]["bufrate"],
            name = element_name[idx],
            connectgaps=True
        ))
        
    fig.show(config=config)


if __name__ == "__main__":
    # Parse argument
    parser = argparse.ArgumentParser()
    parser.add_argument('--dir', '-d', help='directory which have your LiveProfiler Log', required=True)
    args = parser.parse_args()

    # Check if directory exists
    if not os.path.isdir(args.dir):
        print("Invalid directory %s" % (args.dir))
        exit(1)
    
    if not os.path.isfile(args.dir + "/log"):
        print("No log file in your directory")
        exit(1)

    if not os.path.isfile(args.dir + "/log_metadata"):
        print("No log metadata file in your directory")
        exit(1)

    parse_log_file()
    visualize()