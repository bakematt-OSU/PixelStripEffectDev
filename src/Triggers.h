#ifndef TRIGGERS_H
#define TRIGGERS_H

#include <Arduino.h>
#include "Sensors.h"      // Requires the Microphone class definition for PEAK mode
#include <ArduinoFFT.h> // Corrected library name

// --- FFT and Audio Sampling Constants ---
#define SAMPLES 512              // Must be a power of 2
#define SAMPLING_FREQUENCY 10000 // Hz, must be > 2 * max frequency to analyse

// Define a "function pointer" type for our callback.
// This is a template for any function that can be triggered.
using TriggerCallback = void (*)(bool isActive, uint8_t value);

class AudioTrigger {
public:
    // Enum to select detection method
    enum DetectionMode { PEAK, BASS };

    // Constructor now takes a detection mode and the analog pin for the mic.
    AudioTrigger(Microphone& mic, DetectionMode mode = PEAK, uint8_t audioPin = A0, int threshold = 400, int peakMax = 1500, int minBrightness = 20)
        : mic_(mic), 
          mode_(mode),
          audioPin_(audioPin),
          threshold_(threshold), 
          peakMax_(peakMax), 
          minBrightness_(minBrightness),
          callback_(nullptr),
          FFT() // Initialize the FFT object correctly.
    {
        // Calculate the sampling period in microseconds for FFT
        sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
    }

    // Method to register a function that will be called when the trigger fires.
    void onTrigger(TriggerCallback cb) {
        callback_ = cb;
    }

    // This method should be called repeatedly in the main loop.
    void update() {
        if (!callback_) {
            return; // Nothing to do if no callback is registered
        }
        
        if (mode_ == PEAK) {
            updateWithPeakDetection();
        } else { // mode_ == BASS
            updateWithBassDetection();
        }
    }

    // --- Configuration Methods ---
    void setThreshold(int newThreshold) {
        threshold_ = newThreshold;
    }
    
    void setDetectionMode(DetectionMode newMode) {
        mode_ = newMode;
    }

private:
    // Peak-detection logic.
    void updateWithPeakDetection() {
        if (!mic_.available()) return;

        int peak = mic_.readPeak();

        if (peak >= threshold_) {
            int value = map(peak, threshold_, peakMax_, minBrightness_, 255);
            callback_(true, constrain(value, minBrightness_, 255));
        } else {
            callback_(false, 0);
        }
    }

    // FFT bass-detection logic.
    void updateWithBassDetection() {
        // 1. Collect audio samples
        unsigned long microseconds = micros();
        for (int i = 0; i < SAMPLES; i++) {
            vReal[i] = analogRead(audioPin_);
            vImag[i] = 0;
            while (micros() - microseconds < sampling_period_us) {
                // wait
            }
            microseconds += sampling_period_us;
        }

        // 2. Perform FFT
        // CORRECTED: The method names use camelCase.
        FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
        FFT.complexToMagnitude(vReal, vImag, SAMPLES);

        // 3. Analyze bass frequency bins
        double bassMagnitude = 0;
        for (int i = 3; i < 14; i++) {
            bassMagnitude += vReal[i];
        }

        // 4. Trigger callback
        if (bassMagnitude > threshold_) {
            int value = map(bassMagnitude, threshold_, peakMax_, minBrightness_, 255);
            callback_(true, constrain(value, minBrightness_, 255));
        } else {
            callback_(false, 0);
        }
    }

    // --- Member Variables ---
    Microphone& mic_;
    DetectionMode mode_;
    uint8_t audioPin_;
    int threshold_;
    int peakMax_;
    int minBrightness_;
    TriggerCallback callback_;

    // --- FFT Specific Variables ---
    // The ArduinoFFT class is a template, it needs the data type.
    ArduinoFFT<double> FFT; 
    unsigned int sampling_period_us;
    double vReal[SAMPLES];
    double vImag[SAMPLES];
};

#endif // TRIGGERS_H
