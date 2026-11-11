AmpFett SVT-CL Bass Amp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.9

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
      sags with the signal; switchable to the full rectifier and filter
      chain of the schematic with 50 or 60 Hz mains (since 1.5)
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
  in the schematic show 5.5 V -> 30 V). 40 Hz is +0.5 dB instead of -3 dB
  with "BAL OUT load" off; with the estimated T1 on it is -2.8 dB.
  Estimated (not in the manual): inductance, leakage and losses of the
  output transformer, the inductor L1 (winding resistance, coupling),
  internal resistances of the power supply, which transformer tap feeds
  the negative feedback (4 ohm assumed), the 6550 model (fitted to data
  sheet points), the log taper of the A pots (15 % at center), the source
  impedance (1 kohm), the BAL OUT transformer T1 (10 H, 50 ohm, 20 kohm
  iron losses), the mains transformer source resistances. No speaker; the
  SLAVE / BALANCED outputs themselves are not available, only the load
  of T1 on the preamp output (switchable).

Parameters
  Gain, Bass, Midrange (0 ... 10); Frequency (220 Hz, 450 Hz, 800 Hz,
  1.6 kHz, 3 kHz); Treble, Master (0 ... 10); Ultra Hi (Off / On); Ultra
  Lo (Off / On); Input (0 dB / -15 dB jack); Output tap (4 ohm / 2 ohm);
  Instrument level (mV peak, 1 ... 1000); Input reference (peak of your
  playing / 0 dBFS); Output level (-36 ...
  +24 dB); Pot taper (A pots) (log 15 % / log 10 % / linear; GAIN, BASS,
  TREBLE); Power supply (Simple / As schematic, 50 Hz mains / As
  schematic, 60 Hz mains); BAL OUT load (T1) (Off / On (T1 estimated));
  Power tube mismatch (%) (0 ... 10).
  Knob 10 = clockwise end.
  Defaults: all knobs 5, Frequency 800 Hz, Ultra Hi and Ultra Lo off,
  0 dB input, 4 ohm tap, 100 mV, 0 dB, log 15 %, simple power supply,
  BAL OUT load off, tube mismatch 8 % (projects saved before 1.5 load
  with 0 %, so they sound as before).
  Changing Power supply or BAL OUT load rebuilds the circuit in the
  background (1-3 s, the old setting keeps playing until then).
  (The hidden parameter "Oversampling" is kept for old projects; since 1.1
  it is always 1x. The hidden parameters "Standby" and "Power" are kept for
  old projects as well; since 1.3 they have no effect.)

User interface
  Front panel drawn after the layout in the owner's manual (no photo, no
  logo; "AmpFett" lettering in Damion, SIL Open Font License): 0 dB and
  -15 dB INPUT jacks (click a jack to plug in there), GAIN, toggle
  switches ULTRA HI and ULTRA LO, BASS, MIDRANGE, FREQUENCY (five detented
  positions), TREBLE, MASTER (lever up = on). Since 1.3 without the right
  part of the panel (STANDBY / POWER / FAULT lamp, STANDBY and POWER
  switches); the panel is narrower and the default window 1200 wide. Knobs: drag vertically, mouse wheel, double-click = center. Below the
  panel the model strip (not on the amp): OUTPUT TAP (4 / 2 ohm), POT TAPER, POWER SUPPLY (SIMPLE / 50 HZ /
  60 HZ), BAL OUT (OFF / T1 EST.), the DRIVE meters (grid swing of V1B input, V1A gain, V2B
  midrange, power amp input, 6550 grids, 100 % = grid reaches 0 V; output
  power into the selected load, mark = 300 W), TUBE MISMATCH, INPUT
  (instrument level) and OUTPUT. Below that the small-signal frequency response at the operating
  point (relative to 1 kHz). Window resizable. Needs libcairo2 and libX11.

