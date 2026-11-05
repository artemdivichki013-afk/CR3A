#include "CR3ALookAndFeel.h"
#include <BinaryData.h>

using namespace juce;

namespace cr3a
{
static constexpr float kStart = MathConstants<float>::pi * 1.25f;
static constexpr float kEnd   = MathConstants<float>::pi * 2.75f;

CR3ALookAndFeel::CR3ALookAndFeel()
{
    availableFonts = Font::findAllTypefaceNames();

    auto src = ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);
    if (src.isValid())
    {
        logoMask = Image (Image::SingleChannel, src.getWidth(), src.getHeight(), true);
        for (int y = 0; y < src.getHeight(); ++y)
            for (int x = 0; x < src.getWidth(); ++x)
                logoMask.setPixelAt (x, y, Colours::white.withAlpha (src.getPixelAt (x, y).getAlpha()));
    }
    setTheme (0);
}

void CR3ALookAndFeel::setTheme (int index)
{
    themeIdx = jlimit (0, (int) themes().size() - 1, index);
    applyColours();
}

void CR3ALookAndFeel::applyColours()
{
    const auto& th = t();
    setColour (ComboBox::backgroundColourId, col (th.panelBottom));
    setColour (ComboBox::textColourId, col (th.text));
    setColour (ComboBox::outlineColourId, col (th.panelEdge));
    setColour (ComboBox::arrowColourId, col (th.textDim));
    setColour (PopupMenu::backgroundColourId, th.look == Look::Glass ? Colour (0xf2f3f4fa) : th.look == Look::Aero ? Colour (0xf2eaf8ff) : col (th.panelTop).withAlpha (1.0f));
    setColour (PopupMenu::textColourId, col (th.text));
    setColour (PopupMenu::headerTextColourId, col (th.textDim));
    setColour (PopupMenu::highlightedBackgroundColourId, col (th.accent));
    setColour (PopupMenu::highlightedTextColourId, th.look == Look::Paper ? Colours::white : (th.look == Look::Graphite ? Colour (0xff1b1c1f) : Colours::white));
    setColour (Label::textColourId, col (th.text));
    setColour (TextEditor::backgroundColourId, col (th.displayBg));
    setColour (TextEditor::textColourId, col (th.displayText));
    setColour (TextEditor::outlineColourId, col (th.panelEdge));
    setColour (TextEditor::highlightColourId, col (th.accent).withAlpha (0.5f));
    setColour (Slider::textBoxTextColourId, col (th.displayText));
}

Font CR3ALookAndFeel::font (float h, bool bold) const
{
    const auto& th = t();
    StringArray names = StringArray::fromTokens (th.fonts, ";", "");
    String use = Font::getDefaultSansSerifFontName();
    for (auto& n : names)
        if (availableFonts.contains (n)) { use = n; break; }
    return Font (FontOptions (use, h, bold ? Font::bold : Font::plain));
}

// ============================================================================
//  helpers
// ============================================================================
void CR3ALookAndFeel::bevel (Graphics& g, Rectangle<float> r, bool raised)
{
    const auto hi = raised ? Colours::white : Colour (0xff404040);
    const auto lo = raised ? Colour (0xff404040) : Colours::white;
    const auto hi2 = raised ? Colour (0xffdfdfdf) : Colour (0xff808080);
    const auto lo2 = raised ? Colour (0xff808080) : Colour (0xffdfdfdf);
    auto b = r.reduced (0.5f);
    g.setColour (lo);  g.drawLine (b.getX(), b.getBottom(), b.getRight(), b.getBottom()); g.drawLine (b.getRight(), b.getY(), b.getRight(), b.getBottom());
    g.setColour (hi);  g.drawLine (b.getX(), b.getY(), b.getRight(), b.getY());           g.drawLine (b.getX(), b.getY(), b.getX(), b.getBottom());
    auto c = b.reduced (1.0f);
    g.setColour (lo2); g.drawLine (c.getX(), c.getBottom(), c.getRight(), c.getBottom()); g.drawLine (c.getRight(), c.getY(), c.getRight(), c.getBottom());
    g.setColour (hi2); g.drawLine (c.getX(), c.getY(), c.getRight(), c.getY());           g.drawLine (c.getX(), c.getY(), c.getX(), c.getBottom());
}

void CR3ALookAndFeel::rimGloss (Graphics& g, Rectangle<float> r, float radius, float alpha)
{
    auto top = r.reduced (1.5f).removeFromTop (r.getHeight() * 0.5f);
    ColourGradient gr (Colours::white.withAlpha (alpha), top.getX(), top.getY(),
                       Colours::white.withAlpha (alpha * 0.15f), top.getX(), top.getBottom(), false);
    g.setGradientFill (gr);
    g.fillRoundedRectangle (top, radius);
}

