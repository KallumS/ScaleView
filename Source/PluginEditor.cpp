#include "PluginEditor.h"

namespace
{
// The icon's proportions. It keeps this shape inside whatever the host gives
// it, centred, rather than stretching.
constexpr float iconAspect = 2.0f;

const juce::Colour colourBackground { 35, 39, 46 };
const juce::Colour colourUnlit      { 77, 79, 89 };
const juce::Colour colourLabel      { 184, 189, 204 };
const juce::Colour colourNameUnlit  { 158, 163, 179 };
const juce::Colour colourNameLit    { 20, 23, 28 };   // #14171c, on a lit circle
const juce::Colour colourHeld       { 255, 242,   0 };   // #FFF200, ring around a played note, always
const juce::Colour colourChord      { 242, 245, 250 };   // brighter than a scale name

juce::Colour highlightColour (int index)
{
    const auto& highlight = scaleview::highlights[(size_t) juce::jlimit (
        0, (int) scaleview::highlights.size() - 1, index)];
    return juce::Colour ((juce::uint8) highlight.red,
                         (juce::uint8) highlight.green,
                         (juce::uint8) highlight.blue);
}

// Menu ids. Roots and scales are packed into one id so the menu can be built
// from the tables without a parallel lookup.
enum MenuIds
{
    clearScaleId = 1,
    randomScaleId,
    showNoteNamesId,
    simpleNamesId,
    highlightBaseId = 100,
    scaleBaseId     = 1000   // scaleBaseId + scaleIndex * 100 + rootIndex
};
} // namespace

//==============================================================================
ScaleViewEditor::ScaleViewEditor (ScaleViewProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    processor.addChangeListener (this);

    setResizable (true, true);

    if (auto* sizeConstrainer = getConstrainer())
    {
        sizeConstrainer->setFixedAspectRatio (iconAspect);
        sizeConstrainer->setSizeLimits (200, 100, 1600, 800);
    }

    const auto size = processor.getEditorSize();
    setSize (size.x, size.y);

    refreshChord();
    startTimerHz (30);
}

ScaleViewEditor::~ScaleViewEditor()
{
    stopTimer();
    processor.removeChangeListener (this);
}

//==============================================================================
void ScaleViewEditor::refreshChord()
{
    const auto notes = processor.getHeldNotes();

    heldClasses = {};
    for (const int note : notes)
        heldClasses[static_cast<size_t> (note % 12)] = true;

    chordLabel = juce::String (scaleview::chordName (notes, processor.getKey()));
}

void ScaleViewEditor::timerCallback()
{
    const auto mask = processor.getHeldMask();
    if (mask == lastHeldMask) return;

    lastHeldMask = mask;
    refreshChord();
    repaint();
}

//==============================================================================
ScaleViewEditor::Layout ScaleViewEditor::getLayout() const
{
    const auto width  = (float) getWidth();
    const auto height = (float) getHeight();

    const auto boxWidth  = juce::jmin (width, height * iconAspect);
    const auto boxHeight = juce::jmin (height, width / iconAspect);
    const auto originX   = (width - boxWidth) * 0.5f;
    const auto originY   = (height - boxHeight) * 0.5f;

    const auto step        = boxWidth / 7.0f;          // one white-key slot
    const auto labelHeight = boxHeight * 0.20f;
    const auto radius      = juce::jmin (step * 0.40f, (boxHeight - labelHeight) * 0.20f);

    return { originX,
             step,
             radius,
             originY + (boxHeight - labelHeight) * 0.33f,
             originY + (boxHeight - labelHeight) * 0.76f,
             originY + boxHeight - labelHeight,
             labelHeight,
             boxWidth };
}

void ScaleViewEditor::drawCircle (juce::Graphics& g, float centreX, float centreY,
                                  float radius, int pitchClass, const scaleview::Key& key,
                                  juce::Colour highlight) const
{
    const bool lit = key.lit[(size_t) pitchClass];

    g.setColour (lit ? highlight : colourUnlit);
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    if (heldClasses[static_cast<size_t> (pitchClass)])
    {
        /*  Always colourHeld, no exceptions - making it depend on the circle
            underneath was tried and looked broken. The ring is drawn OUTSIDE
            the filled circle, so what it has to contrast with is the
            background, not the fill, and #FFF200 reads 12.8:1 there. */
        g.setColour (colourHeld);
        g.drawEllipse (centreX - radius - 2.0f, centreY - radius - 2.0f,
                       (radius + 2.0f) * 2.0f, (radius + 2.0f) * 2.0f, 1.5f);
    }

    if (! processor.getShowNoteNames() || radius < 7.0f)
        return;

    const auto name = key.names[(size_t) pitchClass];

    // "Bbb" has to fit the same circle as "B".
    const float fit = name.length() >= 3 ? 0.62f : (name.length() == 2 ? 0.80f : 0.95f);

    g.setFont (juce::Font (juce::FontOptions (juce::jmax (7.0f, radius * fit))));
    g.setColour (lit ? colourNameLit : colourNameUnlit);
    g.drawText (name, juce::Rectangle<float> (centreX - radius, centreY - radius,
                                              radius * 2.0f, radius * 2.0f),
                juce::Justification::centred, false);
}

