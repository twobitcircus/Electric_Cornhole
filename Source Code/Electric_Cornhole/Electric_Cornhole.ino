/*
This is ELECTRIC CORNHOLE by Two Bit Circus!

Electric Cornhole is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Repetier-Firmware is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

To see a copy of the GNU General Public License
see <http://www.gnu.org/licenses/>.

This software is designed to work on the Intel Galileo Rev 2
along with an Adafruit amplified MP3 Shield
and an Adafruit Ptoto Screw Shield and an HT1632C 16x24 LED Matrix

http://www.adafruit.com/product/2188 - Intel Galileo R2
http://www.adafruit.com/products/1788 - Adafruit amplified MP3 Shield
http://www.adafruit.com/product/196 - Proto Screwshield / Wingshield
http://www.adafruit.com/products/555 - HT1632C 16x24 LED Matrix

In addition to the major electronic components, six microswitches will be required
along with two pushbuttons for selecting game type. 

These components can be sourced from any vendor, but some minor adjustments to 
the code, particularly pin assignments, may be required. This exercise is left 
to the student. 

The SD and SPI libraries are probably included with the defaule Galileo libraries.
Download and install the HT1632C 16x24 LED Matrix libraries from the 
vendor's website and take a peek at their tutorials and hookup guides as well.

No library is required for the MP3 shield, since all functionality is included
in this sketch. 

Main author: Two Bit Circus

*/


//#define SHUTUP // no audio
/************************************************
 * Scoring
 *************************************************/
// Each hole is assigned a different number of points
// One of these holes has a point multiplier set in the 
// game play section of this code. It makes the next score
// worth double, but you can change it to be anything,
// even to HALF points if you feel like being mean!
#define Hole0Points 7
#define Hole1Points 4
#define Hole2Points 4
#define Hole3Points 3
#define Hole4Points 1
#define Hole5Points 1
/************************************************
 * SFX Names
 *************************************************
 IMPORTANT! These files live on the root of the Galileo's SD card
 and NOT the SD card in the MP3 shield.
 
 Leave the MP3 shield SD slot EMPTY. 

These are all MP3 files. Supposedly OGG files work too
but this isn't tested, plus the FLAC encoder is free. 
Which sound does what is pretty self explanatory. Remember
that the playback is a "blocking" function, which means that
no scoring or display updating can occur while playback is
happening, so keep them short in duration. 
*/
#define WelcomeSound "welcome.mp3"
#define Hole0Sound "cow.mp3"
#define Hole1Sound "piggy.mp3"
#define Hole2Sound "goose.mp3"
#define Hole3Sound "turkey.mp3"
#define Hole4Sound "goat.mp3"
#define Hole5Sound "chicken.mp3"
#define WinSound "youwin.mp3"
#define ConfirmSound "confirm.mp3"
#define CleanupSound "cleanup.mp3"
#define GameADescription "gamea.mp3"
#define GameBDescription "gameb.mp3"

/************************************************
 * Pins and buttons
 *************************************************
These inputs are internally "pulled up" on the CPU
so you trigger them by shorting them to GND (LOW)
All of the COM leads of the switches can share the GND
connection, then each N.O. (normally open) lead will
go to the corresponding input. Analog inputs are being
used in digital mode in order to simplify wiring. That way
all of the score inputs are on one side, and neatly next 
to each other. Remember, there are not actually a lot
of free pins in this project due to the MP3 chip and 
display board. Hooking switches to pins used by the
display or MP3 shield will not end well. Stick to the
analog pins. Game A and B buttons use pin 8 and 9, and
also short to ground to activate them. Again, stick to
these pins unless you have a compelling reason to change
them and you know you won't step on any toes by doing so. 
*/
#define hole0 A0
#define hole1 A1
#define hole2 A2
#define hole3 A3
#define hole4 A4
#define hole5 A5

#define GameAButton 8
#define GameBButton 9


