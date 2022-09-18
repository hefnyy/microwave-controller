//motor
#define EN          5   //orange wire
#define IN1         7   //green wire
#define IN2         4   //purple wire

//lcd
#include <LiquidCrystal.h>
const int RS=15,E=14,DB4=19,DB5=18,DB6=17,DB7=16;
LiquidCrystal lcd(RS,E,DB4,DB5,DB6,DB7);
//RS orange, E white, DB4 yellow, DB5 green, DB6 indigo, DB7 brown

//led
#define RED         10   //red wire
#define YELLOW      11   //yellow wire
#define GREEN       12   //green wire

//buzzer
#define BUZZER      9    //gray wire

//push buttons
#define BUTTON1     2     //brown wire
#define BUTTON2     3     //brown wire
#define BUTTON3     4     //brown wire
#define DEBOUNCE    200

//sensor
#define TRIG        13    //turquoise wire
#define ECHO        6     //turquoise wire
#define SOUND_SPEED 343

//motor functions
void motorOnOff();
void (*reset)() = 0x0000;
void pauseOrCont();
void END();

//lcd functions
void getTime(); //for manual mode
void updateGetTime(int counter); //updates hr:min:sec format on the screen as user's inputs.
void countdown(int timeArr[3]); //displays countdown
void twoDigitPrint(int number); // eg. 9 --> 09
void mode(); //displays cooking modes
void getKilos(); //for auto mode
void timeCalculation(); //calculates time from kilos

//led functions
void ledOn (int ledColor);
void ledOff (int ledColor);
void blink(String type);

//buzzer functions
void buzzerOn();
void buzzerOff();

//sensor functions
bool isDistanceSmall(); //returns true if distance less than 10cm , false otherwise
void safety(); //implements safety measures if distance is less than 10cm


//global variables
bool motorState=0;
bool button1State=0;
bool button2State=0;
int Time[3]={0,0,0}; //{ hr , min , sec }
int timeInSec;
int hours , minutes , seconds;
int choice;
int operation;
float kilos;

void setup()
{
  noInterrupts();

  Serial.begin(9600);
  lcd.begin(16,2);

  pinMode(EN , OUTPUT);
  pinMode(IN1 , INPUT);
  pinMode(IN2 , INPUT);

  digitalWrite(EN , LOW);
  digitalWrite(IN1 , HIGH); 
  digitalWrite(IN2 , LOW);
  
  pinMode(RED , OUTPUT);
  pinMode(YELLOW , OUTPUT);
  pinMode(GREEN , OUTPUT);
  
  ledOff(RED);
  ledOff(GREEN);
  ledOff(YELLOW);
  
  pinMode(BUZZER , OUTPUT);
  buzzerOff();

  pinMode(BUTTON1 , INPUT_PULLUP);
  pinMode(BUTTON2 , INPUT_PULLUP);
  pinMode(BUTTON3 , INPUT_PULLUP);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG , LOW);
  
  //interrupt pin change button3
  PCICR |=0B00000100; //enable port D
  PCMSK2|=0B00010000; //enable pin  4

  attachInterrupt(digitalPinToInterrupt(2) , motorOnOff , RISING);
  attachInterrupt(digitalPinToInterrupt(3) , pauseOrCont , RISING);

  interrupts();
}

void loop()
{
  if (motorState==0)
  {
    ledOn(RED);
  }
  
  else if(motorState==1)
  {
    ledOn(GREEN);
    ledOff(RED);
    buzzerOn();
    delay(2000);
    buzzerOff();
    
    lcd.setCursor(3,0);
    lcd.print("Microwave");
    delay(2000);
    lcd.clear();
    
    mode();
    getKilos(); //will implement in case of auto
    timeCalculation();
    digitalWrite(EN,motorState);
  }
}

//motor functions

void motorOnOff()
{
  if (button1State==0)
  {
    motorState = 1; 
    button1State = 1;
  }
  else
  {
    reset();
  }
  delay(DEBOUNCE);
}

void pauseOrCont()
{
  if (button2State==0)
  {
    motorState=0;
    lcd.setCursor(4,1);
    button2State = 1;
    lcd.println("Paused");
  }
  else
  {
    motorState=1;
    button2State=0;
    lcd.setCursor(4,1);
    lcd.println("                ");
  }
  delay(DEBOUNCE);
}


void END()
{
  reset();
}

// lcd functions

void getTime()
{
  String timeUnits[3] = {"Hours:" , "Minutes:" , "Seconds:"};
  lcd.setCursor(0,0);
  lcd.print("Enter time:");
  lcd.setCursor(0,1);
  lcd.print("hr:min:sec");
  for (int i=0 ; i<3 ; i++)
  {
    do
    {
      Serial.println(timeUnits[i]);
      while (Serial.available()==0);
      Time[i]=Serial.parseInt();
      updateGetTime(i); //updates according to input
      
    }
    while (Time[i]<0 || Time[i]>60); //keeps asking for the time unit if it's invalid
  }
  lcd.clear();
}

void updateGetTime(int counter)
{
  if (Time[counter]>=0 && Time[counter]<=60)
  {
    switch (counter)
    {
      case 0: //updates hours
      lcd.setCursor(0,1);
      twoDigitPrint(Time[counter]);
      break;
    
      case 1: //updates minutes
      lcd.setCursor(3,1);
      lcd.print(" ");
      lcd.setCursor(4,1);
      twoDigitPrint(Time[counter]);
      break;
    
      case 2: //updates seconds
      lcd.setCursor(7,1);
      lcd.print(" ");
      lcd.setCursor(8,1);
      twoDigitPrint(Time[counter]);
      delay(1000);
      break;
    }
  }
}

