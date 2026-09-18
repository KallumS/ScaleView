/*
    Tests the MIDI path, which the model tests cannot reach: notes arriving on
    the audio thread, note-offs, all-notes-off, and that the plugin passes the
    MIDI on rather than consuming it.

    Needs JUCE, so it is built as a console app by the main CMakeLists.
*/

#include <juce_audio_processors/juce_audio_processors.h>

#include "../Source/PluginProcessor.h"

namespace
{
int failures = 0;

void fail (const juce::String& message)
{
    std::cout << "FAIL: " << message << std::endl;
    ++failures;
}

void check (bool condition, const juce::String& what)
{
    if (! condition) fail (what);
    else std::cout << "  " << what << std::endl;
}

juce::MidiBuffer noteOn (int note, juce::uint8 velocity = 100)
{
    juce::MidiBuffer buffer;
    buffer.addEvent (juce::MidiMessage::noteOn (1, note, velocity), 0);
    return buffer;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    ScaleViewProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> audio (2, 512);
    auto run = [&] (juce::MidiBuffer midi)
    {
        audio.clear();
        processor.processBlock (audio, midi);
        return midi;   // what is left for the next plugin in the chain
    };

    auto chordNow = [&]
    {
        return juce::String (scaleview::chordName (processor.getHeldNotes(), processor.getKey()));
    };

    std::cout << "MIDI reaching the processor:" << std::endl;

    // A chord played and held.
    juce::MidiBuffer chord;
    chord.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
    chord.addEvent (juce::MidiMessage::noteOn (1, 63, (juce::uint8) 100), 10);
    chord.addEvent (juce::MidiMessage::noteOn (1, 67, (juce::uint8) 100), 20);
    auto passed = run (chord);

    check (processor.getHeldNotes() == std::vector<int> ({ 60, 63, 67 }),
           "three note-ons are held, lowest first");
    check (chordNow() == "Cmin", "and they name the chord: " + chordNow());
    check (passed.getNumEvents() == 3,
           "the MIDI is passed on, not consumed (" + juce::String (passed.getNumEvents()) + " events out)");

    // Held across blocks with no MIDI at all.
    run ({});
    run ({});
    check (chordNow() == "Cmin", "still held after silent blocks");

    // Note-off, and note-on with velocity zero, both release.
    juce::MidiBuffer off;
    off.addEvent (juce::MidiMessage::noteOff (1, 63), 0);
    run (off);
    check (chordNow() == "C5", "a note-off releases: " + chordNow());

    run (noteOn (63));
    check (chordNow() == "Cmin", "and it can be played again");

    juce::MidiBuffer zeroVelocity;
    zeroVelocity.addEvent (juce::MidiMessage::noteOn (1, 63, (juce::uint8) 0), 0);
    run (zeroVelocity);
    check (chordNow() == "C5", "a note-on at velocity zero also releases");

    // All notes off clears everything.
    juce::MidiBuffer panic;
    panic.addEvent (juce::MidiMessage::allNotesOff (1), 0);
    run (panic);
    check (processor.getHeldNotes().empty(), "all-notes-off clears every held note");
    check (chordNow().isEmpty(), "and the chord label goes away");

    // The transport moving clears anything left hanging.
    run (noteOn (60));
    processor.prepareToPlay (48000.0, 512);
    check (processor.getHeldNotes().empty(), "prepareToPlay drops stuck notes");

    // The full MIDI range, including the top and bottom of the two masks.
    for (const int note : { 0, 63, 64, 127 })
    {
        run (noteOn (note));
        const auto held = processor.getHeldNotes();
        if (std::find (held.begin(), held.end(), note) == held.end())
            fail ("note " + juce::String (note) + " was not tracked");
    }
    check (processor.getHeldNotes() == std::vector<int> ({ 0, 63, 64, 127 }),
           "notes at both ends of both 64-bit masks are tracked");

    // The chord follows the key, since the editor spells it from the key.
    processor.setScale (9, 0);    // Gb major
    run (juce::MidiBuffer());
    juce::MidiBuffer gb;
    gb.addEvent (juce::MidiMessage::allNotesOff (1), 0);
    gb.addEvent (juce::MidiMessage::noteOn (1, 54, (juce::uint8) 100), 1);
    gb.addEvent (juce::MidiMessage::noteOn (1, 58, (juce::uint8) 100), 2);
    gb.addEvent (juce::MidiMessage::noteOn (1, 61, (juce::uint8) 100), 3);
    run (gb);
    check (chordNow() == "Gb", "in Gb major the chord reads Gb, not F#");

    // State round trip, which no test has covered until now.
    processor.setScale (17, 2);   // Cb harmonic minor
    processor.setHighlightIndex (4);
    juce::MemoryBlock state;
    processor.getStateInformation (state);

    ScaleViewProcessor restored;
    restored.setStateInformation (state.getData(), (int) state.getSize());
    check (restored.getRootIndex() == 17 && restored.getScaleIndex() == 2
           && restored.getHighlightIndex() == 4,
           "the selection survives a save and reload of plugin state");
    check (restored.getKey().label == processor.getKey().label,
           "and restores the same key: " + juce::String (restored.getKey().label));

    /*  Render the editor. Nothing else exercises paint(), and a chord being
        held changes what it draws - the ring around each held note and the
        chord name in place of the scale name.

        Pass a filename to also write the image out, which is how the
        screenshots in the README were made.
    */
    {
        processor.setScale (9, 0);            // Gb major
        juce::MidiBuffer notes;
        notes.addEvent (juce::MidiMessage::allNotesOff (1), 0);
        notes.addEvent (juce::MidiMessage::noteOn (1, 54, (juce::uint8) 100), 1);
        notes.addEvent (juce::MidiMessage::noteOn (1, 58, (juce::uint8) 100), 2);
        notes.addEvent (juce::MidiMessage::noteOn (1, 61, (juce::uint8) 100), 3);
        notes.addEvent (juce::MidiMessage::noteOn (1, 65, (juce::uint8) 100), 4);
        run (notes);

        std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
        editor->setSize (400, 200);

        juce::Image image (juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
        {
            juce::Graphics g (image);
            editor->paintEntireComponent (g, true);
        }

        //  The ring drawn around a held note is #FFF200; nothing else in the
        //  icon is. Keep this in step with colourHeld in PluginEditor.cpp.
        int ringPixels = 0;
        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto pixel = image.getPixelAt (x, y);
                if (pixel.getRed() > 250 && pixel.getGreen() > 237
                    && pixel.getGreen() < 247 && pixel.getBlue() < 8)
                    ++ringPixels;
            }

        check (ringPixels > 100, "the editor draws rings around the held notes ("
               + juce::String (ringPixels) + " ring pixels)");

        if (auto* path = std::getenv ("SCALEVIEW_RENDER_TO"))
        {
            juce::File file (path);
            juce::PNGImageFormat png;
            std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
            if (stream != nullptr && png.writeImageToStream (image, *stream))
                std::cout << "  wrote " << path << std::endl;
        }
    }

    if (failures > 0)
    {
        std::cout << failures << " FAILURE(S)" << std::endl;
        return 1;
    }
    std::cout << "PASS" << std::endl;
    return 0;
}
