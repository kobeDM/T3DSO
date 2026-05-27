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

IP_DEFAULT="10.37.1.39"
PORT_DEFAULT=5024

CONFIG_DEFAULT = "MCA_config.json"
#VERBOSE_DEFAULT=False
VERBOSE_DEFAULT=True

wait=0.1


def READ_DATA(client_socket,command):
        client_socket.sendall(command.encode())
        time.sleep(wait)
        response=client_socket.recv(20*1024*1024)
        f = open('test.dat', 'wb')
        f.write(response)
        f.close()
        #recv = listclient_socket.read_raw())[15:]
        #The head of message: MATH:WF ALL. These are followed by the string
        # #9000000700, the beginning of a binary block in which nineASCII integers
        #are used to give the length of the block (700 bytes). The point number is
        #700 with interpolation. After the length of block, is beginning of the wave
        #data. “0A 0A” means the end of data.
        raw=response.decode().splitlines()
        print(raw[0])
        #print(len(raw))
        
        #data=response.decode().splitlines()[1]
        #print(data)
        for i in range (10):
            print(raw[i])
            #print("{0:02x}".format(raw[i])," ",end=" ")
        #sl2_file_size = len(response)
        #print(response.decode())
        
def SQL(client_socket,command):
        global wait
        client_socket.sendall(command.encode())
        time.sleep(wait)
        response=client_socket.recv(4096)
        print(command.splitlines()[0],":",response.decode(),end="")

def COM(client_socket,command):
        global wait
        client_socket.sendall(command.encode())
        time.sleep(wait)


        
def read_T3DSO():
    global verbose,wait
    print(" ### readT3DSO ###")
    exit_code=0
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", help="config file name", default=CONFIG_DEFAULT)
    parser.add_argument("-v","--verbose", help="verbose mode", action='store_true',default=VERBOSE_DEFAULT)    
    parser.add_argument("-i","--IP", help="IP address",default=IP_DEFAULT)    
    parser.add_argument("-p","--port", help="port",default=PORT_DEFAULT)    
    args = parser.parse_args()
    config_filename = args.c
    verbose=args.verbose
    IP=args.IP
    port=args.port
    if(verbose):
        print("  connecting to "+IP+":"+str(port))
    
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        client_socket.connect((IP, port))

        SQL(client_socket,"*IDN?\n")
        SQL(client_socket,"*OPC?\n")
        #COM(client_socket,"RST\n")        
        command = "MSIZ 14M\n"
        COM(client_socket,command)
        SQL(client_socket,"MEMORY_SIZE?\n")
        
        COM(client_socket,"ACQW SAMPLING\n")
        COM(client_socket,"TRMD SINGLE\n")
        #COM(client_socket,"TRMD NORM\n")
        COM(client_socket,"TRSL POS \n")
        command = "TDIV 500NS\n"
        COM(client_socket,command)
        COM(client_socket,"C1:TRCP DC\n")
        command = "C1:TRLV 500MV\n"
        COM(client_socket,command)
        command = "C1:VDIV 200MV\n"
        COM(client_socket,command)
        COM(client_socket,"ARM_ACQUISITION\n")#start
        time.sleep(3)

        SQL(client_socket,"INR?\n")
        SQL(client_socket,"SAST?\n")#triggered?
        SQL(client_socket,"SARA?\n")#sample rate
        SQL(client_socket,"SANU? C1\n")#sample number
        READ_DATA(client_socket,"C1:WF? DAT2\n")
        SQL(client_socket,"C1:VDIV?\n")


    return exit_code

if __name__ == '__main__':
    exit_code=read_T3DSO()
