#include "PluginEditor.h"
#include "UI/Display.h"
#include "UI/Widgets.h"

using namespace cr3a;

// ============================================================================
//  Content: everything lives in a fixed 1200 x 720 design space that the editor
//  scales to the real window size.
// ============================================================================
class CR3AEditor::Content : public juce::Component, private juce::Timer
{
public:
    Content (CR3AEditor& ed, CR3AProcessor& p)
        : owner (ed), proc (p), apvts (p.apvts),
          formantP (apvts, "Formant", pid::formantOn),
          eqP (apvts, "Equalizer", pid::eqOn),
          compP (apvts, "Compressor", pid::compOn),
          satP (apvts, "Saturation", pid::satOn),
          ottP (apvts, "OTT", pid::ottOn),
          dblP (apvts, "Doubler", pid::dblOn),
          dlyP (apvts, "Delay", pid::dlyOn),
          revP (apvts, "Reverb", pid::revOn),
          widthP (apvts, "Width", pid::widthOn),
          display (p), eqCurve (apvts),
          swLimiter (apvts, pid::limiter, "Limiter"),
          swMono (apvts, pid::mono, "Mono"),
          swBypass (apvts, pid::bypass, "Bypass")
    {
        setOpaque (false);

        for (auto* panel : { &formantP, &eqP, &compP, &satP, &ottP, &dblP, &dlyP, &revP, &widthP })
            addAndMakeVisible (*panel);

        // ---- formant
        formantP.place (*mk (pid::formant, "Formant", true), 0, 0, 126, 200, 12);

        // ---- EQ
        const char* eqIds[]   = { pid::eqLowEnd, pid::eqLow, pid::eqMid, pid::eqMidHigh, pid::eqMidEnd, pid::eqAir, pid::eqHarsh };
        const char* eqNames[] = { "Low End", "Low", "Mid", "Mid High", "Mid End", "Air", "Harsh" };
        for (int i = 0; i < 7; ++i)
            eqP.place (*mk (eqIds[i], eqNames[i], i != 6), i % 4, i / 4, 100, 104);
        eqP.place (eqCurve, 3, 1, 100, 104, 12, 8);

        // ---- compressor
        compP.place (*mk (pid::compAmt, "Comp"), 0, 0, 126, 104, 12);
        compP.place (*mk (pid::compGain, "Gain", true), 0, 1, 126, 104, 12);

        // ---- saturation
        satP.place (*mk (pid::sat, "Sat"), 0, 0, 95, 104, 10);
        satP.place (*mk (pid::dist, "Dist"), 1, 0, 95, 104, 10);
        {
            auto* tone = mk (pid::distTone, "Tone");
            satP.place (*tone, 0, 1, 95, 104, 10);
            tone->setBounds (tone->getX() + 48, tone->getY(), 95, 104);
        }

        // ---- OTT
        ottP.place (*mk (pid::hyper, "Hyper"), 0, 0, 126, 104, 25);
        ottP.place (*mk (pid::ott, "OTT"), 0, 1, 126, 104, 25);

        // ---- doubler
        dblP.place (*mk (pid::dblType, "Type"), 0, 0, 106, 104);
        dblP.place (*mk (pid::dblMix, "Mix"), 1, 0, 106, 104);
        dblP.place (*mk (pid::dblWidth, "Width"), 2, 0, 106, 104);
        dblP.place (*mk (pid::dblDetune, "Detune"), 0, 1, 106, 104);
        dblP.place (*mk (pid::dblPan, "Pan Auto"), 1, 1, 106, 104);
        dblP.place (*mk (pid::dblTime, "Time"), 2, 1, 106, 104);

        // ---- delay
        dlyP.place (*mk (pid::dlyMix, "Mix"), 0, 0, 136, 104);
        dlyP.place (*mk (pid::dlyTime, "Time"), 1, 0, 136, 104);
        dlyP.place (*mk (pid::dlyFb, "Feedback"), 2, 0, 136, 104);
        dlyP.place (*mk (pid::dlyCut, "Cutoff"), 0, 1, 136, 104);
        dlyP.place (*mk (pid::dlyMode, "Mode"), 1, 1, 136, 104);
        swDotted = std::make_unique<SwitchCell> (apvts, pid::dlyDotted, "Dotted");
        dlyP.place (*swDotted, 2, 1, 136, 104);
        swDotted->setBounds (swDotted->getX() + 18, swDotted->getY() + 38, 112, 28);

        // ---- reverb
        revP.place (*mk (pid::revMix, "Mix"), 0, 0, 106, 104);
        revP.place (*mk (pid::revDecay, "Decay"), 1, 0, 106, 104);
        revP.place (*mk (pid::revPre, "Pre-Dly"), 0, 1, 106, 104);
        revP.place (*mk (pid::revTone, "Tone"), 1, 1, 106, 104);

        // ---- width
        widthP.place (*mk (pid::width, "Width", true), 0, 0, 96, 200, 12);

        // ---- bottom strip
        for (auto* c : { mk (pid::inGain, "Input", true), mk (pid::outGain, "Output"), mk (pid::mix, "Mix") })
            addAndMakeVisible (*c);
        addAndMakeVisible (display);
        for (auto* s : { &swLimiter, &swMono, &swBypass }) addAndMakeVisible (*s);

        // ---- top bar
        auto setup = [this] (juce::TextButton& b, const juce::String& txt)
        {
            b.setButtonText (txt);
            addAndMakeVisible (b);
        };
        setup (prevBtn, juce::String::charToString (0x25C0));
        setup (nextBtn, juce::String::charToString (0x25B6));
        setup (presetBtn, proc.getPresetName());
        prevBtn.onClick = [this] { stepPreset (-1); };
        nextBtn.onClick = [this] { stepPreset (+1); };
        presetBtn.onClick = [this] { showPresetMenu(); };

        const auto& list = themes();
        for (size_t i = 0; i < list.size(); ++i) themeBox.addItem (list[i].name, (int) i + 1);
        themeBox.setSelectedId (proc.getThemeIndex() + 1, juce::dontSendNotification);
        themeBox.onChange = [this] { owner.setTheme (themeBox.getSelectedId() - 1); };
        addAndMakeVisible (themeBox);

        startTimerHz (8);
    }

