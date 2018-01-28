


class Flasher
{
  // Class Member Variables
  // These are initialized at startup
  int ledPin;      // the number of the LED pin
  long OnTime;     // milliseconds of on-time
  long OffTime;    // milliseconds of off-time

  // These maintain the current state
  int ledState;                 // ledState used to set the LED
  unsigned long previousMillis;   // will store last time LED was updated

  // Constructor - creates a Flasher 
  // and initializes the member variables and state
  public:
  Flasher(int pin, long on, long off)
  {
  ledPin = pin;
  pinMode(ledPin, OUTPUT);     
    
  OnTime = on;
  OffTime = off;
  
  ledState = LOW; 
  previousMillis = 0;
  }

  void Update()
  {
    // check to see if it's time to change the state of the LED
    unsigned long currentMillis = millis();
     
    if((ledState == HIGH) && (currentMillis - previousMillis >= OnTime))
    {
      ledState = LOW;  // Turn it off
      previousMillis = currentMillis;  // Remember the time
      digitalWrite(ledPin, ledState);  // Update the actual LED
    }
    else if ((ledState == LOW) && (currentMillis - previousMillis >= OffTime))
    {
      ledState = HIGH;  // turn it on
      previousMillis = currentMillis;   // Remember the time
      digitalWrite(ledPin, ledState);   // Update the actual LED
    }
  }
};


Flasher led1(12, 500, 500);
Flasher led2(11, 350, 350);


const int switchPin = 6; //input switch
boolean switchState = false;

void setup()
{
  Serial.begin(9600);
  pinMode(switchPin,INPUT);
  digitalWrite(switchPin, HIGH);
}

void loop(){
  digitalRead(switchPin);

  if(switchPin == LOW){
    switchState = false;
  }else{switchState = true;}
   Serial.println(switchState);
   if (switchState == true) {
    led1.Update();
    led2.Update();
    delay(1000);
   }else {led1.Update();}
         
  }
