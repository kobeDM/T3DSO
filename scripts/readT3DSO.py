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
import influxdb
import argparse
import socket
import termios
import errno

#import termios
#import errno
#import rettest
from datetime import timezone, timedelta
JST = timezone(timedelta(hours=+9), 'JST')

#for keyboard interrupt
fd = sys.stdin.fileno()
old = termios.tcgetattr(fd)
new = termios.tcgetattr(fd)
new[3] &= ~termios.ICANON
new[3] &= ~termios.ECHO
termios.tcsetattr(fd, termios.TCSANOW, new)

quit_flag = False
stop_flag = False


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
TRLV=[]
TRPA=""
CONFIG_DEFAULT = "T3DSO_config.json"

RATEFILE="T3DSO_rate.dat"

VERBOSE_DEFAULT=False
#VERBOSE_DEFAULT=True

#wait=0.08
wait=0.02
#IP="10.37.1.39"
#PORT=5024

verbose=0

def key_monitor():
    global quit_flag,stop_flag
    #sys.stdout.write(" key monitor started as a deamon\n")
    #while True:
    #sys.stdout.write(".\n")
    while (1):
        ch = sys.stdin.read(1)
        #sys.stdout.write(".\n")
        if ch == 'q':
            quit_flag = True
            sys.stdout.write("q command was issued. Quitting the DAQ.\n")
            break
        elif ch == 's':
            stop_flag = True
            sys.stdout.write("s command was issued. Stopping the DAQ at the end of this file.\n")
            break


def init():
        global verbose
        global NUM_CH,HEADER_MESSAGE
        global active,VDIV,TDIV,TRDL,TRLV,TRSL,CHID,TRSE,TRSC,OFST
        for ch in range(NUM_CH):
                active.append(0)
                VDIV.append("")
                TRLV.append("")
                OFST.append("")
                CHID.append("")
                detector.append("")
        if(verbose):
                print(active)
        copy_configfile(config_filename)
        readConfig(config_filename)
        monitor_thread = threading.Thread(target=key_monitor)
        monitor_thread.daemon = True
        monitor_thread.start()


def copy_configfile(config_filename):
    # copy config file        
    if (not os.path.exists(config_filename)):
        print(config_filename+" does not exist.")
        cmd="cp "+T3DSOConfigs+CONFIG_DEFAULT+" "+config_filename
        print(cmd)
        subprocess.run(cmd, shell=True)

def readConfig(filename):
        global IP,port,verbose
        global active,VDIV,TDIV,TRDL,TRLV,TRSL,CHID,TRSE,TRSC,OFST,TRPA
        #if(verbose):
        print("Reading config file ",filename)
        ID=0
        with open(filename) as f:
                d = json.load(f)
                IP=d['T3DSO']['general']['IP']
                port=d['T3DSO']['general']['port']
                TDIV=d['T3DSO']['general']['TDIV']
                TRDL=d['T3DSO']['general']['TRDL']
                TRPA=d['T3DSO']['general']['TRPA']
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
                                TRLV[ID]=d['T3DSO']['individual'][ch]['TRLV']
                        ID=ID+1
        print(" IP:",IP)
        print(" port:",port)
        #print(active)
        
def READ_DATA(client_socket,CS):
        command=CS+":WF? DAT2\n"
        client_socket.sendall(command.encode())
        time.sleep(0.08)
        #return client_socket.recv(4*1000*1024*128)
        return client_socket.recv(4*5000*100)

def CREATE_FILE(outfile):
        print(" file: ",outfile)
        f = open(outfile, 'w')
        f.write("")
        f.close()
def STOP_MESSAGE():
    print("press s to stop at the end of this file")
    print("press q to stop immideately")


def WRITE_DATA(response,timestamp,ev,ch,outfile):
        global HEADER_MESSAGE        
        #print("output file: ",outfile)
        f = open(outfile, 'ab')     
        HEADER_MESSAGE_THIS=HEADER_MESSAGE.replace("\n",";")
        HEADER_MESSAGE_THIS+="event "+str(ev)+";"
        HEADER_MESSAGE_THIS+="CHANNEL "+str(ch)+";"
        HEADER_MESSAGE_THIS+="TIMESTAMP "+str(timestamp)+";"
        HEADER_MESSAGE_THIS+="</HEAD>"
        f.write(HEADER_MESSAGE_THIS.encode())
        f.write(response)
        f.close()

def SQL(client_socket,command):
        global wait,verbose
        client_socket.sendall(command.encode())
        time.sleep(wait)
        response=client_socket.recv(4096)
        if(verbose):
                print(command.splitlines()[0],":",response.decode(),end="")
        return response

