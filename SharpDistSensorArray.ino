/*
SharpDistSensorArray.ino

Source: https://github.com/DrGFreeman/SharpDistSensor

*/


#include <SharpDistSensor.h>
#include <Adafruit_NeoPixel.h>


/* LED BEGIN */ 
#define PIN 3
#define PIN_2 5
#define PIN_3 9
#define PIN_4 11
#define NUM_LEDS 108
#define BRIGHTNESS 50

enum  pattern { NONE, THEATER_CHASE };
enum  direction { FORWARD, REVERSE };

// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
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
    
    // Constructor - calls base-class constructor to initialize LED strip
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
    :Adafruit_NeoPixel(pixels, PIN_2, type)
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

                case THEATER_CHASE:
                    TheaterChaseUpdate();
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
        show();
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
        show();
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
    
};

void Strip2Complete();

uint32_t c_yellow, c_red, c_blue, c_green, c_white, c_magenta, c_lightblue;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);
NeoPatterns strip2(NUM_LEDS, PIN_2, NEO_GRB + NEO_KHZ800, &Strip2Complete);

/* IR Sensor BEGIN */
// Define the number of sensors in the array as a constant
const byte nbSensors = 4; 

// Window size of the median filter (odd number, 1 = no filtering)
const byte medianFilterWindowSize = 21;

#define midiChannel (char)0

// Define the array of SharpDistSensor objects
SharpDistSensor sensorArray[] = {
  SharpDistSensor(A0, medianFilterWindowSize), // First sensor using pin A0
  SharpDistSensor(A1, medianFilterWindowSize), // Second sensor using pin A1
  SharpDistSensor(A2, medianFilterWindowSize), // Third sensor using pin A2
  SharpDistSensor(A3, medianFilterWindowSize), // Fourth sensor using pin A3
//  // Add as many sensors as required
};

// Define an array of integers that will store the measured distances
uint16_t distArray[nbSensors];
uint16_t oldNote1[3];
uint16_t oldNote2[3];
char hexval[5];

//MIDI stuff
//const byte notePitches[] = {C3, D3, E3, F3, G3, A3, B3};
uint8_t intensity;
uint8_t intensity2;
uint8_t oldIntensity1;
uint8_t oldIntensity2;
uint8_t note;
uint8_t note2;

uint16_t timer = 0;

/* IR Sensor END */

void setup() {
  // Set some parameters for each sensor in array
  for (byte i = 0; i < nbSensors; i++) {
    sensorArray[i].setModel(SharpDistSensor::GP2Y0A710K0F_5V_DS);  // Set sensor model
  }

   Serial.begin(115200);
   delay(1000);
	c_red     = strip.Color(255,0,0);
	c_blue    = strip.Color(0, 0, 255);
	c_lightblue = strip.Color(0, 0, 100);
	strip.setBrightness(BRIGHTNESS);
	strip.begin();
	strip.show(); // Initialize all pixels to 'off'
	strip2.TheaterChase(strip2.Color(0, 0, 100), strip2.Color(200, 0, 0), 0);
  
}

void loop() {
    // Read distance for each sensor in array into an array of distances
    for (byte i = 0; i < nbSensors; i++) {
      distArray[i] = sensorArray[i].getDist();
    }
	readIntensity(distArray[2], distArray[3]);
	readNotes(distArray[0], distArray[1]);
	playNotes();


    int a = map(distArray[0], 1000, 3000, 0, 108);
    constrain(a, 0, 108);
    colorWipe(round10(a)-10, round10(a),  c_blue);
    int wait = (1000 - map(distArray[2], 1000, 2000, 0, 1000));
    if((millis() - timer) > wait) {
        timer = millis();
        strip2.TheaterChaseUpdate();
    }

}


void colorWipe(uint32_t from1, uint32_t to1, uint32_t c) {
  for(uint16_t i=0;i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c_lightblue); 
//    strip2.setPixelColor(i, c_lightblue);
  }
  for(uint16_t i=from1; i<to1; i++) {
    strip.setPixelColor(i, c);
    strip.show();
  }
  
}

void readIntensity(int val, int val2)
{
  intensity = (uint8_t) (map(val, 1000, 5000, 0x00, 0x90));
  intensity2 = (uint8_t) (map(val2, 1000, 5000, 0x00, 0x90));
}

void readNotes(int val, int val2)
{
  note = (uint8_t) (map(val, 1000, 3000, 37, 59));
  note2 = (uint8_t) (map(val2, 1000, 3000, 37, 59));
}

int round15(int n) {
  return (n/15 + (n%15>2)) * 10;
}
int round10(int n) {
  return (n/10 + (n%10>2)) * 10;
}

void playNotes() {

     noteOn(0x90, note, intensity);
     noteOn(0x91, note2, intensity2);
}

void noteOn(byte channel, byte pitch, byte velocity) {
  Serial.write(channel);
  Serial.write(pitch);
  Serial.write(velocity);
}

void noteOff(byte channel, byte pitch, byte velocity) {
}


void Strip2Complete() {
  
}

