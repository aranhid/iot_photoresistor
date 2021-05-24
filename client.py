import os
import time
import serial
import tempfile
from dateutil import parser
from datetime import datetime
from pymongo import MongoClient
from serial.tools import list_ports

board_id = '228'
com_port = None
client = None
my_temp_dir = None


def get_board_port():
    ports = list_ports.comports()
    for port in ports:
        try:
            s = serial.Serial(port.device, baudrate=115200, timeout=1)
            time.sleep(1)
            s.write(b"i")
            time.sleep(0.1)
            if s.in_waiting > 0:
                answer = s.read_all().decode().strip()
                s.close()
                if answer == board_id:
                    return(port.device)
        except:
            print("Connection is broken")
            find_board()
    return None


def get_data(port):
    try:
        s = serial.Serial(port, baudrate=115200, timeout=1)
        time.sleep(1)
        s.write(b"g")
        time.sleep(0.1)
        if s.in_waiting > 0:
            answer = s.read_all().decode().strip()
            s.close()
            return answer
    except:
        print("Connection is broken")
        find_board()
    return None


def find_board():
    global com_port

    while(True):
        com_port = get_board_port()

        if com_port:
            print("Board detected on " + com_port)
            return com_port
        else:
            print("Board not detected")
        time.sleep(5)


def createMongoClient():
    global client
    if client == None:
        client = MongoClient("roboforge.ru", username="admin",
                             password="pinboard123", authSource="admin",
                             serverSelectionTimeoutMS=5000,
                             socketTimeoutMS=2000)


def sendData(dt, value):
    global my_temp_dir
    file_name = my_temp_dir + "\\file.txt"
    with open(file_name, "a") as file:
        file.write(str(dt) + " " + str(value) + '\n')
        file.close()
    
    data = []
    with open(file_name, "r") as file:
        lines = file.readlines()
        for line in lines:
            *file_dt, file_value = line.strip().split(" ")
            file_dt = " ".join(file_dt)
            file_dt = parser.parse(file_dt)
            file_value = int(file_value)
            data.append({"datetime": file_dt, "value": file_value})
        file.close()

    db = client["tsybulya"]
    coll = db["photoresistor"]
    try:
        print("Send to server")
        coll.insert_many(data)
        with open(file_name, "w") as file:
            file.write("")
            file.close()
    except:
        print("No internet connection")

if __name__ == "__main__":
    my_temp_dir = os.path.join(tempfile.gettempdir(), "tsybulya")
    os.makedirs(my_temp_dir, exist_ok=True)
    createMongoClient()
    print("Connecting...")
    find_board()
    while(True):
        dt = datetime.now()
        if (dt.second == 0):
            print(dt)
            data = get_data(com_port)
            if (data):
                sendData(dt, data)
                print(data)
