from flask import Flask, render_template,url_for,request,redirect,make_response
import json
from flask_mqtt import Mqtt
from time import time
import datetime
from bson import objectid
from pymongo import MongoClient
from random import random

# get current time
time_buf = datetime.datetime.now() #timestamp

app = Flask (__name__)

app.config['MQTT_BROKER_URL'] = 'mqtt.flespi.io'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = 'L6cuIYqQjTyE5hCyaR9zqRe2Efy7FpNejPwPDMtKwPfpXFNXmKkArmrWqPRT2uX9'
app.config['MQTT_PASSWORD'] = ''
app.config['MQTT_REFRESH_TIME'] = 1.0 # refresh time in seconds
app.config['MQTT_KEEPALIVE'] = 5

topic_1 = 'sensor/humidity' #topic chứa giá trị độ ẩm
topic_2 = 'sensor/temperature' #topic chứa giá trị nhiệt độ
topic_3 = 'sensor/rain_ammount' #topic chứa giá trị lượng mưa

mqtt_client = Mqtt(app) 

#xử lý các sự kiện nếu server connect với broker thì sẽ gọi hàm handle_connect
@mqtt_client.on_connect()
def handle_connect(client, userdata, flags, rc):
    if rc == 0:
        print('Connected successfully')
        mqtt_client.subscribe(topic_1) #subcriber topic1
        mqtt_client.subscribe(topic_2) #subcriber topic2
        mqtt_client.subscribe(topic_3) #subcriber topic3
    else:
        print('Bad  connection. Code:', rc)

#Study note
#The client is a client object.

#rc (return code) is used for checking that the connection was established. (see below).

#Note: It is also common to subscribe in the on_connect callback
client = MongoClient("")

db = client.sensors_db #select the database
todos = db.sensors #select collectionname

#Dict object
#humidity_payload =  str(list(todos.find({"topic": topic_1}).sort([('timestamp', -1)]).limit(1))).split(',') #dữ liệu khởi tạo nên là dữ liệu cuối cùng được nhận trong database
#humidity_payload = humidity_payload[2].split(":")
#humidity_payload = humidity_payload [1].replace("'","")

humidity_payload = 0

#temperature_payload = str(list(todos.find({"topic": topic_2}).sort([('timestamp', -1)]).limit(1))).split(',')
#temperature_payload = temperature_payload[2].split(":")
#temperature_payload = temperature_payload[1].replace("'","")

temperature_payload = 0

#rain_payload = str(list(todos.find({"topic": topic_3}).sort([('timestamp', -1)]).limit(1))).split(',')
#rain_payload = rain_payload[2].split(":")
#rain_payload = rain_payload[1].replace("'","")

rain_payload = 0
#Lấy dữ liệu cuối cùng trong database push lên web khi vừa khởi động flask server

@mqtt_client.on_message() #xử lý sự kiện khi nhận được dữ liệu từ broker
def handle_mqtt_message(client, userdata, message):
    if message.topic == topic_1:     
        global humidity_payload
        humidity_payload = message.payload.decode()
        todos.insert_one({'topic': message.topic, 'value': message.payload.decode(),'timestamp':time_buf.strftime("%Y-%m-%d %H:%M:%S")})
    elif message.topic == topic_2:
        global temperature_payload
        temperature_payload = message.payload.decode()
        todos.insert_one({'topic': message.topic, 'value': message.payload.decode(),'timestamp':time_buf.strftime("%Y-%m-%d %H:%M:%S")})
    elif message.topic == topic_3:
        global rain_payload 
        rain_payload = message.payload.decode()
        todos.insert_one({'topic': message.topic, 'value': message.payload.decode(),'timestamp':time_buf.strftime("%Y-%m-%d %H:%M:%S")})
    data = dict (
        topic=message.topic,
        payload=message.payload.decode()
    ) 
    #todos.insert_one({'topic': message.topic, 'value': message.payload.decode(),'timestamp':time_buf.strftime("%Y-%m-%d %H:%M:%S")})
    print('Received message on topic: {topic} with payload: {payload}'.format(**data))
        
@app.route('/')
def index():
    return render_template('index.html')


@app.route('/data', methods=["GET", "POST"]) #xử lý gửi data cho template xử lý hiển thị đồ thị live load
def data():
    # Data Format
    # [TIME, Temperature, Humidity, Rain]
    Temperature = int(float(temperature_payload))
    Humidity = int(float(humidity_payload))
    Rain = int(float(rain_payload))

    data = [time()*1000, Temperature, Humidity,Rain]

    response = make_response(json.dumps(data))

    response.content_type = 'application/json'

    return response


@app.get("/update_humid")
def update_humid():
   humid = str(humidity_payload)
   return humid

@app.get("/update_temperature")
def update_temperature():
   temperature = str(temperature_payload)
   return temperature

@app.get("/update_rain")
def update_rain():
   rain = str(rain_payload)
   return rain


if __name__ == '__main__':
    app.run(host="192.168.1.197", port=5000, debug=True, threaded=False)
    #socketio.run(app, host=local_ip, port=5000, use_reloader=False, debug=False)



