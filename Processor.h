#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

struct SurcoTelemetry
{
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> inPeakL { 0.0f };
    std::atomic<float> inPeakR { 0.0f };
    std::atomic<float> outPeakL { 0.0f };
    std::atomic<float> outPeakR { 0.0f };
    std::atomic<float> inRmsL { 0.0f };
    std::atomic<float> inRmsR { 0.0f };
    std::atomic<float> outRmsL { 0.0f };
    std::atomic<float> outRmsR { 0.0f };
    std::atomic<float> compReductionDb { 0.0f };
    std::atomic<float> gateAttenuationDb { 0.0f };
    std::atomic<float> deEssReductionDb { 0.0f };
    std::atomic<float> deEssDetectorDb { -96.0f };
    std::atomic<bool> inputOverload { false };
    std::atomic<bool> eqOverload { false };
};

SurcoTelemetry& getSurcoTelemetry (juce::AudioProcessor& proc);
void renderSurcoEditorSnapshot (const juce::File& outPng);
juce::AudioProcessorEditor* makeEditor (juce::AudioProcessor&, juce::AudioProcessorValueTreeState&);
