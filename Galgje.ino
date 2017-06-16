/*
 * Arduino hangman with PS2 keyboard and LiquidCristal LCD display. Made for a school project, second year mechatronics.
 * The game can be started by turning the power on or, if already turned on, by pressing ESC. 
 * The ESC button on the keyboard serves as a reset function and can be used at any stage of the game, as long as the arduino has power.
 * The game is played by two players. The first player can enter a word of max twelve letters (any more will not be stored correctly), only a-z are valid inputs.
 * If a mistake is made the ESC button will need to be pressed to reset the game. Once the word is entered the first player can press ENTER to iniciate the next phase.
 * Player two will now be able to guess letters by pressing them on the keyboard. Only a-z are valid inputs.
 * If a correct letter is pressed it will appear on the correct spot along the ___ line. If there are multiple of this letter they will all appear.
 * If a player makes an incorrect guess a part of the gallows will be build. After 7 wrong guesses player two will lose.
 * If player two wins 'Victory!' will appear on the screen. If player two lost ' You lose!' will appear on the screen.
 * If a letter is guessed that already has been used (correct or incorrect) then it will say so on the screen and the game will not take further action towards the score.
 * The system can be powered by either an USB connection to a computer or by connecting a 9V battery to the arduino.
 * 
 * Wiring and code developed by Mike Van Pelt - 14/06/2017
 */


#include <PS2Keyboard.h>
#include <LiquidCrystal.h>
#include <ShiftRegister74HC595.h>
 
ShiftRegister74HC595 sr (1, A0, A1, A2);  // shift register object (number of shift registers, data pin, clock pin, latch pin)
 
LiquidCrystal lcd(12, 11, 4, 5, 6, 7);  // RS- p12, Enable- p11, D4-7- p4-7

void(* resetFunc) (void) = 0;  //Declare reset fuction at address 0
 
PS2Keyboard keyboard;
const int DataPin = 2;   // Data pin
const int IRQpin =  3;   // Clock pin
int we = 1;  //Word input - 1=input word - 0=guess word
int n[12];  //Word array
static int o[26];  //Guess array
int j = 0;  //Number of letters in word, 0 base
int k = 0;  //Number of letters guessed correctly
int l = 0;  //Variable to check if a letter is wrong
int a = 7;  //Variable to control the gallows and keep track of the lose score
char c;  //Character input
byte d;  //Raw data input

//Create the custom character used to build the gallows
byte customChar6[8] = {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111, 0b11111, 0b11111};  //Base - (14,1)
byte customChar5[8] = {0b11111, 0b10100, 0b11000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000};  //Top left - (14,0)
byte customChar4[8] = {0b11100, 0b00100, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000};  //Rope - (15,0)
byte customChar3[8] = {0b11100, 0b00100, 0b01110, 0b01010, 0b01110, 0b00000, 0b00000, 0b00000};  //Head - (15,0)
byte customChar2[8] = {0b11100, 0b00100, 0b01110, 0b01010, 0b01110, 0b00100, 0b00100, 0b00100};  //Body - (15,0)
byte customChar1[8] = {0b11100, 0b00100, 0b01110, 0b01010, 0b01110, 0b10101, 0b01110, 0b00100};  //Arms - (15,0)
byte customChar0[8] = {0b00100, 0b00100, 0b01010, 0b10001, 0b00000, 0b00000, 0b00000, 0b00000};  //Legs - (15,1)


void setup(){
   keyboard.begin(DataPin, IRQpin);  //Initialize keyboard  
   lcd.begin(16, 2);  //Initialize lcd screen                
   lcd.setCursor(0,0);  //Set lcd cursor position              
   lcd.print("Enter word max12");  //Print text on lcd screen
   lcd.setCursor(0,1);
   lcd.cursor();     //Sets visible cursor on lcd screen current position
   Serial.begin(9600);  //Initialize serial monitor (used for testing)
}
 
