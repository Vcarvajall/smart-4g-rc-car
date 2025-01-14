#include "us_sensor.h"
#include "speedometer.h"
#include "camera.h"
#include "drive.h"
#include "steer.h"
#include "imu.h"
#include "messenger.h"

// Buffer for incoming messages
String msg;

// Camera variables
double cameraPosStartTime;
double cameraPosUpdateTime = 250;
bool absoluteMovement = true;
uint16_t panVelocity = 0;
uint16_t tiltVelocity = 0;

// Ultrasonic sensor readings
double frontUS_lastMeasure;
double frontUS_lastUpdate;
double frontUS_readTime = 50; //50 ms between readings
double frontUS_updateTime = 300;
bool frontUsReading = false;
bool frontCollision = false;
float frontStopDist = 10; //cm

double backUS_lastMeasure;
double backUS_lastUpdate;
double backUS_readTime = 50; // 50 ms between readings
double backUS_updateTime = 0;
bool backUsReading = false;
bool backCollision = false;
float backStopDist = 10; //cm

// IMU Variables
bool readImu = false;  // To start continuous imu readings
double imu_lastMeasure;
double imu_lastUpdate;
double imu_readTime = 100; // 100ms between readings
double imu_updateTime = 500;

// Speedometer variables
double speedometer_lastMeasure;
double speedometer_lastUpdate;
double speedometer_readTime = 25; // 25 ms between readings
double speedometer_updateTime = 200;

// To measure the current time
double currTime;

void setup() {
  // Waiting to stablish connection with master
  Serial.begin(115200);
  waitForConnection();

  // Initializing all car components
  speedometerInit();
  driveInit();
  steerInit();
  cameraInit();
  usSensorInit();
  imuInit();

  // Sending signal that all components all initialized
  Serial.println("ready");  
}

void loop() {
  // READ ANY NEW INCOMING COMMANDS
  if(Serial.available() >= 3){  //All commands are at least 2 bytes + '\n'
    msg = Serial.readStringUntil('\n');
    parse_message(msg);
  }

  // UPDATING CURRENT TIME
  currTime = millis();

  // FRONT ULTRASONIC SENSOR MEASUREMENT
  if ((frontUsReading) && (currTime - frontUS_lastMeasure >= frontUS_readTime)){
    USSensorData data = measureFrontDistance();

    // Update the distance value
    if ((frontUS_updateTime >= 0) && (currTime - frontUS_lastUpdate >= frontUS_updateTime)){
      sendUSSensorData(data);
      frontUS_lastUpdate = millis();
    }
    
    // Collision detection
    if (frontCollision){
      if (data.distance < frontStopDist) stopDrive();
    }
    
    frontUS_lastMeasure = millis();
  }

  // BACK ULTRASONIC SENSOR MEASUREMENT
  if ((backUsReading) && (currTime - backUS_lastMeasure >= backUS_readTime)){
    USSensorData data = measureBackDistance();

    // Update the distance value
    if ((backUS_updateTime >= 0) && (currTime - backUS_lastUpdate >= backUS_updateTime)){
      sendUSSensorData(data);
      backUS_lastUpdate = millis();
    }

    // Collision detection
    if (backCollision){
      if (data.distance < backStopDist) stopDrive();
    }
    
    backUS_lastMeasure = millis();
  }

  // IMU MEASUREMENT
  if ((readImu) && (currTime - imu_lastMeasure >= imu_readTime)){
    IMUData data = compute6dof();

    // Update the IMU values
    if ((imu_updateTime >= 0) && (currTime - imu_lastUpdate >= imu_updateTime)){
      sendIMUData(data);
      imu_lastUpdate = millis();
    }

    imu_lastMeasure = millis();
  }

  // RPM MEASUREMENT
  if (currTime - speedometer_lastMeasure >= speedometer_readTime){
    double rpm = computeRPM(currTime - speedometer_lastMeasure);

    // Update the IMU values
    if ((speedometer_updateTime >= 0) && (currTime - speedometer_lastUpdate >= speedometer_updateTime)){
      sendRPM(rpm);
      speedometer_lastUpdate = millis();
    }
    
    speedometer_lastMeasure = millis();
  }

  // MOVE THE CAMERA IF NECESSARY
  if ((!absoluteMovement) && (currTime - cameraPosStartTime >= cameraPosUpdateTime)){
    incrementPanAngle(panVelocity);
    incrementTiltAngle(tiltVelocity);
    cameraPosStartTime = millis();
  }
}

