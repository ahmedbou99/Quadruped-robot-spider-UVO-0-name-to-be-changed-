1. Overview
The broker runs Mosquitto on Windows with TLS 1.2 certificates bound to the hostname mqtt.local.
2. Broker Connection
2.1 Host & Ports
All clients must connect using the hostname mqtt.local (not an IP address). The TLS certificate is issued with CN=mqtt.local and SAN covering mqtt.local and localhost.

Protocol        | Port	 | TLS	             |  Usage
MQTT	        | 1883	 |No (internal only )| ESP32-CAM local tests
MQTT + TLS      | 8883	 |Yes (TLS 1.2)	     |ESP32-CAM production
WebSocket + TLS | 9001	 |Yes (TLS 1.2)	     |Web Dashboard
3. Authentication

Client      	|   Username	  | Password	|        Role
ESP32-CAM	    |   spider_robot  |	spider123	|     Hardware client
Web Dashboard   |	dashboard	  | dash123 	|     Frontend client
ESP32 (dev)     |  	esp32	      | esp32       |   	Dev / superuser

3.2 TLS Certificate
Clients connecting on port 8883 or 9001 must trust the broker CA certificate. The CA file is:  ca.crt  (obtain from MQTT admin)

4. Access Control (ACL)
User          |	         Publish / topic write	                 | Subscribe / topic read
spider_robot  |robot/spider/cam/stream                       	 |robot/spider/cmd/#
dashboard	  |robot/spider/cmd/#                                |robot/spider/cam/stream  robot/spider/status 
esp32	      |# (all topics — dev only)	                     |# (all topics — dev only)

5. Topic Structure
Topic                  	   | Direction        	     |Description
robot/spider/cmd/gait	   | dashboard → robot	     |Set walking gait mode
robot/spider/cmd/speed	   | dashboard → robot	     |Set movement speed
robot/spider/cmd/direction | dashboard → robot	     |Set movement direction
robot/spider/cmd/rotation  | dashboard → robot       |Set body rotation
robot/spider/cam/stream	   | robot → dashboard       |Camera stream data
robot/spider/status        | broker (LWT)→ dashboard |Online/offline status (LWT)

5.2  Payload Formats : 

robot/spider/cmd/gait
{ "gait": "tripod" }
Allowed values: "tripod", "wave", "ripple", "crawl"

robot/spider/cmd/speed

{ "speed": 50 }
speed: integer 0–100 (percentage of max speed)

robot/spider/cmd/direction

{ "direction": "forward" }
Allowed values: "forward", "backward", "left", "right", "stop"

robot/spider/cmd/rotation

{ "angle": 45 }
angle: integer -180 to 180 (degrees, clockwise positive)
 
robot/spider/cam/stream
{ "frame": "<base64_jpeg>", "timestamp": 1714500000 }
frame: base64-encoded JPEG image. timestamp: Unix epoch seconds.

robot/spider/status  (LWT)
{ "status": "online" }   or   { "status": "offline" }
Published automatically by the broker via Last Will and Testament (LWT) when the robot connects or disconnects unexpectedly.

6. Last Will & Testament (LWT)

Topic  : robot/spider/status
Payload: { "status": "offline" }
QoS    : 1
Retain : true

7. QoS Guidelines
Topic category    |Recommended QoS	
cmd/* (movement)  |QoS 1	
cam/stream	      |QoS 0	
status (LWT)	  |QoS 1	
8. Integration Notes
8.1 Hardware Team (ESP32-CAM + Arduino Uno R3)
•	Connect to mqtt.local:8883 using TLS with the provided ca.crt.
•	Use username spider_robot with password from admin.
•	Subscribe to robot/spider/cmd/# on startup.
•	Forward received commands over Serial UART to Arduino Uno R3.
•	Configure LWT before connecting (see Section 6).
•	Arduino Uno R3 operates at 5V logic — use a voltage divider or level shifter on ESP32-CAM RX pin (3.3V).

8.2 Dashboard Team (Web Frontend)
•	Connect via WebSocket TLS: wss://mqtt.local:9001
•	Use username dashboard with password from admin.
•	Subscribe to  robot/spider/cam/stream.
•	Publish commands to robot/spider/cmd/* .
•	Add mqtt.local → 127.0.0.1 to the hosts file on the dev machine.