Levels
  INPUT sets the peak voltage of your playing at the jack (see "Input
  level" below) (default 100 mV; passive basses about 100-300 mV, active basses up
  to 1 V and more). The output is the voltage at the selected output
  transformer tap: 60 V peak = 0 dBFS into 4 ohm, 42.4 V peak = 0 dBFS
  into 2 ohm (the same power, 450 W, in both cases; 300 W into 4 ohm are
  49 V peak).

Oversampling and latency
  Since 1.1 the amp runs at the host rate only (no oversampling): 2x and
  4x needed more than one CPU core. Aliasing stays around -85 dB for bass
  fundamentals even with heavy overdrive (measured at 83 and 220 Hz, gain
  8, master 6); only notes above about 1 kHz driven hard fold back audibly.
  The reported latency is 31 samples, as before (the plugin delays by that
  amount so projects from 1.0 keep their timing).

CPU
  About 1.5 times the AmpFett B-15NF: roughly 40-75 % of one core at
  48 kHz and 1x on a typical desktop (the whole amp with six output tubes
  and the power supply is solved every sample; 1.1 solves the large power
  amp system about 10 % faster); stereo with different left and right
  signals doubles that. With
  the same signal on both channels it is computed once.

Input level (INPUT, since 1.6)
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

1.8
  - Turning knobs costs less CPU: when only resistor values change (pots),
    the circuit matrix is updated with rank-one corrections instead of being
    inverted again; a full inversion every 16 updates keeps the rounding in
    check. Load while turning three knobs: about 0.58 -> 0.46 of a core
    (block 128). Sound with fixed settings bit-identical; while turning the
    difference to a full inversion is below -80 dB.
  - Switches no longer click: MIDRANGE frequency, ULTRA HI, ULTRA LO, input
    jack, output tap and the pot taper used to switch the circuit from one
    sample to the next, which made the output jump. Now the output fades out
    over 3 ms, the switch happens in the silent spot, and after the plugin
    latency (31 samples) it fades in over 4 ms. Measured with a sine: the
    click is down from up to -1 dB to -75 dB for most switches; ULTRA HI,
    ULTRA LO, the input jack and the log 10 % taper leave a short ringing
    near 18 kHz at about -40 to -47 dB. Oversampling, power supply and BAL
    OUT load keep the core swap of 1.7.

1.7
  - No more dropout and click when the circuit is switched (oversampling, power supply, BAL OUT load):
    the old circuit fades out over 3 ms, the new one fades in, and the old
    one is freed outside the audio thread. Before, the switch could take
    long enough for a dropout at small buffers (128 samples) and the output
    jumped.
  - Restart after a pause (Play, FX switched on) costs about a third less
    time in its first block: only the left-channel amp is set up, the right
    one is copied from it when the channels differ.
  - Restart without "plop": the resting state the plugin restarts from now
    sits on the current knob settings. Before it was the default setting,
    and at every Play the circuit jumped to the project settings and settled
    audibly (SVT-CL up to -31 dBFS, B-15NF -38 dBFS for about 50 ms). When
    the settings change and then stay for 0.5 s, a background thread settles
    a new resting state (about 0.5 s computing time). Until it is ready (for
    example right after loading a project) a restart fades in over 150 ms
    instead.

1.6
  - INPUT now means the peak voltage of your playing: the plugin remembers
    the highest input peak and scales to it (see "Input level"). Before, it
    was the voltage at 0 dBFS of the track, so a track peaking at -12 dBFS
    reached the amp with only a quarter of the set voltage. Old projects
    keep the old behaviour and sound bit-identical.

