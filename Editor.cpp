#include "Processor.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <vector>

namespace
{
using namespace juce;
using State = AudioProcessorValueTreeState;

// =========================================================================
// FL HEAT STUDIO DARK PALETTE (Exact FLStudioLookAndFeel.h hexes)
// =========================================================================
const Colour kFLBgDarkest      { 0xff171a1d };
const Colour kFLPanelRecessed  { 0xff1e2124 };
const Colour kFLPanelMain      { 0xff262a2e };
const Colour kFLPanelRaised    { 0xff2f3439 };
const Colour kFLBevelLight     { 0xff3c4248 };
const Colour kFLBevelDark      { 0xff121417 };
const Colour kFLTextBright     { 0xffd4d8dc };
const Colour kFLTextMuted      { 0xff8c939a };
const Colour kFLTextDim        { 0xff5c6268 };
const Colour kFLAccentOrange   { 0xffff9e2c };
const Colour kFLAccentGreen    { 0xff6ee755 };
const Colour kFLAccentCyan     { 0xff42a5f5 };
const Colour kFLAccentRed      { 0xffef5350 };
const Colour kFLButtonOff      { 0xff34393e };

// Shared LED colors
const Colour kLedEmerald       { 0xff32f567 };
const Colour kLedLime          { 0xff94f525 };
const Colour kLedAmber         { 0xffffb620 };
const Colour kLedRed           { 0xffff2e2e };

Font getFLFont (float height, bool bold = true)
{
    return Font (FontOptions ("Segoe UI", height, bold ? Font::bold : Font::plain));
}

// =========================================================================
// Exact Personal Signature of papielguapo (from OUTPUT / PersonalSignature.h)
// =========================================================================
inline void drawPapielguapoSignature (Graphics& g, Rectangle<float> area, Colour colour)
{
    Path signature;
    signature.startNewSubPath(0,22);signature.lineTo(12,2);signature.cubicTo(25,-3,25,7,13,12);signature.lineTo(7,12);
    signature.startNewSubPath(14,18);signature.cubicTo(21,6,29,9,22,18);signature.cubicTo(18,24,13,22,18,16);signature.lineTo(28,12);signature.lineTo(24,20);signature.lineTo(34,14);
    signature.startNewSubPath(29,29);signature.lineTo(39,10);signature.cubicTo(49,5,48,15,40,19);signature.lineTo(36,18);signature.lineTo(49,13);
    signature.startNewSubPath(51,10);signature.lineTo(46,20);signature.quadraticTo(49,23,57,14);
    signature.startNewSubPath(54,6);signature.lineTo(55,4);
    signature.startNewSubPath(54,17);signature.cubicTo(66,16,67,9,61,11);signature.cubicTo(50,17,56,26,68,17);
    signature.cubicTo(82,1,80,-3,73,8);signature.lineTo(68,19);signature.quadraticTo(68,23,78,16);
    signature.cubicTo(86,6,93,10,86,18);signature.cubicTo(79,26,77,16,84,13);signature.lineTo(91,11);signature.lineTo(81,29);signature.cubicTo(75,37,70,29,88,22);
    signature.lineTo(95,13);signature.lineTo(91,20);signature.quadraticTo(92,24,101,14);signature.lineTo(98,20);signature.quadraticTo(101,22,108,14);
    signature.cubicTo(116,7,122,10,115,18);signature.cubicTo(108,26,106,17,112,13);signature.lineTo(120,11);signature.lineTo(116,20);signature.lineTo(127,13);
    signature.startNewSubPath(121,29);signature.lineTo(132,10);signature.cubicTo(142,5,142,15,134,19);signature.lineTo(129,18);signature.lineTo(143,13);
    signature.cubicTo(153,6,156,12,148,19);signature.cubicTo(140,25,137,17,146,12);signature.quadraticTo(149,17,155,13);

    g.setColour (Colours::black.withAlpha (0.7f));
    const auto transform = signature.getTransformToScaleToFit (area, true);
    g.strokePath (signature, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded), transform.translated (0.0f, 1.0f));
    g.setColour (colour);
    g.strokePath (signature, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded), transform);
}

void drawIndicatorLed (Graphics& g, float cx, float cy, float diameter, Colour col, bool lit) noexcept
{
    const float r = diameter * 0.5f;
    const Rectangle<float> bezel (cx - r - 1.2f, cy - r - 1.2f, diameter + 2.4f, diameter + 2.4f);

    g.setColour (Colour (0xff101317));
    g.fillEllipse (bezel);
    g.setColour (Colour (0xff343940));
    g.drawEllipse (bezel, 0.8f);

    const Rectangle<float> bulb (cx - r, cy - r, diameter, diameter);
    if (lit)
    {
        ColourGradient halo (col.withAlpha (0.48f), cx, cy, col.withAlpha (0.0f), cx + r * 2.5f, cy + r * 2.5f, true);
        g.setGradientFill (halo);
        g.fillEllipse (cx - r * 2.3f, cy - r * 2.3f, diameter * 2.3f, diameter * 2.3f);
        g.setColour (col);
        g.fillEllipse (bulb);
        g.setColour (Colours::white.withAlpha (0.85f));
        g.fillEllipse (cx - r * 0.45f, cy - r * 0.52f, r * 0.78f, r * 0.62f);
    }
    else
    {
        g.setColour (col.darker (2.6f).withAlpha (0.65f));
        g.fillEllipse (bulb);
        g.setColour (Colours::white.withAlpha (0.15f));
        g.fillEllipse (cx - r * 0.38f, cy - r * 0.44f, r * 0.52f, r * 0.42f);
    }
}

void drawCurveGlyph (Graphics& g, float cx, float cy, int type, Colour col) noexcept
{
    Path p;
    if (type == 0) // Narrow bell
    {
        p.startNewSubPath (cx - 6.0f, cy + 2.5f);
        p.lineTo (cx - 2.5f, cy + 2.5f);
        p.quadraticTo (cx, cy - 5.0f, cx + 2.5f, cy + 2.5f);
        p.lineTo (cx + 6.0f, cy + 2.5f);
    }
    else if (type == 1) // Wide bell
    {
        p.startNewSubPath (cx - 6.5f, cy + 2.5f);
        p.quadraticTo (cx, cy - 4.5f, cx + 6.5f, cy + 2.5f);
    }
    else if (type == 2) // High-pass slope
    {
        p.startNewSubPath (cx - 7.0f, cy + 3.5f);
        p.lineTo (cx - 2.0f, cy - 3.0f);
        p.lineTo (cx + 7.0f, cy - 3.0f);
    }
    else if (type == 3) // Low-pass slope
    {
        p.startNewSubPath (cx - 7.0f, cy - 3.0f);
        p.lineTo (cx + 2.0f, cy - 3.0f);
        p.lineTo (cx + 7.0f, cy + 3.5f);
    }
    g.setColour (col);
    g.strokePath (p, PathStrokeType (1.4f));
}

