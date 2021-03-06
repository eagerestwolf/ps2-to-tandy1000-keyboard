// Tandy 1000/1000A/1000SX keyboard converter by Adrian Black
// Version 0.1 10-AUG-2020
//
// Converts from AT/PS2 keyboard _TO_ the Tandy 1000 so you can use a regular keyboard with a Tandy.
// Power provided by the Tandy so you can create a cable with the arduino in the middle. 
// A standard 5 pin DIN extension cable is all you need to make this work.
//
// Warning it's possible to insert the DIN 5 into the Tandy turned one or more pins!!
// Make sure you have the center notch on the DIN-5 facing straight up -- or use the proper DIN cable to prevent this issue!
//
// Original timing diagrams used from: http://www.kbdbabel.org/
// Updating timing using Rigol DS1054Z and a real Tandy 1000 keyboard and Tandy 1000 Technical Reference Manual Page 105
// Verified scancodes using scope but they are all listed in the Tandy 1000 (original) Technical manual (page 106-107)
//
// Pinout: http://www.kbdbabel.org/conn/index.html
//
// Idea and code based on this project: https://github.com/kesrut/pcxtkbd
// Using some code snippets from here: https://github.com/techpaul/PS2KeyAdvanced
// Uses this library to read raw codes from the PS2 keyboard: https://github.com/techpaul/PS2KeyRaw/find/master
// 
// Todo: 
// Handle the Pause key
// Handle the print screen key
// Get the CAPS/NUN/SCROLL LED working 
/*
Tandy 1000 8-pin DIN (using DIN 5 is fine but watch alignment!)

1 - Data
2 - Busy
3 - Ground
4 - Clock
5 - +5v
6 - Reset
7 - NC
8 - NC

IBM - Tandy
2 - 1 Data
4 - 3 Ground
5 - 5 +5v
1 - 4 Clock
*/

#include <PS2KeyRaw.h> //for library, see github repo above

#define DATAPIN 4 // PS2 data pin
#define IRQPIN  3 // PS2 clock pin

//uint8_t line = 0; delete this?

boolean capson = false; // tracking of caps lock status
boolean numon = false;  // tracking of num lock status
boolean shifton = false; // tracking of shift key for \ and ` key

unsigned char translationTable [256];

#define xt_clk 2 // Tandy 1000 CLOCK PIN
#define xt_data 5 // Tandy 1000 DATA PIN

