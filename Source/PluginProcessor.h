/*
    ScaleView - a key signature / scale reference as a VST3 and Audio Unit
    plugin. It does not touch the audio; it exists to be looked at.

    The processor holds the selection so that it is saved with the host
    project and can be stored in presets, and tells the editor when it
    changes.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "ScaleModel.h"

class ScaleViewProcessor final : public juce::AudioProcessor,
                                 public juce::ChangeBroadcaster
{
public:
    ScaleViewProcessor();
    ~ScaleViewProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    /*  The selection. rootIndex and scaleIndex are -1 when no scale is
        chosen, which is the state the icon starts in: every circle the same
        colour.
    */
    int getRootIndex() const noexcept      { return rootIndex; }
    int getScaleIndex() const noexcept     { return scaleIndex; }
    int getHighlightIndex() const noexcept { return highlightIndex; }
    bool getShowNoteNames() const noexcept { return showNoteNames; }

    /// The lit circles and their names for the current selection.
    scaleview::Key getKey() const;

    void setScale (int newRootIndex, int newScaleIndex);
    void clearScale()                     { setScale (-1, -1); }
    void setRandomScale();
    void setHighlightIndex (int index);
    void setShowNoteNames (bool shouldShow);

    /// The editor's last size, so reopening the window keeps it.
    juce::Point<int> getEditorSize() const noexcept { return { editorWidth, editorHeight }; }
    void setEditorSize (int width, int height) noexcept { editorWidth = width; editorHeight = height; }

private:
    int rootIndex { -1 };
    int scaleIndex { -1 };
    int highlightIndex { 0 };
    bool showNoteNames { true };

    int editorWidth { 400 };
    int editorHeight { 200 };

    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleViewProcessor)
};
