/*
Fan Controller Code Enclosure Version 0.1
Designed to run on an Trinket Pro processor board


I/O DEFINITIONS

A0 - Digital Output 24V LED
A1 - Digital Output 12V LED
A2 - 
A3 - Ambient Temperature Thermistor
A4 -
A5 - Battery Voltage
D3 - Fan contact input (INT1 pin)
D4 - Fan On/Off Control
D5 - 24V/12V select
D6 - 
D7 - N/A
D8 - 
D9 - Fan LED 
D10 - Fan Override LED
D11 - Operating Mode LED Grn OK
D12 - Operating Mode LED Blue Cooling
D13 - 
*/


/*----------------------------------------------------------
 *                      Define Variables 
 *----------------------------------------------------------- */                  

 const int fanContact = 3; //INT 1
 const int fanControl = 4; // fan on/off
 const int okLED = 11; //green operating mode LED
 const int coolLED = 12; //blue coolLED
 const int fanLED = 9; //green fan LED
 const int ovrLED = 10; //amber override LED
 const int twelveLED = A1; //green 12V LED
 const int twentyfourLED = A0; //green 24V LED
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
 float ambTemperature = 0.0; //main system battery enclosure temperature
 float steinhart = 0.0;
 boolean fanOn = false;      //fan status running/not running
 boolean thermistorFault = false; //used in thermistor check
 boolean voltageFault = false; //used in low voltage disconnect
 volatile boolean fanOvrState = false; //fan overide contact close to run fan
 unsigned long startTime = 0; //used in delay timers
 volatile byte state = LOW;


  
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
     sensorValue = sensorValue + analogRead(A3);
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

/* check operating state function
this function will read battery temperature and set the operating
mode to:
0 = low voltage fault
1 = thermistor fault
2 = OK
3 = cool
4 = fan contact overide 
*/


// Check Operating State Function

byte checkOperatingState() {
measureVoltage();
measureAmbTemp();
if (battVolt < 10.0) {
	operatingState = 0;
} else {
  if ((ambTemperature > 200.0) || (ambTemperature < -20.0)) {
		    operatingState=1;
  } else {
    if (ambTemperature < 100.0){
            operatingState = 2; //OK Mode
            } else {  
             if ((ambTemperature >= 100.0)){
             operatingState = 3;//cool mode
            }  
    } //closing brace for else
  } //closing brace for else
} //closing brace for else
  return operatingState;
} //closing brace for check operating mode function


//ISR Routines  VARIBALE NAMES MUST BE CORRECTED

//user override contact ISR rising edge
void toggle() {
  state = !state;
}

/* FOR WORKING ON INT ROUTINES
void fanOverideRise() {
   fanOvrState = true; //fan overide state
   //digitalWrite(okLED,LOW); 
   //digitalWrite(coolLED,LOW);
   //digitalWrite(fanLED,LOW);
   //digitalWrite(ovrLED,HIGH);
   //digitalWrite(fanControl,HIGH); 
}  //closing brace overide rising isr

//user override contact ISR falling edge
void fanOverideFall() {
   fanOvrState=false; // not in fan overide state
   digitalWrite(ovrLED,LOW);
   //digitalWrite(fanControl,LOW); 
   //checkOperatingState();
}  //closing brace overide falling isr 
*/  //FOR WORKING ON INT ROUTINES


//Timer Interrupt ISR
ISR(TIMER1_COMPA_vect)        //timer compare interrupt service routine
{
 timeCounter += 1;            //increment countHour on each interrupt
 x = (timeCounter%30);        // test for 5 minute interval
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
 pinMode(fanContact,INPUT_PULLUP); //input for manual fan ON contact
 pinMode(batterySelect,INPUT_PULLUP); //pin input for 12/24 V select 
 pinMode(okLED,OUTPUT); // green operating mode OK LED
 digitalWrite(okLED, LOW);
 pinMode(coolLED,OUTPUT); // blue cool operating mode LED
 digitalWrite(coolLED, LOW);
 pinMode(fanLED,OUTPUT); // green fan run LED
 digitalWrite(fanLED, LOW);
 pinMode(ovrLED,OUTPUT); // amber override LED
 digitalWrite(ovrLED, LOW);
 pinMode(batterySelect,INPUT_PULLUP); //input for 12/24V select
 pinMode(twelveLED,OUTPUT); //
 digitalWrite(twelveLED, LOW);
 pinMode(twentyfourLED,OUTPUT); //
 digitalWrite(twentyfourLED, LOW);

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

interrupts();    // enable all interrupts
attachInterrupt (digitalPinToInterrupt (3),toggle,CHANGE);  // attach Override Falling ISR
//attachInterrupt (digitalPinToInterrupt (3),toggle,FALLING);  // attach Override Falling ISR
digitalRead(batterySelect); //determine 12/24 selection
if(batterySelect == HIGH) {
  // HIGH = 12V LOW = 24V
  digitalWrite(twelveLED, HIGH); //turn on 24V LED
  digitalWrite(twentyfourLED, LOW);
  } else { digitalWrite(twentyfourLED, HIGH);//turn on 12V LED 
           digitalWrite(twelveLED, LOW);
}
checkOperatingState();

} //closing brace for setup loop

/*---------------------------------------------
                    MAIN LOOP
----------------------------------------------*/
void loop() {
/* digitalWrite(okLED ,LOW);
digitalWrite(coolLED ,LOW);
digitalWrite(fanLED ,LOW);
//digitalWrite(ovrLED ,LOW);

if (operatingState == 0) {
  digitalWrite(okLED, HIGH);
  delay(200);
  digitalWrite(okLED, LOW);
  delay(200);
} else { 
  if (operatingState == 1) {
    digitalWrite(ovrLED, HIGH);
    delay(200);
    digitalWrite(ovrLED, LOW);
    delay(200);
} else {
  if (operatingState == 2) {
    digitalWrite(okLED, HIGH); 
} else { 
  if (operatingState == 3) {
    digitalWrite(coolLED, HIGH);
    digitalWrite(fanLED, HIGH);      
  }
  }
  }
  
}

Serial.print("Operating state = "); //for debug
Serial.println(operatingState); //for debug
Serial.println();
Serial.print("Battery Voltage = "); //for debug
Serial.println(battVolt); //for debug
Serial.print("Amb Temp = ");//for debug
Serial.println(ambTemperature); //for debug
Serial.print("Fan Overide = ");//for debug
Serial.println(fanOvrState); //for debug
Serial.println();
delay(500);
*/

digitalWrite(ovrLED,state);

} //closing brace for main loop

