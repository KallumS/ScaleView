/*
    ScaleModel - the musical core of ScaleView, with no dependency on JUCE or
    on any host, so it can be unit tested on its own.

    Note names are spelled for the key that is selected rather than always
    being sharps or flats: each degree of a seven-note scale takes the next
    letter of the alphabet and whatever accidental that letter then needs.
    C# major reads C# D# E# F# G# A# B#, while Db major - the same seven
    notes - reads Db Eb F Gb Ab Bb C.

    Ported from the ScaleView Pro ReaScript.
*/

#pragma once

#include <array>
#include <string>
#include <vector>

namespace scaleview
{

// Pitch classes: 0 = C ... 11 = B. These plain names are used for the notes
// that are outside the selected scale, where the key implies no spelling.
inline constexpr std::array<const char*, 12> sharpNames {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
inline constexpr std::array<const char*, 12> flatNames {
    "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };

// The seven letters and the pitch class each one names on its own.
inline constexpr std::array<const char*, 7> letters { "C", "D", "E", "F", "G", "A", "B" };
inline constexpr std::array<int, 7>         letterPitches { 0, 2, 4, 5, 7, 9, 11 };

// Which circle sits where: five black keys on the top row, seven white on the
// bottom, the black ones over the gaps between the white keys they sit between.
inline constexpr std::array<int, 5> blackPitches { 1, 3, 6, 8, 10 };
inline constexpr std::array<int, 7> whitePitches { 0, 2, 4, 5, 7, 9, 11 };
inline constexpr std::array<int, 5> blackSlots   { 1, 2, 4, 5, 6 };

struct Root
{
    const char* name;
    int letter;       // index into letters
    int accidental;   // semitones, -1 flat, +1 sharp

    int pitchClass() const { return ((letterPitches[letter] + accidental) % 12 + 12) % 12; }
};

/*  Both spellings of every pitch class, plus Cb. C# major and Db major are the
    same seven notes spelled differently, so each needs its own entry.
*/
inline const std::vector<Root> roots {
    { "C",  0,  0 }, { "C#", 0,  1 }, { "Db", 1, -1 }, { "D",  1,  0 },
    { "D#", 1,  1 }, { "Eb", 2, -1 }, { "E",  2,  0 }, { "F",  3,  0 },
    { "F#", 3,  1 }, { "Gb", 4, -1 }, { "G",  4,  0 }, { "G#", 4,  1 },
    { "Ab", 5, -1 }, { "A",  5,  0 }, { "A#", 5,  1 }, { "Bb", 6, -1 },
    { "B",  6,  0 }, { "Cb", 0, -1 },
};

/*  intervals are semitones from the root; letterSteps is how many letter names
    each degree sits above the root letter, which is what makes the spelling
    come out right. A seven-note scale walks the letters in order; the others
    follow the conventional spelling, so major blues repeats a letter for its
    b3 and 3 (C D Eb E G A) and the diminished scales repeat one across their
    eight notes.
*/
struct Scale
{
    const char* name;
    std::vector<int> intervals;
    std::vector<int> letterSteps;
};

inline const std::vector<Scale> scales {
    { "Major",            { 0, 2, 4, 5, 7, 9, 11 }, { 0, 1, 2, 3, 4, 5, 6 } },
    { "Minor (Natural)",  { 0, 2, 3, 5, 7, 8, 10 }, { 0, 1, 2, 3, 4, 5, 6 } },
    { "Harmonic Minor",   { 0, 2, 3, 5, 7, 8, 11 }, { 0, 1, 2, 3, 4, 5, 6 } },
    { "Ionian",           { 0, 2, 4, 5, 7, 9, 11 }, { 0, 1, 2, 3, 4, 5, 6 } },
    { "Dorian",           { 0, 2, 3, 5, 7, 9, 10 }, { 0, 1, 2, 3, 4, 5, 6 } },
    { "Phrygian",         { 0, 1, 3, 5, 7, 8, 10 }, { 0, 1, 2, 3, 4, 5, 6 } },
    { "Lydian",           { 0, 2, 4, 6, 7, 9, 11 }, { 0, 1, 2, 3, 4, 5, 6 } },
    { "Mixolydian",       { 0, 2, 4, 5, 7, 9, 10 }, { 0, 1, 2, 3, 4, 5, 6 } },
    { "Aeolian",          { 0, 2, 3, 5, 7, 8, 10 }, { 0, 1, 2, 3, 4, 5, 6 } },
    { "Major Pentatonic", { 0, 2, 4, 7, 9 },        { 0, 1, 2, 4, 5 } },
    { "Minor Pentatonic", { 0, 3, 5, 7, 10 },       { 0, 2, 3, 4, 6 } },
    { "Major Blues",      { 0, 2, 3, 4, 7, 9 },     { 0, 1, 2, 2, 4, 5 } },
    { "Minor Blues",      { 0, 3, 5, 6, 7, 10 },    { 0, 2, 3, 4, 4, 6 } },
    { "Whole Tone",       { 0, 2, 4, 6, 8, 10 },    { 0, 1, 2, 3, 4, 5 } },
    { "Diminished Whole-Half", { 0, 2, 3, 5, 6, 8, 9, 11 }, { 0, 1, 2, 3, 4, 5, 5, 6 } },
    { "Diminished Half-Whole", { 0, 1, 3, 4, 6, 7, 9, 10 }, { 0, 1, 2, 2, 3, 4, 5, 6 } },
};

// Highlight colours offered in the menu; the first is the default. Keep these
// pale: the note names drawn on top of them are dark.
struct Highlight
{
    const char* name;
    int red, green, blue;   // 0..255

    double luminance() const
    {
        return (0.2126 * red + 0.7152 * green + 0.0722 * blue) / 255.0;
    }
};

inline const std::vector<Highlight> highlights {
    { "Teal",        51, 204, 158 },
    { "Orange",     250, 140,  38 },
    { "Light Green", 140, 222, 102 },
    { "White",      242, 245, 250 },
    { "Light Blue", 102, 184, 250 },
    { "Light Pink", 250, 158, 199 },
    { "Gold",       242, 199,  56 },
};

/*  Spells pitch class pc using the given letter, e.g. letter G and pc 6 gives
    "Gb". Returns an empty string when that would need more than a double
    accidental, which only happens in spellings nobody writes.
*/
inline std::string spellAs (int letter, int pitchClass)
{
    letter = ((letter % 7) + 7) % 7;

    const int offset = ((pitchClass - letterPitches[letter] + 6) % 12 + 12) % 12 - 6;

    switch (offset)
    {
        case -2: return std::string (letters[letter]) + "bb";
        case -1: return std::string (letters[letter]) + "b";
        case  0: return letters[letter];
        case  1: return std::string (letters[letter]) + "#";
        case  2: return std::string (letters[letter]) + "x";
        default: return {};
    }
}

/*  Which of the twelve circles are lit, and how each one is named, for one
    key. A default-constructed Key is the "no scale selected" state: nothing
    lit, plain sharp names.
*/
struct Key
{
    std::array<bool, 12> lit {};
    std::array<std::string, 12> names {};
    std::string label { "No scale selected" };
    bool hasScale { false };

    Key()
    {
        for (int pc = 0; pc < 12; ++pc)
            names[static_cast<size_t> (pc)] = sharpNames[static_cast<size_t> (pc)];
    }
};

inline Key buildKey (int rootIndex, int scaleIndex)
{
    Key key;

    if (rootIndex < 0 || rootIndex >= static_cast<int> (roots.size())
        || scaleIndex < 0 || scaleIndex >= static_cast<int> (scales.size()))
        return key;

    const auto& root  = roots[static_cast<size_t> (rootIndex)];
    const auto& scale = scales[static_cast<size_t> (scaleIndex)];
    const int rootPc  = root.pitchClass();

    std::array<bool, 12> spelled {};
    int sharps = 0, flats = 0;

    for (size_t degree = 0; degree < scale.intervals.size(); ++degree)
    {
        const int pc = (rootPc + scale.intervals[degree]) % 12;
        const auto name = spellAs (root.letter + scale.letterSteps[degree], pc);

        key.lit[static_cast<size_t> (pc)] = true;

        if (! name.empty())
        {
            key.names[static_cast<size_t> (pc)] = name;
            spelled[static_cast<size_t> (pc)] = true;

            if (name.find ('#') != std::string::npos || name.find ('x') != std::string::npos)
                ++sharps;
            if (name.find ('b', 1) != std::string::npos)   // skip the note letter B
                ++flats;
        }
    }

    // The notes outside the scale have no spelling of their own, so name them
    // in whichever direction the key leans.
    const auto& outside = flats > sharps ? flatNames : sharpNames;

    for (size_t pc = 0; pc < 12; ++pc)
        if (! spelled[pc])
            key.names[pc] = outside[pc];

    key.label = std::string (root.name) + " " + scale.name;
    key.hasScale = true;
    return key;
}

} // namespace scaleview
