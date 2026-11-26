#include "Processor.h"
#include "HeatDSP.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace
{
using State = juce::AudioProcessorValueTreeState;

enum Param {
    preamp, drive, hpf, lpf, lowFreq, lowGain, lowMidFreq, lowMidGain, lowMidQ,
    highMidFreq, highMidGain, highMidQ, highFreq, highGain, compMode, compThreshold,
    compRatio, compAttack, compRelease, compMakeup, gateMode, gateThreshold,
    gateRange, gateAttack, gateRelease, extSidechain, fader, preampMode, eqMode, deEss,
    phaseInv, pad20, hfBell, lfBell, eqIn, eqPreDyn, eqDynSc, dynIn, expIn, cmpIn,
    fltDynSc, fltPreDyn, filterSlope, hpSlopeParam, lpSlopeParam,
    dsFreq, dsThresh, dsScFilter, dsAudioMode, dsMonitor, dsPrePost,
    heatDrive, heatTone, heatBypass, evenHarmonics, oddHarmonics, midGain, sideGain,
    inflateEffect, inflateCurve, inflateClip, oversampling, analogNoise,
    meterInOutParam, meterScaleParam, count
};

constexpr const char* ids[] = {
    "preamp","drive","hpf","lpf","lowFreq","lowGain","lowMidFreq","lowMidGain","lowMidQ",
    "highMidFreq","highMidGain","highMidQ","highFreq","highGain","compMode","compThreshold",
    "compRatio","compAttack","compRelease","compMakeup","gateMode","gateThreshold",
    "gateRange","gateAttack","gateRelease","extSidechain","fader","preampMode","eqMode","deEss",
    "phaseInv","pad20","hfBell","lfBell","eqIn","eqPreDyn","eqDynSc","dynIn","expIn","cmpIn",
    "fltDynSc","fltPreDyn","filterSlope","hpSlope","lpSlope",
    "dsFreq","dsThresh","dsScFilter","dsAudioMode","dsMonitor","dsPrePost",
    "heatDrive","heatTone","heatBypass","evenHarmonics","oddHarmonics","midGain","sideGain",
    "inflateEffect","inflateCurve","inflateClip","oversampling","analogNoise",
    "meterInOut","meterScale"
};
static_assert (std::size (ids) == count);

State::ParameterLayout layout()
{
    State::ParameterLayout result;
    auto add = [&] (Param p, const char* name, float min, float max, float def, const char* unit, bool log = false)
    {
        juce::NormalisableRange<float> range (min, max);
        if (p == lowMidGain || p == highMidGain || p == midGain || p == sideGain || p == evenHarmonics || p == oddHarmonics)
            range.interval = 1.0f;
        if (log) range.setSkewForCentre (std::sqrt (min * max));
        result.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ids[p], 1 }, name, range, def,
            juce::AudioParameterFloatAttributes().withLabel (unit)));
    };
    auto choice = [&] (Param p, const char* name, juce::StringArray options, int def = 0)
    {
        result.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ids[p], 1 }, name, options, def));
    };
    auto flag = [&] (Param p, const char* name, bool def)
    {
        result.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ids[p], 1 }, name, def));
    };

    add (preamp, "Input trim", -24, 48, 0, "dB");
    add (drive, "Preamp drive", 0, 24, 0, "dB");
    add (hpf, "High pass", 20, 500, 20, "Hz", true);
    add (lpf, "Low pass", 1000, 20000, 20000, "Hz", true);
    add (lowFreq, "Low shelf frequency", 30, 450, 100, "Hz", true);
    add (lowGain, "Low shelf gain", -18, 18, 0, "dB");
    add (lowMidFreq, "Low mid frequency", 100, 3000, 500, "Hz", true);
    add (lowMidGain, "Low mid gain", -18, 18, 0, "dB");
    add (lowMidQ, "Low mid Q", 0.3f, 8, 1, "", true);
    add (highMidFreq, "High mid frequency", 200, 12000, 2500, "Hz", true);
    add (highMidGain, "High mid gain", -18, 18, 0, "dB");
    add (highMidQ, "High mid Q", 0.3f, 8, 1, "", true);
    add (highFreq, "High shelf frequency", 1500, 18000, 8000, "Hz", true);
    add (highGain, "High shelf gain", -18, 18, 0, "dB");
    choice (compMode, "Compressor detector", { "Peak", "RMS" });
    add (compThreshold, "Compressor threshold", -60, 0, -18, "dB");
    add (compRatio, "Compressor ratio", 1, 20, 1, ":1", true);
    add (compAttack, "Compressor attack", 0.1f, 100, 0.1f, "ms", true);
    add (compRelease, "Compressor release", 10, 2000, 100, "ms", true);
    add (compMakeup, "Compressor makeup", -20, 20, 0, "dB");
    choice (gateMode, "Gate mode", { "Gate", "Expand" });
    add (gateThreshold, "Gate threshold", -80, 0, -45, "dB");
    add (gateRange, "Gate range", 0, 80, 0, "dB");
    add (gateAttack, "Gate attack", 0.1f, 50, 0.1f, "ms", true);
    add (gateRelease, "Gate release", 5, 2000, 150, "ms", true);
    flag (extSidechain, "External sidechain", false);
    add (fader, "Output fader", -80, 12, 0, "dB");
    choice (preampMode, "Preamp model", { "Standard", "Vocals Line", "Vocals Mic" });
    choice (eqMode, "EQ curve", { "Black", "Brown" });
    // Vocals De-Esser Stage parameters (default ON, with TYPE: HighPass/BandPass, MODE: Split/Wide, PRE/POST routing!)
    flag (deEss, "De-Ess", true);

    // Console switches
    flag (phaseInv, "Phase invert", false);
    flag (pad20, "Pad -20 dB", false);
    flag (hfBell, "HF Bell", false);
    flag (lfBell, "LF Bell", false);
    flag (eqIn, "EQ In", true);
    flag (eqPreDyn, "EQ Pre Dyn", true);
    flag (eqDynSc, "EQ Dyn SC", false);
    flag (dynIn, "Dynamics In", true);
    flag (expIn, "Gate/Exp In", true);
    flag (cmpIn, "Compressor In", true);
    flag (fltDynSc, "Filters Dyn SC", false);
    flag (fltPreDyn, "Filters Pre Dyn", true);
    choice (filterSlope,  "Filter slope", { "6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct" }, 2);
    choice (hpSlopeParam, "HP Slope",     { "0 dB", "6 dB", "12 dB", "18 dB", "24 dB", "36 dB", "48 dB", "Brickwall" }, 3);
    choice (lpSlopeParam, "LP Slope",     { "0 dB", "6 dB", "12 dB", "18 dB", "24 dB", "36 dB", "48 dB", "Brickwall" }, 3);

    // Vocals De-Esser Stage parameters
    add (dsFreq, "DeEsser Freq", 3000.0f, 12000.0f, 6200.0f, "Hz", true);
    add (dsThresh, "DeEsser Thresh", -60.0f, 0.0f, 0.0f, "dB");
    choice (dsScFilter, "DeEsser SC Filter", { "HighPass", "BandPass" }, 1);
    choice (dsAudioMode, "DeEsser Audio Mode", { "Split", "Wide" }, 0);
    flag (dsMonitor, "DeEsser SC Monitor", false);
    flag (dsPrePost, "DeEsser Pre Dyn", true);

    // FL HEAT / OUTPUT EVEN & ODD Color Stage parameters
    // heatBypass: false = FL HEAT (Drive & Tone), true = OUTPUT EVEN & ODD Harmonics (0..100%)
    add (heatDrive,     "Heat Drive",     -10.0f, 10.0f, 0.0f, "");
    add (heatTone,      "Heat Tone",      -10.0f, 10.0f, 0.0f, "");
    flag (heatBypass,   "Color Mode (Heat / Even-Odd)", false);
    add (evenHarmonics, "Even Harmonics",   0.0f, 100.0f, 0.0f, "%");
    add (oddHarmonics,  "Odd Harmonics",    0.0f, 100.0f, 0.0f, "%");

    // OUTPUT Mid/Side & Inflate Stage parameters
    add (midGain,       "Mid Gain",       -20.0f, 20.0f, 0.0f, "dB");
    add (sideGain,      "Side Gain",      -20.0f, 20.0f, 0.0f, "dB");
    add (inflateEffect, "Inflate Effect", 0.0f, 100.0f, 0.0f, "%");
    add (inflateCurve,  "Inflate Curve", -50.0f, 50.0f, 0.0f, "");
    flag (inflateClip,  "Clip 0 dB", false);
    choice (oversampling,    "Oversampling",         { "1x Native", "2x OS", "4x OS", "8x OS" }, 0);
    flag (analogNoise,       "Analog Noise",         false);
    flag (meterInOutParam,   "Meter In / Out",       false);
    flag (meterScaleParam,   "Meter Scale (-18 VU)", false);

    return result;
}

