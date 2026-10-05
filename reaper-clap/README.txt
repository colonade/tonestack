Laney AOR 50 Preamp - native CLAP plugin for REAPER (Linux x86-64)
Version 2.9

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
  Defaults: Preamp 1, HIGH, all controls 5, tube distortion on, 4x oversampling.

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

Meters and load
  TUBE DRIVE: five bars show how hard each triode stage (V101A, V101B,
  V102A, V102B, V103B) is driven at its grid; the white mark is the onset of
  overdrive (1.0), green below, amber around, red above. The text line in
  the model strip shows the CPU load of the plugin in percent of one core,
  the oversampling in use and the sample rate.

Notes
  True stereo: left and right are processed by two independent engines. A
  mono input is sent to both channels.
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer); if REAPER
  does not find the plugin after scanning, tell me your distribution and I
  will build against an older glibc.
