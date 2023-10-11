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

Between these two processes there is never any communication. The propulsion process is only concerned about when the boat should stop and start, and the turning process is only concerned about what angle to set the servo.

##### Propulsion Process
The propulsion process links the ultrasonic sensor and the H-bridge motor driver. It is responsible for controlling the motor and stopping it when there is an obstacle in front of the boat.

##### Turning Process
The turning process links the ESP32-CAM module and the servo. It is responsible for controlling the rudder and turning it in the direction of the red target.

### Design Details
#### ESP32-CAM
[[Dylan]]

#### PWM Function
For both the motor and the servo, a generic PWM function was designed. The function takes in a pin, a wavelength in microseconds, and a percentage float value. The actual implementation of the function will be detailed below in another section.

The PWM function is passed one of three pins, the H-bridge clockwise pin, the H-bridge counter-clockwise pin, or the servo pin. For the servo pin, the wavelength is 20 milliseconds, while for either of the H-bridge pins, the wavelength is 10 milliseconds. The percentage value when the pin is the servo pin changes the angle of the servo, while the percentage value for the clockwise H-bridge pin is always 40% and the percentage value for the counter-clockwise H-bridge pin is always 100%.

#### Propulsion Process
The propulsion process can be split further into two sub-processes: the motor control process, and the obstacle detection process.

##### Motor Control Process
The motor control process has three states, forward running the motor at 40%, backwards at 100%, and stopped.

The first state occurs when there are no obstacles in front of the boat, as well as a red object target to track. The software will then tell the motor to run at 40% speed.

The second state only occurs for a short period of time, and is used when an obstacle has been detected and the boat blips backwards to cancel any forward momentum.

The third state always occurs after the second state, and waits there until the obstacles is cleared. It then transitions back to the first state. The third state also occurs when there is no red target to track.

Again, the software doesn't directly interact with the motor, but instead interacts with the H-bridge motor driver. The motor driver has two inputs, one for clockwise rotation, and one for counter-clockwise rotation. 

##### Obstacle Detection Process
The state the motor is in is determined by the obstacle detection process. The process listens to the ultrasonic sensor and if the distance is less than a certain threshold, it will tell the motor to blip backwards. 

There is a get_distance function that takes in a pointer to an integer. The function will then set the integer to the distance in centimeters. The function is detailed in another section below. After this the distance is checked, and if it is less than 20 centimeters, the motor is told to blip backwards, and stop. Otherwise, the motor is told to run at 40% speed.

### Implementation

#### ESP32-CAM
[[Dylan]]

#### TTGO
#### Pins Used on the TTGO
```c
#define SERVO 2

#define HBRIDGE_CW 12
#define HBRIDGE_CCW 13

#define SENSOR_ECHO 36
#define SENSOR_TRIG 32


void setup () {
  ...
  pinMode(SERVO, OUTPUT);

  pinMode(HBRIDGE_CW, OUTPUT);
  pinMode(HBRIDGE_CCW, OUTPUT);

  pinMode(SENSOR_TRIG, OUTPUT);
  pinMode(SENSOR_ECHO, INPUT);
  ...
}
```

#### PWM Function
```c
// wavelength in microseconds
void pwm(int pin, int wave_length, float percentage) {
  float uptime = wave_length * percentage / 100.0;
  float downtime = wave_length * (100 - percentage) / 100.0;

  digitalWrite(pin, HIGH);
  delayMicroseconds(uptime);
  digitalWrite(pin, LOW);
  delayMicroseconds(downtime);
}
```

Initially, instead of a delayMicrosecond function being used, millis() was used instead and time was tracked. This was done in an effort to reduce blocking. However, this led to issues occurring throughout the code, and it was deemed that the blocking was not a big enough issue to warrant the extra complexity.

#### Motor
In order to control the motor, it is interfaced with the H-bridge motor driver. In order to make the motor spin clockwise, thereby moving the boat forward the H-bridge clockwise pin is set to high while the counter-clockwise pin is set to low. To make the motor spin counter-clockwise, thereby moving the boat backwards, the H-bridge counter-clockwise pin is set to high while the clockwise pin is set to low. To stop the motor, both pins are set to low.

Example of forward motion:
```c
  ...
  digitalWrite(HBRIDGE_CW, LOW);
  digitalWrite(HBRIDGE_CCW, HIGH);
  ...
}

void forwards() { pwm(HBRIDGE_CW, 10000, 40); }
```
Reverse motion:
```c
void abrupt_stop() {
  static long last = millis();
  digitalWrite(HBRIDGE_CW, LOW);
  digitalWrite(HBRIDGE_CCW, HIGH);

  delay(500);
  digitalWrite(HBRIDGE_CCW, LOW);
}
```

Stopped:
```c
  digitalWrite(HBRIDGE_CW, LOW);
  digitalWrite(HBRIDGE_CCW, LOW);
```

#### Z6322 Ultrasonic Distance Sensor
In order to detect obstacles, the ultrasonic sensor is used. The sensor has two pins, one for the trigger, and one for the echo. A get_distance function was written, and takes in a pointer to an integer. The function will then set the integer to the distance in centimeters.
```c
void get_distance(int* p_centimeters) {
  long duration;
  digitalWrite(SENSOR_TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(SENSOR_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(SENSOR_TRIG, LOW);

  duration = pulseIn(SENSOR_ECHO, HIGH);
  *p_centimeters = (duration / 2) / 29.1;
}
```

From the datasheet of the sensor [[DO I REFERENCE]], to start ranging, a 10 microsecond pulse must be sent to the trigger pin. Before this we make sure the trigger pin is low. Then set low again after the pulse to the trigger pin. The sensor will then return a pulse to the echo pin, with the duration of the pulse being the time it took for the pulse to travel to the obstacle and back.

The datasheet then gives a formula to convert the duration of the pulse to centimeters. The formula is:
$$\text {cm} = \frac{\text {duration} \times \text {speed of sound}}{2}$$

Initially, a library was used to interface with the sensor. However, out of interest of wanting to fully understand how the software of the boat worked, the library was removed and the code was written from scratch.