#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_2PIN_PWM
#define MOZZI_CONTROL_RATE 256

#include <Mozzi.h>
#include <Oscil.h>
#include <tables/sin8192_int8.h>
#include <AutoMap.h>

Oscil<SIN8192_NUM_CELLS, MOZZI_AUDIO_RATE> aSin0(SIN8192_DATA); // audio oscillators
Oscil<SIN8192_NUM_CELLS, MOZZI_AUDIO_RATE> aSin1(SIN8192_DATA);
Oscil<SIN8192_NUM_CELLS, MOZZI_AUDIO_RATE> aSin2(SIN8192_DATA);
Oscil<SIN8192_NUM_CELLS, MOZZI_AUDIO_RATE> aSin3(SIN8192_DATA);

Oscil<SIN8192_NUM_CELLS, MOZZI_AUDIO_RATE> kFreq(SIN8192_DATA); // FM LFO

Oscil<SIN8192_NUM_CELLS, MOZZI_CONTROL_RATE> kVol0(SIN8192_DATA); // volume LFOs
Oscil<SIN8192_NUM_CELLS, MOZZI_CONTROL_RATE> kVol1(SIN8192_DATA);
Oscil<SIN8192_NUM_CELLS, MOZZI_CONTROL_RATE> kVol2(SIN8192_DATA);
Oscil<SIN8192_NUM_CELLS, MOZZI_CONTROL_RATE> kVol3(SIN8192_DATA);

const char INPUT_PIN_FREQ0 = A0; // analog read pins for audio oscillators
const char INPUT_PIN_FREQ1 = A1; 
const char INPUT_PIN_FREQ2 = A2;
const char INPUT_PIN_FREQ3 = A3;

const char INPUT_PIN_FM_FREQ = A4; // analog read pins for frequency lfo
const char INPUT_PIN_FM_INTENSITY = A5;

int freq0; // audio oscillator frequencies
int freq1;
int freq2;
int freq3;

int fm_freq_raw; // FM lfo frequency analog reading
float fm_freq; // scaled version
int fm_intensity;
char fm_component;

float kvolfreq0 = 0.007; // initial volume lfo frequencies
float kvolfreq1 = 0.05;
float kvolfreq2 = 0.054;
float kvolfreq3 = 2.12;

char v0,v1,v2,v3; // audio volumes updated each control interrupt and reused in audio till next control

int volume_lfo_select = 0; // state of lfo select button (0 for volume LFO1, etc)
int select_busy = 0; // debouncing-ish purposes
int decrease_busy = 0;
int increase_busy = 0;
int control_count = 0;

const float MIN_VOL_FREQ = 0.005;
const float MAX_VOL_FREQ = 4.69;
const float VOL_FREQ_INCREMENT = 0.005;
const int VOL_FREQ_BUFFER = 8;

AutoMap kMapFmMod(0, 1023, 0, 24);

int audio_count = 0; // debugging

void volumeLfoSelectButton(){  // detects the user cycling through volume lfo's

  if (digitalRead(2) == HIGH){ // if select button is not pressed
    select_busy = 0; // select button is not busy
  }
  else if ((digitalRead(2) == LOW) && (select_busy == 0)) { // if select button pressed AND button is not busy (debounced)
    select_busy = 1; // button is now busy
    volume_lfo_select++; // change to next volume lfo
    if (volume_lfo_select == 1) {
      digitalWrite(3, HIGH); // display '1' in binary on LEDS
      digitalWrite(4, LOW);
    }
    if (volume_lfo_select == 2) {
      digitalWrite(3, LOW); // display '2'
      digitalWrite(4, HIGH);
    }
    if (volume_lfo_select == 3) {
      digitalWrite(3, HIGH); // display '3'
      digitalWrite(4, HIGH);
    }
    if (volume_lfo_select == 4){
      volume_lfo_select = 0;
      digitalWrite(3, LOW); // display '0'
      digitalWrite(4, LOW);
    }
  }
  
}

