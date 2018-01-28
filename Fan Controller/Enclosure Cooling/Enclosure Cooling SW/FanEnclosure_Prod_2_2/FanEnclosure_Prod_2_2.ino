/*
Fan Controller Code Enclosure Version Prod 2.2
Designed to run on an Trinket Pro processor board
Schematic rev 8
This code is identical to Prod 1.0 except for I/O
Pin assignments

Rev History
2.0 Prod release
2.1 Changed POST to flash heat LED to signify Battery Enclosure F/W loaded
2.2 Changed POST to 3 sets of 3 flashes

I/O DEFINITIONS
A0 - Ambient Temperature Thermistor
A1 - Batery Temperature Thermistor
A2 -
A3 - Digital Output 24V LED
A4 - Digital Output 12V LED
A5 - Battery Voltage
D3 - Remote contact input (INT1 pin)
D4 - Fan On/Off Control
D5 - 24V/12V select
D6 - 
D7 - N/A
D12 - Operating Mode LED Red Heating 
D8 - Fan LED 
D9 - Remote Contact LED
D10 - Operating Mode LED Grn OK
D11 - Operating Mode LED Blue Cooling
D13 - 
*/


/*----------------------------------------------------------
 *                      Define Variables 
 *----------------------------------------------------------- */                  

 const int remoteContact = 3; //INT 1
 const int fanControl = 4; // fan on/off
 const int okLED = 10; //green operating mode LED
 const int coolLED = 11; //blue coolLED
 const int heatLED = 12; //red heatLED
 const int fanLED = 8; //green fan LED
 const int remoteLED = 9; //amber override LED
 const int twelveLED = A4; //green 12V LED
 const int twentyfourLED = A3; //green 24V LED
 int voltLED; //value determined during setup loop
 const int batterySelect = 5;  //select 12 or 24 volt battery 
 const int thermistorNominal=10000;
 const int temperatureNominal=25; 
 const int bCoefficient=3950;
 const int seriesResistor=10000;//nominal value, measure for better accuracy
 //int coolLEDState = LOW;
 //int heatLEDState = LOW;
 //int okLEDState = LOW;
 //int fanLEDState = LOW;
 //int remoteLEDState = LOW;
 unsigned int timeCounter = 0; //counter incremented by TIMER INT ISR
 unsigned int x = 0; //used with timeCounter to determine 5 minute interval
 volatile byte operatingState = 2; //set operating mode to OK
 boolean batteryType = LOW; //set to 24V battery initially
 float sensorValue = 0.0; //should this be float or int?
 float battVolt = 0.0;        //battery voltage 
 float ambTemperature = 0.0; //main system battery enclosure temperature
 float steinhart = 0.0;
 float lowVolt = 0.0;
 //boolean fanOn = false;   //fan status running/not running
 //boolean thermistorFault = false; //used in thermistor check
 //boolean voltageFault = false; //used in low voltage disconnect
 volatile boolean remoteContactState = false; //NO remote contact -  close to run fan
 unsigned long startTime = 0; //used in delay timers
 unsigned long delay200 = 200; //delay 200ms



  
//End of Variables Definition ------------------------- 


/*----------------------------------------------
 * FUNCTION DEFINITIONS & ISRs
 -----------------------------------------------*/

//measure battery voltage
float measureVoltage() {
sensorValue=0.0;
for (int i=0; i<5; i++) {
     sensorValue = sensorValue + analogRead(A5);
     delay(10);
   }
sensorValue=sensorValue/5.0;
battVolt = ((sensorValue *0.00488*7.0)+0.5); //voltage scaled to 35V full scale
return battVolt;
} //end of measureVoltage()

