Yamaha B100-II Bass Amp - native CLAP plugin for REAPER (Linux x86-64)
Version 1.0

Installation
  mkdir -p ~/.clap && cp Yamaha_B100II.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Yamaha B100-II Bass Amp".

What it is
  The same engine as the web simulator (https://borg.fluxstatus.de/tonestack/
  yamaha-b100ii/), translated to C++: the whole PA board of the B100-115II
  from the 1979 service manual - TR101 source follower input, tone controls
  (treble, middle, bass), TR102, volume, TR103/TR104, TR105 with the five
  gyrator EQ bands (60, 150, 320, 640, 1250 Hz) - as a linear network solved
  sample by sample. Optionally the MA board power amp with its clipping and
  a 15" speaker. Same results as the WebAssembly core of the web page
  (checked against it: identical, LOW jack within floating-point rounding).

Parameters
  Input jack (HIGH / LOW, LOW = -12 dB like the real jacks), Volume, Treble,
  Middle, Bass (0-10), EQ 60/150/320/640/1250 Hz (-5 ... +5, 0 = flat, about
  +-10 dB nominal at the ends), Pot taper (log 15 %, log 10 %, linear),
  Power amp, Instrument level, Speaker, Output level.
  Defaults: HIGH, all controls 5, EQ flat, power amp off, speaker off.

User interface
  Photo of the front panel with turnable knobs: drag vertically (mouse wheel
  works too), double-click = centre. Click the HIGH or LOW jack to plug in
  there. Model strip below: POWER AMP, SPEAKER, pot taper (button cycles),
  INPUT (instrument level) and OUTPUT (level in dB). Every change is sent to
  REAPER as a parameter (automatable); REAPER's own parameter view stays
  available via its UI button. The window can be resized.
  Needs libcairo2 and libX11 (present anyway with REAPER on Linux).

Power amp (MA board)
  Off (default): the output is the PA board output, linear, with the level
  relative to the input: all controls at 5 (log taper) give about -20 dB at
  1 kHz and -9 dB at 100 Hz, Volume 10 about -4 dB / +7 dB - use OUTPUT to
  make up. On: the whole amp with absolute levels. INPUT
  sets what peak voltage at the jack corresponds to 0 dBFS of the track
  (default 100 mV - a passive bass peaks around 100-300 mV, an active one
  up to 1 V). The chain is calibrated to the service manual (table 3: 0.775 mV
  at HIGH give 2.2 V at the speaker, 69 dB): TR101 x8.7, PA board, MA board
  x83 (1 + 820k/10k). The MA board clips at about +-42 V (100 W at 8 ohm clean
  = 40 V peak; soft knee from 38 V), then passes the 2200 uF output capacitor
  (9 Hz high-pass into 8 ohm). 48 V = 0 dBFS at the output, so a clean amp
  peaks at -1.2 dBFS; heavy clipping can go a little above 0 dBFS because the
  output capacitor tilts the clipped waveform - turn OUTPUT down then.
  The POWER AMP meter shows the peak the stage is asked for before clipping
  (0 dB = 42 V) and turns red when it clips.

Speaker
  The SPEAKER button (parameter "Speaker") adds a 15" bass speaker after the
  amp: Eminence Legend CB158, Thiele-Small data from the data sheet (Fs 34 Hz,
  Qts 0.34, Vas 336 l, Re 6.15 ohm, Le 0.33 mH), in a 78 l vented box tuned to
  50 Hz, the size of an Ampeg Portaflex cabinet and Eminence's own
  recommendation for this driver. Low end from the acoustic equivalent
  circuit (-3 dB at 61 Hz, +1.7 dB around 100 Hz); top end: voice-coil
  inductance (3 kHz) plus an estimated second-order roll-off at 4 kHz for cone
  break-up. No room, no microphone. Same model as in the Laney plugin.

Frequency response
  Below the model strip the plugin draws the small-signal frequency response
  of the current setting (blue), relative to 1 kHz, from the same network as
  the audio (complex nodal analysis, 20 Hz to 20 kHz), with input divider
  and speaker when selected. Dashed grey: the service-manual setting (volume,
  treble, middle, bass 10, EQ flat) with the same jack and speaker. E and B
  mark the low E (41 Hz) and low B (31 Hz) of a bass. The power amp is not in
  the plot (it is flat apart from the 9 Hz high-pass).

Polarity and latency
  The output has the same polarity as the input (the PA board itself
  inverts; the engine undoes that). No latency.

Idle
  After 0.5 s of silence at the input with the output decayed, the plugin
  tells REAPER it may stop calling it (CLAP "sleep") and outputs silence
  itself; the CPU line then shows "idle (silent input)".

Notes
  True stereo: left and right are processed by two independent engines. A
  mono input is sent to both channels.
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer).
