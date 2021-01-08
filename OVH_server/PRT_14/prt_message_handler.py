# cmd to run: nohup python3 /home/ubuntu/PRT_14/prt_message_handler.py &
# to find the running process: ps aux | grep prt_message_handler.py
import paho.mqtt.client as mqtt
import sqlite3

# CHANGE THIS
USERNAME = "xxxx"
PASSWORD = "xxxx"
BROKER_IP = "xxx.xxx.xxx.xxx"
DB_PATH = "YOUR_PATH_TO_SQLite_db_file"

# Receive raw message coming from MQTT broker
# Decode it into 2 values: epoch and power
# The first 4 bytes represente Power in int in mW
# The last 4 bytes represente Epoch in int


def decode_prt_message(message):
    epoch = int.from_bytes(message[4:], byteorder='little')
    power = int.from_bytes(
        message[0:4], byteorder='little', signed=True)/1000.0
    return (epoch, power)

# The callback for when the client receives a CONNACK response from the server.


def on_connect(client, userdata, flags, rc):
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("PRT14/power")

# The callback for when a PUBLISH message is received from the server.


def on_message(client, userdata, msg):
    decodedMessage = decode_prt_message(msg.payload)
    # Check if the message is correctly decoded
    if decodedMessage[0] < 2**32:
        # Insert data into the database
        conn = None
        try:
            conn = sqlite3.connect(DB_PATH, check_same_thread=False)
            sql = ''' INSERT INTO puissance_table(epoch_time,puissance) VALUES(?,?) '''
            cur = conn.cursor()
            cur.execute(sql, decodedMessage)
            conn.commit()
            cur.close()
        except sqlite3.Error as e:
            print(e)
        finally:
            if conn:
                conn.close()
        conn.close()


client = mqtt.Client("decoder_prt14")
client.username_pw_set(USERNAME, PASSWORD)
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER_IP)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()
