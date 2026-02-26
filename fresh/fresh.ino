// include the library code:
#include <LiquidCrystal.h>
#include <DallasTemperature.h>
#include <OneWire.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int buttonDelay = 300;

int menu = 0; // -1 - powered off, 0 - Home screen, 1 - Menu selector 1, 2 - menu selector 2, 3 - menu selector 3, 4 - delay selector
int buttonLadder = 0;
unsigned long lastRead = 0;

bool inUse = true;
bool number1 = false;
bool number2 = false;
bool cleaning = false;

int currentSprayDelay = 3;
int sprayDelayShown = -1; // -1 - current delay, 0 - 1000ms, 1 - 10000ms, 2 - 60000ms, 3 - 600000
bool poweredOff = false;

int sprayCount = 2400;
int temp = 21;

const uint8_t lightSensor = 10;

#define ONE_WIRE_BUS 8
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature temperature(&oneWire);

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  printMenu();
  pinMode(A5, INPUT_PULLUP);
  lastRead = millis();

  pinMode(lightSensor, INPUT);
  temperature.begin();
}

void loop() {
  readButtons();

}

void spray(){
}

// Function that will read button ladder input buttonDelay after the last press
void readButtons(){
  if(millis() - lastRead >= buttonDelay){
    buttonLadder = analogRead(5);
    switch (isButton(buttonLadder)) {
      case 0:
        //Code for debugging:
        //lcd.setCursor(0, 0);
        //lcd.print("last read: ");
        //lcd.print(lastRead);
        //lcd.setCursor(0, 1);
        //lcd.print("currently: ");
        //lcd.print(millis());
        break;
      case 1: 
        buttonPress1();
        lastRead = millis();
        break;
      case 2:
        buttonPress2();
        lastRead = millis();
        break;
      case 3:
        buttonPress3();
        lastRead = millis();
        break;
    }
    // no button =~ 1005
    // button 1 =~ 27
    // button 2 =~ 228
    // button 3 =~ 358
    // button 4 (magnetic contact sensor) =~ 449
  }
}

// Function that takes resistor ladder value and returns which button is pressed (1 - 3 for buttons, 4 for magnetic contact sensor, 0 for garbage values)
int isButton(int x){
  if (x < 80){
    return 1;
  }
  else if (x < 275){
    return 2;
  }
  else if (x < 400){
    return 3;
  }
  else if (x < 500){
    return 4;
  }
  else return 0;
}

// Function that handles button 1 (select button) being pressed
void buttonPress1() {
  switch (menu){
    case -1:
      break;
    case 0:
      menu = 1;
      printMenu();
      break;
    case 1:
      menu = 4;
      sprayDelayShown = -1;
      printMenu();
      break;
    case 2:
      sprayCount = 2400;
      menu = 0;
      printMenu();
      break;
    case 3:
      menu = 0;
      printMenu();
      break;
    case 4:
      //code that changes the delay based on the displayed delay
      if (sprayDelayShown != -1) currentSprayDelay = sprayDelayShown;
      menu = 1;
      printMenu();
      break;
  }
}

// Function that handles button 2 (interact button) being pressed
void buttonPress2() {
  switch (menu){
    case -1:
      break;
    case 0:
      if (sprayCount > 0){
        spray();
        sprayCount -= 1;
        printMenu();
      }
      break;
    case 1:
      menu = 2;
      printMenu();
      break;
    case 2:
      menu = 3;
      printMenu();
      break;
    case 3:
      menu = 1;
      printMenu();
      break;
    case 4:
      //code that changes the displayed delay here
      if(sprayDelayShown == 3){
        sprayDelayShown = -1;
      }
      else{
        sprayDelayShown += 1;
      }
      printMenu();
      break;
  }
}

// Function that handles button 3 (powerbutton) being pressed
void buttonPress3() {
  if (menu == -1){
    poweredOff = false;
    menu = 0;
    printMenu();
  }
  else {
    poweredOff = true;
    menu = -1;
    printMenu();
  }
}


// Function that returns state:
// 0; In Use - Type Unknown
// 1; In Use - Number 1
// 2; In Use - Number 2
// 3; In Use - Cleaning
int state(){
  if (inUse) {
    if (number1) {
      return 1;
    }
    else if (number2) {
      return 2;
    }
    else if (cleaning) {
      return 3;
    }
    else return 0;
  }
}

bool LightOn()
{
  int l = analogRead(lightSensor);
  if (l < 300)
    return false;
  return true;
}

// Function that returns the current spray delay in ms
int currentSprayDelayms(){
  switch (currentSprayDelay){
    default:
    case 0:
      return 1000;
    case 1:
      return 10000;
    case 2:
      return 60000;
    case 3:
      return 600000;
  }
}

// Function that prints the current menu to the lcd (TODO)
void printMenu(){
  switch (menu){
    case -1:
      lcd.clear();
      lcd.print("  Powered Off   ");
      break;
    case 0:
      printMain();
      break;
    case 1:
      printMenuSelector1();
      break;
    case 2:
      printMenuSelector2();
      break;
    case 3:
      printMenuSelector3();
      break;
    case 4:
      printDelaySelector();
      break;
  }
}

// Function that prints the home screen to the lcd
void printMain(){
  
  lcd.setCursor(0, 0);
  lcd.print(sprayCount);
  lcd.print(" sprays   ");
  lcd.setCursor(13, 0);

  temperature.requestTemperatures();
  float tempC = temperature.getTempCByIndex(0);
  lcd.print(temp );
  lcd.print(tempC);
  lcd.print("C");

  lcd.setCursor(0,1);
  lcd.print("Settings   Spray");
}

// Function that prints settings selector 1 to the lcd
void printMenuSelector1(){
  lcd.setCursor(0, 0);
  lcd.print("     Delay      ");

  lcd.setCursor(0, 1);
  lcd.print("Edit        Next");
}

// Function that prints settings selector 2 to the lcd
void printMenuSelector2(){
  lcd.setCursor(0, 0);
  lcd.print("  Spray Count   ");
  
  lcd.setCursor(0, 1);
  lcd.print("Reset       Next");
}

// Function that prints settings selector 3 to the lcd
void printMenuSelector3(){
  lcd.setCursor(0, 0);
  lcd.print("      Quit      ");

  lcd.setCursor(0, 1);
  lcd.print("Confirm     Next");
}

// Function that print the delay selector screen to the lcd
void printDelaySelector(){
  lcd.setCursor(0, 0);
  switch (sprayDelayShown){
    case -1:
      switch (currentSprayDelay){
        case 0:
          lcd.print("Delay is now 1s ");
          break;
        case 1:
          lcd.print("Delay is now 10s");
          break;
        case 2:
          lcd.print("Delay is now 1m ");
          break;
        case 3:
          lcd.print("Delay is now 10m");
          break;
      }
      break;
    case 0:
      lcd.print("   Delay: 1s    ");
      break;
    case 1:
      lcd.print("   Delay: 10s   ");
      break;
    case 2:
      lcd.print("   Delay: 1m    ");
      break;
    case 3:
      lcd.print("   Delay: 10m   ");
      break;
  }
  
  lcd.setCursor(0, 1);
  lcd.print("Set         Next");
}