from flask import Flask, render_template
from flask_mqtt import Mqtt
import time

app = Flask (__name__)
app.config['MQTT_BROKER_URL'] = 'mqtt.flespi.io'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = 'YERVPkXy4fbkQuav4MjCqa6FwE9bp0UesAX7KKtDPsNtKLPFnW7BU16et6gJUYLc'
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

@mqtt_client.on_message() #xử lý sự kiện khi nhận được dữ liệu từ broker
def handle_mqtt_message(client, userdata, message):
    data = dict (
        topic=message.topic,
        payload=message.payload.decode()
    )
    print('Received message on topic: {topic} with payload: {payload}'.format(**data))
    

    
@app.route('/')
def index():
    return render_template('index.html')

if __name__ == '__main__':
    app.run(host='192.168.1.197', port=5000, debug=True, threaded=False)


