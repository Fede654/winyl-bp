# Winyl Analysis Edition - Technical Architecture

## Architecture Overview

The modernized Winyl follows a **hybrid architecture** that preserves the proven audio engine while introducing a modern web-based user interface and analysis capabilities.

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    Winyl Analysis Edition                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────┐    ┌─────────────────────────────────┐ │
│  │   WebView2 UI       │    │       C++ Core Engine          │ │
│  │                     │    │                                 │ │
│  │ ┌─────────────────┐ │    │ ┌─────────────────────────────┐ │ │
│  │ │   HTML/CSS/JS   │ │◄──►│ │        LibAudio             │ │ │
│  │ │                 │ │    │ │                             │ │ │
│  │ │ • Analysis UI   │ │    │ │ • BASS Integration          │ │ │
│  │ │ • Controls      │ │    │ │ • ASIO/WASAPI Drivers       │ │ │
│  │ │ • Visualizations│ │    │ │ • Audio Processing          │ │ │
│  │ │ • Settings      │ │    │ │ • Format Support            │ │ │
│  │ └─────────────────┘ │    │ └─────────────────────────────┘ │ │
│  │                     │    │                                 │ │
│  │ ┌─────────────────┐ │    │ ┌─────────────────────────────┐ │ │
│  │ │   JavaScript    │ │    │ │     Analysis Engine         │ │ │
│  │ │   Bridge Layer  │ │◄──►│ │                             │ │ │
│  │ │                 │ │    │ │ • FFT Processing            │ │ │
│  │ └─────────────────┘ │    │ │ • Loudness Metering         │ │ │
│  └─────────────────────┘    │ │ • Dynamic Range Analysis    │ │ │
│                             │ │ • Format Analysis           │ │ │
│  ┌─────────────────────┐    │ └─────────────────────────────┘ │ │
│  │   Native Window     │    │                                 │ │
│  │                     │    │ ┌─────────────────────────────┐ │ │
│  │ • WebView2 Host     │    │ │      Platform Layer         │ │ │
│  │ • Window Management │    │ │                             │ │ │
│  │ • File Associations │    │ │ • Windows 10+ APIs          │ │ │
│  │ • System Integration│    │ │ • File System Access        │ │ │
│  └─────────────────────┘    │ │ • Hardware Integration      │ │ │
│                             │ └─────────────────────────────┘ │ │
│                             └─────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## Component Architecture

### 1. Audio Engine (LibAudio) - PRESERVED
**Status**: Minimal changes, maximum preservation  
**Language**: C++ (existing codebase)  
**Dependencies**: BASS library, Windows audio APIs

#### Components:
```cpp
namespace LibAudio {
    class AudioEngine {
        // Core playback functionality (PRESERVED)
        BASS::Player bass_player;
        ASIO::Driver asio_driver;
        WASAPI::Endpoint wasapi_endpoint;
        
        // Real-time audio data access (NEW)
        AudioDataProvider data_provider;
    };
    
    class AudioDataProvider {
        // NEW: Provides audio samples for analysis
        std::vector<float> GetSamples(size_t frame_count);
        SampleRate GetCurrentSampleRate();
        ChannelCount GetChannelCount();
        
        // Thread-safe audio data extraction
        std::atomic<bool> analysis_enabled{false};
        RingBuffer<float> analysis_buffer;
    };
}
```

### 2. Analysis Engine - NEW
**Language**: C++20  
**Dependencies**: FFTW3, custom DSP algorithms

#### Real-Time Analysis Pipeline:
```cpp
namespace Analysis {
    class AnalysisEngine {
        std::unique_ptr<SpectrumAnalyzer> spectrum_analyzer;
        std::unique_ptr<LoudnessMeter> loudness_meter;
        std::unique_ptr<DynamicRangeAnalyzer> dr_analyzer;
        std::unique_ptr<PhaseAnalyzer> phase_analyzer;
        std::unique_ptr<DistortionAnalyzer> distortion_analyzer;
        
    public:
        void ProcessAudioFrame(const AudioFrame& frame);
        AnalysisResults GetCurrentResults();
        void SetAnalysisMode(AnalysisType type);
    };
    
    // Spectrum Analysis
    class SpectrumAnalyzer {
        FFTW::Plan fft_plan;
        std::vector<float> window_function;  // Hann, Blackman, etc.
        std::vector<float> magnitude_spectrum;
        std::vector<float> peak_hold;
        
    public:
        SpectrumData Analyze(const std::vector<float>& samples);
        void SetFFTSize(size_t size);  // 1024, 2048, 4096, 8192
        void SetWindowFunction(WindowType type);
    };
    
    // EBU R128 Loudness
    class LoudnessMeter {
        // EBU R128 filters
        HighPassFilter hp_filter;      // 75Hz highpass
        HighShelfFilter hs_filter;     // RLB weighting
        
        // Time constants
        CircularBuffer<float> momentary_buffer;   // 400ms
        CircularBuffer<float> short_term_buffer;  // 3s
        std::vector<float> integrated_buffer;     // Entire track
        
    public:
        LoudnessResults ProcessFrame(const AudioFrame& frame);
        float GetMomentary();    // M measurement
        float GetShortTerm();    // S measurement  
        float GetIntegrated();   // I measurement
        float GetLoudnessRange(); // LRA
        float GetTruePeak();     // TP
    };
}
```

