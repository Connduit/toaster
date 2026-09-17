#include "Decimator.h"

#include <stdexcept>

// Decimator::Decimator(int factor)
//     : factor_(factor),
//       counter_(0)
// {
//     if (factor_ <= 0)
//         throw std::invalid_argument(
//             "Decimation factor must be greater than zero"
//         );
// }
// 
// bool Decimator::processSample(float input, float& output)
// {
//     ++counter_;
// 
//     if (counter_ >= factor_)
//     {
//         counter_ = 0;
//         output = input;
//         return true;
//     }
// 
//     return false;
// }
// 
// AudioData Decimator::process(const AudioData& input)
// {
//     AudioData output;
// 
//     output.reserve(
//         input.size() / factor_ + 1
//     );
// 
//     for (const float sample : input)
//     {
//         ++counter_;
// 
//         if (counter_ >= factor_)
//         {
//             counter_ = 0;
// 
//             output.push_back(sample);
//         }
//     }
// 
//     return output;
// }