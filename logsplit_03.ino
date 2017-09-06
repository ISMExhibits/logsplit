/* Lincoln's Log Split
 *  
 *  LOGIC
 *  The program is meant to loosely simulate splitting wood. To that end it detects the apparatus being
 *  hit with a mallet "hard enough", lights some LEDS for visual feedback, and plays an appropriate 
 *  sound effect. The lights and sound vary depending on how many times the log has been struck. On the 
 *  first hit, the sound effect is a metal hammer striking a metal wedge and wood creaking/cracking.
 *  The LEDS run the animation with the "hit" color. If the second hit comes too fast the sound effect 
 *  is a glancing blow with a thud, no splitting sound. The LEDS do not animate. If the second and  
 *  and subsequent hits are detected after a short pause the program plays an effect similar to the first
 *  strike. After a number of strikes between 2 to 5 (selected randomly) the programs plays a "spit
 *  sound" which has hammer strike, wood cracking, and pieces falling to the ground. The LEDS animate
 *  with a success color. The number of pieces split is sent to the display, and the program resets.
 *  
 *  SENSOR
 *  This program listens to a pizeo element to detect vibration. 
 *  It reads an analog pin (A1) and compares the result to a threshold
 *  If the result is greater than the threshold the, it toggles an LED (13)
 *  and returns "true" to the other parts of the program to trigger lights and
 *  sound.
 *  
 *  LIGHTS
 *  The leds are DOTSTAR from Adafruit (144led/m, black pcb). They are driven with the 
 *  FastLED library. When a knock is detected an animation starts (or restarts if it is already
 *  running). The animation moves four dots down the strip (which is installed in a circle). The 
 *  dots position is determined by four sin waves 90 degrees out of phase from one another. On each
 *  itereation the sin functions fully light one led and all the rest are faded. This give the effect
 *  of trailing "streaks".
 *  
 *  SOUND
 *  The sound is driven by a Teensy Audio Adapter board and the Teensy Audio Library. When a knock is 
 *  detected a sound file is selected from a list and played. There are serveral types of sound: "glancing 
 *  hits", "hit with crack", and "hit with split". On sucessive knocks the program plays a random number 
 *  of "hits with cracks" (up to 4) before playing a "hit with split" sound.
 *  
 */

#include <Adafruit_DotStar.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <CmdMessenger.h>

////// GUItool: begin automatically generated code
AudioPlaySerialflashRaw playFlashRaw1; //xy=149,388
AudioMixer4 mixer1; //xy=445,386
AudioOutputAnalog dac1; //xy=591,379
AudioConnection patchCord1(playFlashRaw1, 0, mixer1, 0);
AudioConnection patchCord2(mixer1, dac1);
////// GUItool: end automatically generated code

//Audio Definitions
#define PROP_AMP_ENABLE  5
#define FLASH_CHIP_SELECT 6

//LED Definitions
#define NUMPIXELS 144 // Number of LEDs in strip
#define LED_ENABLE 7
#define DOTSTAR_BRG (1 | (2 << 2) | (0 << 4))

// LED Pattern types supported:
enum  pattern { NONE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE };
// Patern directions supported:
enum  direction { FORWARD, REVERSE };
// Sound Types
enum sound {STRIKE, BREAK, MISS, CREAK};

// these constants won't change:
const int knockSensor = A1; // the piezo is connected to analog pin 0
const int threshold = 100;  // threshold value to decide when the detected sound is a knock or not
// these variables will change:
int sensorReading = 0;      // variable to store the value read from the sensor pin
boolean knocked = false;

//sensor timing
elapsedMicros updateSensor = 0;
const int sensorDelay = 1000; //1000hz sample rate
elapsedMillis sensorLockout = 0;
const int lockout = 300; //let the ringing subside
 
// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_DotStar
{
    public:
 
    // Member Variables:  
    pattern  ActivePattern;  // which pattern is running
    direction Direction;     // direction to run the pattern
    
    unsigned long Interval;   // milliseconds between updates
    unsigned long lastUpdate; // last update of position
    
    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern
    
    void (*OnComplete)();  // Callback on completion of pattern
    
    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(uint16_t n, uint8_t o, void (*callback)())
    :Adafruit_DotStar(n, o)
    {
        OnComplete = callback;
    }
    
    // Update the pattern
    void Update()
    {
        if((millis() - lastUpdate) > Interval) // time to update
        {
            lastUpdate = millis();
            switch(ActivePattern)
            {
                case RAINBOW_CYCLE:
                    RainbowCycleUpdate();
                    break;
                case THEATER_CHASE:
                    TheaterChaseUpdate();
                    break;
                case COLOR_WIPE:
                    ColorWipeUpdate();
                    break;
                case SCANNER:
                    ScannerUpdate();
                    break;
                case FADE:
                    FadeUpdate();
                    break;
                default:
                    break;
            }
        }
    }
  
    // Increment the Index and reset at the end
    void Increment()
    {
        if (Direction == FORWARD)
        {
           Index++;
           if (Index >= TotalSteps)
            {
                Index = 0;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
        else // Direction == REVERSE
        {
            --Index;
            if (Index <= 0)
            {
                Index = TotalSteps-1;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
    }
    
    // Reverse pattern direction
    void Reverse()
    {
        if (Direction == FORWARD)
        {
            Direction = REVERSE;
            Index = TotalSteps-1;
        }
        else
        {
            Direction = FORWARD;
            Index = 0;
        }
    }
    
    // Initialize for a RainbowCycle
    void RainbowCycle(uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = RAINBOW_CYCLE;
        Interval = interval;
        TotalSteps = 255;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            setPixelColor(i, Wheel(((i * 256 / numPixels()) + Index) & 255));
        }
        ledShow();
        Increment();
    }
 
    // Initialize for a Theater Chase
    void TheaterChase(uint32_t color1, uint32_t color2, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = THEATER_CHASE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
   }
    
    // Update the Theater Chase Pattern
    void TheaterChaseUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            if ((i + Index) % 3 == 0)
            {
                setPixelColor(i, Color1);
            }
            else
            {
                setPixelColor(i, Color2);
            }
        }
        ledShow();
        Increment();
    }
 
    // Initialize for a ColorWipe
    void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = COLOR_WIPE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Color Wipe Pattern
    void ColorWipeUpdate()
    {
        setPixelColor(Index, Color1);
        ledShow();
        Increment();
    }
    
    // Initialize for a SCANNNER
    void Scanner(uint32_t color1, uint8_t interval)
    {
        ActivePattern = SCANNER;
        Interval = interval;
        //TotalSteps = (numPixels() - 1) * 2;
        TotalSteps = (numPixels() - 1);
        Color1 = color1;
        Index = 0;
    }
 
    // Update the Scanner Pattern
    void ScannerUpdate()
    { 
        for (int i = 0; i < numPixels(); i++)
        {
            if (i == Index)  // Scan Pixel to the right
            {
                 setPixelColor(i, Color1);
            }
            else if (i == TotalSteps - Index) // Scan Pixel to the left
            {
                 setPixelColor(i, Color1);
            }
            else // Fading tail
            {
                 setPixelColor(i, DimColor(getPixelColor(i)));
            }
        }
        ledShow();
        Increment();
    }
    
    // Initialize for a Fade
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = FADE;
        Interval = interval;
        TotalSteps = steps;
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Fade Pattern
    void FadeUpdate()
    {
        // Calculate linear interpolation between Color1 and Color2
        // Optimise order of operations to minimize truncation error
        uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
        uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
        uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;
        
        ColorSet(Color(red, green, blue));
        ledShow();
        Increment();
    }
   
    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color)
    {
        // Shift R, G and B components one bit to the right
        uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
        return dimColor;
    }
 
    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
        for (int i = 0; i < numPixels(); i++)
        {
            setPixelColor(i, color);
        }
        ledShow();
    }

    void ledShow(){
      SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
      digitalWrite(LED_ENABLE, HIGH);  // enable access to LEDs
      show();                     // Refresh strip
      digitalWrite(LED_ENABLE, LOW);
      SPI.endTransaction();   // allow other libs to use SPI again
    }
    
    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    }
 
    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    }
 
    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
        return color & 0xFF;
    }
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos)
    {
        WheelPos = 255 - WheelPos;
        if(WheelPos < 85)
        {
            return Color(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if(WheelPos < 170)
        {
            WheelPos -= 85;
            return Color(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            WheelPos -= 170;
            return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }
 };

void Ring1Complete();
NeoPatterns Ring1(NUMPIXELS, DOTSTAR_BRG, &Ring1Complete);

CmdMessenger cmdMessenger = CmdMessenger(Serial1);
CmdMessenger cmdMessengerExt = CmdMessenger(Serial);

enum
{
  kLight  , //
};

enum
{
  kPlay  , //
};


//Counter
int strikeNum = 0;
int strikeCount = 0;

//Timing
elapsedMillis sound = 0;
elapsedMillis sinceStrike = 0;
int swingPeriod = 300;

void setup() {
  Serial.begin(9600);  // wait up to 3 seconds for the Serial device to become available
  Serial1.begin(9600);
  long unsigned debug_start = millis ();
  while (!Serial && ((millis () - debug_start) <= 3000));
  Serial.println("Indiana State Museum Logsplit....ver. 03"); 
  cmdMessenger.printLfCr();
  cmdMessengerExt.printLfCr();
  cmdMessengerExt.attach(kPlay, playTest);
  //Audio Setup (see audio.ino)
  audioSetup();

 //Led Set-up
     // Initialize all the pixelStrips
     pinMode(7, OUTPUT);
     digitalWrite(7,LOW);
     Ring1.begin();
  
     // Kick off a pattern
     Ring1.Scanner(Ring1.Color(255,255,0), 10); //Framerate of 100 frames/sec
     // Session Set-up
     strikeNum = random(0,5);

}

void loop() {
  // put your main code here, to run repeatedly:
  cmdMessengerExt.feedinSerialData();
  Ring1.Update();
  if(knock()){
    if(sinceStrike > swingPeriod){
       sinceStrike = 0;
      if(strikeCount >= strikeNum){ //Break sound and lights, send command to display
        playFromArray(1);
        Ring1.Scanner(Ring1.Color(255,255,0), 5); //Framerate of 200 frames/sec SOFTWOOD
        //Ring1.Scanner(Ring1.Color(255,0,255), 5); //Framerate of 200 frames/sec HARDWOOD
        strikeCount = 0;
        //strikeNum = random(0,3); //hardwood
        strikeNum = random(0,3); //softwood
        cmdMessenger.sendCmd(kLight,"hardwood");             
       }
      else if (strikeCount < strikeNum){ //strike sound and lights
        playFromArray(0);
        Ring1.Fade(Ring1.Color(255,255,0),Ring1.Color(0,0,0),60, 5); //Framerate of 100 frames/sec SOFTWOOD
        //Ring1.Fade(Ring1.Color(255,0,255),Ring1.Color(0,0,0),60, 5); //Framerate of 100 frames/sec HARDWOOD
        strikeCount++;
       }
      playFromArray(2); //Miss sound, no lights
    } 
  }
  if (sinceStrike > 300000){
    playFromArray(3);
    Ring1.Scanner(Ring1.Color(200,20,20), 5); //Framerate of 200 frames/sec
    sinceStrike = 0;
  }
   if ((sinceStrike > 5000) && (strikeCount > 0) ){
    playFromArray(3);
    strikeCount = 0;
   }
}

void playTest(){
  playFromArray(4);
}

boolean knock(){
  if (updateSensor > sensorDelay){
    updateSensor = updateSensor - sensorDelay;
    if (sensorLockout > lockout){
      // read the sensor and store it in the variable sensorReading:
      sensorReading = analogRead(knockSensor);
      
      // if the sensor reading is greater than the threshold:
      if (sensorReading >= threshold) {
        // Uncomment to send the string "Knock!" back to the computer, followed by newline
        Serial.print("Knock: ");
        Serial.println(sensorReading);
        sensorLockout = sensorLockout - lockout;  //reset the "debounce/lockout period"
        return true;
      }
    }
  }  
  return false;
}


/*Completion Routines - get called on completion of a pattern
*------------------------------------------------------------
*/ 
// Ring1 Completion Callback
void Ring1Complete() //Fade to black and stop
{
for (int i=0; i < 9; i++){ //Fade all to black
        for (int j = 0; j < 144; j++){
          Ring1.setPixelColor(j, Ring1.DimColor(Ring1.getPixelColor(j)));
        }
        Ring1.ledShow();
      }
      Ring1.ActivePattern = NONE;
   
}
