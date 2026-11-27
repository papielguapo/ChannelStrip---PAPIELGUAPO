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
    // Recessed dark metal socket with outer shadow & chrome rim
    g.setColour (Colours::black.withAlpha (0.75f));
    g.fillEllipse (cx - r - 2.0f, cy - r - 1.2f, diameter + 4.0f, diameter + 4.0f);

    const Rectangle<float> bezel (cx - r - 1.5f, cy - r - 1.5f, diameter + 3.0f, diameter + 3.0f);
    ColourGradient rimGrad (Colour (0xff485058), cx, bezel.getY(),
                            Colour (0xff121518), cx, bezel.getBottom(), false);
    g.setGradientFill (rimGrad);
    g.fillEllipse (bezel);

    g.setColour (Colour (0xff0b0d10));
    g.fillEllipse (cx - r - 0.5f, cy - r - 0.5f, diameter + 1.0f, diameter + 1.0f);

    const Rectangle<float> bulb (cx - r, cy - r, diameter, diameter);
    if (lit)
    {
        // Multi-stage realistic bloom halo
        ColourGradient outerHalo (col.withAlpha (0.38f), cx, cy, col.withAlpha (0.0f), cx + r * 3.1f, cy + r * 3.1f, true);
        g.setGradientFill (outerHalo);
        g.fillEllipse (cx - r * 3.0f, cy - r * 3.0f, diameter * 3.0f, diameter * 3.0f);

        ColourGradient innerHalo (col.brighter (0.4f).withAlpha (0.65f), cx, cy, col.withAlpha (0.0f), cx + r * 1.7f, cy + r * 1.7f, true);
        g.setGradientFill (innerHalo);
        g.fillEllipse (cx - r * 1.7f, cy - r * 1.7f, diameter * 1.7f, diameter * 1.7f);

        // 3D glass dome bulb gradient
        ColourGradient bulbGrad (col.brighter (0.55f), cx - r * 0.25f, cy - r * 0.3f,
                                 col.darker (0.35f),   cx + r * 0.6f,  cy + r * 0.7f, true);
        g.setGradientFill (bulbGrad);
        g.fillEllipse (bulb);

        // Hot filament center & crisp specular reflection
        g.setColour (Colours::white.withAlpha (0.92f));
        g.fillEllipse (cx - r * 0.42f, cy - r * 0.48f, r * 0.76f, r * 0.58f);
    }
    else
    {
        // Smoked dark glass lens when unlit
        ColourGradient offGrad (col.darker (2.2f).withAlpha (0.55f), cx - r * 0.25f, cy - r * 0.3f,
                                Colour (0xff121519),                  cx + r * 0.7f,  cy + r * 0.7f, true);
        g.setGradientFill (offGrad);
        g.fillEllipse (bulb);
        g.setColour (Colours::white.withAlpha (0.22f));
        g.fillEllipse (cx - r * 0.40f, cy - r * 0.46f, r * 0.56f, r * 0.42f);
    }
}

void drawChassisScrew (Graphics& g, float cx, float cy, float r = 4.4f, float angleRad = 0.45f) noexcept
{
    // Recessed socket well
    g.setColour (Colours::black.withAlpha (0.85f));
    g.fillEllipse (cx - r - 1.2f, cy - r - 0.8f, (r + 1.2f) * 2.0f, (r + 1.2f) * 2.0f);
    g.setColour (Colour (0xff40464d).withAlpha (0.55f));
    g.drawEllipse (cx - r - 0.8f, cy - r - 0.8f, (r + 0.8f) * 2.0f, (r + 0.8f) * 2.0f, 0.7f);

    // Metallic round screw head with realistic radial sheen
    ColourGradient headGrad (Colour (0xff565d65), cx - r * 0.55f, cy - r * 0.55f,
                             Colour (0xff191c20), cx + r * 0.75f, cy + r * 0.75f, false);
    g.setGradientFill (headGrad);
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

    g.setColour (Colour (0xff0b0d10));
    g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 0.85f);

    // Single diagonal slotted flathead screw groove (exact 1:1 match to reference photo!)
    const float slotLen = r * 0.70f;
    const float c1 = std::cos (angleRad), s1 = std::sin (angleRad);
    g.setColour (Colour (0xff080a0c).withAlpha (0.95f));
    g.drawLine (cx - c1 * slotLen, cy - s1 * slotLen, cx + c1 * slotLen, cy + s1 * slotLen, 1.45f);
    g.setColour (Colours::white.withAlpha (0.22f));
    g.drawLine (cx - c1 * slotLen, cy - s1 * slotLen + 0.7f, cx + c1 * slotLen, cy + s1 * slotLen + 0.7f, 0.65f);
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