// ============================================================================
//  background / panels
// ============================================================================
void CR3ALookAndFeel::paintBackground (Graphics& g, int w, int h)
{
    const auto& th = t();
    const auto b = Rectangle<float> (0, 0, (float) w, (float) h);
    g.setGradientFill (ColourGradient (col (th.bgTop), 0, 0, col (th.bgBottom), 0, (float) h, false));
    g.fillRect (b);

    switch (th.look)
    {
        case Look::Graphite:
        {
            g.setColour (Colours::white.withAlpha (0.018f));
            for (int y = 0; y < h; y += 4) g.fillRect (0, y, w, 1);
            break;
        }
        case Look::Aero:
        {
            auto blob = [&] (float cx, float cy, float rad, Colour c)
            {
                g.setGradientFill (ColourGradient (c, cx, cy, c.withAlpha (0.0f), cx + rad, cy, true));
                g.fillEllipse (cx - rad, cy - rad, rad * 2, rad * 2);
            };
            blob ((float) w * 0.15f, (float) h * 0.12f, 360.0f, Colours::white.withAlpha (0.55f));
            blob ((float) w * 0.85f, (float) h * 0.30f, 320.0f, Colour (0xff9be86a).withAlpha (0.35f));
            blob ((float) w * 0.55f, (float) h * 1.00f, 480.0f, Colour (0xff4ac0ff).withAlpha (0.40f));
            // bubbles
            Random rnd (7);
            for (int i = 0; i < 16; ++i)
            {
                const float rr = 14.0f + rnd.nextFloat() * 46.0f, x = rnd.nextFloat() * (float) w, y = rnd.nextFloat() * (float) h;
                g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.05f), x, y, Colours::white.withAlpha (0.22f), x + rr, y + rr, true));
                g.fillEllipse (x - rr, y - rr, rr * 2, rr * 2);
                g.setColour (Colours::white.withAlpha (0.35f));
                g.drawEllipse (x - rr, y - rr, rr * 2, rr * 2, 1.0f);
            }
            break;
        }
        case Look::XP:
        {
            // rolling hills of "Bliss" behind the window frame
            Path hill; hill.startNewSubPath (0, (float) h * 0.78f);
            hill.cubicTo ((float) w * 0.3f, (float) h * 0.62f, (float) w * 0.65f, (float) h * 0.95f, (float) w, (float) h * 0.72f);
            hill.lineTo ((float) w, (float) h); hill.lineTo (0, (float) h); hill.closeSubPath();
            g.setGradientFill (ColourGradient (Colour (0xff4fa334), 0, (float) h * 0.7f, Colour (0xff2c7a1e), 0, (float) h, false));
            g.fillPath (hill);
            break;
        }
        case Look::Glass:
        {
            auto blob = [&] (float cx, float cy, float rad, Colour c)
            {
                g.setGradientFill (ColourGradient (c, cx, cy, c.withAlpha (0.0f), cx + rad, cy, true));
                g.fillEllipse (cx - rad, cy - rad, rad * 2, rad * 2);
            };
            blob ((float) w * 0.12f, (float) h * 0.20f, 420.0f, Colour (0xff4c8dff).withAlpha (0.65f));
            blob ((float) w * 0.80f, (float) h * 0.15f, 380.0f, Colour (0xffff7ad9).withAlpha (0.55f));
            blob ((float) w * 0.62f, (float) h * 0.95f, 460.0f, Colour (0xff8a6bff).withAlpha (0.55f));
            blob ((float) w * 0.25f, (float) h * 0.95f, 360.0f, Colour (0xff3fe0d0).withAlpha (0.45f));
            break;
        }
        case Look::Win98:
            break;
        case Look::Terminal:
        {
            g.setColour (Colour (0xff3ae872).withAlpha (0.035f));
            for (int y = 0; y < h; y += 3) g.fillRect (0, y, w, 1);
            g.setGradientFill (ColourGradient (Colours::transparentBlack, (float) w * 0.5f, (float) h * 0.5f,
                                               Colours::black.withAlpha (0.45f), 0, 0, true));
            g.fillRect (b);
            break;
        }
        case Look::Paper:
        {
            Random rnd (3);
            g.setColour (Colours::black.withAlpha (0.018f));
            for (int i = 0; i < 1400; ++i) g.fillRect (rnd.nextFloat() * (float) w, rnd.nextFloat() * (float) h, 1.0f, 1.0f);
            break;
        }
    }
}

