#ifndef TOASTER_AUDIO_H
#define TOASTER_AUDIO_H

#include <vector>
#include <string>
#include <fstream>
#include <cstdint>

class AudioSink {
public:
    enum class Format { WAV, RAW, NONE };
    
    AudioSink();
    ~AudioSink();
    
    // Configuration
    void setFormat(Format format) { format_ = format; }
    void setFilename(const std::string& filename) { filename_ = filename; }
    void setSampleRate(uint32_t rate) { sample_rate_ = rate; }
    
    // Push audio data
    void write(const std::vector<float>& audio);
    
    // Finalize (close file, etc.)
    void finalize();
    
    // Get stats
    size_t getSampleCount() const { return sample_count_; }
    float getDurationSeconds() const { return sample_count_ / sample_rate_; }
    
private:
    void writeWav(const std::vector<float>& audio);
    void writeRaw(const std::vector<float>& audio);
    
    Format format_ = Format::WAV;
    std::string filename_ = "output.wav";
    uint32_t sample_rate_ = 48000;
    size_t sample_count_ = 0;
    
    std::ofstream file_;
    bool finalized_ = false;
    bool header_written_ = false;
};

#endif