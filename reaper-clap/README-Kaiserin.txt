Kaiserin Reverb (after Empress Effects Reverb) - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 1.2

Two formats, one engine
  Kaiserin_Reverb.clap and Kaiserin_Reverb.vst3 contain the same code and
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
  mkdir -p ~/.clap && cp Kaiserin_Reverb.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Kaiserin Reverb (after Empress Effects
  Reverb)".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Kaiserin_Reverb.vst3 ~/.vst3/
  (Kaiserin_Reverb.vst3 is a folder - copy it as a whole.) Then "Re-scan".

What it is
  A stereo reverb with the controls of the Empress Effects Reverb pedal as
  described in its published user manual (firmware 6.41): MODE, DECAY, MIX,
  OUTPUT, LOW, HI, THING 1, THING 2 and the infinite hold of the SELECT
  footswitch. The Empress Reverb is a digital unit whose algorithms are not
  published, so this is an own design built from published methods, not a
  copy of its sound. "Empress Effects" is a trademark of its owner; this
  plugin is independent. Six modes (the original has twelve families):
    HALL       16-line feedback delay network (Jot/Chaigne), Hadamard
               matrix, decay per band (low/mid/high), input diffusion,
               early reflections. THING 1 modulation (up to 0.8 ms),
               THING 2 early reflection level.
    PLATE      plate reverb after Dattorro (JAES 45/9, 1997), delays scaled
               to the sample rate. THING 1 pre-delay (0 - 250 ms),
               THING 2 "bright early decay" (input bandwidth 2.5 - 20 kHz;
               HI sets the damping inside the tank).
    SPRING     three springs, dispersive waveguides (chains of stretched
               all-passes, Abel/Valimaki/Parker) - the same spring model as
               in the Laney AOR 50 and Lametta G100 plugins. THING 1 rattle
               decay, THING 2 rattle level (the part above 4.2 kHz).
    ROOM       short network with dense early reflections. THING 1
               pre-delay (0 - 120 ms), THING 2 early reflection level.
    MOD        network with strongly modulated delay lines (chorus-like).
               THING 1 rate (0.1 - 6 Hz), THING 2 depth (up to 4 ms).
    SPARKLE    hall with an octave up inside the feedback (shimmer).
               THING 1 sparkle level, THING 2 sparkle length (how long the
               high octaves ring).

Controls
  All knobs at 12 o'clock give a usable sound, as on the original.
  DECAY   mid-band decay time (RT60) per mode:
            HALL 0.6 - 20 s, PLATE 0.5 - 10 s, SPRING about 0.7 - 6 s,
            ROOM 0.15 - 4 s, MOD 0.6 - 20 s, SPARKLE 1 - 30 s
          (12 o'clock = geometric middle, e.g. HALL 3.5 s)
  MIX     fully left = dry only, fully right = reverb only, equal level at
          2 o'clock (7); up to there the dry signal stays at full level.
  OUTPUT  12 o'clock = unity, -20 to +20 dB.
  LOW     left of 12: low cut on the reverb (up to 600 Hz) and shorter low
          decay; right of 12: up to +6 dB at 200 Hz and up to twice the low
          decay.
  HI      left of 12: high cut on the reverb (down to 1.25 kHz) and darker
          decay; right of 12: up to +6 dB at 4 kHz and up to the full mid
          decay in the highs.
  HOLD    infinite hold: the input to the reverb is muted, the tail keeps
          sounding (lossless), the dry signal still passes.
  The hall is about as loud as the dry signal for noise at 12 o'clock
  (each mode measured), slightly louder with longer DECAY.

Measured (engine/tests/kaitest.cpp)
  Octave-band RT60 from the impulse response matches the setting within
  about 5 % at 125 - 500 Hz for HALL, ROOM, MOD, SPARKLE; PLATE within 10 %
  above 0.5 s; SPRING measures 10 - 25 % shorter than the knob value
  (losses in the tank), the display marks it "approx.". All modes stay
  stable with every knob at maximum and in HOLD; no non-finite output.
  CPU at 48 kHz: 0.3 % (PLATE) to 1.3 % (SPARKLE) of one core, SPRING
  about 5 %.

Mono / stereo
  Mono in, stereo out. With a stereo input the reverb is fed with the sum,
  the dry signal stays on its side. Latency 0. The plugin reports its tail
  length (CLAP tail) so a host keeps processing after the input ends.

Interface
  The front panel is an own drawing (no photo, no logo). Below it: the
  decay times per band, the meaning and value of THING 1/2 in the chosen
  mode, and the tone of the reverb (energy of the decay per frequency
  with LOW and HI, relative to 1 kHz; dashed: LOW and HI at 12 o'clock).
  Drag a knob up/down, mouse wheel for fine steps, double-click = 12 o'clock.
  The mouse wheel on the mode buttons steps through the modes.

Changes
  1.2  The lettering "MODEL (not on the unit)" in the header line of the model
       strip is gone; the plugin version stays there. Sound unchanged.
  1.1  The value popup at the knobs is gone (it appeared while turning a
       knob or with the mouse over a control); with it the switch VALUE POPUP
       in the header line of the model strip. Sound unchanged.
  1.0  First version.
