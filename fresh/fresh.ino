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
unsigned long lastPowerButtonPress = 0;
bool powerButtonInterruptAttached = true;

const int contactSensorPin = A1;
bool doorOpen = true; //we assume that the door is open when the program starts

int currentSprayDelay = 3;
int currentDelayAddress = sizeof(int);
int sprayDelayShown = -1; // -1 - current delay, 0 - 15000ms, 1 - 30000ms, 2 - 60000ms, 3 - 600000
bool poweredOff = false;

int lastSavedSprayCount = 2400;
int sprayCount = 2400;
int sprayCountAddress = 0;
int temp = 0;
unsigned long lastPolledTemp = 0;

const int freshenerPin = 6;

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
unsigned long motionStartedCalibrating = 0;
bool motionDoneCalibrating = false;

unsigned long freshenerTurntOn = 0;
unsigned long freshenerTurntOff = 0;
bool waitingForSpray = false;

NewPing sonar(A3, A4);
int distance = 0;

int currentState = 0;

void setup() {
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  temperature.begin();
  pinMode(lightSensor, INPUT);
  pinMode(motionPin, INPUT);

  // sets up pins for input and output
  pinMode(buttonLadderPin, INPUT_PULLUP);
  pinMode(contactSensorPin, INPUT_PULLUP);
  pinMode(freshenerPin, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);

  // ensures time stamps make sense
  lastRead = millis();
  lastMotion = millis();
  motionStartedCalibrating = millis();

  // sets green LED on to signal it is powered
  digitalWrite(greenLED, HIGH);

  // attaches interrupts to contact sensor and power button
  attachInterrupt(digitalPinToInterrupt(contactSensorPin), Door, CHANGE);
  attachInterrupt(digitalPinToInterrupt(powerButtonPin), debouncedPowerButton, FALLING);

  //Uncomment these lines **once** so the EEPROM gets set up properly, else there is no guarantee that it will function
  //EEPROM.put(sprayCountAddress, sprayCount);
  //EEPROM.put(currentDelayAddress, currentSprayDelay);
  EEPROM.get(sprayCountAddress, sprayCount);
  EEPROM.get(currentDelayAddress, currentSprayDelay);
  lastSavedSprayCount = sprayCount;

  // Prints the home screen
  printMenu();
}

void loop() {

  readButtons();

  //if the state changes, the LEDs will reflect this
  int lastState = currentState;
  currentState = state(currentState);
  if(lastState != currentState) updateStateRGB();

  //ensures motion sensor is done calibrating before asking for data
  if(!motionDoneCalibrating && (millis() - motionStartedCalibrating >= 60000)){
    motionDoneCalibrating = true;
  }

  //queues a spray on delay based on state
  sprayIfNecessary();
  reattachPowerButtonInterruptIfNecessary();
  pollTempIfNecessary();
}

void spray(){
  //save millis() so can be undone after 16000ms
  if(millis() - freshenerTurntOff >= 1000){
    freshenerTurntOn = millis();
    digitalWrite(freshenerPin, HIGH);
    waitingForSpray = true;
    spraysExpected -= 1;
    sprayCount -= 1;
    sprayScheduled = millis();
  }
}

//after a delay of 250ms the power button is available for pressing again
void reattachPowerButtonInterruptIfNecessary() {
  if((!powerButtonInterruptAttached) && millis() - lastPowerButtonPress > 250) {
    attachInterrupt(digitalPinToInterrupt(powerButtonPin), debouncedPowerButton, FALLING);
    powerButtonInterruptAttached = true;
  } 
}

//if temperature hasnt been measures in 5 seconds, it refreshes and reprints the menu
void pollTempIfNecessary(){
  if(millis() - lastPolledTemp >= 5000){
    temperature.requestTemperatures();
    temp = temperature.getTempCByIndex(0);
    lastPolledTemp = millis();
    printMenu();
  }
}

