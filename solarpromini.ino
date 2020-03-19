
#include <stdio.h>
#include "LowPower.h" // https://github.com/rocketscream/Low-Power

#define DEBUG false


#define arr_len( x )  ( sizeof( x ) / sizeof( *x ) )


#define STEP_IDLE           0
#define STEP_SEARCH         100
#define STEP_SEARCH_ERROR  -100
#define STEP_SEARCH_LEFT    105
#define STEP_SEARCH_BREAK   110
#define STEP_SEARCH_RIGHT   115
#define STEP_SEARCH_END     199


#define DIR_STOP          0
#define DIR_FW            1
#define DIR_BW            2
#define DIR_BREAK         3


#define pinLED              12
#define pinMotLeft_FW       9
#define pinMotLeft_BW       6
#define pinMotRight_FW      11
#define pinMotRight_BW      10
#define pinAnalogSource     8
#define pinAnalog           A0


int intStep = STEP_SEARCH;
//int intStep = STEP_IDLE;

int intThreshold = 20;


unsigned long ulLEDTime = 0;
unsigned long ulInfoTime = 0;
unsigned long ulStartTime = 0;
unsigned long ulWakeups = 0;

int intLight = 0;
int intMaxLight = 0;

#define MEAS_VALUES 5
int intMeasArray[MEAS_VALUES];


void setup()
{
  pinMode(pinLED, OUTPUT);
  digitalWrite(pinLED, HIGH);

  pinMode(pinAnalogSource, OUTPUT);
  digitalWrite(pinAnalogSource, LOW);

  pinMode(pinMotLeft_FW, OUTPUT);
  pinMode(pinMotLeft_BW, OUTPUT);
  pinMode(pinMotRight_FW, OUTPUT);
  pinMode(pinMotRight_BW, OUTPUT);

  digitalWrite(pinMotLeft_FW, LOW);
  digitalWrite(pinMotLeft_BW, LOW);
  digitalWrite(pinMotRight_FW, LOW);
  digitalWrite(pinMotRight_BW, LOW);

  delay(300);
  Serial.begin(115200);
  Serial.println("");
  Serial.println("solarpromini 20190502");

  digitalWrite(pinLED, LOW);

  ulLEDTime = millis();
  ulInfoTime = millis();
  ulStartTime = millis();
}


void loop()
{
  if (intStep != STEP_IDLE)
  {
    if (timeOut(ulLEDTime, 200))
    {
      ulLEDTime = millis();

      digitalWrite(pinLED, !digitalRead(pinLED));
    }
  }



  String strCommand = "";

  while (Serial.available())
  {
    char c = Serial.read();
    strCommand += c;
  }

  if (strCommand != "")
  {
    strCommand.trim();
    strCommand.toLowerCase();
    Serial.println("command: \"" + strCommand + "\"");

    if (strCommand == "idle")
    {
      intStep = STEP_IDLE;
    }
    else if (strCommand == "s")
    {
      intStep = STEP_SEARCH;
    }
    else if (strCommand == "m")
    {
      Serial.println(measLight());
    }
    else if (strCommand.indexOf("t") != -1)
    {
      if (strCommand.length() > 1)
      {
        strCommand.remove(0, 1);
        intThreshold = strCommand.toInt();
      }

      Serial.println(intThreshold);
    }
    else if (strCommand == "d")
    {

    }
  }



  switch (intStep)
  {
    default:
      Serial.println("STEP: " + String(intStep));
      intStep = STEP_IDLE;
      break;

    case STEP_IDLE:
      if (timeOut(ulInfoTime, 2000) == true) // 2 seconds
      {
        ulInfoTime = millis();

        Serial.println("IDLE");
      }


      ulWakeups += 1;

      if (ulWakeups * 8 > 1800) // 30 minutes
      {
        if (measLight() > 150)
        {
          intStep = STEP_SEARCH;
        }
        else
        {
          ulWakeups = 1;
          PowerSave();
        }
      }
      else
      {
        PowerSave();
      }
      break;

    case STEP_SEARCH:
      Serial.println("SEARCH");

      intMaxLight = 0;

      intStep = STEP_SEARCH_LEFT;
      ulStartTime = millis();
      break;

    case STEP_SEARCH_LEFT:
      Serial.println("SEARCH LEFT");

      Motor(true, DIR_BW, 25);
      //Motor(false, DIR_FW, 25);

      intLight = measLight();

      if (intLight > intMaxLight)
      {
        intMaxLight = intLight;
      }


      if ((intLight + intThreshold  + 20 < intMaxLight) || (timeOut(ulStartTime, 10000) == true))
      {
        intMaxLight = 0;

        intStep = STEP_SEARCH_BREAK;
        ulStartTime = millis();
      }
      break;

    case STEP_SEARCH_BREAK:
      Serial.println("SEARCH BREAK");

      Motor(true, DIR_STOP, 0);
      //Motor(false, DIR_STOP, 0);

      if (timeOut(ulStartTime, 500) == true)
      {
        intStep = STEP_SEARCH_RIGHT;
        ulStartTime = millis();
      }
      break;

    case STEP_SEARCH_RIGHT:
      Serial.println("SEARCH RIGHT");

      Motor(true, DIR_FW, 15);
      //Motor(false, DIR_BW, 15);

      intLight = measLight();

      if (intLight > intMaxLight)
      {
        intMaxLight = intLight;
      }


      if ((intLight + intThreshold < intMaxLight) || (timeOut(ulStartTime, 10000) == true))
      {
        intStep = STEP_SEARCH_END;
        ulStartTime = millis();
      }
      break;

    case STEP_SEARCH_END:
      Serial.println("SEARCH END");

      Motor(true, DIR_STOP, 0);
      //Motor(false, DIR_STOP, 0);

      intStep = STEP_IDLE;
      ulWakeups = 0;
      break;

    case STEP_SEARCH_ERROR:
      Serial.println("error: SEARCH");

      intStep = STEP_SEARCH_END;
      break;
  }
}


