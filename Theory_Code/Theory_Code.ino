#include <Servo.h>
#include <SoftwareSerial.h>
#include <NewPing.h>

int rain_sensor = 5;
int water_lvl_sensor = A1;
int soil_moisture_sensor = A0;
int water_pot_lvl_sensor = A2;
int gas_sensor = A3;
int flame_sensor = 10;
int tank_pump = 12;
int irri_pump = 13;
int water_pot_pump = 11;
int sonar_echo = 6;
int sonar_trigger = 7;
int food_recycle = 4;
int exhaust_fan = 8; 
int r;
int w = 1;
int w_p = 1; //water_bowl
int s; 
bool first_tank = true;
bool first_pot = true;
bool sensor_frst_time_boot = true;
bool safety_trigger = false;
bool fire_safety_trigger = false;
bool irri_halt = false;
bool tank_hatch_open = false;
bool msg_on = false;
bool msg_off = true;
int pos=0;

Servo servo;
SoftwareSerial SIM900D(2,3);
NewPing sonar (sonar_trigger, sonar_echo, 200); // (trigger pin, echo pin, max distance in cm)

void setup() {
  servo.attach(9);
  servo.write(pos);

  pinMode(rain_sensor, INPUT);
  pinMode(flame_sensor, INPUT);
  pinMode(tank_pump, OUTPUT);
  pinMode(irri_pump, OUTPUT);
  pinMode(water_pot_pump, OUTPUT);
  pinMode(13,INPUT);
  SIM900D.begin(19200);
  Serial.begin(9600);

}

void loop() {
  if(sensor_frst_time_boot) //since during sensor boot up they throw random values. If not excluded then might create problem with the whole system
  {
    sensor_frst_time_boot = false;
    delay(1000);
  }
  
  irrigation();
  livestock_farm();
  
  Serial.println();
  delay(200);

}

void GSM(String x, bool flame)
{
  String msg;
  if(flame)
  {
    if(x == "on")
    {
      msg = "\rFire has been detected.\rFire safety protocol has been initiated.";
    }
    else
    {
      msg = "\rFire has been put out.\rFire safety protocol has been terminated.";
    }
  }
  else
  {
    msg = "Safety protocol of the irrigation pump is "+x;
  }

    delay(50);
    SIM900D.print("AT+CMGF=1\r");
    SIM900D.println("AT + CMGS = \"+88018********\"");
    SIM900D.println(msg);
    SIM900D.println();
}

void livestock_farm()
{
  //flame sensor
  int flame_sens = digitalRead(flame_sensor);
  
  Serial.print("flame = ");
  Serial.println(flame_sens);
  
  if(flame_sens and not(fire_safety_trigger))
  {
    GSM("on", true);
    fire_safety_trigger = true;
  }
  else if(fire_safety_trigger and not(flame_sens))
  {
    GSM("off",true);
    fire_safety_trigger = false;
  }

  //water bowl
  int water_pot_sens = analogRead(water_pot_lvl_sensor);
  Serial.print("water_pot_lvl = ");
  Serial.println(water_pot_sens);

  if(water_pot_sens >= 509)//0% = 1019 (4.98V) and 100% = 0 (0V); so 50% = 509.5
  {
    Serial.println("Water in water pot is not enough.");
    first_pot = false;
    w_p = 0; //not enough water in tank 
  }
  else if(water_pot_sens <= 100 and not(first_pot)) //tank is deemed to be full at around 90%; 90%==917.1
  {
    Serial.println("Water in water is pot enough");
    w_p = 1; //enough water in tank
  }

  //water pump for water bowl

  if(w_p == 0)
  {
    Serial.println("Water pot pump is ON.");
    digitalWrite(water_pot_pump, HIGH);
  }
  else
  {
    Serial.println("Water pot pump is OFF.");
    digitalWrite(water_pot_pump, LOW);
  }

  int gas_sens = analogRead(gas_sensor);
  Serial.print("Gas quality = ");
  Serial.print((1019-gas_sens/1019*.0)*100.0);
  Serial.println(" % (The lower the pertange the better the air quality is.)");

  if(gas_sens <= 510) // 50% = 509.5 
  {
    digitalWrite(exhaust_fan, HIGH);
    Serial.println("Exhaust fan is ON");
  }
  else
  {
    digitalWrite(exhaust_fan, LOW);
    Serial.println("Exhaust fan is OFF");
  }


  int food_pot = sonar.ping_cm();
  Serial.print("food pile hight = ");
  Serial.print(200 - food_pot);
  Serial.println(" cm");

  
  if(food_pot >= 100)
  {
    digitalWrite(food_recycle, HIGH);
    Serial.println("Not enough Food in the pile./rFood is being recycled:");
  }
  else
  {
    digitalWrite(food_recycle, LOW);
    Serial.println("Enough food is in the pile.");
  }

  
}

