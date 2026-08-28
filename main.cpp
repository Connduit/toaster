#include <rtl-sdr.h>

#include <cmath>
#include <complex>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

constexpr uint32_t CENTER_FREQ = 94'000'000;
constexpr uint32_t SAMPLE_RATE = 2'400'000;
constexpr uint32_t AUDIO_RATE  = 48'000;
constexpr int GAIN = 300; // 30.0 dB

constexpr size_t BUFFER_SIZE = 16 * 16384;

std::ofstream wav;
std::complex<float> previous{1.0f, 0.0f};

uint32_t audio_samples = 0;

// Write a little-endian 16-bit integer
void write_u16(std::ofstream& f, uint16_t value)
{
    f.put(value & 0xff);
    f.put((value >> 8) & 0xff);
}

// Write a little-endian 32-bit integer
void write_u32(std::ofstream& f, uint32_t value)
{
    f.put(value & 0xff);
    f.put((value >> 8) & 0xff);
    f.put((value >> 16) & 0xff);
    f.put((value >> 24) & 0xff);
}

void write_wav_header()
{
    wav.write("RIFF", 4);
    write_u32(wav, 0);              // File size - filled in later
    wav.write("WAVE", 4);

    wav.write("fmt ", 4);
    write_u32(wav, 16);             // PCM header size
    write_u16(wav, 1);              // PCM
    write_u16(wav, 1);              // Mono
    write_u32(wav, AUDIO_RATE);
    write_u32(wav, AUDIO_RATE * 2); // Byte rate
    write_u16(wav, 2);              // Block align
    write_u16(wav, 16);             // Bits/sample

    wav.write("data", 4);
    write_u32(wav, 0);              // Data size - filled in later
}

void fix_wav_header()
{
    uint32_t data_size = audio_samples * 2;
    uint32_t file_size = 36 + data_size;

    wav.seekp(4);
    write_u32(wav, file_size);

    wav.seekp(40);
    write_u32(wav, data_size);
}

void callback(
    unsigned char* buffer,
    uint32_t length,
    void*)
{
    /*
     * RTL-SDR format:
     *
     * buffer[0] = I
     * buffer[1] = Q
     * buffer[2] = I
     * buffer[3] = Q
     * ...
     */

    static float decimation_counter = 0;

    for (uint32_t n = 0; n < length; n += 2)
    {
        float i = (buffer[n]     - 127.5f) / 127.5f;
        float q = (buffer[n + 1] - 127.5f) / 127.5f;

        std::complex<float> current(i, q);

        /*
         * FM discriminator.
         *
         * The phase difference between consecutive
         * complex samples is proportional to
         * instantaneous frequency.
         */
        float audio =
            std::arg(current * std::conj(previous));

        previous = current;

        /*
         * Very crude decimation:
         *
         * 2.4 MHz / 48 kHz = 50
         *
         * So keep approximately one sample out
         * of every 50.
         *
         * NOTE: A real receiver should low-pass
         * filter before decimation.
         */
        decimation_counter++;

        if (decimation_counter >= 50)
        {
            decimation_counter -= 50;

            // Scale to 16-bit PCM
            float scaled = audio * 10000.0f;

            if (scaled > 32767)
                scaled = 32767;

            if (scaled < -32768)
                scaled = -32768;

            int16_t pcm = static_cast<int16_t>(scaled);

            write_u16(wav, static_cast<uint16_t>(pcm));

            audio_samples++;
        }
    }
}

int main()
{
    if (rtlsdr_get_device_count() == 0)
    {
        std::cerr << "No RTL-SDR found\n";
        return 1;
    }

    rtlsdr_dev_t* device = nullptr;

    if (rtlsdr_open(&device, 0) != 0)
    {
        std::cerr << "Failed to open RTL-SDR\n";
        return 1;
    }

    std::cout << "Configuring RTL-SDR...\n";

    rtlsdr_set_center_freq(device, CENTER_FREQ);
    rtlsdr_set_sample_rate(device, SAMPLE_RATE);

    rtlsdr_set_tuner_gain_mode(device, 1);
    rtlsdr_set_tuner_gain(device, GAIN);

    rtlsdr_reset_buffer(device);

    wav.open("output.wav", std::ios::binary);

    if (!wav)
    {
        std::cerr << "Failed to create WAV file\n";
        rtlsdr_close(device);
        return 1;
    }

    write_wav_header();

    std::cout << "Receiving 94 MHz FM...\n";
    std::cout << "Press Ctrl+C to stop.\n";

    /*
     * Start asynchronous IQ reception.
     *
     * The callback will continuously receive
     * chunks of IQ data.
     */
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

    std::cout << "Saved output.wav\n";

    return 0;
}


/*
#include <rtl-sdr.h>

#include <iostream>
#include <csignal>



rtlsdr_dev_t *device = nullptr;

void handle_signal(int)
{
    rtlsdr_cancel_async(device);
}

void callback(unsigned char *buffer, uint32_t length, void *context)
{
	std::cout << "Received " << length << " bytes" << std::endl;
}



int main()
{

	int devCount = rtlsdr_get_device_count();
	std::cout << "device count = " << devCount << std::endl;

	int result = rtlsdr_open(&device, 0);

	if (result != 0)
	{
		std::cout << "Failed to open device" << std::endl;
	}


	rtlsdr_set_center_freq(device, 94'000'000);
	rtlsdr_set_sample_rate(device, 2'400'000);

	// 30.0 dB of manual gain
	rtlsdr_set_tuner_gain_mode(device, 1);
	rtlsdr_set_tuner_gain(device, 300);

	rtlsdr_reset_buffer(device);

	std::cout << "Starting Receiver..." << std::endl;


	rtlsdr_read_async(device, callback, nullptr, 0, 16 * 16384);


	std::signal(SIGINT, handle_signal);
	//rtlsdr_cancel_async(dev);
	rtlsdr_close(device);

	return 0;
}
*/