/*void help(){
  Serial.println(F("|---------------------COMMAND SYNTAX---------------------|"));
  Serial.println(F("  {COMMAND}{NUMERICAL VALUE TO PASS THE COMMAND}          "));
  Serial.println(F("  SS130 - Will set the steer direction value to 130°      "));
  Serial.println();
  Serial.println(F("|------------------DIRECTION COMMANDS--------------------|"));
  Serial.println(F("  SG - Get the current steer direction                    "));
  Serial.println(F("  SC - Change steer direcion in a range between 0-180     "));
  Serial.println(F("  SI - Increase the steer direction                       "));
  Serial.println(F("  SD - Decrease the steer direction                       "));
  Serial.println(F("  SC - Center the steer direction                         "));
  Serial.println(F("  SM - Set steer direction max endstop                    "));
  Serial.println(F("  Sm - Set steer direction min endstop                    "));
  Serial.println(F("  Sc - Set steer direction center                         "));
  Serial.println();
  Serial.println(F("|-------------------CAMERA COMMANDS----------------------|"));
  Serial.println(F("  PG - Get the current cam pan angle                      "));
  Serial.println(F("  PS - Set camera pan angle in a range between 0-180      "));
  Serial.println(F("  PI - Increase camera pan angle                          "));
  Serial.println(F("  PD - Decrease camera pan angle                          "));
  Serial.println(F("  PC - Center the camera pan angle                        "));
  Serial.println(F("  PM - Set camera pan angle max endstop                   "));
  Serial.println(F("  Pm - Set camera pan angle min endstop                   "));
  Serial.println(F("  Pc - Set camera pan angle center                        "));
  Serial.println(F("  TG - Get the current cam tilt angle                     "));
  Serial.println(F("  TS - Set camera tilt angle in a range between 0-180     "));
  Serial.println(F("  TI - Increase camera tilt angle                         "));
  Serial.println(F("  TD - Decrease camera tilt angle                         "));
  Serial.println(F("  TC - Center the camera tilt angle                       "));
  Serial.println(F("  TM - Set camera tilt angle max endstop                  "));
  Serial.println(F("  Tm - Set camera tilt angle min endstop                  "));
  Serial.println(F("  Tc - Set camera tilt angle center                       "));
  Serial.println();
  Serial.println(F("|--------------------DRIVE COMMANDS----------------------|"));
  Serial.println(F("  DP - Drive the motor forward in a range between 0-180   "));
  Serial.println(F("  DI - Increase the drive power                           "));
  Serial.println(F("  DD - Decrease the drive power                           "));
  Serial.println(F("  DC - Change the drive direction 1-forward, 0-backward   "));
  Serial.println(F("  DS - Stop the drive                                     "));
  Serial.println(F("  DM - Set drive maximum allowed power                    "));
  Serial.println();
  Serial.println(F("|----------------ULTRASONIC SENSOR COMMANDS--------------|"));
  Serial.println(F("  FO - Front ultrasonic sensor one shot reading           "));
  Serial.println(F("  FC - Front ultrasonic sensor continuous reading         "));
  Serial.println(F("  FS - Stop front ultrasonic sensor reading               "));
  Serial.println(F("  FT - Change minimum time between sensor readings        "));
  Serial.println(F("  BO - Back ultrasonic sensor one shot reading            "));
  Serial.println(F("  BC - Back ultrasonic sensor continuous reading          "));
  Serial.println(F("  BS - Stop back ultrasonic sensor reading                "));
  Serial.println(F("  BT - Change minimum time between sensor readings        "));
  Serial.println();
  Serial.println(F("|----------------------IMU COMMANDS----------------------|"));
  Serial.println(F("  Ic - Calibrate offset values for the imu, takes 5-10m   "));
  Serial.println(F("  IO - IMU one shot reading                               "));
  Serial.println(F("  IC - Read IMU computed values continuously              "));
  Serial.println(F("  IS - Stop continuous IMU readings                       "));
  Serial.println(F("  IT - Change time between continuous IMU readings        "));
  Serial.println();
  Serial.println(F("|-------------COLLISION DETECTION COMMANDS---------------|"));
  Serial.println(F("  FE - Front side collision detection enabled             "));
  Serial.println(F("  FD - Front side collision detection disabled            "));
  Serial.println(F("  F! - Set front emergecy stop threshold distance         "));
  Serial.println(F("  BE - Back side collision detection enabled              "));
  Serial.println(F("  BD - Back side collision detection disabled             "));
  Serial.println(F("  B! - Set back emergecy stop threshold distance          "));
}*/

