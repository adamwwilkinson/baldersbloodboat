## Software

### Introduction
The purpose of the software embedded in the boat is to carry out two simple operations:
- if there is a red target in sight, the boat should maneuver straight towards it, else stop the motor
- if there is an obstacle in the front of the boat, stop in time to avoid collision

To do these operations several components are needed:
- a TTGO T-Display module to control the motor, the rudder, and the ultrasonic sensor
- an ESP32-CAM module to detect the red target
- a Z6322 ultrasonic distance sensor mounted in front of detect obstacles
- a L9110 Bridge Motor Driver to control the direction of the motor
- a YM2707 motor for the propulsion itself (although in the code this is not directly interacted with)
- a Z6392 servo to control the rudder (this is directly interacted with in the code)

### Software Requirements
#### Obstacle Detection
The software should provide a way to detect and prevent collisions with obstacles directly in front of it. There isn't a way for the boat to move in any direction but forward, so this is the only direction that needs to be monitored.

It should listen to the ultrasonic sensor and if the distance is less than a certain threshold, it should manipulate the motor to prevent the boat from colliding with the obstacle.

#### Tracking the Red Target
[[Dylan]]

#### Propeller Control
The software should provide instructions on how to control the motor. Its speed should be either two values 0, or 40%, and the software should be able to decide which one to use as well as how to control the direction of the motor.

The direction can be controlled by the L9110 Bridge Motor Driver. The motor driver has two inputs, one for each direction. The software should provide a way to control these inputs.

The 40% value is motivated by the fact the whole system is powered by a 9V battery, and uses a voltage regulator to bring the volts down to 5. A motor at running anywhere near max speed would draw too much current and cause the voltage regulator to overheat. The 40% value is a compromise between speed and current draw.

#### Rudder Control
The software should provide instructions on how to control the rudder. The servo should be constraint to the minimum and maximum angles that the rudder can turn to. The software should provide a way to control the angle of the servo.

The servo should listen to a value given by the camera in relation to where the red object is, and turn the rudder accordingly.

### System Architecture
Within the boat there exist two main components: the ESP32-CAM module and the TTGO. The ESP32-CAM module is responsible for detecting the red target processing that information, and sending it to the TTGO. The TTGO is responsible for the rest of the functions on the boat.

#### ESP32-CAM
[[Dylan]]

#### TTGO
The TTGO is the main component of the boat. It is responsible for controlling the motor, the rudder, and the ultrasonic sensor. It is also responsible for receiving information from the ESP32-CAM module and processing it. All these functions can be grouped into two processes: the propulsion process and the turning process.

Between these two processes there is never any communication. The propulsion process is only concerned about when the boat should stop and start, and the turning process is only concerned about when the boat should turn left or right.

##### Propulsion Process
The propulsion process links the ultrasonic sensor and the H-bridge motor driver. It is responsible for controlling the motor and stopping it when there is an obstacle in front of the boat.

##### Turning Process
The turning process links the ESP32-CAM module and the servo. It is responsible for controlling the rudder and turning it in the direction of the red target.

### Design Details
#### ESP32-CAM
[[Dylan]]

#### Propulsion Process
The propulsion process can be split further into two sub-processes: the obstacle detection process and the motor control process.

##### Obstacle Detection Process

##### Motor Control Process
The motor control process has three states, forward running the motor at 40%, backwards at 100%, and stopped.
The second state only occurs for a short period of time, and is used when an obstacle has been detected and the boat blips backwards to cancel any forward momentum.

The first state occurs when there is no o



