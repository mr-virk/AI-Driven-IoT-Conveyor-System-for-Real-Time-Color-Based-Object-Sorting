#forimage processing
import cv2
import numpy as np
import requests
from io import BytesIO
#for MQTT
import random
import time

from paho.mqtt import client as mqtt_client

#MQTT Server
broker = 'xyz'
port = 8883
topic = "topic"
# generate client ID with pub prefix randomly
client_id = f'python-mqtt-{random.randint(0, 1000)}'
username = 'user'
password = 'pass'
ca_cert_path = './emqxsl-ca.crt'
############
# Define the ESP32-CAM URL
esp32cam_url = 'http://0.0.0.0/capture' # ENter IP of ESP32-CAM

####### MQTT #################################
def connect_mqtt():
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION2, client_id)
    client.tls_set(ca_certs=ca_cert_path)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client

def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")

    client.subscribe(topic)
    client.on_message = on_message

####### MQTT END-1 #################################

# Fetch the image from the ESP32-CAM
response = requests.get(esp32cam_url)

# Convert the image bytes to numpy array
image_array = np.frombuffer(response.content, dtype=np.uint8)

# Decode the image
image = cv2.imdecode(image_array, cv2.IMREAD_COLOR)

# Convert BGR to RGB
image_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

# Convert the image to HSV
image_hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)

# Define color ranges in HSV
lower_red = np.array([0, 50, 50])
upper_red = np.array([10, 255, 255])
lower_green = np.array([35, 50, 50])
upper_green = np.array([85, 255, 255])
lower_blue = np.array([100, 50, 50])
upper_blue = np.array([130, 255, 255])

# Create masks
mask_red = cv2.inRange(image_hsv, lower_red, upper_red)
mask_green = cv2.inRange(image_hsv, lower_green, upper_green)
mask_blue = cv2.inRange(image_hsv, lower_blue, upper_blue)

# Extract color pixels
red_pixels = cv2.bitwise_and(image_rgb, image_rgb, mask=mask_red)
green_pixels = cv2.bitwise_and(image_rgb, image_rgb, mask=mask_green)
blue_pixels = cv2.bitwise_and(image_rgb, image_rgb, mask=mask_blue)

# Check which color has more pixels
red_count = np.count_nonzero(mask_red)
green_count = np.count_nonzero(mask_green)
blue_count = np.count_nonzero(mask_blue)

if red_count > green_count and red_count > blue_count:
    color_detected = "Red"
elif green_count > red_count and green_count > blue_count:
    color_detected = "Green"
else:
    color_detected = "Blue"

print("Detected color:", color_detected)

####### MQTT #################################
def publish(client):
    time.sleep(1)
    if len(color_detected) <= 6:  # Check if the length of the message is 6 letters or less
        result = client.publish(topic, color_detected)  # Publish the message to the specified topic
        # result: [0, 1] - Indicates the result of the publish operation, where 0 means success and 1 means failure
        status = result[0]  # Extract the status code from the result
        if status == 0:  # Check if the status code indicates success
            print(f"Sent `{color_detected}` to topic `{topic}`")  # Print a success message if the message was sent successfully
        else:
            print(f"Failed to send message to topic {topic}")  # Print a failure message if the message failed to send
    else:
        print("Message should be 6 letters or less.")

def run():
    client = connect_mqtt()
    client.loop_start()
    publish(client)
    client.loop_stop()

run()