//measure ambient temperature
float measureAmbTemp() {
sensorValue=0.0;
for (int i=0; i<5; i++) {
     sensorValue = sensorValue + analogRead(A0);
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


// Check Operating State Function

byte checkOperatingState() {
//0 = low voltage fault
//1 = thermistor fault
//2 = OK
//3 = cool
//4 = remote contact has been closed  - set by ISR
if (digitalRead(remoteContact) == HIGH) {
  operatingState = 4;
}
measureVoltage();
measureAmbTemp();
if (battVolt < lowVolt) {
	operatingState = 0;
} else {
  if ((ambTemperature > 200.0) || (ambTemperature < -20.0)) {
		    operatingState=1;
  } else {
    if ((ambTemperature < 100.0)&&(digitalRead(remoteContact)== LOW)){
            operatingState = 2; //OK Mode
            } else {  
             if ((ambTemperature >= 100.0)&& (digitalRead(remoteContact)== LOW)){
             operatingState = 3;//cool mode
            }  
    } //closing brace for else
  } //closing brace for else
} //closing brace for else
  return operatingState;
} //closing brace for check operating mode function


//ISR Routines  VARIBALE NAMES MUST BE CORRECTED

//user remote contact ISR
void remote() {
   remoteContactState = !remoteContactState;
   //operatingState = 4;
}  // closing brace remote contact isr


//Timer Interrupt ISR
ISR(TIMER1_COMPA_vect)        //timer compare interrupt service routine
{
 timeCounter += 1;            //increment countHour on each interrupt
 x = (timeCounter%2);        // test for 2 second interval
 if (x == 0){
  timeCounter = 0;
  checkOperatingState();      // does this need volatile variable?
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
 pinMode(remoteContact,INPUT_PULLUP); //input for manual fan ON contact
 pinMode(batterySelect,INPUT_PULLUP); //pin input for 12/24 V select 
 pinMode(okLED,OUTPUT); // green operating mode OK LED
 digitalWrite(okLED, LOW);
 pinMode(coolLED,OUTPUT); // blue cool operating mode LED
 digitalWrite(coolLED, LOW);
 pinMode(heatLED,OUTPUT); // red heat operating mode LED
 digitalWrite(heatLED, LOW);
 pinMode(fanLED,OUTPUT); // green fan run LED
 digitalWrite(fanLED, LOW);
 pinMode(remoteLED,OUTPUT); // amber override LED
 digitalWrite(remoteLED, LOW);
 pinMode(batterySelect,INPUT_PULLUP); //input for 12/24V select
 pinMode(twelveLED,OUTPUT); //
 digitalWrite(twelveLED, LOW);
 pinMode(twentyfourLED,OUTPUT); //
 digitalWrite(twentyfourLED, LOW);

 analogReference(DEFAULT);
 //Serial.begin(9600);  //to be removed after testing
 
// POST
digitalWrite(coolLED, HIGH);
delay(500);
digitalWrite(coolLED, LOW);
delay(500);
digitalWrite(coolLED, HIGH);
delay(500);
digitalWrite(coolLED, LOW);
delay(500);
digitalWrite(coolLED, HIGH);
delay(500);
digitalWrite(coolLED, LOW);
delay(1000);
digitalWrite(coolLED, HIGH);
delay(500);
digitalWrite(coolLED, LOW);
delay(500);
digitalWrite(coolLED, HIGH);
delay(500);
digitalWrite(coolLED, LOW);
delay(500);
digitalWrite(coolLED, HIGH);
delay(500);
digitalWrite(coolLED, LOW);
delay(1000);
digitalWrite(coolLED, HIGH);
delay(500);
digitalWrite(coolLED, LOW);
delay(500);
digitalWrite(coolLED, HIGH);
delay(500);
digitalWrite(coolLED, LOW);
delay(500);
digitalWrite(coolLED, HIGH);
delay(500);
digitalWrite(coolLED, LOW);
delay(1000);
//End of POST
  
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

interrupts();    // enable all interrupts
attachInterrupt (digitalPinToInterrupt (3),remote,CHANGE);

//initialize remote contact state
remoteContactState = digitalRead(remoteContact);

//digitalRead(batterySelect); //determine 12/24 selection
// HIGH = 12V LOW = 24V

batteryType = digitalRead(batterySelect);
if(batteryType == HIGH) {
  voltLED = twelveLED;
  lowVolt = 11.5;
  digitalWrite(voltLED, HIGH); //turn on 12V LED
  digitalWrite(twentyfourLED, LOW);
}
if(batteryType == LOW) {
  voltLED = twentyfourLED;
  lowVolt = 23.0;
  digitalWrite(voltLED, HIGH);//turn on 24V LED 
  digitalWrite(twelveLED, LOW);
}

checkOperatingState();

} //closing brace for setup loop

/*---------------------------------------------
                    MAIN LOOP
----------------------------------------------*/
void loop() {

//0 = low voltage fault
//1 = thermistor fault
//2 = OK
//3 = cool
//4 = remote contact has been closed  - set by ISR

while (operatingState == 4) {
  digitalWrite(remoteLED, HIGH);
  digitalWrite(fanControl, HIGH);
  digitalWrite(coolLED, LOW);
  digitalWrite(fanLED, LOW);
  digitalWrite(okLED, LOW);
}
if (operatingState == 0) {
  digitalWrite(coolLED, LOW);
  digitalWrite(fanLED, LOW); 
  digitalWrite(remoteLED, LOW);
  digitalWrite(okLED, LOW);
  digitalWrite(fanControl, LOW);
  digitalWrite(voltLED, LOW); 
  startTime = millis(); //set startTime for delay loop
  while ((startTime + delay200) > millis() ) {
   } //200 mS delay
  digitalWrite(voltLED, HIGH); 
  startTime = millis(); //set startTime for delay loop
  while ((startTime + delay200) > millis() ) {
   } //200 mS delay
} else { 
  if (operatingState == 1) {
     digitalWrite(okLED, LOW);
    digitalWrite(coolLED, LOW);
    digitalWrite(fanLED, LOW); 
    digitalWrite(fanControl, LOW);
    digitalWrite(remoteLED, HIGH);
    startTime = millis(); //set startTime for delay loop
  while ((startTime + delay200) > millis() ) {
   } //200 mS delay
    digitalWrite(remoteLED, LOW);
  startTime = millis(); //set startTime for delay loop
  while ((startTime + delay200) > millis() ) {
   } //200 mS delay
} else {
  if (operatingState == 2) {
    digitalWrite(coolLED, LOW);
    digitalWrite(fanLED, LOW);
    digitalWrite(remoteLED, LOW);
    digitalWrite(okLED, HIGH);
    digitalWrite(fanControl, LOW); 
} else { 
  if (operatingState == 3) {
    digitalWrite(remoteLED, LOW);
    digitalWrite(coolLED, HIGH);
    digitalWrite(fanLED, HIGH); 
    digitalWrite(fanControl, HIGH);     
  }
  }
  }
}


} //closing brace for main loop

