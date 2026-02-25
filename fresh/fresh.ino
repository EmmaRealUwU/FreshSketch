/*
  LiquidCrystal Library - Hello World

 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

 This sketch prints "Hello World!" to the LCD
 and shows the time.

  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 modified 7 Nov 2016
 by Arturo Guadalupi

 This example code is in the public domain.

 https://docs.arduino.cc/learn/electronics/lcd-displays

*/

// include the library code:
#include <LiquidCrystal.h>

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

int currentDelay = 1000;
int delayShown = 0; //0 - 1000ms, 1 - 10000ms, 2 - 60000ms, 3 - 600000
bool poweredOff = false;
bool lastPressed = false;

int sprayCount = 2400;
int temp = 21;

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  printMain();
  pinMode(A5, INPUT_PULLUP);
  lastRead = millis();
}

void loop() {

  if(poweredOff){
    //seizes functionality when powered off
  }
  else {
    //after buttondelay, the value is read off port A5
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

      // set the cursor to column 0, line 1
      // (note: line 1 is the second row, since counting begins with 0):
      //lcd.setCursor(0, 1);
      // print the read value:
      //lcd.print(buttonLadder);
    }
  }
}


void spray(){
}

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

void printMenuSelector1(){
  lcd.setCursor(0, 0);
  lcd.print("     Delay      ");

  lcd.setCursor(0, 1);
  lcd.print("Edit        Next");
}

void printMenuSelector2(){
  lcd.setCursor(0, 0);
  lcd.print("  Spray Count   ");
  
  lcd.setCursor(0, 1);
  lcd.print("Reset       Next");
}

void printMenuSelector3(){
  lcd.setCursor(0, 0);
  lcd.print("      Quit      ");

  lcd.setCursor(0, 1);
  lcd.print("Confirm     Next");
}

void printDelaySelector(){
  lcd.setCursor(0, 0);
  lcd.print("cant be bothered");
  
  lcd.setCursor(0, 1);
  lcd.print("     W.I.P.     ");
}

void buttonPress1() {
  switch (menu){
    case -1:
      break;
    case 0:
      menu = 1;
      printMenuSelector1();
      break;
    case 1:
      menu = 4;
      printDelaySelector();
      break;
    case 2:
      sprayCount = 2400;
      break;
    case 3:
      menu = 0;
      printMain();
      break;
    case 4:
      //code that changes the delay based on the displayed delay
      menu = 1;
      printMenuSelector1();
  }
}

void buttonPress2() {
  switch (menu){
    case -1:
      break;
    case 0:
      spray();
      sprayCount -= 1;
      lcd.setCursor(0, 0);
      lcd.print(sprayCount);
      lcd.print(" sprays ");
      break;
    case 1:
      menu = 2;
      printMenuSelector2();
      break;
    case 2:
      menu = 3;
      printMenuSelector3();
      break;
    case 3:
      menu = 1;
      printMenuSelector1();
      break;
    case 4:
      //code that changes the displayed delay here
      break;
  }
}

void buttonPress3() {
  switch (menu){
    case (-1):
      poweredOff = false;
      menu = 0;
      printMain();
      break;
    default:
      poweredOff = true;
      menu = -1;
      lcd.clear();
      lcd.print("  Powered Off   ");
  }
}
