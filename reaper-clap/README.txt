Laney AOR 50 Preamp - CLAP and VST3 plugin for REAPER (Linux x86-64)
Version 3.32

Two formats, one engine
  Laney_AOR50.clap and Laney_AOR50.vst3 contain the same code and sound identical
  (checked sample by sample). Use whichever your DAW prefers; REAPER
  handles both. CLAP is the native format, the VST3 is built from it with
  clap-wrapper (free-audio, MIT) and the VST3 SDK 3.8 (Steinberg, MIT) and
  passes Steinberg's validator (47 of 47 tests). A project saved with one
  format does not load its settings into the other.
  Third-party licenses (MIT, public domain; Mac Audio Unit also Apache 2.0): THIRD-PARTY-LICENSES.txt.

Updating
  Close REAPER first. Copying a new .clap over the file a running REAPER has
  loaded makes REAPER crash (at once or a while later). scripts/update-
  tonestack.sh refuses to run while REAPER is open and never overwrites a
  plugin file in place.

Installation (CLAP)
  mkdir -p ~/.clap && cp Laney_AOR50.clap ~/.clap/
  In REAPER: Options > Preferences > Plug-ins > CLAP: add ~/.clap to the
  path if it is not there and press "Re-scan". The plugin then appears in
  the FX browser under CLAP as "Laney AOR 50 Preamp".

Installation (VST3)
  mkdir -p ~/.vst3 && cp -r Laney_AOR50.vst3 ~/.vst3/
  (Laney_AOR50.vst3 is a folder - copy it as a whole.) REAPER searches ~/.vst3
  by default; otherwise add it under Options > Preferences > Plug-ins >
  VST, then "Re-scan". The plugin appears under VST3 as "Laney AOR 50 Preamp".

