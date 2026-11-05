AmpFett SVT-CL Bass Amp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.0

Two formats, one engine
  AmpFett_SVTCL.clap and AmpFett_SVTCL.vst3 contain the same code and
  sound identical. Use whichever your DAW prefers. CLAP is the native
  format, the VST3 is built from it with clap-wrapper (free-audio, MIT) and
  the VST3 SDK 3.8 (Steinberg, MIT). A project saved with one format does
  not load its settings into the other.
  Third-party licenses: THIRD-PARTY-LICENSES.txt.

Updating
  Close REAPER first. Copying a new .clap over the file a running REAPER has
  loaded makes REAPER crash (at once or a while later). scripts/update-
  tonestack.sh refuses to run while REAPER is open and never overwrites a
  plugin file in place.

Installation (CLAP)
  mkdir -p ~/.clap && cp AmpFett_SVTCL.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "AmpFett SVT-CL Bass Amp".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r AmpFett_SVTCL.vst3 ~/.vst3/
  (AmpFett_SVTCL.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  AmpFett SVT-CL is a rebuild of the Ampeg SVT-CL, the 300 W all-tube bass
  head, after the service manual (preamp 07S519-12, power amp 07S419-12,
  power supply, tube board, speaker output board) and the owner's guide /
  owner's manual specifications. The whole amp is solved sample by sample
  as one circuit (nodal DK method, like the other tonestack amps; 94 nodes,
  51 states, 29 nonlinear ports), from the input jacks to the output
  transformer taps:
    - inputs 0 dB and -15 dB with their switching contacts (22 k / 100 k,
      -14.9 dB on the -15 dB jack), V1B (12AX7)
    - ULTRA LO (bass boost / mid cut network in front of GAIN), GAIN
      (1 M log), V1A (12AX7) with ULTRA HI (0.033 uF across part of the
      cathode resistor)
    - the passive tone stack: BASS and TREBLE (1 M log), then V2B (12AX7)
      with MIDRANGE (50 k linear) between its cathode and the output
      divider: a series resonance of the tapped inductor L1 (100 / 300 /
      800 mH) with one of five capacitors - FREQUENCY 220, 450, 800 Hz,
      1.6 and 3 kHz; counter-clockwise cuts, clockwise boosts
    - MASTER (50 k linear), the cathode follower V2A, PREAMP OUT
    - power amp: input stage V1A (12AX7) with the negative feedback from
      the output transformer, the cathodyne phase inverter V1B, the
      drivers V2/V3 (12AU7, gain stage plus cathode follower per side),
      six 6550 in push-pull with fixed bias, screen grids from the 345 V
      line, the output transformer (1750 ohm primary, 4 and 2 ohm taps)
    - the power supply (660 V, 470 V, 345 V, 380 V / 365 V, -180 V bias)
      sags with the signal
  Checked against the schematic and the specifications
  (engine/tests/svttest.cpp):
    - preamp signal levels at 1 kHz, 15 mV in, GAIN and MASTER full (the
      boxes in the schematic): after V1B 0.84 V (0.9), after V1A 6.84 V
      (7), V2B anode 7.67 V (8), preamp output 0.38 V (0.4)
    - BASS +-13.1 dB at 40 Hz (spec +-12); TREBLE +14.4 / -17.6 dB at
      4 kHz (+15 / -20); MIDRANGE +10.7 ... +9.9 / -19.8 ... -18.3 dB at
      220 / 450 / 800 / 1600 / 3000 Hz (+10 / -20); ULTRA LO +1.8 dB at
      40 Hz / -10.8 dB at 500 Hz (+2 / -10); ULTRA HI +9.3 dB at 8 kHz
      (+9); -15 dB input -14.95 dB
    - frequency response at 15 kHz -2.9 dB (spec -3)
    - power at 3 % THD about 360 W into 4 ohm and into 2 ohm (spec at
      least 300 W)
  Known deviations: overall gain 72.6 dB instead of 67 dB, and 300 W are
  reached with about 0.25 V instead of 0.4 V at the power amp input (with
  the schematic values the driver amplifies about 8 times, while the boxes
  in the schematic show 5.5 V -> 30 V). 40 Hz is +0.5 dB instead of -3 dB.
  Estimated (not in the manual): inductance, leakage and losses of the
  output transformer, the inductor L1 (winding resistance, coupling),
  internal resistances of the power supply, which transformer tap feeds
  the negative feedback (4 ohm assumed), the 6550 model (fitted to data
  sheet points), the log taper of the A pots (15 % at center), the source
  impedance (1 kohm). No speaker, no SLAVE / BALANCED outputs.

Parameters
  Gain, Bass, Midrange (0 ... 10); Frequency (220 Hz, 450 Hz, 800 Hz,
  1.6 kHz, 3 kHz); Treble, Master (0 ... 10); Ultra Hi (Off / On); Ultra
  Lo (Off / On); Input (0 dB / -15 dB jack); Output tap (4 ohm / 2 ohm);
  Standby (Off / On); Power (Off / On); Instrument level at 0 dBFS (mV
  peak, 1 ... 1000); Oversampling (1x, 2x, 4x); Output level (-36 ...
  +24 dB); Pot taper (A pots) (log 15 % / log 10 % / linear; GAIN, BASS,
  TREBLE).
  Knob 10 = clockwise end. Standby on or Power off mutes the output.
  Defaults: all knobs 5, Frequency 800 Hz, Ultra Hi and Ultra Lo off,
  0 dB input, 4 ohm tap, standby off, power on, 100 mV, 1x, 0 dB, log 15 %.

User interface
  Front panel drawn after the layout in the owner's manual (no photo, no
  logo; "AmpFett" lettering in Damion, SIL Open Font License): 0 dB and
  -15 dB INPUT jacks (click a jack to plug in there), GAIN, toggle
  switches ULTRA HI and ULTRA LO, BASS, MIDRANGE, FREQUENCY (five detented
  positions), TREBLE, MASTER, the STANDBY / POWER / FAULT lamp (green =
  playing, red = standby), toggle switches STANDBY and POWER (lever up =
  on). Knobs: drag vertically, mouse wheel, double-click = center. Below the
  panel the model strip (not on the amp): OVERSAMPLING, OUTPUT TAP (4 / 2
  ohm), POT TAPER, the DRIVE meters (grid swing of V1B input, V1A gain, V2B
  midrange, power amp input, 6550 grids, 100 % = grid reaches 0 V; output
  power into the selected load, mark = 300 W), INPUT (instrument level) and
  OUTPUT. Below that the small-signal frequency response at the operating
  point (relative to 1 kHz). Window resizable. Needs libcairo2 and libX11.

