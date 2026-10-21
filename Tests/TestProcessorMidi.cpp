/*
    Tests the MIDI path, which the model tests cannot reach: notes arriving on
    the audio thread, note-offs, all-notes-off, and that the plugin passes the
    MIDI on rather than consuming it.

    Needs JUCE, so it is built as a console app by the main CMakeLists.
*/

#include <juce_audio_processors/juce_audio_processors.h>

#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

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
// WCAG's relative luminance and contrast ratio, as Apple's Accessibility
// page measures it.
double luminance (juce::Colour c)
{
    auto channel = [] (juce::uint8 v)
    {
        const auto x = v / 255.0;
        return x <= 0.04045 ? x / 12.92 : std::pow ((x + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel (c.getRed()) + 0.7152 * channel (c.getGreen())
         + 0.0722 * channel (c.getBlue());
}

double contrast (juce::Colour a, juce::Colour b)
{
    const auto la = luminance (a), lb = luminance (b);
    return (juce::jmax (la, lb) + 0.05) / (juce::jmin (la, lb) + 0.05);
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

    //  The highlight is stored by name, so a name that is no longer in the
    //  palette has to fall back to the default rather than break. Gold was
    //  swapped out for White; this is the same guard the ReaScript suites
    //  keep, and it is why reordering the table cannot repoint a choice.
    //  The root and scale are checked too, so a state block that failed to
    //  parse at all could not pass this by leaving the default in place.
    {
        auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize());
        check (xml != nullptr, "the saved state is readable XML");
        xml->setAttribute ("highlight", "Gold");

        juce::MemoryBlock patched;
        juce::AudioProcessor::copyXmlToBinary (*xml, patched);

        ScaleViewProcessor dropped;
        dropped.setStateInformation (patched.getData(), (int) patched.getSize());
        check (dropped.getHighlightIndex() == 0
               && dropped.getRootIndex() == 17 && dropped.getScaleIndex() == 2,
               "a highlight name no longer in the palette falls back to the default");
    }
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

        /*  Apple's HIG Foundations (ScaleView for REAPER's decision 0013).
            C is outside Gb major, so its circle is unlit: #3A404A, under the
            name's letters #DDE1E7 at 8.0:1. Sampled below the letter. */
        {
            const float step = 400.0f / 7.0f, radius = juce::jmin (step * 0.40f, 160.0f * 0.20f);
            const auto pixel = image.getPixelAt ((int) (0.5f * step), (int) (160.0f * 0.76f + radius * 0.75f));
            check (std::abs (pixel.getRed() - 58) <= 2 && std::abs (pixel.getGreen() - 64) <= 2
                   && std::abs (pixel.getBlue() - 74) <= 2,
                   "an unlit circle is #3A404A (got " + pixel.toDisplayString (false) + ")");
        }

        /*  The final sweep: every circle's outer edge shows 3:1 against the
            background, with every highlight. C (unlit in Gb major) has a
            #6D7581 rim; B (lit) is the highlight itself. Walk down from each
            centre to just past its edge - short of where a ring would be -
            and take the strongest contrast found there. */
        {
            const juce::Colour background (35, 39, 46);
            const float step = 400.0f / 7.0f, radius = juce::jmin (step * 0.40f, 160.0f * 0.20f);
            const float centreY = 160.0f * 0.76f;
            const auto savedHighlight = processor.getHighlightIndex();

            for (size_t h = 0; h < scaleview::highlights.size(); ++h)
            {
                processor.setHighlightIndex ((int) h);
                juce::Image shot (juce::Image::ARGB, 400, 200, true);
                {
                    juce::Graphics g (shot);
                    editor->paintEntireComponent (g, true);
                }

                for (const auto slot : { 0, 6 })   // C unlit, B lit
                {
                    const auto x = (int) (((float) slot + 0.5f) * step);
                    double best = 1.0;
                    for (int y = (int) (centreY + radius * 0.85f); y <= (int) (centreY + radius + 1.0f); ++y)
                        best = juce::jmax (best, contrast (shot.getPixelAt (x, y), background));

                    check (best >= 3.0, juce::String (slot == 0 ? "an unlit" : "a lit")
                           + " circle's edge is 3:1 on the background with "
                           + scaleview::highlights[h].name + " ("
                           + juce::String (best, 2) + ":1)");
                }
            }
            processor.setHighlightIndex (savedHighlight);
        }

        /*  The system face (Apple's Typography page): the first wanted face
            that is installed, else JUCE's default; never one not installed. */
        check (ScaleViewEditor::pickFace ({ "Arial", "Segoe UI" }, { "Segoe UI" }) == "Segoe UI",
               "the system face is used when installed");
        check (ScaleViewEditor::pickFace ({ "Arial", "Verdana" }, { "Segoe UI" }).isEmpty(),
               "a system face not installed is not asked for");
       #if JUCE_WINDOWS
        check (ScaleViewEditor::systemFaces().contains ("Segoe UI"), "Windows asks for Segoe UI");
       #else
        check (ScaleViewEditor::systemFaces().isEmpty(), "this platform keeps JUCE's default sans-serif");
       #endif

        auto* view = dynamic_cast<ScaleViewEditor*> (editor.get());
        check (view != nullptr, "the editor is ScaleView's");
        if (view != nullptr)
        {
            using KM = ScaleViewEditor::KeyMenu;
            check (ScaleViewEditor::menuForKey (juce::KeyPress ('s')) == KM::scales
                   && ScaleViewEditor::menuForKey (juce::KeyPress ('S', juce::ModifierKeys::shiftModifier, 'S')) == KM::scales
                   && ScaleViewEditor::menuForKey (juce::KeyPress (juce::KeyPress::returnKey)) == KM::scales,
                   "S and Return open the scale list");
            check (ScaleViewEditor::menuForKey (juce::KeyPress ('o')) == KM::options,
                   "O opens the options");
            check (ScaleViewEditor::menuForKey (juce::KeyPress ('x')) == KM::none
                   && ScaleViewEditor::menuForKey (juce::KeyPress ('s', juce::ModifierKeys::commandModifier, 's')) == KM::none,
                   "other keys, and Cmd+S, are left to the host");
            check (view->getWantsKeyboardFocus(), "the editor takes keyboard focus");

            /*  The menus are JUCE's drawing, so they take the house colours: a
                surface lighter than the icon, words 7:1, a #3A404A edge and no
                stripes (JUCE's default draws one every third line). */
            {
                using PM = juce::PopupMenu;
                auto& look = view->getLookAndFeel();
                const juce::Colour ground (35, 39, 46);
                const auto menu = look.findColour (PM::backgroundColourId);
                check (luminance (menu) > luminance (ground)
                       && menu.getRed() < menu.getGreen() && menu.getGreen() < menu.getBlue(),
                       "a menu is a blue-shifted grey lighter than the icon (#" + menu.toDisplayString (false) + ")");
                check (contrast (look.findColour (PM::textColourId), menu) >= 7.0
                       && contrast (look.findColour (PM::headerTextColourId), menu) >= 7.0
                       && contrast (look.findColour (PM::highlightedTextColourId),
                                    look.findColour (PM::highlightedBackgroundColourId)) >= 7.0,
                       "a menu's words are 7:1, under the pointer too");

                juce::Image surface (juce::Image::ARGB, 120, 60, true);
                {
                    juce::Graphics g (surface);
                    look.drawPopupMenuBackgroundWithOptions (g, 120, 60, PM::Options());
                }
                bool plain = true;
                for (int y = 1; y < 59; ++y)
                    plain = plain && surface.getPixelAt (60, y) == menu;
                check (plain && surface.getPixelAt (0, 30) == juce::Colour (58, 64, 74)
                       && surface.getPixelAt (60, 0) == juce::Colour (58, 64, 74),
                       "a menu is one plain colour inside a #3A404A edge");
            }
            check (view->getFace() == ScaleViewEditor::pickFace (juce::Font::findAllTypefaceNames(),
                                                                 ScaleViewEditor::systemFaces()),
                   "the editor uses the face it picked: \"" + view->getFace() + "\"");

            check (view->getTitle().contains (processor.getKey().label)
                   && view->getTitle().contains ("playing"),
                   "VoiceOver reads the key and the chord: " + view->getTitle());
            // JUCE hands out a component's handler only once it is on screen,
            // so ask for the one the editor would give.
            const auto handler = view->createAccessibilityHandler();
            check (handler != nullptr && handler->getRole() == juce::AccessibilityRole::image,
                   "the icon is an image to accessibility");
        }

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