void volumeLfoIncreaseDecrease(float &kvolfreq, int volume_lfo_number){ // detects the user increasing or decreasing the volume lfo's frequencies, then updates

  if (kvolfreq < MIN_VOL_FREQ){ // if minumum frequency reached
      analogWrite(6, 50); // turn on LED indicator
  }
  else if (digitalRead(5) == HIGH){ // if button not pressed
    digitalWrite(6, LOW); // make sure LED is off (still in frequency range implied)
    decrease_busy = 0; // set to not busy
  }
  else if ((digitalRead(5) == LOW) && (decrease_busy == 0)){ // if decrease frequency button pressed AND button not busy
    digitalWrite(6, LOW);
    kvolfreq = kvolfreq - VOL_FREQ_INCREMENT; // decrease LFO frequency value
    if (volume_lfo_number == 0){
      kVol0.setFreq(kvolfreq);  // set new frequency
    }
    else if (volume_lfo_number == 1){
      kVol1.setFreq(kvolfreq);
    }
    else if (volume_lfo_number == 2){
      kVol2.setFreq(kvolfreq);
    }
    else if (volume_lfo_number == 3){
      kVol0.setFreq(kvolfreq);
    }
    decrease_busy = VOL_FREQ_BUFFER; // once this reaches zero (after 8 interrupts) the frequency will decrease again if the button is still being held
  }

  if (kvolfreq > MAX_VOL_FREQ){ // if maximum frequency reached
    //digitalWrite(6, HIGH); // turn on LED indicator
    analogWrite(6, 100);
  }
  else if (digitalRead(7) == HIGH){ // if button not pressed
    increase_busy = 0; // set to not busy
  }
  else if ((digitalRead(7) == LOW) && (increase_busy == 0)){ // if increase frequency button pressed AND button not busy
    digitalWrite(6, LOW);
    kvolfreq = kvolfreq + VOL_FREQ_INCREMENT; // increase LFO0 frequency value
    if (volume_lfo_number == 0){
      kVol0.setFreq(kvolfreq);  // set new frequency
    }
    else if (volume_lfo_number == 1){
      kVol1.setFreq(kvolfreq);
    }
    else if (volume_lfo_number == 2){
      kVol2.setFreq(kvolfreq);
    }
    else if (volume_lfo_number == 3){
      kVol0.setFreq(kvolfreq);
    }
    increase_busy = VOL_FREQ_BUFFER; // once this reaches zero (after 8 interrupts) the frequency will decrease again if the button is still being held
  }

  if (decrease_busy){ // if not 0 (0 would mean button is not busy, so control counting is not necessary)
    decrease_busy--; // count interrupts (for debouncing and also to allow hold to increase at a usable speed)
  }
  if (increase_busy){
    increase_busy--;
  }

}

void setup(){

  aSin0.setFreq(100); // set audio oscillator frequencies
  aSin1.setFreq(100);
  aSin2.setFreq(100);
  aSin3.setFreq(100);

  kFreq.setFreq(100); // set FM lfo frequency

  kVol0.setFreq(kvolfreq0); // set volume LFO frequencies
  kVol1.setFreq(kvolfreq1);
  kVol2.setFreq(kvolfreq2);
  kVol3.setFreq(kvolfreq3);

  v0 = v1 = v2 = v3 = 127; // set initial volumes

  pinMode(2, INPUT_PULLUP); // to select volume LFO
  pinMode(3, OUTPUT); // selected LFO LED indicator (binary output 2 bits)
  pinMode(4, OUTPUT); // selected LFO LED indicator
  digitalWrite(3, LOW); // initialise as '0'
  digitalWrite(4, LOW);

  pinMode(5, INPUT_PULLUP); // to decrease frequency
  pinMode(6, OUTPUT); // min/max frequency LED indicator
  digitalWrite(6, LOW);
  pinMode(7, INPUT_PULLUP); // to increase frequency

  // debugging
  Serial.begin(115200);

  startMozzi();
}

void loop(){

  audioHook();
}


void updateControl(){

  freq0 = mozziAnalogRead<10>(INPUT_PIN_FREQ0); // read audio frequencies (8 bit resolution, 0-255)
  freq1 = mozziAnalogRead<10>(INPUT_PIN_FREQ1);
  freq2 = mozziAnalogRead<10>(INPUT_PIN_FREQ2);
  freq3 = mozziAnalogRead<10>(INPUT_PIN_FREQ3);

  fm_freq_raw = mozziAnalogRead<10>(INPUT_PIN_FM_FREQ); // read FM LFO frequency
  //fm_freq = kMapFmMod(fm_freq_raw); // scaling
  fm_freq = fm_freq_raw * 0.001;
  fm_intensity = (mozziAnalogRead<10>(INPUT_PIN_FM_INTENSITY)); // read intensity
  fm_intensity = kMapFmMod(fm_intensity);

  kFreq.setFreq(fm_freq);
  fm_component = (kFreq.next());

  aSin0.setFreq(freq0 + (fm_component * fm_intensity)); // update audio frequencies (*4 for warmer frequency range)
  aSin1.setFreq(freq1);
  aSin2.setFreq(freq2);
  aSin3.setFreq(freq3);

  volumeLfoSelectButton(); // detect volume lfo selection change

  if (volume_lfo_select == 0){
    volumeLfoIncreaseDecrease(kvolfreq0, 0);  // update the respective frequencies
  }
  else if (volume_lfo_select == 1){
    volumeLfoIncreaseDecrease(kvolfreq1, 1);
  }
  else if (volume_lfo_select == 2){
    volumeLfoIncreaseDecrease(kvolfreq2, 2);
  }
  else if (volume_lfo_select == 3){
    volumeLfoIncreaseDecrease(kvolfreq3, 3);
  }

  v0 = kVol0.next(); // update volumes
  v1 = kVol1.next();
  v2 = kVol2.next();
  v3 = kVol3.next();


  // debugging
  Serial.print("1...");
  Serial.println((float)fm_intensity);
}

AudioOutput updateAudio(){


  long asig1 = (long) 
  aSin0.next()*v0 +
  aSin1.next()*v1 +
  aSin2.next()*v2 +
  aSin3.next()*v3;

  // debugging
  /*
   if(!audio_count) // if = 0
   {
     Serial.println(asig1);
     audio_count = 256;
   }
   audio_count--;
   */

  return MonoOutput::fromAlmostNBit(20, asig1).clip();
}
