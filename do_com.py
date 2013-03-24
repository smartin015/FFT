import serial
import threading
import sys
import os

if len(sys.argv) > 1:
    os.system("avrdude -p ATMEGA88P -P COM6 -c buspirate -b 115200 -u -U flash:w:"+sys.argv[2])
    

ser = serial.Serial('COM6', 115200, timeout=1)

continue_thread = True
def readout():
    while(ser and continue_thread):
        val = ser.readline()
        if val == "":
            continue
        sys.stdout.write(val)
    
thread = threading.Thread(target = readout)
thread.start()

cmd = raw_input("> ")
try:
    while (True):
        ser.write(cmd + "\n")
        cmd = raw_input()
except:
    continue_thread = False
    print "Closing thread..."
    thread.join()
    print "Closing serial..."
    ser.close()