Image createFLHeatBackdrop (int w, int h)
{
    Image img (Image::ARGB, w, h, true);
    Graphics g (img);

    const float col1W = 192.0f;
    const float col2W = 192.0f;
    const float col3X = col1W + col2W;
    const float col3W = static_cast<float> (w) - col3X;

    g.fillAll (kFLPanelMain);
    ColourGradient heatGlow (kFLAccentOrange.withAlpha (0.055f), w * 0.45f, 110.0f,
                             Colours::transparentBlack, w * 0.45f, 650.0f, true);
    g.setGradientFill (heatGlow);
    g.fillAll();

    auto drawFLCard = [&] (Rectangle<float> r, const String& title, Colour dotCol)
    {
        g.setColour (kFLPanelRecessed);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (Colour (0xff141618));
        g.drawRoundedRectangle (r, 5.0f, 1.0f);
        g.setColour (Colour (0xff363b40).withAlpha (0.4f));
        g.drawHorizontalLine (roundToInt (r.getBottom() - 1.0f), r.getX() + 4.0f, r.getRight() - 4.0f);

        if (title.isNotEmpty())
        {
            Rectangle<float> hdr (r.getX() + 1.0f, r.getY() + 1.0f, r.getWidth() - 2.0f, 19.0f);
            g.setColour (Colour (0xff16181b));
            g.fillRoundedRectangle (hdr, 4.0f);
            g.setColour (dotCol);
            g.fillEllipse (hdr.getX() + 6.0f, hdr.getCentreY() - 2.8f, 5.6f, 5.6f);
            g.setColour (kFLTextBright);
            g.setFont (getFLFont (10.5f, true));
            g.drawText (title, hdr.withTrimmedLeft (15.0f), Justification::centredLeft, false);
        }
    };

    // =========================================================================
    // COLUMN 1: Separated COMPRESSOR and GATE / EXPAND cards!
    // =========================================================================
    drawFLCard ({ 5.0f,   5.0f, col1W - 10.0f, 188.0f }, "INPUT GAIN & FL HEAT", kFLAccentOrange);
    drawFLCard ({ 5.0f, 198.0f, col1W - 10.0f, 224.0f }, "COMPRESSOR (FAST ATK)", kFLAccentCyan);
    drawFLCard ({ 5.0f, 427.0f, col1W - 10.0f, 285.0f }, "GATE / EXPAND (FAST ATK)", kFLAccentCyan);
    drawFLCard ({ 5.0f, 717.0f, col1W - 10.0f, 198.0f }, "FILTERS (HP / LP + dB/OCT)", kFLAccentCyan);

    // =========================================================================
    // COLUMN 2: Separated EQ HF, EQ MID (HMF & LMF), EQ LF, DE-ESSER, and MID/SIDE!
    // =========================================================================
    drawFLCard ({ col1W + 5.0f,   5.0f, col2W - 10.0f, 118.0f }, "EQ HF (HIGH)", kFLAccentRed);
    drawFLCard ({ col1W + 5.0f, 128.0f, col2W - 10.0f, 250.0f }, "EQ MID (HMF & LMF)", kFLAccentCyan);
    drawFLCard ({ col1W + 5.0f, 383.0f, col2W - 10.0f, 178.0f }, "EQ LF (LOW)", kFLAccentOrange);
    drawFLCard ({ col1W + 5.0f, 566.0f, col2W - 10.0f, 174.0f }, "VOCALS DE-ESSER (SPLIT)", kFLAccentRed);
    drawFLCard ({ col1W + 5.0f, 745.0f, col2W - 10.0f, 170.0f }, "MID / SIDE dB (OUTPUT)", kFLAccentCyan);

    // =========================================================================
    // COLUMN 3: INFLATE at top, MASTER FADER + DUAL L/R METER in middle,
    //           and CHANNELSTRIP + papielguapo signature BELOW the slider!
    // =========================================================================
    drawFLCard ({ col3X + 5.0f,   5.0f, col3W - 10.0f, 154.0f }, "INFLATE & CLIP 0 dB", kFLAccentOrange);
    drawFLCard ({ col3X + 5.0f, 164.0f, col3W - 10.0f, 624.0f }, "MASTER OUT & L/R METER", kFLAccentCyan);
    drawFLCard ({ col3X + 5.0f, 793.0f, col3W - 10.0f, 122.0f }, "", kFLAccentOrange);

    g.setColour (Colour (0xff0b0d10));
    g.drawRect (0, 0, w, h, 1);
    return img;
}

class FLHeatLookAndFeel final : public LookAndFeel_V4
{
public:
    FLHeatLookAndFeel()
    {
        setColour (Slider::textBoxTextColourId, Colours::transparentBlack);
        setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
        setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setColour (TextEditor::textColourId, Colours::white);
        setColour (TextEditor::backgroundColourId, Colour (0xff121417));
        setColour (TextEditor::outlineColourId, kFLAccentOrange);
        setColour (TextEditor::focusedOutlineColourId, kFLAccentOrange);
    }

    Slider::SliderLayout getSliderLayout (Slider& slider) override
    {
        auto layout = LookAndFeel_V4::getSliderLayout (slider);
        if (slider.isRotary())
        {
            const auto b = slider.getLocalBounds();
            layout.sliderBounds = b;
            layout.textBoxBounds = Rectangle<int> (2, b.getCentreY() - 10, b.getWidth() - 4, 20);
        }
        return layout;
    }

