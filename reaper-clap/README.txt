Laney AOR 50 Preamp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 3.9

Two formats, one engine
  Laney_AOR50.clap and Laney_AOR50.vst3 contain the same code and sound identical
  (checked sample by sample). Use whichever your DAW prefers; REAPER
  handles both. CLAP is the native format, the VST3 is built from it with
  clap-wrapper (free-audio, MIT) and the VST3 SDK 3.8 (Steinberg, MIT) and
  passes Steinberg's validator (47 of 47 tests). A project saved with one
  format does not load its settings into the other.
  Third-party licenses (all MIT or public domain): THIRD-PARTY-LICENSES.txt.

Installation (CLAP)
  mkdir -p ~/.clap && cp Laney_AOR50.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Laney AOR 50 Preamp".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Laney_AOR50.vst3 ~/.vst3/
  (Laney_AOR50.vst3 is a folder - copy it as a whole.) REAPER searches ~/.vst3
  by default; otherwise add it under Options > Preferences > Plug-ins >
  VST, then "Re-scan". The plugin appears under VST3 as "Laney AOR 50 Preamp".

What it is
  The same engine as the web simulator (linear network from schematic
  DRG 1054, tube stage with Koren 12AX7 model and Newton solver, power amp
  with negative feedback and presence), translated to C++. Verified against
  the simulator: linear within -137 dB, tube stage 2x oversampling bit-exact.
  About 20 times faster than the JSFX version, so 8x oversampling runs in
  real time without trouble.

Parameters
  Input jack, Channel, Preamp 1 Volume, Preamp 2 Volume, Preamp 1 Level,
  Treble, Middle, Bass, Master Volume, Mid/Top/Bass Boost, Bass mod,
  FX loop + power amp (Presence), Middle/Presence pot taper, Output level,
  Tube distortion, Instrument level, Oversampling (1x/2x/4x/8x, default 4x),
  Presence (active with "FX loop + power amp").
  Defaults: Preamp 1, HIGH, all controls 5, tube distortion on, 8x oversampling,
  bass mod on, FX loop + power amp on, stock tubes (3 x 12AX7).

User interface
  Photo of the front panel with turnable knobs: drag vertically (mouse wheel
  works too), double-click = centre. Click PULL ON / PULL BOOST for the pull
  switches, click the HIGH/LOW jack to choose the input. Model strip below
  (settings that do not exist on the amp): toggle switches for BASS MOD,
  FX LOOP (+ power amp), DISTORTION and SPEAKER (lever up = on, click to
  flip, mouse wheel up/down), a rotary selector for OVERSAMPLING (click
  turns it on, wheel turns either way), a three-position toggle for the
  middle/presence pot TAPER (click the upper or lower half to move the
  lever), the TUBE DRIVE meters, INPUT and OUTPUT. Below: the three tube
  sockets (click to change the type) and the frequency response. Text in
  the lower part is sized for the screen (12-14 pixels at the default
  width), independent of the photo resolution. Every change is
  sent to REAPER as a parameter (automatable); REAPER's own parameter view
  stays available via its UI button. The window can be resized horizontally.
  Dimmed controls have no effect in the current setting; REVERB is not
  modelled.
  Needs libcairo2 and libX11 (present anyway with REAPER on Linux).

Tubes
  Three parameters Tube V101, Tube V102, Tube V103 (each socket is a twin
  triode) choose the tube type: 12AX7/ECC83 (stock), 12AT7/ECC81, 12AU7/ECC82,
  5751, 12AY7. The models for 12AX7, 12AT7 and 12AU7 are Norman Koren's
  published parameters; 5751 and 12AY7 are Koren-type models fitted to the
  data sheet operating point (mu fixed) and are marked "fitted" - less
  certain away from that point. The operating points (bias, plate voltages)
  follow from the model and the circuit, so a different tube also shifts the
  bias of its stage. In the user interface the row above the frequency
  response shows the sockets; clicking a socket cycles the type, RESET TO
  STOCK restores 3 x 12AX7. Stock tubes are bit-exact with version 3.3.
  Not modelled: sample-to-sample variation, microphonics, ageing; the phase
  inverter V104 and the power tubes stay linear.

