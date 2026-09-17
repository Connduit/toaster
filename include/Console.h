#pragma once

#include <iostream>
#include <string>

// Thin wrapper around console I/O. Centralizes access to stdin/stderr so
// nothing else in the program touches std::cin/std::cerr directly.
// stdout is reserved for raw audio streaming in this project, so status
// and prompt output goes to stderr by default.
//
// Takes its streams by reference (defaulting to std::cin/std::cerr)
// rather than owning them outright, so a test could point a Console at
// a std::istringstream/std::ostringstream instead.
class Console {
public:
    explicit Console(std::istream& in = std::cin, std::ostream& out = std::cerr)
        : in_(in), out_(out) {}

    // Blocks until a full line is read from the input stream. Returns
    // false (line left unchanged) if the stream has closed (EOF).
    bool readLine(std::string& line) { return static_cast<bool>(std::getline(in_, line)); }

    // Writes a line to the output stream, followed by a newline.
    void printLine(const std::string& message) { out_ << message << "\n"; }

private:
    std::istream& in_;
    std::ostream& out_;
};
