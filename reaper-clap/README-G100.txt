Lametta G100-212II Guitar Amp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.12

Two formats, one engine
  Lametta_G100.clap and Lametta_G100.vst3 contain the same code and sound
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
  mkdir -p ~/.clap && cp Lametta_G100.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Lametta G100-212II Guitar Amp".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Lametta_G100.vst3 ~/.vst3/
  (Lametta_G100.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  Lametta G100-212II is a rebuild of the Yamaha G100-212II guitar amplifier after the service
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
  Nonlinear tank (1.2, switch REVERB TANK, default PHYSICAL): the state of
  the already vibrating spring changes how the next note acts - a rimshot
  sets the springs ringing, and a second hit 50-200 ms later lands on a
  moving spring and sounds different from the same hit alone. Three
  mechanisms, all with estimated values (no tank was measured):
    - transducers: soft core saturation of the drive coil (80 mA peak) and
      a coupling of each magnet bead that drops with its momentary
      displacement, at both ends of the springs
    - springs: stiffer (up to 3 % faster), more damped (loop gain up to
      10 % lower) and softly limited at large twist, following the envelope
      of the spring motion (1 ms attack, 20 ms release)
    - back-EMF: the moving beads at the drive end induce a voltage in series
      with the drive coil (only the deviation from the nominal coupling;
      the linear part is already in the calibrated tank)
  Since 1.3 the scale of the bead displacement and the spring twist is the
  rated peak current of the tank (19 mA; 1.2: 80 mA, which made the
  difference inaudible at normal levels). Difference to the linear tank,
  plucked notes, REVERB 10, relative to the reverb itself: channel A,
  volume 5, 100 mV -24 dB; channel B, gain 10, master 5 -10 dB; channel
  A, volume 10 -2 dB. LINEAR 1.1 computes the tank exactly as in 1.1
  (bit-identical output).
  Speaker (since 1.10, SPEAKER switch, default off): the two Celestion
  G12T-75 (8 ohm, in parallel) of the combo in its open-back cabinet, fed by
  the power amp through the 2200 uF output capacitor. Based on the frequency
  response in the Celestion data sheet (read from the chart, about +-0.5 dB;
  the model follows it within 1.1 dB up to 7 kHz), Thiele-Small values of the
  8 ohm speaker (Fs 85 Hz, Re 6.8 ohm, Qms 10.6, Qes 0.63; from a user
  database, not from Celestion) and the cabinet size from the service manual
  (687 x 560 x 280 mm). The open back is estimated: the wave from the rear
  of the cones travels about 0.6 m around the cabinet, which removes bass
  below about 100 Hz and adds up to 3 dB around 200-600 Hz. The output
  capacitor and the speaker form a series circuit below resonance (+2 dB at
  40 Hz). Same level at 1 kHz as without the speaker. Not included: room,
  floor, microphone. Details: docs/g100/LAUTSPRECHER.md in the source.
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
  reverb tank as a whole (1.2: also its nonlinearity) and the +-17 V limit of
  the driver IC.

Parameters
  A: Volume, Treble, Middle, Bass, Pull Fat, Pull Bright; Channel (A/B);
  B: Gain, Master Volume, Treble, Middle, Bass, Pull Fat, Pull Bright;
  EQ: Level (-5 ... +5, 0 = flat), Q (0 = broad ... 10 = narrow),
  Frequency (0 ... 10 after the panel scale: 100 Hz, 200 at 2.8, 500 at 5,
  1 kHz at 7.2, 5 kHz at 10); Reverb (0 ... 10, linear pot, 1.1); Reverb tank nonlinearity
  (0 ... 10, 0 = linear as 1.1, 5 = as 1.3; 1.4, was a switch in 1.2/1.3); Input
  jack (High / Low); Power amp
  (clipping / linear); Speaker (off / on, 1.10); Instrument level; Input reference; Oversampling (1x, 2x, 4x);
  Output level; Pot taper (log 15 % / log 10 % / linear; the A pots:
  volume, gain, master, tone controls, Q).
  Knob 10 = clockwise end. Channel B with everything up is very hot: the
  diodes clip from a few millivolts at the jack - that is the amp.
  Defaults: all knobs 5, Level 0, Reverb 0, pull switches in, channel A,
  High jack, power amp clipping, speaker off, 100 mV, 2x, 0 dB, reverb tank 5.

User interface
  Front panel drawn after the drawing in the service manual (no photo, no
  logo): knobs to turn (drag vertically, mouse wheel, double-click = 5), the
  pull switches as a click on the "PULL FAT" / "PULL BRIGHT" label above the
  knob (pulled = light collar around the knob), the A/B button with its
  indicator (A green, B red), the HIGH and LOW jacks (click to plug in),
  REVERB. Below the panel the model strip (not on the
  amp): SPEAKER (ON / OFF, 1.10), OVERSAMPLING, POWER AMP (CLIP / LINEAR), POT
  TAPER, the DRIVE meters
  (input stage, channel A stages, channel B diodes, channel B FET and EQ
  stages relative to their limits, 100 % = clipping; output power into
  4 ohm, mark = 100 W; the meters of the channel not selected are dimmed),
  REVERB TANK (small knob 0 ... 10 since 1.4), INPUT (instrument level) and
  OUTPUT. Below that the small-signal frequency response at the operating
  point (relative to 1 kHz; with SPEAKER on the sound pressure; dashed: all
  knobs at 5, Level 0, channel A). Window resizable. Needs libcairo2 and libX11.

Levels
  INPUT sets the peak voltage of your playing at the jack (see "Input
  level" below) (default 100 mV; single coils about 100-200 mV, humbuckers and
  active pickups up to 1 V). The output is the voltage at the speaker
  terminals, 40 V peak = 0 dBFS (100 W into 4 ohm is 28 V peak); with
  SPEAKER on the sound pressure, scaled to the same level at 1 kHz.

Oversampling and latency
  1x is enough for clean and moderately driven playing; 2x (default) or 4x
  reduce aliasing when the diodes, the FET or the op-amps clip hard and cost
  two or four times the CPU. Changing it prepares a new engine in the
  background (the amp settles first) and switches over after a moment
  ("switching ..." on the toggle). The reported latency is always 31 samples
  (the oversampling filters; at 1x the plugin delays by the same amount), so
  it does not change with the setting.

CPU
  About 13 % of one core at 48 kHz and 1x on a typical desktop, about 30 %
  at 2x (1.5: about a third less than before) (both channels, the EQ and the reverb circuit are solved every
  sample, the spring model runs with them - also with REVERB at 0; 1.1
  needs almost twice the CPU of 1.0); stereo with different left and right
  signals doubles that. With the same
  signal on both channels it is computed once. After 2 s of silence at the
  input the plugin goes idle.

Input level (INPUT, since 1.7)
  INPUT is the peak voltage of your playing at the input jack. The plugin
  remembers the highest input peak it has seen and scales the track so
  that this peak arrives at the amp with the set voltage: a track that
  peaks at -12 dBFS and one that peaks at -3 dBFS drive the amp the same.
  The line below the INPUT knob shows the remembered peak ("at your peak
  (-11.7 dBFS)"); it is saved with the project, so the project sounds the
  same when opened again and when rendered. Click the line to forget it
  (e.g. after a louder or quieter take). It only grows: until the loudest
  note has been played once, earlier notes get more level; while it grows
  during the very first loud note, that note's rising edge is limited to
  the set voltage. Below -40 dBFS nothing is raised further.
  Typical values: passive bass about 100-200 mV on a hard attack, active
  bass up to 1 V; single-coil guitar about 100-200 mV, humbucker or active
  pickups up to 1 V. Default 100 mV.
  Projects saved with an earlier version keep the old meaning (INPUT = the
  voltage at 0 dBFS of the track) and sound as before; the line then reads
  "at 0 dBFS (old)", a click switches to the new behaviour (parameter
  "Input reference").

1.10
  - SPEAKER switch: the two Celestion G12T-75 in the open-back combo (see
    "What it is"). Default off; off, the plugin sounds bit-identical to 1.9
    (checked with 72 renders: four settings, block sizes 128 / random 256 /
    random 4096, 44.1 / 96 kHz, 1x / 2x / 4x). Measured with sines
    against the model: within 0.01 dB up to 2.3 kHz (0.07 dB at 6.5 kHz,
    2x). Switching it while playing crossfades over 20 ms (smooth curve):
    with a sine the residue above 4 kHz is about -95 dB relative to the
    signal. CPU of the speaker: about 0.5 % of a core at 2x.
  - The REVERB TANK knob moved to the right of the DRIVE meters to make room
    for the switch.

1.9
  - Turning knobs costs less CPU: when only resistor values change (pots),
    the circuit matrix is updated with rank-one corrections instead of being
    inverted again; a full inversion every 16 updates keeps the rounding in
    check. Load while turning three knobs: about 0.41 -> 0.35 of a core (2x,
    block 128). Sound with fixed settings bit-identical; while turning the
    difference to a full inversion is below -80 dB.
  - Switches no longer click: PULL FAT, PULL BRIGHT, channel A/B, input jack
    and the pot taper used to switch the circuit from one sample to the
    next, which made the output jump. Now the output fades out over 3 ms,
    the switch happens in the silent spot, and after the plugin latency (31
    samples) it fades in over 4 ms. Measured with a sine: the click is down
    from up to -9 dB to about -76 dB.

1.8
  - No more dropout and click when the circuit is switched (oversampling):
    the old circuit fades out over 3 ms, the new one fades in, and the old
    one is freed outside the audio thread. Before, the switch could take
    long enough for a dropout at small buffers (128 samples) and the output
    jumped.
  - Restart after a pause (Play, FX switched on) costs about a third less
    time in its first block: only the left-channel amp is set up, the right
    one is copied from it when the channels differ.

1.7
  - INPUT now means the peak voltage of your playing: the plugin remembers
    the highest input peak and scales to it (see "Input level"). Before, it
    was the voltage at 0 dBFS of the track, so a track peaking at -12 dBFS
    reached the amp with only a quarter of the set voltage. Old projects
    keep the old behaviour and sound bit-identical.

1.6
  - Rebuilt with the shared solver code of the SVT-CL 1.4 (matrix of the
    Newton step built per device block). Sound bit-identical to 1.5.

1.5
  - Faster solver (dense matrix products): about 35 % less CPU. Sound
    unchanged apart from rounding (difference below -240 dB).
  - Fixed: after inserting or enabling the plugin on a record-armed track
    it stayed silent for live input (meters dead) until Play was pressed
    once. The idle mode after silence is now internal only; the plugin no
    longer asks the host to stop calling it.

1.4
  - REVERB TANK is a knob 0 ... 10 instead of a switch (Nutzer: "large
    range"): 0 = linear (bit-identical to 1.1), 5 = as 1.3 (bit-identical),
    below 5 all scales grow (bead coupling and spring scale up to 300 mA,
    core saturation up to 320 mA: almost linear), above 5 the spring gets
    much more sensitive (scale down to 1.2 mA) with stronger stiffening
    (up to 12 % shorter delay) and damping (up to 30 %); bead coupling and
    limit only half as fast. Measured in the plugin (defaults, REVERB 10,
    plucked notes): difference to linear relative to the reverb -77 dB at
    1, -36 dB at 5, -8 dB at 7.5, +2.5 dB at 10 (reverb 1.8 dB quieter).
    Projects saved with 1.2/1.3 (switch values 0/1) load as 0 (linear) or
    1 (very mild).

1.3
  - Nonlinear tank audible at normal levels: scale of bead displacement
    and spring twist lowered to the rated peak current (19 mA instead of
    80 mA). LINEAR 1.1 unchanged (bit-identical to 1.1).

1.2
  - Spring tank nonlinear (transducers, springs, back-EMF), see "Nonlinear
    tank" above; new parameter Reverb tank (added at the end, default
    Physical - projects saved with 1.1 load with the physical tank; switch
    to "Linear (as in 1.1)" for the old sound, which is bit-identical).
  - CPU about the same as 1.1.

1.1
  - Spring reverb: driver, tank and return amplifier as part of the
    circuit, the springs as a physical model; new parameter Reverb (added
    at the end, projects saved with 1.0 load unchanged with Reverb 0).
  - About twice the CPU of 1.0.

1.0
  - First release. No speaker, no reverb.

VALUE POPUP (new in 1.11)
  In the header line of the model strip, "VALUE POPUP ON / OFF" switches the
  small value display that appears at a knob while the mouse is over it or
  while turning. Click to toggle. The choice is stored for all tonestack
  plugins of this user (Linux: ~/.config/tonestack/ui.conf, macOS:
  ~/Library/Application Support/tonestack/ui.conf), not per project.
  No change in sound.

Notes
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer).
  Yamaha is a trademark of Yamaha Corporation; this is an independent
  circuit simulation, not affiliated with or endorsed by them.

NEW NAME (1.12)
  The plugin is now called Lametta G100-212II (before: Yamaha G100-212II). It is a
  rebuild of the Yamaha G100-212II guitar amplifier after its service manual; Yamaha is a
  trademark of Yamaha Corporation, this is an independent simulation.
  No change in sound. File names and the plugin ID changed (Lametta_G100.clap / .vst3,
  de.tonestack.lametta-g100): projects that used the old plugin do not find it any more;
  insert the new one and set the controls again. Remove the old files
  Yamaha_*.clap / Yamaha_*.vst3 by hand if you installed them yourself
  (the update script and the Mac installer do it).