void loop(){
  //Code for entering a word
  while (we == 1) {
    if (keyboard.available()) {  //Checks if they keyboard is active
      c = d = keyboard.read();  //Reads the keyboard raw input data
      uint8_t pinValues[] = {c}; // The ASCII value of 'c' being sent out of the (SR)
      sr.setAll(pinValues);      // shift register.

      if (c == PS2_ESC) {
        resetFunc();  //Reset the program
      }

      if (d >= 97 && d <= 122) {  //Code to check for invalid letters (only a-z allowed)
        lcd.setCursor(j,1);
        n[j] = d;  //Stores the letter in the array
        j++;  //Increases the letter count of the word 
        lcd.print(c);
      }
      else if (c == PS2_ENTER) {  //Check if enter key is pressed (word is done)
        lcd.clear();
        lcd.setCursor(0,0);
        for (int i = 0; i < j; i++) {
          lcd.print("_");  //Print a number of _ equal to word size
        }
        we = 0;  //Set this value to 0 to initialize guessing phase
        lcd.setCursor(0,1);
      }
      else {
        lcd.setCursor(0,0);
        lcd.print("Letter invalid  ");
        delay(2000);
        lcd.setCursor(0,0);
        lcd.print("Enter word max12");
      }
      Serial.println(c);  //For testing
      Serial.println(d);  //For testing
    }
  }

  //Code for guessing a word
  while (we == 0) {
    if (keyboard.available()) {
      c = d = keyboard.read();
      uint8_t pinValues[] = {c}; // The ASCII value of 'c' being sent out of the (SR)
      sr.setAll(pinValues);      // shift register.
      Serial.println(c);  //For testing
      Serial.println(d);  //For testing

      if (c == PS2_ESC) {
        resetFunc();  //Reset the program
      }

      int t = d - 97;  //Calculate the value of the unicode (where a = 0, z = 25)
      if (d >= 97 && d <= 122) {  //Code to check for invalid letters (only a-z allowed)
        if (o[t] == 0) {  //Check if the character was already used
          o[t] = 1;  //Set the value of the array that corresponds with place in alphabet to 1 
          for (int i = 0; i <= j; i++) {  //Check every character in the word
            if (n[i] == d) {  //Check if input equals a letter in the word
              lcd.setCursor(i,0);  //Place cursor in the correct spot in the word
              lcd.print(c);
              k++;  //Increase letter guessed correctly count
              if (k == j && a > 0) {  //Check if all the letters are guessed correctly
                lcd.setCursor(0,1);
                lcd.print("Victory!");
              }
            }
            else {
              l++;  //Increase l for each letter that is wrong (has to check all letters in word)
              if (l == j+1 && k != j) {  //Checks if none of the letters corresponded with the word
                a--;
                switch (a){  //Reads value of a and builds the corrosponding pieces of the gallows
                  case 0:  //Legs - Game over
                    lcd.createChar(0, customChar0);  //Create the custom character with data from the customChar arrays
                    lcd.setCursor(15, 1);
                    lcd.write(byte(0));  //Same as print, except that it prints the raw byte instead of the converted ascii character
                    delay(1000);
                    lcd.setCursor(0,1);
                    lcd.print("You lose!");
                    break;
                  case 1:  //Arms
                    lcd.createChar(1, customChar1);
                    lcd.setCursor(15, 0);
                    lcd.write(byte(1));
                    break;
                  case 2:  //Body
                    lcd.createChar(1, customChar2);
                    lcd.setCursor(15, 0);
                    lcd.write(byte(1));
                    break;
                  case 3:  //Head
                    lcd.createChar(1, customChar3);
                    lcd.setCursor(15, 0);
                    lcd.write(byte(1));
                    break;
                  case 4:  //Rope
                    lcd.createChar(1, customChar4);
                    lcd.setCursor(15, 0);
                    lcd.write(byte(1));
                    break;
                  case 5:  //Top left
                    lcd.createChar(2, customChar5);
                    lcd.setCursor(14, 0);
                    lcd.write(byte(2));
                    break;
                  case 6:  //Base
                    lcd.createChar(3, customChar6);
                    lcd.setCursor(14, 1);
                    lcd.write(byte(3));
                    break;
                  default:
                    break;
                }
              }
            }
          }
        }
        else {  //Character was already used
          lcd.setCursor(0,1);
          lcd.print(c);
          lcd.setCursor(2,1);
          lcd.print("Already used");
          delay(2000);
          lcd.setCursor(0,1);
          lcd.print("              ");
        }
      }
      else {
        lcd.setCursor(0,1);
        lcd.print("Letter invalid");
        delay(2000);
        lcd.setCursor(0,1);
        lcd.print("              ");
      }
      l=t = 0;  //Reset values to be used again later
    }
  }
}
