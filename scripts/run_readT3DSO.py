#! /usr/bin/python3
#  2026 May, Kentaro Miuchi
#

#"""ehternet access to T3DSO1204"""

import struct
import sys
import os
import csv
import subprocess
import time
import datetime
import threading
import json
import argparse
import socket

#import errno
#import rettest
from datetime import timezone, timedelta
JST = timezone(timedelta(hours=+9), 'JST')


#PATHs
HOME = os.environ["HOME"]+"/"
T3DSOdir=HOME+"T3DSO/"
T3DSObin=T3DSOdir+"bin/"
T3DSOConfigs=T3DSOdir+"configfiles/"
T3DSOScripts=T3DSOdir+"scripts/"
T3DSORootMacros=T3DSOdir+"root_macros/"
readT3DSO=T3DSOScripts+"readT3DSO.py"

NUM_CH=4
active=[]
CHID=[]
VDIV=[]
detector=[]
OFST=[]
CONFIG_DEFAULT = "T3DSO_config.json"


VERBOSE_DEFAULT=False
#VERBOSE_DEFAULT=True

wait=0.1

verbose=0
def init():
        global verbose
        global NUM_CH,HEADER_MESSAGE
        global active,VDIV,TDIV,TRDL,TRLV,TRSL,CHID,TRSE,TRSC,OFST
        for ch in range(NUM_CH):
                active.append(0)
                VDIV.append("")
                OFST.append("")
                CHID.append("")
                detector.append("")
        if(verbose):
                print(active)
        copy_configfile(config_filename)
        run=1

def make_new_period() -> str:
    p = 0
    while (os.path.isdir("per"+str(p))):
        p += 1
    newper = "per" + str(p)
    cmd = "mkdir " + newper
    #print(cmd)
    subprocess.run(cmd, shell=True)
    cmd = "cp " + config_filename+" "+newper
    #print(cmd)
    subprocess.run(cmd, shell=True)

    
    return newper

def copy_configfile(config_filename):
    # copy config file        
    if (not os.path.exists(config_filename)):
        print(config_filename+" does not exist.")
        cmd="cp "+T3DSOConfigs+CONFIG_DEFAULT+" "+config_filename
        print(cmd)
        subprocess.run(cmd, shell=True)

def readConfig(filename):
        global IP,port,verbose
        global active,VDIV,TDIV,TRDL,TRLV,TRSL,CHID,TRSE,TRSC,OFST
        #if(verbose):
        print("Reading config file ",filename)
        ID=0
        with open(filename) as f:
                d = json.load(f)
                IP=d['T3DSO']['general']['IP']
                port=d['T3DSO']['general']['port']
                TDIV=d['T3DSO']['general']['TDIV']
                TRDL=d['T3DSO']['general']['TRDL']
                #TRLV=d['T3DSO']['general']['TRLV']
                TRSE=d['T3DSO']['general']['TRSE']
                TRSL=d['T3DSO']['general']['TRSL']
                TRSC=d['T3DSO']['general']['TRSC']
                for ch in d['T3DSO']['individual']:
                        CHID[ID]=ch
                        active[ID]=d['T3DSO']['individual'][ch]['active']
                        if(verbose):
                                print(ch,"(active:",active[ID],")")
                        if(active[ID]):
                                detector[ID]=d['T3DSO']['individual'][ch]['detector']
                                VDIV[ID]=d['T3DSO']['individual'][ch]['VDIV']
                                OFST[ID]=d['T3DSO']['individual'][ch]['OFST']
                        ID=ID+1
        print(" IP:",IP)
        print(" port:",port)
        
def run_daq(args):
    stop_flag=False
    config_filename = args.c
    num_of_events = args.n
    num_file_per_period = args.f
    verbose = args.verbose
    ratefile="rate.txt"
            
    print("number of events per file: "+str( num_of_events)+" sec.")
    print("number of files per period: "+str(num_file_per_period))
    copy_configfile(config_filename)
    run=1
    while(stop_flag==False):
        new_per = make_new_period()
        cmd="cp "+config_filename+" "+new_per
        subprocess.run(cmd, shell=True)
        os.chdir(new_per)
        print("***********",new_per,"***********")
        cmd=readT3DSO+" -c "+config_filename+" -n "+str(num_of_events)+" -f "+ str(num_file_per_period)
        if verbose:
                cmd=readT3DSO+" -c "+config_filename+" -n "+str(num_of_events)+" -f "+ str(num_file_per_period)+" -v"

        cp=subprocess.run(cmd, shell=True)
        stop_flag=cp.returncode;
        os.chdir("../")



if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", help="config file name", default=CONFIG_DEFAULT)
    parser.add_argument("-v","--verbose", help="verbose mode", action='store_true')
    parser.add_argument("-n", help="num of eevnts per file", default=1000)
    parser.add_argument("-f", help="num of files per period", default=100)
    args = parser.parse_args()
    config_filename = args.c
    init()
    #copy_configfile(config_filename)
    readConfig(config_filename)
    try:
        run_daq(args)
    except KeyboardInterrupt:
        print()
        print("===========================")
        print("aborted DAQ")
        print("===========================")
    
#main()