void CR3ALookAndFeel::paintPanel (Graphics& g, Rectangle<float> r, const String& title, bool on, bool hasPower)
{
    const auto& th = t();
    const float rad = th.radius;
    const float hh = headerH;
    auto titleArea = r.withHeight (hh);
    const float tx = hasPower ? 34.0f : 14.0f;
    const String up = title.toUpperCase();

    switch (th.look)
    {
        case Look::Graphite:
        {
            g.setColour (Colours::black.withAlpha (0.35f));
            g.fillRoundedRectangle (r.translated (0, 2.0f), rad);
            g.setGradientFill (ColourGradient (col (th.panelTop), 0, r.getY(), col (th.panelBottom), 0, r.getBottom(), false));
            g.fillRoundedRectangle (r, rad);
            g.setColour (Colours::white.withAlpha (0.045f));
            g.drawRoundedRectangle (r.reduced (1.0f), rad - 1, 1.0f);
            g.setColour (col (th.panelEdge));
            g.drawRoundedRectangle (r, rad, 1.0f);
            g.setColour (Colours::black.withAlpha (0.35f));
            g.fillRect (titleArea.getX() + 10, titleArea.getBottom() - 1.0f, titleArea.getWidth() - 20, 1.0f);
            g.setColour (Colours::white.withAlpha (0.04f));
            g.fillRect (titleArea.getX() + 10, titleArea.getBottom(), titleArea.getWidth() - 20, 1.0f);
            g.setColour (col (on ? th.text : th.textDim));
            g.setFont (font (11.5f, true).withExtraKerningFactor (0.14f));
            g.drawText (up, titleArea.withTrimmedLeft (tx), Justification::centredLeft);
            break;
        }
        case Look::Aero:
        {
            g.setColour (Colour (0xff0c3d5e).withAlpha (0.20f));
            g.fillRoundedRectangle (r.translated (0, 3.0f), rad);
            g.setGradientFill (ColourGradient (col (th.panelTop), 0, r.getY(), col (th.panelBottom), 0, r.getBottom(), false));
            g.fillRoundedRectangle (r, rad);
            rimGloss (g, r, rad, 0.35f);
            g.setColour (col (th.panelEdge));
            g.drawRoundedRectangle (r, rad, 1.5f);
            g.setColour (Colour (0xff1d9ee8).withAlpha (0.35f));
            g.drawRoundedRectangle (r.expanded (0.5f), rad, 1.0f);
            const auto ta = titleArea.withTrimmedLeft (tx);
            g.setFont (font (13.0f, true));
            g.setColour (Colours::white.withAlpha (0.9f)); g.drawText (title, ta.translated (0, 1), Justification::centredLeft);
            g.setColour (col (on ? th.text : th.textDim)); g.drawText (title, ta, Justification::centredLeft);
            break;
        }
        case Look::XP:
        {
            g.setColour (Colours::black.withAlpha (0.25f));
            g.fillRoundedRectangle (r.translated (2, 3), rad);
            // frame
            g.setColour (Colour (0xff0b3fc9));
            g.fillRoundedRectangle (r, rad);
            auto body = r.withTrimmedTop (hh).reduced (3.0f, 0).withTrimmedBottom (3.0f);
            g.setColour (col (th.panelBottom));
            g.fillRect (body);
            // title bar
            auto tb = r.withHeight (hh).reduced (0, 0);
            ColourGradient tg (Colour (0xff3b8cff), 0, tb.getY(), Colour (0xff0a53e4), 0, tb.getBottom(), false);
            tg.addColour (0.12, Colour (0xff1f6ef2)); tg.addColour (0.85, Colour (0xff0a50e0));
            g.setGradientFill (tg);
            Path p; p.addRoundedRectangle (tb.getX(), tb.getY(), tb.getWidth(), tb.getHeight(), rad, rad, true, true, false, false);
            g.fillPath (p);
            g.setColour (Colours::white.withAlpha (0.35f));
            g.drawLine (tb.getX() + rad, tb.getY() + 1.5f, tb.getRight() - rad, tb.getY() + 1.5f);
            g.setFont (font (13.0f, true));
            g.setColour (Colours::black.withAlpha (0.45f)); g.drawText (title, titleArea.withTrimmedLeft (tx).translated (1, 1), Justification::centredLeft);
            g.setColour (Colours::white.withAlpha (on ? 1.0f : 0.65f)); g.drawText (title, titleArea.withTrimmedLeft (tx), Justification::centredLeft);
            break;
        }
        case Look::Glass:
        {
            g.setColour (Colour (0xff3a3f78).withAlpha (0.16f));
            g.fillRoundedRectangle (r.translated (0, 6.0f).reduced (4, 0), rad + 2);
            g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.46f), r.getX(), r.getY(), Colours::white.withAlpha (0.16f), r.getRight(), r.getBottom(), false));
            g.fillRoundedRectangle (r, rad);
            g.setColour (Colours::white.withAlpha (0.65f));
            g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
            g.setColour (Colours::white.withAlpha (0.35f));
            g.drawRoundedRectangle (r.reduced (2.5f), rad - 2, 1.0f);
            // specular streak
            Path sp; sp.addRoundedRectangle (r.getX() + 8, r.getY() + 1.5f, r.getWidth() * 0.55f, 3.0f, 1.5f);
            g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.85f), r.getX() + 8, 0, Colours::white.withAlpha (0.0f), r.getX() + r.getWidth() * 0.6f, 0, false));
            g.fillPath (sp);
            g.setColour (col (on ? th.text : th.textDim));
            g.setFont (font (13.0f, true));
            g.drawText (title, titleArea.withTrimmedLeft (tx), Justification::centredLeft);
            break;
        }
        case Look::Win98:
        {
            g.setColour (col (th.panelBottom));
            g.fillRect (r);
            bevel (g, r, true);
            auto tb = r.reduced (3.0f).withHeight (hh - 7.0f);
            g.setGradientFill (ColourGradient (Colour (0xff000080), tb.getX(), 0, Colour (0xff1084d0), tb.getRight(), 0, false));
            g.fillRect (tb);
            g.setFont (font (12.5f, true));
            g.setColour (Colours::white.withAlpha (on ? 1.0f : 0.6f));
            g.drawText (title, tb.withTrimmedLeft (tx - 3.0f), Justification::centredLeft);
            break;
        }
        case Look::Terminal:
        {
            g.setColour (col (th.panelBottom));
            g.fillRoundedRectangle (r, rad);
            g.setGradientFill (ColourGradient (col (th.panelTop), 0, r.getY(), col (th.panelBottom), 0, r.getBottom(), false));
            g.fillRoundedRectangle (r.reduced (1.0f), rad);
            g.setColour (col (th.panelEdge));
            g.drawRoundedRectangle (r, rad, 1.0f);
            g.setColour (col (th.accent));
            const float L = 7.0f;                                    // corner pins like the original skin
            for (auto pt : { r.getTopLeft(), r.getTopRight(), r.getBottomLeft(), r.getBottomRight() })
            {
                const float sx = pt.x < r.getCentreX() ? 1.0f : -1.0f, sy = pt.y < r.getCentreY() ? 1.0f : -1.0f;
                g.drawLine (pt.x, pt.y + sy * 0.5f, pt.x + sx * L, pt.y + sy * 0.5f, 1.6f);
                g.drawLine (pt.x + sx * 0.5f, pt.y, pt.x + sx * 0.5f, pt.y + sy * L, 1.6f);
            }
            g.setColour (col (on ? th.accent : th.textDim));
            g.setFont (font (11.5f, true).withExtraKerningFactor (0.1f));
            g.drawText ("> " + up, titleArea.withTrimmedLeft (tx - 6.0f), Justification::centredLeft);
            break;
        }
        case Look::Paper:
        {
            g.setColour (Colours::black.withAlpha (0.07f));
            g.fillRoundedRectangle (r.translated (0, 2.0f), rad);
            g.setGradientFill (ColourGradient (col (th.panelTop), 0, r.getY(), col (th.panelBottom), 0, r.getBottom(), false));
            g.fillRoundedRectangle (r, rad);
            g.setColour (col (th.panelEdge));
            g.drawRoundedRectangle (r, rad, 1.0f);
            g.setColour (col (th.panelEdge));
            g.fillRect (titleArea.getX() + 12, titleArea.getBottom() - 1.0f, titleArea.getWidth() - 24, 1.0f);
            g.setColour (col (on ? th.text : th.textDim));
            g.setFont (font (11.5f, true).withExtraKerningFactor (0.16f));
            g.drawText (up, titleArea.withTrimmedLeft (tx), Justification::centredLeft);
            break;
        }
    }
}

