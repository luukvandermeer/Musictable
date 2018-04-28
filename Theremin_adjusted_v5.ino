/*******************************************************************************
*******************************************************************************/
//Definieren van de in-/outputs
#define trigPin 7
#define echoPin 6
#define led 12
#define led2 11
#define led3 5
#define led4 9
#define led5 8

//variabelen toegevoegd om te debuggen
int distance;

// compiler error handling
#include "Compiler_Errors.h"

// serial rate
#define baudRate 57600

// include the relevant libraries
#include <MPR121.h>
#include <Wire.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(12, 10); //Soft TX on 10, we don't use RX in this code

//Touch Board pin setup
byte pin_pitch = 0;       // pin to control pitch
//Add other pitch to controle other instrument 



// Theremin setup
byte min_note = 40;  // lowest midi pitch
byte max_note = 100;  // highest midi pitch Wellicht aanpassen naar een prettigere hoogte
//byte min_volume = 0;  // lowest volume
//byte max_volume = 127;  // highest volume (max is 127)
byte direction_pitch = 0;  // 0 = highest pitch with touch, 1 = lowest pitch with touch
int nonlinear_pitch = -10;  // fscale 'curve' param, 0=linear, -10 = max change at large distance
//byte direction_volume = 0;  // 0 = loudest volume with touch, 1 = lowest volume with touch
//int nonlinear_volume = -10; // fscale 'curve' param, 0=linear, -10 = max change at large distance

// midi instrument setup
byte instrument = 80 - 1;  // starting instrument (-1 b/c I think instrument list is off by 1)
byte min_instrument = 0;  // 
byte max_instrument = 127;  // max is 127

// initialization stuff for proximity
int level_pitch = 0;
int level_pitch_old = 0;
int level_volume = 80; //volume aangepast zodat deze op standaard op 100 staat
int level_volume_old = 80; //volume aangepast zodat deze op standaard op 100 staat
byte note=0;
byte note_old=0;
byte volume = 80; //volume aangepast zodat deze standaard op 100 staat
byte volume_old = 80; //volume aangepast zodat standaard op 100 staat
int min_level_pitch = 1000; // dummy start values - updated during running
int max_level_pitch = 0; // dummy start values - updated during running
int min_level_volume = 1000; // dummy start values - updated during running
int max_level_volume = 0; // dummy start values - updated during running

//VS1053 setup
//byte note = 0; //The MIDI note value to be played
byte resetMIDI = 1; //Tied to VS1053 Reset line
byte ledPin = 13; //MIDI traffic inidicator
byte velocity = 60;  // midi note velocity for turning on and off


/* Code MIDI DRUMPAD */
// Touch Board Setup variables
#define firstPin 1
#define lastPin 11

// VS1053 setup
byte note_1 = 0; // The MIDI note value to be played
//byte resetMIDI = 8; // Tied to VS1053 Reset line
//byte ledPin = 13; // MIDI traffic inidicator
int  instrument_1 = 0;

// key definitions
const byte allNotes[] = {36, 36, 36, 36, 36, 65, 66, 67, 68, 69, 70};



