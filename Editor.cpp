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
    if (type == 2) // High-pass slope
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
    g.strokePath (p, PathStrokeType (1.3f));
}

void drawStereoMiniMeter (Graphics& g, Rectangle<float> area, float dbL, float dbR, bool isInputMode, bool isVuCalibrated) noexcept
{
    g.setColour (Colour (0xff121417));
    g.fillRoundedRectangle (area, 3.0f);
    g.setColour (Colour (0xff2c3136));
    g.drawRoundedRectangle (area, 3.0f, 0.9f);

    const int numSegs = 12;
    const float padTop = 14.0f;
    const float padBot = 14.0f;
    const float barTop = area.getY() + padTop;
    const float barH   = area.getHeight() - padTop - padBot;
    const float colW   = (area.getWidth() - 10.0f) * 0.5f;
    const float xL     = area.getX() + 3.5f;
    const float xR     = xL + colW + 3.0f;

    g.setColour (isVuCalibrated ? kFLAccentOrange : (isInputMode ? kFLAccentCyan : kFLAccentGreen));
    g.setFont (getFLFont (7.2f, true));
    const String topTag = String (isInputMode ? "IN" : "OUT") + (isVuCalibrated ? " VU" : " PK");
    g.drawText (topTag, Rectangle<float> (area.getX(), area.getY() + 1.5f, area.getWidth(), 10.0f),
                Justification::centred, false);

    // In -18 dBFS RMS (VU) mode, 0 VU is at 75% height (-18 dBFS -> 0 VU, +6 VU = -12 dBFS at top)
    auto dbToNorm = [isVuCalibrated] (float db) noexcept -> float
    {
        if (isVuCalibrated)
            return std::clamp ((db + 30.0f) / 36.0f, 0.0f, 1.0f); // db is already in VU (dBFS + 18)
        return std::clamp ((db + 48.0f) / 48.0f, 0.0f, 1.0f);
    };

    const float gap  = 1.6f;
    const float segH = (barH - (numSegs - 1) * gap) / static_cast<float> (numSegs);
    const float nL   = dbToNorm (dbL);
    const float nR   = dbToNorm (dbR);

    auto drawCol = [&] (float bx, float normVal)
    {
        for (int i = 0; i < numSegs; ++i)
        {
            const float segNorm = static_cast<float> (i + 1) / static_cast<float> (numSegs);
            const int invIdx    = numSegs - 1 - i;
            const float sy      = barTop + invIdx * (segH + gap);

            Colour onCol  = isInputMode ? kFLAccentCyan : kFLAccentGreen;
            Colour offCol = Colour (0xff1a1f22);
            if (invIdx == 0)      { onCol = kFLAccentRed;    offCol = Colour (0xff2c1618); }
            else if (invIdx <= 2) { onCol = kFLAccentOrange; offCol = Colour (0xff2c2216); }

            const bool lit = (normVal >= segNorm - 0.03f);
            g.setColour (lit ? onCol : offCol);
            g.fillRect (bx, sy, colW, segH);
        }
    };

    drawCol (xL, nL);
    drawCol (xR, nR);

    // Draw 0 VU (-18 dBFS sweet-spot target line) when in calibrated RMS mode!
    if (isVuCalibrated)
    {
        const float zeroVuY = barTop + (1.0f - dbToNorm (0.0f)) * barH;
        g.setColour (Colours::white.withAlpha (0.85f));
        g.drawHorizontalLine (roundToInt (zeroVuY), area.getX() + 1.5f, area.getRight() - 1.5f);
    }

    g.setColour (kFLTextMuted);
    g.setFont (getFLFont (7.5f, true));
    g.drawText ("L", Rectangle<float> (xL - 1.0f, area.getBottom() - 12.0f, colW + 2.0f, 10.0f), Justification::centred, false);
    g.drawText ("R", Rectangle<float> (xR - 1.0f, area.getBottom() - 12.0f, colW + 2.0f, 10.0f), Justification::centred, false);
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
            Rectangle<float> hdr (r.getX() + 1.0f, r.getY() + 1.0f, r.getWidth() - 2.0f, 17.0f);
            g.setColour (Colour (0xff16181b));
            g.fillRoundedRectangle (hdr, 4.0f);
            g.setColour (dotCol);
            g.fillEllipse (hdr.getX() + 6.0f, hdr.getCentreY() - 2.5f, 5.0f, 5.0f);
            g.setColour (kFLTextBright);
            g.setFont (getFLFont (9.0f, true));
            g.drawText (title, hdr.withTrimmedLeft (15.0f), Justification::centredLeft, false);
        }
    };

    // Column 1: INPUT + FL HEAT (Drive & Tone), Dynamics & Gate, Filters
    drawFLCard ({ 5.0f, 5.0f, col1W - 10.0f, 188.0f }, "INPUT GAIN & FL HEAT", kFLAccentOrange);
    drawFLCard ({ 5.0f, 198.0f, col1W - 10.0f, 514.0f }, "DYNAMICS & GATE (FAST ATK)", kFLAccentCyan);
    drawFLCard ({ 5.0f, 717.0f, col1W - 10.0f, 198.0f }, "FILTERS (HP / LP + dB/OCT)", kFLAccentCyan);

    // Column 2: 6-Knob EQ (HF, MID, LF + 2 Bell buttons), De-Esser (Split Band)
    drawFLCard ({ col1W + 5.0f, 5.0f, col2W - 10.0f, 710.0f }, "EQ (HF / MID / LF)", kFLAccentOrange);
    drawFLCard ({ col1W + 5.0f, 720.0f, col2W - 10.0f, 195.0f }, "VOCALS DE-ESSER (SPLIT)", kFLAccentRed);

    // Column 3: Header (ChannelStrip + papielguapo signature), INFLATE (above Master Fader!), MID/SIDE, MASTER FADER & OUTPUT + L/R METERS
    drawFLCard ({ col3X + 5.0f, 5.0f, col3W - 10.0f, 118.0f }, "", kFLAccentOrange);
    drawFLCard ({ col3X + 5.0f, 128.0f, col3W - 10.0f, 134.0f }, "INFLATE & CLIP 0 dB", kFLAccentOrange);
    drawFLCard ({ col3X + 5.0f, 267.0f, col3W - 10.0f, 102.0f }, "MID / SIDE dB (OUTPUT)", kFLAccentCyan);
    drawFLCard ({ col3X + 5.0f, 374.0f, col3W - 10.0f, 408.0f }, "MASTER FADER & METER", kFLAccentGreen);
    drawFLCard ({ col3X + 5.0f, 787.0f, col3W - 10.0f, 128.0f }, "OUTPUT TRIM & L/R METER", kFLAccentOrange);

    g.setColour (Colour (0xff0b0d10));
    g.drawRect (0, 0, w, h, 1);
    return img;
}