void Motor(bool left, byte dir, byte speed)
{
  if (false)
  {
    if (left)
    {
      Serial.print("Motor left ");
    }
    else
    {
      Serial.print("Motor right ");
    }

    switch (dir)
    {
      case DIR_STOP:
        Serial.println("STOP");
        break;

      case DIR_FW:
        Serial.print("FW ");
        Serial.println(speed);
        break;

      case DIR_BW:
        Serial.print("BW ");
        Serial.println(speed);
        break;

      case DIR_BREAK:
        Serial.println("BREAK");
        break;
    }
  }


  if (left)
  {
    switch (dir)
    {
      case DIR_STOP:
        analogWrite(pinMotLeft_FW, 0);
        analogWrite(pinMotLeft_BW, 0);
        break;

      case DIR_FW:
        analogWrite(pinMotLeft_FW, speed);
        analogWrite(pinMotLeft_BW, 0);
        digitalWrite(pinMotLeft_BW, LOW);
        break;

      case DIR_BW:
        analogWrite(pinMotLeft_FW, 0);
        digitalWrite(pinMotLeft_FW, LOW);
        analogWrite(pinMotLeft_BW, speed);
        break;

      case DIR_BREAK:
        analogWrite(pinMotLeft_FW, 0);
        analogWrite(pinMotLeft_BW, 0);
        break;
    }
  }
  else
  {
    switch (dir)
    {
      case DIR_STOP:
        analogWrite(pinMotRight_FW, 0);
        analogWrite(pinMotRight_BW, 0);
        break;

      case DIR_FW:
        analogWrite(pinMotRight_FW, speed);
        analogWrite(pinMotRight_BW, 0);
        break;

      case DIR_BW:
        analogWrite(pinMotRight_FW, 0);
        analogWrite(pinMotRight_BW, speed);
        break;

      case DIR_BREAK:
        analogWrite(pinMotRight_FW, 0);
        analogWrite(pinMotRight_BW, 0);
        break;
    }
  }
}


void PowerSave()
{
  digitalWrite(pinLED, LOW);
  Motor(true, DIR_STOP, 0);
  Motor(false, DIR_STOP, 0);

  Serial.println("sleep " + String(ulWakeups * 8));
  delay(10);

  if (DEBUG == false)
  {
    LowPower.powerSave(SLEEP_8S, ADC_ON, BOD_ON, TIMER2_OFF);
  }
  else
  {
    delay(8000);
  }
}


int measLight()
{
  digitalWrite(pinAnalogSource, HIGH);

  for (int i = 0; i < arr_len(intMeasArray); i += 1)
  {
    delay(1);
    Meas(analogRead(pinAnalog));
  }

  digitalWrite(pinAnalogSource, LOW);

  int intOutput = average(intMeasArray);

  Serial.println(intOutput);

  return intOutput;
}


void Meas(int val)
{
  for (int i = arr_len(intMeasArray) - 1; i >= 0; i -= 1)
  {
    intMeasArray[i] = intMeasArray[i - 1];
  }

  intMeasArray[0] = val;

  /*for (int i = 0; i < arr_len(intMeasArray); i += 1)
    {
    Serial.println(intMeasArray[i]);
    }*/
}


float average(int arr[])
{
  float fOutput = 0.0;

  for (int i = 0; i < arr_len(intMeasArray); i += 1)
  {
    fOutput += intMeasArray[i];
  }

  fOutput = fOutput / arr_len(intMeasArray);

  /*Serial.println("");
    Serial.println(fOutput);*/

  return fOutput;
}


bool timeOut(unsigned long starttime, unsigned long timeOut)
{
  if ((millis() - starttime) < timeOut)
  {
    /*Serial.print("TimeOut: ");
      Serial.print(millis() - starttime);
      Serial.print(" / ");
      Serial.print(timeOut);
      Serial.println("ms");*/
    return false;
  }
  else
  {
    return true;
  }
}
