// Simple Karplus-Strong //

#define SAMPLE_RATE 22050
#define SIZE        256
#define OFFSET      32
#define LED_PIN     13
#define LED_PORT    PORTB
#define LED_BIT     5
#define SPEAKER_PIN 11

  int out;
  int last = 0;
  int curr = 0;
  uint8_t delaymem[SIZE];
  uint8_t locat = 0;
  uint8_t bound = SIZE; //bound is the period of the delayline
  int accum = 0;
  int lowpass = 0.1;
  bool trig = false;

// rhythm index...
int pulse = 500;
int breve = pulse * 8;
int wholeTrip = pulse * 6;
int whole = pulse * 4;
int halfTrip = pulse * 3 ;
int half = pulse * 2 ;
int quarter = pulse ;
int quaver = pulse / 2;
int quaverTrip = pulse / 3; 
int semi = pulse /4 ;
int fives = pulse / 5; 
int semiTrip = pulse / 6;
int sevens = pulse / 7 ;
int demiSemi  = pulse / 8; // lol wut
int demiTrip = pulse / 9;
int demiDemi = pulse / 12;  

int rhythmTable[16]=
{whole, quarter, quaver, semi, demiSemi, demiDemi, breve, wholeTrip, 
halfTrip, quaverTrip, semiTrip, demiTrip, fives, sevens, quaver, semi}; // 0-15 rhythm elements in a zero-indexed array!


ISR(TIMER1_COMPA_vect) {
   
  OCR2A = 0xff & out; //send output to timer A, how sound leaves the pin!

  if (trig) { //if the karplus gets triggered
    
    for (int i = 0; i < SIZE; i++) delaymem[i] = random(); //fill the delay line with random numbers
    
    trig = false; //turn off the trigger
    
  } else { 
    
    delaymem[locat++] = out;        //the input to the delay line the output of the previous call of this function
    if (locat >= bound) locat = 0;  //this makes the delay line wrap around. bound is the effective length of the line.
                                   //if you surpass the bound, move pointer back to zero.
                            
    
    curr = delaymem[locat]; //get the new output of the delay line.
    
    out = accum >> lowpass; //divide the previous output by two
                            //controls the dampening factor of the string, reduces the energy by lopass
    
    //accum = accum - out + ((last>>1) + (curr>>1)); //1 pole IR filter, a smoothing function, 
                                                   //averages the current output and the previous output

    accum = accum - out + ((last>>1) + (curr>>1)); 

    last = curr;
    
  }
   
}

void startPlayback()
{
    pinMode(SPEAKER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT); 
    // Set up Timer 2 to do pulse width modulation on the speaker
    // pin.

    ASSR &= ~(_BV(EXCLK) | _BV(AS2));
  // Use internal clock (datasheet p.160)
   
    TCCR2A |= _BV(WGM21) | _BV(WGM20);
    TCCR2B &= ~_BV(WGM22);
  // Set fast PWM mode  (p.157)

    TCCR2A = (TCCR2A | _BV(COM2A1)) & ~_BV(COM2A0);
    TCCR2A &= ~(_BV(COM2B1) | _BV(COM2B0));
    // Do non-inverting PWM on pin OC2A (p.155)
        // On the Arduino this is pin 11.
        
    TCCR2B = (TCCR2B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
  // No prescaler (p.158)
  
    OCR2A = 0;
  // Set initial pulse width to zero/
  
    cli();
  // Set up Timer 1 to send a sample every interrupt. or, clear interrupts

    TCCR1B = (TCCR1B & ~_BV(WGM13)) | _BV(WGM12);
    TCCR1A = TCCR1A & ~(_BV(WGM11) | _BV(WGM10));
    // Set CTC mode (Clear Timer on Compare Match) (p.133)
    // Have to set OCR1A *after*, otherwise it gets reset to 0!

    TCCR1B = (TCCR1B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
    // No prescaler (p.134)
    
    OCR1A = F_CPU / SAMPLE_RATE;
 // Set the compare register (OCR1A).
    // OCR1A is a 16-bit register, so we have to do this with
    // interrupts disabled to be safe. 
    // 16e6 / x = y


    TIMSK1 |= _BV(OCIE1A);
   // Enable interrupt when TCNT1 == OCR1A (p.136)
   
    sei();
    
}

void stopPlayback()
{

    TIMSK1 &= ~_BV(OCIE1A);
 // Disable playback per-sample interrupt.
 
    TCCR1B &= ~_BV(CS10);
 // Disable the per-sample timer completely.
 
    TCCR2B &= ~_BV(CS10);
 // Disable the PWM timer.
 
    digitalWrite(SPEAKER_PIN, LOW);
 // turn pin down
}


void setup() {

  startPlayback();
  randomSeed(analogRead(0));
 
 // Serial.begin(9600);
}


void loop() 
{

int j = 1;
  
for (int i = 0; i > -1; i = i + j) //wrap entire karplus structure in this loop.
                              //set up a timer which will add infinitely to itself
    {
      if (i == 30) // catch the edge of the timer!
      {
      j = -1;  // switch direction at peak
      }
  
  LED_PORT ^= 1 << LED_BIT; // toggles an LED for debug purposes. 
  trig = true; //true at the trigger fires a karplus grain.
  

//PITCH CONTROL 
  //  bound = random(OFFSET, SIZE); 
  // BOUND is pithc--random notes between offset and size effect the pitch of an articulated karplus grain  
 
 //int tab = map(analogRead(1), 0, 1023, 0, 15); //convert an analog input to read through the rhythm table
   int tab = random(0, 15);  // use a random sequence of numbers through the rhythm table
   
   int thermal = map(analogRead(5), 0, 1023, 0, 100); //attenutate the thermal variation 
   bound = rhythmTable[tab] + thermal; // put a little thermal variation on the pitch


//LOPASS FILTERING
    //int value = analogRead(2); // set up an analog input. use sensor 5 for thermistor!
    int value = random(0, 1023); // put a random articulator variable onto the lopass input
    float falue = map(value, 0,1023,1,3000) / 1000.0; //shift the value to between 0.0 and 3.0
    lowpass = falue; //route that value to the lopass variable

//RHYTHMIC ARTICULATION  
int bond = bound*i; //takes the bound, multiples it by the expanding variable, will go to 30!

//int vari = i * 3;
//bond = bond + vari;

delay (bond);
  
  //Serial.println(i);
  }
  
}