struct Filter
{
    double b0=1,b1=0,b2=0,a1=0,a2=0;
    double lastHz=-1,lastQ=-1,lastDb=-999,lastFs=-1;
    int lastType=-1;
    std::array<double,2> z1{},z2{};
    void set (double b, double c, double d, double a, double e, double f)
    { b0=b/a; b1=c/a; b2=d/a; a1=e/a; a2=f/a; }
    double tick (int ch, double x)
    {
        const auto y=b0*x+z1[ch]; z1[ch]=b1*x-a1*y+z2[ch]; z2[ch]=b2*x-a2*y; return y;
    }
    void tuneFirstOrder (int type, double hz, double fs)
    {
        if (lastType == type + 10 && std::abs(hz - lastHz) < 1e-4 && std::abs(fs - lastFs) < 1e-2)
            return;
        lastType = type + 10; lastHz = hz; lastFs = fs;
        const double fc = std::clamp (hz, 10.0, 0.475 * fs);
        const double k = std::tan (juce::MathConstants<double>::pi * fc / fs);
        const double norm = 1.0 / (1.0 + k);
        if (type == 0) // 6 dB/oct HPF
        {
            b0 = norm; b1 = -norm; b2 = 0.0;
            a1 = (k - 1.0) * norm; a2 = 0.0;
        }
        else // 6 dB/oct LPF
        {
            b0 = k * norm; b1 = k * norm; b2 = 0.0;
            a1 = (k - 1.0) * norm; a2 = 0.0;
        }
    }
    void tune (int type, double hz, double q, double db, double fs)
    {
        if (lastType == type && std::abs(hz - lastHz) < 1e-4 && std::abs(q - lastQ) < 1e-5
            && std::abs(db - lastDb) < 1e-4 && std::abs(fs - lastFs) < 1e-2)
            return;
        lastType = type; lastHz = hz; lastQ = q; lastDb = db; lastFs = fs;
        const auto w=juce::MathConstants<double>::twoPi*std::min (hz,0.475*fs)/fs;
        const auto c=std::cos(w), s=std::sin(w), alpha=s/(2*q), A=std::pow(10.0,db/40);
        if (type<2)
        {
            const double k=type==0 ? (1+c)/2 : (1-c)/2;
            set(k,type==0 ? -2*k : 2*k,k,1+alpha,-2*c,1-alpha);
        }
        else if (type==3 || type==4) set(1+alpha*A,-2*c,1-alpha*A,1+alpha/A,-2*c,1-alpha/A);
        else
        {
            const auto p=A+1,m=A-1,t=s*std::sqrt(A*((A+1/A)*(1/q-1)+2));
            if (type==2) set(A*(p-m*c+t),2*A*(m-p*c),A*(p-m*c-t),p+m*c+t,-2*(m+p*c),p+m*c-t);
            else set(A*(p+m*c+t),-2*A*(m+p*c),A*(p+m*c-t),p-m*c+t,2*(m-p*c),p-m*c-t);
        }
    }
};

// Stepped HP/LP Filter from 0 dB (Off) up to Brickwall (96 dB/oct 16-pole Butterworth cascade)
// Slope steps: 0=0 dB (Off), 1=6 dB, 2=12 dB, 3=18 dB, 4=24 dB, 5=36 dB, 6=48 dB, 7=Brickwall (96 dB/oct)
struct SlopeFilter
{
    std::array<Filter, 8> stages{};
    int numBiquads = 0;
    bool hasFirstOrder = false;
    int slopeStep = 3;

    void clear() noexcept { stages = {}; }

