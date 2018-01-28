/*
Fan Controller Code Version 0.7
Designed to run on an Trinket Pro processor board

Added Timer 1 interrupt at 1 Hz
This version combines the Thermistor Sensor Reading and the Thermistor Convert functions into
1 function
readSensor() function has been deleted

I/O DEFINITIONS

A1 - Ambient Temperature Thermistor
A2 - Battery Temperature Thermistor
A0 - Battery Voltage
D3 - Fan contact input (INT1 pin)
D4 - Fan Rotor
D5 - 24V/12V select
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
 const int batterySelect = 5;  //select 12 or 24 volt battery 
 const int thermistorNominal=10000;
 const int temperatureNominal=25; 
 const int bCoefficient=3950;
 const int seriesResistor=10000;//nominal value,measure for better accuracy
 int coolLEDState = LOW; //used in LED flash code
 int heatLEDState = LOW;
 int okLEDState = LOW;
 int fanLEDState = LOW;
 int ovrState = LOW;
 unsigned int timeCounter = 0; //counter incremented by TIMER INT ISR
 unsigned int x = 0; //used with timeCounter to determine 5 minute interval
 byte operatingState = 0; //set operating mode to OK
 float sensorValue = 0.0; //should this be float or int?
 float battVolt = 0.0;        //battery voltage 
 float battTemperature = 0.0; //main system battery temperature
 float ambTemperature = 0.0; //main system battery enclosure temperature
 float steinhart = 0.0;
 boolean fanOn = false;      //fan status running/not running
 boolean fanFault = false;  //fan locked rotor signal HIGH == locked rotor
 boolean thermistorFault = false; //used in thermistor check
 boolean voltageFault = false; //used in low voltage disconnect
 boolean fanOvrState = false; //fan overide contact close to run fan
 unsigned long startTime = 0; //used in delay timers
 //unsigned long delay333 = 333; //delay 333ms
 
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
sensorValue=sensorValue/5.0;
battVolt = (sensorValue *0.00488*3.0); //voltage scaled to 15V full scale
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
ISR(TIMER1_COMPA_vect)        //timer compare interrupt service routine
{
 timeCounter += 1;            //increment countHour on each interrupt
 x = (timeCounter%30);        // test for 5 minute interval
 if (x == 0){
  timeCounter = 0;
  measureVoltage();           //does this need volitile variable?
  measureBattTemp();          //does this need volitile variable?
  measureAmbTemp();           //does this need volitile variable?
 }
} //end of timer isr



//Periodically measure voltage and temperture to
//determine operating state 

/* ----------------------------------------------
                 SETUP LOOP
-------------------------------------------------*/                 

void setup() {
 pinMode(fanControl,OUTPUT); //PWM fan control
 digitalWrite(fanControl, LOW);
 pinMode(fanRotor,INPUT); //pin input for fan rotor lock
 pinMode(fanContact,INPUT_PULLUP); //input for manual fan ON contact
 pinMode(batterySelect,INPUT_PULLUP); //pin input for 12/24 V select 
 pinMode(okLED,OUTPUT); // green operating mode OK LED
 digitalWrite(okLED, LOW);
 pinMode(heatLED,OUTPUT); // red heat operating mode LED
 digitalWrite(heatLED, LOW);
 pinMode(coolLED,OUTPUT); // blue cool operating mode LED
 digitalWrite(coolLED, LOW);
 pinMode(fanLED,OUTPUT); // green fan run LED
 digitalWrite(fanLED, LOW);
 pinMode(ovrLED,OUTPUT); // amber override LED
 digitalWrite(ovrLED, LOW);
 //set up external interrupts
 attachInterrupt (1, fanOveride, CHANGE);  // attach overide ISR

analogReference(DEFAULT);
Serial.begin(9600);  //to be removed after testing

// set up Timer 1 interrupt
noInterrupts();      // disable all interrupts

//set timer1 interrupt at 1Hz
  TCCR1A = 0;        // set entire TCCR1A register to 0
  TCCR1B = 0;        // same for TCCR1B
  TCNT1  = 0;        //initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 15624;     // = (16mHz) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

interrupts();        // enable all interrupts
measureVoltage();           //does this need volitile variable?
measureBattTemp();          //does this need volitile variable?
measureAmbTemp();           //does this need volitile variable?

} //closing brace for setup loop

/*---------------------------------------------
                    MAIN LOOP
----------------------------------------------*/
void loop() {
Serial.print("Battery Voltage = "); //for debug
Serial.println(battVolt); //for debug
//Serial.print("Amb Temp = ");//for debug
//Serial.println(ambTemperature); //for debug
//Serial.print("Batt Temp = "); //for debug
//Serial.println(battTemperature); //for debug
//Serial.println();
delay(500);
// Next step -
//rewrite checkoperatingstate function and verify that works, then convert it to TIMER ISR
//

} //closing brace for main loop