void CR3ALookAndFeel::paintDisplayBezel (Graphics& g, Rectangle<float> r)
{
    const auto& th = t();
    const float rad = th.look == Look::Win98 ? 0.0f : jmax (3.0f, th.radius * 0.7f);
    if (th.look == Look::Win98) { g.setColour (col (th.panelBottom)); g.fillRect (r); bevel (g, r, false); g.setColour (col (th.displayBg)); g.fillRect (r.reduced (3)); return; }
    g.setColour (Colours::black.withAlpha (th.dark ? 0.4f : 0.15f));
    g.fillRoundedRectangle (r.translated (0, 1.5f), rad);
    g.setGradientFill (ColourGradient (col (th.displayBg).brighter (0.06f), 0, r.getY(), col (th.displayBg), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, rad);
    g.setColour (th.look == Look::Terminal ? col (th.panelEdge) : Colours::white.withAlpha (th.dark ? 0.08f : 0.25f));
    g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    if (th.look == Look::Terminal)
    {
        g.setColour (col (th.accent).withAlpha (0.05f));
        for (float y = r.getY() + 2; y < r.getBottom(); y += 3.0f) g.fillRect (r.getX() + 2, y, r.getWidth() - 4, 1.0f);
    }
    if (th.look == Look::Aero || th.look == Look::Glass) rimGloss (g, r, rad, 0.10f);
}

void CR3ALookAndFeel::paintLogo (Graphics& g, Rectangle<float> r, Colour c)
{
    if (! logoMask.isValid()) return;
    g.setColour (c);
    g.setImageResamplingQuality (Graphics::highResamplingQuality);
    g.drawImage (logoMask, r, RectanglePlacement::centred, true);
}

// ============================================================================
//  knob
// ============================================================================
void CR3ALookAndFeel::paintKnob (Graphics& g, Rectangle<float> area, float pos, bool bipolar, int steps, bool hover, bool drag)
{
    const auto& th = t();
    const float D = jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float R = D * 0.5f - 2.0f;                       // outer ring radius
    const float r = R * (th.look == Look::Terminal ? 0.68f : 0.72f);   // body radius
    const float angle = kStart + pos * (kEnd - kStart);
    const float zero = kStart + 0.5f * (kEnd - kStart);
    const float a0 = bipolar ? jmin (zero, angle) : kStart;
    const float a1 = bipolar ? jmax (zero, angle) : angle;
    const float ringW = jmax (2.5f, D * 0.05f);
    auto ptAt = [&] (float ang, float rad) { return Point<float> (c.x + rad * std::sin (ang), c.y - rad * std::cos (ang)); };

    auto arc = [&] (float from, float to, float rad, float w, Colour colr)
    {
        if (to - from < 0.001f) return;
        Path p; p.addCentredArc (c.x, c.y, rad, rad, 0, from, to, true);
        g.setColour (colr);
        g.strokePath (p, PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded));
    };
    auto stepDots = [&] (Colour dot)
    {
        if (steps < 2) return;
        for (int i = 0; i < steps; ++i)
        {
            const float ang = kStart + (float) i / (float) (steps - 1) * (kEnd - kStart);
            auto p = ptAt (ang, R + 0.5f);
            g.setColour (dot);
            g.fillEllipse (p.x - 1.6f, p.y - 1.6f, 3.2f, 3.2f);
        }
    };

    switch (th.look)
    {
        case Look::Graphite:
        {
            arc (kStart, kEnd, R - ringW * 0.5f, ringW, col (0xff1a1b1d));
            arc (a0, a1, R - ringW * 0.5f, ringW, col (th.accent).withAlpha (hover || drag ? 1.0f : 0.92f));
            stepDots (col (th.textDim));
            g.setColour (Colours::black.withAlpha (0.5f));
            g.fillEllipse (c.x - r, c.y - r + 2.0f, r * 2, r * 2);
            g.setGradientFill (ColourGradient (col (0xff6d7077), c.x - r * 0.6f, c.y - r * 0.9f, col (0xff2a2b2f), c.x + r * 0.5f, c.y + r, false));
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setColour (Colours::black.withAlpha (0.55f));
            g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.0f);
            g.setColour (Colours::white.withAlpha (0.16f));
            g.drawEllipse (c.x - r + 1, c.y - r + 1, r * 2 - 2, r * 2 - 2, 1.0f);
            const float ri = r * 0.78f;
            g.setGradientFill (ColourGradient (col (0xff3b3d42), c.x, c.y - ri, col (0xff25262a), c.x, c.y + ri, false));
            g.fillEllipse (c.x - ri, c.y - ri, ri * 2, ri * 2);
            g.setColour (Colours::white.withAlpha (drag ? 1.0f : 0.92f));
            g.drawLine (Line<float> (ptAt (angle, r * 0.28f), ptAt (angle, r * 0.86f)), 2.6f);
            break;
        }
        case Look::Aero:
        {
            arc (kStart, kEnd, R - ringW * 0.5f, ringW + 1, Colour (0xff0c3d5e).withAlpha (0.22f));
            arc (a0, a1, R - ringW * 0.5f, ringW + 0.5f, hover || drag ? Colour (0xff5fe0ff) : col (th.accent));
            stepDots (col (th.textDim));
            g.setColour (Colour (0xff0c3d5e).withAlpha (0.28f));
            g.fillEllipse (c.x - r, c.y - r + 3.0f, r * 2, r * 2);
            g.setGradientFill (ColourGradient (Colour (0xffeafaff), c.x, c.y - r, Colour (0xff4aaee8), c.x, c.y + r, false));
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setColour (Colour (0xff2b86b8));
            g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.4f);
            // gloss
            Path gl; gl.addEllipse (c.x - r * 0.78f, c.y - r * 0.96f, r * 1.56f, r * 1.0f);
            g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.95f), c.x, c.y - r, Colours::white.withAlpha (0.05f), c.x, c.y, false));
            g.fillPath (gl);
            g.setColour (Colour (0xff0c3d5e));
            g.drawLine (Line<float> (ptAt (angle, r * 0.30f), ptAt (angle, r * 0.80f)), 2.6f);
            break;
        }
        case Look::XP:
        {
            arc (kStart, kEnd, R - ringW * 0.5f, ringW, col (th.track));
            arc (a0, a1, R - ringW * 0.5f, ringW, hover || drag ? Colour (0xff4b86ff) : col (th.accent));
            stepDots (col (0xff6c6a5c));
            g.setColour (Colours::black.withAlpha (0.25f));
            g.fillEllipse (c.x - r + 1, c.y - r + 2.0f, r * 2, r * 2);
            g.setGradientFill (ColourGradient (Colours::white, c.x - r * 0.4f, c.y - r * 0.6f, Colour (0xffcfcab4), c.x + r * 0.7f, c.y + r * 0.9f, false));
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setColour (Colour (0xff003c74));
            g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.3f);
            g.setColour (Colours::white.withAlpha (0.8f));
            g.drawEllipse (c.x - r + 2, c.y - r + 2, r * 2 - 4, r * 2 - 4, 1.0f);
            g.setColour (Colour (0xff0b3fc9));
            g.drawLine (Line<float> (ptAt (angle, r * 0.25f), ptAt (angle, r * 0.82f)), 2.8f);
            break;
        }
        case Look::Glass:
        {
            arc (kStart, kEnd, R - ringW * 0.5f, ringW, Colours::black.withAlpha (0.12f));
            arc (a0, a1, R - ringW * 0.5f, ringW, hover || drag ? col (th.accent).brighter (0.2f) : col (th.accent));
            stepDots (Colours::black.withAlpha (0.35f));
            g.setColour (Colour (0xff3a3f78).withAlpha (0.22f));
            g.fillEllipse (c.x - r, c.y - r + 3.5f, r * 2, r * 2);
            g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.85f), c.x - r * 0.5f, c.y - r * 0.7f, Colour (0xffcbd5f2).withAlpha (0.65f), c.x + r * 0.6f, c.y + r, false));
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setColour (Colours::white.withAlpha (0.9f));
            g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.2f);
            g.setColour (Colours::black.withAlpha (0.08f));
            g.drawEllipse (c.x - r - 0.8f, c.y - r - 0.8f, r * 2 + 1.6f, r * 2 + 1.6f, 1.0f);
            Path hl; hl.addCentredArc (c.x, c.y, r * 0.82f, r * 0.82f, 0, -2.6f, -0.9f, true);
            g.setColour (Colours::white.withAlpha (0.95f));
            g.strokePath (hl, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (Colour (0xff1b1c21));
            const auto p = ptAt (angle, r * 0.62f);
            g.fillEllipse (p.x - 2.6f, p.y - 2.6f, 5.2f, 5.2f);
            break;
        }
        case Look::Win98:
        {
            const int ticks = 21;
            for (int i = 0; i < ticks; ++i)
            {
                const float f = (float) i / (float) (ticks - 1);
                const float ang = kStart + f * (kEnd - kStart);
                const bool lit = bipolar ? ((f >= 0.5f && f <= pos + 0.001f && pos >= 0.5f) || (f <= 0.5f && f >= pos - 0.001f && pos <= 0.5f))
                                         : f <= pos + 0.001f;
                const auto p = ptAt (ang, R - 1.5f);
                g.setColour (lit ? col (th.accent) : col (0xff808080));
                g.fillRect (p.x - 1.5f, p.y - 1.5f, 3.0f, 3.0f);
            }
            g.setColour (col (0xffc0c0c0));
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setColour (Colours::white);
            Path hi; hi.addCentredArc (c.x, c.y, r - 0.5f, r - 0.5f, 0, MathConstants<float>::pi * 1.0f, MathConstants<float>::pi * 1.5f + 1.57f, true);
            g.strokePath (hi, PathStrokeType (1.5f));
            g.setColour (Colour (0xff404040));
            Path lo; lo.addCentredArc (c.x, c.y, r - 0.5f, r - 0.5f, 0, 0.0f, MathConstants<float>::pi, true);
            g.strokePath (lo, PathStrokeType (1.5f));
            g.setColour (col (0xff808080));
            g.drawEllipse (c.x - r + 2.5f, c.y - r + 2.5f, r * 2 - 5, r * 2 - 5, 1.0f);
            g.setColour (Colours::black);
            g.drawLine (Line<float> (ptAt (angle, r * 0.2f), ptAt (angle, r * 0.8f)), 2.0f);
            break;
        }
        case Look::Terminal:
        {
            const int ticks = 27;
            for (int i = 0; i < ticks; ++i)
            {
                const float f = (float) i / (float) (ticks - 1);
                const float ang = kStart + f * (kEnd - kStart);
                bool lit;
                if (bipolar) lit = (pos >= 0.5f) ? (f >= 0.5f - 0.001f && f <= pos + 0.001f) : (f <= 0.5f + 0.001f && f >= pos - 0.001f);
                else lit = f <= pos + 0.001f;
                g.setColour (lit ? col (th.accent).withAlpha (hover || drag ? 1.0f : 0.95f) : col (0xff1e3a27));
                g.drawLine (Line<float> (ptAt (ang, R * 0.84f), ptAt (ang, R)), lit ? 2.4f : 1.6f);
            }
            g.setColour (Colours::black);
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setGradientFill (ColourGradient (col (0xff6a7a70), c.x - r * 0.5f, c.y - r * 0.7f, col (0xff161e19), c.x + r * 0.4f, c.y + r, false));
            g.fillEllipse (c.x - r + 1.5f, c.y - r + 1.5f, r * 2 - 3, r * 2 - 3);
            g.setColour (col (0xff1c241f));
            g.fillEllipse (c.x - r * 0.62f, c.y - r * 0.62f, r * 1.24f, r * 1.24f);
            g.setColour (col (th.accent));
            const auto p = ptAt (angle, r * 0.84f);
            g.fillEllipse (p.x - 2.2f, p.y - 2.2f, 4.4f, 4.4f);
            break;
        }
        case Look::Paper:
        {
            arc (kStart, kEnd, R - 1.5f, 2.0f, col (th.track));
            arc (a0, a1, R - 1.5f, 2.4f, col (th.accent));
            stepDots (col (th.textDim));
            g.setColour (Colours::black.withAlpha (0.10f));
            g.fillEllipse (c.x - r, c.y - r + 2.0f, r * 2, r * 2);
            g.setGradientFill (ColourGradient (Colours::white, c.x, c.y - r, col (0xffe6e4de), c.x, c.y + r, false));
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setColour (col (th.panelEdge));
            g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.0f);
            g.setColour (col (th.accent));
            g.drawLine (Line<float> (ptAt (angle, r * 0.35f), ptAt (angle, r * 0.86f)), 2.4f);
            break;
        }
    }
}

