#include <SoftwareSerial.h>
#include <Servo.h>
#include <NewPing.h>
#include <BlynkSimpleSerialBLE.h>

#define BLYNK_USE_DIRECT_CONNECT
#define BLYNK_PRINT Serial // Defines the object that is used for printing
#define BLYNK_DEBUG        // Optional, this enables more detailed prints
#define MAX_DISTANCE 200
#define GUNPIN 12
#define FIREPIN 11
#define TRIGPIN 10
#define ECHOPIN 9
#define RADARPIN 7


SoftwareSerial SerialHC06(11, 10); // RX, TX

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "YqFp6Zm4JC_0WE4XdkESqpcUebmfdoJS";
int startTimer;
BlynkTimer timer;

NewPing sonar(TRIGPIN, ECHOPIN, MAX_DISTANCE);
Servo radarservo, gunservo, fireservo;
int fireCount = 20; //number of times objects been detected, if count is 0, gun fires
int pos = 0; //position of radar servo, used to aim gunservo
int mPos = 0; //manuall position of radar
bool reverse = false; //direction of rotation of radar
int radret[3];
int radarRef[180]; //referense library, what it scanned first when started
int ledState = HIGH;
bool manuall = false; //if manuall mode is engaged

Servo servo1;

void start()
{
  if (pos >= 175)
  {
    reverse = true;
  }
  if (pos <= 5)
  {
    reverse = false;
  }
  Blynk.virtualWrite(V3, pos);
  //Blynk.virtualWrite(V5, millis() / 1000);
  if (manuall == false)
  {
    Blynk.virtualWrite(V5, fireCount);
    if (fireCount <= 0)
    {
      gunFire();
      fireCount = 20;
    }
    if (reverse == true)
    {
      radar();
      pos--;
    }
    else
    {
      radar();
      pos++;
    }
    radarservo.write(pos);

  }
}


void setup()
{
  //Debug console
  //DebugSerial.begin(9600);
  //DebugSerial.println("Waiting for connections...");
  radarservo.attach(RADARPIN);
  gunservo.attach(GUNPIN);
  fireservo.attach(FIREPIN);
  pinMode(12, OUTPUT); //LED
  Serial.begin(9600);

  //makes first radar sweep for reference points
  radarservo.write(0);
  delay(1000);
  for (pos = 0; pos <= 180; pos += 1)
  {

    radarservo.write(pos);
    if (pos < 180)
    {
      radarRef[pos] = ping();
    }
  }
  pos = 0;
  radarservo.write(0);
  gunservo.write(90);
  fireservo.write(180);
  delay(5000);

  SerialHC06.begin(9600);
  servo1.attach(8);
  //Serial.println("Waiting for connections...");
  Blynk.begin(SerialHC06, auth);
  startTimer = timer.setInterval(300L, start);

}

void loop()
{
  Blynk.run();
  timer.run(); // Initiates BlynkTimer
}

BLYNK_WRITE(V0)
{
  if (param.asInt() == 1)
  {
    manuall = true;
    pos = 0;
    fireCount = 20;
    radarservo.write(pos);
  }
  else
  {
    manuall = false;
  }
}

BLYNK_WRITE(V1)
{
  if (manuall == true)
  {
    fireservo.write(0);
  }
}

BLYNK_WRITE(V2)
{
  if (manuall == true)
  {
    if (pos != param.asInt())
    {
      mPos = param.asInt();
      gunservo.write(mPos);
    }
  }
}

BLYNK_READ(V3)
{
  //Blynk.virtualWrite(V3, pos);

}

BLYNK_WRITE(V4)
{
  if (manuall == true)
  {
    fireservo.write(180);
  }
}


void gunFire()
{
  fireservo.write(0);
  delay(3000);
  fireservo.write(180);
  delay(3000);
  fireCount = 0;
}

void radar()
{
  if (pos <= 180)
  {
    int reading = ping();
    if (radarRef[pos] - 10 > reading)
    {
      if (fireCount < 10)
      {
        gunservo.write(pos);
      }
      fireCount--;
    }
    else
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
  //  Serial.print("Ping: ");
  //  Serial.print(avg);
  //  Serial.println("cm");
  //  Serial.print("Reference: ");
  //  Serial.print(radarRef[pos]);
  //  Serial.println("cm");
  Blynk.virtualWrite(V6, avg);
  return avg;
}