    void tune (int type, double hz, int stepIdx, double fs) noexcept
    {
        slopeStep = std::clamp (stepIdx, 0, 7);
        numBiquads = 0;
        hasFirstOrder = false;

        if (slopeStep == 0) // 0 dB/oct (Off / Bypass)
            return;

        if (slopeStep == 1) // 6 dB/oct (1st-order RC)
        {
            hasFirstOrder = true;
            stages[0].tuneFirstOrder (type, hz, fs);
        }
        else if (slopeStep == 2) // 12 dB/oct SSL style
        {
            numBiquads = 1;
            stages[0].tune (type, hz, 0.78, 0.0, fs);
        }
        else if (slopeStep == 3) // 18 dB/oct SSL E-Series style
        {
            numBiquads = 1;
            hasFirstOrder = true;
            stages[0].tune (type, hz, 0.84, 0.0, fs);
            stages[1].tuneFirstOrder (type, hz, fs);
        }
        else if (slopeStep == 4) // 24 dB/oct SSL G-Series style
        {
            numBiquads = 2;
            stages[0].tune (type, hz, 0.56, 0.0, fs);
            stages[1].tune (type, hz, 1.34, 0.0, fs);
        }
        else if (slopeStep == 5) // 36 dB/oct (6-pole Butterworth cascade)
        {
            numBiquads = 3;
            constexpr double q6[3] = { 0.51763809, 0.70710678, 1.93185165 };
            for (int k = 0; k < 3; ++k) stages[k].tune (type, hz, q6[k], 0.0, fs);
        }
        else if (slopeStep == 6) // 48 dB/oct (8-pole Butterworth cascade)
        {
            numBiquads = 4;
            constexpr double q8[4] = { 0.50979558, 0.60134489, 0.89997622, 2.56291545 };
            for (int k = 0; k < 4; ++k) stages[k].tune (type, hz, q8[k], 0.0, fs);
        }
        else // 7 = Brickwall (96 dB/oct 16-pole Butterworth cascade!)
        {
            numBiquads = 8;
            constexpr double q16[8] = {
                0.50241929, 0.52249861, 0.56694403, 0.64682178,
                0.78815462, 1.06067769, 1.72244710, 5.10114862
            };
            for (int k = 0; k < 8; ++k) stages[k].tune (type, hz, q16[k], 0.0, fs);
        }
    }

    inline double tick (int ch, double x) noexcept
    {
        if (slopeStep == 0)
            return x;
        double y = x;
        for (int k = 0; k < numBiquads; ++k)
            y = stages[k].tick (ch, y);
        if (hasFirstOrder)
            y = stages[numBiquads].tick (ch, y);
        return y;
    }
};

// =========================================================================
// Exact DeEsserStage from Vocals\Source\VocalsDSP.h
// =========================================================================
struct VocalsDeEsser
{
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1L = 0.0f, z2L = 0.0f, z1R = 0.0f, z2R = 0.0f;

        void reset() noexcept { z1L = z2L = z1R = z2R = 0.0f; }

        void setHighPass(double sr, float freq, float q = 0.7071f) noexcept
        {
            const float w0 = 2.0f * 3.14159265358979323846f * std::clamp(freq, 20.0f, static_cast<float>(sr * 0.48));
            const float omega = w0 / static_cast<float>(sr);
            const float cosw = std::cos(omega), sinw = std::sin(omega);
            const float alpha = sinw / (2.0f * std::max(0.1f, q));
            const float a0 = 1.0f + alpha;
            b0 = ((1.0f + cosw) * 0.5f) / a0;
            b1 = -(1.0f + cosw) / a0;
            b2 = ((1.0f + cosw) * 0.5f) / a0;
            a1 = (-2.0f * cosw) / a0;
            a2 = (1.0f - alpha) / a0;
        }

        void setLowPass(double sr, float freq, float q = 0.7071f) noexcept
        {
            const float w0 = 2.0f * 3.14159265358979323846f * std::clamp(freq, 20.0f, static_cast<float>(sr * 0.48));
            const float omega = w0 / static_cast<float>(sr);
            const float cosw = std::cos(omega), sinw = std::sin(omega);
            const float alpha = sinw / (2.0f * std::max(0.1f, q));
            const float a0 = 1.0f + alpha;
            b0 = ((1.0f - cosw) * 0.5f) / a0;
            b1 = (1.0f - cosw) / a0;
            b2 = ((1.0f - cosw) * 0.5f) / a0;
            a1 = (-2.0f * cosw) / a0;
            a2 = (1.0f - alpha) / a0;
        }

        void setBandPass(double sr, float freq, float q = 1.10f) noexcept
        {
            const float w0 = 2.0f * 3.14159265358979323846f * std::clamp(freq, 20.0f, static_cast<float>(sr * 0.48));
            const float omega = w0 / static_cast<float>(sr);
            const float cosw = std::cos(omega), sinw = std::sin(omega);
            const float alpha = sinw / (2.0f * std::max(0.1f, q));
            const float a0 = 1.0f + alpha;
            b0 = alpha / a0;
            b1 = 0.0f;
            b2 = -alpha / a0;
            a1 = (-2.0f * cosw) / a0;
            a2 = (1.0f - alpha) / a0;
        }

        inline float processL(float x) noexcept
        {
            const float y = b0 * x + z1L;
            z1L = b1 * x - a1 * y + z2L;
            z2L = b2 * x - a2 * y;
            return y;
        }

