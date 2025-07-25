# Testing and Quality Assurance Strategy

## Overview
This document defines the comprehensive testing strategy for Winyl Analysis Edition, with **audio quality preservation** as the highest priority. Any regression in audio performance constitutes a critical failure.

## Testing Philosophy

### Core Principles:
1. **Audio Quality is Sacred**: No compromise on bit-perfect playback
2. **Measure, Don't Assume**: Use professional audio measurement tools
3. **Continuous Validation**: Test after every significant change
4. **Real-World Testing**: Test with actual audiophile equipment and users

### Testing Pyramid:
```
                   ┌─────────────────┐
                   │  Manual Testing │  ← Community + Expert validation
                   │   (Human ears)  │
                   └─────────────────┘
                  ┌───────────────────┐
                  │ Integration Tests │  ← Full system scenarios
                  └───────────────────┘
               ┌─────────────────────────┐
               │    Component Tests      │  ← Individual module testing  
               └─────────────────────────┘
          ┌─────────────────────────────────┐
          │        Unit Tests               │  ← Algorithm validation
          └─────────────────────────────────┘
     ┌─────────────────────────────────────────┐
     │        Audio Regression Tests           │  ← Bit-perfect validation
     └─────────────────────────────────────────┘
```

## Audio Quality Testing (CRITICAL)

### 1. Bit-Perfect Playback Validation
**Priority**: CRITICAL  
**Frequency**: After every audio engine change  
**Automation**: Fully automated

#### Test Setup:
```cpp
class BitPerfectTest {
private:
    // Professional audio interface for loopback testing
    std::unique_ptr<AudioInterface> professional_dac;  // RME, Focusrite, etc.
    std::unique_ptr<AudioAnalyzer> analyzer;           // APx, RMAA, or custom
    
public:
    // Test bit-perfect playback for all formats
    void TestAllFormats() {
        std::vector<TestFile> test_files = {
            {"sine_1kHz_16bit_44kHz.wav", ExpectedResult::BitPerfect},
            {"sine_1kHz_24bit_96kHz.flac", ExpectedResult::BitPerfect},
            {"sine_1kHz_32bit_192kHz.wav", ExpectedResult::BitPerfect},
            {"white_noise_24bit_48kHz.flac", ExpectedResult::BitPerfect},
            {"silence_24bit_96kHz.wav", ExpectedResult::BitPerfect},
            {"full_scale_1kHz_24bit.wav", ExpectedResult::BitPerfect}
        };
        
        for (const auto& test_file : test_files) {
            ValidateBitPerfectPlayback(test_file);
        }
    }
    
private:
    void ValidateBitPerfectPlayback(const TestFile& file) {
        // 1. Load known reference file
        LoadReferenceFile(file.filepath);
        
        // 2. Set up digital loopback recording
        SetupDigitalLoopback();
        
        // 3. Start recording and playback simultaneously
        StartRecording();
        PlayFile(file.filepath);
        
        // 4. Compare recorded output with original file
        auto recorded_data = StopRecordingAndGetData();
        auto reference_data = LoadReferenceData(file.filepath);
        
        // 5. Verify bit-perfect match
        bool is_bit_perfect = CompareBitPerfect(recorded_data, reference_data);
        
        ASSERT_TRUE(is_bit_perfect) << "Bit-perfect test failed for " << file.filepath;
        
        // 6. Additional validation
        ValidateNoClipping(recorded_data);
        ValidateNoArtifacts(recorded_data);
        ValidateDynamicRange(recorded_data, file.expected_dr);
    }
    
    bool CompareBitPerfect(const AudioData& recorded, const AudioData& reference) {
        // Account for potential latency offset
        size_t best_offset = FindBestAlignment(recorded, reference);
        
        // Compare sample by sample
        for (size_t i = 0; i < reference.size(); ++i) {
            size_t recorded_idx = i + best_offset;
            if (recorded_idx >= recorded.size()) break;
            
            // Allow for minimal rounding errors in floating point conversion
            float diff = std::abs(recorded[recorded_idx] - reference[i]);
            if (diff > 1e-6f) {  // Stricter than typical audio precision
                return false;
            }
        }
        return true;
    }
};
```

