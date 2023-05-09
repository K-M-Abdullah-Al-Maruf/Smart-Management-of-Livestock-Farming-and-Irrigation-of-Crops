#include <Servo.h>
#include <SoftwareSerial.h>
#include "DHT.h"

#define DHTPIN 52
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

int rain_sensor = 53;
int water_lvl_sensor = A1;
int soil_moisture_sensor = A0;
int tank_pump = 23;
int irri_pump =22;
int r;
int w = 1;
int s; 
bool first_tank = true;
bool safety_trigger = false;
bool irri_halt = false;
bool tank_hatch_open = false;
bool msg_on = false;
bool msg_off = true;
int pos=0;

Servo servo;
SoftwareSerial SIM900D(24,25);

void setup() {
  servo.attach(11);
  servo.write(pos);

  pinMode(A11, INPUT);
  pinMode(A12, INPUT);
  pinMode(water_lvl_sensor, INPUT);
  pinMode(soil_moisture_sensor, INPUT);
  pinMode(rain_sensor, INPUT);
  pinMode(tank_pump, OUTPUT);
  pinMode(irri_pump, OUTPUT);
  
  SIM900D.begin(19200);
  Serial.begin(9600);
  dht.begin();

}

void loop() {
  
  irrigation();

  livestock();
  
  delay(300);

}

void GSM(String x)
{
  String msg = "Safety protocol of the irrigation pump is "+x;
  Serial.println("sending notification to phone");
  delay(500);
  SIM900D.print("AT+CMGF=1\r");
  SIM900D.println("AT + CMGS = \"+88018********\"");
  SIM900D.println(msg);
  SIM900D.println();
}

void irrigation()
{
  r = digitalRead(rain_sensor);
  
  // if moisture is less or equal to 30%, s = 0; else s = 1
  int moisture_sens = analogRead(soil_moisture_sensor);
  Serial.print("moisture = ");
  Serial.println(moisture_sens);
  if(moisture_sens >= (1019-306)) //0% = 1019 (4.98V) and 100% = 0 (0V); so 30% = 305.7  
  {
    s = 0; //moisture is not enough
  }
  else
  {
    s = 1; //moisture is enough
  }
  


  //if water level is less or equal to 30%, w = 0; else w = 1
  int water_lvl_sens = analogRead(water_lvl_sensor);
  Serial.print("tank = ");
  Serial.println(water_lvl_sens);
  if(water_lvl_sens >= 509)//0% = 1019 (4.98V) and 100% = 0 (0V); so 50% = 509.5
  {
    first_tank = false;
    w = 0; //not enough water in tank 
    
    if(water_lvl_sens >= 917)// 10% = 101.9, this water level is dangerous for irrigation pumps. Hence safety measure for the pump is on
    {
      safety_trigger = true; 
    }
    else if(water_lvl_sens <= 764) //25% = 254.75, this water level is safe for irrigation pumps. Hence safety measure for the pump is off
    {
      safety_trigger = false;
    }
    
  }
  else if(water_lvl_sens <= 100 and not(first_tank)) //tank is deemed to be full at around 90%; 90%==917.1
  {
    first_tank = false;
    safety_trigger = false;
    w = 1; //enough water in tank
  }
  else if(water_lvl_sens <= 764 and safety_trigger) //25% = 254.75, this water level is safe for irrigation pumps. Hence safety measure for the pump is off
  {
      safety_trigger = false;
  }

  
  
  Serial.println((1019-moisture_sens)/1019);
  Serial.println(r);
  Serial.println((1019-water_lvl_sens)/1019);
  
  //irrigation pump logic
  if((not r) and (not s) and (w or not(safety_trigger)))
  {
    if(not safety_trigger)
    {
      digitalWrite(irri_pump, HIGH);
      Serial.println("ON");
      Serial.println(1);
    }
    else
    {
      Serial.println("OFF");
      Serial.println(0);
    }
    
  }
  else
  {
    digitalWrite(irri_pump, LOW);
    Serial.println("OFF");
    Serial.println(0);
  }

  // Tank pump logic
  if(not w and first_tank) // independent of s;
  {
    Serial.println("ON");
    Serial.println(1);
    digitalWrite(tank_pump, HIGH);
  }
  else if(water_lvl_sens <= 100 and not(r) and not(w))//  when no rain, pump will be off if tank capacity is almost around 90%=917.1
  {
    Serial.println("OFF");
    Serial.println(0);
    w = 1;
    digitalWrite(tank_pump, LOW);
  }
  else if(water_lvl_sens <= 306 and r)//  when there is rain, pump will be off if tank capacity is almost around 70%=713.3
  {
    Serial.println("OFF");
    Serial.println(0);
    digitalWrite(tank_pump, LOW);
  }
  else if(not (r) and water_lvl_sens > 100 and (not(first_tank)))
  {
    Serial.println("ON");
    Serial.println(1);
    digitalWrite(tank_pump, HIGH);
  }

  //Tank hatch open
  if(r and not(w)and (not(tank_hatch_open)))// independent of s
  {
    for(pos = 0; pos <= 120; pos += 2) //component has mechanicle overshoot error of +(20 to 22) degrees
    {
      servo.write(pos);
      delay(10);
    }
    
    tank_hatch_open = true;
  }
  
  //Tank hatch close
  else if(tank_hatch_open and (w or not(r)))
  {
    for(pos = 120; pos >= 0; pos -= 2) //component has mechanicle overshoot error of +(20 to 22) degrees
    {
      servo.write(pos);
      delay(10);
    }
    tank_hatch_open = false;
  }

  //for lab view
  if(tank_hatch_open)
  {
    Serial.println("Open");
    Serial.println(pos+19);
  }
  else
  {
    Serial.println("Closed");
    Serial.println(pos+2);
  }

  //for lab view and GSM here
  if(safety_trigger)
  {
    Serial.println("ON");
    Serial.println(1);
    if(not msg_on)
    {
      GSM("on");
      msg_on = true;
      msg_off = false;
    }
  }
  else
  {
    Serial.println("OFF");
    Serial.println(0);
    if(not msg_off)
    {
      GSM("off");
      msg_on = false;
      msg_off = true;
    }
  }
  Serial.println();
}

void livestock()
{
  float humidity_sens = dht.readHumidity();
  float temp_sens = dht.readTemperature();
  Serial.print("humidity = ");
  Serial.println(humidity_sens);
  Serial.print("temp = ");
  Serial.println(temp_sens);

  Serial.print("a11 = ");
  Serial.println(analogRead(A11));
  Serial.print("a12 = ");
  Serial.println(analogRead(A12));
}