class FLHeatLookAndFeel final : public LookAndFeel_V4
{
public:
    FLHeatLookAndFeel()
    {
        setColour (Slider::textBoxTextColourId, Colours::white);
        setColour (Slider::textBoxBackgroundColourId, Colour (0xff14171a));
        setColour (Slider::textBoxOutlineColourId, kFLAccentOrange);
    }

    Slider::SliderLayout getSliderLayout (Slider& slider) override
    {
        auto layout = LookAndFeel_V4::getSliderLayout (slider);
        if (slider.isRotary())
        {
            const auto b = slider.getLocalBounds();
            layout.sliderBounds = b;
            layout.textBoxBounds = Rectangle<int> (2, b.getCentreY() - 9, b.getWidth() - 4, 18);
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
            l->setFont (getFLFont (10.0f, true));
            l->setInterceptsMouseClicks (false, false);
        }
        return l;
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
        const float capH = 48.0f;
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
        g.setFont (getFLFont (std::min (8.8f, b.getHeight() * 0.44f), true));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (2, 1), Justification::centred, 2);
    }
};

// =========================================================================
// Functional Rotary Knob: Drag to adjust, Click/Double-Click to type value,
// Value Box is completely hidden unless actively typing!
// =========================================================================
class ConsoleDial final : public Slider
{
public:
    ConsoleDial()
    {
        setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle (Slider::TextBoxBelow, false, 52, 18);
        setDoubleClickReturnValue (false, 0.0);
        setWantsKeyboardFocus (true);
        setPopupDisplayEnabled (true, true, nullptr);
    }

    void mouseDown (const MouseEvent& e) override
    {
        dragMoved = false;
        if (e.mods.isRightButtonDown())
        {
            showTextBox();
            return;
        }
        Slider::mouseDown (e);
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (e.getDistanceFromDragStart() > 2)
            dragMoved = true;
        Slider::mouseDrag (e);
    }

