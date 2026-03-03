// include the library code:
#include <LiquidCrystal.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <NewPing.h>
#include <EEPROM.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int buttonDelay = 300;

int menu = 0; // -1 - powered off, 0 - Home screen, 1 - Menu selector 1, 2 - menu selector 2, 3 - menu selector 3, 4 - delay selector
unsigned long lastRead = 0;
const int buttonLadderPin = A5;
unsigned long startUse = 0;
const int powerButtonPin = 13;

const int contactSensorPin = A1;
bool doorOpen = true; //we assume that the door is open when the program starts

int currentSprayDelay = 3;
int currentDelayAddress = sizeof(int);
int sprayDelayShown = -1; // -1 - current delay, 0 - 1000ms, 1 - 10000ms, 2 - 60000ms, 3 - 600000
bool poweredOff = false;

int sprayCount = 2400;
int sprayCountAddress = 0;
int temp = 0;

const int redLED = 9;
const int greenLED = 10;

const int lightSensor = A0;

#define ONE_WIRE_BUS 8
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature temperature(&oneWire);

int spraysExpected = 0;
unsigned long sprayScheduled = 0;

const int motionPin = 7;
unsigned long lastMotion = 0;

NewPing sonar(A3, A4);
int distance = 0;

int currentState = 0;

void setup() {
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  temperature.begin();
  pinMode(lightSensor, INPUT);
  pinMode(motionPin, INPUT);

  pinMode(buttonLadderPin, INPUT_PULLUP);
  pinMode(contactSensorPin, INPUT_PULLUP);
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  lastRead = millis();
  lastMotion = millis();
  digitalWrite(greenLED, HIGH);

  attachInterrupt(contactSensorPin, Door, CHANGE);

  //Uncomment these lines **once** so the EEPROM gets set up properly, else it will (sometimes) not function
  //EEPROM.put(sprayCountAddress, sprayCount);
  //EEPROM.put(currentDelayAddress, currentSprayDelay);
  EEPROM.get(sprayCountAddress, sprayCount);
  EEPROM.get(currentDelayAddress, currentSprayDelay);


  // Print a message to the LCD.
  printMenu();
}

void loop() {

  readButtons();

  int lastState = currentState;
  currentState = state(currentState);
  if(lastState != currentState) updateStateRGB();

  //go spray on delay based on state
  sprayIfNecessary();
}

void spray(){
  //code that does a spray
  sprayCount -= 1;
  EEPROM.put(sprayCountAddress, sprayCount);
}

// 0; In Use - Type Unknown:  CYAN
// 1; In Use - Number 1:      YELLOW
// 2; In Use - Number 2:      RED
// 3; In Use - Cleaning:      BLUE
// 4; Not In Use:             GREEN
// if powered off,            LED off
void updateStateRGB(){
  if(poweredOff) {
    digitalWrite(redLED, LOW);
  }
  else{
    switch(currentState){
      case 0:
      case 1:
      case 2:
      case 3:
        digitalWrite(redLED, HIGH);
        break;
      case 4:
        digitalWrite(redLED, LOW);
        break;
    }
  }
}

void sprayIfNecessary(){
  if((menu == 0) && (spraysExpected > 0) && (millis() - sprayScheduled >= currentSprayDelay)){
    spray();
    spraysExpected -= 1;
    sprayScheduled = millis();
  }
}

// Function that will read button ladder input buttonDelay after the last press
void readButtons(){
  if(millis() - lastRead >= buttonDelay){
    int buttonLadder = analogRead(buttonLadderPin);
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
      EEPROM.put(sprayCountAddress, sprayCount);
      menu = 0;
      sprayScheduled = millis();
      printMenu();
      break;
    case 3:
      menu = 0;
      sprayScheduled = millis();
      printMenu();
      break;
    case 4:
      //code that changes the delay based on the displayed delay
      if (sprayDelayShown != -1) {
        currentSprayDelay = sprayDelayShown;
        EEPROM.put(currentDelayAddress, currentSprayDelay);
      }
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
        EEPROM.put(sprayCountAddress, sprayCount);
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
    sprayScheduled = millis();
    currentState = 4;
    updateStateRGB();
    printMenu();
  }
  else {
    poweredOff = true;
    menu = -1;
    updateStateRGB();
    printMenu();
  }
}


bool doorBeenClosed = false;
bool satDown = false;
int toiletPaperUsed = 0;

// Function that returns state:
// 0; In Use - Type Unknown
// 1; In Use - Number 1
// 2; In Use - Number 2
// 3; In Use - Cleaning
// 4; Not In Use
int state(int prevState){
  if (InUse() == false){ 
    return 4;
  }
  else if (prevState == 4){//if the use *just* started
    resetStateValues();
    return 0; //since the loop repeats in less than a second, we dont have to start measuring inmediately
  }
  else if (prevState != 0) //if a state has already been decided, just return that state
    return prevState;

  //door closes and no decision has been made yet
  //gather the data needed to determine weather they are pooping or peeing
  if (doorOpen == false)
  {
    doorBeenClosed = true;
    //check for pooping and peeing
    if (SitDown()){
      satDown == true;
    }

    if (ToiletPaperTaken())
    {
      //adjust values based on timing and lightuse and stuff???
      toiletPaperUsed += 1;
    }
  }
  //if the door has not been closed yet, and the restroom  has been in use for longer than a minute
  if (doorOpen && doorBeenClosed == false && millis() - startUse > 60000)
  {
    return 3;
  }

  //if the door is opened again after it has been closed
  //decide weather the person has been pooping or peeing
  if (doorOpen && doorBeenClosed == true)
  {
    if (satDown == false || toiletPaperUsed <= 4000)
      return 1;
    else
      return 2;
  }

  return prevState;
}

void resetStateValues()
{
  startUse = millis(); 
  doorBeenClosed = false;
  satDown = false;
  toiletPaperUsed = 0;
}

bool InUse()
{
  //if the light is off, we dont need to check for anything else
  if (LightOn == false)
    return false;

  //check if there is motion
  if (digitalRead(motionPin) == HIGH)
    lastMotion = millis();
  
  //if there has been no motion for a few seconds its empty
  if (millis() - lastMotion < 5000)
      return false;

  //also check distance??

  else
    return true;
}

bool LightOn()
{
  int l = analogRead(lightSensor);
  if (l < 300)
    return false;
  return true;
}

bool SitDown()
{
  if (sonar.ping_cm() < 300)
    return true;
  return false;
}

bool ToiletPaperTaken()
{
  int l = analogRead(lightSensor);
  if (l < 500)
    return true;
  return false;
}

void Door(){
  doorOpen = !doorOpen;
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
  temp = temperature.getTempCByIndex(0);
  lcd.print(temp);
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