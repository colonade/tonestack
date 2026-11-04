Yamaha G100-212II Guitar Amp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.0

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
    - the A/B relay, the load of the reverb driver, the parametric EQ (IC4
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
  No speaker and no reverb (the reverb comes later; its loads are in).
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
  amp; the log taper of the A pots; the source impedance (1 kohm).

Parameters
  A: Volume, Treble, Middle, Bass, Pull Fat, Pull Bright; Channel (A/B);
  B: Gain, Master Volume, Treble, Middle, Bass, Pull Fat, Pull Bright;
  EQ: Level (-5 ... +5, 0 = flat), Q (0 = broad ... 10 = narrow),
  Frequency (0 ... 10 after the panel scale: 100 Hz, 200 at 2.8, 500 at 5,
  1 kHz at 7.2, 5 kHz at 10); Input jack (High / Low); Power amp
  (clipping / linear); Instrument level; Oversampling (1x, 2x, 4x);
  Output level; Pot taper (log 15 % / log 10 % / linear; the A pots:
  volume, gain, master, tone controls, Q).
  Knob 10 = clockwise end. Channel B with everything up is very hot: the
  diodes clip from a few millivolts at the jack - that is the amp.
  Defaults: all knobs 5, Level 0, pull switches in, channel A, High jack,
  power amp clipping, 100 mV, 2x, 0 dB.

User interface
  Front panel drawn after the drawing in the service manual (no photo, no
  logo): knobs to turn (drag vertically, mouse wheel, double-click = 5), the
  pull switches as a click on the "PULL FAT" / "PULL BRIGHT" label above the
  knob (pulled = light collar around the knob), the A/B button with its
  indicator (A green, B red), the HIGH and LOW jacks (click to plug in),
  REVERB drawn but inactive. Below the panel the model strip (not on the
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
  About 10 % of one core at 48 kHz and 1x on a typical desktop, about 25 %
  at 2x and 65 % at 4x (both channels and the EQ are solved every sample);
  stereo with different left and right signals doubles that. With the same
  signal on both channels it is computed once. After 2 s of silence at the
  input the plugin goes idle.

1.0
  - First release. No speaker, no reverb.

Notes
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer).
  Yamaha is a trademark of Yamaha Corporation; this is an independent
  circuit simulation, not affiliated with or endorsed by them.
