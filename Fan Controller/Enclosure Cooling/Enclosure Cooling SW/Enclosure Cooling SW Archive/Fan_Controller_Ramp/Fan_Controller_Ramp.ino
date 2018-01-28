/*
Fan Controller Code Version 0.1
Designed to run on an ATTiny84 processor


I/O DEFINITIONS
A0 - Battery Voltage
A1 - Ambient Temperature Thermistor
A2 - Battery Temperature Thermistor
D0 - Used as Analog Input
D1 - Used as Analog Input
D2 - Used as Analog Input
D3 - Fan LED 
D4 - SCK Operating Mode LED Blue Cooling
D5 - MISO Operating Mode LED Red Heating
D6 - MOSI Operating Mode LED Grn OK
D7 - Fan On/Off Control PWM
D8 - Fan contact input (INT0 pin)
D9 - Not Used
D10 - Fan Rotor
*/


/*----------------------------------------------------------
 *                      Define Variables 
 *----------------------------------------------------------- */                  

 const int fanContact = 8; //INT 0
 const int rotor = 10;
 const int fanControl = 7; //PWM
 const int okLED = 6; //green operating mode LED
 const int heatLED = 5; //red heat LED
 const int coolLED = 4; //blue coolLED
 const int fanLED = 3; //green fan LED
 const int battery = A0; //analog input battery voltage measurement
 const int thermistorAmb = A1; //analog input ambient temperature measurement
 const int thermistorBatt = A2; //analog input battery temperature measurement
 const int thermistorNominal=10000;
 const int temperatureNominal=25; 
 const int bCoefficient=3950;
 const int seriesResistor=10000;//nominal value,measure for better accuracy
 int coolLEDState = LOW; //used in LED flash code
 int heatLEDState = LOW;
 int okLEDState = LOW;
 int fanLEDState = LOW;
 byte dutyCycle = 0; //used in analog write to fan control
 byte mode = 0; //define operating mode 0=OK 1=HEAT 2=COOL
 // add variable for use in delay timing 
 float sensorValue = 0.0; 
 float battVolt = 0.0;        //battery voltage 
 float battTemperature = 0.0; //main system battery temperature
 float ambTemperature = 0.0; //main system battery enclosure temperature
 float steinhart = 0.0;
 float temperature = 0.0;
 boolean fanOn = false;      //fan status running/not running
 volatile boolean fanFault = false;  //fan locked rotor signal from ISR
 boolean thermistorFault = false; //used in thermistor check
 boolean voltageFault = false; //used in low voltage disconnect
 unsigned long startTime = 0; //used in delay timers
 unsigned long waitTime1 = 333; //delay 333ms
//End of Variables Definition ------------------------- 


/*----------------------------------------------
 * FUNCTION DEFINITIONS & ISRs
 -----------------------------------------------*/
//readSensor function
 float readSensor(int sensorPin) {
   float sensorValue = 0;
   for (int i=0; i<5; i++) {
     sensorValue = sensorValue + analogRead(sensorPin);
     delay(10);
   }
   sensorValue = constrain(sensorValue, 0, 5115);
   sensorValue = map(sensorValue, 0, 5115, 0, 1023);
   return sensorValue;
 }   