/************************************************
 * Scoerboard stuff
 *************************************************/
#include "HT1632.h" // Score board library

/*
Please pay close attention to properly wiring the scoreboard 
to the Galileo. The datasheets for the LED matrix will identify
the pins and their names, but you will be responsible for hooking
them up to the proper outputs on the Galileo. In addition to these
four pins, a GND and 5V connection to the Galileo will be 
required as well. The Galileo can supply enough power to run
the display, so additional power supply will be required. 
*/

#define DATA 0
#define WR   1
#define CS   2
#define CS2  5
char filename[64];

/************************************************
 * MP3 player declares!
 *************************************************/

#include <SPI.h>
#include <SD.h>

#define CHUNK_SIZE 32
#define PAGE_SIZE 4096

//Create the variables to be used by SdFat Library
File track;

//This is the name of the file on the microSD card you would like to play
//Stick with normal 8.3 nomeclature. All lower-case works well.
char trackName[] = "soundfx1.mp3";
int trackNumber = 1;

char errorMsg[100]; //This is a generic array used for sprintf of error messages

#define TRUE  0
#define FALSE  1

//MP3 Player Shield pin mapping. See the schematic
#define MP3_XCS 7 //Control Chip Select Pin (for accessing SPI Control/Status registers)
#define MP3_XDCS 6 //Data Chip Select / BSYNC Pin
#define MP3_DREQ 3 //Data Request Pin: Player asks for more data
#define MP3_RESET 8 //Reset is active low

//VS10xx SCI Registers
#define SCI_MODE 0x00
#define SCI_STATUS 0x01
#define SCI_BASS 0x02
#define SCI_CLOCKF 0x03
#define SCI_DECODE_TIME 0x04
#define SCI_AUDATA 0x05
#define SCI_WRAM 0x06
#define SCI_WRAMADDR 0x07
#define SCI_HDAT0 0x08
#define SCI_HDAT1 0x09
#define SCI_AIADDR 0x0A
#define SCI_VOL 0x0B
#define SCI_AICTRL0 0x0C
#define SCI_AICTRL1 0x0D
#define SCI_AICTRL2 0x0E
#define SCI_AICTRL3 0x0F

/************************************************
 * Game Variables
 *************************************************/
int Score = 0;
int PointMultiplier = 1;  // Point Multiplier.
long timerStartTime = 0; // This is when the scoring timer starts. 
long previousMillis;
long ElapsedTime = 0;
int gameNumber = 1; // default to game 1 (A)
boolean confirmNewGame = false; // This is set high as a confirmation for a new game to avoid accidental presses
boolean timerRunning = false; // Sets whether the timer is running or not. 
/************************************************
 * More stuff!
 *************************************************/


//Set up the LED matrix
HT1632LEDMatrix matrix = HT1632LEDMatrix(DATA, WR, CS);


void setup() {
  Serial.begin(9600);
  initlaiizeMP3Shield(); // Initialize MP3 chip.... obviously
  matrix.begin(HT1632_COMMON_16NMOS);  
  matrix.fillScreen();
  delay(500);
  // Set up input buttons
  pinMode(hole0,INPUT_PULLUP);
  pinMode(hole1,INPUT_PULLUP);
  pinMode(hole2,INPUT_PULLUP);
  pinMode(hole3,INPUT_PULLUP);
  pinMode(hole4,INPUT_PULLUP);
  pinMode(hole5,INPUT_PULLUP);
  pinMode(GameAButton,INPUT_PULLUP);
  pinMode(GameBButton,INPUT_PULLUP);
  Serial.println("Welcome to Electric Cornhole!");
  // Blink the screen to confirm life.
  matrix.clearScreen(); 
  // draw some text!
  matrix.setTextSize(2);    // size 1 == 8 pixels high
  matrix.setTextColor(1);   // 'lit' LEDs
  delay(1000);
  matrix.clearScreen();  

  NewGameSequence();
  Score = 0;
  timerRunning = false;
} // end of SETUP