// 0; In Use - Type Unknown:  CYAN
// 1; In Use - Number 1:      YELLOW
// 2; In Use - Number 2:      RED
// 3; In Use - Cleaning:      BLUE
// 4; Not In Use:             GREEN
// if powered off,            LED off
void updateStateRGB() {
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
  //if on home screen, and there are sprays expected, it sets up the freshener to spray (if not already waiting for one)
  if((!waitingForSpray) && (menu == 0) && (spraysExpected > 0) && (millis() - sprayScheduled + 15000 >= currentSprayDelayms())){
    spray();
  }

  //if the freshener has been on for more than 15 seconds (with leeway), it registers that a spray has actually happened, turning the freshener off, freeing it up for another spray, and saving the actual spray coutn to EEPROM
  if(waitingForSpray && (millis() - freshenerTurntOn >= 16000)){
    waitingForSpray = false;
    EEPROM.put(sprayCountAddress, lastSavedSprayCount - 1);
    digitalWrite(freshenerPin, LOW);
    freshenerTurntOff = millis();
    sprayScheduled = freshenerTurntOff;
  }
}

// Function that will read button ladder input buttonDelay after the last press
void readButtons(){
  if(millis() - lastRead >= buttonDelay){
    int buttonLadder = analogRead(buttonLadderPin);
    switch (isButton(buttonLadder)) {
      case 1: 
        buttonPress1();
        lastRead = millis();
        break;
      case 2:
        buttonPress2();
        lastRead = millis();
        break;
    }
    // no button =~ 1005
    // button 1 =~ 27
    // button 2 =~ 228
  }
}

// Function that takes resistor ladder value and returns which button is pressed (1 - 2 for buttons 1 and 2. 0 is reserved for noise values)
int isButton(int x){
  if (x < 80){
    return 1;
  }
  else if (x < 275){
    return 2;
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
        spraysExpected += 1;
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

void debouncedPowerButton() {
  buttonPress3();
  lastPowerButtonPress = millis();
  detachInterrupt(digitalPinToInterrupt(powerButtonPin));
  powerButtonInterruptAttached = false;
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
    if (satDown == false || toiletPaperUsed <= 4000) {
      sprayScheduled = millis();
      spraysExpected += 1;
      return 1;
    }
    else {
      sprayScheduled = millis();
      spraysExpected += 2;
      return 2;
    }
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

  //check if there is motion (only once motion sensor is done calibrating)
  if (motionDoneCalibrating){
    if (digitalRead(motionPin) == HIGH)
      lastMotion = millis();
  }
  
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
  if (sonar.ping_cm() < 150)
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
  switch(currentSprayDelay){
    case 0:
      return 15000;
      break;
    case 1:
      return 30000;
      break;
    case 2:
      return 60000;
      break;
    case 3:
      return 600000;
      break;
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
  
  lcd.print(temp);
  lcd.print("C");

  lcd.setCursor(0,1);
  lcd.print("Settings   Spray");
}

// Function that prints settings selector 1 to the lcd
void printMenuSelector1(){
  lcd.setCursor(0, 0);
  lcd.print("Dist: ");
  lcd.print(distance);
  //lcd.print("     Delay      ");

  lcd.setCursor(0, 1);
  lcd.print("Edit        Next");
}

// Function that prints settings selector 2 to the lcd
void printMenuSelector2(){
  lcd.setCursor(0, 0);
  lcd.print(analogRead(lightSensor));
  //lcd.print("  Spray Count   ");
  
  lcd.setCursor(0, 1);
  lcd.print("Reset       Next");
}

// Function that prints settings selector 3 to the lcd
void printMenuSelector3(){
  lcd.setCursor(0, 0);
  lcd.print(lastMotion);
  //lcd.print("      Quit      ");

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
          lcd.print("Delay is now 15s ");
          break;
        case 1:
          lcd.print("Delay is now 30s");
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