What it is
  The same engine as the web simulator (linear network from schematic
  DRG 1054, tube stage with Dempwolf triode models and Newton solver, power amp
  with negative feedback and presence), translated to C++. Verified against
  the simulator: linear within -137 dB, tube stage 2x oversampling bit-exact.
  About 20 times faster than the JSFX version. CPU load measured on a
  2.1 GHz Xeon core at 48 kHz (bass DI, mono or identical L/R, see 3.13):
  linear 3 %, tube distortion 1x 9 %, 2x 17 %, 4x 29 %, 8x 52 % of one core.
  A current desktop CPU needs roughly half of that. These figures are for
  the preamp; since 3.20 the power amp circuit comes on top (with "FX loop +
  power amp" in tube mode). Measured on this test machine with a bass line,
  3.21: tube 1x about 50 %, 2x about 95 %, 4x about 115 %, 8x about 195 %
  (3.20: 50 / 62 / 77 / 133 %). If playback stutters, choose 1x.

Parameters
  Input jack, Channel, Preamp 1 Volume, Preamp 2 Volume, Preamp 1 Level,
  Treble, Middle, Bass, Master Volume, Mid/Top/Bass Boost, Bass mod,
  FX loop + power amp (Presence), Output level,
  Tube distortion, Instrument level, Oversampling (1x/2x/4x, default 2x; 8x removed in 3.28),
  Presence (active with "FX loop + power amp"), Tube V101/V102/V103, Speaker,
  Soft and Late (circuit ideas, see 3.12), Power tube mismatch (0-10 %, see
  3.20).
  Defaults: Preamp 1, HIGH, both preamp volumes and Preamp 1 Level at 2, the
  other controls at 5 (12 o'clock), all pull switches on, tube distortion on,
  2x oversampling, bass mod on, FX loop + power amp on, tubes 3 x 5751, Soft
  on, Late and speaker off, tube mismatch 8 %.

User interface
  Photo of the front panel with turnable knobs: drag vertically (mouse wheel
  works too), double-click = centre. Click PULL ON / PULL BOOST for the pull
  switches, click the HIGH/LOW jack to choose the input. Model strip below
  (settings that do not exist on the amp): toggle switches for BASS MOD,
  FX LOOP (+ power amp), DISTORTION, SPEAKER, SOFT and LATE (lever up = on, click to
  flip, mouse wheel up/down), a rotary selector for OVERSAMPLING (1x, eco,
  2x, 4x: click a label to select it directly, click the knob to turn it
  on, wheel turns either way), INPUT, OUTPUT and TUBE MISMATCH. Below: the three tube sockets (click to change
  the type), the tube DRIVE meters and
  the frequency response. Text in
  the lower part is sized for the screen (12-14 pixels at the default
  width), independent of the photo resolution. Every change is
  sent to REAPER as a parameter (automatable); REAPER's own parameter view
  stays available via its UI button. The window can be resized horizontally.
  Dimmed controls have no effect in the current setting; REVERB is not
  modelled.
  Needs libcairo2 and libX11 (present anyway with REAPER on Linux).

Tubes
  Three parameters Tube V101, Tube V102, Tube V103 (each socket is a twin
  triode) choose the tube type: 12AX7/ECC83 (stock), 12AT7/ECC81, 12AU7/ECC82,
  5751, 12AY7. Since 3.25 all five use the Dempwolf/Zoelzer triode formula,
  fitted to data sheet operating points (plate current, transconductance,
  plate resistance); the 12AX7 also to the five preamp operating points of
  the schematic. Less certain away from those points. The operating points (bias, plate voltages)
  follow from the model and the circuit, so a different tube also shifts the
  bias of its stage. In the user interface the row above the frequency
  response shows the sockets; clicking a socket cycles the type, RESET TO
  STOCK restores 3 x 12AX7. Stock tubes are bit-exact with version 3.3.
  Not modelled: sample-to-sample variation of the preamp tubes, microphonics,
  ageing. With tube distortion on, the phase inverter, the power tubes and
  the output transformer are computed as a circuit (see 3.20).

Frequency response
  Below the model strip the plugin draws the small-signal frequency response
  of the current setting (blue), relative to 1 kHz, computed from the same
  circuit network as the audio (complex nodal analysis, 20 Hz to 20 kHz).
  The dashed grey line is the reference: original parts, all controls at 5,
  boosts off, same channel, input and FX-loop setting. With tube distortion
  on, the curve still shows the small-signal response; the power amp part
  then comes from the power amp circuit at its operating point (with the
  speaker as load). The window
  can be resized; the plot scales with it.

Meters and load
  TUBE DRIVE: five bars show how hard each triode stage (V101A, V101B,
  V102A, V102B, V103B) is driven at its grid; the white mark is the onset of
  overdrive (1.0), green below, amber around, red above. The text line in
  the model strip shows the CPU load of the plugin in percent of one core,
  the oversampling in use and the sample rate.

Idle
  When input and output have been silent for 2 s, the plugin
  tells REAPER it may stop calling it (CLAP "sleep") and, whether or not the
  host honours that, skips its own processing and outputs silence; the CPU
  line then shows "idle (silent input)". On the next note it continues from
  the state in which it went to sleep (the circuit had come to rest), so the
  result does not depend on whether or how long it slept.

Speaker (since 3.6)
  The SPEAKER toggle in the model strip (parameter "Speaker") adds a
  15" bass speaker after the preamp/power amp: Eminence Legend CB158, Thiele-
  Small data from the data sheet (Fs 34 Hz, Qts 0.34, Vas 336 l, Re 6.15 ohm,
  Le 0.33 mH), in a 78 l vented box tuned to 50 Hz, the size of an Ampeg
  Portaflex cabinet and Eminence's own recommendation for this driver. The
  low end is the acoustic equivalent circuit (-3 dB at 61 Hz, +1.7 dB around
  100 Hz); the top is the voice-coil inductance (3 kHz) plus an estimated
  second-order roll-off at 4 kHz for cone break-up. No room, no microphone.
  The frequency-response plot includes it when it is on. Off = identical to 3.5.

3.32
  - X11 error handling only during the plugin's own window calls. Before,
    each plugin hooked permanently into the process-wide X error handler
    chain; with several of our plugins loaded and one removed first, that
    chain could point into unloaded code. Tested: host window destroyed
    under an open plugin window, no crash, host handler restored afterwards.
  - Frequency plot: when the power amp's resting state for a setting was
    missing (e.g. after moving TUBE MISMATCH), the window computed it on
    REAPER's main thread (up to a few tenths of a second, REAPER's interface
    froze). Now a background thread does it and the plot redraws afterwards.

3.31
  - Plugin window: much cheaper to draw. The photo used to be rescaled for
    every frame (about 30 times a second) and the window redrawn whenever a
    meter moved by a hair; with the window open that cost up to half a CPU
    core on the GUI thread, which the CPU figure in the model strip does not
    include (it measures only the audio processing). Now the photo is
    scaled once per window size, meters redraw only when a bar moves by a
    pixel, the CPU figure updates twice a second. Measured here: Laney window
    45 % -> 7 % of a core, Yamaha 18 % -> 1.4 %, AmpFett 22 % -> 2.8 %.

3.30
  - SOFT and BASS MOD switches: no more CPU spike either. The capacitors
    stay in place; the switch is a resistor in series with the extra
    capacitor that glides in 15 ms (like LATE since 3.29). Sound differs from
    3.29 by less than -114 dB in every combination.

3.29
  - LATE switch: no more CPU spike. It used to rebuild the whole tube stage
    and blend over from a second, freshly computed copy (about 150 ms at two
    to three times the normal load, which JACK could punish). Now the 2.2 MOhm
    resistor glides in like a pot (15 ms), without a second copy. Sound with
    LATE on is bit-identical to 3.28; with LATE off a 1 Ohm resistor remains
    in its place (difference below -120 dB).

3.28
  - 8x oversampling removed. It needed about two CPU cores; under JACK the
    audio server could throw REAPER out (crash, beeping from dropouts).
    Projects saved with 8x now run at 4x. Sound at 4x and below unchanged.

3.27
  - Crash/stall when switching OVERSAMPLING (or eco) while playing: the
    power amp's resting state for a new sample rate was computed on the
    audio thread (0.3 s here, seconds on slower machines or under Rosetta;
    REAPER could crash). Now all rates are prepared in the background right
    after loading; the audio thread never computes it.
  - Click on a label of the OVERSAMPLING selector selects it directly (no
    need to step through 4x and 8x to reach eco).
  - Idle at silence works again: since 3.21 the plugin never went idle with
    the power amp on, so it used full CPU during silence. Now it sleeps after
    2 s of silence and continues from the frozen state on the next note.
  - Power supply without simulated mains hum: high voltage and bias as DC
    behind their internal resistance (matched to the transformer model:
    B+ 400 V, bias -35.9 V, sag 4 V per 40 mA). Sound otherwise unchanged.
  - Pot TAPER switch removed: middle and presence are always logarithmic
    (22KA in the schematic, as the former default). The parameter stays
    hidden for old projects but has no effect.

3.26
  - Faster: the circuit solver works with a slightly looser convergence
    limit (10 instead of 1 microvolt at the tube ports; difference to 3.25
    below -100 dB even at very low levels) and with precomputed sparse
    matrices. About 20 % less CPU at 2x oversampling.
  - New oversampling setting "eco" (parameter "Eco", on the OVERSAMPLING
    selector between 1x and 2x): preamp at 2x, power amp at the base rate as
    up to 3.20. About a third less CPU than 2x; level and phase within
    0.2 dB / 0.5 degrees of 2x, slightly more aliasing when the power amp
    clips hard.
  - Mac/Logic: run Logic natively, not under Rosetta (see LIESMICH-Mac.txt).

3.25
  - Preamp tube types 12AT7, 12AU7, 5751 and 12AY7 now use the Dempwolf
    triode formula (like the 12AX7 since 3.21), fitted to data sheet
    operating points: 5751 and 12AY7 within 1 % (the old 12AY7 model gave
    only 1.7 instead of 3 mA at its data sheet point), 12AT7 within 4 %,
    12AU7 within 8 %. With the default 3 x 5751 the preamp is about 0.9 dB
    louder at low level and has slightly more even harmonics.
  - Power tubes EL34: new pentode model after Derk Reefman (parameters from
    a uTracer measurement of an EL34, scaled to the data sheet point
    250/250/-12.2 V: 100 mA, 14.9 mA): sharper knee at low plate voltage,
    screen current that rises when the plate swings low, plate resistance
    20 kOhm as in the data sheet (old Koren model: 39 kOhm). Idle current
    unchanged (about 33 mA), small-signal level unchanged (output scale
    re-matched); a bit more third harmonic at medium level, slightly more
    output before clipping.

3.24
  - Tube mismatch now defaults to 8 % (a realistic pair of power tubes;
    double-click on the knob also sets 8 %). Projects saved with older
    versions keep 0 %, so they sound as before.

3.23
  - The knob in the model strip is now labelled TUBE MISMATCH (was MISMATCH).
    Sound unchanged.

3.22
  - Mono tracks: input and output can now be mono or stereo (1->1, 1->2,
    2->1, 2->2). Logic offers Audio Units on a mono track only with a mono
    input; with stereo only, the plugin most likely did not show up there.
    Sound unchanged (the engine works in mono anyway).

3.21 (remaining simplifications of the model removed)
  - Preamp supply: the preamp rails (260 / 252 / 240 V behind R26, R14, R4
    and their filter caps) now hang on the power supply of the power amp
    circuit. When the power amp draws current or the preamp tubes swing,
    the rails sag and carry the ripple into the preamp, like on the amp
    ("bloom" on hard notes). Only in tube mode with "FX loop + power amp".
  - The power amp circuit now runs oversampled: at 2x and 4x at twice the
    sample rate, at 8x at four times (before: always at the base rate, so
    hard power-amp clipping produced aliasing). Latency still exactly 31
    samples in every mode (checked by phase at 200 Hz to 5 kHz).
  - Linear mode and the frequency-response plot now use the same power amp
    circuit, linearised at its operating point (with the speaker as load).
    Linear and tube mode agree within about 0.1-0.4 dB at small signal.
    Part value "Gp" no longer has an effect (only in the web page's fallback
    engine without WebAssembly).
  - Output transformer core with hysteresis (estimate, Rayleigh-type: loop
    area grows with the cube of the flux swing). Adds odd harmonics on low,
    loud notes; nothing below about 4 % of the saturation flux.
  - 12AX7 computed with the triode formula of Dempwolf and Zoelzer (2011,
    exponent 1.26 as measured there), fitted to the same data as in 3.20:
    RCA data sheet 0.52 / 1.18 mA (data sheet 0.5 / 1.2 mA; Koren fit 3.20:
    0.35 / 1.60 mA). Affects V101-V103 when set to 12AX7 and the phase
    inverter; the level at 1 kHz is kept as before.
  - Mains transformer as a transformer (primary 230 V / 50 Hz, leakage,
    shared core): the bias winding now sags together with the high voltage.
    Idle voltages: 400 / 399 / 327 V (plan 400 / 398 / 326), bias -35.9 V.
  - CPU: see "What it is"; the oversampled power amp roughly doubles its
    share at 2x and more.

3.20 (power amp as a circuit, tube models fitted to more data)
  - With tube distortion and "FX loop + power amp" on, the power amp is now
    computed as the whole circuit of the schematic (DRG 1054), like the
    AmpFett: phase inverter V104 (12AX7, long-tailed pair with R38 negative
    feedback and presence), two EL34 with fixed bias (-36 V), output
    transformer with core saturation, power supply with bridge rectifier,
    choke and filter caps (the supply sags under load), bias supply, and the
    speaker (CB158 in the 78 l vented box) as the load on the 8 ohm tap.
    Before, the power tubes were one soft limiter and the transformer one
    saturating inductance. Now the push-pull stage produces the typical odd
    harmonics (3rd harmonic dominant) already well below clipping, the
    supply sag and the phase inverter shape the clipping, and the voltage
    at the speaker terminal follows the speaker impedance like on a real
    tube amp: up to about +5 dB around 100 Hz relative to 1 kHz (the speaker
    resonance with the vented box), more treble above 3 kHz. Level at 1 kHz
    and small signal as before.
    Idle voltages as in the schematic: B+ 405 / 403 / 329 V (plan 400 / 398 /
    326), phase inverter plates 217 / 208 V (225 / 209), bias -35.9 V (-36),
    EL34 idle current 35 mA each.
    Estimated (not in the schematic): EL34 (stock tube type according to
    dealers), output transformer 4 kOhm : 8 ohm with 40 H, leakage, winding
    capacitance, core loss and saturation, choke 5 H, mains transformer
    resistance, preamp load on the 326 V rail. The linear mode keeps the
    simpler linear power amp (no speaker load), so linear and tube mode now
    differ in the bass by a few dB when the power amp is on.
  - New parameter "Power tube mismatch" (0-10 %, MISMATCH knob in the model
    strip): difference of the idle current of the two EL34. Mismatched
    tubes add even harmonics (2nd, 4th) and a small DC through the output
    transformer. The knob glides over 0.3 s (no thump). Projects from older
    versions load with 0 %.
  - 12AX7 model refitted: Koren's 1996 parameters gave only 0.1 mA at
    100 V / -1 V (RCA data sheet 0.5 mA) and 10-25 % too little current in the
    preamp stages of the Laney (plates 6-10 V above the schematic). Now fitted
    to the RCA data sheet and the five preamp operating points of the
    schematic: plates within 1-6 V. Affects V101-V103 when set to 12AX7 and
    the phase inverter. 5751 and 12AY7 (default 5751) unchanged.
  - CPU: the power amp circuit costs about 15-25 % of one core extra on this
    test machine (half of that on a current desktop CPU), independent of the
    oversampling setting (it runs at the base sample rate).

3.19 (findings from the AmpFett B-15NF tube check)
  - Tubes are now computed with the full formula instead of an interpolated
    table. The table added small kinks that produced false high harmonics
    on quiet playing (4th to 6th harmonic around -75 to -90 dB; now below
    -105 dB). Distorted sound otherwise unchanged (difference to 3.18 about
    -75 dB on a bass line). Side effect: about a third less CPU, because
    the solver needs fewer steps with the smooth curve.
  - Switching the FX off and on again in REAPER: the amp resumed from the
    state frozen when it was switched off, a short odd sound. Now it starts
    from the idle state (after start of processing or a pause of more than
    0.2 s), like an amp that kept running silently.

3.18 (code audit)
  - Linear mode (tube distortion off) now plays at the same absolute level as
    tube mode (within 0.6 dB at small signals); before it was normalised and
    up to 60 dB louder at low instrument levels. The instrument level now
    also applies in linear mode.
  - Power amp with tube limiting: the level was 1/Gp too low (double
    division) - corrected, now within 0.2 dB of the linear power amp.
  - Presence knob: smoothed every 32 samples (no zipper noise).
  - Output level: glides over 20 ms (no clicks).
  - Switching between linear and tube mode while a crossfade is still
    running no longer jumps.
  - Projects saved before the presence, tube, speaker or Soft/Late
    parameters existed load with the settings they had then (presence 5,
    stock tubes, speaker, Soft and Late off) instead of the new defaults.
  - Frequency-response grid lines at 20, 30 ... 90, 200 ... Hz; plot redraws
    reliably after fast knob moves; the window no longer leaks 1.5 MB of
    memory each time it is closed.

3.17 (Mac version)
  The plugin now also exists for macOS (Audio Unit for Logic and VST3,
  Apple Silicon and Intel, from macOS 11) with the same front panel. Linux:
  internal restructuring only, sound and panel identical to 3.16.

3.16 (new default setting)
  A newly inserted plugin starts with: Preamp 1 (high gain) with all pull
  switches on (Preamp 1, Treble/Top Boost, Middle/Mid Boost, Bass/Bass
  Boost), Preamp 1 Volume 2, Preamp 2 Volume 2, Preamp 1 Level 2, Treble,
  Middle, Bass, Master and Presence 5 (12 o'clock), tubes 3 x 5751, Soft on,
  Late off, FX loop + power amp on, bass mod on, tube distortion on.
  Projects saved earlier keep their settings. Sound otherwise identical to
  3.15. "RESET TO STOCK" still sets the tubes of the real amp (3 x 12AX7).

3.15 (softer onset of distortion, power amp and output transformer)
  - Grid current of the preamp tubes after Dempwolf/Zoelzer (2011, fitted to
    measured 12AX7): it now sets in gradually from about -0.5 V instead of
    as a sharp diode at 0 V. Measured with a 110 Hz sine (8x oversampling),
    the range from 1 % to 10 % distortion widens from 8 to 10-12 dB, and the
    distortion starts a little earlier and grows more gradually.
  - With "FX loop + power amp" and tube distortion on, the power amp is no
    longer linear: the output tubes limit softly at about 30 V peak into
    8 ohms (about 56 W) inside the negative feedback loop, and the output
    transformer saturates at low notes (magnetising inductance 0.08 H
    referred to 8 ohms, saturation flux 0.09 V*s: at full power it starts
    below about 50 Hz, i.e. low E and low B). With the clean channel at
    Preamp 2 Volume 5 and Master 10 the power amp reaches its limit from
    about 100 mV instrument level. All values are estimates (no measurement
    of the real amp); the frequency-response plot stays small-signal.
  - Not modelled: power supply sag.
  - CPU load about 2-3 % of a core higher. Sound in linear mode unchanged.

3.14
  The plugin version is shown in the plugin window (model strip, "PLUGIN
  VERSION 3.14"), so you can check in REAPER which build is loaded.
  Pot values checked once more against schematic DRG 1054: VR101 100 kB,
  VR102 1 MB, VR103 100 kB, VR104 100 kB, VR105 220 kB, VR106 1 MB (wired as
  variable resistor), VR107 22 kA, VR108 22 kA (variable resistor to C110);
  B = linear, A = log (the TAPER switch applies to the two A pots, middle and
  presence). All values and connections match the model. Sound unchanged
  (bit-identical to 3.13).

3.13 (load and switching - please update)
  Tested like the web simulators: latency, CPU load, every parameter,
  clicks when switching. Findings and changes:
  - Latency is what the plugin reports: 31 samples (0.65 ms at 48 kHz) in
    every mode, block size and sample rate, also on the first note after a
    pause (idle). That is not audible. What can feel like a delay is the
    circuit itself: its coupling capacitors delay the lowest notes (group
    delay at 41 Hz about 8 ms, at 82 Hz about 4 ms; with SPEAKER 14 ms and
    9 ms), as a real amp and cabinet do. The Yamaha has about 2 ms at 41 Hz.
  - The real problem was CPU load: the default 8x oversampling needed more
    than a whole core in stereo on the test machine. With a small buffer that
    means dropouts, and a larger buffer means latency. Now:
    * Identical left and right input (a mono track, or a bass DI on a stereo
      track) is computed once instead of twice: half the load. As soon as
      the channels differ, the right engine takes over the state of the left
      one and runs separately; back to single processing only once both
      give the same output again, so nothing jumps.
    * Default oversampling 2x instead of 8x (new instances; saved projects
      keep their value - set OVERSAMPLING to 2x there if REAPER struggles).
      4x/8x remain available, e.g. for mixing or rendering.
    * Mode switches (channel, input, distortion, oversampling) recompute the
      new circuit at playback speed instead of three times faster, so a
      switch no longer overloads small buffers; it takes effect after about
      0.2 s instead of 0.1 s.
  - Clicks when switching: SPEAKER was switched hard (now cross-faded over
    20 ms), and with tube distortion BASS MOD, SOFT, LATE and the tube
    selectors restarted the circuit from its operating point (now a fresh
    circuit is computed and cross-faded like a mode switch). Measured jump
    at the switch, relative to the steepest slope of the music: SPEAKER
    4.6 -> 1.0, BASS MOD 4.5 -> 1.0, tubes 1.4-1.8 -> 1.0-1.1.
  Sound otherwise unchanged: left channel bit-identical to 3.12 with the
  same settings; CLAP and VST3 bit-identical; ASan stress tests clean.

3.12
  The two circuit ideas from the web simulator as parameters and toggle
  switches: SOFT ("Sanft": C3 at V101B removed - the cathode bypass of the
  second stage, which makes that stage hit hardest; with it gone the stage
  has less gain and saturates more gently) and LATE ("Später": a 2.2 MOhm
  resistor before the Preamp 2 Volume pot, about -10 dB into the following
  stages, so distortion starts later; only with the HIGH input). Both are
  calculated exactly as in the web simulator. The tube drive meters moved
  into the tube row to make room. Parameters 24 and 25 were appended, so
  saved projects keep their settings.
3.11 (crash fixes - please update)
  Fixes two ways the plugin could take REAPER down, in CLAP and VST3:
  - switching DISTORTION or OVERSAMPLING again while the previous switch
    was still being prepared (fast clicks) used an engine that had just
    been deleted (use-after-free; present since 2.x, made likely by the
    toggle switches);
  - an X11 error on the plugin's own window (drawing into or destroying a
    window the host had already removed, e.g. when closing the editor)
    went to Xlib's default handler, which ends the whole process. The
    plugin now ignores X errors on its own display connection, notices
    when the host destroys its window, and serialises all editor calls
    (with VST3 the wrapper may call the timer from a helper thread).
  INPUT and OUTPUT knobs can be turned again (3.9 missed them when hit-
  testing). Checked with AddressSanitizer under stress (audio thread
  running, editor opened and closed repeatedly, random clicks).
3.10
  Latency is now exactly 31 samples with oversampling too (was 1/os
  sample short); the output of the oversampled modes is shifted by that
  fraction of a sample, otherwise unchanged. Linear and 1x bit-identical.
3.9
  Model strip with toggle switches (bass mod, FX loop, distortion,
  speaker, taper) and a rotary selector for oversampling; speaker moved
  from the tube row into the strip; all text below the photo enlarged and
  sized in screen pixels (before: down to about 5 pixels at the default
  width). Audio unchanged.
3.8
  VST3 version (same engine, sound bit-identical to the CLAP). Knob drags
  in the plugin window are now sent to the host as gestures (begin, values,
  end), so REAPER records them as automation in touch/latch mode and a VST3
  host learns about them at all. Audio unchanged.
3.7 (code review)
  Fixed latency of 31 samples in every mode (see below). The tube tables and
  the operating-point cache are now built and used thread-safely (several
  instances on parallel tracks, GUI thread); the tables are built when the
  plugin is created, not on the audio thread. The idle/sleep state now also
  works with tube distortion (a key mismatch had disabled it since 3.4).
  Non-finite input samples are treated as silence and a diverged solver
  restarts the circuit instead of outputting NaN forever. Denormals are
  flushed during processing. Plugin state is saved with full precision.
  With a mono input the second engine is no longer kept running. The audio
  with tube distortion and oversampling is bit-identical to 3.6.1.
3.6.1
  Frequency-response plot refreshes on the wall clock (before, it could lag
  behind a knob move when the process used little CPU). Audio unchanged.

Polarity and latency (since 3.5)
  The output has the same polarity as the input. The preamp itself inverts
  (five inverting tube stages), which earlier versions reproduced; a sound
  engineer pointed out that a plugin should not flip polarity, so the engine
  now undoes it. Apart from the sign the output is identical to 3.4.
  With tube distortion and oversampling the engine has 31 samples of latency
  (oversampling filters). Since 3.7 the plugin always reports 31 samples
  and delays the linear and 1x paths by the same amount, so the latency
  never changes while the plugin is active (which CLAP forbids) and
  switching modes stays click-free; REAPER compensates it automatically.
  Measured (3.10): the delay is exactly 31 samples in every mode - linear,
  1x, 2x, 4x and 8x - to within 0.02 samples, at any sample rate (0.70 ms
  at 44.1 kHz, 0.65 ms at 48 kHz, 0.32 ms at 96 kHz). With oversampling the
  output starts to move 8-10 samples before that: the pre-ringing of the
  linear-phase oversampling filters, far below audibility. Before 3.10 the
  oversampled paths were 1/os sample short of 31 (30.5 at 2x, 30.75 at 4x,
  30.875 at 8x), which only mattered when blending with the dry signal
  (about -2 dB at 20 kHz in a 50/50 mix).

Notes
  True stereo: left and right are processed by two independent engines;
  identical channels are computed once (3.13). A mono input is sent to both
  channels.
  Built on Ubuntu 24.04 against glibc 2.39 (needs 2.38 or newer); if REAPER
  does not find the plugin after scanning, tell me your distribution and I
  will build against an older glibc.