    void paint (juce::Graphics& g) override
    {
        auto& lf = lfOf (*this);
        const auto& th = lf.t();
        lf.paintBackground (g, getWidth(), getHeight());

        if (th.look == Look::XP)                       // a Luna "taskbar" behind the bottom strip
        {
            auto dock = juce::Rectangle<float> (8, 585, 1184, 127);
            g.setColour (juce::Colour (0xff0b3fc9)); g.fillRoundedRectangle (dock, 6.0f);
            g.setColour (lf.col (th.panelBottom));    g.fillRoundedRectangle (dock.reduced (3.0f), 4.0f);
        }

        // logo + wordmark
        const auto ink = lf.col (th.look == Look::Terminal ? th.accent : th.text);
        lf.paintLogo (g, juce::Rectangle<float> (20, 10, 66, 44), ink);
        g.setColour (ink);
        g.setFont (lf.font (32.0f, true).withExtraKerningFactor (0.22f));
        g.drawText ("CR3A", juce::Rectangle<int> (102, 8, 200, 34), juce::Justification::centredLeft);
        g.setFont (lf.font (10.5f, true).withExtraKerningFactor (0.3f));
        g.setColour (lf.col (th.textDim));
        g.drawText ("VOCAL CHAIN", juce::Rectangle<int> (104, 38, 200, 16), juce::Justification::centredLeft);

        g.setFont (lf.font (10.0f));
        g.setColour (lf.col (th.textDim).withAlpha (0.8f));
        g.drawText ("THEME", juce::Rectangle<int> (getWidth() - 16 - 190, 6, 190, 14), juce::Justification::centredLeft);
    }

    void resized() override
    {
        formantP.setBounds (16, 76, 150, 246);
        eqP.setBounds (176, 76, 438, 246);
        compP.setBounds (624, 76, 150, 246);
        satP.setBounds (784, 76, 214, 246);
        ottP.setBounds (1008, 76, 176, 246);

        dblP.setBounds (16, 334, 342, 246);
        dlyP.setBounds (368, 334, 440, 246);
        revP.setBounds (818, 334, 236, 246);
        widthP.setBounds (1064, 334, 120, 246);

        cells[inIdx()]->setBounds (16, 596, 86, 108);
        display.setBounds (112, 592, 752, 112);
        cells[outIdx()]->setBounds (874, 596, 86, 108);
        cells[mixIdx()]->setBounds (970, 596, 86, 108);
        swLimiter.setBounds (1066, 600, 118, 28);
        swMono.setBounds (1066, 634, 118, 28);
        swBypass.setBounds (1066, 668, 118, 28);

        prevBtn.setBounds (400, 16, 32, 30);
        presetBtn.setBounds (438, 16, 300, 30);
        nextBtn.setBounds (744, 16, 32, 30);
        themeBox.setBounds (getWidth() - 16 - 190, 22, 190, 28);
    }

private:
    // creates a knob cell, wires the display read-out and keeps it alive
    KnobCell* mk (const char* id, const char* caption, bool bipolar = false)
    {
        auto c = std::make_unique<KnobCell> (apvts, id, caption, bipolar);
        c->onReadout = [this] (const juce::String& n, const juce::String& v) { display.setReadout (n, v); };
        cells.push_back (std::move (c));
        return cells.back().get();
    }
    size_t inIdx() const  { return cells.size() - 3; }
    size_t outIdx() const { return cells.size() - 2; }
    size_t mixIdx() const { return cells.size() - 1; }