### 3. WebView2 UI Layer - NEW
**Language**: HTML5, CSS3, JavaScript (ES2022)  
**Framework**: Custom lightweight framework (no React/Vue overhead)

#### UI Component Structure:
```html
<!-- Main Application Layout -->
<div id="app" class="winyl-app">
    <!-- Analysis Display Area -->
    <section id="analysis-panel" class="analysis-panel">
        <div id="spectrum-analyzer" class="analyzer-component">
            <canvas id="spectrum-canvas"></canvas>
            <div class="spectrum-controls">
                <select id="fft-size">4096</select>
                <select id="window-type">Hann</select>
            </div>
        </div>
        
        <div id="loudness-meters" class="meter-group">
            <div class="lufs-meter momentary"></div>
            <div class="lufs-meter short-term"></div>
            <div class="lufs-meter integrated"></div>
        </div>
        
        <div id="technical-info" class="info-panel">
            <span class="format-info"></span>
            <span class="output-info"></span>
            <span class="performance-info"></span>
        </div>
    </section>
    
    <!-- Playback Controls -->
    <section id="transport-panel" class="transport-panel">
        <div class="playback-controls">
            <button id="play-pause">⏯️</button>
            <button id="stop">⏹️</button>
            <button id="next">⏭️</button>
        </div>
        
        <div class="progress-area">
            <div class="progress-bar"></div>
            <div class="time-display">
                <span class="current-time">0:00</span>
                <span class="total-time">0:00</span>
            </div>
        </div>
    </section>
</div>
```

#### JavaScript Bridge API:
```javascript
// Bridge between JavaScript UI and C++ engine
class WinylBridge {
    // Audio Control
    async playFile(filePath) { return window.chrome.webview.hostObjects.winyl.PlayFile(filePath); }
    async pause() { return window.chrome.webview.hostObjects.winyl.Pause(); }
    async stop() { return window.chrome.webview.hostObjects.winyl.Stop(); }
    async seek(position) { return window.chrome.webview.hostObjects.winyl.Seek(position); }
    
    // Analysis Data (Real-time updates)
    async getSpectrumData() { return window.chrome.webview.hostObjects.winyl.GetSpectrumData(); }
    async getLoudnessData() { return window.chrome.webview.hostObjects.winyl.GetLoudnessData(); }
    async getDynamicRangeData() { return window.chrome.webview.hostObjects.winyl.GetDynamicRangeData(); }
    
    // Configuration
    async setAnalysisMode(mode) { return window.chrome.webview.hostObjects.winyl.SetAnalysisMode(mode); }
    async setOutputDevice(deviceId) { return window.chrome.webview.hostObjects.winyl.SetOutputDevice(deviceId); }
}

// Real-time visualization
class SpectrumVisualizer {
    constructor(canvasElement) {
        this.canvas = canvasElement;
        this.ctx = canvasElement.getContext('2d');
        this.requestId = null;
    }
    
    start() {
        const render = async () => {
            const spectrumData = await winylBridge.getSpectrumData();
            this.drawSpectrum(spectrumData);
            this.requestId = requestAnimationFrame(render);
        };
        render();
    }
    
    drawSpectrum(data) {
        // High-performance canvas rendering
        // 60fps smooth visualization
    }
}
```

### 4. C++ ↔ JavaScript Bridge - NEW
**Language**: C++20  
**Technology**: WebView2 Host Objects

```cpp
// C++ side of the bridge
class WinylWebViewBridge {
    std::shared_ptr<LibAudio::AudioEngine> audio_engine;
    std::shared_ptr<Analysis::AnalysisEngine> analysis_engine;
    
public:
    // Expose to JavaScript
    void PlayFile(const std::wstring& file_path);
    void Pause();
    void Stop();
    void Seek(double position);
    
    // Real-time data for UI
    std::string GetSpectrumData();  // JSON serialized
    std::string GetLoudnessData();  // JSON serialized
    std::string GetDynamicRangeData();  // JSON serialized
    
    // Configuration
    void SetAnalysisMode(const std::string& mode);
    void SetOutputDevice(const std::string& device_id);
    
private:
    // JSON serialization helpers
    std::string SerializeSpectrum(const Analysis::SpectrumData& data);
    std::string SerializeLoudness(const Analysis::LoudnessResults& data);
};
```

## Data Flow Architecture