    void mouseUp (const MouseEvent& e) override
    {
        Slider::mouseUp (e);
        if (!dragMoved && !e.mods.isRightButtonDown() && e.getNumberOfClicks() >= 1)
            showTextBox();
    }

    void mouseDoubleClick (const MouseEvent&) override
    {
        showTextBox();
    }

private:
    bool dragMoved = false;
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
        canvas.setInterceptsMouseClicks (false, true);

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

        // 5. EQ (6 KNOBS: HF dB & Hz, MID dB & Hz, LF dB & Hz + 2 BELL BUTTONS)
        setupKnob (kHfGain,    "highGain",      "SSLRed");
        setupKnob (kHfFreq,    "highFreq",      "SSLRed");

        setupKnob (kMidGain,   "highMidGain",   "SSLGreen");
        setupKnob (kMidFreq,   "highMidFreq",   "SSLGreen");

        setupKnob (kLfGain,    "lowGain",       "SSLBrown");
        setupKnob (kLfFreq,    "lowFreq",       "SSLBrown");

        // 6. DE-ESSER: 2 KNOBS (FREQ & THRESH) + SSL SPLIT BAND BUTTON
        setupKnob (kDsFreq,    "dsFreq",        "SSLSilver");
        setupKnob (kDsThr,     "dsThresh",      "SSLRed");

        // 7. INFLATE (ABOVE MASTER FADER!): 2 KNOBS (INFLATE & CURVE) + CLIP 0 dB BUTTON
        setupKnob (kInfEff,    "inflateEffect", "SSLRed");
        setupKnob (kInfCrv,    "inflateCurve",  "SSLSilver");

        // 8. MID / SIDE dB (from OUTPUT)
        setupKnob (kMidDb,     "midGain",       "SSLBlue");
        setupKnob (kSideDb,    "sideGain",      "SSLBlue");

        // 9. OUTPUT TRIM & MASTER FADER
        setupKnob (kOutput,    "compMakeup",    "SSLSilver");

        fader.setSliderStyle (Slider::LinearVertical);
        fader.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        fader.setDoubleClickReturnValue (true, 0.0);
        fader.setPopupDisplayEnabled (true, true, &canvas);
        canvas.addAndMakeVisible (fader);
        faderAtt = std::make_unique<State::SliderAttachment> (state, "fader", fader);

        // FL HEAT section buttons
        setupParamBtn  (bHeatByp, "HEAT BYPASS",   "RedBtn",   "heatBypass");
        setupParamBtn  (bPhase,   "FASE 180°",     "RedBtn",   "phaseInv");
        setupParamBtn  (bPad,     "PAD -20 dB",    "AmberBtn", "pad20");

        // Dynamics buttons
        setupParamBtn  (bLink,    "EXT SIDECHAIN", "AmberBtn", "extSidechain");
        setupActionBtn (bExpSel,  "MODO: GATE",    "CyanBtn",  [this] { toggleChoice ("gateMode"); });
        setupParamBtn  (bDynIn,   "DYN\nON",       "CyanBtn",  "dynIn");
        setupParamBtn  (bExpIn,   "GATE\nON",      "RedBtn",   "expIn");
        setupParamBtn  (bCmpIn,   "COMP\nON",      "AmberBtn", "cmpIn");

        // Filter Slope selector button (cycles 6, 12, 18, 24 dB/oct with 4 LEDs)
        setupActionBtn (bSlope,   "PENDIENTE dB/OCT", "AmberBtn", [this] { toggleChoice ("filterSlope"); });

        // EQ Bell buttons & Black/Brown curve button
        setupParamBtn  (bHfBell,  "HF CAMPANA",    "RedBtn",   "hfBell");
        setupActionBtn (bEqSel,   "CURVA BLK/BRN", "AmberBtn", [this] { toggleChoice ("eqMode"); });
        setupParamBtn  (bLfBell,  "LF CAMPANA",    "AmberBtn", "lfBell");
        setupParamBtn  (bEqIn,    "EQ\nON",        "CyanBtn",  "eqIn");
        setupParamBtn  (bEqSc,    "EQ A\nSIDECHAIN","RedBtn",  "eqDynSc");
        setupParamBtn  (bEqPre,   "EQ ANTES\nCOMP","AmberBtn", "eqPreDyn");

