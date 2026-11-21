Viereck 34 Tilt (after Quad 34) - CLAP and VST3 plugin for REAPER and other hosts (Linux and Windows x86-64)
Version 1.1

Two formats, one engine
  Viereck_34.clap and Viereck_34.vst3 contain the same code and sound
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
  mkdir -p ~/.clap && cp Viereck_34.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Viereck 34 Tilt (after Quad 34)".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Viereck_34.vst3 ~/.vst3/
  (Viereck_34.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  The tone section of the Quad 34 preamplifier (1982-1996), computed from
  the circuit diagram M12746 (with component values) and the Quad service
  data: the "Ambler" tilt stage around IC12 with the 7-position TILT switch
  (+-1, +-2, +-3 dB, pivot about 850 Hz), the BASS switch (three Step
  positions that cut the lowest octaves by about 5.5 dB below 300 / 170 /
  100 Hz, and three Lift positions +3 / +6 / +9 dB that raise the deep bass),
  the BALANCE pot and the filter stage around IC19 with F1, F2 and SLOPE
  (6 dB/octave RC filter, with SLOPE 12 dB/octave Sallen-Key), the 4066
  analog switches and the output divider (rated output 0.5 V into 830 ohm).
  Both op-amps, all resistors, capacitors and switches are simulated in the
  time domain (nodal analysis), so the op-amps clip at their supply rails
  when driven too hard. "Quad" is a trademark of its owner; this plugin is
  independent. Details: docs/viereck/SCHALTPLAN.md in the repository.

  Checked against the service data and measurements of a real unit
  (engine/tests/viereck_test.cpp):
    - TILT at 100 Hz and 10 kHz within 0.12 dB of Fig. 3, pivot 847 Hz
    - BASS LIFT within 0.41 dB of the measurement at 82.5 Hz and at the
      maxima (+12.3 / +14.0 / +15.2 dB at 20 / 27 / 37 Hz)
    - BASS STEP -5.3 ... -5.6 dB at 10 Hz (measured: 5.5 dB)
    - filters within 1 dB of Fig. 10 at 18/20 kHz; flat within 0.24 dB
      from 20 Hz to 20 kHz with everything off
    - BALANCE from the center +2.5 dB at one end and -65 dB at the other
      (service test procedure)

  Estimated (not in the documents):
    - op-amp TL071 as a controlled source (gain 10^4, 100 ohm output),
      clipping at +-7.5 V (supplies +8.6 / -9.4 V); 4066 switch 120 ohm on
    - the BALANCE pot: its value is neither in the diagram nor in the parts
      list. 5 kohm, fitted to the filter curves of Fig. 10 (they depend on
      the source resistance of the pot). Its law follows the test
      procedure: in the center 2.5 dB below the loud end, between them even
      in dB; towards the quiet end quadratic down to -65 dB.

  Additions:
    - TILT CONTROL STEPS / SMOOTH: STEPS has the 7 positions of the
      original; SMOOTH moves the wiper continuously along the same resistor
      chain.
    - TILT PIVOT 80 Hz ... 4 kHz: the four capacitors of the tilt network
      are scaled together, so the curve shifts without changing its shape
      (within 0.18 dB from 300 Hz to 3 kHz). The gold mark on the scale is
      the original (847 Hz).
    - LEVEL and OUTPUT.

Parameters
  Tilt (-3 ... +3 dB, + brighter, - warmer), Bass (Step 300 Hz, Step 170 Hz,
  Step 100 Hz, 0 dB (off), Lift +3 / +6 / +9 dB), Balance (-1 left ... +1
  right; the right channel gets louder to the right, the left one quieter),
  Filter (Off / Filter 1 / Filter 2), Filter slope 12 dB/oct (only with a
  filter, as on the unit), Tilt control (Steps / Smooth), Tilt pivot
  (80 Hz ... 4 kHz), Level: 0 dBFS (dB re rated output) (-30 ... +10 dB): a
  0 dBFS sine at the input gives LEVEL dB re 0.5 V rms at the output with
  everything flat (default -10 dB; raise it to drive the op-amps into
  clipping); Output level (-24 ... +24 dB).
  One set of controls for both channels; BALANCE works in opposite
  directions on the two channels like the dual pot RV4. With everything flat
  and BALANCE in the center the plugin passes the signal 1:1 (same level,
  same polarity).

User interface
  Own panel design (no photo, no logo): BASS and TILT as rotary switches
  with 7 detents (TILT clockwise = warmer, as on the unit), BALANCE, the
  buttons F1, F2 and SLOPE with red lamps (F1 and F2 exclude each other,
  clicking a lit button switches it off), and on the right TILT PIVOT and
  the lever STEPS / SMOOTH. Knobs: drag vertically, mouse wheel,
  double-click = default. Going back to STEPS rounds TILT to the nearest
  position. Below the panel: drive of the two op-amps (IC12, IC19), the
  settings as text, LEVEL and OUTPUT; below that the frequency response of
  the tone section (small signal, computed from the circuit; dashed: flat).
  Window resizable. Needs libcairo2 and libX11.

Switching and latency
  Switches act at once (the circuit is rebuilt, the capacitor charges
  stay); TILT positions, PIVOT and BALANCE glide over 15 ms, so turning
  TILT across several positions does not click. No latency.

CPU
  About 2 % of one core at 48 kHz in stereo. After 2 s of silence at the
  input the plugin goes idle.

Changes
  1.1  Windows version (VST3 and CLAP with the same front panel, installer and
       ZIP). Project state and values typed into the host no longer depend on
       the number format of the system (decimal comma). Windows: the window
       follows a change of the display scaling while it is open, and the mouse
       wheel also works when Windows sends it to the focused window. Packages
       now include the GPL licence text (LICENSE.txt). Sound unchanged.
  1.0  First version.
