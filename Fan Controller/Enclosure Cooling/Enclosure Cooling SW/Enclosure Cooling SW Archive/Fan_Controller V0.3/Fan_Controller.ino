/*
Fan Controller Code Version 0.3
Designed to run on an Trinket Pro processor board


I/O DEFINITIONS

A0 - Ambient Temperature Thermistor
A1 - Battery Temperature Thermistor
A6 - Battery Voltage
D3 - Fan contact input (INT1 pin)
D4 - Fan Rotor
D6 - Fan On/Off Control PWM
D9 - Fan LED 
D10 -Fan Override LED
D11 -MOSI Operating Mode LED Grn OK
D12 -MISO Operating Mode LED Blue Cooling
D13 -SCK Operating Mode LED Red Heating
*/


/*----------------------------------------------------------
 *                      Define Variables 
 *----------------------------------------------------------- */                  

 const int fanContact = 3; //INT 1
 const int fanRotor = 4;
 const int fanControl = 6; //PWM
 const int okLED = 11; //green operating mode LED
 const int heatLED = 13; //red heat LED
 const int coolLED = 12; //blue coolLED
 const int fanLED = 9; //green fan LED
 const int ovrLED = 10; //amber override LED
 const int battVoltage = A6; // analog input battery voltage
 const int thermistorAmb = A0; //analog input ambient temperature measurement
 const int thermistorBatt = A1; //analog input battery temperature measurement
 const int thermistorNominal=10000;
 const int temperatureNominal=25; 
 const int bCoefficient=3950;
 const int seriesResistor=10000;//nominal value,measure for better accuracy
 int coolLEDState = LOW; //used in LED flash code
 int heatLEDState = LOW;
 int okLEDState = LOW;
 int fanLEDState = LOW;
 int ovrState = LOW;
 // ovrRide variable below may be removed as "operatingState" will make
 // this unnecessary
 int ovrRide = HIGH;
 byte operatingState = 0; //set operating mode to OK
 // byte dutyCycle = 0; //used in analog write to fan control
 float sensorValue = 0.0; 
 float battVolt = 0.0;        //battery voltage 
 float battTemperature = 0.0; //main system battery temperature
 float ambTemperature = 0.0; //main system battery enclosure temperature
 float steinhart = 0.0;
 float temperature = 0.0;
 boolean fanOn = false;      //fan status running/not running
 boolean fanFault = false;  //fan locked rotor signal HIGH == locked rotor
 boolean thermistorFault = false; //used in thermistor check
 boolean voltageFault = false; //used in low voltage disconnect
 boolean fanOvrState = false; //fan overide contact close to run fan
 unsigned long startTime = 0; //used in delay timers
 unsigned long delay333 = 333; //delay 333ms
 unsigned long delay500 = 500; //delay 500ms

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

//fan slow ramp turn on function - 1 second ramp
void fanRampOn(){
   digitalWrite(fanLED, HIGH); //turn on fan LED
   fanOn = true; //set fan on state to true
   analogWrite(fanControl,64);
   startTime = millis(); //set startTime for delay loop
   while ((startTime + delay333) > millis() ) {
   } //333 mS delay
   analogWrite(fanControl,128);
   startTime = millis(); //set startTime for delay loop
   while ((startTime + delay333) > millis() ) {
   } //333 mS delay
   analogWrite(fanControl,192);
   startTime = millis(); //set startTime for delay loop
   while ((startTime + delay333) > millis() ) {
   } //333 mS delay
   analogWrite(fanControl,255); //fan on to 100%
}

/* Change this to identify a fault based on temperature
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
*/

//check for battery voltage fault function
boolean checkVoltageFault() {
readSensor(battVoltage);
battVolt = (sensorValue*0.00488*3); //voltage scaled to 15V full scale
if (battVolt <10.0) {
  voltageFault = true;
} else {
  voltageFault = false;
}
return voltageFault;
}

//measure battery voltage value - IS THIS REDUNDANT WITH checkVoltageFault()?
float measureVoltage() {
readSensor(battVoltage);
battVolt = (sensorValue*0.00488*3); //voltage scaled to 15V full scale
return battVolt;
}

//check for fan fault function
boolean checkFan() {
//digitalRead(fanRotor); //check if fan is turning
if ((digitalRead(fanRotor) == HIGH) && (fanOn == HIGH)) {
   fanFault = true;
} else {
   fanFault = false;
}
return fanFault;
}