void setup(){


//Code voor de theremin
  Serial.begin(baudRate);


  
  // uncomment the line below if you want to see Serial data from the start
  //while (!Serial);
  
  //Setup soft serial for MIDI control
  mySerial.begin(31250);
  Wire.begin();
   
  // 0x5C is the MPR121 I2C address on the Bare Touch Board
  if(!MPR121.begin(0x5C)){ 
    Serial.println("error setting up MPR121");  
    switch(MPR121.getError()){
      case NO_ERROR:
        Serial.println("no error");
        break;  
      case ADDRESS_UNKNOWN:
        Serial.println("incorrect address");
        break;
      case READBACK_FAIL:
        Serial.println("readback failure");
        break;
      case OVERCURRENT_FLAG:
        Serial.println("overcurrent on REXT pin");
        break;      
      case OUT_OF_RANGE:
        Serial.println("electrode out of range");
        break;
      case NOT_INITED:
        Serial.println("not initialised");
        break;
      default:
        Serial.println("unknown error");
        break;      
    }
    while(1);
  }
  
  // pin 4 is the MPR121 interrupt on the Bare Touch Board
  //MPR121.setInterruptPin(4);
  // initial data update
  MPR121.updateTouchData();

  //Reset the VS1053
  pinMode(resetMIDI, OUTPUT);
  digitalWrite(resetMIDI, LOW);
  delay(100);
  digitalWrite(resetMIDI, HIGH);
  delay(100);
  
  // initialise MIDI
  setupMidi();

// Code voor de ultrasoon sensor
  Serial.begin (9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(led, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  pinMode(led5, OUTPUT);
  
}

void loop(){
/* Code voor de ultrasoon sensor */
long duration, distance;
  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;

  if (distance <= 30) {
    digitalWrite(led, HIGH);
}
  else {
    digitalWrite(led, LOW);
  }
  if (distance < 25) {
      digitalWrite(led2, HIGH);
}
  else {
      digitalWrite(led2, LOW);
  }
  if (distance < 20) {
      digitalWrite(led3, HIGH);
} 
  else {
    digitalWrite(led3, LOW);
  }
  if (distance < 15) {
    digitalWrite(led4, HIGH);
}
  else {
    digitalWrite(led4,LOW);
  }
  if (distance < 10) {
    digitalWrite(led5, HIGH);
}
  else {
    digitalWrite(led5,LOW);
  }

//Code verwerken in Serial Monitor for debugging
// Prints the distance voor the Serial Monitor
       Serial.print("Distance: ");
       Serial.println(distance);



  
/* Code voor de theremin  */
   MPR121.updateAll();
   // pitch
   level_pitch_old = level_pitch;
   level_pitch = MPR121.getFilteredData(pin_pitch);     
   if (level_pitch != level_pitch_old){
     // dynamically setup level max and mins
     if (level_pitch > max_level_pitch){
       max_level_pitch = level_pitch;
     }
     if (level_pitch < min_level_pitch){
       min_level_pitch = level_pitch;
     }   
     // turn off notes if level rises near baseline
     if (fscale(min_level_pitch,max_level_pitch,0,1,level_pitch,nonlinear_pitch) >= 0.95) {
       noteOff(0, note_old, velocity);
       noteOff(0, note, velocity);  
       note_old = note;
       Serial.println("All notes off");
     }
     // set note
     else {
       if (direction_pitch == 0){
         note = fscale(min_level_pitch,max_level_pitch,max_note,min_note,level_pitch,nonlinear_pitch);
       }
       else if (direction_pitch == 1){
         note = fscale(min_level_pitch,max_level_pitch,min_note,max_note,level_pitch,nonlinear_pitch);
       }
       if (note != note_old){
         noteOn(0, note, velocity);  // turn on new note
         noteOff(0, note_old, velocity);  // turn off old note
         note_old = note;
         Serial.print("Note on: ");
         Serial.print(note);
         Serial.print(", Note off ");
         Serial.println(note_old);
       }
     }
   }
   
   // volume
   level_volume_old = level_volume;

   // loop delay
   delay(50);

/* MIDI DRUMPAD*/

 for(int i=firstPin; i<=lastPin; i++){
       
       // you can choose how to map the notes here - try replacing "whiteNotes" with
       // "allNotes" or "drumNotes"
       note_1 = allNotes[lastPin-i];
       if(MPR121.isNewTouch(i)){
         //Note on channel 1 (0x90), some note value (note), 75% velocity (0x60):
         noteOn(0, note_1, 0x60);
         Serial.print("Note ");
         Serial.print(note_1);
         Serial.println(" on");
       } else if(MPR121.isNewRelease(i)) {   
         // Turn off the note with a given off/release velocity
         noteOff(0, note_1, 0x60);  
         Serial.print("Note ");
         Serial.print(note_1);
         Serial.println(" off");       
       }
     }

   
}    


// functions below are little helpers based on using the SoftwareSerial
// as a MIDI stream input to the VS1053 - all based on stuff from Nathan Seidle

//Send a MIDI note-on message.  Like pressing a piano key
//channel ranges from 0-15
void noteOn(byte channel, byte note, byte attack_velocity) {
  talkMIDI( (0x90 | channel), note, attack_velocity);
}

//Send a MIDI note-off message.  Like releasing a piano key
void noteOff(byte channel, byte note, byte release_velocity) {
  talkMIDI( (0x80 | channel), note, release_velocity);
}

//Plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that data values are less than 127
void talkMIDI(byte cmd, byte data1, byte data2) {
  digitalWrite(ledPin, HIGH);
  mySerial.write(cmd);
  mySerial.write(data1);

  //Some commands only have one data byte. All cmds less than 0xBn have 2 data bytes 
  //(sort of: http://253.ccarh.org/handout/midiprotocol/)
  if( (cmd & 0xF0) <= 0xB0)
    mySerial.write(data2);

  digitalWrite(ledPin, LOW);
}

//SETTING UP THE INSTRUMENT 


void setupMidi(){
  
  //Volume
  talkMIDI(0xB0, 0x07, volume); //0xB0 is channel message, set channel volume to near max (127)
  talkMIDI(0xC0, 53, 0);  
}


float fscale( float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve){

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin){
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd;
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float


  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {  
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }

  return rangedValue;
}
