Inspired by and based on Soundcode's 8knobs project, which can be found here below. Utilises the mozzi library.
https://github.com/SoundCodes/8knobs

This project features 4x audio LFOs, 4x amplitude modulator LFOs, and 1x frequency modulator LFO.

//////////////////////

- The audio LFO's frequencies are controlled via 4 potentiometers connected to the Arduino's ADC pins.

- The amplitude modulator LFO's frequencies (which modulate the amplitude the audio LFOs) are controlled by 2 push butons (increase/decrease), with a seperate button to cycle between LFOs. This enables all 4 amplitude LFOs to be modified with only 3 buttons.

- The FM LFO is used to modulate the frequency of just one of the audio LFOs. This is controlled via 2 more potentiometers, for frequency and intensity.

//////////////////////

Initially I experimented with having 8 audio oscillators and 8 amplitude LFOs, as is such in the 8 knobs project. However, the processor could not seem to handle the addition of the FM LFO (at an acceptable control rate), so I decided to lower the number of oscillators to maintain a higher quality audio output.
