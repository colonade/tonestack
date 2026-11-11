GK 800RB Bass Amp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.7

Two formats, one engine
  GK_800RB.clap and GK_800RB.vst3 contain the same code and sound
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
  mkdir -p ~/.clap && cp GK_800RB.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "GK 800RB Bass Amp".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r GK_800RB.vst3 ~/.vst3/
  (GK_800RB.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  A rebuild of the Gallien-Krueger 800RB bass amplifier after the service
  manual (preamp schematic revision C of 1991, power amp schematic, test
  procedure of 8/19/91). The whole preamp is solved sample by sample as one
  circuit (nodal DK method, like the other tonestack amps):
    - input stage U1a (non-inverting, 33k with 560 pF / 3.3k) with the -10 dB pad
      (4.7k in parallel) and LO CUT (0.22 uF in series) as switches,
      VOLUME (50k log taper) with HI BOOST (10 nF via 5.6k on the wiper)
    - MID CONTOUR: the notch filter around U1b (about 500 Hz), switched
      in or bypassed
    - U2a and the four bands: HI MID around U2b, LO MID around U3a (both
      bridged-T, 1 kHz / 250 Hz), BASS and TREBLE around U3b (60 Hz /
      4 kHz), with every capacitor and resistor of the board; effects loop
      bridged (SEND to RETURN)
    - the two 1N759 zener diodes on the signal, DIRECT OUT, the BOOST
      control with the boost switch (footswitch; off = the fixed tap below
      the pot) and the J113 JFET stage that gives the boost its sound
    - the bi-amp crossover (state-variable filter U6/U7, FREQ. tandem pot,
      crossover about 80 Hz - 1 kHz) and the HI MASTER / LO MASTER levels, or full
      range with both amps in parallel when BI-AMP is off
  Op-amps are modelled as voltage-controlled sources with finite gain,
  output resistance and the +-13.5 V supply clamps, so hard clipping of a
  stage sounds like the real thing. The two power amps (300 W low /
  100 W high) are linear blocks with the feedback network of the schematic
  (gain 61.7) and a soft clip at their rail limits (about 49 V and 40 V
  peak into the load). Same engine as the web simulator
  (https://borg.fluxstatus.de/tonestack/gk-800rb/).
  Checked against the manual: the test levels printed on the schematic
  (2 mV at 200 Hz, all knobs at 10) are met within about 1 dB at each
  stage, the -10 dB pad and BOOST 0 ratios of the test procedure within
  0.5 dB, the crossover range 100 Hz - 1 kHz, and the shapes of the 16
  scope photos (200 Hz square wave) at the measuring points.
  Estimated (not in the manual): J113 parameters (Vp -1.75 V, middle of the
  data sheet range; Idss 22 mA fitted so that the boost stage amplifies
  93 mV -> 1.1 V as the schematic says),
  the exact log taper of VOLUME, the clip levels and knee of the power
  amps, the speaker. The absolute sensitivity may be off by a few dB.

Parameters
  -10 dB input pad, Volume, Lo Cut, Mid Contour, Hi Boost, Treble, Hi Mid,
  Lo Mid, Bass, Boost, Boost switch (footswitch: on / off = preset level),
  Bi-Amp (full range / bi-amp), Bi-Amp frequency, Hi Master (100 W amp),
  Lo Master (300 W amp), Power amps (linear / clipping), Speaker,
  Instrument level, Input reference, Oversampling (1x, 2x, 4x), Output level, Output
  (speaker outputs / direct out), Volume pot taper (log 15 % / log 10 % /
  linear).
  Knob 10 = clockwise end. The mid bands only cut (0 dB at 10, about -6 dB
  at 5, -15 dB at 0), as on the amp. BASS ranges from about +6 to -23 dB,
  TREBLE about +21 / -11 dB at 8 kHz.
  Defaults: all knobs 5, pad / Lo Cut / Contour / Hi Boost out, Boost
  switch on, full range, power amps clipping, speaker off, 100 mV, 2x,
  0 dB, speaker outputs.

User interface
  Front panel drawn after the layout of the amp (no photo, no logo): knobs
  to turn (drag vertically, mouse wheel, double-click = 5), the push
  buttons of the amp as switches to click (-10 dB, LO CUT, CONTOUR,
  HI BOOST, BOOST on/off, BI-AMP), BOOST and POWER LEDs. Below the panel
  the model strip (not on the amp): SPEAKER, OVERSAMPLING, OUTPUT
  (SPEAKER / DIRECT), POWER AMPS (CLIP / LINEAR), the DRIVE meters (swing
  of the input stage, the hardest driven EQ stage and the J113 relative to
  their limits, 100 % = clipping; output power of the 300 W and 100 W amp
  into 4 / 8 ohm, mark = rated power), INPUT (instrument level) and
  OUTPUT. Below that the small-signal frequency response at the operating
  point (relative to 1 kHz; dashed: all knobs at 5, switches off). Window
  resizable. Needs libcairo2 and libX11.

Levels
  INPUT sets the peak voltage of your playing at the jack (see "Input
  level" below) (default 100 mV; passive bass about 100-300 mV, active up to 1 V).
  The output is the sum of the two power amp voltages at the speaker
  terminals (or the sound pressure with SPEAKER on, equal at 1 kHz),
  60 V peak = 0 dBFS (300 W into 4 ohm is 49 V peak). DIRECT out is the
  signal after the EQ (before the boost stage), 2 V peak = 0 dBFS.

Speaker
  SPEAKER adds a 15" bass speaker in a vented box (estimated Thiele-Small
  data of an Eminence CB158, as in the Laney), plus a cone roll-off.

Oversampling and latency
  1x is enough for clean and moderately driven playing; 2x (default) or 4x
  reduce aliasing when the J113 or the op-amps clip hard and cost two or
  four times the CPU. Changing it prepares a new engine in the background
  (the amp settles first) and switches over after a moment ("switching ..."
  on the toggle). The reported latency is always 31 samples (the
  oversampling filters; at 1x the plugin delays by the same amount), so it
  does not change with the setting.

CPU
  About 8 % of one core at 48 kHz and 1x on a typical desktop, about 18 %
  at 2x and 35 % at 4x (the whole preamp is solved every sample); stereo
  with different left and right signals doubles that. With the same signal
  on both channels it is computed once. After 2 s of silence at the input
  the plugin goes idle.

Input level (INPUT, since 1.4)
  INPUT is the peak voltage of your playing at the input jack. The plugin
  remembers the highest input peak it has seen and scales the track so
  that this peak arrives at the amp with the set voltage: a track that
  peaks at -12 dBFS and one that peaks at -3 dBFS drive the amp the same.
  The line below the INPUT knob shows the remembered peak ("at your peak
  (-11.7 dBFS)"); it is saved with the project, so the project sounds the
  same when opened again and when rendered. Click the line to forget it
  (e.g. after a louder or quieter take). It only grows: until the loudest
  note has been played once, earlier notes get more level; while it grows
  during the very first loud note, that note's rising edge is limited to
  the set voltage. Below -40 dBFS nothing is raised further.
  Typical values: passive bass about 100-200 mV on a hard attack, active
  bass up to 1 V; single-coil guitar about 100-200 mV, humbucker or active
  pickups up to 1 V. Default 100 mV.
  Projects saved with an earlier version keep the old meaning (INPUT = the
  voltage at 0 dBFS of the track) and sound as before; the line then reads
  "at 0 dBFS (old)", a click switches to the new behaviour (parameter
  "Input reference").

1.6
  - Turning knobs costs less CPU: when only resistor values change (pots),
    the circuit matrix is updated with rank-one corrections instead of being
    inverted again; a full inversion every 16 updates keeps the rounding in
    check. Load while turning three knobs: about 0.18 -> 0.13 of a core (2x,
    block 128). Sound with fixed settings bit-identical; while turning the
    difference to a full inversion is below -80 dB.
  - Switches no longer click: -10 dB pad, LO CUT, MID CONTOUR, HI BOOST,
    BOOST footswitch, BI-AMP and the pot taper used to switch the circuit
    from one sample to the next, which made the output jump. Now the output
    fades out over 3 ms, the switch happens in the silent spot, and after
    the plugin latency (31 samples) it fades in over 4 ms. Measured with a
    sine: the click is down from up to +4 dB to about -75 dB relative to the
    signal (the level of the fades themselves). Speaker and DIRECT OUT
    already crossfade inside the amp (20 ms).

1.5
  - No more dropout and click when the circuit is switched (oversampling):
    the old circuit fades out over 3 ms, the new one fades in, and the old
    one is freed outside the audio thread. Before, the switch could take
    long enough for a dropout at small buffers (128 samples) and the output
    jumped.
  - Restart after a pause (Play, FX switched on) costs about a third less
    time in its first block: only the left-channel amp is set up, the right
    one is copied from it when the channels differ.

1.4
  - INPUT now means the peak voltage of your playing: the plugin remembers
    the highest input peak and scales to it (see "Input level"). Before, it
    was the voltage at 0 dBFS of the track, so a track peaking at -12 dBFS
    reached the amp with only a quarter of the set voltage. Old projects
    keep the old behaviour and sound bit-identical.

1.3
  - Rebuilt with the shared solver code of the SVT-CL 1.4 (matrix of the
    Newton step built per device block). Sound bit-identical to 1.2.

1.2
  - Faster solver (dense matrix products): about 4-9 % less CPU. Sound
    unchanged apart from rounding and solver tolerance (difference below
    -105 dB).
  - Fixed: after inserting or enabling the plugin on a record-armed track
    it stayed silent for live input (meters dead) until Play was pressed
    once. The idle mode after silence is now internal only; the plugin no
    longer asks the host to stop calling it.

1.1
  - New parameter "Volume pot taper" (POT TAPER toggle in the model strip):
    taper of VOLUME (50KA): LOG 15 % (as on the amp, default), LOG 10 % or
    LINEAR; the other pots are linear (B) on the schematic and stay linear.
    Old projects load unchanged. Same selector on the web page.
  - J113 fitted to the schematic's level label (93 mV -> 1.1 V): Vp -1.75 V,
    Idss 22 mA (was 12 mA / -2 V); C7 560 pF in the input stage feedback
    added after a second check against the schematic.

1.0
  - First release.

VALUE POPUP (new in 1.7)
  In the header line of the model strip, "VALUE POPUP ON / OFF" switches the
  small value display that appears at a knob while the mouse is over it or
  while turning. Click to toggle. The choice is stored for all tonestack
  plugins of this user (Linux: ~/.config/tonestack/ui.conf, macOS:
  ~/Library/Application Support/tonestack/ui.conf), not per project.
  No change in sound.

Notes
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer).
  Gallien-Krueger and 800RB are trademarks of their owners; this is an
  independent circuit simulation, not affiliated with or endorsed by them.
