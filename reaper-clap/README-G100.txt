Yamaha G100-212II Guitar Amp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.1

Two formats, one engine
  Yamaha_G100.clap and Yamaha_G100.vst3 contain the same code and sound
  identical. Use whichever your DAW prefers. CLAP is the native format, the
  VST3 is built from it with clap-wrapper (free-audio, MIT) and the VST3 SDK
  3.8 (Steinberg, MIT). A project saved with one format does not load its
  settings into the other.
  Third-party licenses: THIRD-PARTY-LICENSES.txt.

Updating
  Close REAPER first. Copying a new .clap over the file a running REAPER has
  loaded makes REAPER crash (at once or a while later). scripts/update-
  tonestack.sh refuses to run while REAPER is open and never overwrites a
  plugin file in place.

Installation (CLAP)
  mkdir -p ~/.clap && cp Yamaha_G100.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Yamaha G100-212II Guitar Amp".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Yamaha_G100.vst3 ~/.vst3/
  (Yamaha_G100.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  A rebuild of the Yamaha G100-212II guitar amplifier after the service
  manual (YAMU-02234, 1979: preamp schematic of the G100-115II/210/212II,
  power amp schematic, adjustment and performance-check tables). The whole
  preamp is solved sample by sample as one circuit (nodal DK method, like the
  other tonestack amps), both channels at once as in the amp:
    - input stage IC1 (gain 11, diodes between its inputs), HIGH (1 Mohm) or
      LOW (60 kohm, -12 dB) jack
    - the two tone stacks (Fender type, 2.7 nF / 10 k / 1 uF / 0.5 uF with
      TREBLE and BASS 25 k log, MIDDLE 1 k log) with PULL FAT (5.6 nF and
      330 k on the treble end) and their op-amp stages (gain 4.3)
    - channel A: VOLUME with PULL BRIGHT (5.6 nF to the wiper), op-amp stage
      gain 6.6
    - channel B: GAIN with PULL BRIGHT, op-amp stage gain 5.7, then 10 k into
      two silicon diodes to ground (clipping from about +-0.6 V), the 2SK30A
      FET stage and MASTER VOLUME; the mute FET that grounds channel B while
      A is selected is in the circuit too
    - the A/B relay, the spring reverb (1.1: driver IC3, tank, return
      amplifier IC7 and the REVERB pot, see below), the parametric EQ (IC4
      summing stages with the LEVEL pot between their summing points, state-
      variable filter IC5/IC6 with the FREQUENCY tandem pot and the Q pot),
      the output network and the input network of the power amp
  Op-amps (NJM4558) are modelled as voltage-controlled sources with finite
  gain, output resistance and supply clamps (+-13.5 V; IC1 +-11 V because of
  the resistors in its supply), so hard clipping of a stage sounds like the
  real thing. The power amp is a linear block with the shape of its feedback
  network (1.8 M against 100 k || 10 k + 1 uF: shoulder below 16 Hz), a soft
  clip at its limit (100 W into 4 ohm at 10 % THD = 28 V peak; knee 26 V,
  limit 31 V) and the 2200 uF output capacitor into 4 ohm (18 Hz).
  Reverb (1.1): the driver IC3 (TA7220P on +-20 V; 100 k against 2.2 k +
  2.2 n: gain 1, rising 6 dB/oct from 723 Hz to 46, so that the inductive
  drive coil gets a roughly flat current), 10 u and 56 ohm into the tank,
  the return amplifier IC7 1/2 (x101, 1 n, 220 p), 4.7 u, the REVERB pot
  (25 k linear), 2.2 k and 47 k into the output node - all in the same
  circuit as the preamp. The tank itself is a physical model of the
  springs (not a sampled impulse response): below about 4.2 kHz torsion
  waves travel dispersively - low frequencies arrive first, highs later -
  so every echo is an upward chirp that gets longer with each round trip;
  computed as a chain of 100 stretched all-pass filters in a feedback
  loop with the spring's delay (two springs, 33 and 37 ms), decay 2.75 s,
  a 6 kHz low-pass in the loop and a second, nearly non-dispersive path
  above 4.2 kHz with a shorter decay. The drive coil current is the
  model's input, the pickup coil's open-circuit voltage its output (28 ohm
  + 24 mH and 200 ohm + 0.357 H, Accutronics codes B/B - the manual only
  says "Reverb Unit", these fit the 56 ohm / 723 Hz driver). The tank's
  values are estimates (no tank was measured); REVERB 10 gives about as
  much reverb as direct signal on a chord. The footswitch is always on.
  No speaker.
  Checked against the manual: tables 3 (gain A HIGH / A LOW / B HIGH),
  6 (frequency response), 7 (TREBLE / MIDDLE / BASS range), 8 (FAT),
  9 (BRIGHT), 10 (parametric EQ: +-7, +-1, +-15 dB) and the curve of
  fig. 1-2 - all 29 values within the manual's tolerances
  (engine/tests/g100test.cpp).
  Estimated (not in the manual): the power amp gain is calibrated to table 3
  (105 x at 1 kHz; the feedback values alone would give 199 x, 5.6 dB more
  than the measured table); the 2SK30A of channel B (Vp -2 V, Idss 4 mA,
  chosen so that B is 13 dB above A as in table 3); the 1S1555 diode model;
  the on-resistance of the mute FET; the op-amp limits; the knee of the power
  amp; the log taper of the A pots; the source impedance (1 kohm); the
  reverb tank as a whole and the +-17 V limit of the driver IC.

Parameters
  A: Volume, Treble, Middle, Bass, Pull Fat, Pull Bright; Channel (A/B);
  B: Gain, Master Volume, Treble, Middle, Bass, Pull Fat, Pull Bright;
  EQ: Level (-5 ... +5, 0 = flat), Q (0 = broad ... 10 = narrow),
  Frequency (0 ... 10 after the panel scale: 100 Hz, 200 at 2.8, 500 at 5,
  1 kHz at 7.2, 5 kHz at 10); Reverb (0 ... 10, linear pot, 1.1); Input
  jack (High / Low); Power amp
  (clipping / linear); Instrument level; Oversampling (1x, 2x, 4x);
  Output level; Pot taper (log 15 % / log 10 % / linear; the A pots:
  volume, gain, master, tone controls, Q).
  Knob 10 = clockwise end. Channel B with everything up is very hot: the
  diodes clip from a few millivolts at the jack - that is the amp.
  Defaults: all knobs 5, Level 0, Reverb 0, pull switches in, channel A,
  High jack, power amp clipping, 100 mV, 2x, 0 dB.

User interface
  Front panel drawn after the drawing in the service manual (no photo, no
  logo): knobs to turn (drag vertically, mouse wheel, double-click = 5), the
  pull switches as a click on the "PULL FAT" / "PULL BRIGHT" label above the
  knob (pulled = light collar around the knob), the A/B button with its
  indicator (A green, B red), the HIGH and LOW jacks (click to plug in),
  REVERB. Below the panel the model strip (not on the
  amp): OVERSAMPLING, POWER AMP (CLIP / LINEAR), POT TAPER, the DRIVE meters
  (input stage, channel A stages, channel B diodes, channel B FET and EQ
  stages relative to their limits, 100 % = clipping; output power into
  4 ohm, mark = 100 W; the meters of the channel not selected are dimmed),
  INPUT (instrument level) and OUTPUT. Below that the small-signal frequency
  response at the operating point (relative to 1 kHz; dashed: all knobs at
  5, Level 0, channel A). Window resizable. Needs libcairo2 and libX11.

Levels
  INPUT sets what peak voltage at the jack corresponds to 0 dBFS of the
  track (default 100 mV; single coils about 100-200 mV, humbuckers and
  active pickups up to 1 V). The output is the voltage at the speaker
  terminals, 40 V peak = 0 dBFS (100 W into 4 ohm is 28 V peak).

Oversampling and latency
  1x is enough for clean and moderately driven playing; 2x (default) or 4x
  reduce aliasing when the diodes, the FET or the op-amps clip hard and cost
  two or four times the CPU. Changing it prepares a new engine in the
  background (the amp settles first) and switches over after a moment
  ("switching ..." on the toggle). The reported latency is always 31 samples
  (the oversampling filters; at 1x the plugin delays by the same amount), so
  it does not change with the setting.

CPU
  About 20 % of one core at 48 kHz and 1x on a typical desktop, about 45 %
  at 2x (both channels, the EQ and the reverb circuit are solved every
  sample, the spring model runs with them - also with REVERB at 0; 1.1
  needs almost twice the CPU of 1.0); stereo with different left and right
  signals doubles that. With the same
  signal on both channels it is computed once. After 2 s of silence at the
  input the plugin goes idle.

1.1
  - Spring reverb: driver, tank and return amplifier as part of the
    circuit, the springs as a physical model; new parameter Reverb (added
    at the end, projects saved with 1.0 load unchanged with Reverb 0).
  - About twice the CPU of 1.0.

1.0
  - First release. No speaker, no reverb.

Notes
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer).
  Yamaha is a trademark of Yamaha Corporation; this is an independent
  circuit simulation, not affiliated with or endorsed by them.
