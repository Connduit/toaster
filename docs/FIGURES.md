# Figures

### Flowchart

                 ToasterSubsystem
                       │
                 FilterType
                  /          \
                IIR           FIR
                 │             │
           IIRFilter       FIRFilter
                 \             /
                  \           /
                   ChannelFilter
                         │
                         ↓
                    DspProcessor
                         │
               ┌─────────┴─────────┐
               ↓                   ↓
          Demodulator         AudioFilter
                                   │
                              IIR or FIR
                                   │
                                   ↓
                              Decimator
                                   │
                                   ↓
                                 Audio


                    Filter
                       |
              +--------+--------+
              |                 |
          IIRFilter          FIRFilter
              |                 |
              +--------+--------+
                       |
                ChannelFilter
                  /         \
             I Filter      Q Filter
                  \         /
                   \       /
                    Complex IQ
                        |
                   Demodulator
                        |
                   Audio Filter
                    /       \
                IIRFilter  FIRFilter
                        |
                    Decimator
                        |
                      Audio
                        |
                    AudioSink