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
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
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
    bool getSimpleNames()   const noexcept { return simpleNames; }

    /// The lit circles and their names for the current selection.
    scaleview::Key getKey() const;

    /*  Which MIDI notes are being held. The audio thread writes these as two
        64-bit masks and the editor reads them, so nothing is allocated or
        locked on the audio thread.
    */
    std::pair<juce::uint64, juce::uint64> getHeldMask() const noexcept
    {
        return { heldLow.load (std::memory_order_relaxed),
                 heldHigh.load (std::memory_order_relaxed) };
    }

    /// The held notes as numbers, lowest first. For the editor, not the audio thread.
    std::vector<int> getHeldNotes() const;

    void setScale (int newRootIndex, int newScaleIndex);
    void clearScale()                     { setScale (-1, -1); }
    void setRandomScale();
    void setHighlightIndex (int index);
    void setShowNoteNames (bool shouldShow);
    void setSimpleNames (bool shouldSimplify);

    /// The editor's last size, so reopening the window keeps it.
    juce::Point<int> getEditorSize() const noexcept { return { editorWidth, editorHeight }; }
    void setEditorSize (int width, int height) noexcept { editorWidth = width; editorHeight = height; }

private:
    /// Audio is passed through untouched, whichever precision the host uses.
    template <typename FloatType>
    void passThrough (juce::AudioBuffer<FloatType>& buffer);

    /*  Tracks which notes are down. Called on the audio thread, so it only
        touches the two atomics - no allocation, no locks.
    */
    void trackHeldNotes (const juce::MidiBuffer& midiMessages) noexcept;
    void setNoteHeld (int note, bool isHeld) noexcept;
    void clearHeldNotes() noexcept;

    std::atomic<juce::uint64> heldLow { 0 };    // notes 0-63
    std::atomic<juce::uint64> heldHigh { 0 };   // notes 64-127

    int rootIndex { -1 };
    int scaleIndex { -1 };
    int highlightIndex { 0 };
    bool showNoteNames { true };
    bool simpleNames { false };   // name notes as piano keys, not for the key

    int editorWidth { 400 };
    int editorHeight { 200 };

    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleViewProcessor)
};
