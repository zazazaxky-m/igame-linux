// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "realfft.h"

#include <QColor>
#include <QString>
#include <QVector>

#include <complex>
#include <deque>
#include <vector>

// Audio -> one LED color. No event loop, so tests can feed it recorded/synthetic audio.
// Two analysis paths:
//  - 1024 FFT every 512 samples: meters, kicks/onsets, tempo + beat grid, Smart Sound features
//  - 4096 FFT every 800 samples (60/s): LedFx-style melbank that drives the LedFx effects
class AudioAnalyzer
{
public:
    static constexpr int kRate = 48000;
    static constexpr int kPhaseSlots = 32;
    static constexpr int kMelBands = 24;

    AudioAnalyzer();

    void reset();
    void setMode(const QString &effect); // music, music_smart, music_spectrum, ...
    void setSensitivity(int percent);    // 10..300
    void setSpeed(double speed);         // 0.1..5, reaction speed
    void setColors(const QVector<QColor> &colors);

    void feed(const float *samples, size_t count);
    QColor render(double dt);

    double level() const { return m_loud; }
    double band(int i) const { return m_band[i]; }
    int beatCount() const { return m_beatCount; }
    double bpm() const { return m_bpm; }
    double tempoConfidence() const { return m_tempoConfidence; }
    bool locked() const { return m_locked; }
    const QString &style() const { return m_style; }

    // for tests/tools
    struct BeatEvent { double time; double strength; bool real; };
    std::vector<BeatEvent> takeBeatEvents();

private:
    void analyze(const float *x);
    void detectKick(double t, double rise, double kickDb);
    void updateBeatGrid(double t, double attack);
    void fireBeat(double t, double strength, bool real);
    void estimateTempo();
    void classify(double dt);
    void melFrame();
    double beatOscillator() const; // 0..1 within the beat, -1 = no tempo yet
    QColor draw(const QString &mode, const QVector<QColor> &palette) const;
    double audioTime() const;

    // settings
    QString m_effect = QStringLiteral("music");
    double m_sens = 1.0;
    double m_speed = 1.0;
    QVector<QColor> m_colors;

    // input
    std::vector<float> m_buffer;
    std::vector<float> m_window, m_windowed, m_mags;
    RealFft m_spectrum{1024};
    std::vector<float> m_logMag[2]; // previous two hops
    qint64 m_hops = 0;

    // loudness / bands, 0..1
    double m_ceilDb = -30, m_bandCeilDb[3] = {-40, -40, -40}, m_kickCeilDb = -40;
    double m_power = 0, m_loud = 0, m_loudSlow = 0, m_band[3] = {0, 0, 0};
    double m_silentSeconds = 0;

    // onsets and kicks
    double m_kickDbHist[2] = {-120, -120};
    std::deque<float> m_kickRiseHist, m_fluxHist;
    double m_lastKick = -100, m_lastOnset = -100;
    std::deque<double> m_onsetTimes; // last 5 s
    std::deque<float> m_odf;          // ~6 s, for tempo

    // tempo and beat grid
    double m_bpm = 0, m_tempoConfidence = 0, m_pendingBpm = 0;
    int m_pendingVotes = 0;
    bool m_locked = false;
    double m_phaseHist[kPhaseSlots] = {};
    double m_phase = 0;      // in beats
    double m_beatPhase = 0;
    double m_confirm = 0;
    double m_histPeriod = 1;
    double m_lastBeatTime = -100;
    double m_pendingBeat = 0;
    std::vector<BeatEvent> m_events;

    // melbank
    std::vector<float> m_melRing;
    size_t m_melWrite = 0, m_melFilled = 0;
    int m_melSinceFrame = 0;
    std::vector<float> m_melWindow, m_melWindowed, m_melMag;
    RealFft m_melSpectrum{4096};
    struct MelFilter {
        int first = 0;
        std::vector<float> weights;
        double hz = 0;
    };
    std::vector<MelFilter> m_melFilters;
    double m_melGain = 0.1;
    double m_mel[kMelBands] = {}, m_melCommon[kMelBands] = {}, m_melFiltered[kMelBands] = {};
    double m_lowsRaw = 0, m_lows = 0, m_mids = 0, m_highs = 0;
    bool m_melOnset = false, m_melKick = false;
    double m_melTime = 0;

    // LedFx effect state
    double m_energy[3] = {}, m_scroll[3] = {};
    double m_powerBass = 0, m_powerOverlay = 0, m_powerOverlayPos = 0, m_sparks = 0;
    double m_barColor = 0, m_lastOsc = 0;
    int m_barPhase = 0, m_barBeat = 0;
    double m_rsColor = 0, m_rsShiftTime = 0, m_rsBass = 0, m_rsBassColor = 0, m_rsStrobe = 0, m_rsLastBass = -100;

    // output state
    double m_time = 0;
    double m_swell = 0;
    int m_beatCount = 0;

    // Smart Sound
    double m_lowRatio = 0.3, m_highRatio = 0.1, m_centroid = 1500;
    double m_dbMean = -30, m_dynamics = 4;
    QString m_style = QStringLiteral("pop"), m_previousStyle = QStringLiteral("pop"), m_candidate;
    double m_candidateTime = 0, m_styleBlend = 1, m_listenTime = 0;
};
