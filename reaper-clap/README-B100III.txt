Yamaha B100-III Bass Amp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.0

Two formats, one engine
  Yamaha_B100III.clap and Yamaha_B100III.vst3 contain the same code and
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
  mkdir -p ~/.clap && cp Yamaha_B100III.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Yamaha B100-III Bass Amp".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Yamaha_B100III.vst3 ~/.vst3/
  (Yamaha_B100III.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  A rebuild of the Yamaha B100-112III / B100-115III bass amplifier after the
  service manual (1984: schematic, specifications) and the owner's manual.
  The whole preamp is solved sample by sample as one circuit (nodal DK
  method, like the other tonestack amps), from the input jack to the gate
  of the power amp's input FET:
    - input stage IC101 1/2 (NJM2041, gain 11, double diode D101 between
      its inputs), VOLUME (25 k log) and the buffer IC101 2/2 (gain 2)
    - the active tone control IC103 1/2: TREBLE, MIDDLE and BASS (20 k,
      linear) between the two op-amp inputs, center = flat; TREBLE a shelf
      with 1 k + 0.22 uF, MIDDLE and BASS series resonances with the
      transistor gyrators Tr103 / Tr104 (about 314 Hz and 92 Hz), about
      +-11 to 12 dB each
    - two fully parametric EQs (identical apart from one resistor): a
      summing pair with the LEVEL pot between its summing points, a state-
      variable filter (FREQUENCY: 50 k tandem pot, 0.068 uF; Q: 25 k log
      rheostat against 4.7 k), the band-pass amplifier, PRESET LEVEL (the
      volume of the engaged EQ, +-10 dB) and the switching transistors
      Tr105 / Tr107 (2SC2878) that the footswitch drives
    - the output network over PRE AMP OUT / POWER AMP IN and its loads
      (BAL OUTPUT LEVEL at center, headphone and crossover branches)
  Op-amps are modelled as voltage-controlled sources with finite gain,
  output resistance and supply clamps (+-13.5 V), so hard clipping of a
  stage sounds like the real thing. The power amp is a linear block with
  the shape of its feedback network (330 k against 1 k + 10 uF, DC servo
  IC113: high-pass at 1.6 Hz and 16 Hz) and a soft clip at its limit
  (100 W into 8 ohm at 10 % THD = 40 V peak; knee 37 V, limit 44 V).
  No speaker (no data for the JA3812 woofer could be found), no crossover,
  no balanced output, no headphone amplifier.
  Checked against the specifications (engine/tests/b3test.cpp):
  INPUT -> PRE AMP OUT 20 dB (computed 19.9 dB from the schematic values,
  without any adjustment); LEVEL +-15 dB (+-15.4); FREQUENCY 50 Hz ...
  2.5 kHz (52 Hz ... 2.3 kHz - the pot does not quite reach the panel's
  2.5K mark); Q 0.35 ... 3.0 (0.34 ... 2.1, see below); PRESET LEVEL
  +-10 dB (+-10.1).
  Known deviations: Q only reaches 2.1 instead of 3.0 with the schematic
  values (the ratio of broadest to narrowest Q is fixed at 6.3 by the 25 k
  Q pot against 4.7 k; the broad end matches). The power amp has 332 x
  (50.4 dB) after the schematic, the specification says 41 dB, and the
  input sensitivity (POWER AMP IN 245 mV for 100 W) fits 41 dB too; the
  model uses 41 dB with the shape of the feedback network. INPUT -> SPEAKER
  OUT then is 60.8 dB (specification 64 dB; with the schematic's power amp
  it would be 70 dB - the specification is not consistent in itself).
  Estimated (not in the manual): the D101 diode model, the current gain of
  the gyrator transistors (300), the op-amp limits, open-loop gain and
  output resistance, the knee of the power amp, the log taper of the A pots
  (15 % at center), the G pots as linear, the BAL OUTPUT LEVEL position
  (center, as in the owner's manual), the source impedance (1 kohm). From
  the data sheet: on-resistance of the 2SC2878 switches (1 ohm typ.).

Parameters
  Volume (0 ... 10); Treble, Middle, Bass (-5 ... +5, 0 = flat);
  EQ 1 and EQ 2: Level (-5 ... +5, 0 = flat), Q (0 = broad ... 10 =
  narrow), Frequency (0 ... 10 after the panel scale: 50 Hz, 100 at 2.9,
  250 at 5, 500 at 7.1, 2.5 kHz at 10; the pot ends at about 2.3 kHz),
  Preset level (-5 ... +5, 0 = same volume as with the EQ off), On (the
  footswitch: On / Off); Power amp (clipping / linear); Instrument level;
  Oversampling (2x, 4x); Output level; Pot taper (log 15 % / log 10 % /
  linear; the A pots: volume and Q).
  Knob 10 = clockwise end.
  Defaults: volume 5, all other knobs at center, both EQs on, power amp
  clipping, 100 mV, 2x, 0 dB.

User interface
  Front panel drawn after the drawing in the service manual (no photo, no
  logo): knobs to turn (drag vertically, mouse wheel, double-click =
  center), the indicator lamp of each PARAMETRIC EQ field as a click target
  to switch that EQ on (green) or off - on the amp this is done with the
  footswitch. The three CROSSOVER knobs only feed the LPF/HPF OUTPUT jacks
  for external power amps and are dimmed. Below the panel the model strip
  (not on the amp): OVERSAMPLING, POWER AMP (CLIP / LINEAR), POT TAPER, the
  DRIVE meters (input stage, volume + tone stages, EQ 1, EQ 2 relative to
  their limits, 100 % = clipping; output power into 8 ohm, mark = 100 W),
  INPUT (instrument level) and OUTPUT. Below that the small-signal frequency
  response at the operating point (relative to 1 kHz; dashed: volume 5,
  tone and EQ flat). Window resizable. Needs libcairo2 and libX11.

Levels
  INPUT sets what peak voltage at the jack corresponds to 0 dBFS of the
  track (default 100 mV; passive basses about 100-300 mV, active basses up
  to 1 V and more). The output is the voltage at the speaker terminals,
  50 V peak = 0 dBFS (100 W into 8 ohm is 40 V peak).

Oversampling and latency
  2x (default) or 4x. There is no 1x: at 44.1/48 kHz the solver
  occasionally finds no solution when an EQ is driven into hard clipping
  (two clipping op-amps in one loop), so the circuit always runs at 88 kHz
  or more. 4x further reduces aliasing when the op-amps clip hard and costs
  twice the CPU of 2x. Changing it prepares a new engine in the background
  (the amp settles first) and switches over after a moment ("switching ..."
  on the toggle). The reported latency is always 31 samples (the
  oversampling filters).

CPU
  About a third to half of one core at 48 kHz and 2x on a typical desktop
  (the whole preamp with both EQs is solved every sample); stereo with
  different left and right signals doubles that. With the same signal on
  both channels it is computed once. After 2 s of silence at the input the
  plugin goes idle.

1.0
  - First release. No speaker.

Notes
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer).
  Yamaha is a trademark of Yamaha Corporation; this is an independent
  circuit simulation, not affiliated with or endorsed by them.
