#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include "Processor.h"
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI runtime;
    if (argc == 2 && juce::String(argv[1]) == "--snap")
    {
        auto dir = juce::File::getCurrentWorkingDirectory().getChildFile("docs");
        dir.createDirectory();
        auto outPng = dir.getChildFile("ChannelStrip_FL_Heat_UI.png");
        renderSurcoEditorSnapshot(outPng);
        std::cout << "Saved UI snapshot: " << outPng.getFullPathName() << "\n";
        return outPng.existsAsFile() ? 0 : 1;
    }
    if (argc != 5) { std::cerr << "CheckKey bundle keyParameter thresholdParameter ratioParameter\n"; return 1; }
    juce::AudioPluginFormatManager manager;
    auto format = std::make_unique<juce::VST3PluginFormatHeadless>();
    juce::OwnedArray<juce::PluginDescription> types;
    format->findAllTypesForFile(types, juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]).getFullPathName());
    if (types.size() != 1) return 2;
    manager.addFormat(std::move(format));
    juce::String error;
    auto p = manager.createPluginInstance(*types[0], 48000, 128, error);
    if (!p) { std::cerr << error << '\n'; return 3; }
    auto layout = p->getBusesLayout();
    if (layout.inputBuses.size() < 2) return 4;
    layout.inputBuses.set(1, juce::AudioChannelSet::stereo());
    if (!p->setBusesLayout(layout)) return 5;
    auto set = [&](const char* name, float value) {
        for (auto* param : p->getParameters())
            if (param->getName(128).equalsIgnoreCase(name)) { param->setValueNotifyingHost(value); return; }
        throw std::runtime_error(name);
    };
    set(argv[2], 1); set(argv[3], 0); set(argv[4], 1);
    p->prepareToPlay(48000, 128);
    auto measure = [&](float key) {
        juce::AudioBuffer<float> block(4, 128);
        juce::MidiBuffer midi;
        for (int b = 0; b < 1000; ++b) {
            for (int i = 0; i < 128; ++i) {
                const float wave = std::sin((b * 128 + i) * juce::MathConstants<float>::twoPi / 48);
                for (int c = 0; c < 4; ++c) block.setSample(c, i, (c < 2 ? 0.04f : key) * wave);
            }
            p->processBlock(block, midi);
        }
        return block.getRMSLevel(0, 0, 128);
    };
    const auto quiet = measure(0), loud = measure(0.9f);
    p->releaseResources();
    std::cout << types[0]->manufacturerName << " | RMS key silent: " << quiet << " | key loud: " << loud << '\n';
    return quiet > 0.02f && loud < quiet * 0.5f ? 0 : 6;
}