        // SSL-format De-Esser Split Band button
        setupParamBtn  (bDeEss,   "DE-ESS\nSPLIT BAND", "AmberBtn", "deEss");

        // OUTPUT Inflate CLIP 0 dB button
        setupParamBtn  (bClip0,   "CLIP 0 dB",     "RedBtn",   "inflateClip");

        // IN / OUT Meter Selector Button (switches between Pre-HEAT Input and Final Output)
        setupActionBtn (bInOutMeter, "METER: OUT", "CyanBtn", [this]
        {
            showPreHeatInputMeter = !showPreHeatInputMeter;
            bInOutMeter.setToggleState (showPreHeatInputMeter, dontSendNotification);
            bInOutMeter.setButtonText (showPreHeatInputMeter ? "METER: IN" : "METER: OUT");
        });
        bInOutMeter.setToggleState (false, dontSendNotification);

        // dBFS Peak vs -18 dBFS RMS (0 VU Calibrated) Scale Selector Button
        setupActionBtn (bMeterScale, "dBFS PEAK", "AmberBtn", [this]
        {
            useCalibratedRms18 = !useCalibratedRms18;
            bMeterScale.setToggleState (useCalibratedRms18, dontSendNotification);
            bMeterScale.setButtonText (useCalibratedRms18 ? "-18 RMS VU" : "dBFS PEAK");
        });
        bMeterScale.setToggleState (false, dontSendNotification);

        backdrop = createFLHeatBackdrop (kBaseW, kBaseH);
        layoutCanvasControls();
        syncDynamicStates();

