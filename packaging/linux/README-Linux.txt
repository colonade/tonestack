tonestack plugins for Linux - installation, hosts, limits
=========================================================

Packages
--------
  tonestack-plugins_<version>_amd64.deb   Debian/Ubuntu, x86-64
  tonestack-plugins_<version>_arm64.deb   Debian/Ubuntu, ARM64 (aarch64)
  tonestack-plugins-<version>-linux-<arch>.tar.gz
                                          all other distributions and
                                          Flatpak hosts: install.sh copies
                                          the plugins to ~/.clap and ~/.vst3
  <Name>_CLAP-<v>.zip, <Name>_VST3-<v>.zip
                                          single plugins, x86-64

Install the Debian package with apt, so that the dependencies are resolved:

  sudo apt install ./tonestack-plugins_<version>_amd64.deb
  sudo apt remove tonestack-plugins

(dpkg -i does not install missing dependencies.) If the website offers the
signed apt source (https://tonestack.fluxstatus.de/plugins/), updates also
come with "sudo apt update && sudo apt upgrade". The package puts every
plugin twice into the system folders that the CLAP and VST3 specifications
name for Linux:

  CLAP   /usr/lib/clap/<Name>.clap
  VST3   /usr/lib/vst3/<Name>.vst3/Contents/<arch>-linux/<Name>.so
         /usr/lib/vst3/<Name>.vst3/Contents/Resources/moduleinfo.json

Remove copies that you made by hand in ~/.clap or ~/.vst3, otherwise the
host finds every plugin twice (the package lists them after installing).

Distributions
-------------
  Needs glibc 2.38 or newer: Debian 13 (trixie) and newer, Ubuntu 24.04 and
  newer, and distributions based on them (Linux Mint 22, Pop!_OS 24.04, ...).
  Not Debian 12 or Ubuntu 22.04. libstdc++ is built in; the package depends
  only on libc6, libcairo2 and libx11-6. Other distributions (Fedora, Arch,
  openSUSE, ...) with glibc 2.38 or newer: use the .tar.gz archive.

Hosts
-----
Every host decides itself where it looks. What is known (November 2026):

  Host                        VST3 from /usr/lib/vst3   CLAP from /usr/lib/clap
  REAPER 7                    after setup (see below)   yes
  Bitwig Studio (native)      expected, not yet tested  expected, not yet tested
  Ardour, Mixbus              yes                       (no CLAP support)
  Qtractor                    yes                       yes
  Carla                       yes                       (CLAP only in newer builds)
  Audacity 3.x                yes                       (no CLAP support)
  Tracktion Waveform, other
  JUCE-based hosts            yes (see /usr/lib64 below) (JUCE has no CLAP)
  Renoise                     probably, not documented  (no CLAP support)
  Zrythm                      through Carla             through Carla
  LMMS, MusE                  (no VST3 support)         (no CLAP support)

After installing or updating, restart the host or let it scan again:
  REAPER   Options > Preferences > Plug-ins > VST > Re-scan (and CLAP)
  Ardour   Window > Plugin Manager > Re-scan; Mixbus loads only plugins
           that are already in its scan cache, so scan once after installing
  Bitwig   Settings > Locations; plugins appear after the next scan

REAPER
------
REAPER finds the CLAP versions in /usr/lib/clap by itself, but looks for VST3
only in ~/.vst and ~/.vst3. If you use the CLAP versions in REAPER, nothing
else is needed. For the VST3 versions the package runs

  tonestack-reaper-setup

for every user who has set up REAPER (~/.config/REAPER/reaper.ini), with the
rights of that user: it appends /usr/lib/vst3 to the line vstpath= and changes
nothing else (copy before the first change: reaper.ini.vor-tonestack). This
only works while REAPER is not running - REAPER writes its settings back when
it quits. If REAPER was running, the package says so; quit REAPER and run
tonestack-reaper-setup yourself (as your user, without sudo). Removing the
package takes the entry out again (only one that tonestack-reaper-setup made,
and only if nothing else is left in /usr/lib/vst3). By hand:
tonestack-reaper-setup --undo. Details: man tonestack-reaper-setup.
REAPER started for the first time after installing has no reaper.ini yet:
then start and quit it once and run tonestack-reaper-setup.

Flatpak hosts
-------------
A host installed as a Flatpak (Bitwig, Ardour, Studio Pro and others from
Flathub) does not see /usr/lib at all. Some of them read ~/.vst3 and ~/.clap:
for these use the .tar.gz archive (./install.sh copies the plugins there,
./install.sh --uninstall removes them). Flathub's Ardour only loads plugins
from Flatpak extensions (org.freedesktop.LinuxAudio.Plugins.*); a tonestack
extension is prepared but not yet published on Flathub.

JUCE hosts and /usr/lib64/vst3
------------------------------
Newer JUCE versions leave out /usr/lib/vst3 as soon as /usr/lib64/vst3 or
/usr/local/lib64/vst3 exists (on Debian and Ubuntu they normally do not; they
can appear when a converted RPM package is installed). If a JUCE host does
not find the plugins, check whether one of these folders exists.

Environment variables
---------------------
Carla replaces its default folders when VST3_PATH or CLAP_PATH is set: add
/usr/lib/vst3 and /usr/lib/clap to these variables if you set them.

Checking a download
-------------------
Every release lists SHA-256 sums (SHA256SUMS-*.txt). The release files are
also attested by GitHub (build provenance, signed without a key of ours):

  gh attestation verify tonestack-plugins_<version>_amd64.deb -R colonade/tonestack

shows that the file was built by the workflow of the repository from the
named commit.

Built how
---------
Compiled on GitHub (Ubuntu 24.04, gcc 13) with hardening (_FORTIFY_SOURCE=3,
stack protector, RELRO, BIND_NOW), without symbol table, exporting only the
plugin entry points. Checked there with Steinberg's VST3 validator, pluginval
and clap-validator, and by installing and removing the package. The same
source gives the same bytes: engine/verify.sh compares a local build with the
released files.
