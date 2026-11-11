AmpFett 76 (after UREI 1176LN) - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.0

Two formats, one engine
  AmpFett_76.clap and AmpFett_76.vst3 contain the same code and sound
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
  mkdir -p ~/.clap && cp AmpFett_76.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "AmpFett 76 (after UREI 1176LN)".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r AmpFett_76.vst3 ~/.vst3/
  (AmpFett_76.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  The FET compressor/limiter UREI 1176LN, computed in the time domain part
  by part from the schematic (the revision with op-amp input and class AB
  output stage): input stage, INPUT pot, the FET as a variable resistor with
  its distortion-cancelling gate network, preamplifier, OUTPUT pot, output
  amplifier; side chain with both ratio ladders, gain reduction amplifier,
  full-wave rectifier, ATTACK and RELEASE. It is a feedback compressor like
  the unit: the side chain listens to the preamplifier output.
  On top of that four extensions modelled on the Origin Effects Cali76
  (own design - no schematic of the Cali76 is public): continuously
  variable ratio, DRY, a high-pass filter in the side chain, and a second
  stage in series ("stacked").
  "UREI", "1176", "Universal Audio", "Origin Effects" and "Cali76" are
  trademarks of their owners; this plugin is independent.

Parameters
  On the panel (as on the unit):
    Input, Output (0 ... 10)
    Attack (0 = off: no gain reduction; 1 slow ... 7 fast)
    Release (1 slow ... 7 fast)
    Ratio: 4:1, 8:1, 12:1, 20:1, all buttons, variable
    Meter: GR, +8, +4, Off (the needle shows gain reduction, or the output
      level with 0 VU = +8 / +4 dBu)
  Model strip (not on the unit):
    Variable ratio (4 ... 20:1; the tap slides along both ratio ladders and
      equals the buttons at 4, 8, 12 and 20)
    Dry (0 ... 10: input signal mixed to the output, parallel compression)
    Sidechain HPF on/off and frequency (20 ... 600 Hz, 2nd order)
    Stack on/off: a second, identical stage behind the first; Thru (its
      input), Attack 2, Release 2, Output 2. Ratio and HPF are shared.
    Input level (mV peak) and Input reference: the plugin converts the
      signal into volts. With "Peak of your playing (auto)" the loudest
      peak it has heard corresponds to the chosen voltage (default 300 mV);
      click the line under the INPUT knob of the strip to forget the peak.
      With the fixed reference 0 dBFS corresponds to that voltage. The
      output is converted back by the same factor, so unity gain through
      the circuit is unity gain in the DAW.
    Output level (dB): plain gain after the circuit.
    Pot taper of INPUT and OUTPUT (log 15 % as in the unit, log 10 %,
      linear).
  Defaults: Input 5, Output 5, Attack 4, Release 4, 4:1, meter GR, all
  extensions off, 300 mV. With these the loudest peaks get about 10 dB of
  gain reduction at roughly unchanged level.

User interface
  Own panel design (no photo, no logo): knobs to turn (drag vertically,
  mouse wheel, double-click = default), RATIO and METER buttons to click
  (pressed: dark with a yellow edge), VU meter. Below the panel the model
  strip with the extensions, a ten-LED gain reduction display per stage
  (2 dB per LED) and the gain reduction of the last four seconds.
  Window resizable. Needs libcairo2 and libX11.

How close it is
  Checked against the schematic and the Universal Audio manual in
  engine/tests/u76test.cpp:
    DC voltages at 25 test points of the schematic: within 0.1 to 0.3 V
      (output amplifier +0.45 V)
    "Q" bias: 1.0 dB attenuation of the FET at rest (adjusted that way)
    Frequency response 20 Hz ... 20 kHz: +0 / -0.35 dB (manual: +-1 dB)
    Ratios with a steady tone: 3.4 / 7.1 / 11.4 / 21.2 for 4 / 8 / 12 / 20;
      all buttons about 12:1 with a shifted operating point
    Release at the hold capacitor: 55 ms ... 0.9 s (manual: 50 ms ... 1.1 s)
    Attack at the hold capacitor: 0.15 ... 0.27 ms (fast), 1.8 ... 3.2 ms
      (slow), depending on how far the signal is above threshold (manual:
      20 ... 800 us, no measuring condition given)
    Distortion at 20 dB gain reduction: 0.1 ... 0.2 %
  Things to know:
    - Maximum gain is 36.7 dB. The 45 dB of the data sheet belong to the
      revisions with input transformer; this revision's input stage has
      gain 1.
    - The gate of the FET receives half of the control voltage at once and
      the rest with a time constant of about one second (R7, R9, C5 in the
      schematic). After short peaks the gain recovers quickly, after long
      compression more slowly, and a steady tone settles over a few
      seconds. This follows from the schematic; it was not measured on a
      unit.
    - After switching the ratio the circuit needs one to two seconds until
      the hold capacitor has settled, like the unit. After a pause (FX
      off and on, transport restart) the plugin starts from the settled
      state for the current settings.
  Assumed (not in the schematic): semiconductor data from data sheets
  (BF245A: pinch-off -2 V, 4 mA), forward curve of the FD333 rectifier
  diodes like a 1N4148 with 0.1 nA leakage (data sheet: at most 3 nA at
  125 V), supply as DC voltages +30.3 V and -10 V, ideal output transformer
  (1:1, 10 kOhm load), DIST trimmer at center, the network "T+C" not fitted
  (factory selected, no value given). No unit was measured.
  The extensions are an own design: the high-pass filter sits between the
  preamplifier and the ratio ladder (buffer, 2nd-order Sallen-Key, buffer),
  so the ladder and its load stay as drawn.

CPU and latency
  The whole circuit is solved in every sample, without oversampling and
  without latency. About 15 to 25 % of one core per channel at 48 kHz, twice
  that with the second stage, twice that again at 96 kHz. Stereo tracks are
  computed as two independent channels (no stereo link). Aliasing: mirrored
  harmonics of a 7 kHz tone stay below -60 dB, with all buttons and the
  fastest times below -54 dB. After 2 s of silence at the input the plugin
  goes idle.

Changes
  1.0  First version.
