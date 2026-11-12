Loldano SLO-100 Guitar Amp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.0

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
  like the other tonestack amps; 74 nodes, 58 states, 27 nonlinear ports),
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
    - a speaker as the electrical load (6.4 ohm voice coil, 0.6 mH,
      resonance 110 Hz): it shapes the response of the power amp, it is
      not a cabinet simulation
  The 12AX7 capacitances (1.6 pF grid-cathode, 1.7 pF grid-plate) are real
  capacitors between the electrodes, so the Miller effect comes out of the
  circuit.
  Checked against the schematic (engine/tests/slotest.cpp): supply
  voltages B+1 497.0 V (schematic 497; calibrated), B+2 495.1 V (495),
  B+3 379.7 V (378), B+4 361.3 V (350), B+5 352.8 V (359; B+4 and B+5 are
  probably swapped in the schematic); power at 5 % THD 102 W (a 100 W
  amp). The schematic gives no operating points, gains or frequency
  responses, and no measurement of a real amp was available: the model
  follows the part values; a comparison with the sound of a real SLO-100
  has not been made.
  No cabinet: the output is the voltage at the speaker terminals. For a
  finished guitar sound put a cabinet simulation (impulse response) behind
  it.

Parameters
  Preamp Normal, Preamp Overdrive, Bass, Middle, Treble, Master Normal,
  Master Overdrive, Presence (0 ... 10); Channel (Normal / Overdrive);
  Normal channel: Clean/Crunch; Normal channel: Bright (Normal / Bright);
  Instrument level (mV peak, 1 ... 1000); Oversampling (1x / 2x / 4x);
  Output level (-36 ... +24 dB); Input reference (peak of your playing /
  0 dBFS).
  Knob 10 = clockwise end (the amp's own scale reads 1 to 11).
  Defaults: all knobs 5, both masters 3, Overdrive channel, Clean, Bright
  off, 100 mV, 2x, 0 dB.
  Each channel has its own Preamp and Master knob; Bright and Clean/Crunch
  act on the Normal channel only.

User interface
  Front panel drawn after the layout in the owner's manual (no photo, no
  logo; "Loldano" lettering in Damion, SIL Open Font License): INPUT,
  toggle switches BRIGHT, CRUNCH/CLEAN and the channel switch (lever up =
  BRIGHT, CRUNCH, OVERDRIVE), the eight knobs. Knobs: drag vertically,
  mouse wheel, double-click = default. Below the panel the model strip
  (not on the amp): OVERSAMPLING, the DRIVE meters (grid swing of V1B
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
  1x is for slow machines only; 2x is the default; 4x needs more than one
  core in real time on most machines and is meant for rendering. Latency
  31 samples in every mode (reported to the host). Changing the
  oversampling rebuilds the circuit in the background (about 1 s; the old
  setting keeps playing until then).

CPU
  The whole amp is solved every sample, and that is expensive: at 48 kHz
  and 1x about as much as the AmpFett SVT-CL, at 2x (the default) twice to
  nearly three times that (measured on the build machine: SVT-CL 32 % of
  one core idle and 46 % playing, Loldano at 2x 55 % and 123 %). Hard
  overdrive costs most, because the solver needs more steps per sample.
  If it crackles while playing, set OVERSAMPLING to 1x and render with 4x.
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
  With "Input reference" set to 0 dBFS, INPUT is the voltage at 0 dBFS of
  the track instead.

VALUE POPUP
  The small switch in the model strip turns the value display at the knobs
  on and off (stored for all tonestack plugins together).

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
  First version.
