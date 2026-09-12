#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
// Settings are stored by name rather than by index, so reordering a table
// cannot repoint a saved choice at a different entry.
const juce::Identifier stateTag { "ScaleViewState" };

int indexOfNamed (const juce::String& name, const std::vector<const char*>& names)
{
    for (size_t i = 0; i < names.size(); ++i)
        if (name == names[i])
            return static_cast<int> (i);
    return -1;
}

std::vector<const char*> rootNames()
{
    std::vector<const char*> names;
    for (const auto& root : scaleview::roots) names.push_back (root.name);
    return names;
}

std::vector<const char*> scaleNames()
{
    std::vector<const char*> names;
    for (const auto& scale : scaleview::scales) names.push_back (scale.name);
    return names;
}

std::vector<const char*> highlightNames()
{
    std::vector<const char*> names;
    for (const auto& highlight : scaleview::highlights) names.push_back (highlight.name);
    return names;
}
} // namespace

//==============================================================================
ScaleViewProcessor::ScaleViewProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input",   juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      random (juce::Random::getSystemRandom().nextInt64())
{
}

void ScaleViewProcessor::prepareToPlay (double, int) {}

bool ScaleViewProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Audio is passed through untouched, so accept whatever the track is, as
    // long as the layout in matches the layout out.
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        && ! layouts.getMainOutputChannelSet().isDisabled();
}

template <typename FloatType>
void ScaleViewProcessor::passThrough (juce::AudioBuffer<FloatType>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    // Nothing to do: this plugin only draws. Any output channels beyond the
    // input still have to be cleared, or they would carry whatever was left
    // in the buffer.
    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
}

void ScaleViewProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    passThrough (buffer);
}

void ScaleViewProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    passThrough (buffer);
}

//==============================================================================
juce::AudioProcessorEditor* ScaleViewProcessor::createEditor()
{
    return new ScaleViewEditor (*this);
}

scaleview::Key ScaleViewProcessor::getKey() const
{
    return scaleview::buildKey (rootIndex, scaleIndex);
}

void ScaleViewProcessor::setScale (int newRootIndex, int newScaleIndex)
{
    rootIndex  = newRootIndex;
    scaleIndex = newScaleIndex;
    sendChangeMessage();
}

void ScaleViewProcessor::setRandomScale()
{
    // Never the scale already showing: with so many combinations a repeat is
    // rare, but when it happened the option would look broken.
    int newRoot = 0, newScale = 0;

    do
    {
        newRoot  = random.nextInt (static_cast<int> (scaleview::roots.size()));
        newScale = random.nextInt (static_cast<int> (scaleview::scales.size()));
    }
    while (newRoot == rootIndex && newScale == scaleIndex);

    setScale (newRoot, newScale);
}

void ScaleViewProcessor::setHighlightIndex (int index)
{
    highlightIndex = juce::jlimit (0, static_cast<int> (scaleview::highlights.size()) - 1, index);
    sendChangeMessage();
}

void ScaleViewProcessor::setShowNoteNames (bool shouldShow)
{
    showNoteNames = shouldShow;
    sendChangeMessage();
}

//==============================================================================
void ScaleViewProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state (stateTag);

    state.setProperty ("root",  rootIndex  >= 0 ? juce::String (scaleview::roots[(size_t) rootIndex].name)   : juce::String(), nullptr);
    state.setProperty ("scale", scaleIndex >= 0 ? juce::String (scaleview::scales[(size_t) scaleIndex].name) : juce::String(), nullptr);
    state.setProperty ("highlight", juce::String (scaleview::highlights[(size_t) highlightIndex].name), nullptr);
    state.setProperty ("showNoteNames", showNoteNames, nullptr);
    state.setProperty ("editorWidth", editorWidth, nullptr);
    state.setProperty ("editorHeight", editorHeight, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ScaleViewProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;

    const auto state = juce::ValueTree::fromXml (*xml);
    if (! state.hasType (stateTag)) return;

    rootIndex      = indexOfNamed (state.getProperty ("root").toString(),  rootNames());
    scaleIndex     = indexOfNamed (state.getProperty ("scale").toString(), scaleNames());
    highlightIndex = juce::jmax (0, indexOfNamed (state.getProperty ("highlight").toString(), highlightNames()));
    showNoteNames  = state.getProperty ("showNoteNames", true);

    editorWidth  = state.getProperty ("editorWidth", 400);
    editorHeight = state.getProperty ("editorHeight", 200);

    // A root without a scale, or the other way round, would light nothing.
    if (rootIndex < 0 || scaleIndex < 0)
        rootIndex = scaleIndex = -1;

    sendChangeMessage();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ScaleViewProcessor();
}
