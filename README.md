# ScaleView

A small key signature / scale reference as a **VST3**, **Audio Unit** and
**CLAP** plugin. It shows the twelve pitch classes as circles - five on the top row for
the black keys of an octave, seven on the bottom for the white keys - and
lights the notes of the scale you pick.

Note names are spelled for the key rather than always being sharps or flats:
each degree of a seven-note scale takes the next letter of the alphabet and
whatever accidental that letter then needs. C# major reads C# D# E# F# G# A# B#,
while Db major - the same seven notes - reads Db Eb F Gb Ab Bb C.

![ScaleView](docs/screenshot.png)

Notes you play are ringed, and the label names the chord you are holding -
`Cmin7`, `Bsus2`, `C/E` - instead of the scale name.

The plugin does not touch your audio: it passes both the audio and the MIDI
through untouched and exists to be looked at, so it can sit on any track.

This is a port of [ScaleView for REAPER](https://github.com/KallumS/ScaleView-for-Reaper),
a ReaScript. It matches **ScaleView Pro**, the one of the two scripts there that
reads chords; the musical core is the same, verified against it note for note.

## Using it

| Action | Result |
| --- | --- |
| Left-click | Scale list - pick a root under a scale type |
| Right-click | Random Scale, note names on/off, Simplify Note Names, highlight colour |
| Play | The notes are ringed and the chord is named |

### Chord detection

Put it on a track that MIDI reaches and it names what you are holding. The
chord is **read rather than looked up**: the third, fifth and seventh are
matched against the closed vocabulary of qualities that have agreed names, and
whatever is left over is described on top of it as a sixth, ninth, eleventh or
thirteenth, altered or not. So a voicing nobody thought to put in a table still
gets a symbol - `Cmin7(11)`, `C13b9`, `G7(maj7)`, `Fadd9Add11`.

It reads three suspensions rather than the usual two - `sus4`, `sus2` and
`sus#4`, the last only when the fifth is under it, so C F# G is `Csus#4` while
C E Gb is `C(b5)`.

Every note that could be the root is costed and the cheapest reading wins,
which is how the extensions come out as extensions rather than being discarded.
The **bass is found separately** from the root, so inversions read as slash
chords: B C# F# is `Bsus2` with B underneath but `F#sus4` with F# underneath,
and A C E G is `Amin7` or `C6` depending which is lowest. Where two readings
fit, the one holding a complete triad - a real third with a perfect fifth -
wins, and the odd notes hang off it.

The selected scale only breaks a draw: at equal cost a root that is a scale
degree wins, then a reading whose notes sit in the scale. A chord from outside
the key is still named for what it is, never filtered out. With no scale
selected the naming quietly assumes C major, so it never goes silent - nothing
lights up and no label names a key, and choosing C Major explicitly gives
identical names.

Measured against the whole of music21's core corpus - 3,194 files,
363,963 sonorities of three or more pitch classes - the printed symbol accounts
for exactly the notes played, with the right bass, **99.999%** of the time, and
100% on the Bach chorales, the Chopin mazurkas and the standards vocabulary in
every inversion. The four misses all carry eight pitch classes and are read out
as a list of notes, which is deliberate: past a certain thickness there is no
chord left to find, only a cluster.

Chord roots are spelled for the key, so a chord on Gb reads `Gbmaj7` in Gb
major and `F#maj7` in F# major. Chord symbols stick to the eighteen spellings
real keys are built on, though: Gb minor blues spells two of its notes Bbb and
Dbb, and the circles show them that way because it is correct for the scale,
but the chord they make reads `Amin/C`.

**Simplify Note Names** switches the circles to plain piano-key names - always
sharps, never a double accidental - without changing which notes light up or
what the scale label says. Gb Major still says Gb Major; the circle just reads
F#. The chord detection is untouched by it, only the names it prints.

Unlike the ReaScript this was ported from, the plugin sees **all** the MIDI on
its track, including MIDI items during playback - a script can only watch live
input.

The window is resizable and keeps its 2:1 proportions. Your selection, colour
and window size are saved with the host project and in presets.

### Scales

Major, Minor (Natural), Harmonic Minor, Ionian, Dorian, Phrygian, Lydian,
Mixolydian, Aeolian, Major Pentatonic, Minor Pentatonic, Major Blues, Minor
Blues, Whole Tone, Diminished Whole-Half and Diminished Half-Whole - each from
18 spelled roots (both spellings of every pitch class, plus Cb), so 288 keys.

## Building

You need [CMake](https://cmake.org) 3.22+ and a C++17 compiler. JUCE is
downloaded automatically by the build, pinned in `CMakeLists.txt`.

### macOS - VST3, Audio Unit and CLAP

Xcode is required (free from the App Store); the command line tools alone are
not enough for the AU.

```sh
cmake -B build -G Xcode
cmake --build build --config Release
```

This produces a universal binary for Intel and Apple silicon:

| Built | Install to |
| --- | --- |
| `build/ScaleView_artefacts/Release/VST3/ScaleView.vst3` | `~/Library/Audio/Plug-Ins/VST3/` |
| `build/ScaleView_artefacts/Release/AU/ScaleView.component` | `~/Library/Audio/Plug-Ins/Components/` |
| `build/ScaleView_artefacts/Release/CLAP/ScaleView.clap` | `~/Library/Audio/Plug-Ins/CLAP/` |

Or set `COPY_PLUGIN_AFTER_BUILD` to `ON` in `CMakeLists.txt` and the build will
install them for you.

Logic and GarageBand only load Audio Units; REAPER and Bitwig will take the
CLAP or the VST3.

macOS will not load an unsigned plugin downloaded from the internet, but one
you built yourself is fine. To validate the AU before opening a host:

```sh
auval -v aumf Scvw Klms
```

It validates as `aumf` - a MusicEffect - rather than `aufx`, because an Audio
Unit effect only receives MIDI as a MusicEffect. That is also why it appears
under Logic's MIDI-capable effects.

Logic and GarageBand only rescan on launch, so quit and reopen them after
installing.

### Windows - VST3 and CLAP

```sh
cmake -B build
cmake --build build --config Release
```

Copy `build\ScaleView_artefacts\Release\VST3\ScaleView.vst3` to
`C:\Program Files\Common Files\VST3\`, and
`...\Release\CLAP\ScaleView.clap` to `C:\Program Files\Common Files\CLAP\`.

### Linux - VST3 and CLAP

Install the JUCE dependencies first:

```sh
sudo apt install libasound2-dev libx11-dev libxext-dev libxinerama-dev \
                 libxrandr-dev libxcursor-dev libfreetype-dev libfontconfig1-dev \
                 libgl1-mesa-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Copy the `.vst3` to `~/.vst3/` and the `.clap` to `~/.clap/`.

### A note on CLAP

JUCE does not build CLAP itself, so the plugin is wrapped by
[clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions),
which adds a CLAP target beside the VST3 and AU from the same sources - there
is no separate code for it. It is pinned to a commit rather than the 0.26.0
release, because that release predates JUCE 8.0.12 moving a header into its new
headless module; the pinned commit handles both layouts. Build without it using
`-DSCALEVIEW_BUILD_CLAP=OFF`.

## Tests

`Tests/TestScaleModel.cpp` covers the musical core, and needs neither JUCE nor
a host - it is the same suite the ReaScript has, ported. It checks all 288 root
and scale combinations, that every seven-note scale uses each of the seven
letters exactly once, that enharmonic pairs light the same circles while
reading differently, that the fifteen standard major keys need no double
accidentals, that chord symbols no table ever held come out right, and that
Random Scale spreads and never repeats itself.

```sh
cmake -B build-tests -DSCALEVIEW_BUILD_PLUGIN=OFF
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

`-DSCALEVIEW_BUILD_PLUGIN=OFF` skips JUCE entirely, so the model tests build in
a couple of seconds with nothing downloaded.

A second suite, `Tests/TestProcessorMidi.cpp`, needs JUCE because it drives the
real processor: notes arriving on the audio thread, note-offs and all-notes-off,
that the MIDI is passed on rather than consumed, notes at both ends of the two
64-bit masks that track them, plugin state surviving a save and reload, and the
editor's paint path drawing rings around the held notes. It is built with the
plugin and run by the same `ctest`.

## Layout

| | |
| --- | --- |
| `Source/ScaleModel.h` | The scales, the roots, the spelling engine and the chord reader. No JUCE, no host. |
| `Source/PluginProcessor.*` | Audio and MIDI passthrough, held-note tracking, and the selection, saved with the project |
| `Source/PluginEditor.*` | The icon and its two menus |
| `Tests/TestScaleModel.cpp` | The musical core's tests - scales, spelling, chords |
| `Tests/TestProcessorMidi.cpp` | The MIDI path, plugin state, and the editor's drawing |

## Differences from the ReaScript

- **No docking.** The host owns the plugin window. It is resizable instead and
  keeps its proportions.
- **Random Scale** is seeded from the system random source rather than
  REAPER's clock.
- **It sees more MIDI than the script does.** The ReaScript reads REAPER's
  global input history, which carries live playing only; the plugin sees
  whatever reaches its track, playback included.
- **Settings** are saved with the host project and in presets, rather than in
  REAPER's global settings, so two instances can show different keys.