        inline float processR(float x) noexcept
        {
            const float y = b0 * x + z1R;
            z1R = b1 * x - a1 * y + z2R;
            z2R = b2 * x - a2 * y;
            return y;
        }
    };

    double sampleRate = 44100.0;
    float lastFreq = -1.0f;
    int lastScType = -1;

    Biquad scFilterL, scFilterR;
    Biquad splitLowL, splitLowR;
    Biquad splitHighL, splitHighR;

    float envDb = -96.0f;
    float currentAttenDb = 0.0f;
    float scDetectorDb = -96.0f;

    static inline float dbToGain(float db) noexcept
    {
        return (db <= -96.0f) ? 0.0f : std::pow(10.0f, db * 0.05f);
    }

    static inline float gainToDb(float g) noexcept
    {
        return 20.0f * std::log10(std::max(g, 1.0e-5f));
    }

    void prepare(double sr) noexcept
    {
        sampleRate = (sr > 8000.0) ? sr : 44100.0;
        lastFreq = -1.0f;
        lastScType = -1;
        scFilterL.reset();
        scFilterR.reset();
        splitLowL.reset();
        splitLowR.reset();
        splitHighL.reset();
        splitHighR.reset();
        envDb = -96.0f;
        currentAttenDb = 0.0f;
        scDetectorDb = -96.0f;
    }

    void updateFilters(float freqHz, int scType) noexcept
    {
        if (std::abs(freqHz - lastFreq) < 1.0f && scType == lastScType)
            return;

        lastFreq = freqHz;
        lastScType = scType;

        if (scType == 0)
        {
            scFilterL.setHighPass(sampleRate, freqHz, 0.85f);
            scFilterR.setHighPass(sampleRate, freqHz, 0.85f);
        }
        else
        {
            scFilterL.setBandPass(sampleRate, freqHz, 2.0f);
            scFilterR.setBandPass(sampleRate, freqHz, 2.0f);
        }

        splitLowL.setLowPass(sampleRate, freqHz, 0.7071f);
        splitLowR.setLowPass(sampleRate, freqHz, 0.7071f);
        splitHighL.setHighPass(sampleRate, freqHz, 0.7071f);
        splitHighR.setHighPass(sampleRate, freqHz, 0.7071f);
    }

    void processSample(double& left, double& right,
                       float freqHz,
                       float threshDb,
                       int scType,
                       int audioMode,
                       bool monitorSc,
                       bool enabled) noexcept
    {
        updateFilters(freqHz, scType);

        float lF = static_cast<float>(left);
        float rF = static_cast<float>(right);
        const float scL = scFilterL.processL(lF);
        const float scR = scFilterR.processR(rF);
        const float scPeak = std::max(std::abs(scL), std::abs(scR));
        scDetectorDb = gainToDb(scPeak);

        if (!enabled)
        {
            currentAttenDb = 0.0f;
            return;
        }

        const float attCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.0005f));
        const float relCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.0320f));
        const float coeff = (scDetectorDb > envDb) ? attCoeff : relCoeff;
        envDb = coeff * envDb + (1.0f - coeff) * scDetectorDb;

        float grDb = 0.0f;
        if (envDb > threshDb)
        {
            const float over = envDb - threshDb;
            grDb = -std::min(24.0f, over * 0.8571428f);
        }

        currentAttenDb = -grDb;
        const float grLin = dbToGain(grDb);

        if (monitorSc)
        {
            left = static_cast<double>(scL);
            right = static_cast<double>(scR);
            return;
        }

        if (audioMode == 0)
        {
            const float lowL = splitLowL.processL(lF);
            const float lowR = splitLowR.processR(rF);
            const float highL = splitHighL.processL(lF) * grLin;
            const float highR = splitHighR.processR(rF) * grLin;
            left = static_cast<double>(lowL + highL);
            right = static_cast<double>(lowR + highR);
        }
        else
        {
            left *= static_cast<double>(grLin);
            right *= static_cast<double>(grLin);
        }
    }
};

// =========================================================================
// Exact Inflator DSP from OUTPUT (Phobos67Processor::inflateCore & clip)
// =========================================================================
struct OutputInflator
{
    double effectCurrent = 0.0;
    double curveCurrent  = 0.5;
    double nativeAlpha   = 1.0 / 256.0;

    void prepare (double sr) noexcept
    {
        int rateFactor = 1;
        while (sr > 48000.0 * rateFactor)
            rateFactor *= 2;
        nativeAlpha = 1.0 / (256.0 * rateFactor);
    }

    static double inflateCore (double z, double c, double e) noexcept
    {
        z = juce::jlimit (-1.9999998, 1.9999998, z);
        if (e == 0.0) return z;
        const double a = std::abs (z);
        const double y = a <= 1.0
            ? a * ((1.0 + c) + a * ((0.75 - 1.5 * c - 0.25 * c * c) + a * ((-0.5 + 0.5 * c * c) + a * (-0.25 * (1.0 - c) * (1.0 - c)))))
            : 1.0 - (a - 1.0) * (a - 1.0);
        return (1.0 - e) * z + e * std::copysign (y, z) * 0.9999995529651642;
    }

    inline double processSample (double x, double c, double e, bool clip0dB) noexcept
    {
        const double z = juce::jlimit (clip0dB ? -1.0 : -1.9999998, clip0dB ? 1.0 : 1.9999998, x);
        const double y = (e > 0.000001) ? inflateCore (z, c, e) : z;
        return clip0dB ? juce::jlimit (-1.0, 1.0, y) : y;
    }
};

// =========================================================================
// Warm Studio Console Pink/Tape Analog Noise Floor calibrated to -128 dBFS RMS
// Uses Paul Kellet's 3-pole pinking filter + gentle high-frequency silk rolloff
// with independent decorrelated L/R xorshift64* generators.
// =========================================================================
struct WarmAnalogNoise
{
    uint64_t seed[2] { 0x6A09E667F3BCC908ULL, 0xBB67AE8584CAA73BULL };
    double b0[2]{}, b1[2]{}, b2[2]{}, lp[2]{};

    void reset() noexcept
    {
        b0[0] = b0[1] = b1[0] = b1[1] = b2[0] = b2[1] = lp[0] = lp[1] = 0.0;
    }

    inline double nextWhite (int ch) noexcept
    {
        uint64_t x = seed[ch & 1];
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        seed[ch & 1] = x;
        const uint64_t u = (x * 0x2545F4914F6CDD1DULL) >> 11;
        return (static_cast<double> (u) * (1.0 / 9007199254740992.0)) * 2.0 - 1.0;
    }

    inline double tick (int ch) noexcept
    {
        const int c = ch & 1;
        // Triangular PDF dithered white excitation
        const double w = 0.5 * (nextWhite (c) + nextWhite (c));
        // Paul Kellet 3-pole 1/f pinking network for organic console warmth
        b0[c] = 0.99765 * b0[c] + w * 0.0990460;
        b1[c] = 0.96300 * b1[c] + w * 0.2965164;
        b2[c] = 0.57000 * b2[c] + w * 1.0526913;
        const double pink = b0[c] + b1[c] + b2[c] + w * 0.1848;
        // Gentle analog tape/transformer top-end silk smoothing
        lp[c] = 0.62 * lp[c] + 0.38 * pink;
        // Normalized to exact -128.00 dBFS RMS (10^(-128/20) = 3.9810717055e-7; raw lp RMS is 1.1355)
        constexpr double kTargetRms128dB = 3.981071705534973e-7;
        constexpr double kPinkRmsNorm    = 1.0 / 1.1355;
        return lp[c] * (kTargetRms128dB * kPinkRmsNorm);
    }
};