    void timerCallback() override
    {
        const auto n = proc.getPresetName();
        if (presetBtn.getButtonText() != n) presetBtn.setButtonText (n);
    }

    void stepPreset (int dir)
    {
        const int total = proc.getNumPrograms();
        proc.setCurrentProgram ((proc.getCurrentProgram() + dir + total) % total);
        presetBtn.setButtonText (proc.getPresetName());
    }

    void showPresetMenu()
    {
        juce::PopupMenu m;
        m.setLookAndFeel (&getLookAndFeel());
        const auto& list = factoryPresets();
        m.addSectionHeader ("Factory");
        for (size_t i = 0; i < list.size(); ++i)
            m.addItem ((int) i + 1, list[i].name, true, proc.getPresetName() == list[i].name);

        juce::Array<juce::File> user;
        CR3AProcessor::getUserPresetDirectory().findChildFiles (user, juce::File::findFiles, false, "*.cr3a");
        if (user.size() > 0)
        {
            m.addSeparator();
            m.addSectionHeader ("User");
            for (int i = 0; i < user.size(); ++i)
                m.addItem (1000 + i, user[i].getFileNameWithoutExtension());
        }
        m.addSeparator();
        m.addItem (900, "Save preset...");
        m.addItem (901, "Load preset from file...");

        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn).withMinimumWidth (presetBtn.getWidth()),
                         [this, user] (int r)
        {
            if (r <= 0) return;
            if (r < 900) { proc.setCurrentProgram (r - 1); }
            else if (r >= 1000) { proc.loadUserPreset (user[r - 1000]); }
            else if (r == 900) savePreset();
            else if (r == 901) loadPreset();
            presetBtn.setButtonText (proc.getPresetName());
        });
    }

    void savePreset()
    {
        auto dir = CR3AProcessor::getUserPresetDirectory();
        dir.createDirectory();
        chooser = std::make_shared<juce::FileChooser> ("Save CR3A preset", dir.getChildFile (proc.getPresetName() + ".cr3a"), "*.cr3a");
        auto keep = chooser;
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this, keep] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f != juce::File()) proc.saveUserPreset (f.withFileExtension ("cr3a"));
        });
    }

    void loadPreset()
    {
        auto dir = CR3AProcessor::getUserPresetDirectory();
        chooser = std::make_shared<juce::FileChooser> ("Load CR3A preset", dir, "*.cr3a");
        auto keep = chooser;
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this, keep] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f.existsAsFile()) proc.loadUserPreset (f);
        });
    }

    CR3AEditor& owner;
    CR3AProcessor& proc;
    juce::AudioProcessorValueTreeState& apvts;

    ModulePanel formantP, eqP, compP, satP, ottP, dblP, dlyP, revP, widthP;
    DisplayComponent display;
    EqCurve eqCurve;
    SwitchCell swLimiter, swMono, swBypass;
    std::unique_ptr<SwitchCell> swDotted;
    std::vector<std::unique_ptr<KnobCell>> cells;
    juce::TextButton prevBtn, nextBtn, presetBtn;
    juce::ComboBox themeBox;
    std::shared_ptr<juce::FileChooser> chooser;
};

// ============================================================================
CR3AEditor::CR3AEditor (CR3AProcessor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    lf.setTheme (proc.getThemeIndex());
    setLookAndFeel (&lf);

    content = std::make_unique<Content> (*this, proc);
    addAndMakeVisible (*content);

    constrainer.setFixedAspectRatio ((double) baseW / (double) baseH);
    constrainer.setSizeLimits (600, 360, 2400, 1440);
    setConstrainer (&constrainer);
    setResizable (true, true);
    setSize (baseW, baseH);
}

CR3AEditor::~CR3AEditor()
{
    setLookAndFeel (nullptr);
}

void CR3AEditor::resized()
{
    const float s = (float) getWidth() / (float) baseW;
    content->setTransform (juce::AffineTransform::scale (s));
    content->setBounds (0, 0, baseW, baseH);
}

void CR3AEditor::setTheme (int index)
{
    lf.setTheme (index);
    proc.setThemeIndex (index);
    sendLookAndFeelChange();
    repaint();
}

juce::Component& CR3AEditor::getContent() { return *content; }
