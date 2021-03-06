/*
Fan Controller Code Version 0.5
Designed to run on an Trinket Pro processor board

This version combines teh Thermistor Sensor Reading adn the Thermistor Convert functions into
1 function
readSensor() function has been deleted

I/O DEFINITIONS

A1 - Ambient Temperature Thermistor
A2 - Battery Temperature Thermistor
A0 - Battery Voltage
D3 - Fan contact input (INT1 pin)
D4 - Fan Rotor
D6 - Fan On/Off Control PWM
D9 - Fan LED 
D10 -Fan Override LED
D11 -Operating Mode LED Grn OK
D12 -Operating Mode LED Blue Cooling
D13 -Operating Mode LED Red Heating
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
 //const int battVoltage = A0; // analog input battery voltage
 //const int thermistorAmb = A1; //analog input ambient temperature measurement
 //const int thermistorBatt = A2; //analog input battery temperature measurement
 const int thermistorNominal=10000;
 const int temperatureNominal=25; 
 const int bCoefficient=3950;
 const int seriesResistor=10000;//nominal value,measure for better accuracy
 int coolLEDState = LOW; //used in LED flash code
 int heatLEDState = LOW;
 int okLEDState = LOW;
 int fanLEDState = LOW;
 int ovrState = LOW;
 byte operatingState = 0; //set operating mode to OK
 float sensorValue = 0.0; //should this be float or int?
 float battVolt = 0.0;        //battery voltage 
 float battTemperature = 0.0; //main system battery temperature
 float ambTemperature = 0.0; //main system battery enclosure temperature
 float steinhart = 0.0;
 //float temperature = 0.0;
 boolean fanOn = false;      //fan status running/not running
 boolean fanFault = false;  //fan locked rotor signal HIGH == locked rotor
 boolean thermistorFault = false; //used in thermistor check
 boolean voltageFault = false; //used in low voltage disconnect
 boolean fanOvrState = false; //fan overide contact close to run fan
 unsigned long startTime = 0; //used in delay timers
 unsigned long delay333 = 333; //delay 333ms
 
//End of Variables Definition ------------------------- 


/*----------------------------------------------
 * FUNCTION DEFINITIONS & ISRs
 -----------------------------------------------*/

//measure battery voltage
float measureVoltage() {
sensorValue=0.0;
for (int i=0; i<5; i++) {
     sensorValue = sensorValue + analogRead(A0);
     delay(10);
   }
Serial.print("Sensor Value in function = "); //for debug
Serial.println(sensorValue); //for debug   
sensorValue=sensorValue/5.0;
Serial.print("Sensor Value / 5 in function = "); //for debug
Serial.println(sensorValue); //for debug   
battVolt = (sensorValue *0.00488*3); //voltage scaled to 15V full scale
Serial.print("Battery Voltage in function = "); //for debug
Serial.println(battVolt); //for debug
return battVolt;
} //end of measureVoltage()

//measure ambient temperature
float measureAmbTemp() {
sensorValue=0.0;
for (int i=0; i<5; i++) {
     sensorValue = sensorValue + analogRead(A1);
     delay(10);
   }
sensorValue=sensorValue/5;   //Average of 5 readings
sensorValue = 1023 / sensorValue - 1;
  sensorValue = seriesResistor / sensorValue;
  steinhart = sensorValue / thermistorNominal;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= bCoefficient;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (temperatureNominal + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart; // Invert
  steinhart -= 273.15;  // convert to C
  steinhart = steinhart * 9.0 / 5.0 +32.0;         // convert to F
  ambTemperature = steinhart;
  return ambTemperature;
} //end of measureAmbTemp()

//measure battery temperature
float measureBattTemp() {
sensorValue=0.0;
for (int i=0; i<5; i++) {
     sensorValue = sensorValue + analogRead(A2);
     delay(10);
   }
sensorValue=sensorValue/5;  // should this be 5.0? 
sensorValue = 1023 / sensorValue - 1;
  sensorValue = seriesResistor / sensorValue;
  steinhart = sensorValue / thermistorNominal;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= bCoefficient;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (temperatureNominal + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart; // Invert
  steinhart -= 273.15;  // convert to C
  steinhart = steinhart * 9.0 / 5.0 +32.0;         // convert to F
  battTemperature = steinhart;
  return battTemperature;
} //end of measureBattTemp()


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

/* check operating state function
this function will read battery temperature and set the operating
mode to:
1 = OK
2 = cool
3 = cooling desired
4 = heat
5 = heat desired
6 = low voltage fault
7 = thermistor fault
8 = fan fault
*/


// Probably will eliminate or rewrite this function to be  replaced with Timer ISR?

byte checkOperatingState() {
measureVoltage(); //measure voltage
measureAmbTemp(); //measure ambient temperature
measureBattTemp(); //measure battery temperature

if (battVolt < 10.0) {
	operatingState = 6;}
if ((battTemperature > 200.0) || (ambTemperature > 200.0) || (battTemperature < -20.0) || (ambTemperature < -20.0)){
		    operatingState=7;}
if ((battTemperature >= 50.0) && (battTemperature <= 75.0)) {
            operatingState = 1;} //OK Mode  
if ((battTemperature > 75.0) && ((battTemperature - ambTemperature) > 5.0)){
      operatingState = 2;} //cool mode
if ((battTemperature < 50.0) && ((ambTemperature - battTemperature) > 5.0)){
            operatingState = 4;} //heat mode 
return operatingState;
} //closing brace for check operating mode function




//ISR Routines  VARIBALE NAMES MUST BE CORRECTED
//user override contact ISR
byte fanOveride() { //user fan overide contact
digitalRead(fanContact);
if (fanContact == HIGH){
   operatingState = 8;
}
return fanOvrState;
}
//Timer Interrupt ISR
//Periodically measure voltage and temperture to
//determine operating state 

/* ----------------------------------------------
                 SETUP LOOP
-------------------------------------------------*/                 

void setup() {
 pinMode(fanControl,OUTPUT); //PWM fan control
 pinMode(fanRotor,INPUT_PULLUP); //INT pin input for fan rotor lock
 pinMode(fanContact,INPUT_PULLUP); //input for manual fan ON contact
 pinMode(okLED,OUTPUT); // green operating mode OK LED
 pinMode(heatLED,OUTPUT); // red heat operating mode LED
 pinMode(coolLED,OUTPUT); // blue cool operating mode LED
 pinMode(fanLED,OUTPUT); // green fan run LED
 pinMode(ovrLED,OUTPUT); // amber override LED

//set up external interrupts
attachInterrupt (1, fanOveride, CHANGE);  // attach overide ISR

analogReference(DEFAULT);
Serial.begin(9600);  //to be removed after testing


} //closing brace for setup loop

/*---------------------------------------------
                    MAIN LOOP
----------------------------------------------*/
void loop() {
// measure voltage 
measureVoltage();
Serial.println(""); //for debug
//Serial.print("Battery Voltage Main Loop = "); //for debug
//Serial.println(battVolt); //for debug
delay(2000);



// Next step -
//rewrite checkoperatingstate function and verify that works, then convert it to TIMER ISR
//

} //closing brace for main loop

