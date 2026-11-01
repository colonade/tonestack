Yamaha B100-II Bass Amp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.9

Two formats, one engine
  Yamaha_B100II.clap and Yamaha_B100II.vst3 contain the same code and sound identical
  (checked sample by sample). Use whichever your DAW prefers; REAPER
  handles both. CLAP is the native format, the VST3 is built from it with
  clap-wrapper (free-audio, MIT) and the VST3 SDK 3.8 (Steinberg, MIT) and
  passes Steinberg's validator (47 of 47 tests). A project saved with one
  format does not load its settings into the other.
  Third-party licenses (MIT, public domain; Mac Audio Unit also Apache 2.0): THIRD-PARTY-LICENSES.txt.

Installation (CLAP)
  mkdir -p ~/.clap && cp Yamaha_B100II.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Yamaha B100-II Bass Amp".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Yamaha_B100II.vst3 ~/.vst3/
  (Yamaha_B100II.vst3 is a folder - copy it as a whole.) REAPER searches ~/.vst3
  by default; otherwise add it under Options > Preferences > Plug-ins >
  VST, then "Re-scan". The plugin appears under VST3 as "Yamaha B100-II Bass Amp".

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
  +10 / -9.5 dB at the ends), Pot taper (log 15 %, log 10 %, linear),
  Power amp, Instrument level, Speaker, Output level.
  Defaults: HIGH, all controls 5, EQ flat, power amp off, speaker off.

User interface
  Photo of the front panel with turnable knobs: drag vertically (mouse wheel
  works too), double-click = centre. Click the HIGH or LOW jack to plug in
  there. Model strip below (settings that do not exist on the amp): toggle
  switches for POWER AMP and SPEAKER (lever up = on, click to flip, mouse
  wheel up/down), a three-position toggle for the POT TAPER (log 15 %,
  log 10 %, linear; click the upper or lower half to move the lever), the
  power amp peak meter, INPUT (instrument level) and OUTPUT (level in dB).
  Text in the lower part is sized for the screen (12-14 pixels at the
  default width), independent of the photo resolution. Every change is sent to
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
  The SPEAKER toggle (parameter "Speaker") adds a 15" bass speaker after the
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
  mark the low E (41 Hz) and low B (31 Hz) of a bass. The plot ends at the
  power amp input (MA board input network included); the power amp itself is
  flat above 20 Hz apart from its clipping.

1.9
  - Switching the FX off and on again in REAPER: the circuit resumed from
    the state frozen when it was switched off, a short odd sound. Now it
    starts from rest with all controls in place (after start of processing
    or a pause of more than 0.2 s). Sound otherwise bit-identical to 1.8.

1.8 (code audit, schematic re-check)
  - EQ: the G50K pots have a grounded centre tap (schematic). With the knob
    at 0 the band is now switched off completely; boost/cut at the ends is
    +10 / -9.5 dB, exactly the Table 7 value of the service manual (before
    +11 to +12 dB).
  - Power amp (MA board) input: 10 kOhm and 3300 pF to ground, 1 uF to the
    gate of TR301. The 3300 pF roll off the treble (-3.3 dB at 10 kHz), which
    is exactly what the factory curve Fig. 1 shows: the model now matches
    Fig. 1 within 0.6 dB from 40 Hz to 19 kHz (before: 3.3 dB off at 10 kHz).
  - MA board gain from the schematic: 1.8 MOhm feedback against
    100 kOhm || (10 kOhm + 1 uF), 199x (46 dB) above 16 Hz. The chain is still
    calibrated to Table 3 (0.775 mV at HIGH -> 2.2 V at the speaker).
  - Without the power amp the output is about 7 dB quieter than in 1.7 (the
    MA input network now loads the PA board as in the real amp); turn up
    Output level if needed.
  - Instrument level glides over 20 ms (no click when changing it with the
    power amp on); output level glides too.
  - Plot grid lines and redraw fixed; no memory leak when closing the window.

1.7
  The plugin now also exists for macOS (Audio Unit for Logic and VST3,
  Apple Silicon and Intel, from macOS 11) with the same front panel. Linux:
  internal restructuring only, sound and panel identical to 1.6.

1.6
  The plugin version is shown in the plugin window (model strip, "PLUGIN
  VERSION 1.6"), so you can check in REAPER which build is loaded.
  Pot values checked once more against the schematic and the parts list of
  the service manual (PA board): Volume A25K, Treble A25K, Bass A25K (wired
  as variable resistor), Middle A1K (variable resistor), equalizer 5 x G50K
  with center click. All values and connections match the model; POT TAPER
  applies to volume, treble, middle and bass (the A pots). The service
  manual does not define the "G" curve of the equalizer pots; they are
  modelled linear (the end positions +-10 dB do not depend on it).
  Sound unchanged (bit-identical to 1.5).

1.5
  Tested like the web simulators (latency, CPU load, every parameter,
  clicks when switching). Latency 0 samples, CPU about 3 % of one core.
  Fixed: switching POWER AMP and SPEAKER produced a click (the power amp
  path and the speaker filter were switched hard and reset). Both are now
  cross-faded over 20 ms and the speaker filter keeps running; the jump at
  the switch went from 21x and 99x the steepest slope of the music to 1.5x
  and 1.1x. Sound otherwise unchanged; CLAP and VST3 bit-identical.

1.4 (crash fixes - please update)
  An X11 error on the plugin's own window (drawing into or destroying a
  window the host had already removed, e.g. when closing the editor) went
  to Xlib's default handler, which ends the whole process (REAPER). The
  plugin now ignores X errors on its own display connection, notices when
  the host destroys its window, and serialises all editor calls (with
  VST3 the wrapper may call the timer from a helper thread). INPUT and
  OUTPUT knobs can be turned again (1.3 missed them when hit-testing).
1.3
  Model strip with toggle switches (power amp, speaker, three-position pot
  taper); all text below the photo enlarged and sized in screen pixels
  (before: about 7 pixels at the default width). Audio unchanged.
1.2
  VST3 version (same engine, sound bit-identical to the CLAP). Knob drags
  in the plugin window are now sent to the host as gestures (begin, values,
  end), so REAPER records them as automation in touch/latch mode and a VST3
  host learns about them at all. Audio unchanged.
1.1 (code review)
  Non-finite input samples are treated as silence and a diverged circuit
  state restarts the filter instead of outputting NaN; denormals are
  flushed; state is saved with full precision; with a mono input the second
  engine is no longer kept running. Audio bit-identical to 1.0.

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
