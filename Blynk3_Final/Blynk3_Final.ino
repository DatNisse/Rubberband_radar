#include <SoftwareSerial.h>
#include <Servo.h>
#include <NewPing.h>
#include <BlynkSimpleSerialBLE.h>

#define BLYNK_USE_DIRECT_CONNECT
#define BLYNK_PRINT Serial // Defines the object that is used for printing
#define BLYNK_DEBUG        // Optional, this enables more detailed prints
#define MAX_DISTANCE 200
#define GUNPIN 4
#define FIREPIN 5
#define TRIGPIN 6
#define ECHOPIN 7
#define RADARPIN 8


SoftwareSerial SerialHC06(10, 11); // RX, TX

char auth[] = "YqFp6Zm4JC_0WE4XdkESqpcUebmfdoJS";
BlynkTimer timer;

NewPing sonar(TRIGPIN, ECHOPIN, MAX_DISTANCE);
Servo radarservo, gunservo, fireservo;
int fireCount = 20; //number of times objects been detected, if count is 0, gun fires. this value is used to reduce falsepostivis and missfires.
int rPos = 0, gPos = 0, fPos = 0; //position of servos.
int mPos = 0; //manuall position of gun servo
bool reverse = false; //direction of rotation of radar
int standardInter = 100;
int radret[3];
int radarRef[180]; //referense library, what it scanned first when started
bool manuall = false; //if manuall mode is engaged
bool goStart = true;
int reading = 0;


void setup()
{
  radarservo.attach(RADARPIN);
  gunservo.attach(GUNPIN);
  fireservo.attach(FIREPIN);
  Serial.begin(9600);

  //initiates blynkconnection for HC06 bluetooth module.
  SerialHC06.begin(9600);
  Blynk.begin(SerialHC06, auth);

  timer.setInterval(300L, start);
  timer.setInterval(300L, moveServo);

  //makes first radar sweep for reference points
  radarservo.write(0);

  for (int pos = 0; pos <= 180; pos += 1)
  {
    radarservo.write(pos);
    if (pos < 180)
    {
      radarRef[pos] = ping();
    }
  }

  //after first sweep, resets all servos to startpositions
  fireservo.write(0);
  radarservo.write(0);
  gunservo.write(90);
  fireservo.write(180);
}

void loop()
{
  Blynk.run();
  timer.run(); // Initiates BlynkTimer
}

void moveServo() //function is called to move the servos.
{
  radarservo.attach(RADARPIN);
  gunservo.attach(GUNPIN);
  fireservo.attach(FIREPIN);

  radarservo.write(rPos);
  fireservo.write(fPos);

  if (manuall)
  {
    gunservo.write(mPos);
  }
  else
  {
    gunservo.write(gPos);
  }
}

void start() //main function for determining where the servos are to move.
{
  //determins direction of rotation.
  if (rPos >= 175)
  {
    reverse = true;
  }
  if (rPos <= 5)
  {
    reverse = false;
  }

  //if it's in automatic mode, it starts the rotation.
  if (manuall == false)
  {
    if (fireCount <= 0) //if the target has been confirmed, gunservo is told to fire.
    {
      gunFire();
      fireCount = 20;
    }
    if (reverse == true)
    {
      rPos--;
    }
    else
    {
      rPos++;
    }
    radar();
  }
}

BLYNK_WRITE(V0) //Recives information regarding if the app is in manuall mode or not.
{
  if (manuall == false)
  {
    manuall = true;
    rPos = 0; //resets radar position.
    fireCount = 20; //resets firecount.
  }
  else
  {
    manuall = false;
  }
}

BLYNK_WRITE(V1) //used to reset the firepin after fireing.
{
  if (manuall == true)
  {
    fPos = 0;
  }
}

BLYNK_WRITE(V2) //the value from the slider in blynk app is translated to position for servo to move to
{
  if (manuall == true)
  {
    if (mPos != param.asInt())
    {
      mPos = param.asInt();
      //gunservo.write(mPos);
    }
  }
}

BLYNK_WRITE(V4) //if in manuall mode, fires servo.
{
  if (manuall == true)
  {
    fPos = 180;
  }
}


void gunFire() //tels the firepinservo to turn and thus fire. After fireing, gun is reversed to manualmode for reload and reset.
{
  fPos = 180;
  fireCount = 0;
  manuall = true;
}

void radar() //Logic for the radar detection.
{
  if (rPos <= 180)
  {
    reading = ping(); //Makes call to send out an ping for distance
    if (radarRef[rPos] - 10 > reading) //if something is detected firecount is decresed, when firecount == 0 gun fires.
    {
      if (fireCount < 10) //if target is likely confirmed, orders gunservo to turn to pos.
      {
        gPos = rPos;
        //gunservo.write(pos);
      }
      fireCount--;
    }
    else //if the target was lost, reset firecount to reduce false positivs.
    {
      fireCount = 20;
    }
  }
}

int ping() //ping() sends out the ping from the ultrasonic sensor
{
  //to lessen false possitivs the sensor scans 3 times and uses the avreage value
  for (int i = 0; i < 3 ; i++)
  {
    delay(50);
    unsigned int uS = sonar.ping();
    pinMode(ECHOPIN, OUTPUT);
    digitalWrite(ECHOPIN, LOW);
    pinMode(ECHOPIN, INPUT);
    uS = uS / US_ROUNDTRIP_CM;
    radret[i] = uS;
  }
  int avg = (radret[0] + radret[1] + radret[2]) / 3;
  return avg;
}
