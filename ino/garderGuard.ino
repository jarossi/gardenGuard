#include <DS1307RTC.h>
#include <Time.h>
#include <Wire.h>
#include <OneWire.h>



#define solarPanelVoltagePin A0
#define batteryVoltagePin A1
#define chargeEnablePin 11
#define voltageConversionConst 0.0388
#define dsSensorPin 3

OneWire ds(dsSensorPin);


struct dataRecord{
  time_t time;
  int solarPanelVoltage;
  int batteryVoltage;
  float internalTemp;
  
  
};

dataRecord currentDataRecord;

void setup()   {
  Serial.begin(9600);
  pinMode(chargeEnablePin, OUTPUT);
  digitalWrite(chargeEnablePin, LOW);
  
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(timeStatus()!= timeSet) 
     Serial.println("Unable to sync with the RTC");
  else
     Serial.println("RTC has set the system time");

}


void printDebug(){
  Serial.print(currentDataRecord.time);
  Serial.print(" sP: ");
  Serial.print(currentDataRecord.solarPanelVoltage * voltageConversionConst );
  Serial.print(" batt: ");
  Serial.print(currentDataRecord.batteryVoltage * voltageConversionConst );
  Serial.print(" intTemp: ");
  Serial.println(currentDataRecord.internalTemp);
}


void loop()   {
  currentDataRecord.time=now();
  currentDataRecord.solarPanelVoltage = analogRead(solarPanelVoltagePin) ; // voltage on solarPanel
  currentDataRecord.batteryVoltage = analogRead(batteryVoltagePin);  // voltage on battery to be ch
  currentDataRecord.internalTemp = readTemp();
  printDebug();
  
  
  
  delay(100);

/*
  if ((x > y) & (y < CP) & (x > CP))  { // check input voltage and voltage on battery
    // turn on charge cycle if voltage input good AND battery voltage low.

    digitalWrite(charge_enable, HIGH); // turn on voltage to battery
    chon = (CP - y) * 1000;
    delay(chon); // ON wait 
    digitalWrite(charge_enable, LOW); // turn off charge enable
    delay(choff); // OFF wait
  } // end if
*/

}


float readTemp(){
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;

   if ( !ds.search(addr)) {
    ds.reset_search();
    delay(250);
    //continue;
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    //continue;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  return (float)raw / 16.0; 
}


