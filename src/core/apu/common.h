#pragma once

//        Square 1
// NR10 FF10 -PPP NSSS Sweep period, negate, shift
// NR11 FF11 DDLL LLLL Duty, Length load (64-L)
// NR12 FF12 VVVV APPP Starting volume, Envelope add mode, period
// NR13 FF13 FFFF FFFF Frequency LSB
// NR14 FF14 TL-- -FFF Trigger, Length enable, Frequency MSB

//        Square 2
//      FF15 ---- ---- Not used
// NR21 FF16 DDLL LLLL Duty, Length load (64-L)
// NR22 FF17 VVVV APPP Starting volume, Envelope add mode, period
// NR23 FF18 FFFF FFFF Frequency LSB
// NR24 FF19 TL-- -FFF Trigger, Length enable, Frequency MSB

//        Wave
// NR30 FF1A E--- ---- DAC power
// NR31 FF1B LLLL LLLL Length load (256-L)
// NR32 FF1C -VV- ---- Volume code (00=0%, 01=100%, 10=50%, 11=25%)
// NR33 FF1D FFFF FFFF Frequency LSB
// NR34 FF1E TL-- -FFF Trigger, Length enable, Frequency MSB

//        Noise
//      FF1F ---- ---- Not used
// NR41 FF20 --LL LLLL Length load (64-L)
// NR42 FF21 VVVV APPP Starting volume, Envelope add mode, period
// NR43 FF22 SSSS WDDD Clock shift, Width mode of LFSR, Divisor code
// NR44 FF23 TL-- ---- Trigger, Length enable

//        Control/Status
// NR50 FF24 ALLL BRRR Vin L enable, Left vol, Vin R enable, Right vol
// NR51 FF25 NW21 NW21 Left enables, Right enables
// NR52 FF26 P--- NW21 Power control/status, Channel length statuses

#define SQUARE1_CHANNEL gb->apu.square1
#define SQUARE2_CHANNEL gb->apu.square2
#define WAVE_CHANNEL gb->apu.wave
#define NOISE_CHANNEL gb->apu.noise
#define CONTROL_CHANNEL gb->apu.control

#define SAMPLE_RATE (4194304 / 48000)

// clocked at 512hz
#define FRAME_SEQUENCER_CLOCK 512

// 4 * 1024^2 / 512
#define FRAME_SEQUENCER_STEP_RATE 8192

enum EnvelopeMode {
    SUB = 0,
    ADD = 1
};