//
// PS2 raw keycodes
/* Single Byte Key Codes */
#define PS2_KEY_NUM      0x77
#define PS2_KEY_SCROLL   0x7E
#define PS2_KEY_CAPS     0x58
#define PS2_KEY_L_SHIFT  0x12
#define PS2_KEY_R_SHIFT  0x59
/* This is Left CTRL and ALT but Right version is in E0 with same code */
#define PS2_KEY_L_CTRL     0X14
#define PS2_KEY_L_ALT      0x11
#define PS2_KEY_ESC      0x76
#define PS2_KEY_BS       0x66
#define PS2_KEY_TAB      0x0D
#define PS2_KEY_ENTER    0x5A
#define PS2_KEY_SPACE    0x29
#define PS2_KEY_KP0      0x70
#define PS2_KEY_KP1      0x69
#define PS2_KEY_KP2      0x72
#define PS2_KEY_KP3      0x7A
#define PS2_KEY_KP4      0x6B
#define PS2_KEY_KP5      0x73
#define PS2_KEY_KP6      0x74
#define PS2_KEY_KP7      0x6C
#define PS2_KEY_KP8      0x75
#define PS2_KEY_KP9      0x7D
#define PS2_KEY_KP_DOT   0x71
#define PS2_KEY_KP_PLUS  0x79
#define PS2_KEY_KP_MINUS 0x7B
#define PS2_KEY_KP_TIMES 0x7C
#define PS2_KEY_KP_EQUAL 0x0F // Some keyboards have an '=' on right keypad
#define PS2_KEY_0        0X45
#define PS2_KEY_1        0X16
#define PS2_KEY_2        0X1E
#define PS2_KEY_3        0X26
#define PS2_KEY_4        0X25
#define PS2_KEY_5        0X2E
#define PS2_KEY_6        0X36
#define PS2_KEY_7        0X3D
#define PS2_KEY_8        0X3E
#define PS2_KEY_9        0X46
#define PS2_KEY_APOS     0X52
#define PS2_KEY_COMMA    0X41
#define PS2_KEY_MINUS    0X4E
#define PS2_KEY_DOT      0X49
#define PS2_KEY_DIV      0X4A
#define PS2_KEY_SINGLE   0X0E // key shared with tilde needs special handling
#define PS2_KEY_A        0X1C
#define PS2_KEY_B        0X32
#define PS2_KEY_C        0X21
#define PS2_KEY_D        0X23
#define PS2_KEY_E        0X24
#define PS2_KEY_F        0X2B
#define PS2_KEY_G        0X34
#define PS2_KEY_H        0X33
#define PS2_KEY_I        0X43
#define PS2_KEY_J        0X3B
#define PS2_KEY_K        0X42
#define PS2_KEY_L        0X4B
#define PS2_KEY_M        0X3A
#define PS2_KEY_N        0X31
#define PS2_KEY_O        0X44
#define PS2_KEY_P        0X4D
#define PS2_KEY_Q        0X15
#define PS2_KEY_R        0X2D
#define PS2_KEY_S        0X1B
#define PS2_KEY_T        0X2C
#define PS2_KEY_U        0X3C
#define PS2_KEY_V        0X2A
#define PS2_KEY_W        0X1D
#define PS2_KEY_X        0X22
#define PS2_KEY_Y        0X35
#define PS2_KEY_Z        0X1A
#define PS2_KEY_SEMI     0X4C
#define PS2_KEY_BACK     0X5D //backslash -- needs special handling
#define PS2_KEY_OPEN_SQ  0X54
#define PS2_KEY_CLOSE_SQ 0X5B
#define PS2_KEY_EQUAL    0X55
#define PS2_KEY_F1       0X05
#define PS2_KEY_F2       0X06
#define PS2_KEY_F3       0X04
#define PS2_KEY_F4       0X0C
#define PS2_KEY_F5       0X03
#define PS2_KEY_F6       0X0B
#define PS2_KEY_F7       0X83
#define PS2_KEY_F8       0X0A
#define PS2_KEY_F9       0X01
#define PS2_KEY_F10      0X09
#define PS2_KEY_F11      0X78
#define PS2_KEY_F12      0X07
#define PS2_KEY_KP_COMMA 0X6D


// Extended key codes -- scan code is received with a preceeding E0 then the code below
// All of the keys below need special handling

#define PS2_KEY_IGNORE   0x12
#define PS2_KEY_PRTSCR   0x7C
#define PS2_KEY_L_GUI    0x1F // windows key
#define PS2_KEY_R_GUI    0x27
#define PS2_KEY_MENU     0x2F
#define PS2_KEY_BREAK    0x7E // Break is CTRL + PAUSE generated inside keyboard */
#define PS2_KEY_HOME     0x6C
#define PS2_KEY_END      0x69
#define PS2_KEY_PGUP     0x7D
#define PS2_KEY_PGDN     0x7A
//#define PS2_KEY_L_ARROW  0x6B
//#define PS2_KEY_R_ARROW  0x74
//#define PS2_KEY_UP_ARROW 0x75
//#define PS2_KEY_DN_ARROW 0x72
#define PS2_KEY_INSERT   0x70
#define PS2_KEY_DELETE   0x71
#define PS2_KEY_KP_ENTER 0x5A
#define PS2_KEY_KP_DIV   0x4A



PS2KeyRaw keyboard;

byte i = 0 ;
unsigned char ascan;


void setup() 
{
  delay(1000);
  setupTable () ; 
  keyboard.begin( DATAPIN, IRQPIN );

  pinMode(xt_clk, OUTPUT) ; 
  pinMode(xt_data, OUTPUT) ; 
  digitalWrite(xt_clk, LOW) ; 
  digitalWrite(xt_data, HIGH) ; 
}