void CR3ALookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider& s)
{
    const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);
    const int steps = (int) s.getProperties().getWithDefault ("steps", 0);
    paintKnob (g, Rectangle<int> (x, y, w, h).toFloat(), pos, bipolar, steps, s.isMouseOver (true), s.isMouseButtonDown());
}

// ============================================================================
//  switches / power / buttons
// ============================================================================
void CR3ALookAndFeel::paintPower (Graphics& g, Rectangle<float> r, bool on, bool hover)
{
    const auto& th = t();
    const auto c = r.getCentre();
    const float d = jmin (r.getWidth(), r.getHeight());
    const float rad = d * 0.5f;
    auto icon = [&] (Colour ic, float w)
    {
        Path p; p.addCentredArc (c.x, c.y + 0.5f, rad * 0.42f, rad * 0.42f, 0, 0.62f, MathConstants<float>::twoPi - 0.62f, true);
        p.startNewSubPath (c.x, c.y - rad * 0.55f); p.lineTo (c.x, c.y - rad * 0.08f);
        g.setColour (ic);
        g.strokePath (p, PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded));
    };
    switch (th.look)
    {
        case Look::Graphite:
        case Look::Paper:
        {
            const auto base = th.look == Look::Paper ? col (0xffffffff) : col (0xff26272a);
            g.setColour (base); g.fillEllipse (c.x - rad, c.y - rad, d, d);
            g.setColour (hover ? col (th.text) : col (th.panelEdge)); g.drawEllipse (c.x - rad + 0.5f, c.y - rad + 0.5f, d - 1, d - 1, 1.2f);
            if (on && th.look == Look::Graphite) { g.setColour (Colours::white.withAlpha (0.12f)); g.fillEllipse (c.x - rad - 2, c.y - rad - 2, d + 4, d + 4); }
            icon (on ? col (th.ledOn) : col (th.ledOff), 1.8f);
            break;
        }
        case Look::Aero:
        {
            g.setGradientFill (ColourGradient (on ? Colour (0xffb5f58f) : Colour (0xffe0f0fa), c.x, c.y - rad, on ? Colour (0xff3ab51c) : Colour (0xff86b6d4), c.x, c.y + rad, false));
            g.fillEllipse (c.x - rad, c.y - rad, d, d);
            g.setColour (Colour (0xff1f6a8c)); g.drawEllipse (c.x - rad, c.y - rad, d, d, 1.2f);
            g.setColour (Colours::white.withAlpha (0.8f)); g.fillEllipse (c.x - rad * 0.65f, c.y - rad * 0.9f, rad * 1.3f, rad * 0.8f);
            icon (on ? Colour (0xff0e5a10) : Colour (0xff3f7fa0), 1.8f);
            break;
        }
        case Look::XP:
        {
            auto b = r.reduced (1.0f);
            g.setGradientFill (ColourGradient (on ? Colour (0xff7be26a) : Colour (0xffe8e5d6), b.getX(), b.getY(), on ? Colour (0xff2e9e24) : Colour (0xffb9b5a1), b.getX(), b.getBottom(), false));
            g.fillRoundedRectangle (b, 4.0f);
            g.setColour (Colours::white); g.drawRoundedRectangle (b, 4.0f, 1.4f);
            g.setColour (hover ? Colour (0xff0b3fc9) : Colour (0xff4a4a40)); g.drawRoundedRectangle (b.expanded (0.5f), 4.0f, 1.0f);
            icon (on ? Colours::white : Colour (0xff5e5c50), 1.8f);
            break;
        }
        case Look::Glass:
        {
            g.setColour (Colours::black.withAlpha (0.12f)); g.fillEllipse (c.x - rad, c.y - rad + 1.5f, d, d);
            g.setGradientFill (ColourGradient (on ? Colour (0xff6ff08f) : Colours::white.withAlpha (0.8f), c.x, c.y - rad, on ? Colour (0xff1fb84a) : Colours::white.withAlpha (0.35f), c.x, c.y + rad, false));
            g.fillEllipse (c.x - rad, c.y - rad, d, d);
            g.setColour (Colours::white.withAlpha (0.9f)); g.drawEllipse (c.x - rad + 0.5f, c.y - rad + 0.5f, d - 1, d - 1, 1.2f);
            icon (on ? Colours::white : Colour (0x99202228), 1.8f);
            break;
        }
        case Look::Win98:
        {
            auto b = r.reduced (1.0f);
            g.setColour (col (0xffc0c0c0)); g.fillRect (b);
            bevel (g, b, ! on);
            g.setColour (on ? col (0xff00aa22) : col (0xff606060));
            g.fillRect (b.reduced (b.getWidth() * 0.32f));
            break;
        }
        case Look::Terminal:
        {
            auto b = r.reduced (1.5f);
            g.setColour (col (0xff07110a)); g.fillRect (b);
            g.setColour (col (th.panelEdge)); g.drawRect (b, 1.0f);
            if (on) { g.setColour (col (th.accent).withAlpha (0.25f)); g.fillRect (b.expanded (2)); g.setColour (col (th.accent)); g.fillRect (b.reduced (3.5f)); }
            else { g.setColour (col (th.ledOff)); g.fillRect (b.reduced (3.5f)); }
            break;
        }
    }
}

