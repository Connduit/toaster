# Toaster
RF RX and TX
- uses rtl-sdr (for now)

https://deepwiki.com/osmocom/rtl-sdr/3-public-api-reference


Toaster --> ToasterSubsystem -->





               RTL-SDR
                  │
                  │ IQ bytes
                  ▼
          rtlsdr_read_async()
                  │
                  ▼
             callback()
                  │
                  ▼
             I/Q conversion
                  │
                  ▼
            complex IQ
                  │
                  ▼
          channel filtering
                  │
                  ▼
          FM demodulation
                  │
                  ▼
           audio filtering
                  │
                  ▼
             decimation
                  │
                  ▼
              PCM audio
                  │
                  ▼
             output.wav
                  │
                  │
                  └─────────────┐
                                │
                          more IQ data
                                │
                                ▼
                           callback()
                                │
                                ▼
                              repeat