void irrigation()
{
  r = digitalRead(rain_sensor);
  
  // if moisture is less or equal to 30%, s = 0; else s = 1
  int moisture_sens = analogRead(soil_moisture_sensor);
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
  
  Serial.print("Soil moisture = ");
  Serial.print(((1019.0-moisture_sens)/1019.0)*100.0);
  Serial.println("%");
  
  if(r == 0)
  {
    Serial.print("It is raining");  
  }
  else
  {
    Serial.println("It is not raining");
  }
  Serial.print("Soil moisture = ");
  Serial.print(((1019.0-water_lvl_sens)/1019.0)*100.0);
  Serial.println("%");
  

  
  //irrigation pump logic
  if((not r) and (not s) and (w or not(safety_trigger)))
  {
    if(not safety_trigger)
    {
      digitalWrite(irri_pump, HIGH);
      Serial.println("Irrigation pump is ON.");
    }
    else
    {
      digitalWrite(irri_pump, LOW);
      Serial.println("Irrigation pump is OFF.");
    }
    
  }
  else
  {
    digitalWrite(irri_pump, LOW);
    Serial.println("Irrigation pump is OFF.");
  }

  // Tank pump logic
  if(not w and first_tank) // independent of s;
  {
    Serial.println("Tank pump is ON.");
    digitalWrite(tank_pump, HIGH);
  }
  else if(water_lvl_sens <= 100 and not(r) and not(w))//  when no rain, pump will be off if tank capacity is almost around 90%=917.1
  {
    Serial.println("Tank pump is ON.");
    w = 1;
    digitalWrite(tank_pump, LOW);
  }
  else if(water_lvl_sens <= 306 and r)//  when there is rain, pump will be off if tank capacity is almost around 70%=713.3
  {
    Serial.println("Tank pump is OFF.");
    digitalWrite(tank_pump, LOW);
  }
  else if(not (r) and water_lvl_sens > 100 and (not(first_tank)))
  {
    Serial.println("Tank pump is ON.");
    digitalWrite(tank_pump, HIGH);
  }
  else if(w)
  {
    Serial.println("Tank pump is OFF.");
    digitalWrite(tank_pump, LOW);
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

  if(tank_hatch_open)
  {
    Serial.print("Tank hatch is Open.\rAngle = ");
    Serial.print(pos+19);
    Serial.println(" degree.");
  }
  else
  {
    Serial.print("Tank hatch is Closed.\rAngle = ");
    Serial.print(pos+2);
    Serial.println(" degree.");
  }

  if(safety_trigger)
  {
    Serial.println("Safety trigger is ON");
    if(not msg_on)
    {
      GSM("on", false);
      msg_on = true;
      msg_off = false;
    }
  }
  else
  {
    Serial.println("Safety trigger is OFF");
    if(not msg_off)
    {
      GSM("off", false);
      msg_on = false;
      msg_off = true;
    }
  }
}