### Real-Time Analysis Pipeline:
```
Audio File → BASS Decoder → LibAudio → Analysis Engine → Bridge → WebView2 UI
                ↓
           Audio Output
        (ASIO/WASAPI)
```

### Analysis Data Flow:
```cpp
// 1. Audio samples extracted from BASS stream
std::vector<float> samples = audio_engine->GetAnalysisSamples(1024);

// 2. Multiple analyzers process same data
auto spectrum = spectrum_analyzer->Analyze(samples);
auto loudness = loudness_meter->ProcessFrame(samples);
auto dr = dr_analyzer->ProcessFrame(samples);

// 3. Results aggregated and sent to UI
AnalysisResults results{spectrum, loudness, dr};
bridge->SendToUI(results);

// 4. UI updates visualizations at 60fps
// JavaScript receives data and updates canvas/meters
```

## Threading Model

### Core Threads:
1. **Audio Thread** (BASS): Real-time audio playback (preserved from original)
2. **Analysis Thread** (NEW): Real-time DSP processing  
3. **UI Thread** (NEW): WebView2 and user interface
4. **Bridge Thread** (NEW): Data serialization and communication

### Thread Communication:
```cpp
class ThreadSafeDataExchange {
    // Lock-free data structures for real-time safety
    boost::lockfree::spsc_queue<AudioFrame> audio_frames;
    boost::lockfree::spsc_queue<AnalysisResults> analysis_results;
    
    // High-priority audio thread never blocks
    void PushAudioFrame(const AudioFrame& frame) {
        audio_frames.push(frame);  // Lock-free
    }
    
    // Analysis thread processes when data available
    bool PopAudioFrame(AudioFrame& frame) {
        return audio_frames.pop(frame);  // Lock-free
    }
};
```

## Performance Considerations

### Real-Time Requirements:
- **Audio Thread**: < 10ms latency, never blocks
- **Analysis Thread**: < 16ms processing (60fps UI updates)
- **UI Updates**: Smooth 60fps visualization
- **Memory Usage**: < 100MB typical, < 500MB peak

### Optimization Strategies:
1. **SIMD Instructions**: Use AVX2/AVX-512 for FFT and DSP
2. **Lock-Free Structures**: No blocking between audio and analysis
3. **Memory Pools**: Pre-allocated buffers for real-time processing
4. **GPU Acceleration**: Consider DirectCompute for complex analysis

## File and Project Structure

```
src/
├── audio/              # LibAudio (preserved + minimal changes)
│   ├── AudioEngine.cpp
│   ├── BassIntegration.cpp
│   └── AudioDataProvider.cpp  # NEW
├── analysis/           # Analysis Engine (NEW)
│   ├── SpectrumAnalyzer.cpp
│   ├── LoudnessMeter.cpp
│   ├── DynamicRangeAnalyzer.cpp
│   └── PhaseAnalyzer.cpp
├── bridge/             # WebView2 Bridge (NEW)
│   ├── WinylBridge.cpp
│   └── DataSerialization.cpp
├── ui/                 # WebView2 Host (NEW)
│   ├── MainWindow.cpp
│   └── WebViewHost.cpp
└── web/                # Web UI Assets (NEW)
    ├── index.html
    ├── styles/
    │   └── main.css
    └── scripts/
        ├── winyl-bridge.js
        ├── spectrum-visualizer.js
        └── loudness-meters.js

tests/
├── audio/              # Audio engine regression tests
├── analysis/           # Analysis algorithm validation
└── integration/        # End-to-end testing

docs/
├── architecture/       # This document and related
├── api/               # API documentation
└── user/              # User documentation
```

## Technology Stack

### Core Technologies:
- **C++20**: Modern C++ for new components
- **BASS 2.4**: Audio library (preserved)
- **WebView2**: Microsoft Edge-based web view
- **FFTW3**: Fast Fourier Transform library
- **Windows 10+ APIs**: Modern Windows platform features

### Development Tools:
- **Visual Studio 2022**: Primary IDE
- **CMake**: Build system
- **vcpkg**: Package management
- **GitHub Actions**: CI/CD pipeline
- **Doxygen**: API documentation

### Web Technologies:
- **HTML5**: Modern semantic markup
- **CSS3**: Responsive styling with CSS Grid/Flexbox
- **JavaScript ES2022**: Modern JavaScript features
- **Web Workers**: Background processing for UI
- **Canvas API**: High-performance visualizations

## Security Considerations

### WebView2 Security:
- Content Security Policy (CSP) headers
- Restricted navigation and script execution
- Sandboxed execution environment
- No external network access from web content

### File System Access:
- Controlled file access through native bridge
- No direct file system access from JavaScript
- Validation of all file paths and operations

---

**Document Version**: 1.0  
**Last Updated**: 2024-01-20  
**Next Review**: 2024-02-20  
**Architecture Owner**: Technical Lead