### 2. Latency and Performance Testing
**Priority**: HIGH  
**Frequency**: Weekly during development

#### Real-Time Performance Validation:
```cpp
class PerformanceTest {
public:
    void TestAudioLatency() {
        // Test ASIO latency
        auto asio_latency = MeasureASIOLatency();
        EXPECT_LT(asio_latency, std::chrono::milliseconds(15)) << "ASIO latency too high";
        
        // Test WASAPI latency  
        auto wasapi_latency = MeasureWASAPILatency();
        EXPECT_LT(wasapi_latency, std::chrono::milliseconds(50)) << "WASAPI latency too high";
        
        // Test buffer underruns
        auto underrun_count = CountBufferUnderruns(std::chrono::minutes(5));
        EXPECT_EQ(underrun_count, 0) << "Audio buffer underruns detected";
    }
    
    void TestCPUUsage() {
        // Baseline CPU usage without analysis
        StartAudioPlayback();
        auto baseline_cpu = MeasureCPUUsage(std::chrono::seconds(30));
        
        // CPU usage with analysis enabled
        EnableAllAnalysis();
        auto analysis_cpu = MeasureCPUUsage(std::chrono::seconds(30));
        
        // Analysis should add < 5% CPU usage
        auto analysis_overhead = analysis_cpu - baseline_cpu;
        EXPECT_LT(analysis_overhead, 5.0) << "Analysis CPU overhead too high";
    }
    
private:
    std::chrono::microseconds MeasureASIOLatency() {
        // Generate click at time T
        // Measure when click appears at output
        // Return round-trip latency
    }
};
```

### 3. Format Compatibility Testing
**Priority**: HIGH  
**Frequency**: After format-related changes

#### Comprehensive Format Matrix:
```cpp
class FormatCompatibilityTest {
private:
    struct FormatTestCase {
        std::string format;
        std::string sample_rate;
        std::string bit_depth;
        std::string channels;
        bool should_play;
        std::string expected_error;
    };
    
    std::vector<FormatTestCase> test_matrix = {
        // Lossless formats
        {"FLAC", "44100", "16", "2", true, ""},
        {"FLAC", "96000", "24", "2", true, ""},
        {"FLAC", "192000", "24", "2", true, ""},
        {"FLAC", "384000", "32", "2", true, ""},
        
        // Lossy formats
        {"MP3", "44100", "16", "2", true, ""},
        {"OGG", "44100", "16", "2", true, ""},
        {"AAC", "44100", "16", "2", true, ""},
        
        // High-res formats
        {"DSD64", "2822400", "1", "2", true, ""},
        {"DSD128", "5644800", "1", "2", true, ""},
        
        // Edge cases
        {"FLAC", "22050", "8", "1", true, ""},      // Low quality
        {"FLAC", "192000", "32", "8", true, ""},    // Multi-channel
        {"CORRUPT", "44100", "16", "2", false, "Invalid format"}
    };
    
public:
    void TestAllFormats() {
        for (const auto& test_case : test_matrix) {
            TestFormat(test_case);
        }
    }
    
private:
    void TestFormat(const FormatTestCase& test_case) {
        std::string test_file = GenerateTestFile(test_case);
        
        bool load_success = LoadFile(test_file);
        EXPECT_EQ(load_success, test_case.should_play) 
            << "Format test failed: " << test_case.format;
            
        if (load_success) {
            // Verify playback works
            EXPECT_TRUE(StartPlayback());
            EXPECT_TRUE(IsPlaying());
            
            // Verify metadata is correct
            auto format_info = GetFormatInfo();
            EXPECT_EQ(format_info.sample_rate, std::stoi(test_case.sample_rate));
            EXPECT_EQ(format_info.bit_depth, std::stoi(test_case.bit_depth));
            EXPECT_EQ(format_info.channels, std::stoi(test_case.channels));
        }
    }
};
```

## Analysis Algorithm Testing

### 1. Spectrum Analysis Validation
**Priority**: MEDIUM  
**Frequency**: After analysis changes