def COM(client_socket,command):
        global wait
        client_socket.sendall(command.encode())
        time.sleep(wait)


def post_to_influx(file,daemon):
    from influxdb import InfluxDBClient
    client = InfluxDBClient( host     = "10.37.0.227",port     = "8086",database= "liqcf4" )
    if(not os.path.isfile(file)):
        cmd="touch "+file
        subprocess.run(cmd, shell=True)
    while(1):
        with open(file,'r') as f:
            reader=csv.reader(f,delimiter='\t')
            for data in reader:
                json_data = [
                    {
                        'measurement' : 'T3DSO',
                        'fields' : {
                            'file_id'  : int(data[0]),
                            'evnum'  : int(data[1]),
                            'starttime'  : float(data[2]),
                            'endtime'  : float(data[3]),
                            'realtime'  : float(data[4]),
                            'event_rate'  : float(data[5])
                        },
                        'time': datetime.datetime.fromtimestamp(float(data[3])).astimezone(tz=JST).replace(tzinfo=JST).astimezone(tz=timezone.utc),
                        'tags' : {
                            'device' : 'T3DSO'
                        }
                    }
                ]
                result = client.write_points(json_data)
        time.sleep(1)

        
def readT3DSO(args):
    global verbose,wait,IP,port
    global NUM_CH,HEADER_MESSAGE
    global active,VDIV,TDIV,TRDL,TRLV,TRSL,CHID,TRSE,TRSC,OFST,TRPA
    global quit_flag,stop_flag
    print(" ### readT3DSO ###")
    exit_code=0
    config_filename = args.c
    verbose=args.verbose
    num_of_events = int(args.n)
    num_file_per_period = int(args.f)
    #ratefile="rate.txt"
            
    print("number of events per file: "+str( num_of_events))
    print("number of files per period: "+str(num_file_per_period))
    #print(verbose)    
    init()    
    if(verbose):
        print("  connecting to "+IP+":"+str(port))
    STOP_MESSAGE()

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        client_socket.connect((IP, port))
        # T3DSO initialisation
        if(verbose):
            SQL(client_socket,"*IDN?\n")
            SQL(client_socket,"*OPC?\n")
        #COM(client_socket,"RST\n")#reset        
        #time.sleep(1)
        #COM(client_socket,"STOP\n")#stop 

        # global settings
        COM(client_socket,"CHDR LONG\n") # header length of command reply
        command = "MEMORY_SIZE 14M\n"#does not work <size>:={14K,140K,1.4M,14M}
        COM(client_socket,command)
        #if(verbose):
        SQL(client_socket,"MEMORY_SIZE?\n")
        #COM(client_socket,"TRMD SINGLE\n")
        COM(client_socket,"ACQW SAMPLING\n")
        # channel settings
        for ch in range (NUM_CH):
                if active[ch]:
                        print(" ",CHID[ch],"active. ",end="")
                        print("detector:",detector[ch],end="")
                        print(" VDIV:",VDIV[ch],end="")
                        print(" OFST:",OFST[ch],end="")
                        print(" threshold:",TRLV[ch])

        HEADER_MESSAGE_COMMON="ACTIVE CHANNELS: "+str(active)+";"
        HEADER_MESSAGE_COMMON+=" CHANNEL NAMES: ["+CHID[0]+", "+CHID[1]+", "+CHID[2]+", "+CHID[3]+"] "+";"
        
        #time division
        #command = "TDIV 500NS\n"        
        command = "TDIV "+TDIV+"\n"
        HEADER_MESSAGE_COMMON+=command
        COM(client_socket,command)
        #time.sleep(wait)
        
        #time delay
        #command = "TRDL -2US\n"
        command = "TRDL "+TRDL+"\n"
        HEADER_MESSAGE_COMMON+=command
        COM(client_socket,command)
        #time.sleep(wait)

        #trigger coupling
        #COM(client_socket,"C1:TRCP DC\n")
        command = TRSC+":TRCP DC\n"  
        HEADER_MESSAGE_COMMON+=command
        COM(client_socket,command)

        #trigger select
        #print("  TRSE:",TRSE)
        #command = "C2:TRLV 480mv\n"
	
        for ch in range (NUM_CH):
                if active[ch]:
                        print(" ",CHID[ch],"active. ",end="")
                        command = CHID[ch][0]+CHID[ch][2]+":TRCP DC\n"
                        #print(command)
                        COM(client_socket,command)
        #command = "TRSE EDGE,C1\n"
        #HEADER_MESSAGE_COMMON+=command
        #COM(client_socket,command)
        #time.sleep(wait)
                        

        if TRSE == "PA":
            #print("  TRSE:PA")                    
            command = "TRSE "+TRSE+"\n"
            HEADER_MESSAGE_COMMON+=command
            COM(client_socket,command)
            time.sleep(wait)
            command = "TRPA "+TRPA+"\n"
            #command = "TRPA C1,L,STATE,and\n"
            HEADER_MESSAGE_COMMON+=command
            COM(client_socket,command)
            time.sleep(wait)
            #command = "C1:TRLV 50mV\n"
            #command = "set50\n"
            #HEADER_MESSAGE_COMMON+=command
            #COM(client_socket,command)
            #time.sleep(wait)
            #print(command)
            #HEADER_MESSAGE_COMMON+=command
            #COM(client_socket,command)
            #command = "TRSE "+TRSE+"\n"
            #HEADER_MESSAGE_COMMON+=command
            #COM(client_socket,command)
            #time.sleep(wait)
            command = "TRIG_PATTERN?\n"
            print(SQL(client_socket,command).decode())
            #command = "C1:TRIGGER_LEVEL?\n"
            #print(SQL(client_socket,command).decode())

        else:
            command = "TRSE "+TRSE+","+TRSC+"\n"
            HEADER_MESSAGE_COMMON+=command
            COM(client_socket,command)
            #time.sleep(wait)
            # trigger slope
            #COM(client_socket,"TRSL NEG \n")        
            command = TRSC+":TRSL "+TRSL+"\n"
            HEADER_MESSAGE_COMMON+=command
            COM(client_socket,command)
            command = TRSC+":TRLV "+TRLV[int(TRSC[1])-1]+"\n"
            HEADER_MESSAGE_COMMON+=command
            COM(client_socket,command)
            command = "TRIG_LEVEL?\n"
            print(SQL(client_socket,command).decode())
            command = "TRIG_LEVEL2?\n"
            print(SQL(client_socket,command).decode())
            

        #for ch in range (NUM_CH):
            #if active[ch]:
                #command = CHID[ch][0]+CHID[ch][2]+":TRLV "+TRLV[ch]+"\n"
                #COM(client_socket,command)
                #command = CHID[ch][0]+CHID[ch][2]+":TRLV2 "+TRLV[ch]+"\n"
                #COM(client_socket,command)
            #print(command)
            #COM(client_socket,command)
            #command = "C2:TRLV "+TRLV[1]+"\n"
            #COM(client_socket,command)
            #command = "TRSE EDGE,SR,C2\n"
            #COM(client_socket,command)
            #command = "C3:TRLV "+TRLV[2]+"\n"
            #COM(client_socket,command)
            #command = "C4:TRLV "+TRLV[3]+"\n"
            #COM(client_socket,command)


        #trigger levels
        #command = TRSC+":TRLV -20MV\n"
        #command = TRSC+":TRLV "+TRLV+"\n"
        #print(command)
        #HEADER_MESSAGE_COMMON+=command
        #COM(client_socket,command)

        
        #command = "TRSE PA\n"  
        #COM(client_socket,command)
        #
        #COM(client_socket,command)        
        
        #
        #COM(client_socket,command)
        #command = "TRSE "+TRSE+",C1\n"
        #COM(client_socket,command)
        command = "TRIG_SELECT?\n"
        print(SQL(client_socket,command).decode())
        #print(" trigger configuration: source:"+TRSC+", slope:"+TRSL+", level:"+TRLV+"V")
        
        for ID in range (NUM_CH):
                if active[ID]:
                        #VDIV
                        command = "C"+str(CHID[ID][2:])+":VDIV "+VDIV[ID]+"\n"
                        COM(client_socket,command)
                        command="C"+str(CHID[ID][2:])+":VDIV?\n"
                        sql_return=SQL(client_socket,command)
                        HEADER_MESSAGE_COMMON+=sql_return.decode()

                        
                        #VOFFSET
                        command = "C"+str(CHID[ID][2:])+":OFST "+OFST[ID]+"\n"
                        COM(client_socket,command)
                        command="C"+str(CHID[ID][2:])+":OFST?\n"
                        sql_return=SQL(client_socket,command)
                        HEADER_MESSAGE_COMMON+=sql_return.decode()+";"
        
        fileID=0
        COM(client_socket,"TRMD SINGLE\n")
        #COM(client_socket,"TRMD NORM\n")
        #time.sleep(10)
        while(fileID < num_file_per_period):
                DATAFILE="T3DSO_"+str(fileID)+".dat"
                CREATE_FILE(DATAFILE)
                time_from = time.time()
                dt_jst = datetime.datetime.fromtimestamp(time_from, JST) 
                print("  started at "+ str(time_from),end="")
                print(" (",dt_jst.strftime('%Y/%m/%d %H:%M:%S'),")")
                #COM(client_socket,"TRMD SINGLE\n")
                for ev in range(num_of_events):
                        #if(ev% 10 ==0):
                        print(" ",ev,"/",num_of_events,end="\r")
                        HEADER_MESSAGE_EV=HEADER_MESSAGE_COMMON+"EVID"+str(ev)+";"
                        #check for ready to start
                
                        #COM(client_socket,"TRMD SINGLE\n")
                        #time.sleep(wait)
                        while(1):
                                sql_return=SQL(client_socket,"INR?\n").decode()                               
                                if(verbose):
                                    print("INR? ",sql_return)
                                if len(sql_return) < 20 and len(sql_return.split(" "))>1:
                                        if(int(sql_return.split(" ")[1].split("\n")[0])==0):
                                                break
        
                        #SQL(client_socket,"INR?\n")
                        COM(client_socket,"ARM\n")#start
                        time.sleep(0.1)
                        #time.sleep(5)
                        #COM(client_socket,"STOP\n")#stop  

                        #COM(client_socket,"RMD STOP\n")#stop         
                        while(1):
                                sql_return=SQL(client_socket,"INR?\n").decode()                                
                                if(verbose):
                                    print("INR?? ",sql_return)
                                if len(sql_return) < 20 and len(sql_return.split(" "))>1:
                                        if((int((sql_return.split(" ")[1]).split("\n")[0])&0x1)>0):
                                                break
                                        else:
                                            time.sleep(wait)
                                            if(verbose):
                                                print(sql_return)
                                
                                if quit_flag:
                                    sys.stdout.write("q command was issued. Quitting the DAQ.")
                                    sys.stdout.flush()
                                    break
                                #time.sleep(wait)
                                #sql_return=SQL(client_socket,"INR?\n").decode()
                        timestamp = time.time()
                        #if(verbose):
                        #print(" data ready. reading...")
                        sql_return=SQL(client_socket,"SAST?\n").decode()#triggered?
                        HEADER_MESSAGE_EV+=str(sql_return);
                        sql_return=SQL(client_socket,"SARA?\n").decode()#sample rate
                        HEADER_MESSAGE_EV+=str(sql_return);
                        sql_return=SQL(client_socket,"SANU? C1\n").decode()#sample number
                        HEADER_MESSAGE_EV+=str(sql_return);
                        #if int(SQL(client_socket,"INR?\n").decode().split(" ")[1]):
                        for ID in range (NUM_CH):                  
                            if active[ID]:
                                #print(ID," ",str(CHID[ID]))
                                HEADER_MESSAGE="<HEAD>"+HEADER_MESSAGE_EV
                                response=READ_DATA(client_socket,"C"+str(CHID[ID][2:]))
                                WRITE_DATA(response,timestamp,ev,CHID[ID],DATAFILE)
                        #quit_flag=True
                        if quit_flag:
                            sys.stdout.write("q command was issued. Quitting the DAQ.")
                            sys.stdout.flush()
                            break

                time_to = time.time()        
                dt_jst = datetime.datetime.fromtimestamp(time_from, JST) 
                print("  finished at "+ str(time_from),end="")
                print(" (",dt_jst.strftime('%Y/%m/%d %H:%M:%S'),")")
                realtime=int(time_to-time_from)
                if(realtime>0):
                    print("  ",str(ev+1),"events aquired in ",str(realtime),"sec.",f"{(ev+1)/realtime:.2f}"," cps")
                    with open(RATEFILE,mode='a') as f:
                        f.write(str(fileID)+"\t"+str(ev+1)+"\t"+str(time_from)+"\t"+str(time_to)+"\t"+str(realtime)+"\t"+str(f"{(ev+1)/realtime:.2f}")+"\n")
                        
                if(quit_flag or stop_flag):
                        return(1)
                fileID=fileID+1
                
        return(0)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", help="config file name", default=CONFIG_DEFAULT)
    parser.add_argument("-v","--verbose", help="verbose mode", action='store_true')
    parser.add_argument("-n", help="num of eevnts per file", default=100)
    parser.add_argument("-f", help="num of files per period", default=100)
    args = parser.parse_args()
    config_filename = args.c

    #init()
    #copy_configfile(config_filename)
    #readConfig(config_filename)
    #monitor_thread = threading.Thread(target=key_monitor)
    #monitor_thread.daemon = True
    #monitor_thread.start()
    influx_thread=threading.Thread(target=post_to_influx,args=("T3DSO_rate.dat","daemon"),daemon=True)
    influx_thread.start()

    exit_code=readT3DSO(args)
    termios.tcsetattr(fd, termios.TCSANOW, old)

    sys.exit(exit_code)
