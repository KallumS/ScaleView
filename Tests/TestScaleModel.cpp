/*
    Tests for the musical core, ported from the ScaleView ReaScript's Lua
    suite so the plugin has to meet the same bar. No JUCE, no host: build and
    run this on its own with
        c++ -std=c++17 Tests/TestScaleModel.cpp -o test && ./test
*/

#include "../Source/ScaleModel.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <set>

using namespace scaleview;

namespace
{
int failures = 0;

void fail (const std::string& message)
{
    std::printf ("FAIL: %s\n", message.c_str());
    ++failures;
}

int indexOfRoot (const std::string& name)
{
    for (size_t i = 0; i < roots.size(); ++i)
        if (name == roots[i].name) return static_cast<int> (i);
    return -1;
}

int indexOfScale (const std::string& name)
{
    for (size_t i = 0; i < scales.size(); ++i)
        if (name == scales[i].name) return static_cast<int> (i);
    return -1;
}

// The scale as it reads from the root upwards.
std::string spelling (const std::string& rootName, const std::string& scaleName)
{
    const int rootIndex = indexOfRoot (rootName);
    const int scaleIndex = indexOfScale (scaleName);
    if (rootIndex < 0 || scaleIndex < 0) { fail ("unknown key " + rootName + " " + scaleName); return {}; }

    const auto key = buildKey (rootIndex, scaleIndex);
    const int rootPc = roots[static_cast<size_t> (rootIndex)].pitchClass();

    std::string out;
    for (int step = 0; step < 12; ++step)
    {
        const auto pc = static_cast<size_t> ((rootPc + step) % 12);
        if (key.lit[pc])
        {
            if (! out.empty()) out += " ";
            out += key.names[pc];
        }
    }
    return out;
}

void expect (const std::string& root, const std::string& scale, const std::string& expected)
{
    const auto got = spelling (root, scale);
    if (got != expected)
        fail (root + " " + scale + " spelled '" + got + "', expected '" + expected + "'");
    else
        std::printf ("  %-26s %s\n", (root + " " + scale).c_str(), got.c_str());
}
} // namespace

