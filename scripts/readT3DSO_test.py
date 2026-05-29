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
#import termios
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

NUM_CH=4
active=[]
CHID=[]
VDIV=[]
detector=[]
OFST=[]
CONFIG_DEFAULT = "T3DSO_config.json"


#VERBOSE_DEFAULT=False
VERBOSE_DEFAULT=True

wait=0.1
#IP="10.37.1.39"
#PORT=5024
def init():
        global NUM_CH,HEADER_MESSAGE
        global active,VDIV,TDIV,TRDL,TRLV,TRSL,CHID,TRSE,TRSC,OFST
        for ch in range(NUM_CH):
                active.append(0)
                VDIV.append("")
                OFST.append("")
                CHID.append("")
                detector.append("")
        print(active)
        HEADER_MESSAGE="<HEAD>"

def copy_configfile(config_filename):
    # copy config file        
    if (not os.path.exists(config_filename)):
        print(config_filename+" does not exist.")
        cmd="cp "+T3DSOConfigs+CONFIG_DEFAULT+" "+config_filename
        print(cmd)
        subprocess.run(cmd, shell=True)

def readConfig(filename):
        global IP,port
        #global active,VDIV,TDIV,TRDL,TRLV,TRSL,CHID,TRSE,TRSC
        global active,VDIV,TDIV,TRDL,TRLV,TRSL,CHID,TRSE,TRSC,OFST
                
        #global active,detector,VDIV,TDIV,TRDL,TRLV,TRSL,CHID
        #global MCA_type
        print("Reading config file ",filename)
        ID=0
        with open(filename) as f:
                d = json.load(f)
                IP=d['T3DSO']['general']['IP']
                port=d['T3DSO']['general']['port']
                TDIV=d['T3DSO']['general']['TDIV']
                TRDL=d['T3DSO']['general']['TRDL']
                TRLV=d['T3DSO']['general']['TRLV']
                TRSE=d['T3DSO']['general']['TRSE']
                TRSL=d['T3DSO']['general']['TRSL']
                TRSC=d['T3DSO']['general']['TRSC']
                for ch in d['T3DSO']['individual']:
                        CHID[ID]=ch
                        active[ID]=d['T3DSO']['individual'][ch]['active']
                        print(ch,"(active:",active[ID],")")
                        if(active[ID]):
                                detector[ID]=d['T3DSO']['individual'][ch]['detector']
                                VDIV[ID]=d['T3DSO']['individual'][ch]['VDIV']
                                OFST[ID]=d['T3DSO']['individual'][ch]['OFST']
                        ID=ID+1
        print(" IP:",IP)
        print(" port:",port)
        #print(active)
        
def READ_DATA(client_socket,CS):
        command=CS+":WF? DAT2\n"
        client_socket.sendall(command.encode())
        time.sleep(wait)
        #response=
        return client_socket.recv(4*1024*1024)

def CREATE_FILE(outfile):
        print("create file: ",outfile)
        f = open(outfile, 'w')
        f.write("")
        f.close()

def WRITE_DATA(response,ch,outfile):
        global HEADER_MESSAGE
        print("output file: ",outfile)
        f = open(outfile, 'ab')        
        HEADER_MESSAGE_THIS=HEADER_MESSAGE.replace("\n"," ")
        HEADER_MESSAGE_THIS+="CHANNEL CH"+ch
        HEADER_MESSAGE_THIS+="</HEAD>"
        f.write(HEADER_MESSAGE_THIS.encode())
        f.write(response)
        f.close()

def SQL(client_socket,command):
        global wait
        client_socket.sendall(command.encode())
        time.sleep(wait)
        response=client_socket.recv(4096)
        print(command.splitlines()[0],":",response.decode(),end="")
        return response

def COM(client_socket,command):
        global wait
        client_socket.sendall(command.encode())
        time.sleep(wait)


        
