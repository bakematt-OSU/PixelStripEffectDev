#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Arduino_LSM6DSOX.h>

#define SAMPLE_BUFFER_SIZE 512
#define SAMPLE_RATE 8000

class Accelerometer {
public:
    bool begin();
    void stop();
    float sampleRate() const;
    bool available() const;
    void read(float &x, float &y, float &z);
    void readX(float &x);
    void readY(float &y);
    void readZ(float &z);

private:
    int _sampleRate;
};

class TemperatureSensor {
public:
    bool begin();
    void stop();
    bool available() const;
    float readCelsius();
    float readFahrenheit();
    String readCelsiusString();
    String readFahrenheitString();
};

class Microphone {
public:
    // Singleton accessor
    static Microphone &instance() {
        static Microphone inst(400);  // default threshold
        return inst;
    }

    // Disable copy/assign
    Microphone(const Microphone&) = delete;
    void operator=(const Microphone&) = delete;

    // Public API
    void begin();
    void stop();
    const int16_t *getBuffer() const;
    int getSampleCount() const;
    void setThreshold(int threshold);
    bool available() const;
    int readPeak();

private:
    Microphone(int threshold);  // ✅ declared here for singleton

    static void onPDMDataStatic();
    void onPDMData();

    int16_t _sampleBuffer[SAMPLE_BUFFER_SIZE];
    volatile int _samplesRead;
    int _amplitudeThreshold;
    static Microphone *_instance;
};

#endif // SENSORS_H
