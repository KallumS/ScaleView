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
change it in the other. The chord engine in `ScaleModel.h` is a line-for-line
port of that repository's, and every weight in it was arrived at by breaking a
test there - read its CLAUDE.md before touching one.

Parity is verified by **diffing output**, which is the technique to reuse rather
than eyeballing cases. Write a dumper on each side that prints one line per
case, run both, `diff`:

- **Spelling:** all 288 root/scale combinations, lit notes and every note name
  including the ones outside the scale. Byte-identical.
- **Chords:** 36,283 voicings with no scale selected - every distinct sonority
  from the Bach chorales, the Beethoven quartets, the Chopin mazurkas, the jazz
  standards vocabulary and every three-to-five-note pitch class set - plus
  1,679 voicings in each of ten keys including ones whose notes need double
  accidentals. Byte-identical, about 53,000 names.
- **The doubled-root bass rule:** a sweep cannot test it, because no voicing in
  one doubles a note. It went through 97,346 real voicings taken from the
  corpora - 51,248 of them doubling a pitch class - in six keys: 584,076 names,
  byte-identical.
- **Costing the fifth rather than guessing it.** The held ring is **always
  white**: making it depend on the circle underneath was tried and looked
  broken, because the ring is drawn outside the fill and so contrasts with the
  background rather than with the highlight.
- **The four naming changes from the Scaler comparison** (dim9, two-note chords
  named by their third, the minor sixth against the half-diminished, and a
  third beating a shape with none): 82,478 voicings in six keys, **every
  two-note pair included**, byte-identical. A sweep of three-note-and-up
  voicings cannot reach the two-note rule at all.

The ReaScript's `tools/runner.lua` reads MIDI note numbers on stdin and writes
the name; a twenty-line C++ file doing the same against `chordName` is the other
half. Watch out for `Root::name` being `const char*`: comparing it to `argv[1]`
with `==` compares pointers, which cost a confusing five minutes.

## How the musical core works

Each scale carries `intervals` (semitones from the root) alongside
`letterSteps` (how many letter names each degree sits above the root letter).
A seven-note scale walks the letters in order, so each degree takes the next
letter and whatever accidental it then needs: C# major reads C# D# E# F# G# A# B#
while Db major, the same seven notes, reads Db Eb F Gb Ab Bb C. Scales that
cannot take one letter per degree keep their conventional spelling, so major
blues repeats a letter for its b3 and 3 and the diminished scales repeat one.

Chords are **read, not looked up**. A table is matched exactly, so a voicing it
does not hold reads out as a list of notes, and lengthening it never ends - a
chord is a quality with any number of tones stacked on top. The old table here
named 28% of all three-to-five-note voicings; this names all of them.

The symbol splits in two. The **third, fifth and seventh** are a closed
vocabulary - about thirty combinations, each with an agreed name - so that half
is a table, `coreRank`, ordered by how common the quality is. Everything above
it is **described**: whatever the core did not consume is read off as a sixth,
ninth, eleventh or thirteenth, altered or not. Every candidate root is costed
and the cheapest wins, where the cost covers how unusual the quality is, what
its extensions cost, and whether the root had to be named after a slash - which
is what keeps C E A as `Amin/C` rather than a C6 missing its fifth.

The bass is found separately from the root, which is what makes inversions come
out as slash chords. It is the lowest note sounding, with one exception
(`readAsRootPosition`): a root sounding in more than one octave that is a
first, third or fifth degree of the key reads as root position and loses the
slash, so E G C C is `C` rather than `C/E`. That is how Scaler reads a doubled
root, and it was the last disagreement with it. The rule can only ever remove a
slash, never invent one, and it leaves the cost model alone - the reading is
still chosen with the lowest note as the bass. The degrees come off the
selected scale (`Key::tonic`), so F A D D is `Dmin/F` in C major and `Dmin` in
D minor.

With no scale selected the naming assumes C major (`assumedKey`, and
`assumedTonic` for the rule above) rather than going quiet; the assumption is
invisible, because no circle lights and choosing C major explicitly gives
identical names.

`chordNoteName` is the one place the two spellings diverge on purpose. Circles
follow the key; a chord symbol is written with the eighteen spellings real keys
are built on, the ones in `roots`. Anything else falls back to a plain name
leaning the way the key does - the double accidentals Gb minor blues produces,
because nobody writes `Bbbmin`, and the theoretical spellings nobody builds a
chord on, B# among them.

**Simplify Note Names** is one function, `simplified()`, which overwrites a
key's twelve names with the plain sharp table and clears `usesFlats`. It is
applied in exactly one place - `PluginProcessor::getKey()` - so everything
downstream, circles and chord symbols alike, follows without knowing about it.
Which notes light and what the scale label says are unaffected, because both
come from the root and scale indices rather than from the names. Chord
*detection* is untouched by it; only the names it prints change.

## Environment

- `raw.githubusercontent.com` is reachable, so JUCE, CLAP, the VST3 SDK and
  Apple's developer docs can be fetched directly. `reaper.fm` and `juce.com`
  are blocked by the sandbox's egress proxy.
- A built CLAP can be checked without a DAW by `dlopen`ing it, resolving
  `clap_entry`, and walking the factory - the same thing a host does when
  scanning. That caught nothing so far but proves the module is well formed.