```cpp
class SpectrumAnalysisTest {
public:
    void TestKnownSignals() {
        // Test with known sine waves
        TestSineWave(1000.0f, -20.0f);  // 1kHz at -20dBFS
        TestSineWave(440.0f, -12.0f);   // A4 at -12dBFS
        TestSineWave(10000.0f, -30.0f); // 10kHz at -30dBFS
        
        // Test with complex signals
        TestMultiTone();
        TestWhiteNoise();
        TestPinkNoise();
    }
    
private:
    void TestSineWave(float frequency, float amplitude_db) {
        // Generate known sine wave
        auto test_signal = GenerateSineWave(frequency, amplitude_db, 48000, 1.0);
        
        // Analyze spectrum
        auto spectrum = analyzer->AnalyzeSpectrum(test_signal);
        
        // Find peak frequency
        auto peak_freq = FindPeakFrequency(spectrum);
        auto peak_amplitude = FindPeakAmplitude(spectrum);
        
        // Validate accuracy
        EXPECT_NEAR(peak_freq, frequency, 1.0f) << "Frequency detection error";
        EXPECT_NEAR(peak_amplitude, amplitude_db, 0.1f) << "Amplitude measurement error";
        
        // Validate that other frequencies are below noise floor
        ValidateSpectralPurity(spectrum, frequency);
    }
    
    void TestMultiTone() {
        // Generate signal with multiple known frequencies
        auto signal = GenerateMultiTone({440.0f, 880.0f, 1320.0f}, {-10.0f, -15.0f, -20.0f});
        
        auto spectrum = analyzer->AnalyzeSpectrum(signal);
        auto peaks = FindPeaks(spectrum);
        
        EXPECT_EQ(peaks.size(), 3) << "Should detect exactly 3 peaks";
        
        // Validate each peak
        for (size_t i = 0; i < peaks.size(); ++i) {
            EXPECT_NEAR(peaks[i].frequency, expected_frequencies[i], 2.0f);
            EXPECT_NEAR(peaks[i].amplitude, expected_amplitudes[i], 0.5f);
        }
    }
};
```

### 2. Loudness Metering Validation
**Priority**: MEDIUM  
**Frequency**: After loudness algorithm changes

```cpp
class LoudnessTest {
public:
    void TestEBUR128Compliance() {
        // Use EBU R128 reference signals
        TestFile reference_signal = LoadEBUTestSignal("EBU_3341_1kHz_-23LUFS.wav");
        
        auto loudness_result = analyzer->MeasureLoudness(reference_signal);
        
        // Validate against known reference values
        EXPECT_NEAR(loudness_result.integrated, -23.0f, 0.1f) << "Integrated loudness error";
        EXPECT_NEAR(loudness_result.range, 0.0f, 0.1f) << "Loudness range error for sine wave";
        EXPECT_LT(loudness_result.true_peak, 0.0f) << "True peak should be below 0dBFS";
    }
    
    void TestDynamicContent() {
        // Test with music content that has known LUFS values
        std::vector<TestFile> reference_files = {
            {"classical_music_-18LUFS.flac", -18.0f, 15.0f},  // High DR
            {"pop_music_-14LUFS.flac", -14.0f, 8.0f},        // Medium DR
            {"electronic_-8LUFS.flac", -8.0f, 4.0f}          // Low DR (loud)
        };
        
        for (const auto& file : reference_files) {
            auto result = analyzer->MeasureLoudness(file.path);
            
            EXPECT_NEAR(result.integrated, file.expected_lufs, 0.5f) 
                << "LUFS measurement error for " << file.path;
            EXPECT_NEAR(result.range, file.expected_lra, 1.0f)
                << "LRA measurement error for " << file.path;
        }
    }
};
```

## User Interface Testing

### 1. WebView2 Integration Testing
**Priority**: MEDIUM  
**Frequency**: After UI changes

