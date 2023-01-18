from socket import MsgFlag
from flask import Flask, render_template, request, url_for, redirect
from flask import jsonify # <- `jsonify` instead of `json`

import numpy as np
import pandas as pd
# import numpy as np
import random, time

from paho.mqtt import client as mqtt_client


broker = 'broker.emqx.io'
port = 1883

outTopic = "/DS/flask"      # for sending data through flask
inTopic = "/DS/Arduino"     # for receiving message from Arduino

# lets define control string as the LS 4 bit with
# 7th and 8th bit for quality
# 5th and 6th bit for duration
# 0000 00 00   #0 # do nothing or future use
# 0000 01 01   #5  # default short duration and low quality
# 0000 01 10   #6  # short duration and medium quality
# 0000 01 11   #7 # short duration and high quality
# 0000 10 01   #9 # medium duration and low quality
# 0000 10 10   #10 # medium duration and medium quality
# 0000 10 11   #11  # medium duration and high quality
# 0000 11 01   #13 # long duration and low quality
# 0000 11 10   #14 # long duration and medium quality
# 0000 11 11   #15  # long duration and high quality

controlToArduinoStr = "0"   # to control arduino from flask using MQTT publish

fileNumber = 0

# generate client ID with pub prefix randomly
client_id = f'python-mqtt-{random.randint(0, 100)}'
username = 'emqx'
password = 'public'

flagI = 0
  
def connect_mqtt() -> mqtt_client:
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(client_id)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client

dataPoints = 0
dataList = list()
dataListShared = list()
def getSignalFromMsg(message):   
    global dataPoints 
    keyDataPair = message.split(',')
#    print("Key data pair is")
#    print(keyDataPair)
    # print(keyDataPair)
    # print("data printed")
    for i in keyDataPair:
        if(len(i) < 1):
            continue
        ################## QUICK AND DIRTY for removing noisy readings ############
        #################### Better to use signal filtering approaches ############
        dataPoints += 1
        value = int(i)
#        if( value < 300):
#            continue
        ###########################################################################
        dataList.append(value)
        dataListShared.append(value)        

lastMsgTime = 0        
def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        global lastMsgTime
        lastMsgTime = time.time()

        message = msg.payload.decode()

        getSignalFromMsg(message)


        # print(f"Received " + message + "` from `{msg.topic}` topic")


        global dataPoints
        print("Total data points recieved = " + str(dataPoints))

    client.subscribe(inTopic, 1)
    client.on_message = on_message

import time;
def publish(client):
    global controlToArduinoStr
    msg_count = 0
    msg = controlToArduinoStr
    result = client.publish(outTopic, msg)
    # result: [0, 1]
    status = result[0]
    if status == 0:
        print(f"Send `{msg}` to topic `{outTopic}`")
    else:
        print(f"Failed to send message to topic {outTopic}")
    msg_count += 1

def print_to_csv(data):
    import pandas as pd
    df = pd.DataFrame(data, columns=["signal"])
    # print(df)
    df.to_csv('DS_signal_only.csv', index = False)       


app = Flask(__name__)
client = connect_mqtt()

@app.route('/getData', methods= ['GET', 'POST'])
def stuff():
    global lastMsgTime
    global dataList
    if((time.time() - lastMsgTime) > 10):
        if(len(dataList) > 50):
            print_to_csv(dataList)
            dataList.clear()
    y = dataListShared.copy()
    dataListShared.clear()
    return jsonify(data = y)

def getMessageByte(formDataDuration, formDataQuality):
    if(formDataDuration == "Short : 0.1 sec"):
        if( formDataQuality == "Low"):
            controlString = "5"
        elif(formDataQuality == "Medium"):
            controlString = "6"
        elif(formDataQuality == "High"):
            controlString = "7"
    if(formDataDuration == "Medium : 1 sec"):
        if( formDataQuality == "Low"):
            controlString = "9"
        elif(formDataQuality == "Medium"):
            controlString = "10"
        elif(formDataQuality == "High"):
            controlString = "11"
    if(formDataDuration == "Long : 5 sec"):
            if( formDataQuality == "Low"):
                controlString = "13"
            elif(formDataQuality == "Medium"):
                controlString = "14"
            elif(formDataQuality == "High"):
                controlString = "15"            
    return controlString

@app.route('/sendData', methods= ['GET', 'POST'])
def sendData():
    global dataPoints
    dataPoints = 0
    if request.method == "POST":
        global controlToArduinoStr
        duration = request.form.get("signalDuration")
        quality = request.form.get("signalQuality")
        controlToArduinoStr = getMessageByte(duration, quality)
    publish(client) 
    return redirect(url_for('home'))

@app.route("/")
def home():    
    subscribe(client)
    client.loop_start()
    return render_template("index.html")

if __name__ == "__main__":   
    app.run()