void ScaleViewEditor::paint (juce::Graphics& g)
{
    g.fillAll (colourBackground);

    const auto layout = getLayout();
    const auto key = processor.getKey();
    const auto highlight = highlightColour (processor.getHighlightIndex());

    // Top row: the five black keys, over the gaps between the white keys.
    for (size_t i = 0; i < scaleview::blackPitches.size(); ++i)
        drawCircle (g, layout.originX + (float) scaleview::blackSlots[i] * layout.step,
                    layout.topY, layout.radius, scaleview::blackPitches[i], key, highlight);

    // Bottom row: the seven white keys.
    for (size_t i = 0; i < scaleview::whitePitches.size(); ++i)
        drawCircle (g, layout.originX + ((float) i + 0.5f) * layout.step,
                    layout.bottomY, layout.radius, scaleview::whitePitches[i], key, highlight);

    if (layout.labelHeight > 6.0f)
    {
        const auto showingChord = chordLabel.isNotEmpty();

        g.setFont (juce::Font (juce::FontOptions (juce::jmax (9.0f, layout.labelHeight * 0.62f))));
        g.setColour (showingChord ? colourChord : colourLabel);
        g.drawText (showingChord ? chordLabel : juce::String (key.label),
                    juce::Rectangle<float> (layout.originX, layout.labelY,
                                            layout.boxWidth, layout.labelHeight),
                    juce::Justification::centred, false);
    }
}

void ScaleViewEditor::resized()
{
    processor.setEditorSize (getWidth(), getHeight());
}

//==============================================================================
void ScaleViewEditor::mouseUp (const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
        showOptionsMenu();
    else
        showScaleMenu();
}

void ScaleViewEditor::showScaleMenu()
{
    juce::PopupMenu menu;
    menu.addItem (clearScaleId, "Clear scale", true, processor.getScaleIndex() < 0);
    menu.addSeparator();

    for (size_t scaleIndex = 0; scaleIndex < scaleview::scales.size(); ++scaleIndex)
    {
        juce::PopupMenu roots;

        for (size_t rootIndex = 0; rootIndex < scaleview::roots.size(); ++rootIndex)
        {
            const bool ticked = (int) rootIndex == processor.getRootIndex()
                             && (int) scaleIndex == processor.getScaleIndex();

            roots.addItem (scaleBaseId + (int) scaleIndex * 100 + (int) rootIndex,
                           juce::String (scaleview::roots[rootIndex].name) + " "
                               + scaleview::scales[scaleIndex].name,
                           true, ticked);
        }

        menu.addSubMenu (scaleview::scales[scaleIndex].name, roots);
    }

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [this] (int result)
                        {
                            if (result == clearScaleId)
                                processor.clearScale();
                            else if (result >= scaleBaseId)
                                processor.setScale ((result - scaleBaseId) % 100,
                                                    (result - scaleBaseId) / 100);
                        });
}

void ScaleViewEditor::showOptionsMenu()
{
    juce::PopupMenu menu;
    menu.addItem (randomScaleId, "Random Scale");
    menu.addSeparator();
    menu.addItem (showNoteNamesId, "Show note names", true, processor.getShowNoteNames());
    menu.addItem (simpleNamesId, "Simplify Note Names", true, processor.getSimpleNames());

    juce::PopupMenu colours;
    for (size_t i = 0; i < scaleview::highlights.size(); ++i)
        colours.addItem (highlightBaseId + (int) i, scaleview::highlights[i].name,
                         true, (int) i == processor.getHighlightIndex());

    menu.addSubMenu ("Highlight Colour", colours);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [this] (int result)
                        {
                            if (result == randomScaleId)
                                processor.setRandomScale();
                            else if (result == showNoteNamesId)
                                processor.setShowNoteNames (! processor.getShowNoteNames());
                            else if (result == simpleNamesId)
                                processor.setSimpleNames (! processor.getSimpleNames());
                            else if (result >= highlightBaseId && result < scaleBaseId)
                                processor.setHighlightIndex (result - highlightBaseId);
                        });
}