```cpp
class UIIntegrationTest {
public:
    void TestBasicPlaybackControls() {
        // Load test file
        LoadTestFile("test_audio.flac");
        
        // Test play button
        ClickPlayButton();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_TRUE(IsAudioPlaying());
        
        // Test pause button
        ClickPauseButton();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_FALSE(IsAudioPlaying());
        
        // Test stop button
        ClickPlayButton();
        ClickStopButton();
        EXPECT_FALSE(IsAudioPlaying());
        EXPECT_EQ(GetPlaybackPosition(), 0.0);
    }
    
    void TestDragDropFunctionality() {
        // Simulate drag and drop
        SimulateDragDrop("test_file.flac");
        
        // Verify file was loaded
        EXPECT_TRUE(IsFileLoaded());
        EXPECT_EQ(GetLoadedFileName(), "test_file.flac");
        
        // Verify metadata display
        auto metadata = GetDisplayedMetadata();
        EXPECT_FALSE(metadata.title.empty());
        EXPECT_FALSE(metadata.artist.empty());
    }
    
    void TestRealTimeUpdates() {
        LoadAndPlay("test_file.flac");
        
        // Check that position updates
        auto initial_position = GetDisplayedPosition();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto updated_position = GetDisplayedPosition();
        
        EXPECT_GT(updated_position, initial_position);
        
        // Check that spectrum updates
        auto initial_spectrum = GetDisplayedSpectrum();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto updated_spectrum = GetDisplayedSpectrum();
        
        EXPECT_NE(initial_spectrum, updated_spectrum) << "Spectrum should update in real-time";
    }
};
```

### 2. Analysis Visualization Testing
**Priority**: LOW  
**Frequency**: Before releases

```cpp
class VisualizationTest {
public:
    void TestSpectrumDisplay() {
        // Play known signal
        PlaySineWave(1000.0f, -20.0f);
        
        // Capture spectrum display
        auto spectrum_image = CaptureSpectrumDisplay();
        
        // Analyze display for expected peak
        auto peak_pixel = FindBrightestPixel(spectrum_image);
        auto peak_frequency = PixelToFrequency(peak_pixel.x);
        
        EXPECT_NEAR(peak_frequency, 1000.0f, 50.0f) << "Spectrum display error";
    }
    
    void TestLoudnessMeters() {
        // Play signal with known loudness
        PlaySignalWithLoudness(-23.0f);
        
        // Check meter display
        auto meter_reading = ReadLoudnessMeterDisplay();
        EXPECT_NEAR(meter_reading, -23.0f, 1.0f) << "Loudness meter display error";
    }
};
```

## Integration and System Testing

### 1. End-to-End Scenarios
**Priority**: HIGH  
**Frequency**: Before releases

```cpp
class EndToEndTest {
public:
    void TestCompleteUserWorkflow() {
        // 1. Start application
        StartApplication();
        EXPECT_TRUE(IsApplicationRunning());
        
        // 2. Load file via drag and drop
        DragDropFile("test_album/track01.flac");
        EXPECT_TRUE(IsFileLoaded());
        
        // 3. Start playback
        ClickPlay();
        EXPECT_TRUE(IsPlaying());
        
        // 4. Enable analysis features
        EnableSpectrumAnalysis();
        EnableLoudnessMetering();
        
        // 5. Verify real-time analysis
        std::this_thread::sleep_for(std::chrono::seconds(5));
        EXPECT_TRUE(IsSpectrumUpdating());
        EXPECT_TRUE(IsLoudnessUpdating());
        
        // 6. Change output device
        SelectOutputDevice("ASIO - RME Fireface");
        EXPECT_TRUE(IsPlaying()); // Should continue playing
        
        // 7. Load different format
        DragDropFile("test_album/track02.mp3");
        EXPECT_TRUE(IsFileLoaded());
        
        // 8. Export analysis data
        ExportAnalysisData("analysis_output.csv");
        EXPECT_TRUE(std::filesystem::exists("analysis_output.csv"));
        
        // 9. Clean shutdown
        CloseApplication();
        EXPECT_FALSE(IsApplicationRunning());
    }
    
    void TestStressScenarios() {
        // Test with large files
        LoadFile("very_large_file_2GB.flac");
        EXPECT_TRUE(PlaysWithoutIssues(std::chrono::minutes(10)));
        
        // Test rapid file switching
        for (int i = 0; i < 100; ++i) {
            LoadFile(GetRandomTestFile());
            PlayForSeconds(2);
        }
        EXPECT_FALSE(HasMemoryLeaks());
        
        // Test long-duration analysis
        EnableAllAnalysis();
        PlayFile("long_test_file_1hour.flac");
        WaitForCompletion();
        EXPECT_TRUE(AnalysisDataIsValid());
    }
};
```

