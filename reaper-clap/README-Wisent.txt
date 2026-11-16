Wisent Echoreck (after Binson Echorec PE 603 T) - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.2

New in 1.2: WOBBLE (a worn drum whose disc does not run true) as a model
control. Default 0 = as in 1.1 (bit for bit); projects saved with 1.0 or
1.1 load unchanged.

New in 1.1: DRY (the trimmer TD, dry level) and VARISPEED (drum speed
0.25x to 2x, a later modification) as model controls; VOLUME default 10;
trimmer TSW fully open (SWELL louder); V3A as a real tube (limits the echo
when the loop runs away). Projects saved with 1.0 load unchanged (the new
controls take their defaults).

Two formats, one engine
  Wisent_Echoreck.clap and Wisent_Echoreck.vst3 contain the same code and
  sound identical. Use whichever your DAW prefers. CLAP is the native
  format, the VST3 is built from it with clap-wrapper (free-audio, MIT) and
  the VST3 SDK 3.8 (Steinberg, MIT). A project saved with one format does
  not load its settings into the other.
  Third-party licenses: THIRD-PARTY-LICENSES.txt.

Updating
  Close REAPER first. Copying a new .clap over the file a running REAPER has
  loaded makes REAPER crash (at once or a while later). scripts/update-
  tonestack.sh refuses to run while REAPER is open and never overwrites a
  plugin file in place.