void drawFLHeatBackdropVector (Graphics& g, int w, int h)
{
    const float col1W = 192.0f;
    const float col2W = 192.0f;
    const float col3X = col1W + col2W;
    const float col3W = static_cast<float> (w) - col3X;

    // Deep anodized metal chassis base
    g.fillAll (Colour (0xff121518));

    int cardIdx = 0;
    auto drawFLCard = [&] (Rectangle<float> r, const String& title, Colour dotCol, bool withScrews = true)
    {
        ++cardIdx;
        // Deep outer drop shadow into chassis gap
        g.setColour (Colours::black.withAlpha (0.82f));
        g.fillRoundedRectangle (r.expanded (0.9f).translated (0.0f, 2.2f), 6.0f);

        // Brushed dark aluminum module surface gradient (exact 1:1 vertical brushed metal look)
        ColourGradient cardGrad (Colour (0xff2b2d32), r.getX(), r.getY(),
                                 Colour (0xff1c1e22), r.getX(), r.getBottom(), false);
        cardGrad.addColour (0.28, Colour (0xff2e3035));
        cardGrad.addColour (0.55, Colour (0xff26282d));
        cardGrad.addColour (0.85, Colour (0xff1f2125));
        g.setGradientFill (cardGrad);
        g.fillRoundedRectangle (r, 5.5f);

        // Subtle overhead studio spotlight sheen on each hardware module
        ColourGradient sheen (Colours::white.withAlpha (0.055f), r.getCentreX(), r.getY() + 24.0f,
                              Colours::transparentBlack, r.getCentreX(), r.getBottom(), true);
        g.setGradientFill (sheen);
        g.fillRoundedRectangle (r, 5.5f);

        // Fine vertical brushed anodized aluminum hairlines (exact 1:1 match to reference photo!)
        uint32 rng = 0x9E3779B9u ^ static_cast<uint32> (cardIdx * 7919);
        auto nextRand = [&]() noexcept -> float
        {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            return static_cast<float> (rng & 0xFFFFu) / 65535.0f;
        };
        const int yTopGrain = roundToInt (r.getY() + (title.isNotEmpty() ? 21.0f : 3.0f));
        const int yBotGrain = roundToInt (r.getBottom() - 3.0f);
        for (int lx = roundToInt (r.getX() + 3.0f); lx < roundToInt (r.getRight() - 3.0f); ++lx)
        {
            const float alpha = 0.012f + 0.028f * nextRand();
            g.setColour ((lx & 1) ? Colours::white.withAlpha (alpha) : Colours::black.withAlpha (alpha * 1.55f));
            g.drawVerticalLine (lx, static_cast<float> (yTopGrain), static_cast<float> (yBotGrain));
        }
        // Subtle horizontal cross-sheen bands for realistic machined metal depth
        for (int ly = yTopGrain; ly < yBotGrain; ly += 3)
        {
            const float alpha = 0.006f + 0.010f * nextRand();
            g.setColour ((ly & 1) ? Colours::white.withAlpha (alpha) : Colours::black.withAlpha (alpha));
            g.drawHorizontalLine (ly, r.getX() + 3.0f, r.getRight() - 3.0f);
        }

        if (title.isNotEmpty())
        {
            Rectangle<float> hdr (r.getX() + 1.0f, r.getY() + 1.0f, r.getWidth() - 2.0f, 20.0f);
            ColourGradient hdrGrad (Colour (0xff1a1d21), hdr.getX(), hdr.getY(),
                                    Colour (0xff111316), hdr.getX(), hdr.getBottom(), false);
            g.setGradientFill (hdrGrad);
            g.fillRoundedRectangle (hdr, 5.0f);

            // Engraved groove line below header
            g.setColour (Colours::black.withAlpha (0.78f));
            g.drawHorizontalLine (roundToInt (hdr.getBottom()), r.getX() + 2.0f, r.getRight() - 2.0f);
            g.setColour (Colours::white.withAlpha (0.06f));
            g.drawHorizontalLine (roundToInt (hdr.getBottom() + 1.0f), r.getX() + 2.0f, r.getRight() - 2.0f);

            // Glowing status dot in header
            g.setColour (dotCol.withAlpha (0.32f));
            g.fillEllipse (hdr.getX() + 4.8f, hdr.getCentreY() - 4.8f, 9.6f, 9.6f);
            g.setColour (dotCol);
            g.fillEllipse (hdr.getX() + 6.7f, hdr.getCentreY() - 2.9f, 5.8f, 5.8f);

            g.setColour (kFLTextBright);
            g.setFont (getFLFont (10.2f, true));
            g.drawText (title, hdr.withTrimmedLeft (16.0f), Justification::centredLeft, false);
        }

        // Machined 3D bevel frame: bright top specular edge, dark bottom/outer edge
        g.setColour (Colour (0xff090b0d));
        g.drawRoundedRectangle (r, 5.5f, 1.3f);
        g.setColour (Colours::white.withAlpha (0.11f));
        g.drawRoundedRectangle (r.reduced (1.0f), 4.8f, 0.85f);
        g.setColour (Colours::white.withAlpha (0.20f));
        g.drawHorizontalLine (roundToInt (r.getY() + 1.0f), r.getX() + 6.0f, r.getRight() - 6.0f);

        // Realistic corner mounting screws on hardware modules
        if (withScrews)
        {
            const float syTop = r.getY() + 10.5f;
            const float syBot = r.getBottom() - 9.5f;
            if (title.isNotEmpty())
                drawChassisScrew (g, r.getRight() - 10.5f, syTop, 4.1f, 0.35f + cardIdx * 0.4f);
            drawChassisScrew (g, r.getX() + 10.0f,     syBot, 4.2f, 1.10f + cardIdx * 0.5f);
            drawChassisScrew (g, r.getRight() - 10.0f, syBot, 4.2f, 2.05f + cardIdx * 0.3f);
        }
    };

    // =========================================================================
    // COLUMN 1: Separated COMPRESSOR and GATE / EXPAND cards!
    // =========================================================================
    drawFLCard ({ 5.0f,   5.0f, col1W - 10.0f, 188.0f }, "INPUT GAIN CONSOLE", kFLAccentOrange, true);
    drawFLCard ({ 5.0f, 198.0f, col1W - 10.0f, 224.0f }, "COMPRESSOR (FAST ATK)", kFLAccentCyan, true);
    drawFLCard ({ 5.0f, 427.0f, col1W - 10.0f, 285.0f }, "GATE / EXPAND (FAST ATK)", kFLAccentCyan, true);
    drawFLCard ({ 5.0f, 717.0f, col1W - 10.0f, 198.0f }, "FILTERS (HP / LP + dB/OCT)", kFLAccentCyan, true);

    // =========================================================================
    // COLUMN 2: Separated EQ HF, EQ MID (HMF & LMF), EQ LF, DE-ESSER, and MID/SIDE!
    // =========================================================================
    drawFLCard ({ col1W + 5.0f,   5.0f, col2W - 10.0f, 118.0f }, "EQ HF (HIGH)", kFLAccentRed, true);
    drawFLCard ({ col1W + 5.0f, 128.0f, col2W - 10.0f, 250.0f }, "EQ MID (HMF & LMF)", kFLAccentCyan, true);
    drawFLCard ({ col1W + 5.0f, 383.0f, col2W - 10.0f, 178.0f }, "EQ LF (LOW)", kFLAccentOrange, true);
    drawFLCard ({ col1W + 5.0f, 566.0f, col2W - 10.0f, 174.0f }, "VOCALS DE-ESSER (SPLIT)", kFLAccentRed, true);
    drawFLCard ({ col1W + 5.0f, 745.0f, col2W - 10.0f, 170.0f }, "MID / SIDE dB (OUTPUT)", kFLAccentCyan, true);

    // =========================================================================
    // COLUMN 3: INFLATE at top, MASTER FADER + DUAL L/R METER in middle,
    //           and CHANNELSTRIP + papielguapo signature BELOW the slider!
    // =========================================================================
    drawFLCard ({ col3X + 5.0f,   5.0f, col3W - 10.0f, 154.0f }, "INFLATE & CLIP 0 dB", kFLAccentOrange, true);
    drawFLCard ({ col3X + 5.0f, 164.0f, col3W - 10.0f, 624.0f }, "MASTER OUT & L/R METER", kFLAccentCyan, true);
    drawFLCard ({ col3X + 5.0f, 793.0f, col3W - 10.0f, 122.0f }, "", kFLAccentOrange, false);

    g.setColour (Colour (0xff0b0d10));
    g.drawRect (0, 0, w, h, 1);
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
            layout.sliderBounds = slider.getLocalBounds();
            layout.textBoxBounds = {};
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
        const bool isWidth = (cid == "WidthBipolar");
        const double neutralValue = isWidth ? 100.0 : 0.0;
        const bool isBipolar = isWidth || (minVal < -0.001 && maxVal > 0.001 && std::abs (minVal + maxVal) < 1.0);
        // Factory FL HEAT bipolar behavior: lights up from 12 o'clock as you move it (Cyan < 0, Yellow/Orange > 0)
        // Unipolar knobs use Yellow/Orange ("SSLYellow") or Blue/Cyan ("SSLBlue")!
        const bool isBlueKnob = (cid == "SSLBlue" || cid == "SSLGreen");
        const Colour accent = isBipolar
            ? ((slider.getValue() < neutralValue - 0.0001) ? kFLAccentCyan : kFLAccentOrange)
            : (isBlueKnob ? kFLAccentCyan : kFLAccentOrange);

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
        g.setColour (Colour (0xff0b0d10));
        g.strokePath (bgArc, PathStrokeType (trackW + 2.4f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colour (0xff181b1f));
        g.strokePath (bgArc, PathStrokeType (trackW + 0.8f, PathStrokeType::curved, PathStrokeType::rounded));

        const float startValAngle = isBipolar
            ? (rotaryStartAngle + 0.5f * (rotaryEndAngle - rotaryStartAngle))
            : rotaryStartAngle;
        if (std::abs (angle - startValAngle) > 0.015f)
        {
            Path valArc;
            valArc.addCentredArc (cx, cy, arcRad, arcRad, 0.0f,
                                  std::min (startValAngle, angle), std::max (startValAngle, angle), true);
            g.setColour (accent.withAlpha (0.18f));
            g.strokePath (valArc, PathStrokeType (trackW + 7.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (accent.withAlpha (0.38f));
            g.strokePath (valArc, PathStrokeType (trackW + 3.4f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (accent);
            g.strokePath (valArc, PathStrokeType (trackW, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (Colours::white.withAlpha (0.45f));
            g.strokePath (valArc, PathStrokeType (trackW * 0.35f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Deep drop shadow under 3D hardware knob
        g.setColour (Colours::black.withAlpha (0.76f));
        g.fillEllipse (cx - knobRad - 1.5f, cy - knobRad + 2.8f, (knobRad + 1.5f) * 2.0f, (knobRad + 1.5f) * 2.0f);

        // Outer dark skirt & knurled serrations (32 teeth around the knob skirt)
        const float skirtRad = knobRad + 2.2f;
        ColourGradient skirtGrad (Colour (0xff2b3036), cx, cy - skirtRad,
                                  Colour (0xff0d0f12), cx, cy + skirtRad, false);
        g.setGradientFill (skirtGrad);
        g.fillEllipse (cx - skirtRad, cy - skirtRad, skirtRad * 2.0f, skirtRad * 2.0f);

        constexpr int kNumKnurls = 28;
        for (int k = 0; k < kNumKnurls; ++k)
        {
            const float ka = static_cast<float> (k) * (MathConstants<float>::twoPi / static_cast<float> (kNumKnurls));
            const float kx1 = cx + std::cos (ka) * (knobRad * 0.90f);
            const float ky1 = cy + std::sin (ka) * (knobRad * 0.90f);
            const float kx2 = cx + std::cos (ka) * (skirtRad - 0.3f);
            const float ky2 = cy + std::sin (ka) * (skirtRad - 0.3f);
            g.setColour ((k & 1) ? Colours::white.withAlpha (0.14f) : Colours::black.withAlpha (0.65f));
            g.drawLine (kx1, ky1, kx2, ky2, 1.0f);
        }

        // Chamfered metallic silver/gunmetal bezel ring (like hardware SSL/console knobs)
        ColourGradient rimGrad (Colour (0xff8a939e), cx - knobRad * 0.5f, cy - knobRad,
                                Colour (0xff252a30), cx + knobRad * 0.5f, cy + knobRad, false);
        g.setGradientFill (rimGrad);
        g.fillEllipse (cx - knobRad, cy - knobRad, knobRad * 2.0f, knobRad * 2.0f);

        // Recessed brushed aluminum top cap with 4-lobe conical metallic sheen (exact 1:1 match to reference photo!)
        const float capRad = knobRad * 0.84f;
        ColourGradient capGrad (Colour (0xff3c4249), cx - capRad * 0.65f, cy - capRad * 0.65f,
                                Colour (0xff181b1f), cx + capRad * 0.65f, cy + capRad * 0.65f, false);
        capGrad.addColour (0.32, Colour (0xff2b3036));
        capGrad.addColour (0.50, Colour (0xff383e45));
        capGrad.addColour (0.72, Colour (0xff1d2025));
        g.setGradientFill (capGrad);
        g.fillEllipse (cx - capRad, cy - capRad, capRad * 2.0f, capRad * 2.0f);

        // Anisotropic conical reflection lobes across the turned-aluminum cap
        {
            Path sheenLobe1, sheenLobe2;
            sheenLobe1.addPieSegment (cx - capRad, cy - capRad, capRad * 2.0f, capRad * 2.0f,
                                      -0.85f, -0.25f, 0.12f);
            sheenLobe2.addPieSegment (cx - capRad, cy - capRad, capRad * 2.0f, capRad * 2.0f,
                                      MathConstants<float>::pi - 0.85f, MathConstants<float>::pi - 0.25f, 0.12f);
            g.setColour (Colours::white.withAlpha (0.09f));
            g.fillPath (sheenLobe1);
            g.fillPath (sheenLobe2);
        }

        // Concentric micro-turned lathe rings on the aluminum cap
        for (float rr = capRad * 0.32f; rr < capRad * 0.92f; rr += 2.2f)
        {
            g.setColour (Colours::white.withAlpha (0.035f));
            g.drawEllipse (cx - rr, cy - rr, rr * 2.0f, rr * 2.0f, 0.55f);
        }

        // Subtle top-left specular arc highlight on cap
        g.setColour (Colours::white.withAlpha (0.22f));
        g.drawEllipse (cx - capRad + 0.5f, cy - capRad + 0.5f, (capRad - 0.5f) * 2.0f, (capRad - 0.5f) * 2.0f, 0.85f);

        // High-contrast engraved white pointer line with shadow
        const float pAng = angle - MathConstants<float>::halfPi;
        const float px1 = cx + std::cos (pAng) * (capRad * 0.22f);
        const float py1 = cy + std::sin (pAng) * (capRad * 0.22f);
        const float px2 = cx + std::cos (pAng) * (knobRad * 0.95f);
        const float py2 = cy + std::sin (pAng) * (knobRad * 0.95f);
        g.setColour (Colours::black.withAlpha (0.78f));
        g.drawLine (px1 + 0.7f, py1 + 0.9f, px2 + 0.7f, py2 + 0.9f, 2.5f);
        g.setColour (Colour (0xfff8fafc));
        g.drawLine (px1, py1, px2, py2, 2.1f);
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

        // Deep recessed dark fader slot with inner shadow and metallic lip
        const Rectangle<float> outerSlot (cx - 4.5f, trackTop - 2.0f, 9.0f, trackBot - trackTop + 4.0f);
        g.setColour (Colour (0xff07080a));
        g.fillRoundedRectangle (outerSlot, 3.5f);
        g.setColour (Colour (0xff323840).withAlpha (0.65f));
        g.drawRoundedRectangle (outerSlot, 3.5f, 0.9f);

        const float clampedPos = std::clamp (sliderPos, trackTop, trackBot);
        const Rectangle<float> fill (cx - 1.8f, clampedPos, 3.6f, trackBot - clampedPos);
        g.setColour (kFLAccentOrange.withAlpha (0.24f));
        g.fillRoundedRectangle (fill.expanded (1.8f, 0.0f), 2.0f);
        g.setColour (kFLAccentOrange.withAlpha (0.85f));
        g.fillRoundedRectangle (fill, 1.5f);

        // Realistic sculpted brushed-aluminum console fader cap (matching reference image!)
        const float capW = 36.0f;
        const float capH = 54.0f;
        const Rectangle<float> cap (cx - capW * 0.5f,
                                    std::clamp (clampedPos - capH * 0.5f, trackTop - 12.0f, trackBot - capH + 12.0f),
                                    capW, capH);

        // Long realistic cast shadow below the fader cap
        g.setColour (Colours::black.withAlpha (0.78f));
        g.fillRoundedRectangle (cap.translated (0.0f, 4.5f).expanded (1.0f, 1.0f), 4.5f);

        // Metallic silver/titanium multi-stop vertical gradient for curved fader profile
        ColourGradient cg (Colour (0xff9aa3ad), cap.getX(), cap.getY(),
                           Colour (0xff2b3036), cap.getX(), cap.getBottom(), false);
        cg.addColour (0.14, Colour (0xffcdd4dc));
        cg.addColour (0.42, Colour (0xff5c646d));
        cg.addColour (0.50, Colour (0xff23272c));
        cg.addColour (0.58, Colour (0xff7e8791));
        cg.addColour (0.86, Colour (0xffb4bcc6));
        g.setGradientFill (cg);
        g.fillRoundedRectangle (cap, 4.0f);

        // Machined horizontal grip grooves across the fader cap
        for (float gy : { -18.0f, -14.0f, -10.0f, 10.0f, 14.0f, 18.0f })
        {
            const float ly = cap.getCentreY() + gy;
            g.setColour (Colours::black.withAlpha (0.55f));
            g.drawHorizontalLine (roundToInt (ly), cap.getX() + 3.0f, cap.getRight() - 3.0f);
            g.setColour (Colours::white.withAlpha (0.30f));
            g.drawHorizontalLine (roundToInt (ly + 1.0f), cap.getX() + 3.0f, cap.getRight() - 3.0f);
        }

        // Outer dark edge & top specular highlight
        g.setColour (Colour (0xff101316));
        g.drawRoundedRectangle (cap, 4.0f, 1.2f);
        g.setColour (Colours::white.withAlpha (0.45f));
        g.drawRoundedRectangle (cap.reduced (1.0f), 3.2f, 0.8f);

        // Crisp center index line (dark groove + glowing orange/amber center line matching reference photo!)
        g.setColour (Colour (0xff0b0d10));
        g.fillRect (cap.getX() + 2.0f, cap.getCentreY() - 2.2f, cap.getWidth() - 4.0f, 4.4f);
        g.setColour (kFLAccentOrange);
        g.fillRect (cap.getX() + 2.5f, cap.getCentreY() - 1.1f, cap.getWidth() - 5.0f, 2.2f);
    }

    void drawButtonBackground (Graphics& g, Button& button,
                               const Colour& /*backgroundColour*/,
                               bool over, bool down) override
    {
        const auto b = button.getLocalBounds().toFloat().reduced (0.8f);
        const bool on = button.getToggleState();
        const String cid = button.getComponentID();

        // Recessed dark mounting bezel around every hardware push-button
        g.setColour (Colour (0xff090b0d));
        g.fillRoundedRectangle (b.expanded (1.6f), 4.4f);
        g.setColour (Colours::black.withAlpha (0.72f));
        g.fillRoundedRectangle (b.translated (0.0f, 2.0f), 3.6f);

        const Colour glow = (cid == "RedBtn")   ? kFLAccentRed
                          : (cid == "GreenBtn" || cid == "CyanBtn") ? kFLAccentCyan
                                                : kFLAccentOrange;
        if (on || down)
        {
            // Translucent backlit polycarbonate hardware switch (1:1 match to reference image!)
            g.setColour (glow.withAlpha (0.28f));
            g.fillRoundedRectangle (b.expanded (2.0f), 4.5f);

            // Rich deep translucent body with inner hot-spot glow
            ColourGradient fg (glow.darker (0.35f), b.getX(), b.getY(),
                               glow.darker (0.85f), b.getX(), b.getBottom(), false);
            fg.addColour (0.5, glow.darker (0.50f));
            g.setGradientFill (fg);
            g.fillRoundedRectangle (b, 3.2f);

            ColourGradient centerGlow (glow.withAlpha (0.55f), b.getCentreX(), b.getCentreY(),
                                       Colours::transparentBlack, b.getX(), b.getY(), true);
            g.setGradientFill (centerGlow);
            g.fillRoundedRectangle (b, 3.2f);

            // Bright neon edge rim & top specular highlight
            g.setColour (glow.brighter (0.25f).withAlpha (0.90f));
            g.drawRoundedRectangle (b.reduced (0.4f), 3.0f, 1.1f);
            g.setColour (Colours::white.withAlpha (0.42f));
            g.drawHorizontalLine (roundToInt (b.getY() + 1.0f), b.getX() + 3.0f, b.getRight() - 3.0f);
        }
        else
        {
            // Tactile matte dark-grey hardware switch with 3D top bevel
            const Colour topCol = over ? Colour (0xff454b52) : Colour (0xff383d43);
            const Colour botCol = over ? Colour (0xff262a2f) : Colour (0xff1e2226);
            ColourGradient offGrad (topCol, b.getX(), b.getY(), botCol, b.getX(), b.getBottom(), false);
            g.setGradientFill (offGrad);
            g.fillRoundedRectangle (b, 3.2f);

            g.setColour (Colours::white.withAlpha (0.20f));
            g.drawHorizontalLine (roundToInt (b.getY() + 1.0f), b.getX() + 3.0f, b.getRight() - 3.0f);
            g.setColour (Colour (0xff3a4047).withAlpha (0.55f));
            g.drawRoundedRectangle (b.reduced (0.5f), 2.8f, 0.8f);
            g.setColour (Colour (0xff0e1013));
            g.drawRoundedRectangle (b, 3.2f, 1.0f);
        }
    }

    void drawButtonText (Graphics& g, TextButton& button, bool /*over*/, bool /*down*/) override
    {
        const bool on = button.getToggleState();
        // Reference photo uses crisp white/illuminated text on both active backlit and inactive buttons!
        if (on)
        {
            g.setColour (Colours::black.withAlpha (0.65f));
            g.setFont (getFLFont (10.0f, true));
            g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (2, 1).translated (0, 1), Justification::centred, 1);
            g.setColour (Colour (0xfff8fafc));
        }
        else
        {
            g.setColour (kFLTextBright);
        }
        g.setFont (getFLFont (10.0f, true));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (2, 1), Justification::centred, 1);
    }
};

// =========================================================================
// Functional Rotary Knob (Uniform 52x52 across the entire plugin):
// - No internal JUCE Label overlay (Slider::NoTextBox) so 100% of the knob area
//   (exact center, top, bottom, and sides) responds immediately to click & drag!
// - Scroll Mouse Wheel over the knob to step/adjust the value!
// - Double-click right mouse button to reset the knob to its default value!
// - Double-click left mouse button to open an inline numeric editor.
// =========================================================================
class ConsoleDial final : public Slider
{
public:
    ConsoleDial()
    {
        setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
        setDoubleClickReturnValue (false, 0.0);
        setSliderSnapsToMousePosition (false);
        setMouseDragSensitivity (115);
        setVelocityBasedMode (false);
        setScrollWheelEnabled (true);
        setWantsKeyboardFocus (true);
        setPopupDisplayEnabled (true, true, nullptr);
    }

    void bindResetParameter (RangedAudioParameter* param) noexcept
    {
        boundParam = param;
    }

    void resetToDefaultValue()
    {
        closeInlineEditor();
        if (boundParam != nullptr)
        {
            const float defNorm = boundParam->getDefaultValue();
            boundParam->beginChangeGesture();
            boundParam->setValueNotifyingHost (defNorm);
            boundParam->endChangeGesture();
        }
        else if (isDoubleClickReturnEnabled())
        {
            setValue (getDoubleClickReturnValue(), sendNotificationSync);
        }
    }

    bool hitTest (int x, int y) override
    {
        return getLocalBounds().contains (x, y);
    }

    void mouseDown (const MouseEvent& e) override
    {
        closeInlineEditor();
        if (e.mods.isRightButtonDown() || e.mods.isPopupMenu())
        {
            const uint32 nowMs = Time::getMillisecondCounter();
            if (nowMs - lastRightClickMs <= 450)
            {
                lastRightClickMs = 0;
                resetToDefaultValue();
                return;
            }
            lastRightClickMs = nowMs;
            return;
        }
        dragStartProp  = valueToProportionOfLength (getValue());
        dragStartPt    = e.position;
        isDraggingDial = true;
        if (boundParam != nullptr)
            boundParam->beginChangeGesture();
        if (onDragStart != nullptr)
            onDragStart();
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (!isDraggingDial || e.mods.isRightButtonDown() || e.mods.isPopupMenu())
            return;
        // e.position is in local unscaled component coordinates (52x52),
        // so dragging anywhere on the knob (center, top, bottom, sides) moves smoothly across the full range!
        const float sensitivity = e.mods.isShiftDown() ? 360.0f : 105.0f;
        const float dy = e.position.y - dragStartPt.y;
        const float dx = e.position.x - dragStartPt.x;
        const float delta = (-dy + dx * 0.5f) / sensitivity;
        const double newProp = std::clamp (dragStartProp + static_cast<double> (delta), 0.0, 1.0);
        setValue (proportionOfLengthToValue (newProp), sendNotificationSync);
    }

    void mouseUp (const MouseEvent& /*e*/) override
    {
        if (isDraggingDial)
        {
            isDraggingDial = false;
            if (onDragEnd != nullptr)
                onDragEnd();
            if (boundParam != nullptr)
                boundParam->endChangeGesture();
        }
    }

    void mouseWheelMove (const MouseEvent& /*e*/, const MouseWheelDetails& wheel) override
    {
        if (!isEnabled())
            return;
        const double delta = (std::abs (wheel.deltaY) > std::abs (wheel.deltaX)) ? wheel.deltaY : wheel.deltaX;
        if (std::abs (delta) < 1.0e-5)
            return;
        const double stepProp = (getInterval() >= 1.0)
            ? (1.0 / std::max (1.0, (getMaximum() - getMinimum()) / getInterval()))
            : 0.025;
        const double dir = (wheel.isReversed ? -1.0 : 1.0) * (delta > 0.0 ? 1.0 : -1.0);
        const double curProp = valueToProportionOfLength (getValue());
        const double nextProp = std::clamp (curProp + dir * stepProp, 0.0, 1.0);
        if (boundParam != nullptr) boundParam->beginChangeGesture();
        if (onDragStart != nullptr) onDragStart();
        setValue (proportionOfLengthToValue (nextProp), sendNotificationSync);
        if (onDragEnd != nullptr) onDragEnd();
        if (boundParam != nullptr) boundParam->endChangeGesture();
    }

    void mouseDoubleClick (const MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown() || e.mods.isPopupMenu())
        {
            lastRightClickMs = 0;
            resetToDefaultValue();
            return;
        }
        openInlineEditor();
    }

private:
    void openInlineEditor()
    {
        closeInlineEditor();
        inlineEditor = std::make_unique<TextEditor>();
        inlineEditor->setFont (getFLFont (11.5f, true));
        inlineEditor->setColour (TextEditor::textColourId, Colours::white);
        inlineEditor->setColour (TextEditor::backgroundColourId, Colour (0xff121417));
        inlineEditor->setColour (TextEditor::outlineColourId, kFLAccentOrange);
        inlineEditor->setColour (TextEditor::focusedOutlineColourId, kFLAccentOrange);
        inlineEditor->setJustification (Justification::centred);
        inlineEditor->setBounds (2, getHeight() / 2 - 10, getWidth() - 4, 20);
        inlineEditor->setText (getTextFromValue (getValue()), false);
        inlineEditor->selectAll();
        inlineEditor->onReturnKey = [this]
        {
            if (inlineEditor != nullptr)
            {
                const String txt = inlineEditor->getText().trim();
                const double val = getValueFromText (txt);
                setValue (std::clamp (val, getMinimum(), getMaximum()), sendNotificationSync);
            }
            closeInlineEditor();
        };
        inlineEditor->onEscapeKey = [this] { closeInlineEditor(); };
        inlineEditor->onFocusLost = [this] { closeInlineEditor(); };
        addAndMakeVisible (*inlineEditor);
        inlineEditor->grabKeyboardFocus();
    }

    void closeInlineEditor()
    {
        inlineEditor.reset();
    }

    std::unique_ptr<TextEditor> inlineEditor;
    RangedAudioParameter* boundParam = nullptr;
    uint32 lastRightClickMs = 0;
    double dragStartProp = 0.0;
    Point<float> dragStartPt {};
    bool isDraggingDial = false;
};

class MasterFader final : public Slider
{
public:
    void bindResetParameter (RangedAudioParameter* param) noexcept
    {
        boundParam = param;
    }

    void resetToDefaultValue()
    {
        if (boundParam != nullptr)
        {
            const float defNorm = boundParam->getDefaultValue();
            boundParam->beginChangeGesture();
            boundParam->setValueNotifyingHost (defNorm);
            boundParam->endChangeGesture();
        }
        else
        {
            setValue (0.0, sendNotificationSync);
        }
    }

    void mouseDown (const MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown() || e.mods.isPopupMenu())
        {
            const uint32 nowMs = Time::getMillisecondCounter();
            if (nowMs - lastRightClickMs <= 420)
            {
                lastRightClickMs = 0;
                resetToDefaultValue();
                return;
            }
            lastRightClickMs = nowMs;
        }
        Slider::mouseDown (e);
    }

    void mouseDoubleClick (const MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown() || e.mods.isPopupMenu())
        {
            lastRightClickMs = 0;
            resetToDefaultValue();
            return;
        }
        Slider::mouseDoubleClick (e);
    }

private:
    RangedAudioParameter* boundParam = nullptr;
    uint32 lastRightClickMs = 0;
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

        // 1. HIGH-GAIN INPUT (Yellow) + FL HEAT (Bipolar) / OUTPUT EVEN & ODD (0..100% Unipolar)
        setupKnob (kInputGain, "preamp",        "SSLYellow");
        setupKnob (kHeatDrive, "heatDrive",     "SSLYellow");
        setupKnob (kHeatTone,  "heatTone",      "SSLBlue");
        setupKnob (kEvenHarm,  "evenHarmonics", "SSLYellow");
        setupKnob (kOddHarm,   "oddHarmonics",  "SSLBlue");

        // 2. DYNAMICS (COMP): alternating Yellow & Blue
        setupKnob (kCompRat,   "compRatio",     "SSLYellow");
        setupKnob (kCompThr,   "compThreshold", "SSLBlue");
        setupKnob (kCompRel,   "compRelease",   "SSLYellow");

        // 3. GATE / EXPANDER: alternating Blue & Yellow
        setupKnob (kExpThr,    "gateThreshold", "SSLBlue");
        setupKnob (kExpRng,    "gateRange",     "SSLYellow");
        setupKnob (kExpRel,    "gateRelease",   "SSLBlue");

        // 4. FILTERS: HP & LP (2 knobs Hz + 2 stepped knobs from 0 dB to Brickwall!)
        setupKnob (kHpf,       "hpf",           "SSLYellow");
        setupKnob (kLpf,       "lpf",           "SSLBlue");
        setupKnob (kHpSlope,   "hpSlope",       "SSLYellow");
        setupKnob (kLpSlope,   "lpSlope",       "SSLBlue");

        // 5. 4-BAND EQ: Each 2-knob block has 1 Yellow & 1 Blue; Mid rows alternate Yellow / Blue
        setupKnob (kHfGain,    "highGain",      "SSLYellow");
        setupKnob (kHfFreq,    "highFreq",      "SSLBlue");

        setupKnob (kMidGain,   "highMidGain",   "SSLYellow");
        setupKnob (kMidQ,      "highMidQ",      "SSLBlue");
        setupKnob (kMidFreq,   "highMidFreq",   "SSLYellow");

        setupKnob (kLmfGain,   "lowMidGain",    "SSLBlue");
        setupKnob (kLmfQ,      "lowMidQ",       "SSLYellow");
        setupKnob (kLmfFreq,   "lowMidFreq",    "SSLBlue");

        setupKnob (kLfGain,    "lowGain",       "SSLYellow");
        setupKnob (kLfFreq,    "lowFreq",       "SSLBlue");

        // 6. DE-ESSER: 2 KNOBS (1 Yellow + 1 Blue)
        setupKnob (kDsFreq,    "dsFreq",        "SSLYellow");
        setupKnob (kDsThr,     "dsThresh",      "SSLBlue");

        // 7. MID / SIDE dB: 2 KNOBS (1 Yellow + 1 Blue)
        setupKnob (kMidDb,     "midGain",       "SSLYellow");
        setupKnob (kSideDb,    "sideGain",      "SSLBlue");

        // 8. INFLATE (ABOVE MASTER FADER): 2 KNOBS (1 Yellow + 1 Blue)
        setupKnob (kInfEff,    "inflateEffect", "SSLYellow");
        setupKnob (kInfCrv,    "inflateCurve",  "SSLBlue");

        // 9. MASTER OUT FADER (Very last stage in DSP, alongside Dual L/R General Meter!)
        fader.setSliderStyle (Slider::LinearVertical);
        fader.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        fader.setDoubleClickReturnValue (true, 0.0);
        fader.setPopupDisplayEnabled (true, true, &canvas);
        fader.bindResetParameter (state.getParameter ("fader"));
        canvas.addAndMakeVisible (fader);
        faderAtt = std::make_unique<State::SliderAttachment> (state, "fader", fader);

        // TYPE: A (FL HEAT) / TYPE: B (OUTPUT EVEN & ODD) selector button (all uniform size!)
        setupParamBtn  (bHeatByp, "TYPE: A",  "AmberBtn", "heatBypass");
        setupParamBtn  (bPhase,   String::fromUTF8 ("\xc3\x98"), "RedBtn", "phaseInv");
        setupParamBtn  (bPad,     "MONO",     "AmberBtn", "pad20");

        // Dynamics buttons (Red -> Blue -> Yellow sequence for the 3-button row!)
        setupParamBtn  (bLink,    "EXT SC",   "AmberBtn", "extSidechain");
        setupParamBtn  (bExpSel,  "GATE",     "CyanBtn",  "gateMode");
        setupParamBtn  (bDynIn,   "COMP SC",  "RedBtn",   "fltDynSc");
        setupParamBtn  (bExpIn,   "GATE ON",  "CyanBtn",  "expIn");
        setupParamBtn  (bCmpIn,   "COMP ON",  "AmberBtn", "cmpIn");
        setupParamBtn (bAutoMakeup, "AUTO", "AmberBtn", "autoMakeup");

        // EQ Bell buttons & Black/Brown curve button (Red -> Blue -> Yellow sequence for the 3-button row!)
        setupParamBtn  (bHfBell,  "BELL",     "RedBtn",   "hfBell");
        setupParamBtn  (bEqSel,   "BLK/BRN",  "AmberBtn", "eqMode");
        setupParamBtn  (bLfBell,  "BELL",     "AmberBtn", "lfBell");
        setupParamBtn  (bEqIn,    "EQ ON",    "RedBtn",   "eqIn");
        setupParamBtn  (bEqSc,    "EQ SC",    "CyanBtn",  "eqDynSc");
        setupParamBtn  (bEqPre,   "PRE DYN",  "AmberBtn", "eqPreDyn");

        // 3 De-Esser buttons (Red -> Blue -> Yellow sequence for the 3-button row!)
        setupParamBtn  (bDsType,    "TYPE: BP",  "RedBtn",   "dsScFilter");
        setupParamBtn  (bDsMode,    "SPLIT",     "CyanBtn",  "dsAudioMode");
        setupParamBtn  (bDsPrePost, "PRE DYN",   "AmberBtn", "dsPrePost");

        // Warm -128 dBFS Analog Console Noise button below Mid & Side
        setupKnob (kStereoSep, "stereoWidth", "WidthBipolar");
        kStereoSep.setTooltip ("Width: 0% mono, 100% original, 200% wider. No pan or rotation; does not create stereo from mono.");

        // OUTPUT Inflate CLIP 0 dB button
        setupParamBtn  (bClip0,   "CLIP 0",   "RedBtn",   "inflateClip");

        // General Dual L/R Meter IN / OUT & Scale Selectors (100% bound via State::ButtonAttachment for full DAW automation!)
        setupParamBtn  (bInOutMeter, "MTR OUT", "CyanBtn",  "meterInOut");
        setupParamBtn  (bMeterScale, "PEAK dB", "AmberBtn", "meterScale");

        layoutCanvasControls();
        syncDynamicStates();

        // 1:1 Resizable vector UI across wide range (0.45x to 2.00x) with corner resizer!
        setResizable (true, true);
        constexpr float initScale = 0.72f;
        setResizeLimits (roundToInt (kBaseW * 0.45f), roundToInt (kBaseH * 0.45f),
                         roundToInt (kBaseW * 2.00f), roundToInt (kBaseH * 2.00f));
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

    void mouseDoubleClick (const MouseEvent&) override
    {
        toggleChoice ("oversampling");
        syncDynamicStates();
        canvas.repaint();
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
        auto* rangedParam = state.getParameter (paramId);
        d.setComponentID (colorId);
        d.bindResetParameter (rangedParam);
        canvas.addAndMakeVisible (d);
        attachments.push_back (std::make_unique<State::SliderAttachment> (state, paramId, d));

        if (rangedParam != nullptr)
            d.setDoubleClickReturnValue (true, static_cast<double> (rangedParam->convertFrom0to1 (rangedParam->getDefaultValue())));

        if (auto* p = dynamic_cast<AudioParameterFloat*> (rangedParam))
        {
            d.textFromValueFunction = [this, p, paramId] (double v)
            {
                if (paramId == "compRatio")
                {
                    if (v >= 19.95)
                        return String ("20:0");
                    return String (v, 1) + ":1";
                }
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
        // Every single rotary knob in the plugin uses the exact same 52x52 hardware dimensions!
        constexpr int knobSz = 52;
        constexpr int eqKnob = 52;
        // Every single button in the plugin uses the exact same hardware dimensions!
        constexpr int btnW   = 52;
        constexpr int btnH   = 22;

        // =====================================================================
        // COLUMN 1 (x = 0..192):
        // - INPUT GAIN (-24..+48 dB before Input Meter) + TYPE: A / TYPE: B (FL HEAT / EVEN & ODD)
        // - COMPRESSOR (Separated card: y = 198..422)
        // - GATE / EXPAND (Separated card: y = 427..712)
        // - FILTERS: HP Hz, LP Hz + 2 Stepped Knobs FOR dB/OCT (y = 717..915)
        // =====================================================================
        kInputGain.setBounds (48, 30, knobSz, knobSz);
        bPhase.setBounds     (124, 24, btnW, btnH);
        bPad.setBounds       (124, 52, btnW, btnH);
        bHeatByp.setBounds   (124, 80, btnW, btnH);

        kHeatDrive.setBounds (24, 114, knobSz, knobSz);
        kHeatTone.setBounds  (96, 114, knobSz, knobSz);
        kEvenHarm.setBounds  (24, 114, knobSz, knobSz);
        kOddHarm.setBounds   (96, 114, knobSz, knobSz);

        // COMPRESSOR Card (y = 198..422)
        kCompRat.setBounds (22, 228, knobSz, knobSz);
        bLink.setBounds    (118, 234, btnW, btnH);
        kCompThr.setBounds (114, 300, knobSz, knobSz);
        bAutoMakeup.setBounds (118, 260, btnW, btnH);
        kCompRel.setBounds (22, 328, knobSz, knobSz);

        // GATE / EXPAND Card (y = 427..712)
        kExpThr.setBounds  (112, 450, knobSz, knobSz);
        kExpRng.setBounds  (20,  460, knobSz, knobSz);
        bExpSel.setBounds  (116, 520, btnW, btnH);
        kExpRel.setBounds  (20,  558, knobSz, knobSz);

        // Dynamics section enable buttons (3 identical buttons)
        bDynIn.setBounds (12,  658, btnW, btnH);
        bExpIn.setBounds (70,  658, btnW, btnH);
        bCmpIn.setBounds (128, 658, btnW, btnH);

        // Filters: HP Hz & LP kHz (top row) + 2 Stepped Slope Knobs (0 dB .. Brickwall, bottom row)
        kHpf.setBounds     (24,  740, knobSz, knobSz);
        kLpf.setBounds     (112, 740, knobSz, knobSz);
        kHpSlope.setBounds (24,  822, knobSz, knobSz);
        kLpSlope.setBounds (112, 822, knobSz, knobSz);

        // =====================================================================
        // COLUMN 2 (x = 192..384):
        // - EQ HF Card (y = 5..123)
        // - EQ MID (HMF & LMF) Card (y = 128..378)
        // - EQ LF Card (y = 383..561)
        // - DE-ESSER Card (y = 566..740)
        // - MID / SIDE dB Card (y = 745..915)
        // =====================================================================
        // EQ HF Card
        kHfGain.setBounds  (210, 28, eqKnob, eqKnob);
        kHfFreq.setBounds  (314, 28, eqKnob, eqKnob);
        bHfBell.setBounds  (262, 92, btnW, btnH);

        // EQ MID Card (HMF row + LMF row + BLK/BRN button)
        kMidGain.setBounds (198, 150, eqKnob, eqKnob);
        kMidQ.setBounds    (262, 150, eqKnob, eqKnob);
        kMidFreq.setBounds (326, 150, eqKnob, eqKnob);
        bEqSel.setBounds   (262, 224, btnW, btnH);

        kLmfGain.setBounds (198, 270, eqKnob, eqKnob);
        kLmfQ.setBounds    (262, 270, eqKnob, eqKnob);
        kLmfFreq.setBounds (326, 270, eqKnob, eqKnob);

        // EQ LF Card (LF row + LF BELL + EQ routing buttons)
        kLfGain.setBounds  (210, 406, eqKnob, eqKnob);
        kLfFreq.setBounds  (314, 406, eqKnob, eqKnob);
        bLfBell.setBounds  (262, 470, btnW, btnH);

        bEqIn.setBounds    (204, 514, btnW, btnH);
        bEqSc.setBounds    (262, 514, btnW, btnH);
        bEqPre.setBounds   (320, 514, btnW, btnH);

        // DE-ESS Section (y = 566..740): 2 Knobs (FREQ & THRESH) + 3 uniform 52x22 buttons (TYPE, MODE, PRE/POST)
        kDsFreq.setBounds    (214, 592, knobSz, knobSz);
        kDsThr.setBounds     (302, 592, knobSz, knobSz);
        bDsType.setBounds    (204, 688, btnW, btnH);
        bDsMode.setBounds    (262, 688, btnW, btnH);
        bDsPrePost.setBounds (320, 688, btnW, btnH);

        // MID & SIDE dB (from OUTPUT) placed right below De-Esser (y = 745..915) + ANALOG (-128 dB) button!
        kMidDb.setBounds   (214, 772, knobSz, knobSz);
        kSideDb.setBounds  (302, 772, knobSz, knobSz);
        kStereoSep.setBounds (262, 845, knobSz, knobSz);

        // =====================================================================
        // COLUMN 3 (x = 384..540):
        // - INFLATE (TOP OF COLUMN 3, ABOVE MASTER FADER): 2 KNOBS (INFLATE & CURVE) + CLIP 0 dB
        // - MASTER OUT FADER + DUAL L/R GENERAL LED METER
        // - CHANNELSTRIP + PAPIELGUAPO SIGNATURE (BELOW THE SLIDER!)
        // =====================================================================
        kInfEff.setBounds      (400, 30, knobSz, knobSz);
        kInfCrv.setBounds      (472, 30, knobSz, knobSz);
        bClip0.setBounds       (436, 120, btnW, btnH);

        // General Dual L/R Meter Controls (identical size!) + Long Master Out Fader
        bInOutMeter.setBounds  (402, 192, btnW, btnH);
        bMeterScale.setBounds  (468, 192, btnW, btnH);
        fader.setBounds        (466, 232, 44, 528);
    }

    void syncDynamicStates()
    {
        // Switch between TYPE: A (FL HEAT Drive/Tone) and TYPE: B (OUTPUT EVEN/ODD Harmonics 0..100%)
        const bool isEvenOddMode = bHeatByp.getToggleState();
        bHeatByp.setButtonText (isEvenOddMode ? "TYPE: B" : "TYPE: A");
        bHeatByp.setComponentID (isEvenOddMode ? "CyanBtn" : "AmberBtn");
        kHeatDrive.setVisible (!isEvenOddMode);
        kHeatTone.setVisible  (!isEvenOddMode);
        kEvenHarm.setVisible  (isEvenOddMode);
        kOddHarm.setVisible   (isEvenOddMode);

        const int eqCurv = roundToInt (state.getRawParameterValue ("eqMode")->load());
        bEqSel.setToggleState (eqCurv == 1, dontSendNotification);

        const int gMode = roundToInt (state.getRawParameterValue ("gateMode")->load());
        bExpSel.setToggleState (gMode == 1, dontSendNotification);
        bExpSel.setButtonText (gMode == 1 ? "EXPAND" : "GATE");

        // Sync the 3 De-Esser buttons (TYPE, MODE, PRE DYN)
        const int dsSc = roundToInt (state.getRawParameterValue ("dsScFilter")->load());
        bDsType.setToggleState (dsSc == 1, dontSendNotification);
        bDsType.setButtonText (dsSc == 1 ? "TYPE: BP" : "TYPE: HP");

        const int dsMd = roundToInt (state.getRawParameterValue ("dsAudioMode")->load());
        bDsMode.setButtonText (dsMd == 0 ? "SPLIT" : "WIDE");

        bDsPrePost.setButtonText ("PRE DYN");

        showPreHeatInputMeter = bInOutMeter.getToggleState();
        bInOutMeter.setButtonText (showPreHeatInputMeter ? "MTR IN" : "MTR OUT");

        useCalibratedRms18 = bMeterScale.getToggleState();
        bMeterScale.setButtonText (useCalibratedRms18 ? "-18 VU" : "PEAK dB");

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
        auto outDir = File::getCurrentWorkingDirectory().getChildFile ("docs");
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
        // Render 100% vector backdrop at full target resolution (zero bitmap blur at any UI scale!)
        drawFLHeatBackdropVector (g, kBaseW, kBaseH);

        const Colour silkCol = kFLTextBright;

        // =====================================================================
        // COLUMN 1: INPUT GAIN (+48 dB) + FL HEAT / OUTPUT EVEN & ODD, COMPRESSOR, GATE, FILTERS
        // =====================================================================
        const float hDrive      = state.getRawParameterValue ("heatDrive")->load();
        const float evenVal     = state.getRawParameterValue ("evenHarmonics")->load();
        const float oddVal      = state.getRawParameterValue ("oddHarmonics")->load();
        const bool  evenOddMode = bHeatByp.getToggleState();
        const Colour heatLedCol = evenOddMode
                                ? ((oddVal > evenVal + 0.5f) ? kFLAccentCyan : kFLAccentOrange)
                                : ((hDrive < -0.05f) ? kFLAccentCyan : kFLAccentOrange);
        const bool   colorLit   = evenOddMode ? (evenVal > 0.5f || oddVal > 0.5f) : (std::abs (hDrive) > 0.05f);
        // OVER LED on the left of the INPUT +48 dB knob, with "OVER" text to the right of the LED!
        drawIndicatorLed (g, 14.0f, 36.0f, 7.5f, kLedRed,
                          telemetry.inputOverload.load (std::memory_order_relaxed));
        g.setColour (silkCol);
        g.setFont (getFLFont (8.2f, true));
        g.drawText ("OVER", 21, 30, 28, 12, Justification::centredLeft);
        drawIndicatorLed (g, 110.0f, 91.0f, 7.5f, heatLedCol, colorLit);

        drawDialRingAndLabels (g, kInputGain, {}, "INPUT +48 dB");
        if (evenOddMode)
        {
            drawDialRingAndLabels (g, kEvenHarm, {}, "EVEN %");
            drawDialRingAndLabels (g, kOddHarm,  {}, "ODD %");
        }
        else
        {
            drawDialRingAndLabels (g, kHeatDrive, {}, "HEAT DRIVE");
            drawDialRingAndLabels (g, kHeatTone,  {}, "HEAT TONE");
        }

        // COMPRESSOR Card
        drawIndicatorLed (g, 142.0f, 222.0f, 7.5f, kLedAmber, bLink.getToggleState());

        const float curRat = state.getRawParameterValue ("compRatio")->load();
        const String ratLabel = (curRat >= 19.95f)
                              ? "RATE 20:0"
                              : ("RATE " + String (curRat, curRat >= 9.95f ? 0 : 1) + ":1");
        drawDialRingAndLabels (g, kCompRat, {}, ratLabel);
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

        // EXP / GATE LEDs perfectly aligned vertically with the 5-LED columns below (x = 120.0f & 158.0f)!
        const int gMode = roundToInt (state.getRawParameterValue ("gateMode")->load());
        g.setColour (silkCol);
        g.setFont (getFLFont (8.5f, true));
        g.drawText ("EXP",  106, 544, 28, 11, Justification::centred);
        g.drawText ("GATE", 142, 544, 32, 11, Justification::centred);
        drawIndicatorLed (g, 120.0f, 560.0f, 7.0f, kLedAmber, gMode == 1);
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

        drawIndicatorLed (g, 38.0f,  692.0f, 7.5f, kLedRed,       bDynIn.getToggleState());
        drawIndicatorLed (g, 96.0f,  692.0f, 7.5f, kFLAccentCyan, bExpIn.getToggleState());
        drawIndicatorLed (g, 154.0f, 692.0f, 7.5f, kLedAmber,     bCmpIn.getToggleState());

        // FILTERS section: 2 Knobs (HP Hz & LP kHz) + 2 Stepped Slope Knobs (0 dB .. Brickwall)
        drawDialRingAndLabels (g, kHpf, {}, "HP Hz");
        drawCurveGlyph (g, 16.0f, 796.0f, 2, silkCol);
        drawDialRingAndLabels (g, kLpf, {}, "LP kHz");
        drawCurveGlyph (g, 174.0f, 796.0f, 3, silkCol);

        constexpr const char* slopeNames[8] = { "0 dB", "6 dB", "12 dB", "18 dB", "24 dB", "36 dB", "48 dB", "BRICK" };
        const int hpIdx = std::clamp (roundToInt (state.getRawParameterValue ("hpSlope")->load()), 0, 7);
        const int lpIdx = std::clamp (roundToInt (state.getRawParameterValue ("lpSlope")->load()), 0, 7);
        drawDialRingAndLabels (g, kHpSlope, {}, String ("HP: ") + slopeNames[hpIdx]);
        drawDialRingAndLabels (g, kLpSlope, {}, String ("LP: ") + slopeNames[lpIdx]);

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

        drawIndicatorLed (g, 230.0f, 546.0f, 7.5f, kLedRed,       bEqIn.getToggleState());
        drawIndicatorLed (g, 288.0f, 546.0f, 7.5f, kFLAccentCyan, bEqSc.getToggleState());
        drawIndicatorLed (g, 346.0f, 546.0f, 7.5f, kLedAmber,     bEqPre.getToggleState());

        // DE-ESS Section (2 Knobs: FREQ & THRESH + 3 buttons TYPE, MODE, PRE/POST & LEDs)
        drawDialRingAndLabels (g, kDsFreq, {}, "FREQ Hz");
        drawDialRingAndLabels (g, kDsThr,  {}, "THRESH dB");
        const float dsGr = telemetry.deEssReductionDb.load (std::memory_order_relaxed);
        drawIndicatorLed (g, 288.0f, 672.0f, 7.8f,
                          dsGr > 0.2f ? kLedRed : kFLAccentOrange, dsGr > 0.1f);
        drawIndicatorLed (g, 230.0f, 722.0f, 7.2f, kLedRed,       bDsType.getToggleState());
        drawIndicatorLed (g, 288.0f, 722.0f, 7.2f, kFLAccentCyan, roundToInt (state.getRawParameterValue ("dsAudioMode")->load()) == 0);
        drawIndicatorLed (g, 346.0f, 722.0f, 7.2f, kLedAmber,     bDsPrePost.getToggleState());

        // MID / SIDE dB Section (from OUTPUT) placed right below De-Esser + ANALOG -128 dB LED!
        drawDialRingAndLabels (g, kMidDb,  {}, "MID dB");
        drawDialRingAndLabels (g, kSideDb, {}, "SIDE dB");
        drawDialRingAndLabels (g, kStereoSep, {}, "WIDTH");

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

    ConsoleDial kInputGain, kHeatDrive, kHeatTone, kEvenHarm, kOddHarm;
    ConsoleDial kCompRat, kCompThr, kCompRel;
    ConsoleDial kExpThr, kExpRng, kExpRel;
    ConsoleDial kHpf, kLpf, kHpSlope, kLpSlope;
    ConsoleDial kHfGain, kHfFreq;
    ConsoleDial kMidGain, kMidQ, kMidFreq;
    ConsoleDial kLmfGain, kLmfQ, kLmfFreq;
    ConsoleDial kLfGain, kLfFreq;
    ConsoleDial kDsFreq, kDsThr;
    ConsoleDial kInfEff, kInfCrv;
    ConsoleDial kMidDb, kSideDb, kStereoSep;
    MasterFader fader;

    TextButton bHeatByp, bPhase, bPad;
    TextButton bLink, bExpSel, bDynIn, bExpIn, bCmpIn, bAutoMakeup;
    TextButton bHfBell, bEqSel, bLfBell, bEqIn, bEqSc, bEqPre;
    TextButton bDsType, bDsMode, bDsPrePost;
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
    AudioBuffer<float> buf (2, 256);
    MidiBuffer midi;
    buf.clear();
    proc->processBlock (buf, midi);
    std::unique_ptr<AudioProcessorEditor> ed (proc->createEditorIfNeeded());
    if (auto* se = dynamic_cast<SurcoEditor*> (ed.get()))
    {
        se->renderSnapshotToFile (outPng);
    }
}
