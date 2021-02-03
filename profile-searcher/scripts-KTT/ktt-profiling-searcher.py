#!/usr/bin/env python

"""Prototype implementation of profiling-counters based searcher

Usage:
  ktt-profiling-searcher.py -o <KTT_space> --oc <output_compute> --mp <multiprocessors> --co <cuda_cores> (-s <statistics> | --cm <completemappning> | --dt <decision-tree-modelFiles> | --ls <least-square-modelFiles>) --ic <input_compute> -i <index> -b <indices> -p <KTT_counters> [--compute_bound | --memory_bound] [--debug]

Options:
  -h Show this screen.
  -o Output CSV file from KTT describing tuning space.
  --oc Compute capability of device where output CSV has been produced
  --mp Number of multiprocessors on GPU where output CSV has been produced
  --co Number of CUDA cores on GPU where output CSV has been produced
  -s Statistics generated from KTT output file (can be from incomplete space, from different device).
  --cm Complete mapping of tuning parameters to profile counters (mutually exclusive with statistics).
  --dt Decision tree generated by mlKTTPredictor.py (*.sav)
  --ls Least squares nonlinear models generated by create_least_squares_models.R
  --ic Compute capability of device from which statistics were computed.
  -i Index of the actual run within tuning space.
  -p Profiling counters of actual run.
  -b Set number of indices to be returned to KTT [default=5]
  --compute-bound Indicates the problem is compute-bound
  --memory-bound Indicates the problem is memory-bound
  --debug  Adds more extensive logging
"""

from docopt import docopt
import random
import math
from operator import add
import glob
from base import *
from mlKTTPredictor import *
import numpy as np
import os


############## main function emulating KTT tuning and search  ##################
if __name__ == '__main__':

    # parse command line
    arguments = docopt(__doc__)
    tuningSpace = open(arguments['-o'])
    ccCounters = float(arguments['--oc'])
    multiprocessors = int(arguments['--mp'])
    cudaCores = int(arguments['--co'])
    if arguments['--dt']:
        trainedKnowledgeBase = str(arguments['--dt'])
    elif arguments['-s'] :
        statCorr = open(arguments['-s'] + "-corr.csv")
        statVar = open(arguments['-s'] + "-var.csv")
    elif arguments['--ls'] :
        modelFiles = glob.glob(arguments['--ls']+"-model_*.csv")
    elif rguments['--cm'] :
        completeMapping = open(arguments['--cm'])
    ccInput = float(arguments['--ic'])
    currentIndex = int(arguments['-i'])
    NONPROFILE_BATCH = int(arguments['-b'])


    if arguments.get('--compute_bound') :
        setComputeBound()
    if arguments.get('--memory_bound') :
        setMemoryBound()

    debug = arguments.get('--debug')

    #open pipes to communicate with KTT
    readFromKTTfd = int(os.getenv("PY_READ_FD"))
    writeToKTTfd = int(os.getenv("PY_WRITE_FD"))

    readFromKTT = os.fdopen(readFromKTTfd, 'rb', 0)
    writeToKTT = os.fdopen(writeToKTTfd, 'wb', 0)

    # parse tuning space
    words = tuningSpace.readline().split(',')
    tuningParams = []
    for w in words:
        tuningParams.append(w.rstrip())

    configurationsData = []
    for line in tuningSpace.readlines():
        words = line.split(',')
        if len(words) <= 1: break

        tunRow = []
        for w in words:
            tunRow.append(float(w))
        configurationsData.append(tunRow)

    # load statistics
    if arguments['-s'] :
        profilingCountersStats = loadStatisticsCounters(statCorr)
        correlations = loadStatistics(statCorr, tuningParams, profilingCountersStats)
        variations = loadStatistics(statVar, tuningParams, profilingCountersStats)
    elif arguments['--ls'] :
        [tuningParams, tuningparamsAssignments, conditionsAllModels, profilingCountersAssignments, profilingCountersStats] = loadModels(modelFiles)
        completeMappingCounters = makePredictions(configurationsData, tuningparamsAssignments, conditionsAllModels, profilingCountersAssignments)
    elif arguments['--cm'] :
        print("XXX workaround with fixed ranges of TPs and PCs!")
        countersRange = "19:59"
        tuningRange = "4:19"
        rangeC = eval("np.r_[" + countersRange + "]")
        rangeT = eval("np.r_[" + tuningRange + "]")
        profilingCountersStats = loadCompleteMappingCounters(completeMapping, rangeC)
        completeMappingCounters = loadCompleteMapping(completeMapping, rangeT, rangeC)
    elif arguments['--dt'] :
        profilingCountersStats = readPCList(trainedKnowledgeBase + ".pc")

    if debug:
        print("Python ktt-profiling-searcher: Tuning space loaded, statistics prepared.")
    #wait in loop for message from KTT until "quit" is received
    while True :
        buf = readFromKTT.read(50).decode()
        if debug:
            print("Python ktt-profiling-searcher received message from KTT: " + buf)

        # if received "quit" or empty message (happens when the pipe is closed), quit
        if buf == "quit" or buf == "" :
            print("Python ktt-profiling-searcher received message quit from KTT or the pipe was closed. Quitting.")
            break

        words = buf.split(' ')
        if words[0] == "read":
            try :
                currentIndex = int(words[1])
                if debug:
                    print("Python ktt-profiling-searcher parsed message from KTT, read " + str(currentIndex))
            except:
                print("Python ktt-profiling-searcher received unexpected message: " + words[1] + ". Quitting.")
                break
        else :
            print("Python ktt-profiling-searcher received unexpected message: " + words[0] + ". Quitting.")
            break


        #parse profiling counters
        profilingData = open(arguments['-p'])
        words = profilingData.readline().split(',')
        profilingCountersTuning = []
        for w in words:
            profilingCountersTuning.append(w.rstrip())

        countersData = []
        words = profilingData.readline().split(',')
        for w in words:
            countersData.append(float(w))


        # perform the search
        random.seed()
        scoreDistrib = [1.0]*len(configurationsData) #TODO load previously cutted configurations
        bottlenecks = analyzeBottlenecks(profilingCountersTuning, countersData, ccCounters, multiprocessors, cudaCores);
        changes = computeChanges(bottlenecks, profilingCountersStats, ccInput)
        if arguments['-s'] :
            scoreDistrib = scoreTuningConfigurations(changes, tuningParams, configurationsData[currentIndex], configurationsData, correlations, variations, scoreDistrib)
        elif arguments['--ls'] :
            scoreDistrib = scoreTuningConfigurationsExact(changes, tuningParams, configurationsData[currentIndex], configurationsData, completeMappingCounters, scoreDistrib)
        elif arguments['--cm'] :
            scoreDistrib = scoreTuningConfigurationsExact(changes, tuningParams, configurationsData[currentIndex], configurationsData, completeMappingCounters, scoreDistrib)
        elif arguments['--dt'] :
            scoreDistrib = scoreTuningConfigurationsPredictor(changes, tuningParams, configurationsData[currentIndex], configurationsData, scoreDistrib, trainedKnowledgeBase)
        scoreDistrib[currentIndex] = 0

        idx = [0]*NONPROFILE_BATCH
        for i in range(0, NONPROFILE_BATCH) :
            idx[i] = weightedRandomSearchStep(scoreDistrib, len(configurationsData))
            scoreDistrib[idx[i]] = 0.0

        message = ""
        for i in range(0, NONPROFILE_BATCH) :
            message  = message + str(idx[i]) + ","
        message = message + "\n"
        writeToKTT.write(message.encode())