void loop() {
  // This is the actual  game here. 

if (timerRunning == true){
  ElapsedTime = millis() - timerStartTime;
}

  switch(pollSwitches()){  // Poll the switches
  case 0:
    Serial.println("Zero!");
    Score = Score + (Hole0Points*PointMultiplier);
    StartTimer(); // Start the timer!
    PointMultiplier = 1;
    playMP3 (Hole0Sound);
    updateScore();
    break;
  case 1:
    Serial.println("One!");
    Score = Score + (Hole1Points*PointMultiplier);
    StartTimer(); // Start the timer!
    PointMultiplier = 1;
    playMP3 (Hole1Sound);
    updateScore();
    break;
  case 2:
    Serial.println("Two!");
    Score = Score + (Hole2Points*PointMultiplier);
    StartTimer(); // Start the timer!
    PointMultiplier = 1;
    playMP3 (Hole2Sound);
    updateScore();
    break;
  case 3:
    Serial.println("Three!");
    Score = Score + (Hole3Points*PointMultiplier);
    StartTimer(); // Start the timer!
    PointMultiplier = 1;
    playMP3 (Hole3Sound);
    updateScore();
    break;
  case 4:
    Serial.println("Four!");
    Score = Score + (Hole4Points*PointMultiplier);
    StartTimer(); // Start the timer!
    PointMultiplier = 2; // Thjs is the GOAT point multiplier. After a goat, you get double points
    playMP3 (Hole4Sound);
    updateScore();
    break;
  case 5:
    Serial.println("Five!");
    Score = Score + (Hole5Points*PointMultiplier);
    StartTimer(); // Start the timer!
    PointMultiplier = 1;
    playMP3 (Hole5Sound);
    updateScore();
    break;
  case 10: // ***************************** Change Games **********************************
    if (confirmNewGame == false){
      Serial.println("Confirm new game?");
      playMP3 (ConfirmSound);
      confirmNewGame = true;
    }
    else{
      SetToGameB();
      confirmNewGame = false;
    }
    break;

  case 20:
    if (confirmNewGame == false){
      Serial.println("Confirm new game?");
      playMP3 (ConfirmSound);
      confirmNewGame = true;
    }
    else{
      SetToGameA();
      confirmNewGame = false;
    }
    break;
  }  // end of SWITCH


  // Now, check to see if we've won!! (or not)
  switch (gameNumber){
  case 1: // Time to score 21 points
    if (Score >= 21){
      Serial.println("Game A Win sequence");
      Score = (ElapsedTime / 1000);
     timerRunning = false; 
      ShowWinSequence();
    }
    break;
  case 2: // Points in 60 seconds
    if (ElapsedTime > 60000){
      timerRunning = false;
      ElapsedTime = 0; // Reset the time.
      Serial.println("Game B Win sequence");
      ShowWinSequence();
    }
    break;
  } // end of SWITCH
} // end of loop