int main()
{
    std::printf ("the requested spellings:\n");
    expect ("C#", "Major", "C# D# E# F# G# A# B#");
    expect ("F#", "Major", "F# G# A# B C# D# E#");
    expect ("Gb", "Major", "Gb Ab Bb Cb Db Eb F");
    expect ("Cb", "Major", "Cb Db Eb Fb Gb Ab Bb");
    expect ("Db", "Major", "Db Eb F Gb Ab Bb C");

    std::printf ("other scale types:\n");
    expect ("C",  "Major Pentatonic", "C D E G A");
    expect ("A",  "Minor Pentatonic", "A C D E G");
    expect ("C",  "Major Blues", "C D Eb E G A");
    expect ("C",  "Minor Blues", "C Eb F Gb G Bb");
    expect ("C",  "Whole Tone", "C D E F# G# A#");
    expect ("C",  "Diminished Whole-Half", "C D Eb F Gb Ab A B");
    expect ("C",  "Diminished Half-Whole", "C Db Eb E F# G A Bb");
    expect ("Eb", "Harmonic Minor", "Eb F Gb Ab Bb Cb D");
    expect ("F",  "Dorian", "F G Ab Bb C D Eb");
    expect ("B",  "Lydian", "B C# D# E# F# G# A#");
    expect ("A",  "Harmonic Minor", "A B C D E F G#");

    // Enharmonic pairs: the same circles, spelled differently.
    std::printf ("enharmonic pairs light the same circles:\n");
    for (const auto& pair : std::vector<std::pair<std::string, std::string>> {
             { "C#", "Db" }, { "D#", "Eb" }, { "F#", "Gb" },
             { "G#", "Ab" }, { "A#", "Bb" }, { "B",  "Cb" } })
    {
        const auto a = buildKey (indexOfRoot (pair.first),  indexOfScale ("Major"));
        const auto b = buildKey (indexOfRoot (pair.second), indexOfScale ("Major"));
        if (a.lit != b.lit)
            fail (pair.first + " and " + pair.second + " major light different notes");

        const auto sa = spelling (pair.first, "Major");
        const auto sb = spelling (pair.second, "Major");
        if (sa == sb)
            fail (pair.first + " and " + pair.second + " major are spelled identically");
        std::printf ("  %-3s %-24s = %-3s %s\n", pair.first.c_str(), sa.c_str(),
                     pair.second.c_str(), sb.c_str());
    }

    // Notes outside the scale lean the way the key does.
    {
        const auto f = buildKey (indexOfRoot ("F"), indexOfScale ("Major"));
        if (f.names[1] != "Db" || f.names[6] != "Gb")
            fail ("a flat key should name its outside notes as flats");
        const auto g = buildKey (indexOfRoot ("G"), indexOfScale ("Major"));
        if (g.names[1] != "C#" || g.names[3] != "D#")
            fail ("a sharp key should name its outside notes as sharps");
        std::printf ("notes outside the scale follow the key: flats in F major, sharps in G major\n");
    }

    /*  Across every root and scale: the lit notes are unaffected by spelling,
        every seven-note scale uses each of the seven letters exactly once, and
        nothing needs more than a double accidental.
    */
    int combinations = 0;
    for (size_t s = 0; s < scales.size(); ++s)
    {
        for (size_t r = 0; r < roots.size(); ++r)
        {
            const auto key = buildKey (static_cast<int> (r), static_cast<int> (s));
            const auto& scale = scales[s];
            const int rootPc = roots[r].pitchClass();
            const std::string what = std::string (roots[r].name) + " " + scale.name;

            std::array<bool, 12> want {};
            for (const int interval : scale.intervals)
                want[static_cast<size_t> ((rootPc + interval) % 12)] = true;
            if (key.lit != want)
                fail (what + ": wrong notes lit");

            std::map<char, int> letterUse;
            for (size_t pc = 0; pc < 12; ++pc)
            {
                if (! key.lit[pc]) continue;

                const auto& name = key.names[pc];
                if (name.empty() || name[0] < 'A' || name[0] > 'G')
                    { fail (what + ": unspellable name"); continue; }

                const auto accidental = name.substr (1);
                if (! (accidental.empty() || accidental == "#" || accidental == "b"
                       || accidental == "x" || accidental == "bb"))
                    fail (what + ": odd accidental in " + name);

                ++letterUse[name[0]];
            }

            if (scale.intervals.size() == 7)
            {
                if (letterUse.size() != 7)
                    fail (what + ": does not use all seven letters");
                for (const auto& use : letterUse)
                    if (use.second != 1)
                        fail (what + ": letter " + use.first + " used twice");
            }
            ++combinations;
        }
    }
    std::printf ("all %d root/scale combinations: right notes lit, and every\n", combinations);
    std::printf ("seven-note scale uses each of the seven letters exactly once\n");

    // The fifteen real major keys need no double accidentals.
    for (const std::string root : { "C","G","D","A","E","B","F#","C#","F","Bb","Eb","Ab","Db","Gb","Cb" })
    {
        const auto s = spelling (root, "Major");
        if (s.find ('x') != std::string::npos || s.find ("bb") != std::string::npos)
            fail (root + " major should not need a double accidental: " + s);
    }
    std::printf ("the fifteen standard major keys need no double accidentals\n");

    // Every highlight stays pale enough for the dark note names drawn on it.
    for (const auto& highlight : highlights)
        if (highlight.luminance() <= 0.55)
            fail (std::string (highlight.name) + " is too dark for dark note names");
    std::printf ("all %zu highlight colours are pale enough for dark note names\n", highlights.size());

    // Chords, named from the held notes and the bass.
    {
        auto keyFor = [] (const std::string& root, const std::string& scale)
        {
            return buildKey (indexOfRoot (root), indexOfScale (scale));
        };
        auto named = [] (const Key& key, std::vector<int> notes)
        {
            return chordName (notes, key);
        };
        auto check = [&] (const Key& key, std::vector<int> notes,
                          const std::string& expected, const char* why = "")
        {
            const auto got = named (key, notes);
            if (got != expected)
                fail ("chord " + expected + " came out as '" + got + "'");
            else
                std::printf ("  %-14s %-10s %s\n", expected.c_str(),
                             key.label.c_str(), why);
        };

        const auto none = Key {};
        const int C4 = 60;

        std::printf ("chords:\n");
        check (none, { C4, C4 + 4, C4 + 7 }, "C");
        check (none, { C4, C4 + 3, C4 + 7 }, "Cmin");
        check (none, { C4, C4 + 3, C4 + 7, C4 + 10 }, "Cmin7");
        check (none, { C4, C4 + 4, C4 + 7, C4 + 11 }, "Cmaj7");
        check (none, { 59, 61, 66 }, "Bsus2", "B C# F#");
        check (none, { 52, C4, 67 }, "C/E", "E in the bass");
        check (none, { 45, C4, 64, 67 }, "Amin7", "A in the bass");
        check (none, { 48, 64, 67, 69 }, "C6", "C in the bass");
        check (none, { 43, C4, 64, 69 }, "Amin7/G", "neither in the bass");
        check (none, { C4, C4 + 7 }, "C5");
        check (none, { C4 }, "C");
        check (none, { C4, C4 + 4 }, "C E", "not a chord we know");

        // Roots follow the key, as the scale names do.
        std::printf ("chords spelled for the key:\n");
        check (keyFor ("Gb", "Major"), { 54, 58, 61 }, "Gb", "not F#");
        check (keyFor ("F#", "Major"), { 54, 58, 61 }, "F#", "the same notes");
        check (keyFor ("Cb", "Major"), { 59, 63, 66 }, "Cb");

        /*  But a chord symbol is written with the spellings real keys are
            built on - the eighteen in `roots`. Anything else falls back to a
            plain name: the double accidentals a key like Gb minor blues
            produces, and the theoretical spellings nobody builds a chord on,
            B# among them. Both were reported from the ReaScript.
        */
        std::printf ("chords in keys that need double accidentals:\n");
        check (keyFor ("Gb", "Minor Blues"), { C4, C4 + 4, C4 + 9 }, "Amin/C",
               "key spells these Dbb Fb Bbb");
        check (keyFor ("A#", "Harmonic Minor"), { C4, C4 + 3, C4 + 9 }, "Adim/C",
               "key spells the root Gx and the bass B#");

        /*  What the reader does that a table could not. Each of these came
            out of the old table-based engine as a list of notes, and each
            rule behind them is stated in the ReaScript's CLAUDE.md.
        */
        std::printf ("chords no table held:\n");
        const Key plain {};
        check (plain, { C4, C4 + 4, C4 + 7, C4 + 8, C4 + 10 }, "C7b13", "");
        check (plain, { C4, C4 + 4, C4 + 6, C4 + 7, C4 + 11 }, "Cmaj7#11", "");
        check (plain, { C4, C4 + 3, C4 + 5, C4 + 7, C4 + 10 }, "Cmin7(11)",
               "no ninth, so the number cannot claim one");
        check (plain, { C4, C4 + 2, C4 + 3, C4 + 5, C4 + 7, C4 + 10 }, "Cmin11",
               "and with the ninth, it can");
        check (plain, { C4, C4 + 4, C4 + 7, C4 + 9, C4 + 10 }, "C7(13)", "");
        check (plain, { C4, C4 + 1, C4 + 4, C4 + 7, C4 + 9, C4 + 10 }, "C13b9",
               "an altered ninth still fills the stack");
        check (plain, { C4, C4 + 4, C4 + 7, C4 + 8 }, "Caddb6",
               "a flat sixth is a b6 until a seventh arrives");
        check (plain, { 55, 59, 62, 66, 77 }, "G7(maj7)",
               "both sevenths at once, bracketed");
        check (plain, { 69, 71, 77 }, "F(b5)/A",
               "a third outranks a reading with none");
        check (plain, { C4, C4 + 7, C4 + 10 }, "C7(no3)",
               "but a missing third has to be said");
        check (plain, { C4, C4 + 4, C4 + 9 }, "Amin/C",
               "a complete triad inverted beats a sixth with a hole in it");
        check (plain, { 64, 67, 71, 72, 74 }, "Cmaj9/E",
               "a b13 on a minor triad means the root was picked wrong");

        /*  "Simplify Note Names" gives every note its piano-key name, so the
            double accidentals go and the chord symbols simplify with them -
            but the scale label keeps the key as it was chosen.
        */
        std::printf ("simplify note names:\n");
        const auto gbKey = keyFor ("Gb", "Major");
        const auto plainGb = simplified (gbKey);

        if (gbKey.names[6] != "Gb" || plainGb.names[6] != "F#")
            fail ("Gb major should spell pitch class 6 as Gb, simplified as F#");
        if (plainGb.label != gbKey.label)
            fail ("the scale label should survive simplifying: " + plainGb.label);
        if (plainGb.lit != gbKey.lit)
            fail ("simplifying must not change which notes are lit");

        const auto gx = keyFor ("A#", "Harmonic Minor");
        if (gx.names[9] != "Gx" || simplified (gx).names[9] != "A")
            fail ("A# harmonic minor spells pitch class 9 Gx, simplified A");

        check (plainGb, { 54, 58, 61 }, "F#", "the chord symbol simplifies too");
        std::printf ("  Gb Major reads F# simplified, and still says Gb Major\n");

        //  No white highlight: the ring around a played note is white and a
        //  white highlight would swallow it.
        for (const auto& highlight : highlights)
            if (std::string (highlight.name) == "White")
                fail ("White is still in the palette; it clashes with the rings");
        std::printf ("highlights: %zu colours, none of them white\n", highlights.size());

        // The circles keep the key's spelling: only the symbol simplifies.
        const auto sharpKey = keyFor ("A#", "Harmonic Minor");
        if (sharpKey.names[9] != "Gx")
            fail ("the circles should still spell pitch class 9 as Gx");
    }

    /*  Random Scale: lands on a real key, never the one already showing, and
        spreads rather than sticking.
    */
    {
        std::mt19937 rng { std::random_device {}() };
        std::uniform_int_distribution<int> rootPick (0, static_cast<int> (roots.size()) - 1);
        std::uniform_int_distribution<int> scalePick (0, static_cast<int> (scales.size()) - 1);

        int currentRoot = -1, currentScale = -1;
        std::set<std::string> seen;
        std::set<int> seenRoots;

        for (int i = 0; i < 60; ++i)
        {
            int root = 0, scale = 0;
            do { root = rootPick (rng); scale = scalePick (rng); }
            while (root == currentRoot && scale == currentScale);

            if (root == currentRoot && scale == currentScale)
                fail ("random pick repeated the current scale");

            currentRoot = root;
            currentScale = scale;

            const auto key = buildKey (root, scale);
            if (! key.hasScale) fail ("random pick produced no scale");
            seen.insert (key.label);
            seenRoots.insert (root);
        }

        if (seen.size() < 20 || seenRoots.size() < 5)
            fail ("random picks look stuck");
        std::printf ("random scale: 60 picks, %zu distinct over %zu roots, no repeats\n",
                     seen.size(), seenRoots.size());
    }

    if (failures > 0)
    {
        std::printf ("%d FAILURE(S)\n", failures);
        return 1;
    }
    std::printf ("PASS\n");
    return 0;
}