1.5
  - New switch "Power supply": "As schematic, 50 Hz / 60 Hz" computes the
    rectifiers from the mains transformer and the whole filter chain with
    the part values of the schematic (C17/C18, R54, R58, C20, R55, C19,
    bleeders, C23, bias divider with the BIAS pot set for about 24 mA per
    6550). Ripple and sag follow from the circuit: idle hum at the output
    about 0.1 mV (50 Hz) / 2 mV (60 Hz), at hard overdrive the output
    power drops to about 295 W (simple supply about 508 W). CPU about
    +25 to +50 %. Note: with the schematic resistor values the +470 V and
    +345 V lines come out at about 537 V and 397 V, not at the voltages
    printed in the schematic (the preamp line gets 348 V, schematic 343 V);
    the model keeps the part values. "Simple" is the supply of 1.0-1.4.
  - New switch "BAL OUT load (T1)": the transformer of the balanced output
    (R91 220 ohm, T1 primary) loads the preamp output, as in the amp when
    nothing is plugged into BAL OUT. The values of T1 are estimated (10 H,
    50 ohm, 20 kohm). With it the whole amp is -2.8 dB at 40 Hz (spec
    -3 dB) with a lift of about +5.7 dB around 60 Hz (series resonance with
    C26); 1 kHz changes by -0.1 dB. Off by default.
  - New knob "TUBE MISMATCH" (0 ... 10 %, default 8 %), as in the Laney
    and the AmpFett B-15NF: the three 6550 of one side draw half the value
    more idle current, the other side half less (unmatched or unevenly
    aged tubes). Direct current in the output transformer and more even
    harmonics (2nd harmonic at small level about -39 dB instead of -45 dB
    at 8 %). Changes glide over 0.3 s.
  - R80 (1 Mohm, PREAMP OUT buffer) is now on the preamp output, as in the
    schematic. This changes the sound by about -55 dB (low-frequency
    corner of C26 and the following phase); everything else is unchanged
    with both new switches off and tube mismatch at 0 %.

1.4
  - Faster solver for the power amp block (23 coupled equations: valve
    V2A, both 6550 sides, output transformer, power supply): the Newton
    matrix is built per device block, and within one sample its
    factorisation is reused while the error drops fast enough. About
    15 % less CPU at normal levels, about 20 % less with much GAIN and hard
    attacks. The circuit is unchanged; output differs only within the
    solver tolerance (below -100 dB at normal settings). At extreme
    settings (GAIN, BASS, TREBLE 10, ULTRA on) the amp itself reacts
    chaotically: there an input change of -120 dB already changes the
    output by about -35 dB, so 1.3 and 1.4 differ by the same amount.

1.3
  - Front panel narrower: the right part with the STANDBY / POWER / FAULT
    lamp and the STANDBY and POWER switches is gone. The two parameters
    stay hidden so old projects load; they no longer mute. Default window
    1200 wide. Sound unchanged (bit-identical to 1.2).

1.2
  - Hard attacks with much GAIN: when the solver does not converge at once
    it now takes damped steps right away instead of falling back to a slow
    emergency path (up to 240 steps for one sample). That emergency path
    caused single audio blocks to take more than twice the buffer time,
    heard as a crackle on every note with small buffers (64 samples).
    Faster matrix products as well. Sound unchanged apart from solver
    tolerance (difference below -100 dB).
  - Fixed: after inserting or enabling the plugin on a record-armed track
    it stayed silent for live input (meters dead) until Play was pressed
    once. The idle mode after silence is now internal only; the plugin no
    longer asks the host to stop calling it.

1.1
  - Oversampling removed (only 1x): 2x/4x were not usable in real time.
    Solver for the power amp about 10 % faster. Sound at 1x unchanged.
  - Fixed: REAPER crashed when the mouse pointer rested on an input jack
    (tooltip read past the end of the switch list).

1.0
  - First release. No speaker.

VALUE POPUP (new in 1.9)
  In the header line of the model strip, "VALUE POPUP ON / OFF" switches the
  small value display that appears at a knob while the mouse is over it or
  while turning. Click to toggle. The choice is stored for all tonestack
  plugins of this user (Linux: ~/.config/tonestack/ui.conf, macOS:
  ~/Library/Application Support/tonestack/ui.conf), not per project.
  No change in sound.

Notes
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer).
  AmpFett is a rebuild of the Ampeg SVT-CL. Ampeg is a trademark of its
  owner (LOUD / Yamaha Guitar Group); this is an independent circuit
  simulation, not affiliated with or endorsed by them.
