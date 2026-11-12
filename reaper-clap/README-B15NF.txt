AmpFett B-15NF Bass Amp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.25

Two formats, one engine
  AmpFett_B15NF.clap and AmpFett_B15NF.vst3 contain the same code and sound
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
  mkdir -p ~/.clap && cp AmpFett_B15NF.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "AmpFett B-15NF Bass Amp".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r AmpFett_B15NF.vst3 ~/.vst3/
  (AmpFett_B15NF.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  AmpFett is a rebuild of the Ampeg B-15NF Portaflex: the whole tube amp, after the factory
  schematic of 1-66 and the Ampeg "Technical Data" book (1968), solved
  sample by sample as one circuit with all its tubes (nodal DK method):
    - inputs as on the amp: channel 1 BASS (120k, switching jack) and
      GUITAR (120k; the empty BASS jack loads it, -3.8 dB and less treble),
      channel 2 INST. (direct to the grid)
    - two 6SL7 preamps (unbypassed cathodes), volume 1 MOhm log, the
      Baxandall-type treble/bass network of the circuit board, both channels
      mixed into the grid of the phase inverter
    - 6SL7 floating paraphase phase inverter, negative feedback from the
      16-ohm tap into its cathode
    - two 6L6GC in push-pull with fixed bias (-50 V from the 10D6 bias
      supply), output transformer OT-214 (6.5 kOhm : 8/16 ohm) with core
      saturation
    - power supply: 5AR4 rectifier on the PT-108 (750 V CT), 30/40/40/40 uF
      filter chain, so the supply sags under load as on the real amp
  The idle voltages match the voltage table of the factory within the
  meter tolerance (anodes, cathodes, screens, bias, B+); full power is about
  30 W into 8 ohm. Same engine as the web simulator
  (https://borg.fluxstatus.de/tonestack/ampfett-b15nf/).
  Estimated (not in the schematic): transformer inductances and core
  saturation, mains transformer resistance, speaker data, screen grid
  current split of the 6L6GC. Tube models fitted to data sheets (6SL7 Dempwolf form,
  6L6GC Reefman pentode form). Details:
  docs/b15nf/SCHALTPLAN.md in the source.

Parameters
  Input jack (Bass / Guitar on channel 1, Inst. on channel 2), Volume 1,
  Treble 1, Bass 1, Volume 2, Treble 2, Bass 2 (0-10), Power, Speaker,
  Instrument level, Input reference, Oversampling (1x, 2x, 4x), Output level, Power tube
  mismatch (0-10 %), Power supply (simple / full), Mains frequency (full
  power supply only: 60 Hz US original / 50 Hz export), Volume pot taper
  (log 15 % / log 10 % / linear, see 1.16). Standby was
  removed in 1.15 (the parameter is still there for old projects, hidden
  and without effect).
  Knob 10 = clockwise end = boost (treble, bass) / full volume.
  Defaults: Bass jack, all knobs 5, power on, speaker off, 1x, tube
  mismatch 8 %, simple power supply. Projects saved before 1.15 load with
  the full power supply, so they sound as before.

User interface
  Front panel drawn after photos of the original (no photo, no logo):
  knobs to turn (drag vertically, mouse wheel, double-click = 5), click a
  jack to plug in there, click the POWER switch (STANDBY and GROUND were
  removed in 1.15). Below the panel the model strip (not on the amp):
  SPEAKER, OVERSAMPLING, SUPPLY (DC / 60 Hz / 50 Hz, see below), the DRIVE meters (grid swing of V1A/V2A, V1B/V2B, the phase
  inverter and the 6L6GC relative to their bias - 100 % = the grid reaches
  0 V, marked - and the output power into 8 ohm, mark = 30 W), INPUT
  (instrument level), OUTPUT and TUBE MISMATCH (idle current difference of the
  two 6L6GC). Below that the small-signal frequency
  response at the operating point (relative to 1 kHz; dashed: all knobs at
  5 with the same jack). Window resizable. Needs libcairo2 and libX11.

Levels
  INPUT sets the peak voltage of your playing at the jack (see "Input
  level" below) (default 100 mV; passive bass about 100-300 mV, active up to 1 V).
  The output is the voltage at the speaker terminal (or the sound pressure
  with SPEAKER on, equal at 1 kHz), 24 V peak = 0 dBFS. The amp clips softly
  above about 20 V peak.

Power supply and power
  SUPPLY DC (default since 1.15): the high voltage comes from a DC source
  behind an internal resistance and the 5AR4 rectifier tube, the bias from
  a DC source behind the bias divider's resistance. Fitted to the full
  model: 424 V at idle and 342 V on average at full drive (sag), bias
  -49.6 V; operating points, gain, frequency response and output power
  agree within 0.1 dB. No hum, about 20 % less CPU.
  SUPPLY 60 Hz / 50 Hz: the full power supply - mains transformer, 5AR4 on
  the AC voltage, filter capacitors - with hum (about 80 dB below full
  output) and the sag following the mains cycle. Switching between DC and
  the full supply prepares a new engine in the background like the
  oversampling switch.
  POWER off mutes the output (ramp of 20 ms).

Speaker
  SPEAKER adds the 15" speaker of the B-15 in its closed box (estimated
  Thiele-Small data: Fs 40 Hz, Qms 5, Qes 0.55, Vas 280 l, box 85 l), driven
  by the amp's real output impedance, plus a cone roll-off at 3 kHz.

Oversampling and latency
  1x is enough for clean and moderately driven playing; 2x or 4x reduce
  aliasing in heavy distortion and cost two or four times the CPU. Changing
  it prepares a new engine in the background (the amp settles first) and
  switches over after a moment ("switching ..." on the toggle). The
  reported latency is always 31 samples (the oversampling filters; at 1x
  the plugin delays by the same amount), so it does not change with the
  setting.

CPU
  About 25-45 % of one core at 48 kHz and 1x on a typical desktop with the
  full power supply (measured on an Apple M1: about 28 %), about a fifth
  less with the simple one (the whole amp is solved every sample); stereo with
  different left and right signals doubles that. With the same signal on
  both channels it is computed once. After 2 s of silence at the input
  (output below -70 dBFS, i.e. only hum) the plugin goes idle.

Input level (INPUT, since 1.19)
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

1.21
  - Turning knobs costs less CPU: when only resistor values change (pots),
    the circuit matrix is updated with rank-one corrections instead of being
    inverted again; a full inversion every 16 updates keeps the rounding in
    check. Load while turning three knobs: about 0.26 -> 0.22 of a core
    (block 128). Sound with fixed settings bit-identical; while turning the
    difference to a full inversion is below -80 dB.
  - Switches no longer click: input jack and the volume pot taper used to
    switch the circuit from one sample to the next, which made the output
    jump. Now the output fades out over 3 ms, the switch happens in the
    silent spot, and after the plugin latency (31 samples) it fades in over
    4 ms. Measured with a sine: the click is down from up to -8 dB to about
    -76 dB (linear taper: -51 dB). Speaker already crossfades inside the amp
    (20 ms); oversampling and power supply keep the core swap of 1.20.

1.20
  - No more dropout and click when the circuit is switched (oversampling, power supply):
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

1.19
  - INPUT now means the peak voltage of your playing: the plugin remembers
    the highest input peak and scales to it (see "Input level"). Before, it
    was the voltage at 0 dBFS of the track, so a track peaking at -12 dBFS
    reached the amp with only a quarter of the set voltage. Old projects
    keep the old behaviour and sound bit-identical.

1.18
  - Rebuilt with the shared solver code of the SVT-CL 1.4 (matrix of the
    Newton step built per device block). Sound bit-identical to 1.17.

1.17
  - Faster solver (dense matrix products): about 6-8 % less CPU. Sound
    unchanged apart from rounding and solver tolerance (difference below
    -95 dB).
  - Fixed: after inserting or enabling the plugin on a record-armed track
    it stayed silent for live input (meters dead) until Play was pressed
    once. The idle mode after silence is now internal only; the plugin no
    longer asks the host to stop calling it.

1.16
  - New parameter "Volume pot taper" (POT TAPER toggle in the model strip):
    taper of the two 1 MOhm volume pots: LOG 15 % (as on the amp, default),
    LOG 10 % or LINEAR. Treble and bass are linear pots on the schematic and
    stay linear. Old projects load unchanged (log 15 %). Same selector on
    the web page.

1.15
  - Power supply selectable: simple (DC behind internal resistance and
    5AR4, the new default; no hum, about 20 % less CPU, measured: 17 % fewer
    instructions, 1.7 instead of 3.1 Newton steps per sample at idle) or
    full (mains transformer, rectifier on the AC voltage, hum; unchanged,
    sample-identical to 1.14). The simple one is fitted to the full one at
    idle and at full drive; operating points, gain, frequency response and
    output power agree within 0.1 dB.
  - STANDBY and the GROUND switch removed (panel and parameters; the
    Standby parameter stays hidden for old projects and has no effect).
  - Projects from before 1.15 load with the full power supply.
  - Tried and not adopted: own exp/log routines for the tube formulas. With
    the same accuracy they were slower than the system's (24.6 vs 18.7 ns),
    with less accuracy only as fast.

1.14
  - STANDBY with a strong input signal: in some states, single blocks took up
    to 20 times real time (the solver's fallback swung back and forth at the
    kinks of the output transformer's hysteresis and gave up on dozens of
    samples in a row). The fallback now halves its steps while the error
    grows; in the case found, 42 failed samples per block -> none. The
    output is otherwise sample-identical.
  - Frequency plot: the settled amp for the plot is computed in a background
    thread. Before, opening the window the first time at 2x or 4x (or after a
    sample-rate change) froze the host's interface, measured on an M1 about a
    third of a second at 48 kHz and correspondingly longer at higher rates.
  - Background work (oversampling switch, plot) stops at once when the plugin
    is deactivated or removed, instead of being waited for.
  - A host pause while the plugin sleeps (silent input) no longer starts it
    over (as in the Laney).
  - Removed without closing the window first: the window timer is now
    unregistered (Linux).
  - Mac (Audio Unit, VST3): the window draws the photo and printing once per
    size in screen pixels and keeps its text layouts; measured on an M1: 4 to
    6 ms per frame, 1 to 7 % of one core with the window open while playing
    (under Rosetta up to about 10 %). The window's class name is now unique
    per loaded file: before, the VST3 and the Audio Unit (or an old and a new
    copy) used the same name, and when a host loaded both, the window could
    run code from the other file and crash once that file was unloaded.

1.13
  - X11 error handling only during the plugin's own window calls. Before,
    each plugin hooked permanently into the process-wide X error handler
    chain; with several of our plugins loaded and one removed first, that
    chain could point into unloaded code. Tested: host window destroyed
    under an open plugin window, no crash, host handler restored afterwards.

1.12
  - Plugin window: much cheaper to draw. The photo used to be rescaled for
    every frame (about 30 times a second) and the window redrawn whenever a
    meter moved by a hair; with the window open that cost up to half a CPU
    core on the GUI thread, which the CPU figure in the model strip does not
    include (it measures only the audio processing). Now the photo is
    scaled once per window size, meters redraw only when a bar moves by a
    pixel, the CPU figure updates twice a second. Measured here: Laney window
    45 % -> 7 % of a core, Yamaha 18 % -> 1.4 %, AmpFett 22 % -> 2.8 %.

1.11
  - Faster: the circuit solver works with a slightly looser convergence
    limit (10 instead of 1 microvolt; difference to 1.10 below -100 dB even
    at low levels) and with precomputed sparse matrices. About 10-15 % less
    CPU.
  - Mac/Logic: run Logic natively, not under Rosetta (see LIESMICH-Mac.txt).

1.10
  - Power tubes 6L6GC: new pentode model after Derk Reefman with a proper
    knee and screen current, fitted to the data sheet: five single-tube
    operating points (plate and screen current, transconductance, plate
    resistance) and six push-pull full-power conditions (average currents
    and output power). The old Koren model had a far too soft knee (at
    0 V grid and 50 V plate 94 instead of 136 mA) and gave up to 26 % too
    little power in the push-pull conditions. Idle current 26 mA as before;
    small-signal gain about 0.3 dB lower; full power unchanged (about 33 W).

1.9
  - Tube mismatch now defaults to 8 % (a realistic pair of power tubes;
    double-click on the knob also sets 8 %). Projects saved with older
    versions keep 0 %, so they sound as before.

1.8
  - The knob in the model strip is now labelled TUBE MISMATCH (was MISMATCH).
    Sound unchanged.

1.7
  - Mono tracks: input and output can now be mono or stereo (1->1, 1->2,
    2->1, 2->2). Logic offers Audio Units on a mono track only with a mono
    input; with stereo only, the plugin most likely did not show up there.
    Sound unchanged (the engine works in mono anyway).

1.6
  - 6SL7 computed with the triode formula of Dempwolf and Zoelzer (2011),
    fitted to the RCA data sheet and the factory voltage table (fit error
    halved compared to the Koren form). The preamp stages draw slightly more
    current; the amp is about 2 dB louder at the same input (100 mV at
    110 Hz: 10.1 V at the speaker instead of 8.0 V) and distorts a little
    earlier, as the fitted tube says.
  - Output transformer core with hysteresis (estimate, Rayleigh-type, as on
    the Laney): a few more odd harmonics on loud low notes (41 Hz, volume 10:
    5th harmonic -55 dB instead of -61 dB at 4.5 V), nothing at low level.
  - Mains transformer PT-108 as a transformer (primary 120 V, leakage,
    centre tap): STANDBY now opens the centre tap like the real switch; the
    bias supply keeps working from the winding end as in the schematic.
  - New parameter "Mains frequency" (MAINS switch in the model strip):
    60 Hz like the US original, or 50 Hz like the export version. Changes the
    hum and ripple frequency and the supply sag between charging pulses.

1.5
  - Output transformer core saturation (estimate): extra magnetising
    current above a flux of about 4 V*s across the primary. At full power
    it sets in below about 25 Hz; at 41 Hz (low E) it adds a few percent.
    Mostly audible on loud low notes with the volume up; the idle voltages
    and everything else are unchanged.
  - New parameter "Power tube mismatch" (0-10 %, MISMATCH knob in the model
    strip): the two 6L6GC draw different idle currents, which adds even
    harmonics and a DC flux in the output transformer. Glides over 0.3 s.
    Projects from older versions load with 0 %.
  - Tube models checked against more data (engine/tools/tube_fits.py): the
    6SL7 model fits the data sheet and the first-stage voltages of the
    factory table (within 4 V), the 6L6GC model stays as fitted to seven
    data sheet points (Koren's published set is 5-10 % higher). Unchanged.

1.4
  - Switching the FX off and on again in REAPER (or a host reset) caused a
    short odd sound: the amp resumed from the state it had when it was
    switched off, or glided from default knob positions and ramped the
    speaker in. Now it restarts from the settled idle state with all knobs
    and switches already in place - like a real amp that kept running
    silently in the meantime. Triggered by start of processing, reset, or
    a gap in the host's sample clock.

1.3 (tube simulation checked for realism)
  - 6SL7 preamp and phase inverter tubes are now computed with the full
    formula instead of an interpolated table. The table added small kinks
    that produced false high harmonics on quiet notes (4th/5th harmonic
    around -60 dB instead of below -100 dB).
  - Output transformer: primary winding capacitance (500 pF) and core loss
    (150 kOhm) added (estimates). They damp the switching of the 6L6GC like
    a real transformer. Before, hard clipping at 1x produced a fizzy ringing
    at half the sample rate, and single-sample spikes of up to 150 V at the
    speaker terminal (a real amp cannot do that).
  - Speaker voice coil: eddy-current loss (20 ohm parallel to the 1 mH
    inductance, estimate). Its impedance at 10 kHz is now about 25 ohm
    instead of 64 ohm, as typical for a 15" bass speaker; less overshoot
    when clipping, about 2 dB less treble at the terminal above 4 kHz.
  - Unchanged: idle voltages (factory table), output power, tone controls.

1.2 (code review)
  - Changing the oversampling twice quickly (1x -> 2x -> 4x) could leave the
    plugin stuck at the old setting; the toggle then showed "switching ..."
    for good. Fixed.
  - Reset by the host no longer settles the amp on the audio thread (it
    copies a stored idle state instead); safer start of the oversampling
    helper thread.
  - Absurd input (far beyond any instrument level, NaN) no longer stalls
    the audio thread for half a second per sample: the input is limited to
    +-20 V at the jack and a derailed circuit returns to its idle state at
    once.
  - Knob glides recompute the circuit every 64 samples of the base rate
    (before: of the oversampled rate - up to 4x the work while turning).
    Sound of steady settings unchanged.

1.1
  - Treble and bass worked the wrong way round (10 cut, 0 boosted), in both
    channels. Fixed: 10 = full boost, as on the amp. Volume was right.
    Range now at 8 kHz: treble +10 / -29 dB; at 40 Hz: bass +7 / -15 dB
    (relative to the knob at 5).
  - Renamed to AmpFett B-15NF (new plugin ID: projects with 1.0 do not load
    their settings; remove Ampeg_B15NF.clap / Ampeg_B15NF.vst3 by hand).
    Panel lettering in Damion (SIL Open Font License).

VALUE POPUP (new in 1.22)
  In the header line of the model strip, "VALUE POPUP ON / OFF" switches the
  small value display that appears at a knob while the mouse is over it or
  while turning. Click to toggle. The choice is stored for all tonestack
  plugins of this user (Linux: ~/.config/tonestack/ui.conf, macOS:
  ~/Library/Application Support/tonestack/ui.conf), not per project.
  No change in sound.

Notes
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer).
  Ampeg and Portaflex are trademarks of their owners; this is an independent
  circuit simulation, not affiliated with or endorsed by them.

1.23
  Internal change only (the output transformer's core saturation can be
  switched off for check calculations against SPICE; always on in the
  plugin). Sound and controls as 1.22.

1.24
  6L6GC model with the sharp knee of a beam tetrode. Knee and secondary
  emission come from a 6L6GC measured with a curve tracer (D. Reefman,
  uTracer / ExtractModel library); the level parameters were refitted to the
  same data sheet values as before (same fit error). Data sheet push-pull
  AB2: 45.9 W instead of 42.9 W (data sheet 47 W); screen current at full
  power 17 instead of 21 mA (data sheet 16). In the amp: supply voltages 2 V
  closer to the schematic, clean sound unchanged (-55 dB difference), driven
  hard less than 0.1 dB per band. Checked against ngspice (-64 dB) and by
  halving the time step (-45 dB at full overdrive).

1.25
  Grid current scaled per tube type. The law itself was measured on a
  12AX7 and was used unchanged for every tube. Now: 6L6GC from the RCA data
  sheet (push-pull AB2, peak grid-input power 140 mW / 270 mW: about half the
  former current), other tubes by their perveance (measured ratio grid to
  cathode perveance: 12AX7 0.31, 6922 0.26); 6550 and EL34 scaled from the
  6L6GC (estimate). Effect on the sound: below 0.15 dB per band even when
  driven very hard.
