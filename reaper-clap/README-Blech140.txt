Blech 140 (after EMT 140 plate reverb) - CLAP and VST3 plugin for REAPER and other hosts (Linux x86-64 and ARM64, Windows x86-64)
Version 1.8

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
  are computed with their damping; from them the impulse response (10 s),
  which the signal is convolved with at zero latency. "EMT" is a trademark
  of its owner; this plugin is independent and no measurement of a unit.
    Plate      thin plate (Kirchhoff) with tension, simply supported edges;
               tension assumed 700 N/m (matters only below about 50 Hz).
    Damping    thermoelastic loss (constants after Arcas/Chaigne), damper
               plate (grows with the square root of frequency; set by
               DECAY), high-frequency loss. With the damper open the
               plate rings 5 s at 500 Hz and 1.5 s at 10 kHz, as the EMT
               140 TS data sheet (1971) states for the undamped plate.
    Pickups    two points at assumed positions, at different distances
               from the driver: left and right differ.

Controls
  On the unit:
  DECAY     reverberation time at 500 Hz, 1.0 to 5.0 s (data sheet: about
            1 s with the damper closest, about 5 s undamped; 5 s = damper
            fully open). Lows ring longer, highs shorter, e.g. at 2 s:
            3.8 s at 125 Hz, 0.9 s at 4 kHz. Default 2 s.
            Like the motor-driven damper plate of the unit, the reverb
            follows DECAY with a delay: after turning, a new plate is
            computed on a helper thread (about 0.9 s), then the reverb
            cross-fades to it in 0.1 s. The lamp MOTOR is lit meanwhile, the
            meter needle shows the plate currently in use (red mark: DECAY).
  BASS CUT  0 to 3: first-order high-pass on the reverb at 20 (off), 100,
            200, 400 Hz (frequencies assumed).
  Additions (on the unit the console mixes):
  MIX       share of reverb, 0 - 100 % (default 30 %).
  PRE-DELAY 0 - 200 ms before the plate.
  OUTPUT    -24 to +24 dB, 0 dB in the middle.
  The reverb is about as loud as the dry signal for noise at DECAY 2 s,
  slightly louder with longer DECAY.

Measured (engine/tests/blechtest.cpp)
  Third-octave RT60 from the impulse response (Schroeder, T30) against the
  value expected from the model: within 1 - 2 % at 250 Hz - 8 kHz; 125 Hz at
  DECAY 5 s 14.2 instead of 15.2 s (the fade-out from 9 s cuts in).
  Convolution against direct computation -129 dB, first output without
  delay. Impulse response against the exact sum of all modes in the time
  domain better than -56 dB. CPU at 48 kHz about 5 % of one core; a new
  plate costs about 0.9 s on the helper thread. Above about 88 kHz the
  reverb runs at half or a quarter of the sample rate (the plate has no
  modes above 18 kHz): same level as at 48 kHz, parts above 26 kHz below
  -80 dB, about 0.5 ms more delay in the reverb path only.

Mono / stereo
  Mono in, stereo out (two pickups). With a stereo input the plate is fed
  with the sum, the dry signal stays on its side. Latency 0. The plugin
  reports its tail length (10 s + pre-delay) so a host keeps processing
  after the input ends. Memory about 75 MB (1.5: 63 MB); activating takes about 1 s.

Interface
  The front panel is an own drawing (no photo, no logo): meter
  NACHHALLZEIT (reverberation time), DECAY, lamp MOTOR, switch BASS CUT,
  and the model knobs. Below it: the reverberation time of the plate in use
  at 125 Hz, 500 Hz, 2 kHz, 8 kHz and the RT60 over frequency (solid: plate
  now, dashed: DECAY setting while the damper moves).
  Drag a knob up/down, mouse wheel for fine steps, double-click = default.
  BASS CUT: click a position; clicking the centre steps on; mouse wheel.

Changes
  1.8  Values typed into the host in its units are read back correctly, and
       the values derived in the parameter text (dBu, Hz, decay at other
       frequencies) are computed from the shown value. After loading a
       project the plugin tells the host that its values changed (CLAP:
       params rescan; VST3: kParamValuesChanged); an empty state is
       rejected. Linux: built hardened (BIND_NOW, RELRO, _FORTIFY_SOURCE,
       stack protector) and stripped; the CLAP file exports only clap_entry
       (until now also about 900 symbols of its built-in libstdc++); the
       VST3 bundle carries Contents/Resources/moduleinfo.json; also for
       Linux ARM64 (Debian package and archive). Checked with Steinberg's
       VST3 validator, pluginval and clap-validator. Sound unchanged.
  1.7  Texts of the Mac installer corrected (DECAY 1 - 5 s, CPU and memory
       as in this README). Sound unchanged from 1.6.
  1.6  DECAY up to 5 s (was 3.8 s): the losses of the open plate are now
       calibrated to the EMT 140 TS data sheet (undamped about 5 s at
       500 Hz, about 1.5 s at 10 kHz; before, the model's plate alone gave
       about 4 s and 0.9 s). Impulse response 10 s instead of 8 s, so the
       longer bass decay is not cut off. At the same DECAY the band
       reverberation times change slightly: at 2 s by at most 4 %, at
       3.8 s up to 8 % shorter at 1 - 4 kHz and 13 % shorter at 125 Hz.
  1.5  Below 40 kHz sample rate the plate modes are limited to 0.45 x the
       sample rate (before, modes up to 18 kHz were summed even at 22.05 or
       32 kHz and folded back into the audible range). At 44.1 kHz and above
       the sound is unchanged.
  1.4  Windows version (VST3 and CLAP with the same front panel, installer and
       ZIP). Project state and values typed into the host no longer depend on
       the number format of the system (decimal comma). Windows: the window
       follows a change of the display scaling while it is open, and the mouse
       wheel also works when Windows sends it to the focused window. Packages
       now include the GPL licence text (LICENSE.txt). Sound unchanged.
  1.3  The PRE-DELAY value shown at the knob no longer carries the note that
       the control is an addition. Sound unchanged.
  1.2  The lettering "MODEL (not on the unit)" in the header line of the model
       strip is gone; the plugin version stays there. Sound unchanged.
  1.1  The value popup at the knobs is gone (it appeared while turning a
       knob or with the mouse over a control); with it the switch VALUE POPUP
       in the header line of the model strip. Sound unchanged.
  1.0  First version.