void loop() 
{

if(keyboard.available()) //check if byte is waiting to be read out of buffer
  {
  int c = keyboard.read(); //read the keyboard buffer byte into variable c
   
  if( c < 0x80)  // normal PS2 key combos without any special or special commands
  {
     switch (c)
     {
      case PS2_KEY_CAPS: { //capslock -- we have to do special handling and state tracking
        if (capson)
          { _write(0x3a | 0x80); capson = false; } //0x3a | 0x80 caps lock is already on, so turn it off by sending a release command
         else
          { _write(0x3a); capson = true; }  //0x3a caps lock isn't on, so send a caplock key press
        break;
        }
      
       case PS2_KEY_NUM: { //numlock -- we have to do special handling and state tracking
        if (numon)
          { _write(0x45 | 0x80); numon = false; } //0x45 | 0x80
         else
          { _write(0x45); numon = true; }  //0x45
        break;
        }


      case PS2_KEY_KP_TIMES: _write(0x36); _write(0x09);break; //send a shift 8 for *

      case PS2_KEY_KP_PLUS: _write(0x36); _write(0x0d); break;

      case PS2_KEY_BACK: {   
        if (shifton) {
         if (numon)
          { _write(0x4B); _write(0x4B | 0x80); } //numlock is on, so turn it off, press and release delete and turn it back on
         else
          { _write(0x45); _write(0x4B); _write(0x4B | 0x80); _write(0x45|0x80);} //numlock is already off, so just push and release the delete key
        break;
        } else {
         if (numon)
          { _write(0x45 | 0x80); _write(0x47); _write(0x47 | 0x80); _write(0x45); } //numlock is on, so turn it off, press and release delete and turn it back on
         else
          { _write(0x47); _write(0x47 | 0x80); } //numlock is already off, so just push and release the delete key
        }
        break; 
       }


      case PS2_KEY_SINGLE: {  
        if (shifton) {
         if (numon)
          { _write(0x48); _write(0x48 | 0x80); } //numlock is on, so turn it off, press and release delete and turn it back on
         else
          { _write(0x45); _write(0x48); _write(0x48 | 0x80); _write(0x45|0x80); } //numlock is already off, so just push and release the delete key
        } else {
        if (numon)
          { _write(0x45 | 0x80); _write(0x50); _write(0x50 | 0x80); _write(0x45); } //numlock is on, so turn it off, press and release delete and turn it back on
         else
          { _write(0x50); _write(0x50 | 0x80); } //numlock is already off, so just push and release the delete key
        }
        break;
        }

      case PS2_KEY_L_SHIFT: _write(0x2A); shifton = true; break;
      case PS2_KEY_R_SHIFT: _write(0x36); shifton = true; break;      

      default: //no special hanndling, use the translation table to send the right key
      {
        ascan = translationTable[c];
        _write(ascan) ;
      }
     }
     
  }

  if(c == 0xE0) //special keys -- like ones that are replicated and have E0 first so you can tell them apart
    {
    delay(10);
    int cr = keyboard.read(); // first byte was E0 for special, so read the next byte which will be the key pushed
    if(cr == 0xF0)  // oh actually, a special key was released, so we need to handle that.
      {
      delay(10); //wait a little for the MCU to read the byte from the keyboard
      int csr = keyboard.read(); // let's read the next byte from the buffer

      switch (csr) //special handling for special keys released
      {
        case PS2_KEY_KP4: _write(0x2b | 0x80); break;  //left arrow
        case PS2_KEY_KP6: _write(0x4e | 0x80); break;  //right arrow
        case PS2_KEY_KP8: _write(0x29 | 0x80); break;  //up arrow
        case PS2_KEY_KP2: _write(0x4a | 0x80); break;  //down arrow

        case PS2_KEY_DIV: _write(0x35 | 0x80); break; 

        case PS2_KEY_L_CTRL: _write(0x1d | 0x80); break;
        case PS2_KEY_L_ALT: _write(0x38 | 0x80); break;

        case PS2_KEY_HOME: _write(0x58 | 0x80); break;  //home key
        case PS2_KEY_ENTER: _write(0x57 | 0x80); break; //keypad enter


        case PS2_KEY_DELETE: break; // yeah do nothing as this is handled below as a spcial key 
        case PS2_KEY_END: break;
        case PS2_KEY_INSERT: break;
        case PS2_KEY_PGUP: break;
        case PS2_KEY_PGDN: break;
          
        default:
        {
          ascan = translationTable[cr];
          _write(ascan) ;
        }
      }
      
      } else {
    switch (cr) //and handle the special keys first pushed
     {
      case PS2_KEY_KP4: _write(0x2b); break;  //left arrow
      case PS2_KEY_KP6: _write(0x4e); break;  //right arrow
      case PS2_KEY_KP8: _write(0x29); break;  //up arrow
      case PS2_KEY_KP2: _write(0x4a); break;  //down arrow

      case PS2_KEY_L_CTRL: _write(0x1d); break;
      case PS2_KEY_L_ALT: _write(0x38); break;

      case PS2_KEY_DIV: _write(0x35); break; 

      case PS2_KEY_HOME: _write(0x58); break;  //home key
      case PS2_KEY_ENTER: _write(0x57); break; //keypad enter

      case PS2_KEY_END: {  
         if (numon)
          { _write(0x45 | 0x80); _write(0x4F); _write(0x4F | 0x80); _write(0x45); } //numlock is on, so turn it off, press and release delete and turn it back on
         else
          { _write(0x4F); _write(0x4F | 0x80); } //numlock is already off, so just push and release the delete key
        break;
        }


        case PS2_KEY_DELETE: {  // the delete key is on the numpad on the tandy, and only works if numlock is off
         if (numon)
          { _write(0x45 | 0x80); _write(0x53); _write(0x53 | 0x80); _write(0x45); } //numlock is on, so turn it off, press and release delete and turn it back on
         else
          { _write(0x53); _write(0x53 | 0x80); } //numlock is already off, so just push and release the delete key
        break;
        }


        case PS2_KEY_INSERT: { 
         if (numon)
          { _write(0x45 | 0x80); _write(0x55); _write(0x55 | 0x80); _write(0x45); } //numlock is on, so turn it off, press and release delete and turn it back on
         else
          { _write(0x55); _write(0x55 | 0x80); } //numlock is already off, so just push and release the delete key
        break;
        }

        case PS2_KEY_PGUP: {  
         if (numon)
          { _write(0x45 | 0x80); _write(0x49); _write(0x49 | 0x80); _write(0x45); } //numlock is on, so turn it off, press and release delete and turn it back on
         else
          { _write(0x49); _write(0x49 | 0x80); } //numlock is already off, so just push and release the delete key
        break;
        }

        case PS2_KEY_PGDN: {   
         if (numon)
          { _write(0x45 | 0x80); _write(0x51); _write(0x51 | 0x80); _write(0x45); } //numlock is on, so turn it off, press and release delete and turn it back on
         else
          { _write(0x51); _write(0x51 | 0x80); } //numlock is already off, so just push and release the delete key
        break;
        }

      default: //and now all the other special keys.... this section probably shoudl be removed
      {
        ascan = translationTable[cr];
        _write(ascan) ;
      }
      }
     }
    }
  
  if(c == 0xF0) // byte that is sent when a key is released. 
    {
    delay(10);
    int cr = keyboard.read();
    switch (cr)
    {
      case PS2_KEY_CAPS: break; //do nothing on release capslock because it's a special case
      case PS2_KEY_NUM: break; //do nothing on release num lock becauase it's a special case
      case PS2_KEY_KP_TIMES: _write(0x09 | 0x80); _write(0x36 | 0x80); break;
      case PS2_KEY_KP_PLUS: _write(0x0d|0x80); _write(0x36|0x80); break;
      case PS2_KEY_BACK:  break; //do nothing on release capslock because it's a special case
      case PS2_KEY_SINGLE:    break; //do nothing on release capslock because it's a special case   

      case PS2_KEY_L_SHIFT: _write(0x2A | 0x80); shifton = false; break;
      case PS2_KEY_R_SHIFT: _write(0x36 | 0x80); shifton = false; break;     
              
      default:
      { // this is the "break" signal -- when you let go of a key -- so beed to send the scancode to the computer OR'ed with 0x80 which tells it you let go of the key
        ascan = translationTable[cr];
      _write(ascan | 0x80) ;
      }
    }
  }
 }
}


