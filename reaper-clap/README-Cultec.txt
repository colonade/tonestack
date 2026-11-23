Cultec EQ-1A Program Equalizer (after Pultec EQP-1A) - CLAP and VST3 plugin for REAPER and other hosts (Linux and Windows x86-64)
Version 1.1

Two formats, one engine
  Cultec_EQ1A.clap and Cultec_EQ1A.vst3 contain the same code and sound
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
  mkdir -p ~/.clap && cp Cultec_EQ1A.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Cultec EQ-1A Program Equalizer (after
  Pultec EQP-1A)".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Cultec_EQ1A.vst3 ~/.vst3/
  (Cultec_EQ1A.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  The Pultec EQP-1A program equalizer (Pulse Techniques), computed as one
  circuit in the time domain: input transformer Triad HS-56 (terminated
  with 620 ohm), the passive LC network, interstage transformer Triad
  HS-29 (1:2 with center tap, Zobel network 270 pF + 43 kohm), a 12AX7 in
  push-pull (plates 200 kohm, cathodes joined by 820 ohm), coupling
  0.25 uF, a 12AU7 in push-pull (cathodes 4.7 kohm || 50 uF), output
  transformer Peerless S-217-D with its feedback winding returning to the
  12AX7 cathodes through 360 ohm, and the power supply. The amplifier
  values are from the factory drawing DWG E-72,658-2; the passive network
  of the EQP-1A only exists as transcriptions (recproaudio 1997, Gyraf
  2001) - where they differ, the values that match the factory curves are
  used. "Pultec" is a trademark of its owner; this plugin is independent.
  Details: docs/cultec/SCHALTPLAN.md in the repository.

  Second model EQP-1R (switch MODEL, from version 1.1): the older unit,
  computed from its factory drawings (network "Pultec EQP-1R Equalizer"
  and amplifier DWG E-112,353-7). Same circuit form, other values: low
  boost and atten at 30 / 60 / 100 Hz, high boost at 3 / 5 / 8 / 10 /
  12 kHz from a 50 mH + 100 mH coil that the KCS switch partly shorts (the
  peaks of the 8 and 10 kc positions lie at 9.1 and 11.1 kHz, as drawn),
  high atten fixed at 10 kHz (0.13 uF; it stays active in OUT, as drawn),
  12AY7 instead of 12AX7 (plates 180 kohm, cathodes joined by 750 ohm),
  Zobel network 350 pF + 22 kohm. Output transformer and feedback winding
  are the same part as in the EQP-1A. Quiescent voltages without fitting
  within 10 % of the drawing (which itself disagrees by about 10 %);
  overall gain +0.3 dB at 1 kHz without fitting. Estimated: the input and
  interstage transformers Peerless E-372-Q / G-212-Q (no data; the fitted
  Triad values with the leakage set so that the amplifier is flat
  +0 / -1 dB as Pultec specifies: -0.86 / +0.14 dB). No factory curves of
  the EQP-1R exist. Each model keeps its own CPS / KCS positions.

  Controls as on the unit:
    - LOW FREQUENCY BOOST and ATTEN (shelves at 20 / 30 / 60 / 100 Hz, the
      CPS switch). Turning both at once gives the well-known Pultec bass: a
      boost with a dip above it (the manual advises against it - everybody
      does it anyway).
    - HIGH FREQUENCY BOOST, a resonant peak at 3 / 4 / 5 / 8 / 10 / 12 /
      16 kHz (the KCS switch), with BANDWIDTH from SHARP (0) to BROAD (10).
    - HIGH FREQUENCY ATTEN, a shelf at 5 / 10 / 20 kHz (ATTEN SEL).
    - IN / OUT: OUT acts like all boost and attenuate controls at 0; the
      amplifier stays in the signal path (as on the unit).

  Checked against the factory documents (engine/tests/cultec_test.cpp):
    - quiescent voltages without any fitting: +346 / +328 / +292 / +253 V
      (drawing +345 / +325 / +290 / +250), 12AX7 plate 139.4 V (140),
      cathode 1.34 V (1.3), feedback tap 1.13 V (1.1), 12AU7 cathode
      38.7 V (40)
    - overall gain 0 dB with all controls at 0 (drawing: "0 dB"; this sets
      the turns of the feedback winding), flat within -0.95 / +0.24 dB
      from 20 Hz to 20 kHz (data sheet: +0 / -1 dB)
    - 13 factory curves (low boost and atten at all four frequencies, high
      atten at all three, high boost 3 kHz sharp and broad; 105 points):
      0.76 dB rms
    - high boost peaks, all seven frequencies: +18.0 ... +18.9 dB sharp,
      +10.4 ... +11.1 dB broad (factory curves about +18.2 / +10.3 dB)
    - distortion at +10 dBm into 600 ohm: 0.006 % (1 kHz), 0.022 % (50 Hz)
      (data sheet: at most 0.15 %); 0.09 % at +21 dBm (1 kHz)

  Fitted to the factory curves (no data sheets exist): inductance of the
  interstage transformer (317 H) and its core losses (41.5 kohm),
  inductance of the input transformer (38 H), the Q of the high-boost coil
  (20), and coil and capacitor of the 3 kHz position (204 mH / 13.2 nF
  instead of 150 mH / 18 nF in the transcription: the factory curve is
  narrower). Estimated: leakage and winding capacitances of the
  transformers (chosen so that the response meets the data sheet), the
  output transformer inductance (100 H; ratio 12.5 kohm : 600 ohm after the
  CineMag CM-S217D replacement), the "audio" taper of the two bass pots
  (10 % at mid rotation). Not simulated: transformer core saturation (no
  data), hum and noise.

Parameters
  Low frequency boost (0 ... 10), Low frequency atten (0 ... 10), Low
  frequency (20 / 30 / 60 / 100 Hz), High frequency boost (0 ... 10),
  Bandwidth (0 sharp ... 10 broad), High frequency (3 / 4 / 5 / 8 / 10 /
  12 / 16 kHz), High frequency atten (0 ... 10), Atten select (5 / 10 /
  20 kHz), Equalizer in (Out / In), Level: 0 dBFS = dBu at the input
  (0 ... +30 dBu, default +18: a 0 dBFS sine is +18 dBu at the input
  terminals; raise it to drive transformers and tubes harder),
  Oversampling (1x / 2x / 4x, default 4x; the internal rate is limited to
  about 200 kHz, so at 96 kHz "4x" runs 2x), Output level (-24 ... +24 dB),
  Model (EQP-1A / EQP-1R), EQP-1R low frequency (30 / 60 / 100 Hz), EQP-1R
  high frequency (3 / 5 / 8 / 10 / 12 kHz).
  One set of controls for both channels. With all controls at 0 the plugin
  passes the signal at the same level and polarity (the phase of the
  transformers and the latency remain).

User interface
  Own panel design (no photo, no logo): the knobs and switches in the
  layout of the unit. Knobs: drag vertically, mouse wheel, double-click =
  default (0, BANDWIDTH center). The selectors CPS, KCS and ATTEN SEL:
  click right of the center = next position, left = previous, or drag,
  mouse wheel. The lever IN / OUT: click. Below the panel: drive meters
  (output level in dBm with the +21 dBm maximum of the data sheet, grid
  swing of the 12AU7 and 12AX7 or 12AY7 relative to their bias), the settings as
  text, MODEL (lever: up EQP-1A, down EQP-1R, with its own panel), LEVEL,
  OVERSAMPLING and OUTPUT; below that the frequency response
  of the whole circuit (small signal at the operating point, 0 dB = all
  controls 0 at 1 kHz as in the factory diagrams; dashed: all controls 0).
  Window resizable. Needs libcairo2 and libX11 (Linux).

Switching and latency
  The selectors and IN/OUT act at once (the circuit is rebuilt, the
  capacitor charges stay); knobs glide over 15 ms. Changing OVERSAMPLING
  or MODEL builds a new circuit on the main thread (about 0.2 s) and switches to it
  when ready. Latency 64 samples in every setting (oversampling filters;
  at 1x the plugin delays by the same amount).

CPU
  At 48 kHz (measured on Linux): 4x about 35 % of one core per channel,
  2x about 16 %, 1x about 9 %. If both channels carry the same signal, the
  plugin computes it once. After 2 s of silence it goes idle.

Changes
  1.1  Second model EQP-1R (switch MODEL) from its factory drawings.
  1.0  First version.