Frequency response
  Below the model strip the plugin draws the small-signal frequency response
  of the current setting (blue), relative to 1 kHz, computed from the same
  circuit network as the audio (complex nodal analysis, 20 Hz to 20 kHz).
  The dashed grey line is the reference: original parts, all controls at 5,
  boosts off, same channel, input and FX-loop setting. With tube distortion
  on, the curve still shows the linear (small-signal) response. The window
  can be resized; the plot scales with it.

Meters and load
  TUBE DRIVE: five bars show how hard each triode stage (V101A, V101B,
  V102A, V102B, V103B) is driven at its grid; the white mark is the onset of
  overdrive (1.0), green below, amber around, red above. The text line in
  the model strip shows the CPU load of the plugin in percent of one core,
  the oversampling in use and the sample rate.

Idle
  When the input has been silent for a while and the circuit has settled
  (all capacitor voltages within 5 mV of their resting values), the plugin
  tells REAPER it may stop calling it (CLAP "sleep") and, whether or not the
  host honours that, skips its own processing and outputs silence; the CPU
  line then shows "idle (silent input)". With tube distortion this takes about 9 s after the
  last note, because of C12 (3.3 MOhm feedback, about 1 s time constant);
  in linear mode about 1 s. On the next note it continues from the resting
  state, so the result does not depend on whether or how long it slept
  (deviation from uninterrupted processing below -115 dB).

Speaker (since 3.6)
  The SPEAKER toggle in the model strip (parameter "Speaker") adds a
  15" bass speaker after the preamp/power amp: Eminence Legend CB158, Thiele-
  Small data from the data sheet (Fs 34 Hz, Qts 0.34, Vas 336 l, Re 6.15 ohm,
  Le 0.33 mH), in a 78 l vented box tuned to 50 Hz, the size of an Ampeg
  Portaflex cabinet and Eminence's own recommendation for this driver. The
  low end is the acoustic equivalent circuit (-3 dB at 61 Hz, +1.7 dB around
  100 Hz); the top is the voice-coil inductance (3 kHz) plus an estimated
  second-order roll-off at 4 kHz for cone break-up. No room, no microphone.
  The frequency-response plot includes it when it is on. Off = identical to 3.5.

3.9
  Model strip with toggle switches (bass mod, FX loop, distortion,
  speaker, taper) and a rotary selector for oversampling; speaker moved
  from the tube row into the strip; all text below the photo enlarged and
  sized in screen pixels (before: down to about 5 pixels at the default
  width). Audio unchanged.
3.8
  VST3 version (same engine, sound bit-identical to the CLAP). Knob drags
  in the plugin window are now sent to the host as gestures (begin, values,
  end), so REAPER records them as automation in touch/latch mode and a VST3
  host learns about them at all. Audio unchanged.
3.7 (code review)
  Fixed latency of 31 samples in every mode (see below). The tube tables and
  the operating-point cache are now built and used thread-safely (several
  instances on parallel tracks, GUI thread); the tables are built when the
  plugin is created, not on the audio thread. The idle/sleep state now also
  works with tube distortion (a key mismatch had disabled it since 3.4).
  Non-finite input samples are treated as silence and a diverged solver
  restarts the circuit instead of outputting NaN forever. Denormals are
  flushed during processing. Plugin state is saved with full precision.
  With a mono input the second engine is no longer kept running. The audio
  with tube distortion and oversampling is bit-identical to 3.6.1.
3.6.1
  Frequency-response plot refreshes on the wall clock (before, it could lag
  behind a knob move when the process used little CPU). Audio unchanged.

Polarity and latency (since 3.5)
  The output has the same polarity as the input. The preamp itself inverts
  (five inverting tube stages), which earlier versions reproduced; a sound
  engineer pointed out that a plugin should not flip polarity, so the engine
  now undoes it. Apart from the sign the output is identical to 3.4.
  With tube distortion and oversampling the engine has 31 samples of latency
  (oversampling filters). Since 3.7 the plugin always reports 31 samples
  and delays the linear and 1x paths by the same amount, so the latency
  never changes while the plugin is active (which CLAP forbids) and
  switching modes stays click-free; REAPER compensates it automatically.

Notes
  True stereo: left and right are processed by two independent engines. A
  mono input is sent to both channels.
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer); if REAPER
  does not find the plugin after scanning, tell me your distribution and I
  will build against an older glibc.