Levels
  INPUT sets what peak voltage at the jack corresponds to 0 dBFS of the
  track (default 100 mV; passive basses about 100-300 mV, active basses up
  to 1 V and more). The output is the voltage at the selected output
  transformer tap: 60 V peak = 0 dBFS into 4 ohm, 42.4 V peak = 0 dBFS
  into 2 ohm (the same power, 450 W, in both cases; 300 W into 4 ohm are
  49 V peak).

Oversampling and latency
  1x (default) is enough for clean and moderately driven playing; 2x or 4x
  reduce aliasing in heavy distortion and cost two or four times the CPU.
  Changing it prepares a new engine in the background (the amp settles
  first, about 1-3 s of computing) and switches over after a moment. The
  reported latency is always 31 samples (the oversampling filters; at 1x
  the plugin delays by the same amount), so it does not change with the
  setting.

CPU
  About 1.5 times the AmpFett B-15NF: roughly 40-75 % of one core at
  48 kHz and 1x on a typical desktop (the whole amp with six output tubes
  and the power supply is solved every sample); 2x and 4x correspondingly
  more, stereo with different left and right signals doubles that. With
  the same signal on both channels it is computed once.

1.0
  - First release. No speaker.

Notes
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer).
  AmpFett is a rebuild of the Ampeg SVT-CL. Ampeg is a trademark of its
  owner (LOUD / Yamaha Guitar Group); this is an independent circuit
  simulation, not affiliated with or endorsed by them.
