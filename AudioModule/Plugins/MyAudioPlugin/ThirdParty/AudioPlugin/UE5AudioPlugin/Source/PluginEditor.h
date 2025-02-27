/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class UE5AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    UE5AudioPluginAudioProcessorEditor (UE5AudioPluginAudioProcessor&);
    ~UE5AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    UE5AudioPluginAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UE5AudioPluginAudioProcessorEditor)
};
