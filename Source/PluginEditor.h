#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

/*  The icon: twelve circles, five on the top row for the black keys of an
    octave and seven on the bottom for the white keys, with the notes of the
    selected scale lit. Left-click for the scale list, right-click for the
    options.
*/
class ScaleViewEditor final : public juce::AudioProcessorEditor,
                              private juce::ChangeListener
{
public:
    explicit ScaleViewEditor (ScaleViewProcessor&);
    ~ScaleViewEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleViewEditor)
};
