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

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "YqFp6Zm4JC_0WE4XdkESqpcUebmfdoJS";
BlynkTimer timer;

NewPing sonar(TRIGPIN, ECHOPIN, MAX_DISTANCE);
Servo radarservo, gunservo, fireservo;
int fireCount = 20; //number of times objects been detected, if count is 0, gun fires
int rPos = 0, gPos = 0, fPos = 0; //position of servos.
int mPos = 0; //manuall position of gun servo
bool reverse = false; //direction of rotation of radar
int standardInter = 100;
int radret[3];
int radarRef[180]; //referense library, what it scanned first when started
int ledState = HIGH;
bool manuall = false; //if manuall mode is engaged
unsigned long startInter = 0; //used by delayfunction waiter()
unsigned long currrentInter = 0;
bool goStart = true;
int reading = 0;




void setup()
{
  //Debug console
  //DebugSerial.begin(9600);
  //DebugSerial.println("Waiting for connections...");
  radarservo.attach(RADARPIN);
  gunservo.attach(GUNPIN);
  fireservo.attach(FIREPIN);
  Serial.begin(9600);

  //makes first radar sweep for reference points


  SerialHC06.begin(9600);

  //Serial.println("Waiting for connections...");
  Blynk.begin(SerialHC06, auth);
  timer.setInterval(300L, start);
  timer.setInterval(300L, moveServo);
  //timer.setInterval(300L, pusher);
  radarservo.write(0);

  for (int pos = 0; pos <= 180; pos += 1)
  {

    radarservo.write(pos);
    if (pos < 180)
    {
      radarRef[pos] = ping();
    }
  }
  fireservo.write(0);
  
  radarservo.write(0);
  //radarservo.detach();
  
  gunservo.write(90);
  //gunservo.detach();
  
  fireservo.write(180);
  //fireservo.detach();
  
  

}

void loop()
{
  Blynk.run();
  timer.run(); // Initiates BlynkTimer
  
}

void moveServo()
{
  Serial.write("MS");
  radarservo.attach(RADARPIN);
  gunservo.attach(GUNPIN);
  fireservo.attach(FIREPIN);
  
  radarservo.write(rPos);
  fireservo.write(fPos);
  if(manuall)
  {
    gunservo.write(mPos);
  }
  else
  {
    gunservo.write(gPos);
  } 
}
void pusher()
{
  Serial.write("P");
  //detacher();
  radarservo.detach();
  gunservo.detach();
  fireservo.detach();
  Blynk.virtualWrite(V6, reading);
  Blynk.virtualWrite(V3, rPos);
  Blynk.virtualWrite(V5, fireCount);
}

void attacher()
{
  radarservo.attach(RADARPIN);
  gunservo.attach(GUNPIN);
  fireservo.attach(FIREPIN);
}

void detacher()
{
  radarservo.detach();
  gunservo.detach();
  fireservo.detach();
}


void waiter(void func(), int inputInter)
{
  if (goStart == true)
  {
    startInter = millis();
    goStart = false;
  }
  else
  {
    currrentInter = millis();
  }

  if (currrentInter > (startInter + inputInter))
  {
    func();
    goStart = true;
  }

}

void start()
{
  //pusher();
  Serial.write("S");
  if (rPos >= 175)
  {
    reverse = true;
  }
  if (rPos <= 5)
  {
    reverse = false;
  }
  //Blynk.virtualWrite(V3, pos);
  //Blynk.virtualWrite(V5, millis() / 1000);
  if (manuall == false)
  {
    //Blynk.virtualWrite(V5, fireCount);
    if (fireCount <= 0)
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
    //waiter(radar, standardInter);
    //attacher();
    //radarservo.write(pos);
    //moveServo(RADARPIN, pos);

  }
  
  //attacher();
  //moveServo();
}

BLYNK_WRITE(V0)
{
  if (manuall == false)
  {
    manuall = true;
    rPos = 0;
    fireCount = 20;
    //radarservo.write(pos);
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
    //fireservo.write(0);
    fPos = 0;
  }
}

BLYNK_WRITE(V2)
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

BLYNK_WRITE(V4)
{
  if (manuall == true)
  {
    //fireservo.write(180);
    fPos = 180;
  }
}


void gunFire()
{
  //fireservo.write(180);
  fPos = 180;
  fireCount = 0;
  manuall = true;
}

void radar()
{
  if (rPos <= 180)
  {
    reading = ping();
    if (radarRef[rPos] - 10 > reading)
    {
      if (fireCount < 10)
      {
        gPos = rPos;
        //gunservo.write(pos);
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
  //Blynk.virtualWrite(V6, avg);
  return avg;
}