void countdown(int Time[3])
{
  motorState=1;
  ledOn(YELLOW);
  for (hours=Time[0] ; hours>=0 ; hours--)
  {
    lcd.setCursor(4,0);
    twoDigitPrint(hours);
    lcd.setCursor(6,0);
    lcd.print(':');
    
    for (minutes=Time[1] ; minutes>=0 ; minutes--)
    {
      lcd.setCursor(7,0);
      twoDigitPrint(minutes);
      lcd.setCursor(9,0);
      lcd.print(':');      
     
      for (seconds=Time[2] ; seconds>=0 ; seconds--) //loops through seconds
      {
        safety(); //checking for distance limitation
        lcd.setCursor(10,0);
        twoDigitPrint(seconds);
        delay(1000);
        digitalWrite(EN , motorState); //checking for the motor state for cases of pausing
        blink("yellow"); //will only blink if paused
        ledOn(YELLOW); //on during cooking
      }
      Time[2]=59; //resets seconds so it always starts back from 59 after first loop
      if ( Time[0] > 0 )
      {
        Time[1] = 59;
      }
    }
  }
  digitalWrite(EN , LOW);
  blink("all");
  
}

void twoDigitPrint(int number)
{
  if (number<10)
  {
    lcd.print("0");
    lcd.print(number);
  }
  else
  {
    lcd.print( number );
  }
}

void mode()   // automatic or manual
{
  lcd.clear();
  
  lcd.setCursor(0,0);
  lcd.print("1-Auto");
  lcd.setCursor(0,1);
  lcd.print("2-Manual");
  
  Serial.println ("Choose the mode:");
  while(Serial.available()==0);
  choice = Serial.parseInt();
  
  while (choice != 1 && choice!=2)
  {
    Serial.println("INVALID, Choose mode again:") ;
    while(Serial.available()==0);
    choice = Serial.parseInt();
  }
  
  if (choice==1)   // automatic then chicken or meat
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("1-Chicken");
    lcd.setCursor(0,1);
    lcd.print("2-Meat");
    Serial.println ("Choose the cooking operation:");
    while(Serial.available()==0);
    operation = Serial.parseInt();
    while (operation != 1 && operation!=2)
    {
      Serial.println("INVALID, Choose cooking operation again:") ;
      while(Serial.available()==0);
      operation = Serial.parseInt();
    }
  }
  else if (choice==2) //calls the manual functions
  {
    getTime();
    countdown(Time);
  }
}

void getKilos()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("number of kilos:");
  
  Serial.println("Enter number of kilos:");
  while(Serial.available()==0);
  kilos= Serial.parseFloat();
  
  while (kilos <= 0 || kilos>99 )
  {
    Serial.println("INVALID, Enter number of kilos again:") ;
    while(Serial.available()==0);
    kilos = Serial.parseFloat();
  }  
  
  lcd.setCursor(0,1);
  lcd.print(kilos); 
  delay(1000);
}

void timeCalculation()
{
  if (operation == 1) //chicken cooking time
  {
    int Time[3]={0,0,0};
    timeInSec = kilos*1000/250*10;
    Time[0] = timeInSec/3600;
    Time[1] = timeInSec/60;
    Time[2] = timeInSec%60;
    lcd.clear();
    countdown(Time);
   }
   
  else if (operation == 2) //meat cooking time
  {
    timeInSec = kilos*1000/500*15;
    Time[0] = timeInSec/3600;
    Time[1] = timeInSec/60;
    Time[2] = timeInSec%60;
    lcd.clear();
    countdown(Time);
  }  
}

//led functions

void ledOn (int ledColor)
{
  digitalWrite (ledColor , HIGH);
}

void ledOff (int ledColor)
{
  digitalWrite (ledColor , LOW);
}

void blink(String type)
{
  if (type == "all")
  {
    while(true)
    {
      ledOn(RED);
      ledOn(YELLOW);
      ledOn(GREEN);
      buzzerOn();
      
      delay(1000);
       
      ledOff(RED);
      ledOff(YELLOW);
      ledOff(GREEN);
      buzzerOff();
      
      delay(1000);
    }
  }
  else if (type == "yellow")
  {
    while(button2State==1)
    {
      ledOff(YELLOW);
      delay(1000);
      ledOn(YELLOW);
      delay(1000);
    }
  }
}

//buzzer functions

void buzzerOn()
{
  digitalWrite(BUZZER,HIGH);
}

void buzzerOff()
{
  digitalWrite(BUZZER,LOW);
}

//sensor functions

void safety()
{
  while(isDistanceSmall() == true)
  {
   motorState=0;
   ledOff(YELLOW);
   buzzerOn();
   
   ledOn(RED);
   delay(500);
   ledOff(RED);
   delay(500);
   
   lcd.setCursor(3,1);
   lcd.println("Stay back!      ");
  }
  
   motorState=1; 
   ledOn(YELLOW);
   buzzerOff();
   
   lcd.setCursor(3,1);
   lcd.println("                ");  
}

bool isDistanceSmall()
{
  digitalWrite(TRIG , HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG , LOW);
  
  int totalTime = pulseIn(ECHO, HIGH); //in microseconds
  float timeInSeconds = totalTime * 0.000001 / 2.0 ;
  int distance = timeInSeconds * SOUND_SPEED * 100;

  if ( distance<=10 )
  {
   return true;
  }
  else
  {
   return false;
  }
}

//button3 pinchange interrupt
  
  ISR(PCINT2_vect)
{
  if(digitalRead(4) == 0)
  {
    reset();
  }
  delay(DEBOUNCE);
}