void CR3ALookAndFeel::paintSwitch (Graphics& g, Rectangle<float> r, bool on, bool hover, const String& label)
{
    const auto& th = t();
    const float sw = 38.0f, sh = 20.0f;
    auto sr = Rectangle<float> (r.getX(), r.getCentreY() - sh * 0.5f, sw, sh);
    auto textArea = r.withTrimmedLeft (sw + 8.0f);
    const float thumb = sh - 6.0f;
    const float tx = on ? sr.getRight() - thumb - 3.0f : sr.getX() + 3.0f;
    auto thumbR = Rectangle<float> (tx, sr.getY() + 3.0f, thumb, thumb);

    g.setFont (font (11.5f, true).withExtraKerningFactor (th.look == Look::Paper || th.look == Look::Graphite ? 0.1f : 0.0f));

    switch (th.look)
    {
        case Look::Graphite:
        {
            g.setColour (col (0xff151618)); g.fillRoundedRectangle (sr, sh * 0.5f);
            g.setColour (on ? col (th.accent) : col (0xff3a3c41)); g.fillRoundedRectangle (sr.reduced (1.5f), sh * 0.5f - 1.5f);
            g.setGradientFill (ColourGradient (on ? col (0xff4a4c52) : col (0xff8a8d94), 0, thumbR.getY(), on ? col (0xff1f2023) : col (0xff53565c), 0, thumbR.getBottom(), false));
            g.fillEllipse (thumbR);
            g.setColour (hover ? Colours::white.withAlpha (0.3f) : Colours::white.withAlpha (0.12f)); g.drawEllipse (thumbR, 1.0f);
            g.setColour (col (on ? th.text : th.textDim));
            g.drawText (label.toUpperCase(), textArea, Justification::centredLeft);
            break;
        }
        case Look::Aero:
        {
            g.setGradientFill (ColourGradient (on ? Colour (0xff8cf05c) : Colour (0xff9fc8e0), 0, sr.getY(), on ? Colour (0xff2fb01a) : Colour (0xffd3ebf8), 0, sr.getBottom(), false));
            g.fillRoundedRectangle (sr, sh * 0.5f);
            g.setColour (Colour (0xff1f6a8c)); g.drawRoundedRectangle (sr, sh * 0.5f, 1.2f);
            g.setGradientFill (ColourGradient (Colours::white, 0, thumbR.getY(), Colour (0xffb9dcf0), 0, thumbR.getBottom(), false));
            g.fillEllipse (thumbR);
            g.setColour (Colour (0xff3f7fa0)); g.drawEllipse (thumbR, 1.0f);
            g.setColour (col (th.text)); g.drawText (label, textArea, Justification::centredLeft);
            break;
        }
        case Look::XP:
        {
            auto b = sr;
            g.setGradientFill (ColourGradient (on ? Colour (0xff5b98ff) : Colour (0xfff3f1e6), 0, b.getY(), on ? Colour (0xff1c55d6) : Colour (0xffd5d1bd), 0, b.getBottom(), false));
            g.fillRoundedRectangle (b, 4.0f);
            g.setColour (hover ? Colour (0xff0b3fc9) : Colour (0xff003c74)); g.drawRoundedRectangle (b, 4.0f, 1.2f);
            g.setGradientFill (ColourGradient (Colours::white, 0, thumbR.getY(), Colour (0xffd2cdb6), 0, thumbR.getBottom(), false));
            g.fillRoundedRectangle (thumbR, 3.0f);
            g.setColour (Colour (0xff6c6a5c)); g.drawRoundedRectangle (thumbR, 3.0f, 1.0f);
            g.setColour (col (th.text)); g.drawText (label, textArea, Justification::centredLeft);
            break;
        }
        case Look::Glass:
        {
            g.setColour (on ? col (th.accent2) : Colour (0x40000000)); g.fillRoundedRectangle (sr, sh * 0.5f);
            g.setColour (Colours::white.withAlpha (0.4f)); g.drawRoundedRectangle (sr.reduced (0.5f), sh * 0.5f, 1.0f);
            g.setColour (Colours::black.withAlpha (0.18f)); g.fillEllipse (thumbR.translated (0, 1.2f));
            g.setGradientFill (ColourGradient (Colours::white, 0, thumbR.getY(), Colour (0xffe4e8f6), 0, thumbR.getBottom(), false));
            g.fillEllipse (thumbR);
            g.setColour (col (th.text)); g.drawText (label, textArea, Justification::centredLeft);
            break;
        }
        case Look::Win98:
        {
            auto b = Rectangle<float> (r.getX(), r.getCentreY() - 10.0f, jmin (r.getWidth(), 86.0f), 20.0f);
            g.setColour (col (0xffc0c0c0)); g.fillRect (b);
            bevel (g, b, ! on);
            g.setColour (Colours::black);
            g.drawText (label, b.translated (on ? 1.0f : 0.0f, on ? 1.0f : 0.0f), Justification::centred);
            if (on) { g.setColour (col (0xff008000)); g.fillRect (b.getX() + 5, b.getCentreY() - 2.0f, 4.0f, 4.0f); }
            break;
        }
        case Look::Terminal:
        {
            g.setColour (on ? col (th.accent) : col (th.textDim));
            g.drawText (String (on ? "[x] " : "[ ] ") + label.toUpperCase(), r, Justification::centredLeft);
            break;
        }
        case Look::Paper:
        {
            g.setColour (on ? col (th.accent) : col (0xffd8d6cf)); g.fillRoundedRectangle (sr, sh * 0.5f);
            g.setColour (col (th.panelEdge)); g.drawRoundedRectangle (sr, sh * 0.5f, 1.0f);
            g.setColour (Colours::white); g.fillEllipse (thumbR);
            g.setColour (Colours::black.withAlpha (0.2f)); g.drawEllipse (thumbR, 1.0f);
            g.setColour (col (on ? th.text : th.textDim)); g.drawText (label.toUpperCase(), textArea, Justification::centredLeft);
            break;
        }
    }
}