class Strip final : public juce::AudioProcessor
{
public:
    Strip() : AudioProcessor (BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
                  .withInput("Sidechain",juce::AudioChannelSet::stereo(),false)
                  .withOutput("Output",juce::AudioChannelSet::stereo(),true)), state(*this,nullptr,"ChannelStrip",layout())
    { for (int i=0;i<count;++i) raw[i]=state.getRawParameterValue(ids[i]); }
    const juce::String getName() const override { return "ChannelStrip"; }
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        const auto main=l.getMainInputChannelSet(), sc=l.getChannelSet(true,1);
        return main==l.getMainOutputChannelSet() && (main==juce::AudioChannelSet::mono() || main==juce::AudioChannelSet::stereo())
            && (sc.isDisabled() || sc==juce::AudioChannelSet::mono() || sc==juce::AudioChannelSet::stereo());
    }
    void prepareToPlay (double rate,int maxBlockSize) override
    {
        fs=rate;
        flHeatDsp.prepare(rate, std::max(64, maxBlockSize));
        reset();
        for (auto* g : { &trim,&output,&heat }) g->reset(rate,0.015);
        // Exact OUTPUT Phobos67Processor Sculp Even/Odd & Mid/Side 50 ms smoothing!
        for (auto* g : { &midSmoothed,&sideSmoothed,&evenSmoothed,&oddSmoothed }) g->reset(rate,0.050);
        trim.setCurrentAndTargetValue(gain(raw[preamp]->load()));
        heat.setCurrentAndTargetValue(gain(raw[drive]->load())-1);
        output.setCurrentAndTargetValue(outputGain(raw[fader]->load(),raw[compMakeup]->load()));
        evenSmoothed.setCurrentAndTargetValue(juce::roundToInt(raw[evenHarmonics]->load()) * 0.01f);
        oddSmoothed.setCurrentAndTargetValue(juce::roundToInt(raw[oddHarmonics]->load()) * 0.01f);
        midSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(raw[midGain]->load()));
        sideSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(raw[sideGain]->load()));
        inflator.prepare(rate);
        inflator.effectCurrent = raw[inflateEffect]->load() / 100.0;
        inflator.curveCurrent  = (raw[inflateCurve]->load() + 50.0) / 100.0;
    }
    void reset() override
    {
        filters={}; scFilters={}; hpSlope.clear(); lpSlope.clear(); scHpSlope.clear(); scLpSlope.clear();
        flHeatDsp.reset();
        lastModel=-1; reduction=attenuation=power=0;
        vocalsDeEsser.prepare(fs);
        inflator.prepare(fs);
    }
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals guard;
        std::array<float,count> v;
        for (int i=0;i<count;++i) v[i]=raw[i]->load(std::memory_order_relaxed);
        auto main=getBusBuffer(buffer,false,0);
        auto sc=getBusBuffer(buffer,true,1);
        const int channels=main.getNumChannels();
        const int numSamples=main.getNumSamples();

        // 0. High-gain Input Trim (-24 to +48 dB) + Phase Invert + Pad (-20 dB) BEFORE Input Meter & BEFORE FL HEAT!
        const double padSign = (v[phaseInv]>0.5f ? -1.0 : 1.0) * (v[pad20]>0.5f ? 0.1 : 1.0);
        trim.setTargetValue(gain(v[preamp]) * padSign);
        float rawInL = 0.0f, rawInR = 0.0f;
        double sumSqInL = 0.0, sumSqInR = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            const float inGain = static_cast<float>(trim.getNextValue());
            for (int ch = 0; ch < channels; ++ch)
            {
                const float s = main.getSample(ch, i) * inGain;
                main.setSample(ch, i, s);
                const float absS = std::abs(s);
                if (ch == 0) { rawInL = std::max(rawInL, absS); sumSqInL += double(s) * double(s); }
                else         { rawInR = std::max(rawInR, absS); sumSqInR += double(s) * double(s); }
            }
        }
        if (channels == 1) { rawInR = rawInL; sumSqInR = sumSqInL; }
        const float blockRmsInL = numSamples > 0 ? static_cast<float>(std::sqrt(sumSqInL / numSamples)) : 0.0f;
        const float blockRmsInR = numSamples > 0 ? static_cast<float>(std::sqrt(sumSqInR / numSamples)) : 0.0f;

        // 1. Color Stage right after Input Trim:
        //    - When heatBypass == false (HEAT mode): run exact 1:1 FL HEAT (Ocelot LUT + Drive/Tone)
        //    - When heatBypass == true  (EVEN/ODD mode): run exact 1:1 OUTPUT Even & Odd Harmonics exciter!
        const float hDrive     = v[heatDrive];
        const float hTone      = v[heatTone];
        const bool  evenOddMode = v[heatBypass] > 0.5f;
        const int   osMode     = juce::jlimit(0, 3, int(v[oversampling]));
        flHeatDsp.setParameters(hDrive, hTone, evenOddMode, osMode);
        if (!evenOddMode && std::abs(hDrive) > 0.001f)
            flHeatDsp.processBlock(main);
        evenSmoothed.setTargetValue(evenOddMode ? (juce::roundToInt(v[evenHarmonics]) * 0.01f) : 0.0f);
        oddSmoothed.setTargetValue(evenOddMode  ? (juce::roundToInt(v[oddHarmonics])  * 0.01f) : 0.0f);

        const bool external=v[extSidechain]>0.5f && sc.getNumChannels()>0;
        const bool rmsDetector=v[compMode]>0.5f;
        const int model=int(v[preampMode]);
        const bool mic=model==2;
        if (model!=lastModel)
        {
            transformer={}; flux={}; dcIn={}; dcOut={};
            transformer[0].tune(0,14.5,0.72,0,fs);
            transformer[1].tune(3,mic?130:95,mic?0.60:0.58,mic?0.65:0.45,fs);
            transformer[2].tune(3,std::min(13500.,fs*0.4),0.75,0.45,fs);
            transformer[3].tune(1,21500,0.68,0,fs);
            lastModel=model;
        }
        const double stageDb=v[drive]*(mic?60.:20.)/24.;
        const double strength=juce::jlimit(0.05,1.,(stageDb+10)/50);
        const double bias=.135*strength, offset=std::tanh(bias), slope=1-offset*offset;
        const double fluxPole=std::exp(-juce::MathConstants<double>::twoPi*115/fs);
        const double preGain=gain(stageDb), autoTrim=gain(-0.92*stageDb);
        const double freq[]={v[hpf],v[lpf],v[lowFreq],v[lowMidFreq],v[highMidFreq],v[highFreq]};
        const double lfGain = v[lfBell]>0.5f ? std::round(double(v[lowGain])) : double(v[lowGain]);
        const double hfGain = v[hfBell]>0.5f ? std::round(double(v[highGain])) : double(v[highGain]);
        const double eq[]={0,0,lfGain,v[lowMidGain],v[highMidGain],hfGain};
        const bool brownEQ=v[eqMode]>0.5f;
        // Super-musical SSL 242 Black & 02 Brown proportional-Q & shelf shaping:
        // Small boosts/cuts stay wide, silky and phase-smooth; deeper boosts/cuts focus Q musically!
        auto musicalBellQ = [brownEQ](double userQ, double gainDb) noexcept -> double
        {
            const double absG = std::abs(gainDb);
            const double propFactor = brownEQ ? (0.48 + 0.015 * absG) : (0.85 + 0.032 * absG);
            return std::clamp(userQ * propFactor, 0.25, 10.0);
        };
        const double shelfSlope = brownEQ ? 0.58 : 0.90;
        const int types[]={0,1,v[lfBell]>0.5f ? 3 : 2,3,4,v[hfBell]>0.5f ? 4 : 5};
        const double q[]={
            0.70710678,
            0.70710678,
            v[lfBell]>0.5f ? musicalBellQ(0.95, lfGain) : shelfSlope,
            musicalBellQ(v[lowMidQ], v[lowMidGain]),
            musicalBellQ(v[highMidQ], v[highMidGain]),
            v[hfBell]>0.5f ? musicalBellQ(0.95, hfGain) : shelfSlope
        };
        const bool eqActive = v[eqIn]>0.5f;
        const bool fltInSc = v[fltDynSc]>0.5f;
        const bool eqInSc = eqActive && v[eqDynSc]>0.5f;
        const bool fltPre = v[fltPreDyn]>0.5f && !fltInSc;
        const bool eqPre = eqActive && v[eqPreDyn]>0.5f && !eqInSc;
        const int legacySlope = int(v[filterSlope]); // 0=6, 1=12, 2=18, 3=24 (default 2)
        int hpStep = int(v[hpSlopeParam]);
        int lpStep = int(v[lpSlopeParam]);
        // If legacy filterSlope was changed from default (2), honor it when hpStep/lpStep are at default (3)
        if (legacySlope != 2 && hpStep == 3 && lpStep == 3)
        {
            hpStep = legacySlope + 1;
            lpStep = legacySlope + 1;
        }
        const bool on[]={v[hpf]>20 && hpStep>0, v[lpf]<20000 && lpStep>0, eqActive && eq[2]!=0, eqActive && eq[3]!=0, eqActive && eq[4]!=0, eqActive && eq[5]!=0};

        if (on[0]) { hpSlope.tune(0, freq[0], hpStep, fs); scHpSlope.tune(0, freq[0], hpStep, fs); }
        else       { hpSlope.clear(); scHpSlope.clear(); }

        if (on[1]) { lpSlope.tune(1, freq[1], lpStep, fs); scLpSlope.tune(1, freq[1], lpStep, fs); }
        else       { lpSlope.clear(); scLpSlope.clear(); }

        for (int j=2;j<6;++j)
        {
            if (on[j])
            {
                filters[j].tune(types[j],freq[j],q[j],eq[j],fs);
                scFilters[j].tune(types[j],freq[j],q[j],eq[j],fs);
            }
            else
            {
                filters[j]={};
                scFilters[j]={};
            }
        }
        heat.setTargetValue(gain(v[drive])-1);
        output.setTargetValue(outputGain(v[fader],v[compMakeup]));
        // 1:1 OUTPUT Sculp Mid/Side target gain (juce::Decibels::decibelsToGain(value("mid")) / ("side"))
        midSmoothed.setTargetValue(juce::Decibels::decibelsToGain(v[midGain]));
        sideSmoothed.setTargetValue(juce::Decibels::decibelsToGain(v[sideGain]));
        const double pole1000 = -1000.0 / fs;
        const auto ca = std::exp(pole1000 / 0.1);
        const auto cr = std::exp(pole1000 / double(v[compRelease]));
        const auto ga = ca;
        const auto gr = std::exp(pole1000 / double(v[gateRelease]));
        const auto rms = std::exp(pole1000 / 10.0);
        const bool dynActive = v[dynIn]>0.5f;
        const bool cmpActive = dynActive && v[cmpIn]>0.5f && (v[compRatio] > 1.001f);
        const bool expActive = dynActive && v[expIn]>0.5f && (v[gateRange] > 0.01f);
        const bool dynNeedsDetector = cmpActive || expActive;
        const double compRatioInv = 1.0 - 1.0 / double(v[compRatio]);
        const double compThrLin   = juce::Decibels::decibelsToGain(double(v[compThreshold]), -160.0);
        const double gateThrLin   = juce::Decibels::decibelsToGain(double(v[gateThreshold]), -160.0);
        const float dsF = v[dsFreq];
        const float dsT = v[dsThresh];
        const int dsSc = int(v[dsScFilter]);
        const int dsMode = int(v[dsAudioMode]);
        const bool dsMon = v[dsMonitor]>0.5f;
        const bool dsPre = v[dsPrePost]>0.5f;
        // De-Esser is active when deEss is true and threshold < 0.0 dB (at 0.0 dB threshold it is 100% transparent for signals <= 0 dBFS!)
        const bool deEssActive = (v[deEss] > 0.5f) && (dsT < -0.05f || dsMon);

        const double targetInfEffect = double(v[inflateEffect]) / 100.0;
        const double targetInfCurve  = (double(v[inflateCurve]) + 50.0) / 100.0;
        const bool clip0dB           = v[inflateClip] > 0.5f;
        const bool analogOn          = v[analogNoise] > 0.5f;
        const bool msActive          = (std::abs(v[midGain]) > 0.001f || std::abs(v[sideGain]) > 0.001f
                                     || midSmoothed.isSmoothing() || sideSmoothed.isSmoothing());

        float blkInPeak=0.f, blkOutPeak=0.f, blkEqPeak=0.f, blkOutL=0.f, blkOutR=0.f;
        double sumSqOutL=0.0, sumSqOutR=0.0;
        for (int i=0;i<numSamples;++i)
        {
            double x[2]{},peak=0;
            const auto k=heat.getNextValue();
            const float vEven = static_cast<float>(evenSmoothed.getNextValue());
            const float vOdd  = static_cast<float>(oddSmoothed.getNextValue());
            const float mGain = static_cast<float>(midSmoothed.getNextValue());
            const float sGain = static_cast<float>(sideSmoothed.getNextValue());
            inflator.effectCurrent += (targetInfEffect - inflator.effectCurrent) * inflator.nativeAlpha;
            inflator.curveCurrent  += (targetInfCurve  - inflator.curveCurrent)  * inflator.nativeAlpha;
            for (int ch=0;ch<channels;++ch)
            {
                auto y=double(main.getSample(ch,i));
                // 1:1 OUTPUT Even & Odd Harmonics exciter when in EVEN/ODD mode:
                // if (v[1]==0.f && v[2]==0.f) x+=0.f; else x = (x + (2*v[1])*(x*x) + (2*v[2])*(x*x*x)) * (1.f / (1 + 1.2f*(v[1]+v[2])));
                if (vEven > 0.0f || vOdd > 0.0f)
                {
                    const float xf = static_cast<float>(y);
                    const float norm = 1.0f / (1.0f + 1.2f * (vEven + vOdd));
                    y = static_cast<double>((xf + (2.0f * vEven) * (xf * xf) + (2.0f * vOdd) * (xf * xf * xf)) * norm);
                }
                if (model!=0)
                {
                    y=transformer[1].tick(ch,transformer[0].tick(ch,y))*preGain;
                    flux[ch]=fluxPole*flux[ch]+(1-fluxPole)*y;
                    y=(std::tanh(y+.055*strength*std::tanh(1.3*flux[ch])+bias)-offset)/slope;
                    if(mic && stageDb>=35) y=std::tanh(1.15*y)/1.15;
                    const double ac=y-dcIn[ch]+.9995*dcOut[ch];
                    dcIn[ch]=y; dcOut[ch]=ac;
                    y=transformer[3].tick(ch,transformer[2].tick(ch,std::tanh(ac*autoTrim*.85)/.85));
                }
                else if (k>0.00001) y=std::tanh(k*y)/k;
                blkInPeak = std::max(blkInPeak, float(std::abs(y)));
                x[ch]=y;
            }
            if (channels == 1) x[1] = x[0];

            // PRE DYN ON: De-Esser runs right after FL HEAT and BEFORE Filters, EQ and Compressor!
            if (deEssActive && dsPre)
                vocalsDeEsser.processSample(x[0], x[1], dsF, dsT, dsSc, dsMode, dsMon, true);
            else if (!deEssActive)
                vocalsDeEsser.currentAttenDb = 0.0f;

            for (int ch=0;ch<channels;++ch)
            {
                double y = x[ch];
                if (fltPre)
                {
                    if (on[0]) y=hpSlope.tick(ch,y);
                    if (on[1]) y=lpSlope.tick(ch,y);
                }
                if (eqPre)
                {
                    for (int j=2;j<6;++j) if (on[j]) y=filters[j].tick(ch,y);
                }
                blkEqPeak = std::max(blkEqPeak, float(std::abs(y)));
                x[ch]=y;
            }
            if (channels == 1) x[1] = x[0];

            double dynGain = 1.0;
            if (dynNeedsDetector || reduction > 1.0e-5 || attenuation > 1.0e-5)
            {
                for(int ch=0;ch<channels;++ch)
                {
                    double det = x[ch];
                    if (fltInSc)
                    {
                        if (on[0]) det=scHpSlope.tick(ch,det);
                        if (on[1]) det=scLpSlope.tick(ch,det);
                    }
                    if (eqInSc)
                    {
                        for (int j=2;j<6;++j) if (on[j]) det=scFilters[j].tick(ch,det);
                    }
                    peak=std::max(peak,std::abs(det));
                }
                if (external)
                {
                    peak=0; for (int ch=0;ch<sc.getNumChannels();++ch) peak=std::max(peak,std::abs(double(sc.getSample(ch,i))));
                }
                power=rms*power+(1-rms)*peak*peak;
                double target = 0.0;
                double gate   = 0.0;
                if ((cmpActive && (rmsDetector ? (power > compThrLin * compThrLin * 0.25) : (peak > compThrLin)))
                    || (expActive && peak < gateThrLin))
                {
                    const double level = juce::Decibels::gainToDecibels(peak,-160.0);
                    if (cmpActive)
                    {
                        double over=(rmsDetector ? juce::Decibels::gainToDecibels(std::sqrt(power),-160.0) : level)-v[compThreshold];
                        if (rmsDetector && over>-3 && over<3) over=(over+3)*(over+3)/12;
                        else over=std::max(0.0,over);
                        target = over * compRatioInv;
                    }
                    if (expActive && level < v[gateThreshold])
                    {
                        gate = (v[gateMode]<0.5f) ? double(v[gateRange]) : std::min(double(v[gateRange]), v[gateThreshold]-level);
                    }
                }
                reduction=target+(target>reduction ? ca : cr)*(reduction-target);
                attenuation=gate+(gate<attenuation ? ga : gr)*(attenuation-gate);
                const double totalGrDb = attenuation + reduction;
                if (totalGrDb > 1.0e-5)
                    dynGain = gain(-totalGrDb);
            }
            const auto faderGain = output.getNextValue();

            double outCh[2]{};
            for (int ch=0;ch<channels;++ch)
            {
                double y = dynGain * x[ch];
                if (!fltPre && !fltInSc)
                {
                    if (on[0]) y=hpSlope.tick(ch,y);
                    if (on[1]) y=lpSlope.tick(ch,y);
                }
                if (eqActive && !eqPre && !eqInSc)
                {
                    for (int j=2;j<6;++j) if (on[j]) y=filters[j].tick(ch,y);
                    blkEqPeak = std::max(blkEqPeak, float(std::abs(y)));
                }
                outCh[ch] = y;
            }
            if (channels == 1) outCh[1] = outCh[0];

            // PRE DYN OFF (Post): De-Esser runs after Comp/EQ and RIGHT BEFORE OUTPUT Sculp Mid/Side!
            if (deEssActive && !dsPre)
                vocalsDeEsser.processSample(outCh[0], outCh[1], dsF, dsT, dsSc, dsMode, dsMon, true);

            // 1:1 OUTPUT Sculp Mid/Side Matrix:
            // float mid=(l+r)*.5f*v[3], side=(l-r)*.5f*v[4]; l=mid+side; r=mid-side;
            if (msActive)
            {
                const float l = static_cast<float>(outCh[0]);
                const float r = static_cast<float>((channels >= 2) ? outCh[1] : outCh[0]);
                const float mid  = (l + r) * 0.5f * mGain;
                const float side = (l - r) * 0.5f * sGain;
                outCh[0] = static_cast<double>(mid + side);
                if (channels >= 2)
                    outCh[1] = static_cast<double>(mid - side);
            }

            for (int ch=0;ch<channels;++ch)
            {
                double y = outCh[ch];
                if (analogOn)
                    y += warmNoise.tick(ch);
                if (targetInfEffect > 0.0001 || inflator.effectCurrent > 0.0001 || clip0dB)
                    y = inflator.processSample(y, inflator.curveCurrent, inflator.effectCurrent, clip0dB);
                // Master Fader Out is the VERY LAST stage before the OUT meter!
                y *= faderGain;
                const float absY = float(std::abs(y));
                blkOutPeak = std::max(blkOutPeak, absY);
                if (ch == 0) { blkOutL = std::max(blkOutL, absY); sumSqOutL += y * y; }
                else         { blkOutR = std::max(blkOutR, absY); sumSqOutR += y * y; }
                main.setSample(ch,i,float(y));
            }
            if (channels == 1) { blkOutR = blkOutL; sumSqOutR = sumSqOutL; }
        }
        const float blockRmsOutL = numSamples > 0 ? static_cast<float>(std::sqrt(sumSqOutL / numSamples)) : 0.0f;
        const float blockRmsOutR = numSamples > 0 ? static_cast<float>(std::sqrt(sumSqOutR / numSamples)) : 0.0f;
        telemetry.inputPeak.store(std::max(rawInL, rawInR), std::memory_order_relaxed);
        telemetry.outputPeak.store(blkOutPeak, std::memory_order_relaxed);
        telemetry.inPeakL.store(rawInL, std::memory_order_relaxed);
        telemetry.inPeakR.store(rawInR, std::memory_order_relaxed);
        telemetry.outPeakL.store(blkOutL, std::memory_order_relaxed);
        telemetry.outPeakR.store(blkOutR, std::memory_order_relaxed);
        telemetry.inRmsL.store(blockRmsInL, std::memory_order_relaxed);
        telemetry.inRmsR.store(blockRmsInR, std::memory_order_relaxed);
        telemetry.outRmsL.store(blockRmsOutL, std::memory_order_relaxed);
        telemetry.outRmsR.store(blockRmsOutR, std::memory_order_relaxed);
        telemetry.compReductionDb.store(float(reduction), std::memory_order_relaxed);
        telemetry.gateAttenuationDb.store(float(attenuation), std::memory_order_relaxed);
        telemetry.deEssReductionDb.store(vocalsDeEsser.currentAttenDb, std::memory_order_relaxed);
        telemetry.deEssDetectorDb.store(vocalsDeEsser.scDetectorDb, std::memory_order_relaxed);
        telemetry.inputOverload.store(blkInPeak >= 0.98f, std::memory_order_relaxed);
        telemetry.eqOverload.store(blkEqPeak >= 0.98f, std::memory_order_relaxed);
    }
    bool acceptsMidi() const override {return false;}
    bool producesMidi() const override {return false;}
    bool isMidiEffect() const override {return false;}
    double getTailLengthSeconds() const override {return 0;}
    int getNumPrograms() override {return 1;}
    int getCurrentProgram() override {return 0;}
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override {return {};}
    void changeProgramName(int,const juce::String&) override {}
    bool hasEditor() const override {return true;}
    juce::AudioProcessorEditor* createEditor() override {return makeEditor(*this,state);}
    void getStateInformation(juce::MemoryBlock& b) override
    { if(auto xml=state.copyState().createXml()) copyXmlToBinary(*xml,b); }
    void setStateInformation(const void* data,int bytes) override
    { if(auto xml=getXmlFromBinary(data,bytes)) if(xml->hasTagName(state.state.getType()) || xml->hasTagName("SurcoE")) state.replaceState(juce::ValueTree::fromXml(*xml)); }
    SurcoTelemetry& getTelemetry() noexcept { return telemetry; }
private:
    static double gain(double db) {return juce::Decibels::decibelsToGain(db,-160.0);}
    static double outputGain(float f,float makeup) {return f<=-80 ? 0.0 : gain(f+makeup);}
    State state;
    SurcoTelemetry telemetry;
    std::array<std::atomic<float>*,count> raw{};
    std::array<Filter,6> filters{};
    std::array<Filter,6> scFilters{};
    SlopeFilter hpSlope{}, lpSlope{}, scHpSlope{}, scLpSlope{};
    std::array<Filter,4> transformer{};
    std::array<double,2> flux{},dcIn{},dcOut{};
    int lastModel=-1;
    flheat::HeatDSP flHeatDsp{};
    VocalsDeEsser vocalsDeEsser{};
    OutputInflator inflator{};
    WarmAnalogNoise warmNoise{};
    juce::SmoothedValue<double> trim,output,heat,midSmoothed,sideSmoothed,evenSmoothed,oddSmoothed;
    double fs=44100,reduction=0,attenuation=0,power=0;
};
}

SurcoTelemetry& getSurcoTelemetry (juce::AudioProcessor& proc)
{
    return static_cast<Strip&>(proc).getTelemetry();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {return new Strip();}
