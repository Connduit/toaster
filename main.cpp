#include <rtl-sdr.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <csignal>
#include <fstream>
#include <iostream>

constexpr uint32_t CENTER_FREQ = 99'900'000;
constexpr uint32_t SAMPLE_RATE = 2'400'000; // samples/second
constexpr uint32_t AUDIO_RATE = 48'000; // final audio samples/second (this is a standard audio sample rate)

constexpr int GAIN = 300; // tuner gain
constexpr size_t BUFFER_SIZE = 16 * 16384; // how large each chunk of sdr data is

// 2.4 MHz / 48 kHz
constexpr int DECIMATION = SAMPLE_RATE / AUDIO_RATE; // decimation == down sampling

// NOTE: each iq sample is 2 bytes

rtlsdr_dev_t* device = nullptr;
std::ofstream wav;

uint32_t audio_samples = 0;

std::complex<float> previous(1.0f, 0.0f);

// -----------------------------------------------------------------------------
// WAV helpers
// -----------------------------------------------------------------------------

void write_u16(uint16_t value)
{
    wav.put(value & 0xff);
    wav.put((value >> 8) & 0xff);
}

void write_u32(uint32_t value)
{
    wav.put(value & 0xff);
    wav.put((value >> 8) & 0xff);
    wav.put((value >> 16) & 0xff);
    wav.put((value >> 24) & 0xff);
}

void write_wav_header()
{
    wav.write("RIFF", 4);
    write_u32(0); // file size is unknown, we leave blank for now

    wav.write("WAVE", 4); // contains wave audio

    wav.write("fmt ", 4); // starts format section
    write_u32(16);			// PCM format information is 16bytes long
    write_u16(1);                    // PCM
    write_u16(1);                    // mono (channel)
    write_u32(AUDIO_RATE);		// this is the sample rate
    write_u32(AUDIO_RATE * 2);  // this is the byte rate
    write_u16(2);				// each sample occupies 2 bytes
    write_u16(16);				// this says 16bits per sample

    wav.write("data", 4);		// starts audio section
    write_u32(0);				// data size is unknown, we leave blank for now
}

// once we stop recording, we use this function to update size of the audio
void fix_wav_header()
{
    uint32_t data_size = audio_samples * 2;
    uint32_t file_size = 36 + data_size; // calculate the file size

    wav.seekp(4); // move to the correct position in the file to write to in order to update file size
    write_u32(file_size);

    wav.seekp(40); // move to correct position in the file to write to in order to update data size
    write_u32(data_size);
}

// -----------------------------------------------------------------------------
// Simple one-pole low-pass filter
// -----------------------------------------------------------------------------

class LowPass
{
public:

    LowPass(float cutoff, float sample_rate)
    {
        float rc = 1.0f / (2.0f * M_PI * cutoff); // filter's rc time constant
        float dt = 1.0f / sample_rate; // time between samples 

        alpha = dt / (rc + dt); // filter's coffeicent 
    }

	// takes in sample, and returns filtered version of sample 
    float process(float input)
    {
        state += alpha * (input - state);
        return state;
    }

private:

    float alpha = 0; // stores filtered coeffiecnt 
    float state = 0; // stores filter's previous output
};

// TODO: should eventually make FIR (finite impulse response) filter instead of single pole filter?
// Channel filter
LowPass channel_filter(200'000.0f, SAMPLE_RATE);

// Audio filter
LowPass audio_filter(15'000.0f, SAMPLE_RATE);

// -----------------------------------------------------------------------------
// RTL-SDR callback
// -----------------------------------------------------------------------------

void callback(
    unsigned char* buffer, // pointer to raw sdr data
    uint32_t length, // number of bytes in the buffer
    void*) // optional user defined pointer (not using it at the moment)
{
    static int decimation_counter = 0; // this is needed cuz we want to downsample. we really only need 50 input samples per 1 output sample

	// loop though raw bytes 2 at a time (one for I and one for Q)
    for (uint32_t n = 0; n < length; n += 2)
    {
        float i =
            (static_cast<float>(buffer[n]) - 127.5f) / 127.5f; // convert 8bit value into float

        float q =
            (static_cast<float>(buffer[n + 1]) - 127.5f) / 127.5f; // convert 8bit value into float

        std::complex<float> current(i, q); // combine i and q values into a single (complex) value 

        /*
         * Channel filtering.
         *
         * This removes a lot of the RF outside
         * the FM station.
         */
		// TODO: make a class/struct for IQ?
		// filter the iq sample
        current = std::complex<float>(
            channel_filter.process(current.real()),
            channel_filter.process(current.imag())
        );

        /*
         * FM discriminator.
         *
         * Phase difference between consecutive
         * samples gives instantaneous frequency.
         */
		// calculate relative phase between the two samples, and store it as an angle. 
		// this gives us an approximation of the instantaneous frequency deviation
        float audio =
            std::arg(current * std::conj(previous)); 

        previous = current; // save off current sample

        /*
         * Audio filtering.
         */
        audio = audio_filter.process(audio);

        /*
         * Downsample:
         *
         * 2.4 MHz → 48 kHz
         */
        decimation_counter++;

		// once we've processed 50 samples (DECIMATION), we've produced one output audio sample
        if (decimation_counter >= DECIMATION) 
        {
			// reset counter
            decimation_counter = 0;

			// multiply by 20k so audio fits into range of 16bit pcm audio
            float scaled = audio * 20'000.0f;
            //float scaled = audio * 10'000.0f;

			// prevents audio from going out their allowed range (-32768, 32767)
            scaled = std::clamp(
                scaled,
                -32768.0f,
                32767.0f
            );

			// convert floating point audio into 16bit int (which is the .wav format)
            int16_t pcm =
                static_cast<int16_t>(scaled);

			// write sample to file
            write_u16(static_cast<uint16_t>(pcm));

            audio_samples++;
        }
    }
}

// -----------------------------------------------------------------------------
// Ctrl+C
// -----------------------------------------------------------------------------

void signal_handler(int)
{
    if (device)
        rtlsdr_cancel_async(device);
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main()
{
    if (rtlsdr_get_device_count() == 0)
    {
        std::cerr << "No RTL-SDR found\n";
        return 1;
    }

    if (rtlsdr_open(&device, 0) != 0)
    {
        std::cerr << "Failed to open RTL-SDR\n";
        return 1;
    }

    std::cout << "Opening RTL-SDR...\n";

    rtlsdr_set_center_freq(device, CENTER_FREQ); // tune to center freq
    rtlsdr_set_sample_rate(device, SAMPLE_RATE); // set sample rate

    //rtlsdr_set_tuner_gain_mode(device, 1); // set tuner gain to manual mode
    //rtlsdr_set_tuner_gain(device, GAIN); // set tuner gain (manual mode most be enabled)

    rtlsdr_reset_buffer(device); // clear any old samples in the SDR's buffer

    wav.open("output.wav", std::ios::binary); // create file

    if (!wav)
    {
        std::cerr << "Could not create output.wav\n";
        rtlsdr_close(device);
        return 1;
    }

    write_wav_header();

    std::signal(SIGINT, signal_handler);

    std::cout << "Receiving 99.9 MHz...\n";
    std::cout << "Press Ctrl+C to stop.\n";

    rtlsdr_read_async(
        device,
        callback,
        nullptr,
        0,
        BUFFER_SIZE
    );

    fix_wav_header();

    wav.close();

    rtlsdr_close(device);
    device = nullptr;

    std::cout << "Saved output.wav\n";

    return 0;
}