//PlayMP3 pulls 32 byte chunks from the SD card and throws them at the VS1053
//We monitor the DREQ (data request pin). If it goes low then we determine if
//we need new data or not. If yes, pull new from SD card. Then throw the data
//at the VS1053 until it is full.
void playMP3(char* fileName) {
#ifdef SHUTUP // if SHUTUP was defined, have a small delay.
  delay(500);
#endif 

#ifndef SHUTUP // if SHUTUP was defined, just skip the whole playback mess. 
  uint8_t *mp3DataBuffer;
  int offset = 0;
  uint32_t size;

  //Serial.println("Start MP3 decoding");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File track = SD.open(fileName, FILE_READ);
  if (!track)
  {
    //Serial.print(fileName);
    //Serial.println(" not found");
    // don't do anything more:
    return;
  }

  size = track.size();

  mp3DataBuffer = (uint8_t *)malloc(size);
  if (!mp3DataBuffer)
  {
    //Serial.println("Failed to alloc mem for data buffer");
    return;
  }
  //Serial.println("Track open");

  offset = 0;
  while (offset < size)
  {
    int nbytes = size - offset;
    int ret;

    /* Read no more than one page at a time */
    if (nbytes > PAGE_SIZE)
      nbytes = PAGE_SIZE;

    ret = track.read(mp3DataBuffer+offset, nbytes);
    if (ret < 0)
    {    
      //Serial.print("Failed to read file, error: ");
      //Serial.println(ret);
      return;
    }

    offset += ret;
  }

  //Serial.print("Read whole file, size is: ");
  //Serial.println(size);

  /* Start feeding data to the VS1053 */
  offset  = 0;
  digitalWrite(MP3_XDCS, LOW); //Select Data
  while(offset < size && !getCommand()) {
    //Once DREQ is released (high) we now feed 32 bytes of data to the VS1053 from our SD read buffer
    while(!digitalRead(MP3_DREQ));

    SPI.transferBuffer(mp3DataBuffer + offset, NULL, (size - offset) > CHUNK_SIZE ? CHUNK_SIZE : size - offset); // Send SPI bytes
    offset += CHUNK_SIZE;

    //getSynthInput();
  }
  digitalWrite(MP3_XDCS, HIGH); //Deselect Data

  while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating transfer is complete

  digitalWrite(MP3_XDCS, HIGH); //Deselect Data

  track.close(); //Close out this track
  free(mp3DataBuffer);

  //sprintf(errorMsg, "Track %s done!", fileName);
  //Serial.println(errorMsg);
#endif
} // end of PlayMP3


//Write to VS10xx register
//SCI: Data transfers are always 16bit. When a new SCI operation comes in 
//DREQ goes low. We then have to wait for DREQ to go high again.
void Mp3WriteRegister(unsigned char addressbyte, unsigned char highbyte, unsigned char lowbyte){
  while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating IC is available
  digitalWrite(MP3_XCS, LOW); //Select control

  //SCI consists of instruction byte, address byte, and 16-bit data word.
  SPI.transfer(0x02); //Write instruction
  SPI.transfer(addressbyte);
  SPI.transfer(highbyte);
  SPI.transfer(lowbyte);
  while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating command is complete
  digitalWrite(MP3_XCS, HIGH); //Deselect Control
}

//Read the 16-bit value of a VS10xx register
unsigned int Mp3ReadRegister (unsigned char addressbyte){
  while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating IC is available
  digitalWrite(MP3_XCS, LOW); //Select control

  //SCI consists of instruction byte, address byte, and 16-bit data word.
  SPI.transfer(0x03);  //Read instruction
  SPI.transfer(addressbyte);

  char response1 = SPI.transfer(0xFF); //Read the first byte
  while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating command is complete
  char response2 = SPI.transfer(0xFF); //Read the second byte
  while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating command is complete

  digitalWrite(MP3_XCS, HIGH); //Deselect Control

  int resultvalue = response1 << 8;
  resultvalue |= response2;
  return resultvalue;
}

//Set VS10xx Volume Register
void Mp3SetVolume(unsigned char leftchannel, unsigned char rightchannel){
  Mp3WriteRegister(SCI_VOL, leftchannel, rightchannel);
}


char getCommand(){
  return false;//this function is disabled

  if(Serial.available()){
    byte b=Serial.read();
    if(b=='s')
      return true;
    else 
      return false;
  }
}



