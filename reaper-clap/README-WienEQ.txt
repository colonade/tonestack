Wien EQ (after adt-audio TM205) - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.6

Two formats, one engine
  Wien_EQ.clap and Wien_EQ.vst3 contain the same code and sound identical.
  Use whichever your DAW prefers. CLAP is the native format, the VST3 is
  built from it with clap-wrapper (free-audio, MIT) and the VST3 SDK 3.8
  (Steinberg, MIT). A project saved with one format does not load its
  settings into the other.
  Third-party licenses: THIRD-PARTY-LICENSES.txt.

Updating
  Close REAPER first. Copying a new .clap over the file a running REAPER has
  loaded makes REAPER crash (at once or a while later). scripts/update-
  tonestack.sh refuses to run while REAPER is open and never overwrites a
  plugin file in place.

Installation (CLAP)
  mkdir -p ~/.clap && cp Wien_EQ.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Wien EQ (after adt-audio TM205)".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Wien_EQ.vst3 ~/.vst3/
  (Wien_EQ.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  A five-band stereo equalizer after the published description of the
  adt-audio ToolMod TM205 stereo mastering equalizer. No schematic of the
  TM205 is public and no unit was measured: this is an own design that
  follows the published figures, not a circuit rebuild. "adt-audio" and
  "ToolMod" are trademarks of their owner; this plugin is independent.
  From the description:
    - MID 1 / MID 2 / MID 3 fully parametric (gain, Q, frequency) after the
      principle of the active Wien-Robinson bridge: 20 Hz - 2 kHz,
      60 Hz - 7 kHz, 250 Hz - 25 kHz, frequency controls "logarithmically
      stepped"; bandwidth from about 5 octaves (technical data: more than
      3 octaves) to a third of an octave
    - LOW: active RC filter, soft bell, 6 dB/octave or less, at 40 Hz; two
      buttons select 60, 100 and 160 Hz
    - HIGH: active RC filter; shelf at the 20 kHz setting (no button),
      otherwise a very soft bell at 12, 8 or 4 kHz (two buttons, 6 dB/octave
      at most)
    - range +-12 dB, versions with +-6 and +-20 dB (switch RANGE)
  From the front panel (photo): seven scale marks at equal angles on every
  knob; gain (+-12 dB version) 4 / 9 / 12 dB at the 1st / 2nd / 3rd mark;
  MID 1 20/30/50/120/240/600/2000 Hz, MID 2 60/100/150/400/700/2000/7000 Hz,
  MID 3 0.25/0.4/0.7/1/2/10/25 kHz; HIGH buttons 8k (left) and 12k (right).
  Assumed (not published): every mid band has the shape of the ideal Wien
  bridge, a 2nd-order band-pass B(s) = (s/Qw) / (1 + s/Qw + (s/w)^2) with
  peak 1; boost H = 1 + G B, cut exactly the inverse 1 / (1 + G B) (band-
  pass in the feedback path), G = 10^(|dB|/20) - 1, so the bandwidth does
  not change with the gain and equal boost and cut cancel. Q of the mid
  bands 0.18 (5 octaves) to 4.32 (1/3 octave) between the -3 dB points of
  the band-pass, log taper. The LOW bells and the HIGH bells (buttons) are
  2nd-order band-passes with Q = 1/3 (passive RC network with equal parts,
  6 dB/octave slopes); ADT gives no Q, and that the bandwidth changes with
  the buttons is not quantified, so it is not modelled. HIGH without a
  button: high H = (sqrt(g) s + w) / (s / sqrt(g) + w), g = 10^(|dB|/20),
  cut = exact inverse, 20 kHz = middle of the slope (+12 dB: +6 dB at
  20 kHz, +2.7 dB at 10 kHz). Knob laws follow the panel scales, monotone
  cubic between the marks: gain 4 / 9 / 12 dB at the marks (scaled for
  +-6 / +-20 dB); frequency after the MID 1 and MID 2 scales, which agree
  (ADT: the three bands are identical apart from the range), center 123 Hz
  / 391 Hz / 1.53 kHz. The printed MID 3 scale (1 kHz at the center) does
  not fit the other two and is off by up to 0.6 octaves from this law.
  The HIGH buttons follow the panel labels (8k left, 12k right); ADT's
  control description lists them the other way round. If you own a TM205, a measurement would show where the real unit
  differs; the curves can then be adjusted.

