Laney AOR 50 Preamp - native CLAP plugin for REAPER (Linux x86-64)
Version 3.4

Installation
  mkdir -p ~/.clap && cp Laney_AOR50.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Laney AOR 50 Preamp".

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
  switches, click the HIGH/LOW jack to choose the input. Model strip below:
  bass mod, FX loop + power amp, distortion, oversampling (button cycles
  1x > 2x > 4x > 8x), pot taper, instrument and output level. Every change is
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

Notes
  True stereo: left and right are processed by two independent engines. A
  mono input is sent to both channels.
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer); if REAPER
  does not find the plugin after scanning, tell me your distribution and I
  will build against an older glibc.