    Label* createSliderTextBox (Slider& slider) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (slider);
        if (slider.isRotary())
        {
            l->setColour (Label::textColourId, Colours::transparentBlack);
            l->setColour (Label::backgroundColourId, Colours::transparentBlack);
            l->setColour (Label::outlineColourId, Colours::transparentBlack);
            l->setColour (Label::textWhenEditingColourId, Colours::white);
            l->setColour (Label::backgroundWhenEditingColourId, Colour (0xff121417));
            l->setColour (Label::outlineWhenEditingColourId, kFLAccentOrange);
            l->setColour (TextEditor::textColourId, Colours::white);
            l->setColour (TextEditor::backgroundColourId, Colour (0xff121417));
            l->setColour (TextEditor::outlineColourId, kFLAccentOrange);
            l->setColour (TextEditor::focusedOutlineColourId, kFLAccentOrange);
            l->setFont (getFLFont (11.5f, true));
            l->setInterceptsMouseClicks (false, false);
        }
        return l;
    }

    void drawLabel (Graphics& g, Label& label) override
    {
        if (dynamic_cast<Slider*> (label.getParentComponent()) != nullptr && !label.isBeingEdited())
            return;
        LookAndFeel_V4::drawLabel (g, label);
    }

    void drawRotarySlider (Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, Slider& slider) override
    {
        const float cx = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
        const float cy = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        const String cid = slider.getComponentID();

        const float diam = static_cast<float> (std::min (width, height)) - 2.0f;
        const float arcRad = diam * 0.40f;
        const float knobRad = diam * 0.30f;
        const float trackW = std::max (2.6f, diam * 0.055f);

        const double minVal = slider.getMinimum();
        const double maxVal = slider.getMaximum();
        const bool isBipolar = (minVal < -0.001 && maxVal > 0.001 && std::abs (minVal + maxVal) < 1.0);
        const bool isCyan = (cid == "SSLBlue" || cid == "SSLGreen");
        const Colour accent = (cid == "SSLRed") ? kFLAccentRed
                            : isBipolar ? ((slider.getValue() >= 0.0) ? kFLAccentOrange : kFLAccentCyan)
                            : (isCyan ? kFLAccentCyan : kFLAccentOrange);

        const int numTicks = 21;
        for (int i = 0; i < numTicks; ++i)
        {
            const float prop = static_cast<float> (i) / static_cast<float> (numTicks - 1);
            const float a = rotaryStartAngle + prop * (rotaryEndAngle - rotaryStartAngle) - MathConstants<float>::halfPi;
            const bool isMajor = (i == 0 || i == numTicks / 2 || i == numTicks - 1);
            const float rIn = diam * 0.44f;
            const float rOut = diam * (isMajor ? 0.49f : 0.468f);
            g.setColour (isMajor ? kFLTextMuted.withAlpha (0.75f) : kFLTextDim.withAlpha (0.42f));
            g.drawLine (cx + std::cos (a) * rIn, cy + std::sin (a) * rIn,
                        cx + std::cos (a) * rOut, cy + std::sin (a) * rOut,
                        isMajor ? 1.3f : 0.85f);
        }

        Path bgArc;
        bgArc.addCentredArc (cx, cy, arcRad, arcRad, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (kFLBevelDark);
        g.strokePath (bgArc, PathStrokeType (trackW + 1.4f, PathStrokeType::curved, PathStrokeType::rounded));

        const float startValAngle = isBipolar
            ? (rotaryStartAngle + 0.5f * (rotaryEndAngle - rotaryStartAngle))
            : rotaryStartAngle;
        if (std::abs (angle - startValAngle) > 0.015f)
        {
            Path valArc;
            valArc.addCentredArc (cx, cy, arcRad, arcRad, 0.0f,
                                  std::min (startValAngle, angle), std::max (startValAngle, angle), true);
            g.setColour (accent.withAlpha (0.25f));
            g.strokePath (valArc, PathStrokeType (trackW + 4.2f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (accent);
            g.strokePath (valArc, PathStrokeType (trackW, PathStrokeType::curved, PathStrokeType::rounded));
        }

        g.setColour (Colours::black.withAlpha (0.52f));
        g.fillEllipse (cx - knobRad, cy - knobRad + 2.0f, knobRad * 2.0f, knobRad * 2.0f);

        g.setColour (Colour (0xff14171a));
        g.fillEllipse (cx - knobRad - 1.2f, cy - knobRad - 1.2f, (knobRad + 1.2f) * 2.0f, (knobRad + 1.2f) * 2.0f);

        ColourGradient capGrad (Colour (0xff383e44), cx, cy - knobRad,
                                Colour (0xff202428), cx, cy + knobRad, false);
        g.setGradientFill (capGrad);
        g.fillEllipse (cx - knobRad, cy - knobRad, knobRad * 2.0f, knobRad * 2.0f);
        g.setColour (Colour (0xff4a525a).withAlpha (0.55f));
        g.drawEllipse (cx - knobRad + 0.8f, cy - knobRad + 0.8f, (knobRad - 0.8f) * 2.0f, (knobRad - 0.8f) * 2.0f, 0.9f);

        const float pAng = angle - MathConstants<float>::halfPi;
        const float px1 = cx + std::cos (pAng) * (knobRad * 0.28f);
        const float py1 = cy + std::sin (pAng) * (knobRad * 0.28f);
        const float px2 = cx + std::cos (pAng) * (knobRad * 0.86f);
        const float py2 = cy + std::sin (pAng) * (knobRad * 0.86f);
        g.setColour (Colours::black.withAlpha (0.55f));
        g.drawLine (px1 + 0.6f, py1 + 0.8f, px2 + 0.6f, py2 + 0.8f, 2.2f);
        g.setColour (Colour (0xfff4f7fa));
        g.drawLine (px1, py1, px2, py2, 2.0f);
    }

    void drawLinearSlider (Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                           const Slider::SliderStyle style, Slider& /*slider*/) override
    {
        if (style != Slider::LinearVertical)
            return;

        const float cx = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
        const float trackTop = static_cast<float> (y) + 18.0f;
        const float trackBot = static_cast<float> (y + height) - 18.0f;

        const Rectangle<float> slot (cx - 3.2f, trackTop, 6.4f, trackBot - trackTop);
        g.setColour (kFLBevelDark);
        g.fillRoundedRectangle (slot, 3.0f);
        g.setColour (Colour (0xff2c3136));
        g.drawRoundedRectangle (slot, 3.0f, 0.9f);

        const float clampedPos = std::clamp (sliderPos, trackTop, trackBot);
        const Rectangle<float> fill (cx - 2.0f, clampedPos, 4.0f, trackBot - clampedPos);
        g.setColour (kFLAccentOrange.withAlpha (0.28f));
        g.fillRoundedRectangle (fill.expanded (1.8f, 0.0f), 2.5f);
        g.setColour (kFLAccentOrange);
        g.fillRoundedRectangle (fill, 2.0f);

        const float capW = 34.0f;
        const float capH = 52.0f;
        const Rectangle<float> cap (cx - capW * 0.5f,
                                    std::clamp (clampedPos - capH * 0.5f, trackTop - 12.0f, trackBot - capH + 12.0f),
                                    capW, capH);
        g.setColour (Colours::black.withAlpha (0.60f));
        g.fillRoundedRectangle (cap.translated (0.0f, 3.0f), 4.0f);
        ColourGradient cg (Colour (0xff3c434a), cap.getX(), cap.getY(), Colour (0xff202428), cap.getX(), cap.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (cap, 4.0f);
        g.setColour (Colour (0xff121417));
        g.drawRoundedRectangle (cap, 4.0f, 1.1f);
        g.setColour (kFLAccentOrange);
        g.fillRect (cap.getX() + 3.0f, cap.getCentreY() - 1.2f, cap.getWidth() - 6.0f, 2.4f);
    }

    void drawButtonBackground (Graphics& g, Button& button,
                               const Colour& /*backgroundColour*/,
                               bool over, bool down) override
    {
        const auto b = button.getLocalBounds().toFloat().reduced (0.8f);
        const bool on = button.getToggleState();
        const String cid = button.getComponentID();

        const Colour glow = (cid == "RedBtn")   ? kFLAccentRed
                          : (cid == "GreenBtn" || cid == "CyanBtn") ? kFLAccentCyan
                                                : kFLAccentOrange;
        if (on || down)
        {
            g.setColour (glow.withAlpha (0.22f));
            g.fillRoundedRectangle (b.expanded (1.2f), 3.5f);
            ColourGradient fg (glow.brighter (0.12f), b.getX(), b.getY(),
                               glow.darker (0.22f), b.getX(), b.getBottom(), false);
            g.setGradientFill (fg);
            g.fillRoundedRectangle (b, 3.0f);
            g.setColour (Colours::white.withAlpha (0.35f));
            g.drawRoundedRectangle (b.reduced (0.5f), 2.5f, 0.8f);
        }
        else
        {
            g.setColour (over ? kFLButtonOff.brighter (0.12f) : kFLButtonOff);
            g.fillRoundedRectangle (b, 3.0f);
            g.setColour (kFLBevelDark);
            g.drawRoundedRectangle (b, 3.0f, 1.0f);
        }
    }

    void drawButtonText (Graphics& g, TextButton& button, bool /*over*/, bool /*down*/) override
    {
        const auto b = button.getLocalBounds().toFloat();
        const bool on = button.getToggleState();

        g.setColour (on ? Colour (0xff121417) : kFLTextBright);
        g.setFont (getFLFont (10.0f, true));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (2, 1), Justification::centred, 1);
    }
};

// =========================================================================
// Functional Rotary Knob: Click & Drag anywhere on the knob to move it smoothly!
// Double-click or Right-click to type a numeric value directly.
// =========================================================================
class ConsoleDial final : public Slider
{
public:
    ConsoleDial()
    {
        setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle (Slider::TextBoxBelow, false, 52, 20);
        setDoubleClickReturnValue (false, 0.0);
        setSliderSnapsToMousePosition (false);
        setMouseDragSensitivity (130);
        setWantsKeyboardFocus (true);
        setPopupDisplayEnabled (true, true, nullptr);
    }

    bool hitTest (int x, int y) override
    {
        return getLocalBounds().contains (x, y);
    }

    void mouseDown (const MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            showTextBox();
            return;
        }
        Slider::mouseDown (e);
    }

    void mouseDoubleClick (const MouseEvent&) override
    {
        showTextBox();
    }
};

// =========================================================================
// Main 3-Column FL HEAT ChannelStrip Editor
// =========================================================================
class SurcoEditor final : public AudioProcessorEditor,
                          private Timer
{
public:
    static constexpr int kBaseW = 540;
    static constexpr int kBaseH = 920;

    SurcoEditor (AudioProcessor& p, State& s)
        : AudioProcessorEditor (p),
          proc (p),
          state (s),
          telemetry (getSurcoTelemetry (p))
    {
        setLookAndFeel (&lnf);

        addAndMakeVisible (canvas);
        canvas.setInterceptsMouseClicks (true, true);

        // 1. HIGH-GAIN INPUT (-24..+48 dB before Input Meter) + FL HEAT (DRIVE & TONE)
        setupKnob (kInputGain, "preamp",        "SSLSilver");
        setupKnob (kHeatDrive, "heatDrive",     "SSLRed");
        setupKnob (kHeatTone,  "heatTone",      "SSLSilver");

        // 2. DYNAMICS (COMP): RATIO, THRESHOLD, RELEASE (Fast Attack fixed)
        setupKnob (kCompRat,   "compRatio",     "SSLSilver");
        setupKnob (kCompThr,   "compThreshold", "SSLSilver");
        setupKnob (kCompRel,   "compRelease",   "SSLSilver");

        // 3. GATE / EXPANDER: THRESHOLD, RANGE EXPAND, RELEASE (Fast Attack fixed)
        setupKnob (kExpThr,    "gateThreshold", "SSLGreen");
        setupKnob (kExpRng,    "gateRange",     "SSLGreen");
        setupKnob (kExpRel,    "gateRelease",   "SSLGreen");

        // 4. FILTERS: HP & LP (2 knobs Hz + 1 button for dB/oct 6, 12, 18, 24)
        setupKnob (kHpf,       "hpf",           "SSLSilver");
        setupKnob (kLpf,       "lpf",           "SSLSilver");

        // 5. 4-BAND EQ (10 KNOBS: HF dB & Hz, HMF dB, BELL Q & Hz, LMF dB, BELL Q & Hz, LF dB & Hz + 2 BELL BUTTONS)
        setupKnob (kHfGain,    "highGain",      "SSLRed");
        setupKnob (kHfFreq,    "highFreq",      "SSLRed");

        setupKnob (kMidGain,   "highMidGain",   "SSLGreen");
        setupKnob (kMidQ,      "highMidQ",      "SSLGreen");
        setupKnob (kMidFreq,   "highMidFreq",   "SSLGreen");

        setupKnob (kLmfGain,   "lowMidGain",    "SSLBlue");
        setupKnob (kLmfQ,      "lowMidQ",       "SSLBlue");
        setupKnob (kLmfFreq,   "lowMidFreq",    "SSLBlue");

        setupKnob (kLfGain,    "lowGain",       "SSLBrown");
        setupKnob (kLfFreq,    "lowFreq",       "SSLBrown");

        // 6. DE-ESSER: 2 KNOBS (FREQ & THRESH) + SSL SPLIT BAND BUTTON
        setupKnob (kDsFreq,    "dsFreq",        "SSLSilver");
        setupKnob (kDsThr,     "dsThresh",      "SSLRed");

        // 7. MID / SIDE dB (from OUTPUT, placed below De-Esser in Column 2!)
        setupKnob (kMidDb,     "midGain",       "SSLBlue");
        setupKnob (kSideDb,    "sideGain",      "SSLBlue");

        // 8. INFLATE (ABOVE MASTER FADER in Column 3!): 2 KNOBS (INFLATE & CURVE) + CLIP 0 dB BUTTON
        setupKnob (kInfEff,    "inflateEffect", "SSLRed");
        setupKnob (kInfCrv,    "inflateCurve",  "SSLSilver");

        // 9. MASTER OUT FADER (Very last stage in DSP, alongside Dual L/R General Meter!)
        fader.setSliderStyle (Slider::LinearVertical);
        fader.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        fader.setDoubleClickReturnValue (true, 0.0);
        fader.setPopupDisplayEnabled (true, true, &canvas);
        canvas.addAndMakeVisible (fader);
        faderAtt = std::make_unique<State::SliderAttachment> (state, "fader", fader);

        // FL HEAT section buttons (all uniform size!)
        setupParamBtn  (bHeatByp, "HEAT BYP", "RedBtn",   "heatBypass");
        setupParamBtn  (bPhase,   String::fromUTF8 ("FASE 180\xc2\xb0"), "RedBtn", "phaseInv");
        setupParamBtn  (bPad,     "-20 dB",   "AmberBtn", "pad20");

        // Dynamics buttons
        setupParamBtn  (bLink,    "EXT SC",   "AmberBtn", "extSidechain");
        setupActionBtn (bExpSel,  "GATE",     "CyanBtn",  [this] { toggleChoice ("gateMode"); });
        setupParamBtn  (bDynIn,   "COMP SC",  "CyanBtn",  "fltDynSc");
        setupParamBtn  (bExpIn,   "GATE ON",  "RedBtn",   "expIn");
        setupParamBtn  (bCmpIn,   "COMP ON",  "AmberBtn", "cmpIn");

        // Filter Slope selector button (cycles 6, 12, 18, 24 dB/oct with 4 LEDs)
        setupActionBtn (bSlope,   "dB/OCT",   "AmberBtn", [this] { toggleChoice ("filterSlope"); });

        // EQ Bell buttons & Black/Brown curve button
        setupParamBtn  (bHfBell,  "BELL",     "RedBtn",   "hfBell");
        setupActionBtn (bEqSel,   "BLK/BRN",  "AmberBtn", [this] { toggleChoice ("eqMode"); });
        setupParamBtn  (bLfBell,  "BELL",     "AmberBtn", "lfBell");
        setupParamBtn  (bEqIn,    "EQ ON",    "CyanBtn",  "eqIn");
        setupParamBtn  (bEqSc,    "EQ SC",    "RedBtn",   "eqDynSc");
        setupParamBtn  (bEqPre,   "PRE DYN",  "AmberBtn", "eqPreDyn");

        // SSL-format De-Esser Split Band button
        setupParamBtn  (bDeEss,   "DE-ESS",   "AmberBtn", "deEss");

        // OUTPUT Inflate CLIP 0 dB button
        setupParamBtn  (bClip0,   "CLIP 0",   "RedBtn",   "inflateClip");

        // General Dual L/R Meter IN / OUT Selector (IN = Pre-HEAT Input, OUT = Final Post-Fader Output)
        setupActionBtn (bInOutMeter, "MTR OUT", "CyanBtn", [this]
        {
            showPreHeatInputMeter = !showPreHeatInputMeter;
            bInOutMeter.setToggleState (showPreHeatInputMeter, dontSendNotification);
            bInOutMeter.setButtonText (showPreHeatInputMeter ? "MTR IN" : "MTR OUT");
        });
        bInOutMeter.setToggleState (false, dontSendNotification);

        // dBFS Peak vs -18 dBFS RMS (0 VU Calibrated) Scale Selector Button
        setupActionBtn (bMeterScale, "PEAK dB", "AmberBtn", [this]
        {
            useCalibratedRms18 = !useCalibratedRms18;
            bMeterScale.setToggleState (useCalibratedRms18, dontSendNotification);
            bMeterScale.setButtonText (useCalibratedRms18 ? "-18 VU" : "PEAK dB");
        });
        bMeterScale.setToggleState (false, dontSendNotification);

        backdrop = createFLHeatBackdrop (kBaseW, kBaseH);
        layoutCanvasControls();
        syncDynamicStates();

        // Always open in compact / small window size by default!
        setResizable (true, true);
        constexpr float initScale = 0.72f;
        setResizeLimits (roundToInt (kBaseW * 0.55f), roundToInt (kBaseH * 0.55f),
                         roundToInt (kBaseW * 1.35f), roundToInt (kBaseH * 1.35f));
        if (auto* c = getConstrainer())
            c->setFixedAspectRatio (static_cast<double> (kBaseW) / static_cast<double> (kBaseH));

        setSize (roundToInt (kBaseW * initScale), roundToInt (kBaseH * initScale));
        startTimerHz (30);
    }

    ~SurcoEditor() override
    {
        stopTimer();
        setLookAndFeel (nullptr);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colour (0xff121417));
    }

    void resized() override
    {
        const float scaleX = static_cast<float> (getWidth())  / static_cast<float> (kBaseW);
        const float scaleY = static_cast<float> (getHeight()) / static_cast<float> (kBaseH);
        const float scale  = std::min (scaleX, scaleY);
        const int offX = roundToInt ((getWidth()  - kBaseW * scale) * 0.5f);
        const int offY = roundToInt ((getHeight() - kBaseH * scale) * 0.5f);
        canvas.setBounds (0, 0, kBaseW, kBaseH);
        canvas.setTransform (AffineTransform::scale (scale).translated (static_cast<float> (offX), static_cast<float> (offY)));
    }

    void setCalibratedRms18ForSnapshot (bool enabled)
    {
        useCalibratedRms18 = enabled;
        bMeterScale.setToggleState (enabled, dontSendNotification);
        bMeterScale.setButtonText (enabled ? "-18 VU" : "PEAK dB");
    }

    void renderSnapshotToFile (const File& pngFile)
    {
        syncDynamicStates();
        canvas.repaint();
        auto snap = canvas.createComponentSnapshot (canvas.getLocalBounds(), true, 2.0f);
        if (snap.isValid())
        {
            pngFile.deleteFile();
            FileOutputStream fos (pngFile);
            if (fos.openedOk())
            {
                PNGImageFormat fmt;
                fmt.writeImageToStream (snap, fos);
            }
        }
    }

private:
    class CanvasComponent final : public Component
    {
    public:
        explicit CanvasComponent (SurcoEditor& o) : owner (o) {}
        void paint (Graphics& g) override { owner.paintCanvas (g); }
        void mouseDoubleClick (const MouseEvent&) override
        {
            owner.toggleChoice ("oversampling");
            owner.syncDynamicStates();
            repaint();
        }
    private:
        SurcoEditor& owner;
    };

    void setupKnob (ConsoleDial& d, const String& paramId, const String& colorId)
    {
        d.setComponentID (colorId);
        canvas.addAndMakeVisible (d);
        attachments.push_back (std::make_unique<State::SliderAttachment> (state, paramId, d));

        if (auto* p = dynamic_cast<AudioParameterFloat*> (state.getParameter (paramId)))
        {
            d.textFromValueFunction = [this, p, paramId] (double v)
            {
                const auto unit = p->getLabel();
                const bool isShelfGain = (paramId == "lowGain"  && !bLfBell.getToggleState())
                                      || (paramId == "highGain" && !bHfBell.getToggleState());
                const bool isSteppedBell = (paramId == "lowMidGain" || paramId == "highMidGain"
                                         || paramId == "midGain"    || paramId == "sideGain")
                                        || (paramId == "lowGain"  && bLfBell.getToggleState())
                                        || (paramId == "highGain" && bHfBell.getToggleState());
                const int decimals = isSteppedBell ? 0 : (isShelfGain ? 2 : (unit == "Hz" ? 0 : 1));
                const double shownVal = isSteppedBell ? std::round (v) : v;
                auto text = String (shownVal, decimals);
                if (unit == "dB" && shownVal > 0.0001) text = "+" + text;
                return text + (unit.isEmpty() ? String() : (" " + unit));
            };
            d.updateText();
        }
    }

    void setupParamBtn (TextButton& b, const String& text, const String& cid, const String& paramId)
    {
        b.setButtonText (text);
        b.setComponentID (cid);
        b.setClickingTogglesState (true);
        canvas.addAndMakeVisible (b);
        btnAttachments.push_back (std::make_unique<State::ButtonAttachment> (state, paramId, b));
    }

    void setupActionBtn (TextButton& b, const String& text, const String& cid, std::function<void()> fn)
    {
        b.setButtonText (text);
        b.setComponentID (cid);
        b.onClick = std::move (fn);
        canvas.addAndMakeVisible (b);
    }

    void toggleChoice (const String& paramId)
    {
        if (auto* p = dynamic_cast<AudioParameterChoice*> (state.getParameter (paramId)))
        {
            const int next = (p->getIndex() + 1) % p->choices.size();
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (next)));
            p->endChangeGesture();
        }
    }

    void layoutCanvasControls()
    {
        constexpr int knobSz = 60;
        constexpr int eqKnob = 48;
        // Every single button in the plugin uses the exact same hardware dimensions!
        constexpr int btnW   = 52;
        constexpr int btnH   = 22;

        // =====================================================================
        // COLUMN 1 (x = 0..192):
        // - INPUT GAIN (-24..+48 dB before Input Meter) + FL HEAT (DRIVE & TONE)
        // - COMPRESSOR (Separated card: y = 198..422)
        // - GATE / EXPAND (Separated card: y = 427..712)
        // - FILTERS: HP Hz, LP Hz + 1 BUTTON & 4 LEDs FOR dB/OCT (y = 717..915)
        // =====================================================================
        kInputGain.setBounds (18, 28, 56, 56);
        bPhase.setBounds     (124, 24, btnW, btnH);
        bPad.setBounds       (124, 52, btnW, btnH);
        bHeatByp.setBounds   (124, 80, btnW, btnH);

        kHeatDrive.setBounds (24, 114, 54, 54);
        kHeatTone.setBounds  (96, 114, 54, 54);

        // COMPRESSOR Card (y = 198..422)
        kCompRat.setBounds (18, 224, knobSz, knobSz);
        bLink.setBounds    (118, 234, btnW, btnH);
        kCompThr.setBounds (110, 276, knobSz, knobSz);
        kCompRel.setBounds (18, 324, knobSz, knobSz);

        // GATE / EXPAND Card (y = 427..712)
        kExpThr.setBounds  (110, 448, 56, 56);
        kExpRng.setBounds  (18,  458, 56, 56);
        bExpSel.setBounds  (116, 520, btnW, btnH);
        kExpRel.setBounds  (18,  556, 56, 56);

        // Dynamics section enable buttons (3 identical buttons)
        bDynIn.setBounds (12,  658, btnW, btnH);
        bExpIn.setBounds (70,  658, btnW, btnH);
        bCmpIn.setBounds (128, 658, btnW, btnH);

        // Filters: HP Hz & LP Hz (2 knobs) + 1 Button & 4 LEDs (6, 12, 18, 24 dB/oct)
        kHpf.setBounds   (22, 748, knobSz, knobSz);
        kLpf.setBounds   (108, 748, knobSz, knobSz);
        bSlope.setBounds (70, 844, btnW, btnH);

        // =====================================================================
        // COLUMN 2 (x = 192..384):
        // - EQ HF Card (y = 5..123)
        // - EQ MID (HMF & LMF) Card (y = 128..378)
        // - EQ LF Card (y = 383..561)
        // - DE-ESSER Card (y = 566..740)
        // - MID / SIDE dB Card (y = 745..915)
        // =====================================================================
        // EQ HF Card
        kHfGain.setBounds  (212, 30, eqKnob, eqKnob);
        kHfFreq.setBounds  (316, 30, eqKnob, eqKnob);
        bHfBell.setBounds  (262, 92, btnW, btnH);

        // EQ MID Card (HMF row + LMF row + BLK/BRN button)
        kMidGain.setBounds (200, 152, eqKnob, eqKnob);
        kMidQ.setBounds    (264, 152, eqKnob, eqKnob);
        kMidFreq.setBounds (328, 152, eqKnob, eqKnob);
        bEqSel.setBounds   (262, 224, btnW, btnH);

        kLmfGain.setBounds (200, 272, eqKnob, eqKnob);
        kLmfQ.setBounds    (264, 272, eqKnob, eqKnob);
        kLmfFreq.setBounds (328, 272, eqKnob, eqKnob);

        // EQ LF Card (LF row + LF BELL + EQ routing buttons)
        kLfGain.setBounds  (212, 408, eqKnob, eqKnob);
        kLfFreq.setBounds  (316, 408, eqKnob, eqKnob);
        bLfBell.setBounds  (262, 470, btnW, btnH);

        bEqIn.setBounds    (204, 514, btnW, btnH);
        bEqSc.setBounds    (262, 514, btnW, btnH);
        bEqPre.setBounds   (320, 514, btnW, btnH);

        // DE-ESS Section (y = 566..740): 2 Knobs (FREQ & THRESH) + 1 SSL Button
        kDsFreq.setBounds  (214, 592, 52, 52);
        kDsThr.setBounds   (302, 592, 52, 52);
        bDeEss.setBounds   (262, 696, btnW, btnH);

        // MID & SIDE dB (from OUTPUT) placed right below De-Esser (y = 745..915)!
        kMidDb.setBounds   (214, 782, 56, 56);
        kSideDb.setBounds  (300, 782, 56, 56);

        // =====================================================================
        // COLUMN 3 (x = 384..540):
        // - INFLATE (TOP OF COLUMN 3, ABOVE MASTER FADER): 2 KNOBS (INFLATE & CURVE) + CLIP 0 dB
        // - MASTER OUT FADER + DUAL L/R GENERAL LED METER
        // - CHANNELSTRIP + PAPIELGUAPO SIGNATURE (BELOW THE SLIDER!)
        // =====================================================================
        kInfEff.setBounds      (400, 30, 52, 52);
        kInfCrv.setBounds      (472, 30, 52, 52);
        bClip0.setBounds       (436, 120, btnW, btnH);

        // General Dual L/R Meter Controls (identical size!) + Long Master Out Fader
        bInOutMeter.setBounds  (402, 192, btnW, btnH);
        bMeterScale.setBounds  (468, 192, btnW, btnH);
        fader.setBounds        (466, 232, 44, 528);
    }

    void syncDynamicStates()
    {
        const int eqCurv = roundToInt (state.getRawParameterValue ("eqMode")->load());
        bEqSel.setToggleState (eqCurv == 1, dontSendNotification);

        const int gMode = roundToInt (state.getRawParameterValue ("gateMode")->load());
        bExpSel.setToggleState (gMode == 1, dontSendNotification);
        bExpSel.setButtonText (gMode == 1 ? "EXPAND" : "GATE");

        // Enforce 1 dB steps when HF or LF is in BELL mode, continuous when in SHELF mode
        kHfGain.setRange (-18.0, 18.0, bHfBell.getToggleState() ? 1.0 : 0.01);
        kLfGain.setRange (-18.0, 18.0, bLfBell.getToggleState() ? 1.0 : 0.01);

        const float rawPeakL = telemetry.inPeakL.load (std::memory_order_relaxed);
        const float rawPeakR = telemetry.inPeakR.load (std::memory_order_relaxed);
        const float outPeakL = telemetry.outPeakL.load (std::memory_order_relaxed);
        const float outPeakR = telemetry.outPeakR.load (std::memory_order_relaxed);

        const float rawRmsL  = telemetry.inRmsL.load (std::memory_order_relaxed);
        const float rawRmsR  = telemetry.inRmsR.load (std::memory_order_relaxed);
        const float outRmsL  = telemetry.outRmsL.load (std::memory_order_relaxed);
        const float outRmsR  = telemetry.outRmsR.load (std::memory_order_relaxed);

        const float selL = showPreHeatInputMeter
                         ? (useCalibratedRms18 ? rawRmsL : rawPeakL)
                         : (useCalibratedRms18 ? outRmsL : outPeakL);
        const float selR = showPreHeatInputMeter
                         ? (useCalibratedRms18 ? rawRmsR : rawPeakR)
                         : (useCalibratedRms18 ? outRmsR : outPeakR);
        const float selMax = std::max (selL, selR);

        // In -18 dBFS RMS calibrated mode, add +18 dB offset so -18 dBFS RMS reads 0.0 VU!
        const float calOffsetDb = useCalibratedRms18 ? 18.0f : 0.0f;
        const float targetL  = Decibels::gainToDecibels (selL, -96.0f) + calOffsetDb;
        const float targetR  = Decibels::gainToDecibels (selR, -96.0f) + calOffsetDb;
        const float targetM  = Decibels::gainToDecibels (selMax, -96.0f) + calOffsetDb;

        meterDbL = std::max (targetL, meterDbL - 1.4f);
        meterDbR = std::max (targetR, meterDbR - 1.4f);
        meterMasterDb = std::max (targetM, meterMasterDb - 1.4f);
    }

    void saveUiScreenshotIfNeeded()
    {
        if (screenshotSaved)
            return;
        auto outDir = File ("C:\\Users\\anton\\Desktop\\EstripPapielguapo\\docs");
        if (!outDir.exists())
            outDir.createDirectory();
        auto snap = canvas.createComponentSnapshot (canvas.getLocalBounds(), true, 2.0f);
        File pngFile = outDir.getChildFile ("ChannelStrip_FL_Heat_UI.png");
        if (snap.isValid())
        {
            FileOutputStream fos (pngFile);
            if (fos.openedOk())
            {
                PNGImageFormat fmt;
                fmt.writeImageToStream (snap, fos);
                screenshotSaved = true;
            }
        }
    }

    void timerCallback() override
    {
        syncDynamicStates();
        canvas.repaint();
        if (++frameCounter == 3)
            saveUiScreenshotIfNeeded();
    }

    void drawDialRingAndLabels (Graphics& g, const Slider& s,
                                std::initializer_list<const char*> /*marks*/,
                                const String& bottomLabel = {},
                                const String& topLabel = {})
    {
        const auto b = s.getBounds().toFloat();
        const float cx = b.getCentreX();
        const float cy = b.getCentreY();
        const float r  = std::min (b.getWidth(), b.getHeight()) * 0.37f;

        if (topLabel.isNotEmpty())
        {
            g.setColour (kFLTextBright);
            g.setFont (getFLFont (11.0f, true));
            g.drawText (topLabel, Rectangle<float> (b.getX() - 10.0f, b.getY() - 13.0f, 68.0f, 13.0f),
                        Justification::centredLeft, false);
        }

        if (bottomLabel.isNotEmpty())
        {
            g.setColour (kFLTextBright);
            g.setFont (getFLFont (10.0f, true));
            g.drawText (bottomLabel, Rectangle<float> (cx - 42.0f, cy + r + 5.5f, 84.0f, 22.0f),
                        Justification::centredTop, false);
        }
    }

    void paintCanvas (Graphics& g)
    {
        if (backdrop.isValid())
            g.drawImageAt (backdrop, 0, 0);

        const Colour silkCol = kFLTextBright;

        // =====================================================================
        // COLUMN 1: INPUT GAIN (+48 dB) + FL HEAT (DRIVE & TONE), COMPRESSOR, GATE, FILTERS
        // =====================================================================
        const float hDrive = state.getRawParameterValue ("heatDrive")->load();
        const bool  hByp   = bHeatByp.getToggleState();
        const Colour heatLedCol = (hDrive < -0.05f) ? kFLAccentCyan : kFLAccentOrange;
        drawIndicatorLed (g, 84.0f, 34.0f, 7.5f, kFLAccentCyan, std::abs (state.getRawParameterValue ("preamp")->load()) > 0.05f);
        // HEAT BYP LED placed on the left side of the HEAT BYP button so the group stays centered!
        drawIndicatorLed (g, 110.0f, 91.0f, 7.5f, heatLedCol, !hByp);

        drawDialRingAndLabels (g, kInputGain, {}, "INPUT +48 dB");
        drawDialRingAndLabels (g, kHeatDrive, {}, "HEAT DRIVE");
        drawDialRingAndLabels (g, kHeatTone,  {}, "HEAT TONE");

        g.setColour (silkCol);
        g.setFont (getFLFont (8.8f, true));
        g.drawText ("OVER", 132, 174, 38, 12, Justification::centredRight);
        drawIndicatorLed (g, 178.0f, 180.0f, 7.2f, kLedRed,
                          telemetry.inputOverload.load (std::memory_order_relaxed));

        // COMPRESSOR Card
        drawIndicatorLed (g, 142.0f, 222.0f, 7.5f, kLedAmber, bLink.getToggleState());

        drawDialRingAndLabels (g, kCompRat, {}, "RATIO");
        drawDialRingAndLabels (g, kCompThr, {}, "THRESHOLD");
        drawDialRingAndLabels (g, kCompRel, {}, "RELEASE");

        // Fixed Ultra-Fast Attack indicator (FAST ATK) for Compressor
        drawIndicatorLed (g, 92.0f, 396.0f, 6.8f, kLedRed, true);
        g.setColour (silkCol);
        g.setFont (getFLFont (8.2f, true));
        g.drawText ("FAST ATK", 102, 390, 62, 12, Justification::centredLeft);

        // GATE / EXPANDER Card
        drawDialRingAndLabels (g, kExpThr, {}, "THRESHOLD");
        drawDialRingAndLabels (g, kExpRng, {}, "RANGE");
        drawDialRingAndLabels (g, kExpRel, {}, "RELEASE");

        // Fixed Ultra-Fast Attack indicator (FAST ATK) for Gate
        drawIndicatorLed (g, 16.0f, 636.0f, 6.8f, kLedRed, true);
        g.setColour (silkCol);
        g.setFont (getFLFont (8.2f, true));
        g.drawText ("FAST ATK", 26, 630, 62, 12, Justification::centredLeft);

        // EXP / GATE LEDs
        const int gMode = roundToInt (state.getRawParameterValue ("gateMode")->load());
        g.setColour (silkCol);
        g.setFont (getFLFont (8.5f, true));
        g.drawText ("EXP", 112, 544, 28, 11, Justification::centred);
        g.drawText ("GATE", 142, 544, 32, 11, Justification::centred);
        drawIndicatorLed (g, 126.0f, 560.0f, 7.0f, kLedAmber, gMode == 1);
        drawIndicatorLed (g, 158.0f, 560.0f, 7.0f, kLedAmber, gMode == 0);

        // Dual 5-LED Gain Reduction & Gate Attenuation Ladders (3, 6, 10, 14, 20 dB)
        const float cmpGr = telemetry.compReductionDb.load (std::memory_order_relaxed);
        const float expGr = telemetry.gateAttenuationDb.load (std::memory_order_relaxed);
        constexpr float grSteps[5] = { 20.0f, 14.0f, 10.0f, 6.0f, 3.0f };
        for (int i = 0; i < 5; ++i)
        {
            const float ly = 576.0f + i * 16.5f;
            drawIndicatorLed (g, 120.0f, ly, 7.0f, kLedEmerald, expGr >= grSteps[i]);
            g.setColour (silkCol);
            g.setFont (getFLFont (8.4f, true));
            g.drawText (String (roundToInt (grSteps[i])), 128, roundToInt (ly - 5.0f), 22, 10, Justification::centred);
            drawIndicatorLed (g, 158.0f, ly, 7.0f, i == 0 ? kLedRed : kLedAmber, cmpGr >= grSteps[i]);
        }

        drawIndicatorLed (g, 38.0f,  692.0f, 7.5f, kFLAccentCyan, bDynIn.getToggleState());
        drawIndicatorLed (g, 96.0f,  692.0f, 7.5f, kLedRed,       bExpIn.getToggleState());
        drawIndicatorLed (g, 154.0f, 692.0f, 7.5f, kLedAmber,     bCmpIn.getToggleState());

        // FILTERS section: 2 Knobs (HP Hz & LP Hz) + 1 Button + 4 Slope LEDs (6, 12, 18, 24 dB/oct)
        drawDialRingAndLabels (g, kHpf, {}, "HP Hz");
        drawCurveGlyph (g, 20.0f, 822.0f, 2, silkCol);
        drawDialRingAndLabels (g, kLpf, {}, "LP kHz");
        drawCurveGlyph (g, 172.0f, 822.0f, 3, silkCol);

        // 4 LEDs for Filter Slope (6, 12, 18, 24 dB/oct)
        const int slopeIdx = roundToInt (state.getRawParameterValue ("filterSlope")->load());
        constexpr const char* slopeNames[4] = { "6 dB", "12 dB", "18 dB", "24 dB" };
        for (int i = 0; i < 4; ++i)
        {
            const float lx = 34.0f + i * 41.0f;
            drawIndicatorLed (g, lx, 882.0f, 7.5f, i >= 2 ? kLedAmber : kFLAccentCyan, slopeIdx == i);
            g.setColour (silkCol);
            g.setFont (getFLFont (8.8f, true));
            g.drawText (slopeNames[i], roundToInt (lx - 18.0f), 892, 36, 12, Justification::centred);
        }

        // =====================================================================
        // COLUMN 2: SEPARATED EQ HF, EQ MID (HMF & LMF), EQ LF + DE-ESSER + MID/SIDE
        // =====================================================================
        // EQ HF Card
        drawDialRingAndLabels (g, kHfGain,  {}, "HF dB");
        drawDialRingAndLabels (g, kHfFreq,  {}, "HF kHz");
        drawIndicatorLed (g, 248.0f, 103.0f, 7.0f, kLedRed, bHfBell.getToggleState());

        // EQ MID Card: HMF row (High-Mid) + BLK/BRN + LMF row (Low-Mid)
        drawDialRingAndLabels (g, kMidGain, {}, "HMF dB");
        drawDialRingAndLabels (g, kMidQ,    {}, "BELL Q");
        drawCurveGlyph (g, 267.0f, 210.0f, 1, silkCol);
        drawCurveGlyph (g, 309.0f, 210.0f, 0, silkCol);
        drawDialRingAndLabels (g, kMidFreq, {}, "HMF kHz");

        const bool isBrown = bEqSel.getToggleState();
        drawIndicatorLed (g, 246.0f, 235.0f, 7.0f, kLedAmber, !isBrown);
        drawIndicatorLed (g, 330.0f, 235.0f, 7.0f, kLedAmber, isBrown);
        g.setColour (silkCol);
        g.setFont (getFLFont (8.4f, true));
        g.drawText ("BLK", 224, 242, 30, 11, Justification::centred);
        g.drawText ("BRN", 318, 242, 30, 11, Justification::centred);

        drawDialRingAndLabels (g, kLmfGain, {}, "LMF dB");
        drawDialRingAndLabels (g, kLmfQ,    {}, "BELL Q");
        drawCurveGlyph (g, 267.0f, 330.0f, 1, silkCol);
        drawCurveGlyph (g, 309.0f, 330.0f, 0, silkCol);
        drawDialRingAndLabels (g, kLmfFreq, {}, "LMF Hz");

        // EQ LF Card
        drawDialRingAndLabels (g, kLfGain,  {}, "LF dB");
        drawDialRingAndLabels (g, kLfFreq,  {}, "LF Hz");
        drawIndicatorLed (g, 248.0f, 481.0f, 7.0f, kLedAmber, bLfBell.getToggleState());

        drawIndicatorLed (g, 230.0f, 546.0f, 7.5f, kFLAccentCyan, bEqIn.getToggleState());
        drawIndicatorLed (g, 288.0f, 546.0f, 7.5f, kLedRed,       bEqSc.getToggleState());
        drawIndicatorLed (g, 346.0f, 546.0f, 7.5f, kLedAmber,     bEqPre.getToggleState());

        // DE-ESS Section (2 Knobs: FREQ & THRESH + SSL format SPLIT BAND button & GR LED)
        drawDialRingAndLabels (g, kDsFreq, {}, "FREQ Hz");
        drawDialRingAndLabels (g, kDsThr,  {}, "THRESH dB");
        const float dsGr = telemetry.deEssReductionDb.load (std::memory_order_relaxed);
        drawIndicatorLed (g, 288.0f, 682.0f, 7.8f,
                          dsGr > 0.2f ? kLedRed : kFLAccentCyan, bDeEss.getToggleState());

        // MID / SIDE dB Section (from OUTPUT) placed right below De-Esser!
        drawDialRingAndLabels (g, kMidDb,  {}, "MID dB");
        drawDialRingAndLabels (g, kSideDb, {}, "SIDE dB");

        // =====================================================================
        // COLUMN 3: INFLATE (ABOVE FADER), MASTER FADER + DUAL L/R GENERAL METER,
        //           AND CHANNELSTRIP + PAPIELGUAPO SIGNATURE BELOW THE SLIDER!
        // =====================================================================
        // INFLATE Section (ABOVE MASTER FADER: 2 Knobs INFLATE & CURVE + CLIP 0 dB Button)
        drawDialRingAndLabels (g, kInfEff, {}, "INFLATE %");
        drawDialRingAndLabels (g, kInfCrv, {}, "CURVE");
        drawIndicatorLed (g, 420.0f, 131.0f, 7.5f, kLedRed, bClip0.getToggleState());

        // =====================================================================
        // DUAL L / R GENERAL LED METER BESIDE THE MASTER FADER
        // =====================================================================
        g.setColour (showPreHeatInputMeter ? kFLAccentCyan : kFLAccentOrange);
        g.setFont (getFLFont (9.5f, true));
        g.drawText ("L", 418, 227, 14, 12, Justification::centred);
        g.drawText ("R", 436, 227, 14, 12, Justification::centred);

        constexpr int kNumMeterRows = 12;
        constexpr const char* mLabelsPeak[kNumMeterRows] = {
            "OL", "0", "-3", "-6", "-9", "-12", "-15", "-18", "-24", "-30", "-36", "-48"
        };
        constexpr float mThreshPeak[kNumMeterRows] = {
            -0.1f, -0.5f, -3.0f, -6.0f, -9.0f, -12.0f, -15.0f, -18.0f, -24.0f, -30.0f, -36.0f, -48.0f
        };

        constexpr const char* mLabelsVu[kNumMeterRows] = {
            "+9", "+6", "+3", "0VU", "-2", "-4", "-6", "-9", "-12", "-15", "-18", "-24"
        };
        constexpr float mThreshVu[kNumMeterRows] = {
             9.0f,  6.0f,  3.0f,  0.0f, -2.0f, -4.0f, -6.0f, -9.0f, -12.0f, -15.0f, -18.0f, -24.0f
        };

        for (int i = 0; i < kNumMeterRows; ++i)
        {
            const float my = 254.0f + i * 34.0f;
            const char* lbl = useCalibratedRms18 ? mLabelsVu[i] : mLabelsPeak[i];
            const float thr = useCalibratedRms18 ? mThreshVu[i] : mThreshPeak[i];
            const bool isZeroVuRow = (useCalibratedRms18 && i == 3);

            g.setColour (isZeroVuRow ? kFLAccentOrange : kFLTextBright);
            g.setFont (getFLFont (8.8f, true));
            g.drawText (lbl, 386, roundToInt (my - 5.5f), 28, 11, Justification::centredRight);

            const Colour ledCol = (i == 0) ? kLedRed
                                : (i <= 2) ? kLedAmber
                                : (showPreHeatInputMeter ? kFLAccentCyan : kLedEmerald);

            // Dual L and R LED columns!
            drawIndicatorLed (g, 425.0f, my, 8.2f, ledCol, meterDbL >= thr);
            drawIndicatorLed (g, 443.0f, my, 8.2f, ledCol, meterDbR >= thr);
        }

        // Dual L / R Numeric Readout Box at bottom of General Meter
        {
            const Rectangle<float> digBox (390.0f, 676.0f, 68.0f, 36.0f);
            g.setColour (Colour (0xff121417));
            g.fillRoundedRectangle (digBox, 3.5f);
            g.setColour (useCalibratedRms18 ? kFLAccentOrange.withAlpha (0.75f) : Colour (0xff2c3136));
            g.drawRoundedRectangle (digBox, 3.5f, 1.0f);

            auto fmtDb = [this] (float v) -> String
            {
                if (v <= -70.0f) return "-inf";
                return (v > 0.05f ? "+" : "") + String (v, 1);
            };

            g.setColour (useCalibratedRms18 ? kFLAccentOrange : kFLTextBright);
            g.setFont (getFLFont (9.0f, true));
            g.drawText ("L: " + fmtDb (meterDbL) + (useCalibratedRms18 ? " VU" : " dB"),
                        Rectangle<float> (digBox.getX() + 3.0f, digBox.getY() + 2.0f, digBox.getWidth() - 6.0f, 15.0f),
                        Justification::centredLeft, false);
            g.drawText ("R: " + fmtDb (meterDbR) + (useCalibratedRms18 ? " VU" : " dB"),
                        Rectangle<float> (digBox.getX() + 3.0f, digBox.getY() + 18.0f, digBox.getWidth() - 6.0f, 15.0f),
                        Justification::centredLeft, false);

            g.setColour (kFLTextMuted);
            g.setFont (getFLFont (8.2f, true));
            g.drawText (showPreHeatInputMeter ? "PRE-HEAT IN" : "POST-FADER OUT",
                        Rectangle<float> (388.0f, 716.0f, 72.0f, 12.0f), Justification::centred, false);
        }

        // Console Fader dB Scale (10, 5, 0, 5, 10, 20, 30, 40, 50, inf)
        struct FaderMark { float db; const char* text; };
        constexpr FaderMark fMarks[] = {
            {  10.0f, "10" }, {   5.0f, "5"  }, {   0.0f, "0"  },
            {  -5.0f, "5"  }, { -10.0f, "10" }, { -20.0f, "20" },
            { -30.0f, "30" }, { -40.0f, "40" }, { -50.0f, "50" },
            { -80.0f, "\xe2\x88\x9e" }
        };
        const float fTop = static_cast<float> (fader.getY()) + 18.0f;
        const float fBot = static_cast<float> (fader.getBottom()) - 18.0f;
        g.setColour (kFLTextBright);
        g.setFont (getFLFont (10.5f, true));
        for (const auto& fm : fMarks)
        {
            const float prop = static_cast<float> (fader.valueToProportionOfLength (fm.db));
            const float fy   = fBot - prop * (fBot - fTop);
            g.drawText (String::fromUTF8 (fm.text), 510, roundToInt (fy - 6.0f), 26, 12, Justification::centredLeft);
        }

        g.setColour (kFLTextBright);
        g.setFont (getFLFont (10.2f, true));
        g.drawText ("MASTER OUT", 448, 764, 86, 14, Justification::centred);

        // CHANNELSTRIP + PAPIELGUAPO SIGNATURE Card BELOW the Master Out slider! (Double-click changes Oversampling)
        const int osIdx = (state.getRawParameterValue ("oversampling") != nullptr)
                        ? roundToInt (state.getRawParameterValue ("oversampling")->load()) : 0;
        constexpr const char* osNames[4] = { "OS: 1x", "OS: 2x", "OS: 4x", "OS: 8x" };
        g.setColour (kFLTextBright);
        g.setFont (getFLFont (16.5f, true));
        g.drawText ("CHANNELSTRIP", 388, 798, 148, 22, Justification::centred);
        drawPapielguapoSignature (g, Rectangle<float> (400.0f, 824.0f, 124.0f, 56.0f), kFLTextBright);
        g.setColour (osIdx > 0 ? kFLAccentCyan : kFLTextMuted);
        g.setFont (getFLFont (9.2f, true));
        g.drawText (String (osNames[std::clamp (osIdx, 0, 3)]) + " (2x CLICK)", 388, 886, 148, 14, Justification::centred);
    }

    AudioProcessor& proc;
    State& state;
    SurcoTelemetry& telemetry;
    FLHeatLookAndFeel lnf;
    Image backdrop;
    CanvasComponent canvas { *this };

    ConsoleDial kInputGain, kHeatDrive, kHeatTone;
    ConsoleDial kCompRat, kCompThr, kCompRel;
    ConsoleDial kExpThr, kExpRng, kExpRel;
    ConsoleDial kHpf, kLpf;
    ConsoleDial kHfGain, kHfFreq;
    ConsoleDial kMidGain, kMidQ, kMidFreq;
    ConsoleDial kLmfGain, kLmfQ, kLmfFreq;
    ConsoleDial kLfGain, kLfFreq;
    ConsoleDial kDsFreq, kDsThr;
    ConsoleDial kInfEff, kInfCrv;
    ConsoleDial kMidDb, kSideDb;
    Slider fader;

    TextButton bHeatByp, bPhase, bPad;
    TextButton bLink, bExpSel, bDynIn, bExpIn, bCmpIn;
    TextButton bSlope;
    TextButton bHfBell, bEqSel, bLfBell, bEqIn, bEqSc, bEqPre;
    TextButton bDeEss;
    TextButton bClip0, bInOutMeter, bMeterScale;

    bool showPreHeatInputMeter = false;
    bool useCalibratedRms18 = false;
    bool screenshotSaved = false;
    int frameCounter = 0;
    float meterDbL = -96.0f;
    float meterDbR = -96.0f;
    float meterMasterDb = -96.0f;

    std::vector<std::unique_ptr<State::SliderAttachment>> attachments;
    std::vector<std::unique_ptr<State::ButtonAttachment>> btnAttachments;
    std::unique_ptr<State::SliderAttachment> faderAtt;
};
}