void parse_message(String msg){
  int16_t input_value = -1;
  String command = msg.substring(0,2);
  if (!isValidCommand(command)) {sendError("Bad commmand"); return;}
  
  if (msg.length() > 2){
    String value = msg.substring(2);
    if (!isInteger(value)) {sendError("Bad value"); return;}
    input_value = value.toInt();
  }
  

  //if (msg == "help"){help();}
  if (command == "SG"){sendResponse(getSteer());}
  else if (command == "SS"){setSteerAngle(input_value);}
  else if (command == "SI"){incrementSteerAngle(input_value);}
  else if (command == "SD"){incrementSteerAngle(-input_value);}
  else if (command == "SC"){centerSteerAngle();}
  else if (command == "SM"){steerConfig("max", input_value);}
  else if (command == "Sm"){steerConfig("min", input_value);}
  else if (command == "Sc"){steerConfig("center", input_value);}

  else if (command == "PG"){sendResponse(getPan());}
  else if (command == "PS"){absoluteMovement=true; setPanAngle(input_value);}
  else if (command == "PI"){incrementPanAngle(input_value);}
  else if (command == "PD"){incrementPanAngle(-input_value);}
  else if (command == "PV"){absoluteMovement=false; panVelocity = input_value;}
  else if (command == "PC"){centerPanAngle();}
  else if (command == "PM"){cameraConfig("pan max", input_value);}
  else if (command == "Pm"){cameraConfig("pan min", input_value);}
  else if (command == "Pc"){cameraConfig("pan center", input_value);}
  
  else if (command == "TG"){sendResponse(getTilt());}
  else if (command == "TS"){absoluteMovement=true; setTiltAngle(input_value);}
  else if (command == "TI"){incrementTiltAngle(input_value);}
  else if (command == "TD"){incrementTiltAngle(-input_value);}
  else if (command == "TV"){absoluteMovement=false; tiltVelocity = input_value;}
  else if (command == "TC"){centerTiltAngle();}
  else if (command == "TM"){cameraConfig("tilt max", input_value);}
  else if (command == "Tm"){cameraConfig("tilt min", input_value);}
  else if (command == "Tc"){cameraConfig("tilt center", input_value);}

  else if (command == "DP"){changeDrivePower(input_value);}
  else if (command == "DI"){incrementDrivePower(input_value);}
  else if (command == "DD"){incrementDrivePower(-input_value);}
  else if (command == "DS"){stopDrive();}
  else if (command == "DC"){changeDriveDirection(input_value);}
  else if (command == "DM"){driveConfig("max", input_value);}
  
  else if (command == "FO"){USSensorData data = measureFrontDistance(); sendUSSensorData(data);}
  else if (command == "FC"){frontUsReading=true;}
  else if (command == "FS"){frontUsReading=false;}
  else if (command == "FT"){frontUS_readTime = input_value; frontUS_lastMeasure = millis();}
  else if (command == "FU"){frontUS_updateTime = input_value; frontUS_lastUpdate = millis();}
  else if (command == "BO"){USSensorData data = measureBackDistance(); sendUSSensorData(data);}
  else if (command == "BC"){backUsReading=true;}
  else if (command == "BS"){backUsReading=false;}
  else if (command == "BT"){backUS_readTime = input_value; backUS_lastMeasure = millis();}
  else if (command == "BU"){backUS_updateTime = input_value; backUS_lastUpdate = millis();}

  else if (command == "Ic"){sendLog("Calibration function not implemented yet...");}
  else if (command == "IO"){IMUData data = compute6dof(); sendIMUData(data);}
  else if (command == "IC"){readImu = true;}
  else if (command == "IS"){readImu = false;}
  else if (command == "IT"){imu_readTime = input_value; imu_lastMeasure = millis();}
  else if (command == "IU"){imu_updateTime = input_value; imu_lastUpdate = millis();}

  else if (command == "FE"){frontCollision=true;}
  else if (command == "FD"){frontCollision=false;}
  else if (command == "F!"){frontStopDist = input_value;}
  else if (command == "BE"){backCollision=true;}
  else if (command == "BD"){backCollision=false;}
  else if (command == "B!"){backStopDist = input_value;}

  else if (command == "RT"){speedometer_readTime = input_value;}
  else if (command == "RU"){speedometer_updateTime = input_value;}
  
  else {sendLog("Unknown command " + command);}
}
