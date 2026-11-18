Loldano SLO-100 Guitar Amp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.8

Two formats, one engine
  Loldano_SLO100.clap and Loldano_SLO100.vst3 contain the same code and
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
  mkdir -p ~/.clap && cp Loldano_SLO100.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Loldano SLO-100 Guitar Amp".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Loldano_SLO100.vst3 ~/.vst3/
  (Loldano_SLO100.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  Loldano SLO-100 is a rebuild of the Soldano SLO-100 (Super Lead
  Overdrive), the 100 W two-channel tube guitar head, after its schematic.
  The whole amp is solved sample by sample as one circuit (nodal DK method,
  like the other tonestack amps; 72 nodes, 51 states, 26 nonlinear ports),
  from the input jack to a speaker as the load:
    - input stage V1B (12AX7)
    - OVERDRIVE channel: PREAMP OVERDRIVE (500 k linear), V2A, the "cold"
      stage V2B (39 k cathode resistor, no bypass capacitor)
    - NORMAL channel: CLEAN/CRUNCH attenuator, BRIGHT (470 pF), PREAMP
      NORMAL (500 k linear), V1A
    - channel switching with photo resistors (as resistors: 600 ohm on,
      50 Mohm off); the channel that is not selected stays connected and
      bleeds through a little, as in the amp
    - V3B with cathode follower V3A, the effects loop divider (100 k /
      1 k, loop closed), V4B with cathode follower V4A
    - tone stack: TREBLE (250 k linear), BASS (1 M log), MIDDLE (25 k
      linear); MASTER NORMAL and MASTER OVERDRIVE (1 M log)
    - long-tailed phase inverter V5 (12AX7) with the negative feedback
      (39 k from the 4 ohm tap) and PRESENCE (25 k linear)
    - four 6L6GC in push-pull (two in parallel per side), fixed bias,
      output transformer 2.2 kohm plate to plate
    - power supply with the filter chain of the schematic (200 uF, choke,
      100 uF, 10 k / 40 uF, 15 k / 10 uF twice); it sags with the signal
    - a 4x12 closed cabinet as the electrical load (see "Cabinet and
      SPEAKER"): it shapes the response of the power amp
  The 12AX7 capacitances (1.6 pF grid-cathode, 1.7 pF grid-plate) are real
  capacitors between the electrodes, so the Miller effect comes out of the
  circuit (left out at the two cathode followers and the second half of
  the phase inverter, where they change the response by less than
  0.03 dB).
  Checked against the schematic (engine/tests/slotest.cpp): supply
  voltages B+1 497.0 V (schematic 497; calibrated), B+2 495.1 V (495),
  B+3 379.7 V (378), B+4 361.3 V (350), B+5 352.8 V (359; B+4 and B+5 are
  probably swapped in the schematic); power at 5 % THD 101 W (a 100 W
  amp). The schematic gives no operating points, gains or frequency
  responses, and no measurement of a real amp was available: the model
  follows the part values; a comparison with the sound of a real SLO-100
  has not been made.
  By default the output is the voltage at the speaker terminals. SPEAKER
  (from 1.5) adds the sound pressure of a Celestion Vintage 30 after its
  data sheet curve; room and microphone are not part of it. For a finished
  guitar sound an impulse response (cabinet with microphone) behind it,
  with SPEAKER off, is the alternative.

Cabinet and SPEAKER
  The cabinet is assumed (it is not in the schematic): a closed 4x12 with
  four 8 ohm speakers, two in series and both pairs in parallel, 8 ohm in
  all. Wired like that the whole cabinet has the same impedance curve as
  one speaker, so the circuit computes it as one: voice coil 6.4 ohm and
  0.6 mH (eddy current loss 30 ohm in parallel), the motion as a resonant
  circuit with the resonance in the closed box at 110 Hz (Qms 6, Qes 1.0;
  estimated). The cabinet is always the load of the output transformer:
  power amp and negative feedback see the impedance peak at the resonance
  and the rise towards the treble, and the voltage at the terminals rises
  there (at the default settings about 5 dB at 110 Hz and a good 3 dB at
  80 and 150 Hz against a load without resonance). This is part of the
  sound with SPEAKER off.
  SPEAKER (from 1.5) adds the sound pressure: a filter on the terminal
  voltage, fitted to the frequency response curve in the data sheet of
  the Celestion Vintage 30 (8 ohm; read off at 46 points from 20 Hz to
  20 kHz and taken as sound pressure per volt at the terminals, as such
  curves are usually measured; within 1.4 dB up to 7 kHz). Re 7.3 ohm and
  Fs 75 Hz come from the data sheet, the other quantities of the filter
  are fitting values. The curve is that of one speaker in the maker's
  test setup; the comb filtering of four speakers, the room and the
  microphone are not computed. Equal level at 1 kHz.

Signal tap
  TAP AMP (default): the whole amp up to the speaker terminals.
  TAP PREAMP (from 1.5): the signal at the grid of the phase inverter V5,
  after preamp, tone stack and MASTER; the input of the phase inverter
  (0.02 uF, 1 Mohm) stays connected as the load. Phase inverter, power
  amp, output transformer, cabinet and power supply are not computed:
  at 2x about half the CPU, at 4x about a quarter less (there the power
  amp runs at 2x anyway). The preamp supply is held at the idle values of
  the whole amp (495 V before the filter chain, phase inverter current
  2.9 mA): no sag. The level is matched so that at the default settings
  it equals the speaker terminals at 1 kHz (x 9.08). With SPEAKER the same
  Vintage 30 filter acts on this signal. Switching crossfades in 20 ms;
  the DRIVE meters for the phase inverter, the 6L6GC and the power show
  "-".
  TAP FX SEND (from 1.7): taps the same preamp network earlier, at the
  effects loop behind V3B and the cathode follower V3A (FX SEND after the
  100 k / 1 k divider; the loop is connected through), so before V4B, V4A,
  the tone stack and MASTER. This is where the second harmonic is highest:
  the following inverting stage V4B partly cancels it again (NORMAL clean,
  PREAMP 10, 400 Hz: k2 at V3B -36 dB at 100 mV and -33 dB at 140 mV,
  at the speaker -48 and -35 dB). Level matched like PREAMP (x 11.65);
  tone stack and MASTER have no effect here. Meant for comparisons with
  circuit simulations measured at V3B, and for the raw preamp sound.

BASS CK (from 1.8, a modification, not on the original)
  Switches the cathode bypass capacitors of V1B, V2A and V3B (1 uF each
  across 1.8 k, corner 88 Hz) to 3.9 uF (corner 23 Hz). Suggested by a
  reader for using the SLO-100 as a bass distortion: with 1 uF an electric
  bass sounds "too much like a circular saw". V2B (cold stage, 39 k, no
  capacitor) and V4B (25 uF) stay as they are; in the Normal channel V1B
  and V3B are affected. Small signal at the speaker, default settings,
  3.9 uF against 1 uF: Overdrive 41 Hz +8.3 dB, 82 Hz +8.9 dB, 220 Hz
  +3.5 dB, 400 Hz +1.2 dB; Normal 41 Hz +4.9 dB, 82 Hz +5.2 dB, 400 Hz
  +0.7 dB. A bass note (40 Hz with overtones) in the Overdrive channel:
  2 to 3 dB more below 130 Hz and about 5 dB less "saw" between 1 and
  5 kHz. The operating point is the same (the capacitor carries no DC);
  when switching, each capacitor keeps its voltage (like a pre-charged
  capacitor being switched in), so there is no click and no settling.

Parameters
  Preamp Normal, Preamp Overdrive, Bass, Middle, Treble, Master Normal,
  Master Overdrive, Presence (0 ... 10); Channel (Normal / Overdrive);
  Normal channel: Clean/Crunch; Normal channel: Bright (Normal / Bright);
  Instrument level (mV peak, 1 ... 5000); Oversampling (1x / 2x / 4x);
  Output level (-36 ... +24 dB); Input reference (peak of your playing /
  0 dBFS); Oversampling when rendering offline (same as live / 8x / 16x,
  from 1.2); Speaker (Celestion Vintage 30, data sheet curve) (Off / On)
  and Signal tap (speaker output / after the preamp / FX send) (from 1.5,
  FX send from 1.7); Cathode caps V1B, V2A, V3B (bass mod) (1 uF original /
  3.9 uF bass, from 1.8; projects from earlier versions load with 1 uF).
  Knob 10 = clockwise end (the amp's own scale reads 1 to 11).
  Defaults: all knobs 5, both masters 3, Overdrive channel, Clean, Bright
  off, 100 mV, 2x, 0 dB, rendering 8x, speaker off, whole amp, 1 uF.
  Each channel has its own Preamp and Master knob; Bright and Clean/Crunch
  act on the Normal channel only.

User interface
  Front panel drawn after the layout in the owner's manual (no photo, no
  logo; "Loldano" lettering in Damion, SIL Open Font License): INPUT,
  toggle switches BRIGHT, CRUNCH/CLEAN and the channel switch (lever up =
  BRIGHT, CRUNCH, OVERDRIVE), the eight knobs. Knobs: drag vertically,
  mouse wheel, double-click = default. Below the panel the model strip
  (not on the amp): OVERSAMPLING, RENDER (oversampling for offline
  rendering, from 1.2), SPEAKER (ON / OFF) and TAP (AMP / PREAMP / FX SEND)
  (from 1.5, FX SEND from 1.7), BASS CK (1 uF / 3.9 uF, from 1.8), the
  DRIVE meters (grid swing of V1B
  input, V2A overdrive or V1A normal, V3B, V4B, the phase inverter and the
  6L6GC grids, 100 % = grid reaches the cathode voltage; output power =
  rms voltage squared / 8 ohm, mark = 100 W), INPUT (instrument level) and
  OUTPUT. Below that the small-signal frequency response at the operating
  point (relative to 1 kHz). Window resizable. Needs libcairo2 and libX11.

Levels
  INPUT sets the peak voltage of your playing at the jack (see "Input
  level" below; default 100 mV). The output is the voltage at the speaker
  terminals: 100 V = 0 dBFS. 100 W sine into 8 ohm are 40 V peak
  (-8 dBFS); driven fully the power amp delivers a square wave of about
  +-46 V with switching spikes at the speaker inductance of up to about
  110 V (measured with all knobs at 10 and 1 V at the input).
  Very much gain: in the Overdrive channel with Preamp at 5, 1 mV at the
  input already gives clear distortion (9 V at the speaker with 24 % THD
  at Master 3). Up to about Master 3 the power amp stays clean.

Oversampling and latency
  Distortion creates harmonics above half the sample rate that fold back
  as inharmonic tones (aliasing). Measured at 48 kHz with a 1.3 kHz tone,
  Overdrive, Preamp 6: 1x -18 dB, 2x -37 dB, 4x -47 dB below the signal.
  The most audible part lies below the note played (nothing masks it):
  with a 1 kHz tone, Overdrive, Preamp 5, 100 mV: 2x -59 dB, 4x -68 dB,
  8x -87 dB, 16x -95 dB. More oversampling only removes aliasing; the
  harmonics stay the same (2x against 16x: within 0.4 dB).
  1x is for slow machines only; 2x is the default. 4x (from 1.3): the
  preamp runs at 4x, the phase inverter and power amp at 2x (see 1.3
  below); it costs about a fifth more than 2x. With the power amp driven
  hard (MASTER high) its aliasing is that of 2x.
  RENDER (from 1.2): when the host renders offline (CLAP render mode,
  VST3 offline processing mode), the amp is rebuilt with 8x or 16x for
  the render and the live setting comes back afterwards. "As live" keeps
  the live setting. Whether a host reports offline rendering is up to the
  host; while it does, the RENDER switch reads "rendering now" (open the
  plugin window during a render to check).
  Measured here: 2x 1.1 times real time, 8x 2.4 times, 16x 4.1 times (so
  a 3-minute song takes about 7 or 12 minutes to render). The switch to
  the render oversampling takes about 3 seconds at the start of the
  render. The Audio Unit on the Mac does not get this information from
  the host and always renders with the live setting.
  Latency 50 samples in every mode (reported to the host; 31 up to 1.1).
  From 1.2 the output filter is flat to 20 kHz (before: -0.7 dB at
  20 kHz) and blocks from 24 kHz on by at least 84 dB (before: -22 dB at
  24 kHz), so nothing between 24 and 26 kHz folds back to 22-24 kHz any
  more. That costs the 19 extra samples of latency.
  Changing the oversampling while playing rebuilds the circuit in the
  background (about 1 s; the old setting keeps playing until then).

CPU
  The whole amp is solved every sample, and that is expensive: at 48 kHz
  and 1x about as much as the AmpFett SVT-CL, at 2x (the default) twice to
  nearly three times that (measured on the build machine: SVT-CL 32 % of
  one core idle and 46 % playing, Loldano 1.1 at 2x 55 % and 123 %). Hard
  overdrive costs most, because the solver needs more steps per sample.
  From 1.3, measured here, driven hard: 2x 0.95 times real time (1.2:
  1.11), 4x 1.15 (1.2: 1.56).
  If it crackles while playing, set OVERSAMPLING to 1x; rendering uses the
  RENDER setting.
  Stereo with different left and right signals doubles the load. With the same
  signal on both channels it is computed once. After 2 s of silence at the
  input the circuit is paused.

Input level (INPUT)
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
  Typical values: single-coil guitar about 100-200 mV on a hard attack,
  humbucker or active pickups up to 1 V. Default 100 mV.
  INPUT goes up to 5000 mV (until the versions of November 2026: 1000 mV),
  enough for active instruments and for driving the first stage hot. The
  value is also shown in dBu, as the rms level of a sine with this peak
  (0 dBu = 0.775 V rms): 100 mV = -20.8 dBu, 1000 mV = -0.8 dBu,
  5000 mV = +13.2 dBu. The convention of audio interfaces (0 dBFS = about
  1 V) plays no role here: the track only supplies numbers, INPUT sets the
  voltage at the amp.
  With "Input reference" set to 0 dBFS, INPUT is the voltage at 0 dBFS of
  the track instead.

VALUE POPUP (removed in 1.8)
  The value popup at the knobs and its switch in the header line of the
  model strip are gone (since 1.8).

Estimated (not in the schematic)
  Output transformer inductance, leakage, capacitance and losses; internal
  resistance of the power supply and of the choke; idle current of the
  6L6GC (35 mA each); the photo resistors (data sheet values, without
  their inertia); the speaker data; the taper of the log pots (15 % at
  mid position); the tube models (12AX7 after Dempwolf with measured grid
  current; 6L6GC from curve-tracer measurements by D. Reefman, grid
  current from the RCA data sheet).
  Not modelled: noise and hum, the inertia of the photo resistors,
  external devices in the effects loop, the SLAVE output, a DEPTH control
  (the original of 1987 has none).

Notes
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer).
  Loldano is a rebuild of the Soldano SLO-100 after an annotated redrawn
  schematic by R. Robinette (robrobinette.com) and the circuit analysis by
  R. Kuehnel (ampbooks.com). "Soldano" and "SLO" are trademarks of their
  owner; this is an independent circuit simulation, not affiliated with or
  endorsed by them.

1.0
  First version (built, not distributed).

1.1
  Looked for avoidable load, counted in instructions per sample: 7 % less
  when driven hard, 11 % less when idle.
  - The tube capacitances of the two cathode followers and of the second
    half of the phase inverter are gone (six states; response changes by
    at most 0.03 dB up to 20 kHz, the six gain stages and the phase
    inverter input keep theirs: without those the response would change
    by 0.2 dB at 10 kHz).
  - The rectifier is no longer a nonlinear element: behind a DC source it
    always conducts, so it is a threshold voltage and a resistance now.
  - BRIGHT uses one capacitor that the switch moves instead of two.
  - Open switch contacts are 100 Gohm instead of 1 Gohm: with 1 Gohm the
    undivided signal leaked past the CLEAN attenuator by 0.05 dB.
  Tried and dropped because they did not help: solving the tubes of the
  channel that is not selected separately, reusing the solver matrix
  across samples, sparse matrix products.
  Where the time goes now (driven hard): about half in the Newton solver
  (matrix build, factorisation, updates), a quarter in the tube curves,
  an eighth in the linear part. These parts are shared with the other
  tonestack amps; making them faster is a change to all of them.

1.2
  Less aliasing where it costs nothing while playing.
  - RENDER: offline renders use 8x (default) or 16x oversampling, see
    "Oversampling and latency". New parameter at the end of the list, old
    projects keep their parameter numbers and get 8x.
  - Output filter: flat to 20 kHz, at least 84 dB down from 24 kHz on
    (Kaiser window instead of Blackman). Below 18 kHz the sound is
    unchanged (measured: harmonics identical to 0.01 dB). Latency 50
    instead of 31 samples.
  - Web page: the WAV download renders with 8x instead of 4x.
  Looked at and not done for now (measured, little gain for the effort):
  running the preamp faster than the power amp (saves only 10 to 20 %
  against full oversampling, because the preamp takes 55 % of the time),
  a faster solver (at most 10 to 20 %, shared with all other amps).

1.3
  Less CPU, so that 4x becomes usable while playing.
  - 4x: the circuit is split at the cathode follower V4A, which drives the
    tone stack from a low impedance. The preamp (V1 to V4, with the tone
    stack, masters and phase inverter input as its load) runs at 4x, the
    tone stack, phase inverter, power amp, speaker and power supply at 2x.
    Between them a low-pass at 46 kHz; the supply voltages B2/B3 and the
    currents between the parts are exchanged every power-amp step (one
    step late, the filter capacitors make that negligible).
    Measured against the whole circuit: at equal rates the split changes
    harmonics by at most 0.15 dB; at 4x/2x the aliasing below the note at
    1 kHz is -68 dB (whole circuit at 4x: -70 dB, 2x: -59 dB), harmonics
    within 0.2 dB of 8x at Master 3. With MASTER 10 the power amp clips
    hard: then aliasing and harmonics are those of 2x (within 1.2 dB).
    CPU at 4x: about a quarter less than 1.2.
  - Solver shortcuts (only in this plugin; the other amps are unchanged,
    byte for byte): exp(-Ua) is skipped where it cannot change the result,
    grid current below -2 V counts as zero (under 0.2 nV across 1 Mohm),
    and the second pass between preamp and power amp accepts a remaining
    error below 0.1 mV. Together 9 % fewer instructions at 2x driven hard;
    output differs from 1.2 by at most -89 dB (the size of the solver's own
    tolerance).
  - Rendering at 8x and 16x keeps the whole circuit (the split with a
    slower power amp would raise its aliasing at high MASTER).

1.4
  - Fix: with OVERSAMPLING 4X (1.3) the circuit started with a thump of up
    to -5 dBFS lasting about 150 ms, at load and at every switch to 4X: the
    power amp part settled with 0 V instead of the cathode voltage of V4A
    and the filter between the two parts started empty. Now silent
    (-132 dBFS in the first 100 ms, as in 1.2).
  - Tube curves from a table (12AX7 and grid current, also in the 6L6GC
    grid; step 1/32, cubic, error below 1e-8 relative).
  - The solver keeps its factorised matrix across samples while the
    circuit is quiet (each part converged in one Newton step).
  Measured against 1.3 (instructions, 0.5 s): driven 2x -12 %, 4x -13 %,
  plucked note 2x -27 %, 4x -38 %, silence 2x -31 %, 4x -40 %; time on the
  build machine driven 2x -19 %, 4x -22 % (4x now in real time there).
  Deviation from a reference with a much tighter solver tolerance: at
  most -106 dBFS, as before.

1.5
  New parameters "Speaker" (SPEAKER toggle: the sound pressure of a
  Celestion Vintage 30 after its data sheet curve) and "Signal tap" (TAP
  toggle: the signal after the preamp without computing the power amp),
  see "Cabinet and SPEAKER" and "Signal tap". Appended at the end; projects
  saved before load without speaker and with the whole amp, as before.
  Sound with SPEAKER off and TAP AMP unchanged (bit for bit).

1.6
  INPUT goes up to 5000 mV peak (was 1000 mV), for active instruments and
  for driving the first stage hot; the value is also shown in dBu (see the
  paragraph on INPUT). The knob covers the new range, so 100 mV now sits a bit
  further left. Saved projects keep their value, and the sound at the
  same INPUT value is unchanged (bit-identical). VST3 only: hosts store
  automation as a fraction of the parameter range, so INPUT automation
  recorded with an earlier version plays back at a different voltage;
  redraw it if you used it.

1.7
  TAP has a third position, FX SEND: the signal at the effects loop
  behind V3B and V3A, before V4B and the tone stack (see "Signal tap").
  The parameter "Signal tap" now has three values (0 speaker output,
  1 after the preamp, 2 FX send); saved projects keep their setting.
  VST3 only: automation of TAP recorded with an earlier version plays
  back differently (stepped values are stored as a fraction of the
  range).
  Fixed: a double-click on INPUT set 292 mV instead of the default
  100 mV (since 1.6).
  New test engine/tests/slothd: k2/k3 at V3B, V4A and the speaker
  (NORMAL clean, PREAMP 10, 100 and 140 mV, 400 Hz) must stay within
  0.3 dB of the values measured on 16 November 2026.

1.8
  New switch BASS CK in the model strip: cathode capacitors of V1B, V2A
  and V3B 3.9 uF instead of 1 uF, for bass (see "BASS CK"). New parameter
  "Cathode caps V1B, V2A, V3B (bass mod)", appended at the end; saved
  projects load with 1 uF and sound as before. With 1 uF the amp computes
  exactly as 1.7. The DRIVE meters moved a little to the right.
  New test engine/tests/slobassck (frequency response, operating point,
  switching without a click).
  The value popup at the knobs is gone (it appeared while turning a knob
  or with the mouse over a control); with it the switch VALUE POPUP in the
  header line of the model strip. Sound unchanged.