AudioProcessorEditor* makeEditor (AudioProcessor& p, State& s)
{
    return new SurcoEditor (p, s);
}

void renderSurcoEditorSnapshot (const File& outPng)
{
    extern AudioProcessor* JUCE_CALLTYPE createPluginFilter();
    std::unique_ptr<AudioProcessor> proc (createPluginFilter());
    proc->prepareToPlay (48000.0, 256);
    for (auto* p : proc->getParameters())
    {
        const auto nm = p->getName (128);
        if (nm == "Input trim")      p->setValueNotifyingHost (0.45f);
        if (nm == "Heat Drive")      p->setValueNotifyingHost (0.72f);
        if (nm == "Heat Tone")       p->setValueNotifyingHost (0.62f);
        if (nm == "Inflate Effect")  p->setValueNotifyingHost (0.58f);
        if (nm == "Inflate Curve")   p->setValueNotifyingHost (0.65f);
        if (nm == "High shelf gain") p->setValueNotifyingHost (0.68f);
        if (nm == "High mid gain")   p->setValueNotifyingHost (0.61f);
        if (nm == "Low mid gain")    p->setValueNotifyingHost (0.39f);
        if (nm == "Low shelf gain")  p->setValueNotifyingHost (0.58f);
        if (nm == "De-Ess")          p->setValueNotifyingHost (1.0f);
        if (nm == "Clip 0 dB")       p->setValueNotifyingHost (1.0f);
        if (nm == "Mid Gain")        p->setValueNotifyingHost (0.60f);
        if (nm == "Side Gain")       p->setValueNotifyingHost (0.58f);
    }
    AudioBuffer<float> buf (2, 256);
    MidiBuffer midi;
    for (int i = 0; i < 256; ++i)
    {
        buf.setSample (0, i, 0.45f * std::sin (i * 0.25f));
        buf.setSample (1, i, 0.38f * std::cos (i * 0.25f));
    }
    proc->processBlock (buf, midi);
    std::unique_ptr<AudioProcessorEditor> ed (proc->createEditorIfNeeded());
    if (auto* se = dynamic_cast<SurcoEditor*> (ed.get()))
    {
        se->setCalibratedRms18ForSnapshot (true);
        se->renderSnapshotToFile (outPng);
    }
}
