#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

/*  The icon: twelve circles, five on the top row for the black keys of an
    octave and seven on the bottom for the white keys, with the notes of the
    selected scale lit. Left-click for the scale list, right-click for the
    options.
*/
class ScaleViewEditor final : public juce::AudioProcessorEditor,
                              private juce::ChangeListener,
                              private juce::Timer
{
public:
    explicit ScaleViewEditor (ScaleViewProcessor&);
    ~ScaleViewEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        refreshChord();   // the chord is spelled for the key, so it changes with it
        repaint();
    }

    /*  The audio thread cannot tell the editor anything safely, so the editor
        watches instead: 30 times a second, see whether the held notes have
        changed, and only then rename the chord and repaint.
    */
    void timerCallback() override;
    void refreshChord();

    void showScaleMenu();
    void showOptionsMenu();

    struct Layout
    {
        float originX, step, radius, topY, bottomY, labelY, labelHeight, boxWidth;
    };

    Layout getLayout() const;
    void drawCircle (juce::Graphics&, float centreX, float centreY, float radius,
                     int pitchClass, const scaleview::Key&, juce::Colour highlight) const;

    ScaleViewProcessor& processor;

    std::pair<juce::uint64, juce::uint64> lastHeldMask { 0, 0 };
    std::array<bool, 12> heldClasses {};   // which circles to ring
    juce::String chordLabel;               // empty when nothing is held

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleViewEditor)
};
