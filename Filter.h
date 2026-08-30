// #include <vector>
// #include <complex>
// 
// // TODO: change to be a template class if we code to work with any type of filter/data type?
// class Filter
// {
// public:
//     //Filter();
//     virtual ~Filter() = default;
// 
//     //virtual float convert(); 
//     virtual float process() = 0;
// private:
// };
// 
// // FIR (finite impulse response)
// // IIR (infinite impulse response)
// 
// /*
// y_n = (x_n + x_(n-1))/2
// where x_n is current sample, x_(n-1) is previous sample, y_n = current sample output
// 
// y_n = ax_n + (1 - a)x_(n-1)
// */
// // https://en.wikipedia.org/wiki/Low-pass_filter#Simple_infinite_impulse_response_filter
// class LowPass : public Filter
// {
// public:
//     LowPass(float sampleRate, float cutoffFreq, float width);
//     ~LowPass();
//     float process();
// private:
//     float processFloat();
//     float processComplex();
// 
//     std::vector<float> coefficients; // alphas
//     // TODO: typedef this
//     std::vector<std::complex<float>> delayLine; // prev finite samples... TODO: this should eventually become a circular buffer
// };
// 
// /*
// // y_n = ax_n - (1 - a)x_(n-1)
// class HighPass : public Filter
// {
// public:
//     HighPass();
//     ~HighPass();
// private:
// };
// 
// class BandPass : public Filter
// {
// public:
//     BandPass();
//     ~BandPass();
// private:
// };
// 
// class BandStop : public Filter
// {
// public:
//     BandStop();
//     ~BandStop();
// private:
// };
// */