/* check operating mode function
this function will read battery temperature and set the operating
mode to either 0 = OK 1 = heat 2 = cool
mode 3 (fan overide contact closed) is set by fanOveride ISR
*/

void checkOperatingState() {
readSensor(thermistorBatt);
thermistorConvert(sensorValue);
battTemperature = temperature;
if ((battTemperature >= 50) && (battTemperature <= 75)) {
operatingState = 0; //OK Mode 
} else {
  if (battTemperature < 50) {
    operatingState = 1; //heat mode
    } else {
    if (battTemperature > 75) {
      operatingState = 2; //cool mode
         }
  }
}
return operatingState;
} //closing brace for check operating mode function

//ISR Routines  VARIBALE NAMES MUST BE CORRECTED
boolean fanOveride() { //user fan overide contact
digitalRead(fanContact);
if (fanContact == HIGH){
   fanOvrState = true;
} else {fanOvrState = false;
}
return fanOvrState;
}

/* ----------------------------------------------
                 SETUP LOOP
-------------------------------------------------*/                 

void setup() {
 pinMode(fanControl,OUTPUT); //PWM fan control
 pinMode(fanRotor,INPUT_PULLUP); //INT pin input for fan rotor lock
 pinMode(fanContact,INPUT); //input for manual fan ON contact
 pinMode(okLED,OUTPUT); // green operating mode OK LED
 pinMode(heatLED,OUTPUT); // red heat operating mode LED
 pinMode(coolLED,OUTPUT); // blue cool operating mode LED
 pinMode(fanLED,OUTPUT); // green fan run LED
 pinMode(ovrLED,OUTPUT); // amber override LED

//set up external interrupts
attachInterrupt (1, fanOveride, CHANGE);  // attach overide ISR

} //closing brace for setup loop

/*---------------------------------------------
                    MAIN LOOP
----------------------------------------------*/
void loop() {
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
  startTime = millis(); //set startTime for delay loop
   while ((startTime + delay500) > millis() ) {
   } //500 mS delay  
    if (coolLEDState == LOW) {
       coolLEDState = HIGH;
    } else {
      coolLEDState = LOW;
    }
  digitalWrite(coolLED, coolLEDState); //flash blue LED for voltage fault
  startTime = millis(); //set startTime for delay loop
   while ((startTime + delay500) > millis() ) {
   } //500 mS delay
}

//check for fan contact closure - ISR will set fanExternal = true
if (fanOvrState == true) {
  fanRampOn(); //turn on fan
  digitalWrite(fanLED, HIGH); //turn on fan LED
  digitalWrite(okLED, LOW); //turn off OK LED
  digitalWrite(coolLED, LOW); //turn off cool LED
  digitalWrite(heatLED, LOW); //turn off heat LED
}
while ((fanOvrState == true) && (fanFault == false) && (voltageFault == false)) {
digitalRead(fanRotor); //check if fan is turning
if (fanRotor == HIGH) {
   fanFault = true;
} else {
  fanFault = false;
}
measureVoltage();
if (battVolt <10.0) {
  voltageFault = true;
  } else {
    voltageFault = false;
  }
}

//check for thermistor fault and wait for fault to clear
checkThermistorFault();
if (thermistorFault = true) {
  analogWrite(fanControl,0); //turn off fan
  digitalWrite(fanLED, LOW); //turn off fan LED
  digitalWrite(okLED, LOW); //turn off OK LED
  digitalWrite(coolLED, LOW); //turn off cool LED
} else {
    thermistorFault = false;
  }  
while (thermistorFault = true) { 
  digitalWrite(heatLED, heatLEDState); //flash red LED for thermistor fault
  startTime = millis(); //set startTime for delay loop
   while ((startTime + delay500) > millis() ) {
   } //500 mS delay   
    if (heatLEDState == LOW) {
       heatLEDState = HIGH;
    } else {
      heatLEDState = LOW;
    }
  digitalWrite(heatLED, heatLEDState); //flash red LED for thermistor fault
  startTime = millis(); //set startTime for delay loop
   while ((startTime + delay500) > millis() ) {
   } //500 mS delay 
} 

//Check operating mode
checkOperatingState();

} //closing brace for main loop