Parameters
  Low: Gain, Mid 1/2/3: Gain, High: Gain (-5 ... +5, 0 = flat; the knob law
  above); Mid 1/2/3: Q (0 = 5 octaves ... 10 = 1/3 octave), Frequency
  (0 ... 10 over the band's range, the knob law above); Low: Frequency
  (40 / 60 / 100 / 160 Hz bell = the two buttons); High: Frequency (20 kHz
  shelf / 12 / 8 / 4 kHz bell; switching crossfades over 20 ms);
  EQ on (on / off, crossfaded over 20 ms with an equally delayed dry signal);
  Range (+-6 / +-12 / +-20 dB); Output level (-24 ... +24 dB, model, not on
  the unit). One set of controls for both channels, as on the unit.
  Defaults: all gains 0, Q and frequency at center, no button pressed, EQ
  on, +-12 dB, output 0 dB.

User interface
  Own panel design (no photo, no logo, not a copy of the ADT front panel)
  with the controls of the description, scale marks and button order as on
  the unit: knobs to turn (drag vertically,
  mouse wheel, double-click = center; the gain knobs have a center detent),
  buttons to click (pressed: dark with a yellow edge), EQ ON with a green
  lamp (clicking the lamp switches too). Below the panel the model strip
  (not on the unit): RANGE, the five bands as text (frequency, Q, bandwidth,
  dB), OUTPUT; below that the frequency response (computed from the analog
  transfer functions; reference line: flat = EQ off). Window resizable.
  Needs libcairo2 and libX11.

Accuracy and latency
  Each band is one second-order section with the poles of the analog filter
  and a numerator matched to the analog magnitude (after M. Vicanek,
  "Matched Second Order Digital Filters", 2016). Below 80 kHz the engine
  runs at twice the sample rate (own half-band filter, flat to 0.005 dB up
  to 20 kHz), so MID 3 up to 25 kHz and the HIGH filter behave like the
  analog ones within the audio band. Measured against the analog curves
  (engine/tests/weqtest.cpp, impulse responses at 44.1, 48 and 96 kHz): at
  most 0.02 dB. Latency 63 samples at 44.1/48 kHz (reported to the host),
  0 from 88.2 kHz. Moving a knob glides over 20 ms without clicks.

CPU
  About 2 % of one core at 48 kHz in stereo, 0.3 % at 96 kHz. After 2 s of
  silence at the input the plugin goes idle.

Changes
  1.6  The lettering "MODEL (not on the unit)" in the header line of the model
       strip is gone; the plugin version stays there. Sound unchanged.
  1.5  The value popup at the knobs is gone (it appeared while turning a
       knob or with the mouse over a control); with it the switch VALUE POPUP.
       BAND LIST now sits where VALUE POPUP was. Sound unchanged.
  1.4  As on the original (front panel, description, technical data):
       LOW is a soft bell (Q 1/3) at 40 / 60 / 100 / 160 Hz; HIGH is a
       shelf at 20 kHz without a button and a soft bell (Q 1/3) at 12 / 8 /
       4 kHz with the buttons (1.2/1.3: shelves everywhere). Gain and
       frequency knobs follow the panel scales: at the center MID 1 / 2 / 3
       are now 123 Hz / 391 Hz / 1.53 kHz (were 200 Hz / 648 Hz / 2.5 kHz),
       4 / 9 / 12 dB at the scale marks (+-12 dB). Panel: seven scale marks
       like the unit, MID 2/3 in kHz, buttons side by side (LOW 60 | 100,
       HIGH 8k | 12k). Projects keep their knob positions, so frequencies
       and gains of saved settings change.
  1.3  Two switches in the header line of the model strip: VALUE POPUP (the
       value display at the knobs) and BAND LIST (the list of all band
       values) can be switched off. Stored for all tonestack plugins of
       this user (Linux: ~/.config/tonestack/ui.conf, macOS: ~/Library/
       Application Support/tonestack/ui.conf), not per project. The list
       no longer overlaps the version text.
  1.2  LOW and HIGH are shelving filters in every button position (were
       bells with Q 1/3); HIGH without a button is a shelf at 20 kHz. The
       sound of LOW and HIGH changes; projects keep their settings.
       Larger numbers on the knob scales and button labels.
  1.1  The value shown while turning a mid band's Q knob used a wrong range
       (0.33 ... 4.32 instead of 0.18 ... 4.32). The filter itself, the band
       text in the model strip and the curve were right; sound unchanged.
  1.0  First version.