        setResizable (true, true);
        const auto area = Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
        const float initScale = (area.getHeight() < 980) ? 0.85f : 1.0f;
        setResizeLimits (roundToInt (kBaseW * 0.65f), roundToInt (kBaseH * 0.65f),
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
        bMeterScale.setButtonText (enabled ? "-18 RMS VU" : "dBFS PEAK");
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
        constexpr int knobSz = 62;
        constexpr int smKnob = 48;

        // =====================================================================
        // COLUMN 1 (x = 0..192):
        // - INPUT GAIN (-24..+48 dB before Input Meter) + FL HEAT (DRIVE & TONE)
        // - DYNAMICS: RATIO, THRESHOLD, RELEASE
        // - GATE: THRESHOLD, RANGE EXPAND, RELEASE
        // - FILTERS: HP Hz, LP Hz + 1 BUTTON & 4 LEDs FOR dB/OCT (6, 12, 18, 24)
        // =====================================================================
        kInputGain.setBounds (18, 28, 56, 56);
        bPhase.setBounds     (104, 28, 78, 22);
        bPad.setBounds       (104, 56, 78, 22);

        kHeatDrive.setBounds (14, 108, 54, 54);
        kHeatTone.setBounds  (74, 108, 54, 54);
        bHeatByp.setBounds   (134, 118, 48, 32);

        // Dynamics (Comp): Ratio, Threshold, Release
        kCompRat.setBounds (20, 224, knobSz, knobSz);
        bLink.setBounds    (102, 232, 80, 22);
        kCompThr.setBounds (108, 282, knobSz, knobSz);
        kCompRel.setBounds (20, 332, knobSz, knobSz);

        // Gate / Expand: Threshold, Range Expand, Release
        kExpThr.setBounds  (108, 400, knobSz, knobSz);
        kExpRng.setBounds  (20, 470, knobSz, knobSz);
        bExpSel.setBounds  (102, 488, 80, 22);
        kExpRel.setBounds  (20, 568, knobSz, knobSz);

        // Dynamics section enable buttons
        bDynIn.setBounds (22,  652, 44, 34);
        bExpIn.setBounds (74,  652, 44, 34);
        bCmpIn.setBounds (126, 652, 44, 34);

        // Filters: HP Hz & LP Hz (2 knobs) + 1 Button & 4 LEDs (6, 12, 18, 24 dB/oct)
        kHpf.setBounds   (22, 748, knobSz, knobSz);
        kLpf.setBounds   (108, 748, knobSz, knobSz);
        bSlope.setBounds (36, 844, 120, 24);

        // =====================================================================
        // COLUMN 2 (x = 192..384):
        // - EQ 6 KNOBS: HF (dB, Hz), MID (dB, Hz), LF (dB, Hz)
        // - 2 BELL BUTTONS (HF CAMPANA, LF CAMPANA) + CURVA BLK/BRN
        // - DE-ESS: 2 KNOBS (FREQ, THRESH) + 1 SSL SPLIT BAND BUTTON
        // =====================================================================
        // HF Band (2 knobs: Gain dB & Freq Hz + 1 Bell button)
        kHfGain.setBounds (208, 36, knobSz, knobSz);
        bHfBell.setBounds (288, 46, 82, 24);
        kHfFreq.setBounds (294, 102, knobSz, knobSz);

        // MID Band (2 knobs: Gain dB & Freq Hz + Black/Brown Curve button)
        kMidGain.setBounds (208, 228, knobSz, knobSz);
        bEqSel.setBounds   (286, 244, 86, 24);
        kMidFreq.setBounds (294, 304, knobSz, knobSz);

        // LF Band (2 knobs: Gain dB & Freq Hz + 1 Bell button)
        kLfFreq.setBounds  (294, 448, knobSz, knobSz);
        kLfGain.setBounds  (208, 504, knobSz, knobSz);
        bLfBell.setBounds  (288, 540, 82, 24);

        // Equaliser routing buttons
        bEqIn.setBounds    (206, 646, 52, 34);
        bEqSc.setBounds    (262, 646, 52, 34);
        bEqPre.setBounds   (318, 646, 52, 34);

        // DE-ESS Section: 2 Knobs (FREQ & THRESH) + 1 SSL Format SPLIT BAND Button
        kDsFreq.setBounds  (214, 756, knobSz, knobSz);
        kDsThr.setBounds   (298, 756, knobSz, knobSz);
        bDeEss.setBounds   (234, 858, 108, 34);

        // =====================================================================
        // COLUMN 3 (x = 384..540):
        // - HEADER: CHANNELSTRIP + PAPIELGUAPO SIGNATURE (FROM OUTPUT)
        // - INFLATE (ABOVE MASTER FADER!): 2 KNOBS (INFLATE & CURVE) + CLIP 0 dB
        // - MID & SIDE dB (2 KNOBS FROM OUTPUT)
        // - MASTER FADER & IN/OUT LED METER
        // - OUTPUT TRIM KNOB + L/R STEREO METER BARS BESIDE IT
        // =====================================================================
        kInfEff.setBounds      (402, 152, smKnob, smKnob);
        kInfCrv.setBounds      (476, 152, smKnob, smKnob);
        bClip0.setBounds       (422, 230, 82, 22);

        kMidDb.setBounds       (402, 290, smKnob, smKnob);
        kSideDb.setBounds      (476, 290, smKnob, smKnob);

        bInOutMeter.setBounds  (390, 398, 58, 22);
        bMeterScale.setBounds  (390, 424, 58, 22);
        fader.setBounds        (454, 404, 46, 360);

        // OUTPUT Trim on the left side of bottom card, Stereo L/R Meter on the right side
        kOutput.setBounds      (400, 814, knobSz, knobSz);
    }

    void syncDynamicStates()
    {
        const int eqCurv = roundToInt (state.getRawParameterValue ("eqMode")->load());
        bEqSel.setToggleState (eqCurv == 1, dontSendNotification);

        const int gMode = roundToInt (state.getRawParameterValue ("gateMode")->load());
        bExpSel.setToggleState (gMode == 1, dontSendNotification);
        bExpSel.setButtonText (gMode == 1 ? "MODO: EXPAND" : "MODO: GATE");

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
            g.setFont (getFLFont (10.0f, true));
            g.drawText (topLabel, Rectangle<float> (b.getX() - 12.0f, b.getY() - 13.0f, 64.0f, 12.0f),
                        Justification::centredLeft, false);
        }

