/**
 * @file Sensors.cpp
 * @brief Implementations for Accelerometer, TemperatureSensor, and Microphone.
 */

#include "Sensors.h"
#include <PDM.h>
#include "Debugger.h"

// ——— ACCELEROMETER ———————————————————————————————————————————

/**
 * @brief Initialize and start the accelerometer.
 * @return true on success; false if IMU.begin() fails.
 */
bool Accelerometer::begin()
{
    if (!IMU.begin())
    {
        DBG.print("Accel", 1, "Failed to initialize IMU!");
        return false;
    }
    _sampleRate = IMU.accelerationSampleRate();
    DBG.print("Accel", 1, "Accelerometer sample rate = ");
    DBG.print("Accel", 1, _sampleRate);
    DBG.print("Accel", 1, " Hz");
    DBG.print("Accel", 1, "");
    DBG.print("Accel", 1, "Acceleration in g's");
    DBG.print("Accel", 1, "X\tY\tZ");
    return true;
}

/**
 * @brief Stop the accelerometer.
 */
void Accelerometer::stop()
{
    IMU.end();
    DBG.print("Accel", 1, "Accelerometer stopped");
}

/**
 * @brief Return the configured sample rate.
 * @return Sample rate in Hz.
 */
float Accelerometer::sampleRate() const
{
    DBG.print("Accel", 2,
              String("Sample Rate Requested = ") +
                  String(_sampleRate) + " Hz");
    return _sampleRate;
}

/**
 * @brief Check for new acceleration data.
 * @return true if an acceleration reading is available.
 */
bool Accelerometer::available() const
{
    bool ready = IMU.accelerationAvailable();
    DBG.print("Accel", 3,
              String("accelerationAvailable() → ") +
                  (ready ? "true" : "false"));
    return ready;
}

/**
 * @brief Read X, Y, Z acceleration.
 * @param[out] x  Measured X-axis g value.
 * @param[out] y  Measured Y-axis g value.
 * @param[out] z  Measured Z-axis g value.
 */
void Accelerometer::read(float &x, float &y, float &z)
{
    IMU.readAcceleration(x, y, z);
    DBG.print("Accel", 3,
              String("readAcceleration(): X=") + String(x, 3) +
                  ", Y=" + String(y, 3) +
                  ", Z=" + String(z, 3));
}

/**
 * @brief Read only the X-axis acceleration.
 * @param[out] x  Measured X-axis g value.
 */
void Accelerometer::readX(float &x)
{
    float unusedY, unusedZ;
    IMU.readAcceleration(x, unusedY, unusedZ);
    DBG.print("Accel", 2,
              String("readAcceleration(): X=") + String(x, 3));
}

/**
 * @brief Read only the Y-axis acceleration.
 * @param[out] y  Measured Y-axis g value.
 */
void Accelerometer::readY(float &y)
{
    float unusedX, unusedZ;
    IMU.readAcceleration(unusedX, y, unusedZ);
    DBG.print("Accel", 2,
              String("readAcceleration(): Y=") + String(y, 3));
}

/**
 * @brief Read only the Z-axis acceleration.
 * @param[out] z  Measured Z-axis g value.
 */
void Accelerometer::readZ(float &z)
{
    float unusedX, unusedY;
    IMU.readAcceleration(unusedX, unusedY, z);
    DBG.print("Accel", 2,
              String("readAcceleration(): Z=") + String(z, 3));
}

// ——— TEMPERATURE SENSOR ————————————————————————————————————————

/**
 * @brief Initialize the IMU for temperature readings.
 * @return true if temperature sensor is ready; false otherwise.
 */
bool TemperatureSensor::begin()
{
    if (!IMU.begin())
    {
        DBG.print("Temp", 1, "Failed to initialize IMU for temperature");
        return false;
    }
    if (!IMU.temperatureAvailable())
    {
        DBG.print("Temp", 1, "Temperature sensor not available");
        return false;
    }
    DBG.print("Temp", 1, "Temperature sensor ready");
    return true;
}

/**
 * @brief Stop temperature sampling.
 */
void TemperatureSensor::stop()
{
    IMU.end();
    DBG.print("Temp", 1, "Temperature sensor stopped");
}

/**
 * @brief Check if a fresh temperature reading is ready.
 * @return true if available; false otherwise.
 */
bool TemperatureSensor::available() const
{
    bool ready = IMU.temperatureAvailable();
    DBG.print("Temp", 1,
              String("temperatureAvailable() → ") +
                  (ready ? "true" : "false"));
    return ready;
}

/**
 * @brief Read the temperature in degrees Celsius.
 * @return Temperature in °C.
 */
float TemperatureSensor::readCelsius()
{
    int rawTemp = 0;
    IMU.readTemperature(rawTemp);
    float celsius = static_cast<float>(rawTemp);
    DBG.print("Temp", 1,
              String("Temperature (°C) = ") + String(celsius, 2));
    return celsius;
}

/**
 * @brief Read the temperature in degrees Fahrenheit.
 * @return Temperature in °F.
 */
float TemperatureSensor::readFahrenheit()
{
    float c = readCelsius();
    float f = c * 9.0f / 5.0f + 32.0f;
    DBG.print("Temp", 1,
              String("Temperature (°F) = ") + String(f, 2));
    return f;
}

/**
 * @brief Get a formatted Celsius string.
 * @return String of the form "XX.XX °C".
 */
String TemperatureSensor::readCelsiusString()
{
    float c = readCelsius();
    return String(c, 2) + " °C";
}

