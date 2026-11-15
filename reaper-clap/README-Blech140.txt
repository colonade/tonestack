Blech 140 (after EMT 140 plate reverb) - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.0

Two formats, one engine
  Blech_140.clap and Blech_140.vst3 contain the same code and sound
  identical. Use whichever your DAW prefers. CLAP is the native format, the
  VST3 is built from it with clap-wrapper (free-audio, MIT) and the VST3
  SDK 3.8 (Steinberg, MIT). A project saved with one format does not load
  its settings into the other.
  Third-party licenses: THIRD-PARTY-LICENSES.txt.

Updating
  Close REAPER first. Copying a new .clap over the file a running REAPER has
  loaded makes REAPER crash (at once or a while later). scripts/update-
  tonestack.sh refuses to run while REAPER is open and never overwrites a
  plugin file in place.

Installation (CLAP)
  mkdir -p ~/.clap && cp Blech_140.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Blech 140 (after EMT 140 plate reverb)".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Blech_140.vst3 ~/.vst3/
  (Blech_140.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  A plate reverb as a physical model of the EMT 140 (Elektromesstechnik,
  from 1957): a tensioned steel plate of 2 m x 1 m x 0.5 mm, one driver,
  two pickups (stereo). All modes of the plate up to 18 kHz (about 23 000)
  are computed with their damping; from them the impulse response (8 s),
  which the signal is convolved with at zero latency. "EMT" is a trademark
  of its owner; this plugin is independent and no measurement of a unit.
    Plate      thin plate (Kirchhoff) with tension, simply supported edges;
               tension assumed 700 N/m (matters only below about 50 Hz).
    Damping    thermoelastic loss (constants after Arcas/Chaigne), damper
               plate (grows with the square root of frequency; set by
               DECAY), high-frequency loss (assumed: RT60 about 0.9 s at
               10 kHz with the damper open).
    Pickups    two points at assumed positions, at different distances
               from the driver: left and right differ.

Controls
  On the unit:
  DECAY     reverberation time at 500 Hz, 1.0 to 3.8 s (the unit: about
            1 to 4 s; above 3.8 s the open plate of the model does not
            reach). Lows ring longer, highs shorter, e.g. at 2 s: 3.8 s at
            125 Hz, 0.9 s at 4 kHz. Default 2 s.
            Like the motor-driven damper plate of the unit, the reverb
            follows DECAY with a delay: after turning, a new plate is
            computed on a helper thread (about 0.7 s), then the reverb
            cross-fades to it in 0.1 s. The lamp MOTOR is lit meanwhile, the
            meter needle shows the plate currently in use (red mark: DECAY).
  BASS CUT  0 to 3: first-order high-pass on the reverb at 20 (off), 100,
            200, 400 Hz (frequencies assumed).
  Model (not on the unit, where the console mixes):
  MIX       share of reverb, 0 - 100 % (default 30 %).
  PRE-DELAY 0 - 200 ms before the plate.
  OUTPUT    -24 to +24 dB, 0 dB in the middle.
  The reverb is about as loud as the dry signal for noise at DECAY 2 s,
  slightly louder with longer DECAY.

Measured (engine/tests/blechtest.cpp)
  Third-octave RT60 from the impulse response (Schroeder, T30) against the
  value expected from the model: within 1 - 2 % at 250 Hz - 8 kHz; 125 Hz at
  DECAY 3.8 s 10.6 instead of 11.2 s (the fade-out from 7 s cuts in).
  Convolution against direct computation -129 dB, first output without
  delay. Impulse response against the exact sum of all modes in the time
  domain better than -57 dB. CPU at 48 kHz about 3 % of one core; a new
  plate costs about 0.7 s on the helper thread. Above about 88 kHz the
  reverb runs at half or a quarter of the sample rate (the plate has no
  modes above 18 kHz): same level as at 48 kHz, parts above 26 kHz below
  -80 dB, about 0.5 ms more delay in the reverb path only.

Mono / stereo
  Mono in, stereo out (two pickups). With a stereo input the plate is fed
  with the sum, the dry signal stays on its side. Latency 0. The plugin
  reports its tail length (8 s + pre-delay) so a host keeps processing
  after the input ends. Memory about 60 MB; activating takes about 0.8 s.

Interface
  The front panel is an own drawing (no photo, no logo): meter
  NACHHALLZEIT (reverberation time), DECAY, lamp MOTOR, switch BASS CUT,
  and the model knobs. Below it: the reverberation time of the plate in use
  at 125 Hz, 500 Hz, 2 kHz, 8 kHz and the RT60 over frequency (solid: plate
  now, dashed: DECAY setting while the damper moves).
  Drag a knob up/down, mouse wheel for fine steps, double-click = default.
  BASS CUT: click a position; clicking the centre steps on; mouse wheel.