        if (bottomLabel.isNotEmpty())
        {
            g.setColour (kFLTextBright);
            g.setFont (getFLFont (8.2f, true));
            g.drawText (bottomLabel, Rectangle<float> (cx - 40.0f, cy + r + 8.0f, 80.0f, 18.0f),
                        Justification::centredTop, false);
        }
    }

    void paintCanvas (Graphics& g)
    {
        if (backdrop.isValid())
            g.drawImageAt (backdrop, 0, 0);

        const Colour silkCol = kFLTextBright;

        // =====================================================================
        // COLUMN 1: INPUT GAIN (+48 dB) + FL HEAT (DRIVE & TONE), DYNAMICS, GATE, FILTERS
        // =====================================================================
        const float hDrive = state.getRawParameterValue ("heatDrive")->load();
        const bool  hByp   = bHeatByp.getToggleState();
        const Colour heatLedCol = (hDrive < -0.05f) ? kFLAccentCyan : kFLAccentOrange;
        drawIndicatorLed (g, 84.0f, 34.0f, 7.5f, kFLAccentCyan, std::abs (state.getRawParameterValue ("preamp")->load()) > 0.05f);
        drawIndicatorLed (g, 158.0f, 106.0f, 7.5f, heatLedCol, !hByp && std::abs (hDrive) > 0.05f);

        drawDialRingAndLabels (g, kInputGain, {}, "INPUT +48 dB", "INPUT");
        drawDialRingAndLabels (g, kHeatDrive, {}, "HEAT DRIVE");
        drawDialRingAndLabels (g, kHeatTone,  {}, "HEAT TONE");

        g.setColour (silkCol);
        g.setFont (getFLFont (7.8f, true));
        g.drawText ("OVER", 136, 174, 34, 11, Justification::centredRight);
        drawIndicatorLed (g, 178.0f, 179.0f, 7.2f, kLedRed,
                          telemetry.inputOverload.load (std::memory_order_relaxed));

        // DYNAMICS (Compressor)
        drawIndicatorLed (g, 142.0f, 220.0f, 7.5f, kLedAmber, bLink.getToggleState());

        drawDialRingAndLabels (g, kCompRat, {}, "RATIO\nCOMPRESS");
        drawDialRingAndLabels (g, kCompThr, {}, "THRESHOLD");
        drawDialRingAndLabels (g, kCompRel, {}, "RELEASE");

        // Fixed Ultra-Fast Attack indicator (FAST ATK) for Compressor
        drawIndicatorLed (g, 14.0f, 404.0f, 6.5f, kLedRed, true);
        g.setColour (silkCol);
        g.setFont (getFLFont (7.2f, true));
        g.drawFittedText ("FAST\nATK", 2, 412, 26, 18, Justification::centred, 2);

        // GATE / EXPANDER
        drawDialRingAndLabels (g, kExpThr, {}, "THRESHOLD");
        drawDialRingAndLabels (g, kExpRng, {}, "RANGE\nEXPAND");
        drawDialRingAndLabels (g, kExpRel, {}, "RELEASE");

        // Fixed Ultra-Fast Attack indicator (FAST ATK) for Gate
        drawIndicatorLed (g, 14.0f, 634.0f, 6.5f, kLedRed, true);
        g.setColour (silkCol);
        g.setFont (getFLFont (7.2f, true));
        g.drawFittedText ("FAST\nATK", 2, 642, 26, 18, Justification::centred, 2);

        // EXP / GATE LEDs
        const int gMode = roundToInt (state.getRawParameterValue ("gateMode")->load());
        g.setColour (silkCol);
        g.setFont (getFLFont (7.8f, true));
        g.drawText ("EXP", 112, 514, 26, 10, Justification::centred);
        g.drawText ("GATE", 142, 514, 30, 10, Justification::centred);
        drawIndicatorLed (g, 125.0f, 530.0f, 7.0f, kLedAmber, gMode == 1);
        drawIndicatorLed (g, 157.0f, 530.0f, 7.0f, kLedAmber, gMode == 0);

        // Dual 5-LED Gain Reduction & Gate Attenuation Ladders (3, 6, 10, 14, 20 dB)
        const float cmpGr = telemetry.compReductionDb.load (std::memory_order_relaxed);
        const float expGr = telemetry.gateAttenuationDb.load (std::memory_order_relaxed);
        constexpr float grSteps[5] = { 20.0f, 14.0f, 10.0f, 6.0f, 3.0f };
        for (int i = 0; i < 5; ++i)
        {
            const float ly = 548.0f + i * 19.0f;
            drawIndicatorLed (g, 120.0f, ly, 7.2f, kLedEmerald, expGr >= grSteps[i]);
            g.setColour (silkCol);
            g.setFont (getFLFont (7.8f, true));
            g.drawText (String (roundToInt (grSteps[i])), 128, roundToInt (ly - 5.0f), 22, 10, Justification::centred);
            drawIndicatorLed (g, 158.0f, ly, 7.2f, i == 0 ? kLedRed : kLedAmber, cmpGr >= grSteps[i]);
        }

        drawIndicatorLed (g, 44.0f,  696.0f, 7.5f, kLedEmerald, bDynIn.getToggleState());
        drawIndicatorLed (g, 96.0f,  696.0f, 7.5f, kLedRed,     bExpIn.getToggleState());
        drawIndicatorLed (g, 148.0f, 696.0f, 7.5f, kLedAmber,   bCmpIn.getToggleState());

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
            drawIndicatorLed (g, lx, 882.0f, 7.5f, i >= 2 ? kLedAmber : kLedEmerald, slopeIdx == i);
            g.setColour (silkCol);
            g.setFont (getFLFont (7.8f, true));
            g.drawText (slopeNames[i], roundToInt (lx - 18.0f), 892, 36, 11, Justification::centred);
        }

        // =====================================================================
        // COLUMN 2: EQ 6 KNOBS (HF, MID, LF) + 2 BELL BUTTONS + DE-ESS
        // =====================================================================
        drawDialRingAndLabels (g, kHfGain, {}, "HF dB", "HF");
        drawIndicatorLed (g, 329.0f, 34.0f, 7.5f, kLedRed, bHfBell.getToggleState());
        drawDialRingAndLabels (g, kHfFreq, {}, "HF kHz");

        drawDialRingAndLabels (g, kMidGain, {}, "MID dB", "MID");
        const bool isBrown = bEqSel.getToggleState();
        drawIndicatorLed (g, 314.0f, 280.0f, 7.5f, kLedAmber, !isBrown);
        drawIndicatorLed (g, 344.0f, 280.0f, 7.5f, kLedAmber, isBrown);
        g.setColour (silkCol);
        g.setFont (getFLFont (8.0f, true));
        g.drawText ("BLK   BRN", 294, 289, 70, 10, Justification::centred);
        drawDialRingAndLabels (g, kMidFreq, {}, "MID kHz");

        drawDialRingAndLabels (g, kLfFreq, {}, "LF Hz");
        drawDialRingAndLabels (g, kLfGain, {}, "LF dB", "LF");
        drawIndicatorLed (g, 329.0f, 528.0f, 7.5f, kLedAmber, bLfBell.getToggleState());

        // Equaliser bottom switches & LEDs
        drawIndicatorLed (g, 232.0f, 692.0f, 7.5f, kLedEmerald, bEqIn.getToggleState());
        drawIndicatorLed (g, 288.0f, 692.0f, 7.5f, kLedRed,     bEqSc.getToggleState());
        drawIndicatorLed (g, 344.0f, 692.0f, 7.5f, kLedAmber,   bEqPre.getToggleState());

        // DE-ESS Section (2 Knobs: FREQ & THRESH + SSL format SPLIT BAND button & GR LED)
        drawDialRingAndLabels (g, kDsFreq, {}, "FREQ Hz");
        drawDialRingAndLabels (g, kDsThr,  {}, "THRESH dB");

        const float dsGr = telemetry.deEssReductionDb.load (std::memory_order_relaxed);
        drawIndicatorLed (g, 288.0f, 844.0f, 8.0f,
                          dsGr > 0.2f ? kLedRed : kLedEmerald, bDeEss.getToggleState());

        // =====================================================================
        // COLUMN 3: HEADER (CHANNELSTRIP + PAPIELGUAPO SIGNATURE),
        //           INFLATE (ABOVE FADER), MID/SIDE, MASTER FADER & OUTPUT + L/R METER
        // =====================================================================
        g.setColour (kFLTextBright);
        g.setFont (getFLFont (16.5f, true));
        g.drawText ("CHANNELSTRIP", 388, 12, 148, 22, Justification::centred);

        {
            const Rectangle<float> heatBadge (438.0f, 36.0f, 48.0f, 16.0f);
            g.setColour (Colour (0xff141619));
            g.fillRoundedRectangle (heatBadge, 3.0f);
            g.setColour (kFLAccentOrange.withAlpha (0.7f));
            g.drawRoundedRectangle (heatBadge, 3.0f, 1.0f);
            g.setColour (kFLAccentOrange);
            g.setFont (getFLFont (9.0f, true));
            g.drawText ("FL HEAT", heatBadge, Justification::centred);
        }

        // Exact papielguapo signature from OUTPUT
        drawPapielguapoSignature (g, Rectangle<float> (404.0f, 60.0f, 116.0f, 48.0f), kFLTextBright);

        // INFLATE Section (ABOVE MASTER FADER: 2 Knobs INFLATE & CURVE + CLIP 0 dB Button)
        drawDialRingAndLabels (g, kInfEff, {}, "INFLATE %");
        drawDialRingAndLabels (g, kInfCrv, {}, "CURVE");
        drawIndicatorLed (g, 410.0f, 241.0f, 7.5f, kLedRed, bClip0.getToggleState());

        // MID / SIDE dB Section (from OUTPUT)
        drawDialRingAndLabels (g, kMidDb,  {}, "MID dB");
        drawDialRingAndLabels (g, kSideDb, {}, "SIDE dB");

        // MASTER FADER & IN/OUT 8-LED METER (dBFS Peak or -18 dBFS RMS = 0 VU Calibrated)
        constexpr const char* mLabelsPeak[8] = { "OL", "0", "-3", "-6", "-12", "-18", "-27", "-36" };
        constexpr float mThreshPeak[8]       = { -0.1f, -0.5f, -3.0f, -6.0f, -12.0f, -18.0f, -27.0f, -36.0f };

        constexpr const char* mLabelsVu[8]   = { "+6", "+3", "0VU", "-3", "-6", "-12", "-18", "-24" };
        constexpr float mThreshVu[8]         = {  6.0f,  3.0f,  0.0f, -3.0f, -6.0f, -12.0f, -18.0f, -24.0f };

        for (int i = 0; i < 8; ++i)
        {
            const float my = 468.0f + i * 22.0f;
            const char* lbl = useCalibratedRms18 ? mLabelsVu[i] : mLabelsPeak[i];
            const float thr = useCalibratedRms18 ? mThreshVu[i] : mThreshPeak[i];
            g.setColour ((useCalibratedRms18 && i == 2) ? kFLAccentOrange : kFLTextBright);
            g.setFont (getFLFont (7.8f, true));
            g.drawText (lbl, 388, roundToInt (my - 5.0f), 26, 10, Justification::centredRight);
            const Colour ledCol = (i == 0) ? kLedRed : (i <= 2 ? kLedAmber : (showPreHeatInputMeter ? kFLAccentCyan : kLedEmerald));
            drawIndicatorLed (g, 424.0f, my, 7.8f, ledCol, meterMasterDb >= thr);
        }

        // Numeric digital readout below LED ladder so user can nail -18 dBFS RMS (0.0 VU)
        {
            const Rectangle<float> digBox (390.0f, 646.0f, 54.0f, 18.0f);
            g.setColour (Colour (0xff121417));
            g.fillRoundedRectangle (digBox, 3.0f);
            g.setColour (useCalibratedRms18 ? kFLAccentOrange.withAlpha (0.7f) : Colour (0xff2c3136));
            g.drawRoundedRectangle (digBox, 3.0f, 0.9f);
            const String digTxt = (meterMasterDb <= -70.0f) ? "-inf"
                                : ((meterMasterDb > 0.05f ? "+" : "") + String (meterMasterDb, 1) + (useCalibratedRms18 ? " VU" : " dB"));
            g.setColour (useCalibratedRms18 ? kFLAccentOrange : kFLTextBright);
            g.setFont (getFLFont (8.2f, true));
            g.drawText (digTxt, digBox, Justification::centred, false);
        }

        // Console Fader dB Scale
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
        g.setFont (getFLFont (9.5f, true));
        for (const auto& fm : fMarks)
        {
            const float prop = static_cast<float> (fader.valueToProportionOfLength (fm.db));
            const float fy   = fBot - prop * (fBot - fTop);
            g.drawText (String::fromUTF8 (fm.text), 498, roundToInt (fy - 6.0f), 28, 12, Justification::centredLeft);
        }

        // Bottom OUTPUT knob section + Stereo L/R Meter beside it!
        drawDialRingAndLabels (g, kOutput, {}, "OUTPUT dB");
        drawStereoMiniMeter (g, Rectangle<float> (478.0f, 808.0f, 46.0f, 98.0f),
                             meterDbL, meterDbR, showPreHeatInputMeter, useCalibratedRms18);
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
    ConsoleDial kMidGain, kMidFreq;
    ConsoleDial kLfGain, kLfFreq;
    ConsoleDial kDsFreq, kDsThr;
    ConsoleDial kInfEff, kInfCrv;
    ConsoleDial kMidDb, kSideDb;
    ConsoleDial kOutput;
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