/**
 * @brief Get a formatted Fahrenheit string.
 * @return String of the form "XX.XX °F".
 */
String TemperatureSensor::readFahrenheitString()
{
    float f = readFahrenheit();
    return String(f, 2) + " °F";
}

// ——— MICROPHONE ——————————————————————————————————————————————

Microphone *Microphone::_instance = nullptr;

/**
 * @brief Construct and set amplitude threshold.
 * @param threshold  Minimum sample amplitude to keep.
 */
Microphone::Microphone(int threshold)
    : _samplesRead(0),
      _amplitudeThreshold(threshold)
{
    _instance = this;
}

/**
 * @brief Begin PDM sampling at defined SAMPLE_RATE.
 */
void Microphone::begin()
{
    if (!PDM.begin(1, SAMPLE_RATE))
    {
        DBG.print("Mic", 1, "PDM.begin() failed!");
        return;
    }
    PDM.onReceive(Microphone::onPDMDataStatic);
    DBG.print("Mic", 1, "Microphone started");
}

/**
 * @brief End PDM sampling.
 */
void Microphone::stop()
{
    PDM.end();
    DBG.print("Mic", 2, "Microphone stopped");
}

/**
 * @brief Static bridge for PDM data callback.
 */
void Microphone::onPDMDataStatic()
{
    if (_instance)
        _instance->onPDMData();
}

/**
 * @brief Internal handler for incoming PDM data.
 */
void Microphone::onPDMData() {
    static unsigned long lastMicUs = 0;
    unsigned long now = micros();
    if (now - lastMicUs < 5000) return;  // no more than once every 5ms
    lastMicUs = now;

    static bool busy = false;
    if (busy) return;  // Prevent ISR reentry
    busy = true;

    int bytes = PDM.available();
    if (bytes <= 0 || bytes > SAMPLE_BUFFER_SIZE * 2) {
        busy = false;
        return;
    }

    static int16_t tempBuf[SAMPLE_BUFFER_SIZE];
    PDM.read(tempBuf, bytes);

    int samples = bytes / 2;
    int count = 0;
    for (int i = 0; i < samples && count < SAMPLE_BUFFER_SIZE; ++i) {
        int16_t s = tempBuf[i];
        if (abs(s) > _amplitudeThreshold) {
            _sampleBuffer[count++] = s;
        }
    }

    _samplesRead = count;
    busy = false;
}


// void Microphone::onPDMData()
// {
//     int bytes = PDM.available();
//     if (bytes > SAMPLE_BUFFER_SIZE * 2)
//         bytes = SAMPLE_BUFFER_SIZE * 2;

//     // int16_t tempBuf[SAMPLE_BUFFER_SIZE];
//     static int16_t tempBuf[SAMPLE_BUFFER_SIZE];

//     PDM.read(tempBuf, bytes);

//     int samples = bytes / 2;
//     int count = 0;
//     for (int i = 0; i < samples && count < SAMPLE_BUFFER_SIZE; ++i)
//     {
//         int16_t s = tempBuf[i];
//         if (abs(s) > _amplitudeThreshold)
//         {
//             _sampleBuffer[count++] = s;
//         }
//     }

//     _samplesRead = count;
// }
// void Microphone::onPDMData() {
//     int bytes = PDM.available();
//     if (bytes > sizeof(_sampleBuffer)) bytes = sizeof(_sampleBuffer);
//     PDM.read(_sampleBuffer, bytes);
//     _samplesRead = bytes / 2;
// }
// void Microphone::onPDMData()
// {
//    int bytes = PDM.available();
//     if (bytes > sizeof(_sampleBuffer)) bytes = sizeof(_sampleBuffer);

//     // Read directly into a temp buffer
//     int16_t raw[SAMPLE_BUFFER_SIZE];
//     int samples = bytes / 2;
//     PDM.read(raw, bytes);

//     int count = 0;
//     for (int i = 0; i < samples && count < SAMPLE_BUFFER_SIZE; ++i) {
//         if (abs(raw[i]) > _amplitudeThreshold) {
//             _sampleBuffer[count++] = raw[i];  // write only filtered
//         }
//     }

//     _samplesRead = count;
// }

/**
 * @brief Get pointer to the raw sample buffer.
 * @return Pointer to int16_t array of size SAMPLE_BUFFER_SIZE.
 */
const int16_t *Microphone::getBuffer() const
{
    return _sampleBuffer;
}

/**
 * @brief Number of buffered samples passing the threshold.
 * @return Count of samples.
 */
int Microphone::getSampleCount() const
{
    return _samplesRead;
}

/**
 * @brief Check if any samples are available.
 * @return true if getSampleCount() > 0.
 */
bool Microphone::available() const
{
    return _samplesRead > 0;
}

/**
 * @brief Set a new amplitude threshold for filtering.
 * @param threshold  New minimum absolute sample value.
 */
void Microphone::setThreshold(int threshold)
{
    _amplitudeThreshold = threshold;
    DBG.print("Mic", 2,
              String("Threshold set to ") + String(_amplitudeThreshold));
}

/**
 * @brief Compute peak amplitude, then clear the buffer.
 * @return Maximum absolute sample value.
 */
int Microphone::readPeak()
{
    int peak = 0;
    for (int i = 0; i < _samplesRead; ++i)
    {
        int v = abs(_sampleBuffer[i]);
        if (v > peak)
            peak = v;
    }
    _samplesRead = 0;
    DBG.print("Mic", 3,
              String("Peak amplitude = ") + String(peak));
    return peak;
}
