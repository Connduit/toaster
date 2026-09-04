#include "Audio.h"
#include <iostream>
#include <cmath>

AudioSink::AudioSink() = default;
AudioSink::~AudioSink() { finalize(); }

void AudioSink::write(const std::vector<float>& audio) 
{
    //std::cout << "AudioSink::write()" << std::endl;

    if (audio.empty()) return;
    
    sample_count_ += audio.size();
    
    switch (format_) 
    {
        case Format::WAV:
            writeWav(audio);
            break;
        case Format::RAW:
            writeRaw(audio);
            break;
        case Format::NONE:
            break;
    }
}

void AudioSink::writeWav(const std::vector<float>& audio) 
{
    std::cout << "writeWav" << std::endl;
    if (!file_.is_open()) 
    {
        file_.open(filename_, std::ios::binary);
        if (!file_) 
        {
            std::cerr << "Failed to open WAV file: " << filename_ << std::endl;
            return;
        }
        std::cout << " header " << std::endl;
        // Write header later (when we know total size)
        // For now, just write a dummy header
        file_.write("RIFF", 4);
        int32_t temp = 0;
        file_.write(reinterpret_cast<const char*>(&temp), 4);
        file_.write("WAVE", 4);
        file_.write("fmt ", 4);
        int32_t fmt_size = 16;
        file_.write(reinterpret_cast<const char*>(&fmt_size), 4);
        int16_t audio_format = 1;
        file_.write(reinterpret_cast<const char*>(&audio_format), 2);
        int16_t num_channels = 1;
        file_.write(reinterpret_cast<const char*>(&num_channels), 2);
        int32_t sample_rate_int = sample_rate_;
        file_.write(reinterpret_cast<const char*>(&sample_rate_int), 4);
        int32_t byte_rate = sample_rate_ * 2;
        file_.write(reinterpret_cast<const char*>(&byte_rate), 4);
        int16_t block_align = 2;
        file_.write(reinterpret_cast<const char*>(&block_align), 2);
        int16_t bits_per_sample = 16;
        file_.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
        file_.write("data", 4);
        int32_t data_size = 0;
        file_.write(reinterpret_cast<const char*>(&data_size), 4);
        
        header_written_ = true;
    }
    
    // Write audio data
    for (float sample : audio) 
    {
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        int16_t s = static_cast<int16_t>(sample * 32767.0f);
        file_.write(reinterpret_cast<const char*>(&s), 2);
    }
    std::cout << " audio written " << std::endl;
}

void AudioSink::writeRaw(const std::vector<float>& audio) {
    if (!file_.is_open()) {
        file_.open(filename_, std::ios::binary);
        if (!file_) {
            std::cerr << "Failed to open RAW file: " << filename_ << std::endl;
            return;
        }
    }
    
    for (float sample : audio) {
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        int16_t s = static_cast<int16_t>(sample * 32767.0f);
        file_.write(reinterpret_cast<const char*>(&s), 2);
    }
}

void AudioSink::finalize() {
    if (finalized_ || !file_.is_open()) return;
    
    // Fix WAV header if needed
    if (format_ == Format::WAV && header_written_) {
        // Seek to beginning and update sizes
        file_.seekp(4, std::ios::beg);
        int32_t file_size = 36 + sample_count_ * 2;
        file_.write(reinterpret_cast<const char*>(&file_size), 4);
        
        file_.seekp(40, std::ios::beg);
        int32_t data_size = sample_count_ * 2;
        file_.write(reinterpret_cast<const char*>(&data_size), 4);
        
        std::cout << "WAV file saved: " << filename_ << std::endl;
        std::cout << "  Samples: " << sample_count_ << std::endl;
        std::cout << "  Duration: " << getDurationSeconds() << " seconds" << std::endl;
    }
    
    file_.close();
    finalized_ = true;
}