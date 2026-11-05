// Dev tool: renders the editor in every theme to PNG files (run under xvfb-run).
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

static void feed (CR3AProcessor& p, int blocks)
{
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    static double phase = 0.0;
    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < 512; ++i)
        {
            double s = 0.0;
            for (int h = 1; h < 40; ++h)
            {
                const double f = 150.0 * h;
                const double env = std::exp (-std::pow (std::log (f / 800.0), 2.0) * 1.2) + 0.5 * std::exp (-std::pow (std::log (f / 2600.0), 2.0) * 2.0);
                s += env * std::sin (phase * h) / h * 1.6;
            }
            phase += 2.0 * juce::MathConstants<double>::pi * 150.0 / 48000.0;
            buf.setSample (0, i, (float) (0.28 * s));
            buf.setSample (1, i, (float) (0.28 * s));
        }
        p.processBlock (buf, midi);
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String outDir = argc > 1 ? argv[1] : ".";
    const int onlyTheme = argc > 2 ? juce::String (argv[2]).getIntValue() : -1;
    const float scale = argc > 3 ? juce::String (argv[3]).getFloatValue() : 1.0f;

    CR3AProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    proc.setCurrentProgram (2);                   // Dark Vox
    feed (proc, 200);

    for (int t = 0; t < (int) cr3a::themes().size(); ++t)
    {
        if (onlyTheme >= 0 && t != onlyTheme) continue;
        proc.setThemeIndex (t);
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        ed->setSize ((int) (1200 * scale), (int) (720 * scale));
        for (int k = 0; k < 6; ++k)
        {
            feed (proc, 20);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
        }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f (outDir + "/theme_" + juce::String (t) + ".png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("wrote %s\n", f.getFullPathName().toRawUTF8());
    }
    return 0;
}