//thermistor convert function
  float thermistorConvert(float sensorValue){ // can (float sensorValue) be changed to (sensorValue)?
  sensorValue = 1023 / sensorValue - 1;
  sensorValue = seriesResistor / sensorValue;
  steinhart = sensorValue / thermistorNominal;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= bCoefficient;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (temperatureNominal + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart; // Invert
  steinhart -= 273.15;  // convert to C
  steinhart = steinhart * 9.0 / 5.0 +32;         // convert to F
  temperature = steinhart;
  return temperature;
}

//fan slow ramp turn on function - 1.28 second ramp
void fanRampOn(){
   analogWrite(fanControl,64);
   startTime = millis(); //set startTime for delay loop
   while ((startTime + waitTime1) > millis() ) {
   } //250 mS delay
   analogWrite(fanControl,128);
   startTime = millis(); //set startTime for delay loop
   while ((startTime + waitTime1) > millis() ) {
   } //250 mS delay
   analogWrite(fanControl,192);
   startTime = millis(); //set startTime for delay loop
   while ((startTime + waitTime1) > millis() ) {
   } //250 mS delay
   analogWrite(fanControl,255);
   digitalWrite(fanLED, HIGH); //turn on fan LED
   fanOn = true; //set fanOn status to "ON" 
}

//check for termistor fault function
boolean checkThermistorFault() {
readSensor(thermistorBatt);
if ((sensorValue > 1000) || (sensorValue < 10)) {
  thermistorFault = true;
}
readSensor(thermistorAmb);
if ((sensorValue > 1000) || (sensorValue< 10)) {
  thermistorFault = true;
}
return thermistorFault;
}

//check for battery voltage fault function
boolean checkVoltageFault() {
readSensor(battery);
battVolt = (sensorValue*0.00488*3); //voltage scaled to 15V full scale
if (battVolt <10.0) {
  voltageFault = true;
}
return voltageFault;
}

//measure battery voltage value - IS THIS REDUNDANT WITH checkVoltageFault()?
float measureVoltage() {
readSensor(battery);
battVolt = (sensorValue*0.00488*3); //voltage scaled to 15V full scale
return battVolt;
}

//check for fan fault function
boolean checkFan() {
analogWrite(fanControl,128); //turn on fan control signal to 1/2 speed
digitalRead(rotor); //check if fan is turning
if (digitalRead(rotor) == LOW) {
   fanFault = true;
}
return fanFault;
}

/* check operating mode function
this function will read battery temperature and set the operating
mode to either 0 = OK 1 = heat 2 = cool
mode 3 (fan overide contact closed) is set by fanOveride ISR
*/

void checkOperatingMode() {
readSensor(thermistorBatt);
thermistorConvert(sensorValue);
battTemperature = temperature;
if ((battTemperature >= 50) && (battTemperature <= 75)) {
mode = 0; //OK Mode 
} else {
  if (battTemperature < 50) {
    mode = 1; //heat mode
    } else {
    if (battTemperature > 75) {
      mode = 2; //cool mode
         }
  }
}
return mode;
} //closing brace for check operating mode function

//ISR Routines
void fanOveride() { //user fan overide contact
mode = 3; //set mode to 3, overide mode
//void fanRampOn(); //turn on fan control signal
//boolean checkFan();
//if (fanFault = true) {
  //analogWrite(fanControl, 0); //turn off fan
  //digitalWrite(heatLED, LOW); turn off heat LED
  //digitalWrite(coolLED, LOW); turn off cool LED
  //digitalWrite(okLED, LOW); turn off OK LED
  //while fanFault = true{} //TEST THIS WHILE STATEMENT
}



void setup() {
 pinMode(fanControl,OUTPUT); //PWM fan control
 pinMode(rotor,INPUT_PULLUP); //INT pin input for fan rotor lock
 pinMode(fanContact,INPUT_PULLUP); //input for manual fan ON contact
 pinMode(okLED,OUTPUT); // green operating mode OK LED
 pinMode(heatLED,OUTPUT); // red heat operating mode LED
 pinMode(coolLED,OUTPUT); // blue cool operating mode LED
 pinMode(fanLED,OUTPUT); // green fan run LED

//set up external interrupts
attachInterrupt (0, fanOveride, LOW);  // attach fan overide ISR

noInterrupts(); //disable interrupts while in setup() loop
float checkVoltage();
while (voltageFault == true) {
  //stop, set voltage fault = true, flash low voltage light
}
boolean checkThermistor();
while (thermistorFault == true){
  // stop, set thermistor fault = true, flash thermistor light
}
checkFan();
while (fanFault == true) {
  // stop, set fanFault = true, flash fan light
}

interrupts(); // turn on interrupts
} //closing brace for setup loop

/*---------------------------------------------
                    MAIN LOOP
----------------------------------------------*/
void loop() {
/*
// check for voltage fault and wait for voltage increase if voltage is low
measureVoltage();
if (battVolt <10.0) {
  voltageFault = true;
  analogWrite(fanControl,0); //turn off fan
  digitalWrite(fanLED, LOW); //turn off fan LED
  digitalWrite(okLED, LOW); //turn off OK LED
  digitalWrite(heatLED, LOW); //turn off heat LED
  digitalWrite(coolLED, LOW); //turn off cool LED
  } else {
    voltageFault = false;
  }
while (voltageFault = true) {
  digitalWrite(coolLED, coolLEDState); //flash blue LED for voltage fault
  delay(500); //convert this to millis while loop  
    if (coolLEDState == LOW) {
       coolLEDState = HIGH;
    } else {
      coolLEDState = LOW;
    }
  measureVoltage();
  if (battVolt >=10.0) {
  voltageFault = false;
  } 
}
//check for thermistor fault and wait for fault to clear
checkThermistorFault();
if (thermistorFault = true) {
  analogWrite(fanControl,0); //turn off fan
  digitalWrite(fanLED, LOW); //turn off fan LED
  digitalWrite(okLED, LOW); //turn off OK LED
  digitalWrite(coolLED, LOW); //turn cool LED
} else {
    thermistorFault = false;
  }  
while (thermistorFault = true) {  
  digitalWrite(heatLED, heatLEDState); //flash red LED for thermistor fault
  delay(500); //convert this to millis while loop  
    if (heatLEDState == LOW) {
       heatLEDState = HIGH;
    } else {
      heatLEDState = LOW;
    }
  checkThermistorFault();
} 

//Check operating mode
checkOperatingMode();
*/

digitalWrite(fanLED, HIGH); //turn on fan LED
fanRampOn();
analogWrite(fanControl,0);
digitalWrite(fanLED, LOW); //turn off fan LED
delay(1000);
} //closing brace for main loop