void CR3ALookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool over, bool)
{
    const bool on = b.getToggleState();
    const auto kind = b.getProperties().getWithDefault ("kind", "switch").toString();
    auto r = b.getLocalBounds().toFloat();
    if (kind == "power") paintPower (g, r, on, over);
    else                 paintSwitch (g, r, on, over, b.getButtonText());
}

void CR3ALookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    const auto& th = t();
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const float rad = th.look == Look::Win98 ? 0.0f : jmin (th.radius, r.getHeight() * 0.5f);
    switch (th.look)
    {
        case Look::Win98:
            g.setColour (col (0xffc0c0c0)); g.fillRect (r); bevel (g, r, ! down); break;
        case Look::XP:
            g.setGradientFill (ColourGradient (down ? Colour (0xffd8d4c0) : Colours::white, 0, r.getY(), down ? Colour (0xffeeecdd) : Colour (0xffd8d4be), 0, r.getBottom(), false));
            g.fillRoundedRectangle (r, 3.0f);
            g.setColour (over ? Colour (0xff3d95ff) : Colour (0xff003c74)); g.drawRoundedRectangle (r, 3.0f, 1.2f); break;
        case Look::Aero:
            g.setGradientFill (ColourGradient (Colours::white.withAlpha (down ? 0.4f : 0.9f), 0, r.getY(), Colour (0xff8ed0f2).withAlpha (over ? 1.0f : 0.8f), 0, r.getBottom(), false));
            g.fillRoundedRectangle (r, rad);
            g.setColour (Colour (0xff1f6a8c)); g.drawRoundedRectangle (r, rad, 1.1f); break;
        case Look::Glass:
            g.setColour (Colours::white.withAlpha (down ? 0.22f : over ? 0.55f : 0.38f)); g.fillRoundedRectangle (r, rad);
            g.setColour (Colours::white.withAlpha (0.75f)); g.drawRoundedRectangle (r, rad, 1.0f); break;
        case Look::Terminal:
            g.setColour (over ? col (0xff143222) : col (0xff0a170f)); g.fillRoundedRectangle (r, rad);
            g.setColour (col (th.accent).withAlpha (over ? 1.0f : 0.6f)); g.drawRoundedRectangle (r, rad, 1.0f); break;
        case Look::Paper:
            g.setColour (down ? col (0xffe6e4de) : Colours::white); g.fillRoundedRectangle (r, rad);
            g.setColour (over ? col (th.accent) : col (th.panelEdge)); g.drawRoundedRectangle (r, rad, 1.0f); break;
        case Look::Graphite:
        default:
            g.setGradientFill (ColourGradient (col (down ? 0xff2a2b2f : 0xff3f4146), 0, r.getY(), col (down ? 0xff303236 : 0xff2b2d31), 0, r.getBottom(), false));
            g.fillRoundedRectangle (r, rad);
            g.setColour (over ? col (0xff8a8d94) : col (th.panelEdge)); g.drawRoundedRectangle (r, rad, 1.0f); break;
    }
}