Installation (CLAP)
  mkdir -p ~/.clap && cp Wisent_Echoreck.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Wisent Echoreck (after Binson Echorec PE
  603 T)".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Wisent_Echoreck.vst3 ~/.vst3/
  (Wisent_Echoreck.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  A tube echo with a magnetic drum, modelled after the schematic of the
  Binson Echorec PE 603 T (a redrawing of the factory schematic). The
  signal is written onto steel wire on a rotating drum; four playback heads
  34 mm apart read it 74, 148, 222 and 297 ms later (50 Hz mains), a
  permanent magnet erases before the next turn. "Binson" and "Echorec" are
  trademarks of their owners; this plugin is independent and no
  measurement of a unit.
    Tubes      input stage V4A, mixer V5A/V5B (ECC83) and record amplifier
               V6A (ECC82) as a time-domain circuit with the part values of
               the schematic (record side at twice the sample rate up to
               48 kHz).
    Drum       record current with bias as a 12 kHz low-pass and a soft
               saturation of the wire (assumed: about 9.5 dB above the
               level in the schematic); playback as the derivative with
               spacing loss (31 um effective, assumed) and gap loss (5 um);
               erase residue -50 dB, wire noise, wow 0.05 % per turn
               (assumed). Speed 457 mm/s at 50 Hz, 548 mm/s at 60 Hz.
    Playback   one ECC83 stage per head, switch matrix, passive tone control,
               VOLUME and the cathode follower V4B (linearized at their
               operating points: millivolt levels); V3A as a tube (cathode
               1.40 V as in the schematic), it clips when the loop runs away.
    Matrix     the schematic shows the contacts without rest position. The
               model reads it as: lower row = heads to the output, upper
               row = heads into the feedback; ECHO without feedback, REPEAT
               with, SWELL = all four heads to the output (trimmer TSW fully
               open assumed: each head still about 4 dB quieter) plus
               feedback. Through the swell bus, switched-off heads leak in at
               about -36 dB against the echo.

Controls
  On the unit:
  INPUT          record level (log taper). The magic eye shows the record
                 current against the saturation of the wire: when the two
                 bands meet, the wire saturates. Default 2.5.
  LENGTH OF SWELL  feedback. With one feedback head the loop starts to
                 oscillate at about 8.9 (50 Hz) or 7.6 (60 Hz: less
                 spacing loss); with VARISPEED at 2x already at about 3.1,
                 below about 0.75x not at all. A runaway loop is loud (up
                 to about +8 dBFS peak, limited by V3A). It never goes fully to zero (47 k below
                 the pot): in REPEAT there is always a second repeat at
                 about -15 dB.
  VOLUME         echo level (log taper). Default 10. INPUT only feeds the
                 drum: with INPUT set so the eye just closes, the echo at
                 VOLUME 10 is about (-11 dB minus the input peak in dBFS)
                 against the dry signal; use DRY to take the dry back.
  BASS, TREBLE   passive tone control of the echo; clockwise = more. The
                 middle position is not flat.
  ECHO / REPEAT / SWELL, head buttons: see "Matrix" above.
  Model (not on the unit):
  LEVEL          the voltage at the input for 0 dBFS (20 mV to 2 V peak,
                 default 200 mV, about a guitar).
  OUTPUT         -24 to +24 dB. The output is calibrated so the dry signal
                 keeps its level at 1 kHz.
  MAINS 50/60 Hz drum speed (shaded-pole motor follows the mains); the
                 change glides in about 0.4 s.
  DRY            the trimmer TD inside the unit (dry level), 0 to 10, plan
                 setting 6.7; 0 = echo only. The output is calibrated at 6.7.
  VARISPEED      drum speed 0.25x to 2x (log taper, default 1x). The
                 PE 603 T has a fixed-speed motor; variable speed was a later
                 modification with a new motor. Spacing loss and record
                 losses follow the wavelength: slower is duller and stops
                 self-oscillating, faster is brighter and runs away earlier.
                 Head 4 from 1.19 s (0.25x) to 149 ms (2x) at 50 Hz.
  WOBBLE         0 to 10, default 0. A worn drum: the disc does not run
                 true, and the spacing between wire and heads grows once
                 per turn by up to 30 um (WOBBLE 10) and goes back again
                 (sinusoidal). The disc is rigid, so every spot of the wire
                 keeps its spacing: it is recorded with it and read with
                 it, by each head at its own time. Spacing loss after
                 Wallace, assumed for recording as well as playback; at
                 50 Hz and WOBBLE 10 the worst spot loses 7.2 dB at 1 kHz
                 and 21.5 dB at 3 kHz, the bass hardly anything. You hear a
                 slow rise and fall of level and treble of the echoes in
                 time with the drum (0.82 s at 50 Hz); every echo and every
                 repeat meets another spot. The speed does not change. The
                 frequency response and echo pattern in the page show the
                 true-running drum. Up to about 7 % of a core more CPU at
                 WOBBLE 10.

Measured (engine/tests/wisenttest.cpp)
  Operating points against the schematic: V4A 68 V (80), mixer 113 V
  (130), V6A 130.5 V (130). Echo times 74.4 / 148.8 / 223.2 / 297.6 ms plus
  latency (60 Hz: 62 / 124 / 186 / 248 ms). Feedback with one head, second
  against first repeat: -15 dB at LENGTH OF SWELL 0, -4 dB at 5, 0 dB at 9.
  Small-signal echo pattern against the time domain within 0.1 dB.
  Aliasing under normal drive below -68 dB.

Mono, latency, CPU
  Mono in (a stereo input is summed), mono out on both channels. Latency 3
  samples up to 48 kHz, 0 at 88.2/96 kHz; from 176.4 kHz the whole unit
  runs at half the rate (latency 8 samples), from 352.8 kHz at a quarter
  (22). The plugin reports its tail (ECHO 0.4 s, with feedback longer;
  endless while the loop oscillates). CPU at 48 kHz about 15 to 30 % of one
  core; when the input is silent and the echoes have died away, the plugin
  sleeps.

Interface
  The front panel is an own drawing (no photo, no logo): magic eye, INPUT,
  LENGTH OF SWELL, VOLUME, BASS, TREBLE, ECHO/REPEAT/SWELL, the head
  buttons (upper row feedback, lower row playback, with the delay of each
  head), and the model controls DRY, VARISPEED, WOBBLE, LEVEL, OUTPUT and
  the mains switch. Below
  it: mode, active heads, record level and drum speed.
  Drag a knob up/down, mouse wheel for fine steps, double-click = default.
  Click a button to press or release it.
