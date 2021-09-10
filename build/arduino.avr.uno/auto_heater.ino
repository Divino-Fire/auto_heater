
//#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//#include <LiquidCrystal_SR.h>
#include <EEPROM.h>
#include <Wire.h>
#include <RTClib.h>
#include <SetTimeout_G.h>
//#include <TimerOne.h> // eliminate this library by injecting your desired function's code into the while loop of countdown in start_up function 

// FUNCTION PROTOTYPES

int makeChoice(uint8_t, unsigned int, bool, char*, bool, char* [] = {}, unsigned int = 250, float = 1.0);
void saveTimer(int, int, int, int, int, int = 0, bool = true, bool = false);

//objects
//LiquidCrystal_I2C lcd(0x20, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); //  used if i2c pic mc is available
LiquidCrystal lcd(12, 11 , 10 , 9, 8, 7); // used only in proteus simulation
//RTC_DS3231 rtc; // harware time module with power failure feature :: used for alarm

// tied to readTemp_ function
const uint8_t temp_pin = 4;
OneWire temp_wire(temp_pin);
DallasTemperature temp_sensor1(&temp_wire); 

//LiquidCrystal_SR lcd(5, 6, 4); // Data Enable/ SER,  Clock/SCL, SCK
RTC_DS3231 rtc; // comment this line after debugging
SetTimeout_G timer1; // custom software timer without power failure feature :: used for timing

//output
const uint8_t heater = 3;
const uint8_t buzzer = 6;
//const uint8_t buzzer = 8;
//const uint8_t LED_heater_on = 9;
//const uint8_t LED_heater_off = 10; 
const uint8_t LED_heater_on = 5;
const uint8_t LED_heater_off = 13; 
//inputs
const uint8_t knob = A0;
const uint8_t ok_button = 2; // used only in the ISR
//const uint8_t temp_sensor = 4; // for the LM35, was not used -- see pin 11 temp_pin instead


//system constants
    //EEPROM addresses
const int month_address = 0;
const int day_address = 1;
const int hour_address = 2;
const int minute_address = 3;
const int onMinutes_address = 4;
const int repeat_address = 5;
const int second_address = 6;
const int completed_address = 7;

struct time_struct {
  int month;
  int day;
  int hour;
  int minute;
  int onMinutes;
  int repeat;
  int completed;
  int second;
};

char* daysOfTheWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Everyday"};
char* monthsOfTheYear[] = {"everyMonth","January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
const int lcd_relaxTime = 250;
const int welcome_delay = 1500;
const char* actions[] = {"SET TIMER --> ", "<SET DATE-TIMER>", " <- SET TEMP ->", "< SAVED TIMER ->", "DISPLAY TIME"};
enum choice {timer_c, dateTimer_c, setTemp_c, seeFutureTimer_c, displayTime_c};
const String welcome_message = "Happy Heating!";
const int buzzer_start_freq = 4500;
const int buzzer_stop_freq = 5000;

//variables
bool allow_future_timer = true;
bool no_action_running = true;
volatile bool ok = LOW; // state of ok button

//SetTimeout_G timer1;  // test code:: remove after debugging
void setup(){
  pinMode(ok_button, INPUT);
  pinMode(heater, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(LED_heater_on, OUTPUT);
  pinMode(LED_heater_off, OUTPUT);
  
  onPinOnly(LED_heater_off);
  //external interrupts
 //attachInterrupt(digitalPinToInterrupt(ok_button), button_state_high_ISR, RISING);
 attachInterrupt(digitalPinToInterrupt(ok_button), button_state_change_ISR, CHANGE);
 //attachInterrupt(digitalPinToInterrupt(ok_button), button_state_low_ISR, FALLING);

 //internal interrupts
 //Timer1.initialize(1000000); //call check_savedTimer_ISR() after every 1 second.
 //Timer1.attachInterrupt(check_savedTimer_ISR);
  
  //lcd
  lcd.begin(16,2);  

  
  Serial.begin(9600);
 // onPin(heater); // remove line after debugging

  //temp sensor
  temp_sensor1.begin();

  //RTC 
   if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
	
	// Comment out below lines once you set the date & time.
    // Following line sets the RTC to the date & time this sketch was compiled
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	
    // Following line sets the RTC with an explicit date & time
    // for example to set January 27 2017 at 12:56 you would call:
    // rtc.adjust(DateTime(2017, 1, 27, 12, 56, 0));
  }
  
}
     //// main >>>>>>>>
void loop() {
   start_up();

}
      ////<<<<<<<<<<<

inline void start_up(){
  
  lcd.setCursor(0,0);
  lcd.print(welcome_message);
  
  delay(welcome_delay);
  while(1){
        choice choosen_action = makeChoice(knob, 5, ok, "    Action?    ", true, actions, 2.0);
        switch (choosen_action) {
          case timer_c: timer();
          break;
          case dateTimer_c: dateTimer();
          break;
          case setTemp_c: setTemp();
          break;
          case seeFutureTimer_c: seeFutureTimer();
          break;
          case displayTime_c: displayTime();
          break;
          }
    }
    
  }
  
void timer(){
  no_action_running = false;
  int time_m = makeChoice(knob, 60, ok_button, "-- SELECT TIME--", false); // select a desired time in minutes from 0 to 60 min

  onPin(heater); onPinOnly(LED_heater_on); // turn on heater
  buzzer_start(buzzer, 1500);
  if (countdown(timer1, time_m*60)){ // count down time_m in seconds using timer1 timer and when complete, off heater and sound buzzer
    offPin(heater); // off heater
    onPinOnly(LED_heater_off);
    buzzer_stop(buzzer, 2000);
    }
  else {offPin(heater); onPinOnly(LED_heater_off); buzzer_stop(buzzer, 1000);};
  no_action_running = true;      
 }

 //end of timer()

 void dateTimer( ){
   no_action_running = false;
   offPin(heater);
   int choosen_day = makeChoice(knob, 8, ok, " --Choose Day-- ", true,  daysOfTheWeek);
   int choosen_hour = makeChoice(knob, 24, ok, "--Choose Hour-- ", false);
   int choosen_minute = makeChoice(knob, 60, ok, "-Choose Minutes-", false);
   int onMinutes = makeChoice(knob, 60, ok, "-- On Minutes? -", false);
   String choosen_minute_str = String(choosen_minute);
   if (choosen_minute_str.length() == 1) choosen_minute_str = "0" + choosen_minute_str;
   lcd_clearRow(0);
   lcd.print("Choosen DateTime");
   lcd_clearRow(1);
   lcd.print(daysOfTheWeek[choosen_day]); lcd.print(" "); lcd.print(choosen_hour); lcd.print(":"); lcd.print(choosen_minute_str);

   DateTime present = rtc.now();
   saveTimer(present.month(), choosen_day, choosen_hour, choosen_minute, onMinutes);

   while(1){
      present = rtc.now();
      //delay(250);
      //if (digitalRead(ok_button)) return;
      if (ok) {no_action_running = true; onPinOnly(LED_heater_off); return;}
      bool isDay = false;
      if (daysOfTheWeek[choosen_day] == "Everyday") isDay = true;
      else isDay = (present.dayOfTheWeek() == choosen_day);
      bool isHour = (present.hour() == choosen_hour);
      bool isMinute = (present.minute() == choosen_minute);
      if (isDay and isHour and isMinute) { //check if choosing present dateTime is same as set time and if same, on heater and when done, off heater.
            onPin(heater); onPinOnly(LED_heater_on);
            buzzer_start(buzzer, 1500);
            if (countdown(timer1, onMinutes*60)){ // count down time_m in seconds using timer1 timer and when complete, off heater and sound buzzer
              offPin(heater);
              onPinOnly(LED_heater_off);
              buzzer_stop(buzzer, 5000);
            }
      }


      // Serial.print("day: "); Serial.println(present.dayOfTheWeek());
      // Serial.print("hour: "); Serial.println(present.hour());
      // Serial.print("minute: "); Serial.println(present.minute());
      // Serial.println("");

   }

   no_action_running = true;
 
 }
 //end of dateTimer()

void setTemp( ){
    SetTimeout_G timer2;
    no_action_running = false;
    int lowTemp = makeChoice(knob, 150, ok_button, "- Set LOW Temp -", false);
    int highTemp = makeChoice(knob, 150, ok_button, "-Set HIGH Temp-", false);
    lcd_clearRow(0);
    lcd.print("LOW:"); lcd.print(lowTemp); lcd.print(" "); lcd.print("HIGH:"); lcd.print(highTemp);
    lcd_clearRow(1);
    int temperature = readTemp_( );
    int previous_temp = temperature;
    lcd.print("   temp: "); lcd.print(temperature);

    delay(250);
    
    // keep temp between high and low temp
    //ok = false;
    timer2.reset();
    while(1){
        delay(50);
        //if (digitalRead(ok_button)) return;
        if (ok) { no_action_running = true; onPinOnly(LED_heater_off); return;} 
        if (timer2.delay(800)){temperature =  readTemp_(); timer2.again();}
        Serial.println(temperature);
        if (temperature != previous_temp){
          lcd_clearRow_Portion(1, 9, 15);
          lcd.print(temperature);
          previous_temp = temperature;
        }
        if ((highTemp - lowTemp)>= 2){

          if (temperature<=lowTemp){onPin(heater); } //buzzer_start onPinOnly(LED_heater_on);
          else if (temperature>=highTemp) {offPin(heater); onPinOnly(LED_heater_off);}  //buzzer_stop
        }

    }

    no_action_running = true;

 }
//end of setTemp()

void seeFutureTimer(){
    time_struct savedTimer = readSavedTimer();
    String day = daysOfTheWeek[savedTimer.day];
    String hour = String(savedTimer.hour);
    String minute = String(savedTimer.minute);
    String onMinutes = String(savedTimer.onMinutes);
    if (minute.length() == 1) minute = "0" + minute;
    bool repeat = savedTimer.repeat;
    bool completed = savedTimer.completed;
    char* timer_actions[] = {"--- Activate ---", "-- Deactivate --", "*See SAVED Timer"};
    enum action {activate, deactivate, seeSavedTimer};
    action timer_action = makeChoice(knob, 3, ok_button, "ACTION on TIMER?", true, timer_actions);
    if (timer_action == activate) {
      allow_future_timer = true;
      lcd_clearRow(0);
      lcd.print("Active - " + onMinutes + "min");
      lcd_clearRow(1);
      lcd.print(day + " " + hour + ":" + minute);
      }
    else if (timer_action == deactivate) {
      allow_future_timer = false;
      lcd_clearRow(0);
      lcd.print("Inactive - " + onMinutes + "min");
      lcd_clearRow(1);
      lcd.print(day + " " + hour + ":" + minute);
    }
    else if (timer_action == seeSavedTimer){
      String timer_state = (allow_future_timer)? "ACTIVE - " : "INACTIVE - ";
      lcd_clearRow(0);
      lcd.print(timer_state + onMinutes + "min");
      lcd_clearRow(1);
      lcd.print(day + " " + hour + ":" + minute);
    }
    //ok = false;
    while(1){
        check_savedTimer_ISR(); // sheck is saved timer is set to go
        delay(100);
        if (ok) return;
    }

}
//end of seeFutureTimer()

void displayTime(){
 // ok = false;
  while(1){
      check_savedTimer_ISR(); // check if saved timer is set to go

      DateTime now = rtc.now();
      String month = String(monthsOfTheYear[now.month()]);
      int day = now.day();
      String week_day = String(daysOfTheWeek[now.dayOfTheWeek()]);
      int hour = now.hour();
      String minute = String(now.minute());
      if (minute.length() == 1) minute = "0" + minute;

      lcd.setCursor(0,0);
      lcd.print(centerText(month + " " + day)); // this line hangs the code
      lcd.setCursor(0,1);
      lcd.print(centerText(week_day + " " + hour + ":" + minute));
      delay(300);
      if (ok) return;
  }
}
//end of displayTime()

inline int get_knob_division(uint8_t knob, int divisions){      
      int div =  map(analogRead(knob), 0, 1023, 0, divisions);  // divide knob into divisions and read division
      if (div == divisions)return (divisions - 1);
      else return div;  
  }

inline float readTemp__(uint8_t temp_sensor){
      return map(analogRead(temp_sensor), 0.0, 307.5, 0.0, 150.0);
 }

float readTemp_( ){
  temp_sensor1.requestTemperatures();
  float temp = temp_sensor1.getTempCByIndex(0);
  return temp;
}


void onPinOnly(uint8_t pin){
  static uint8_t previous_pin;
  static bool first_call = true;
  if (pin != previous_pin){
      onPin(pin);
      offPin(previous_pin);
      previous_pin = pin;
  }
};

inline void onPin(uint8_t pin){
  digitalWrite(pin, HIGH);
  }

inline void offPin(uint8_t pin){
  digitalWrite(pin, LOW);
  }

inline void buzzer_start(uint8_t buzzer, long delay_time){
    tone(buzzer, buzzer_start_freq);
    delay(delay_time);
    noTone(buzzer);
  }
inline void buzzer_stop(uint8_t buzzer, long delay_time){
    tone(buzzer, buzzer_stop_freq);
    delay(delay_time);
    noTone(buzzer);
  }


void button_state_high_ISR(){
    ok = HIGH;
   // Serial.println("button pressed"); // for debugging
  }

void button_state_low_ISR(){
    ok = LOW;
    //Serial.println("button released"); // for Debugging
  }
void button_state_change_ISR(){
    ok = !ok;
  }

void lcd_clearRow(int row){

  lcd.setCursor(0, row);
  lcd.print("                    ");
  lcd.setCursor(0,row);
  
  }

void lcd_clearRow_Portion(unsigned int row, unsigned int col_start, unsigned int col_stop){
    lcd.setCursor(col_start, row);
    if (col_start == col_stop){
      lcd.print(" ");
      lcd.setCursor(col_start, row);
      return;
    }
    else{
      for(int i = 0; i<(1+ col_stop - col_start); i++){
        lcd.print(" ");
      }
      lcd.setCursor(col_start, row);
    }
}

int makeChoice(uint8_t knob, unsigned int divisions, bool ok_, char* row1, bool useArray, char* row2_array[], unsigned int button_delay, float useShortFunction){
    
    int choice = get_knob_division(knob, divisions);
    int previous_choice = get_knob_division(knob, divisions);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(row1);
    lcd_clearRow(1);
    if (useArray) lcd.print(row2_array[choice]);
    else lcd.print(choice);
    //while(1); //for debugging only
    delay(250);
    while(1){
      delay(100);
      if (useShortFunction == 2.0) check_savedTimer_ISR;
      choice = get_knob_division(knob, divisions);
      if (choice != previous_choice){ // update lcd only when present choice is different from previous choice
        lcd_clearRow(1);
        if (useArray) lcd.print(row2_array[choice]);
        else lcd.print(choice);
        previous_choice = choice;
      }
      if (ok) { return choice;}
      
    }

}

bool countdown(SetTimeout_G timer, unsigned long seconds){
    lcd_clearRow(0);
    lcd.print("set time: "); lcd.print(seconds);
    lcd_clearRow(1);
    lcd.print("time left: ");
    timer.reset(); // reset timer just in case it was being used somewhere
    // loop to countdown from choosen time
    while(1){
      delay(50);
      //if (digitalRead(ok_button)){ timer.reset(); return false;}
      if (timer.delay(seconds*1000)){ //convert seconds to milliseconds
        lcd.setCursor(11,1); 
        String timeLeft = String(0) + "    "; // the right space removes static numbers on lcd at the end of countdown
        lcd.print(timeLeft); // print time left to return true
        timer.again(); // prepare timer for the next call
        return true;
        }      
      else {
        lcd.setCursor(11,1); 
        String timeLeft = String(seconds - timer.elapsedSeconds()) + "    "; // the right space removes static numbers on lcd during countdown
        lcd.print(timeLeft); // print time left to return true
        //if (digitalRead(ok_button)){ timer.reset(); return false;}
        if (ok){timer.reset(); return false;}
        }
      }
}

String centerText(String text){
  int i = 1;
  String centered_text = text;

  while(centered_text.length() < 16){
    if (i%2) centered_text = centered_text + " ";
    else centered_text = " " + centered_text;
    i++;
  }
  return centered_text;

}


void saveTimer(int month, int day, int hour, int minute, int onMinutes, int second, bool repeat, bool completed){

  EEPROM.update(month_address, month);
  EEPROM.update(day_address, day);
  EEPROM.update(hour_address, hour);
  EEPROM.update(minute_address, minute);
  EEPROM.update(onMinutes_address, onMinutes);
  EEPROM.update(repeat_address, repeat);
  EEPROM.update(second_address, second);
  EEPROM.update(completed_address, completed);

}

time_struct readSavedTimer(){
     time_struct timer;
      timer.month = EEPROM.read(month_address);
      timer.day = EEPROM.read(day_address);
      timer.hour = EEPROM.read(hour_address);
      timer.minute = EEPROM.read(minute_address);
      timer.onMinutes = EEPROM.read(onMinutes_address);
      timer.second = EEPROM.read(second_address);
      timer.repeat = EEPROM.read(repeat_address);
      timer.completed = EEPROM.read(completed_address);
  
    return timer;
}

void check_savedTimer_ISR(){
      if (no_action_running * allow_future_timer == false) return;
      DateTime present = rtc.now();
      time_struct timer = readSavedTimer();
      bool isDay = false;
      if (daysOfTheWeek[timer.day] == daysOfTheWeek[0]) isDay = true;
      else isDay = (present.dayOfTheWeek() == timer.day);
      bool isHour = (present.hour() == timer.hour);
      bool isMinute = (present.minute() == timer.minute);
      if (isDay and isHour and isMinute) { 
        //check if choosing present dateTime is same as set time and that no other action is running
        // and if true, on heater and when done, off heater.
            onPin(heater);
            onPinOnly(LED_heater_on);
            buzzer_start(buzzer, 10000);
            if (countdown(timer1, timer.onMinutes*60)){ // count down time_m in seconds using timer1 timer and when complete, off heater and sound buzzer
              offPin(heater); // 
              onPinOnly(LED_heater_off);
              buzzer_stop(buzzer, 10000);
            }
            else {offPin(heater); onPinOnly(LED_heater_off); buzzer_stop(buzzer, 1500); }
      }
}