void CR3ALookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool down)
{
    const auto& th = t();
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (th.look == Look::Glass ? col (th.text) : col (th.text));
    auto r = b.getLocalBounds();
    if (th.look == Look::Win98 && down) r.translate (1, 1);
    g.drawText (b.getButtonText(), r, Justification::centred);
}

void CR3ALookAndFeel::drawComboBox (Graphics& g, int w, int h, bool down, int, int, int, int, ComboBox&)
{
    const auto& th = t();
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    const float rad = th.look == Look::Win98 ? 0.0f : jmin (th.radius, r.getHeight() * 0.5f);
    switch (th.look)
    {
        case Look::Win98: g.setColour (Colours::white); g.fillRect (r); bevel (g, r, false); break;
        case Look::XP:    g.setColour (Colours::white); g.fillRoundedRectangle (r, 2.0f); g.setColour (Colour (0xff7f9db9)); g.drawRoundedRectangle (r, 2.0f, 1.0f); break;
        case Look::Aero:  g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.95f), 0, 0, Colour (0xffc9e9f9), 0, (float) h, false)); g.fillRoundedRectangle (r, rad); g.setColour (Colour (0xff1f6a8c)); g.drawRoundedRectangle (r, rad, 1.1f); break;
        case Look::Glass: g.setColour (Colours::white.withAlpha (down ? 0.5f : 0.4f)); g.fillRoundedRectangle (r, rad); g.setColour (Colours::white.withAlpha (0.75f)); g.drawRoundedRectangle (r, rad, 1.0f); break;
        case Look::Terminal: g.setColour (col (0xff0a170f)); g.fillRoundedRectangle (r, rad); g.setColour (col (th.accent).withAlpha (0.6f)); g.drawRoundedRectangle (r, rad, 1.0f); break;
        case Look::Paper: g.setColour (Colours::white); g.fillRoundedRectangle (r, rad); g.setColour (col (th.panelEdge)); g.drawRoundedRectangle (r, rad, 1.0f); break;
        default: g.setGradientFill (ColourGradient (col (0xff3f4146), 0, 0, col (0xff2b2d31), 0, (float) h, false)); g.fillRoundedRectangle (r, rad); g.setColour (col (th.panelEdge)); g.drawRoundedRectangle (r, rad, 1.0f); break;
    }
    Path p;
    const float ax = (float) w - 15.0f, ay = (float) h * 0.5f;
    p.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (th.look == Look::Win98 || th.look == Look::XP ? Colours::black : col (th.text));
    g.fillPath (p);
}

void CR3ALookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 28, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setColour (Label::textColourId, (t().look == Look::Win98 || t().look == Look::XP) ? Colours::black : col (t().text));
}

void CR3ALookAndFeel::paintMeterBar (Graphics& g, Rectangle<float> r, float v01, bool reductionDown)
{
    const auto& th = t();
    v01 = jlimit (0.0f, 1.0f, v01);
    g.setColour (col (th.look == Look::Win98 ? 0xff000000 : th.displayBg));
    g.fillRoundedRectangle (r, 2.0f);
    g.setColour (col (th.displayGrid));
    g.drawRoundedRectangle (r, 2.0f, 1.0f);
    auto inner = r.reduced (2.0f);
    const int segs = 14;
    const float sh = inner.getHeight() / (float) segs;
    for (int i = 0; i < segs; ++i)
    {
        const float f = (float) i / (float) segs;                 // 0 = bottom
        const bool lit = reductionDown ? ((1.0f - f) - 1.0f / segs <= v01 + 0.001f && (1.0f - f) <= v01 + 1.0f / segs)
                                       : f < v01;
        Rectangle<float> seg (inner.getX(), inner.getBottom() - (float) (i + 1) * sh + 1.0f, inner.getWidth(), sh - 1.5f);
        if (reductionDown) seg = Rectangle<float> (inner.getX(), inner.getY() + (float) i * sh + 0.5f, inner.getWidth(), sh - 1.5f);
        const bool l2 = reductionDown ? ((float) (i + 1) / (float) segs <= v01 + 0.0001f) : lit;
        Colour c = col (th.accent);
        if (! reductionDown && f > 0.85f) c = Colour (0xffff5a4a);
        g.setColour (l2 ? c : c.withAlpha (0.14f));
        g.fillRect (seg);
    }
}

} // namespace cr3a
