//READ TEMPERTURE AND TURN ON FAN IF REQUIRED
      //This function works with the Adafruit I2C A/D board. Must
      //be revised to use on chip A/D.
      void fanCooling(){
      battTemperature = readBattTemp();
      ambTemperature = readAmbTemp();
      if ((fanOn == true) && (battTemperature<80.0)){
        digitalWrite(fanControl, LOW); //turn off fan
        fanOn = false; 
      }
      else {
            if ((battTemperature>80.0) && (battTemperature-ambTemperature >5.0)){
            digitalWrite(fanControl,HIGH);  //TURN ON FAN
            fanOn=true;       //SET FAN FLAG  
            Serial.print("fanOn= ");
            Serial.println(fanOn);
              }
            }
          }