## Performance and Memory Testing

### 1. Memory Management
**Priority**: MEDIUM  
**Frequency**: Weekly

```cpp
class MemoryTest {
public:
    void TestMemoryUsage() {
        auto initial_memory = GetCurrentMemoryUsage();
        
        // Load and play multiple files
        for (int i = 0; i < 50; ++i) {
            LoadFile(GetTestFile(i));
            PlayForSeconds(30);
            UnloadFile();
        }
        
        // Force garbage collection
        ForceGarbageCollection();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        auto final_memory = GetCurrentMemoryUsage();
        auto memory_growth = final_memory - initial_memory;
        
        EXPECT_LT(memory_growth, 50 * 1024 * 1024) << "Memory leak detected";
    }
    
    void TestMemoryLeakDetection() {
        // Use tools like Application Verifier, CRT Debug Heap, or Valgrind
        EnableMemoryLeakDetection();
        
        RunStandardTestSuite();
        
        auto leaks = GetDetectedMemoryLeaks();
        EXPECT_EQ(leaks.size(), 0) << "Memory leaks detected: " << FormatLeaks(leaks);
    }
};
```

## Automated Testing Infrastructure

### 1. Continuous Integration
```yaml
# GitHub Actions CI/CD pipeline
name: Winyl Analysis Edition CI

on: [push, pull_request]

jobs:
  audio-regression-tests:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - name: Setup Audio Testing Environment
        run: |
          # Install ASIO4ALL for testing
          # Setup virtual audio devices
          # Install audio measurement tools
      
      - name: Build Application
        run: |
          cmake --build . --config Release
      
      - name: Run Audio Regression Tests
        run: |
          ./tests/audio_regression_tests.exe
          
      - name: Run Bit-Perfect Validation
        run: |
          ./tests/bit_perfect_tests.exe
          
      - name: Upload Audio Test Results
        uses: actions/upload-artifact@v3
        with:
          name: audio-test-results
          path: test_results/
  
  analysis-algorithm-tests:
    runs-on: windows-latest
    steps:
      - name: Run Spectrum Analysis Tests
        run: ./tests/spectrum_tests.exe
        
      - name: Run Loudness Metering Tests  
        run: ./tests/loudness_tests.exe
        
      - name: Validate Against Reference Data
        run: ./tests/reference_validation.exe

  ui-integration-tests:
    runs-on: windows-latest
    steps:
      - name: Setup WebView2
        run: |
          # Install WebView2 runtime
          
      - name: Run UI Tests
        run: ./tests/ui_tests.exe
        
      - name: Run End-to-End Tests
        run: ./tests/e2e_tests.exe
```

### 2. Test Data Management
```cpp
// Test data organization
struct TestDataRepository {
    // Reference audio files for testing
    std::string reference_files_path = "test_data/reference/";
    
    // Generated test signals
    std::string generated_signals_path = "test_data/generated/";
    
    // Expected results for validation
    std::string expected_results_path = "test_data/expected/";
    
    // Performance benchmarks
    std::string benchmarks_path = "test_data/benchmarks/";
};
```

## Quality Gates

### Pre-Commit Checks:
- [ ] All unit tests pass
- [ ] No memory leaks detected
- [ ] Code coverage > 80% for new code
- [ ] Static analysis clean (no new warnings)

### Pre-Release Checks:
- [ ] All audio regression tests pass
- [ ] Bit-perfect validation on 3+ audio interfaces
- [ ] End-to-end testing with community beta testers
- [ ] Performance benchmarks within acceptable ranges
- [ ] Memory usage under target limits
- [ ] UI responsiveness validated

### Community Testing:
- [ ] Beta release to audiophile community
- [ ] Testing with variety of audio hardware
- [ ] Feedback collection and issue resolution
- [ ] Final validation before public release

---

**Document Version**: 1.0  
**Last Updated**: 2024-01-20  
**Review Frequency**: Monthly during active development  
**Quality Owner**: QA Lead + Audio Expert