void initlaiizeMP3Shield(){
  // MP3 stuff...
  pinMode(MP3_DREQ, INPUT);
  pinMode(MP3_XCS, OUTPUT);
  pinMode(MP3_XDCS, OUTPUT);
  pinMode(MP3_RESET, OUTPUT);

  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);

  digitalWrite(MP3_XCS, HIGH); //Deselect Control
  digitalWrite(MP3_XDCS, HIGH); //Deselect Data
  digitalWrite(MP3_RESET, LOW); //Put VS1053 into hardware reset
  // MP3 stuff...
  Serial.println("MP3 Testing");

  if (!SD.begin(0)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("SD card initialized.");

  //From page 12 of datasheet, max SCI reads are CLKI/7. Input clock is 12.288MHz. 
  //Internal clock multiplier is 1.0x after power up. 
  //Therefore, max SPI speed is 1.75MHz. We will use 1MHz to be safe.
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16); //Set SPI bus speed to 1MHz (16MHz / 16 = 1MHz)
  SPI.transfer(0xFF); //Throw a dummy byte at the bus
  //Initialize VS1053 chip 
  delay(10);
  digitalWrite(MP3_RESET, HIGH); //Bring up VS1053
  //delay(10); //We don't need this delay because any register changes will check for a high DREQ

  //Mp3SetVolume(20, 20); //Set initial volume (20 = -10dB) LOUD...ish.  Higher numbers are LOWER volume, with zero being completelty cranked. 
  Mp3SetVolume(0, 0); // LOUD! Maximum warp! These amps go to ELEVEN.

  //Let's check the status of the VS1053
  int MP3Mode = Mp3ReadRegister(SCI_MODE);
  int MP3Status = Mp3ReadRegister(SCI_STATUS);
  int MP3Clock = Mp3ReadRegister(SCI_CLOCKF);

  Serial.print("SCI_Mode (0x4800) = 0x");
  Serial.println(MP3Mode, HEX);

  Serial.print("SCI_Status (0x48) = 0x");
  Serial.println(MP3Status, HEX);

  int vsVersion = (MP3Status >> 4) & 0x000F; //Mask out only the four version bits
  Serial.print("VS Version (VS1053 is 4) = ");
  Serial.println(vsVersion, DEC); //The 1053B should respond with 4. VS1001 = 0, VS1011 = 1, VS1002 = 2, VS1003 = 3

  Serial.print("SCI_ClockF = 0x");
  Serial.println(MP3Clock, HEX);

  //Now that we have the VS1053 up and running, increase the internal clock multiplier and up our SPI rate
  Mp3WriteRegister(SCI_CLOCKF, 0x60, 0x00); //Set multiplier to 3.0x

  //From page 12 of datasheet, max SCI reads are CLKI/7. Input clock is 12.288MHz. 
  //Internal clock multiplier is now 3x.
  //Therefore, max SPI speed is 5MHz. 4MHz will be safe.
  SPI.setClockDivider(SPI_CLOCK_DIV4); //Set SPI bus speed to 4MHz (16MHz / 4 = 4MHz)

  MP3Clock = Mp3ReadRegister(SCI_CLOCKF);
  Serial.print("SCI_ClockF = 0x");
  Serial.println(MP3Clock, HEX);

  //MP3 IC setup complete
} // end initlaiizeMP3Shield

void updateScore(){
  if (Score < 99) { // Properly display scores over 100 points. It could happen. 
    matrix.setTextSize(2);    // size 2 == 16 pixels high
  }
  else {
    matrix.setTextSize(1);  // size 1 == 8 pixels high
  }

  matrix.setTextColor(1);   // 'lit' LEDs
  matrix.clearScreen();  
  matrix.setCursor(1, 1);   // start at top left, with one pixel of spacing
  matrix.print(Score);
  Serial.println("Elapsed Time is " + String(ElapsedTime/1000));
  Serial.println("Score is " + String(Score));
  matrix.writeScreen();
  confirmNewGame = false; // If we just scored a point, then we're not trying to confirm a new game now, are we?
} // end UpdateScore