def read_T3DSO(args):
    global verbose,wait,IP,port
    global NUM_CH,HEADER_MESSAGE
    global active,VDIV,TDIV,TRDL,TRLV,TRSL,CHID,TRSE,TRSC,OFST
    print(" ### readT3DSO ###")
    exit_code=0
    config_filename = args.c
    verbose=args.verbose
    if(verbose):
        print("  connecting to "+IP+":"+str(port))
    
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        client_socket.connect((IP, port))
        # T3DSO initialisation
        SQL(client_socket,"*IDN?\n")
        SQL(client_socket,"*OPC?\n")
        #COM(client_socket,"RST\n")
        COM(client_socket,"STOP\n")#stop 

        # global settings
        COM(client_socket,"CHDR LONG\n")
        command = "MEMORY_SIZE 14M\n"#does not work <size>:={14K,140K,1.4M,14M}
        COM(client_socket,command)
        SQL(client_socket,"MEMORY_SIZE?\n")
        COM(client_socket,"TRMD SINGLE\n")
        COM(client_socket,"ACQW SAMPLING\n")
        # channel settings
        for ch in range (NUM_CH):
                if active[ch]:
                        print(CHID[ch]," active. ",end="")
                        print("detector: ",detector[ch],end="")
                        print(" VDIV:",VDIV[ch],end="")
                        print(" OFST:",OFST[ch])

        HEADER_MESSAGE+="ACTIVE CHANNELS: "+str(active)
        HEADER_MESSAGE+=" CHANNEL NAMES: ["+CHID[0]+", "+CHID[1]+", "+CHID[2]+", "+CHID[3]+"] "


        #COM(client_socket,"TRSL NEG \n")        
        #HEADER_MESSAGE+=command
        #COM(client_socket,command)
        
        #time division
        #command = "TDIV 500NS\n"        
        command = "TDIV "+TDIV+"\n"
        HEADER_MESSAGE+=command
        COM(client_socket,command)
        
        #time delay
        #command = "TRDL -2US\n"
        command = "TRDL "+TRDL+"\n"
        HEADER_MESSAGE+=command
        COM(client_socket,command)

        #trigger coupling
        #COM(client_socket,"C1:TRCP DC\n")
        command = TRSC+":TRCP DC\n"        
        COM(client_socket,command)

        #trigger setting
        command = TRSE+","+TRSC+"\n"
        COM(client_socket,command)

        #trigger level
        command = TRSC+":TRLV "+TRLV+"\n"
        HEADER_MESSAGE+=command
        #command = TRSC+":TRLV -20MV\n"
        COM(client_socket,command)
        #command = "C1:VDIV 50MV\n"
        for ID in range (NUM_CH):
                if active[ID]:
                        command = "C"+str(CHID[ID][2:])+":VDIV "+VDIV[ID]+"\n"
                        COM(client_socket,command)        
                        command = "C"+str(CHID[ID][2:])+":OFST "+OFST[ID]+"\n"
                        COM(client_socket,command)
                        command="C"+str(CHID[ID][2:])+":VDIV?\n"
                        sql_return=SQL(client_socket,command)
                        HEADER_MESSAGE+=sql_return.decode()
                        command="C"+str(CHID[ID][2:])+":OFST?\n"
                        sql_return=SQL(client_socket,command)
                        HEADER_MESSAGE+=sql_return.decode()
                        
                


        #check for ready to start

        while(int(SQL(client_socket,"INR?\n").decode().split(" ")[1])):
                time.sleep(wait)
         
        #COM(client_socket,"TRMD SINGLE\n")
        COM(client_socket,"TRMD NORM\n")

        COM(client_socket,"ARM\n")#start
        time.sleep(3)

        COM(client_socket,"RMD STOP\n")#stop 

        SQL(client_socket,"SAST?\n")#triggered?
        SQL(client_socket,"SARA?\n")#sample rate
        SQL(client_socket,"SANU? C1\n")#sample number
        #SQL(client_socket,"INR?\n")
        DATAFILE="data.dat"
        CREATE_FILE(DATAFILE)
        if int(SQL(client_socket,"INR?\n").decode().split(" ")[1]):
                print(" data ready. reading...")
                for ID in range (NUM_CH):                  
                        if active[ID]:                                
                                response=READ_DATA(client_socket,"C"+str(CHID[ID][2:]))
                                WRITE_DATA(response,str(CHID[ID][2:]),DATAFILE)


    return exit_code

def run_daq(args):
    global stop_flag
    config_filename = args.c
    #presettime = args.presettime
    num_file_per_period = args.f
    ratefile="rate.txt"
            
    #print("preset time for one file: "+str(presettime)+" sec.")
    print("number of files per period: "+str(num_file_per_period))
    copy_configfile(config_filename)
    run=1


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", help="config file name", default=CONFIG_DEFAULT)
    parser.add_argument("-v","--verbose", help="verbose mode", action='store_true')
    parser.add_argument("-f", help="num of files per period", default=60)
    args = parser.parse_args()

    config_filename = args.c

    init()
    copy_configfile(config_filename)
    readConfig(config_filename)


    run_daq(args)
    exit_code=read_T3DSO(args)

