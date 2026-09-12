# ScaleView (VST3 / AU / CLAP)

A JUCE plugin drawing a 2:1 icon: the twelve pitch classes as circles, five
black keys over seven white, with the notes of the selected scale lit, the
notes being played ringed, and the chord being held named underneath. It passes
audio and MIDI through untouched and exists to be looked at.

| | |
| --- | --- |
| `Source/ScaleModel.h` | Scales, roots, the spelling engine and chord naming. **No JUCE, no host** - keep it that way, it is what makes the musical core testable in seconds. |
| `Source/PluginProcessor.*` | Passthrough, held-note tracking, state |
| `Source/PluginEditor.*` | The icon, its two menus, the 30Hz poll |
| `Tests/TestScaleModel.cpp` | The musical core. No JUCE. |
| `Tests/TestProcessorMidi.cpp` | The MIDI path, state round trip, editor paint. Needs JUCE. |

## Working in this repo

```sh
# model tests only - no JUCE downloaded, a couple of seconds
cmake -B build-tests -DSCALEVIEW_BUILD_PLUGIN=OFF && cmake --build build-tests
ctest --test-dir build-tests --output-on-failure

# everything, including the plugin and the JUCE-linked tests
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
ctest --test-dir build --output-on-failure
```

- **When fixing a bug, confirm the new test fails on the old code first.** A
  test that passes against the bug is worthless.
- The editor can be rendered headlessly - no X server, no host. The processor
  test paints it into an image and asserts the rings are drawn; set
  `SCALEVIEW_RENDER_TO=/path/out.png` to also write it out. That is how the
  README screenshot was made and how to eyeball a drawing change.
- Linux builds VST3, CLAP and Standalone, which is enough to verify everything
  but the AU. The AU can only be built on macOS with Xcode, and nothing here
  can sign anything.

## Traps, each of which cost time or nearly shipped broken

- **Do not set `AU_MAIN_TYPE`.** JUCE only chooses a type when the property is
  unset, and with `NEEDS_MIDI_INPUT TRUE` it chooses `kAudioUnitType_MusicEffect`
  - which is what makes Logic send MIDI to an effect at all. An earlier
  hardcoded `kAudioUnitType_Effect` would have validated cleanly and shown a
  permanently blank chord display in Logic. Check with
  `grep JucePlugin_AUMainType` in the generated `Defs.txt`: it must be `'aumf'`.
- **JUCE is pinned to 8.0.15 and the CLAP wrapper to a commit, not its 0.26.0
  release.** That release includes
  `juce_audio_processors/format_types/juce_LegacyAudioParameter.cpp`, a path
  JUCE dropped in 8.0.12 when it moved the file into the new headless module,
  so it will not compile against the pinned JUCE. The pinned commit handles
  both layouts. Bump the wrapper, not JUCE, if this breaks again.
- **JUCE's recommended warning flags are strict** - `-Wsign-conversion`,
  `-Wshadow`, `-Woverloaded-virtual`. Index `std::array` with `size_t`, and
  note that overriding only the float `processBlock` hides the double one:
  both are implemented here through one `passThrough` template, which also
  means 64-bit hosts work.
- **Audio thread rules.** `processBlock` must not allocate or lock. Held notes
  are two `std::atomic<uint64>` masks (notes 0-63 and 64-127) written there and
  read by the editor, which polls at 30Hz and only repaints when they change.
  Do not replace this with a callback from the audio thread.
- **MIDI is read and passed on, never consumed** - the buffer is left alone so
  the plugin is transparent in a chain.
- **Plugin state is stored by name, not index** (`"Gb"`, `"Major"`, `"Teal"`),
  so reordering a table cannot repoint a saved choice. There is a test for the
  round trip.
- The editor keeps a fixed 2:1 aspect through its constrainer, and centres the
  icon in whatever space the host gives it.

## Parity with the ReaScript

`KallumS/ScaleView-for-Reaper` is the original, in Lua. The musical core here
is a port and **must stay in step**: if spelling or chord naming changes in one,
change it in the other.

Both have been verified by diffing output, which is the technique to reuse
rather than eyeballing cases:

- **Spelling:** all 288 root/scale combinations, lit notes and every note name
  including the ones outside the scale. Byte-identical.
- **Chords:** 13,200 voicings - every three- and four-note pitch class set in
  every bass position, across five keys including ones whose notes need double
  accidentals. Byte-identical.

Write a dumper on each side that prints one line per case, run both, `diff`.

## How the musical core works

Each scale carries `intervals` (semitones from the root) alongside
`letterSteps` (how many letter names each degree sits above the root letter).
A seven-note scale walks the letters in order, so each degree takes the next
letter and whatever accidental it then needs: C# major reads C# D# E# F# G# A# B#
while Db major, the same seven notes, reads Db Eb F Gb Ab Bb C. Scales that
cannot take one letter per degree keep their conventional spelling, so major
blues repeats a letter for its b3 and 3 and the diminished scales repeat one.

Chords are named from the held pitch classes and the bass: every class is tried
as a root, a reading with the root in the bass wins outright, otherwise the
commoner chord wins and the bass follows a slash. `chords` is ordered by that
priority - **the order is load-bearing**, and matches the ReaScript's table.

`chordNoteName` is the one place the two spellings diverge on purpose: circles
follow the key, chord symbols stop at one accidental, because Gb minor blues
spells notes Bbb and Dbb and no one writes `Bbbmin`.

## Environment

- `raw.githubusercontent.com` is reachable, so JUCE, CLAP, the VST3 SDK and
  Apple's developer docs can be fetched directly. `reaper.fm` and `juce.com`
  are blocked by the sandbox's egress proxy.
- A built CLAP can be checked without a DAW by `dlopen`ing it, resolving
  `clap_entry`, and walking the factory - the same thing a host does when
  scanning. That caught nothing so far but proves the module is well formed.
