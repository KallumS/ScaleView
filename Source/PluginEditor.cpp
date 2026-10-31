#include "PluginEditor.h"

namespace
{
// The icon's proportions. It keeps this shape inside whatever the host gives
// it, centred, rather than stretching.
constexpr float iconAspect = 2.0f;

const juce::Colour colourBackground { 35, 39, 46 };
const juce::Colour colourUnlit      { 58, 64, 74 };     // #3A404A
const juce::Colour colourEdge       { 109, 117, 129 };  // #6D7581: rim of an unlit circle, 3.2:1 on the background
const juce::Colour colourLabel      { 191, 197, 206 };  // #BFC5CE
const juce::Colour colourNameUnlit  { 221, 225, 231 };  // #DDE1E7: 8.0:1 on colourUnlit
const juce::Colour colourNameLit    { 20, 23, 28 };   // #14171c, on a lit circle
const juce::Colour colourHeld       { 255, 242,   0 };   // #FFF200, ring around a played note, always
const juce::Colour colourChord      { 242, 244, 247 };  // #F2F4F7, brighter than a scale name
const juce::Colour colourMenu       { 42, 47, 55 };     // #2A2F37, a menu: a step lighter than the icon
const juce::Colour colourMenuEdge   { 58, 64, 74 };     // #3A404A, its edge, and the item under the pointer

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
    : AudioProcessorEditor (&p), processor (p),
      face (systemFaces().isEmpty() ? juce::String()
                                    : pickFace (juce::Font::findAllTypefaceNames(), systemFaces()))
{
    menuLook.face = face;
    setLookAndFeel (&menuLook);   // the menus take it from their target, this editor
    processor.addChangeListener (this);

    setResizable (true, true);
    setWantsKeyboardFocus (true);

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
    setLookAndFeel (nullptr);
}

//==============================================================================
void ScaleViewEditor::refreshChord()
{
    const auto notes = processor.getHeldNotes();

    heldClasses = {};
    for (const int note : notes)
        heldClasses[static_cast<size_t> (note % 12)] = true;

    chordLabel = juce::String (scaleview::chordName (notes, processor.getKey()));
    updateAccessibleTitle();
}

void ScaleViewEditor::updateAccessibleTitle()
{
    const auto key = processor.getKey();
    auto title = juce::String ("ScaleView: ") + juce::String (key.label);
    if (chordLabel.isNotEmpty())
        title << ", playing " << chordLabel;

    if (title == getTitle())
        return;

    setTitle (title);
    if (auto* handler = getAccessibilityHandler())
        handler->notifyAccessibilityEvent (juce::AccessibilityEvent::titleChanged);
}

std::unique_ptr<juce::AccessibilityHandler> ScaleViewEditor::createAccessibilityHandler()
{
    // The icon is a picture with a caption: VoiceOver reads the title.
    return std::make_unique<juce::AccessibilityHandler> (*this, juce::AccessibilityRole::image);
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

//==============================================================================
ScaleViewEditor::MenuLook::MenuLook()
{
    using PM = juce::PopupMenu;
    setColour (PM::backgroundColourId,            colourMenu);
    setColour (PM::textColourId,                  colourNameUnlit);  // #DDE1E7, 10.2:1
    setColour (PM::headerTextColourId,            colourLabel);      // #BFC5CE, 7.7:1
    setColour (PM::highlightedBackgroundColourId, colourMenuEdge);
    setColour (PM::highlightedTextColourId,       colourChord);      // #F2F4F7, 9.4:1
}

void ScaleViewEditor::MenuLook::drawPopupMenuBackgroundWithOptions (juce::Graphics& g,
                                                                    int width, int height,
                                                                    const juce::PopupMenu::Options&)
{
    g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
    g.setColour (colourMenuEdge);
    g.drawRect (0, 0, width, height);
}

juce::Font ScaleViewEditor::MenuLook::getPopupMenuFont()
{
    auto font = LookAndFeel_V4::getPopupMenuFont();
    if (face.isNotEmpty())
        font.setTypefaceName (face);
    return font;
}

juce::StringArray ScaleViewEditor::systemFaces()
{
   #if JUCE_WINDOWS
    return { "Segoe UI" };   // Windows' own interface face since Vista
   #else
    /*  Not on a Mac: San Francisco is hidden from lookup by name, and asking
        CoreText for ".AppleSystemUIFont" or ".SFNS..." by name gets Times
        Roman on recent macOS, with a console note to use the system-font
        APIs instead - which JUCE's Font does not expose. So the Mac keeps
        JUCE's default sans-serif (Helvetica), as does Linux. */
    return {};
   #endif
}

juce::String ScaleViewEditor::pickFace (const juce::StringArray& installed,
                                        const juce::StringArray& wanted)
{
    for (const auto& face : wanted)
        if (installed.contains (face, true))
            return face;

    return {};   // JUCE's default sans-serif
}

juce::Font ScaleViewEditor::textFont (float height) const
{
    return face.isEmpty() ? juce::Font (juce::FontOptions (height))
                          : juce::Font (juce::FontOptions (face, height, juce::Font::plain));
}

void ScaleViewEditor::drawCircle (juce::Graphics& g, float centreX, float centreY,
                                  float radius, int pitchClass, const scaleview::Key& key,
                                  juce::Colour highlight) const
{
    const bool lit = key.lit[(size_t) pitchClass];

    auto inner = radius;
    if (! lit)
    {
        /*  A dark circle on the dark background is 1.4:1, too faint to show
            where the note sits (Apple's 3:1 for a shape that means something).
            A rim of colourEdge, 3.2:1, shows it and the fill stays dark, so
            the notes outside the key still recede. Drawn as a filled circle
            under the fill, so the only stroke is still the played-note ring.
            As ScaleView Pro does (its decision 0013, final sweep). */
        g.setColour (colourEdge);
        g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);
        inner = radius - juce::jmax (1.0f, radius / 12.0f);
    }

    g.setColour (lit ? highlight : colourUnlit);
    g.fillEllipse (centreX - inner, centreY - inner, inner * 2.0f, inner * 2.0f);

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

    if (! processor.getShowNoteNames() || radius < minimumNameRadius)
        return;

    const auto name = key.names[(size_t) pitchClass];

    // "Bbb" has to fit the same circle as "B".
    const float fit = name.length() >= 3 ? 0.62f : (name.length() == 2 ? 0.80f : 0.95f);

    g.setFont (textFont (juce::jmax (minimumTextSize, radius * fit)));
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

        g.setFont (textFont (juce::jmax (minimumTextSize, layout.labelHeight * 0.62f)));
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

ScaleViewEditor::KeyMenu ScaleViewEditor::menuForKey (const juce::KeyPress& key)
{
    if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown()
        || key.getModifiers().isAltDown())
        return KeyMenu::none;

    // A letter's key code is its capital; the text character may be absent.
    const auto typed = key.getTextCharacter() != 0 ? key.getTextCharacter()
                                                   : (juce::juce_wchar) key.getKeyCode();
    const auto c = juce::CharacterFunctions::toLowerCase (typed);
    if (c == 's' || key.getKeyCode() == juce::KeyPress::returnKey)
        return KeyMenu::scales;
    if (c == 'o')
        return KeyMenu::options;
    return KeyMenu::none;
}

bool ScaleViewEditor::keyPressed (const juce::KeyPress& key)
{
    switch (menuForKey (key))
    {
        case KeyMenu::scales:  showScaleMenu();   return true;
        case KeyMenu::options: showOptionsMenu(); return true;
        case KeyMenu::none:    break;
    }
    return false;   // let the host have it
}

void ScaleViewEditor::showScaleMenu()
{
    juce::PopupMenu menu;
    menu.addItem (clearScaleId, "Clear Scale", true, processor.getScaleIndex() < 0);
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
    menu.addItem (randomScaleId, "Choose a Random Scale");
    menu.addSeparator();
    menu.addItem (showNoteNamesId, "Show Note Names", true, processor.getShowNoteNames());
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