void ShowWinSequence(){
  ElapsedTime = 0;
  timerRunning = false;
  matrix.blink(true); // Now display "You Win!!"
  matrix.clearScreen();  
  matrix.setTextSize(1);    // size 1 == 8 pixels high
  matrix.setTextColor(1);   // 'lit' LEDs
  matrix.setCursor(3, 0);   // start at top left, with one pixel of spacing
  matrix.print("You");
  matrix.setCursor(2, 8);   // start at top left, with one pixel of spacing
  matrix.print("Win!");
  matrix.writeScreen();
  Serial.println("You Win!");
  delay(2000);
  updateScore(); // Show the winning score
  playMP3(WinSound);
  // whew!
  delay(5000);
  playMP3(CleanupSound); // Sound to let people know to clean up their mess
  delay(10000);
  matrix.blink(false);
  Score = 0; // Reset the score.
  NewGameSequence();
}

void NewGameSequence(){
  timerRunning = false; // Stop the timer!
  ElapsedTime = 0;
  Score = 0; // Reset the score.
  matrix.clearScreen();  
  matrix.setTextSize(1);    // size 1 == 8 pixels high
  matrix.setTextColor(1);   // 'lit' LEDs
  matrix.setCursor(0, 0);   // start at top left, with one pixel of spacing
  matrix.print("Play");
  matrix.setCursor(2, 8);   // start at top left, with one pixel of spacing
  matrix.print("Now!");
  matrix.writeScreen();
  Serial.println("Play Now!");
  playMP3(WelcomeSound);
}

void SetToGameA(){
  timerRunning = false; // Stop the timer!
  ElapsedTime = 0;
  matrix.clearScreen();  
  matrix.setTextSize(1);    // size 1 == 8 pixels high
  matrix.setTextColor(1);   // 'lit' LEDs
  matrix.setCursor(0, 0);   // start at top left, with one pixel of spacing
  matrix.print("Game");
  matrix.setCursor(4, 8);   // start at top left, with one pixel of spacing
  matrix.print(" A");
  matrix.writeScreen();
  Serial.println("Game A Selected");
  gameNumber = 1; // This actually sets the game.
  playMP3(GameADescription);
  Score = 0; // Reset the score.
}

void SetToGameB(){
  timerRunning = false; // Stop the timer!
  ElapsedTime = 0;
  matrix.clearScreen();  
  matrix.setTextSize(1);    // size 1 == 8 pixels high
  matrix.setTextColor(1);   // 'lit' LEDs
  matrix.setCursor(0, 0);   // start at top left, with one pixel of spacing
  matrix.print("Game");
  matrix.setCursor(4, 8);   // start at top left, with one pixel of spacing
  matrix.print(" B");
  matrix.writeScreen();
  Serial.println("Game B Selected");
  gameNumber = 2; // This actually sets the game.
  playMP3(GameBDescription);
  Score = 0; // Reset the score.
}

int pollSwitches(){
  int HoleStrike = 99;  // Poll the holes! 99 means NO holes have been struck.
  if (digitalRead(hole0)==!HIGH){
    HoleStrike = 0;
  }
  if (digitalRead(hole1)==!HIGH){
    HoleStrike = 1;
  }
  if (digitalRead(hole2)==!HIGH){
    HoleStrike = 2;
  }
  if (digitalRead(hole3)==!HIGH){
    HoleStrike = 3;
  }
  if (digitalRead(hole4)==!HIGH){
    HoleStrike = 4;
  }
  if (digitalRead(hole5)==!HIGH){
    HoleStrike = 5;
  }
  if (digitalRead(GameAButton)==!HIGH){
    HoleStrike = 10;
  }// In this case, we select a new game. 
  if (digitalRead(GameBButton)==!HIGH){
    HoleStrike = 20;
  }// In this case, we select a new game. 
  return HoleStrike;
}

void StartTimer(){ // Start the timer unless it's already been started.
  if (timerRunning == false) {
  Serial.println("Timer STARTED!!");
  ElapsedTime = 0;
  timerStartTime = millis();
  timerRunning = true;
  }
}