void setupTable () {  // translate from PS2 keycode to Tandy 1000
  for (int i=0;i<256;i++) {
    translationTable[i]=0x39;  // initialize the whole table so defaults anything not mapped to space
  }
  //left side if the PS2 scan-code and right side is what to send to the Tandy 1000
  translationTable[PS2_KEY_0]=0x0B;
  translationTable[PS2_KEY_1]=0x02;
  translationTable[PS2_KEY_2]=0x03;
  translationTable[PS2_KEY_3]=0x04;
  translationTable[PS2_KEY_4]=0x05;
  translationTable[PS2_KEY_5]=0x06;
  translationTable[PS2_KEY_6]=0x07;
  translationTable[PS2_KEY_7]=0x08;
  translationTable[PS2_KEY_8]=0x09;
  translationTable[PS2_KEY_9]=0x0A;
  translationTable[PS2_KEY_APOS]=0x28; 
  translationTable[PS2_KEY_A]=0x1E;
  translationTable[PS2_KEY_BACK] =0x47 ;
  translationTable[PS2_KEY_BREAK]=0x54;
  translationTable[PS2_KEY_BS]=0x0E;
  translationTable[PS2_KEY_B]=0x30;
  translationTable[PS2_KEY_CLOSE_SQ]=0x1B;
  translationTable[PS2_KEY_COMMA]=0x33;
  translationTable[PS2_KEY_C]=0x2E;
//  translationTable[PS2_KEY_DELETE]=0x53; need special handling
  translationTable[PS2_KEY_DIV]=0x35;
//  translationTable[PS2_KEY_DN_ARROW]=0x4A; need special handling
  translationTable[PS2_KEY_DOT]=0x34;
  translationTable[PS2_KEY_D]=0x20;
//  translationTable[PS2_KEY_END]=0x4F; need special handling
  translationTable[PS2_KEY_ENTER]=0x1C;
  translationTable[PS2_KEY_EQUAL]=0x0D;  
  translationTable[PS2_KEY_ESC]=0x01;
  translationTable[PS2_KEY_E]=0x12;
  translationTable[PS2_KEY_F10]=0x44;
  translationTable[PS2_KEY_F11]=0x59;
  translationTable[PS2_KEY_F12]=0x5A;
  translationTable[PS2_KEY_F1]=0x3B;
  translationTable[PS2_KEY_F2]=0x3C;
  translationTable[PS2_KEY_F3]=0x3D;
  translationTable[PS2_KEY_F4]=0x3E;
  translationTable[PS2_KEY_F5]=0x3F;
  translationTable[PS2_KEY_F6]=0x40;
  translationTable[PS2_KEY_F7]=0x41;
  translationTable[PS2_KEY_F8]=0x42;
  translationTable[PS2_KEY_F9]=0x43;
  translationTable[PS2_KEY_F]=0x21;
  translationTable[PS2_KEY_G]=0x22;
//  translationTable[PS2_KEY_HOME]=0x58; //special handling required
  translationTable[PS2_KEY_H]=0x23;
//  translationTable[PS2_KEY_INSERT]=0x52; // need special handling
  translationTable[PS2_KEY_I]=0x17;
  translationTable[PS2_KEY_J]=0x24;
  translationTable[PS2_KEY_KP0]=0x52;
  translationTable[PS2_KEY_KP1]=0x4F;
  translationTable[PS2_KEY_KP2]=0x50;
  translationTable[PS2_KEY_KP3]=0x51;
  translationTable[PS2_KEY_KP4]=0x4B;
  translationTable[PS2_KEY_KP5]=0x4C;
  translationTable[PS2_KEY_KP6]=0x4D;
  translationTable[PS2_KEY_KP7]=0x47; 
  translationTable[PS2_KEY_KP8]=0x48; 
  translationTable[PS2_KEY_KP9]=0x49;
  translationTable[PS2_KEY_KP_DIV]=0x35; 
  translationTable[PS2_KEY_KP_DOT]=0x56;
  translationTable[PS2_KEY_KP_ENTER]=0x57; 
  translationTable[PS2_KEY_KP_MINUS]=0x0c;
//  translationTable[PS2_KEY_KP_PLUS]=0x4E; //need special handling
//  translationTable[PS2_KEY_KP_TIMES]=0x37;  //need special handling
  translationTable[PS2_KEY_K]=0x25;
  translationTable[PS2_KEY_L]=0x26;
  translationTable[PS2_KEY_L_ALT] = 0x38; 
//  translationTable[PS2_KEY_L_ARROW]=0x28; //need special handling
  translationTable[PS2_KEY_L_CTRL]=0x1D;
  translationTable[PS2_KEY_L_SHIFT]=0x2A;
  translationTable[PS2_KEY_MINUS]=0x0C;
  translationTable[PS2_KEY_M]=0x32;
  translationTable[PS2_KEY_N]=0x31;
  translationTable[PS2_KEY_OPEN_SQ]=0x1A;
  translationTable[PS2_KEY_O]=0x18;
//  translationTable[PS2_KEY_PAUSE]=0x46; special key -- PS2 sends a bunch of junk. Using Scroll Lock instead
//  translationTable[PS2_KEY_PGDN]=0x51; //needs special handling
//  translationTable[PS2_KEY_PGUP]=0x49; //need special handling
  translationTable[PS2_KEY_PRTSCR]=0x37;
  translationTable[PS2_KEY_P]=0x19;
  translationTable[PS2_KEY_Q]=0x10;
  translationTable[PS2_KEY_R]=0x13;
//  translationTable[PS2_KEY_R_ALT]=0x38; special key
//  translationTable[PS2_KEY_R_ARROW]=0x4E; //needs special handling
//  translationTable[PS2_KEY_R_CTRL]=0x12; special key
  translationTable[PS2_KEY_R_SHIFT]=0x36; 
  translationTable[PS2_KEY_SCROLL]=0x46;
  translationTable[PS2_KEY_SEMI]=0x27;
  translationTable[PS2_KEY_SINGLE]=0x50 ;
  translationTable[PS2_KEY_SPACE]=0x39;
  translationTable[PS2_KEY_S]=0x1F;
  translationTable[PS2_KEY_TAB]=0x0F;
  translationTable[PS2_KEY_T]=0x14;
//  translationTable[PS2_KEY_UP_ARROW]=0x29; //needs special handling
  translationTable[PS2_KEY_U]=0x16;
  translationTable[PS2_KEY_V]=0x2F;
  translationTable[PS2_KEY_W]=0x11;
  translationTable[PS2_KEY_X]=0x2D;
  translationTable[PS2_KEY_Y]=0x15;
  translationTable[PS2_KEY_Z]=0x2c;
}


void _write(unsigned char value) //routine that writes to the data and clock lines to the Tandy
{
  while (digitalRead(xt_clk) != LOW) ;
   unsigned char bits[8] ;
   byte p = 0 ; 
   byte j = 0 ;
   for (j=0 ; j < 8; j++)
   {
     if (value & 1) bits[j] = 1 ;
     else bits[j] = 0 ; 
     value = value >> 1 ; 
   }

   byte i = 0 ; 
   for (i=0; i < 8; i++)
   {
    digitalWrite(xt_data, bits[p]) ;
    delayMicroseconds(5) ;
    digitalWrite(xt_clk, HIGH) ;
    delayMicroseconds(7) ;
    digitalWrite(xt_data, HIGH) ;
    delayMicroseconds(5) ;
    digitalWrite(xt_clk, LOW) ;
      p++ ; 
   } 

  digitalWrite(xt_clk, LOW) ;
  delayMicroseconds(10) ;
  digitalWrite(xt_data, LOW) ;
  delayMicroseconds(5) ;
  digitalWrite(xt_data, HIGH) ;
  delay(10);
}
