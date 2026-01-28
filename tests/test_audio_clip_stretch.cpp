#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "magda/daw/core/ClipInfo.hpp"
#include "magda/daw/core/ClipManager.hpp"

/**
 * Tests for audio clip time-stretching and trimming operations
 *
 * These tests verify:
 * - Audio source stretch factor clamping and behavior
 * - Trim operations maintain absolute timeline positions
 * - Stretch operations maintain file time window
 * - Left-edge resize properly trims audio file offset
 * - Audio source position compensation during clip edits
 */

TEST_CASE("AudioSource - Stretch factor basics", "[audio][clip][stretch]") {
    using namespace magda;

    SECTION("Default stretch factor is 1.0") {
        AudioSource source;
        source.filePath = "test.wav";
        source.length = 4.0;

        REQUIRE(source.stretchFactor == 1.0);

        // File window equals length when stretch factor is 1.0
        double fileWindow = source.length / source.stretchFactor;
        REQUIRE(fileWindow == 4.0);
    }

    SECTION("Stretch factor affects file time window") {
        AudioSource source;
        source.filePath = "test.wav";
        source.offset = 0.0;
        source.length = 8.0;
        source.stretchFactor = 2.0;  // 2x slower

        // File window is half the length when stretched 2x
        double fileWindow = source.length / source.stretchFactor;
        REQUIRE(fileWindow == 4.0);

        // Reading from file offset 0-4, displaying as 0-8 seconds
    }

    SECTION("Stretch factor 0.5 = 2x faster") {
        AudioSource source;
        source.filePath = "test.wav";
        source.offset = 0.0;
        source.length = 2.0;
        source.stretchFactor = 0.5;  // 2x faster

        // File window is double the length when compressed 2x
        double fileWindow = source.length / source.stretchFactor;
        REQUIRE(fileWindow == 4.0);

        // Reading from file offset 0-4, displaying as 0-2 seconds
    }
}

TEST_CASE("ClipManager - setAudioSourceStretchFactor clamping", "[audio][clip][stretch]") {
    using namespace magda;

    // Reset and setup
    ClipManager::getInstance().shutdown();

    SECTION("Stretch factor clamped to [0.25, 4.0] range") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 4.0, "test.wav");
        REQUIRE(clipId != INVALID_CLIP_ID);

        const auto* clip = ClipManager::getInstance().getClip(clipId);
        REQUIRE(clip != nullptr);
        REQUIRE(clip->audioSources.size() == 1);

        // Test minimum clamp
        ClipManager::getInstance().setAudioSourceStretchFactor(clipId, 0, 0.1);
        REQUIRE(clip->audioSources[0].stretchFactor == 0.25);

        // Test maximum clamp
        ClipManager::getInstance().setAudioSourceStretchFactor(clipId, 0, 10.0);
        REQUIRE(clip->audioSources[0].stretchFactor == 4.0);

        // Test valid range
        ClipManager::getInstance().setAudioSourceStretchFactor(clipId, 0, 1.5);
        REQUIRE(clip->audioSources[0].stretchFactor == 1.5);

        ClipManager::getInstance().setAudioSourceStretchFactor(clipId, 0, 0.5);
        REQUIRE(clip->audioSources[0].stretchFactor == 0.5);
    }

    SECTION("Invalid source index is ignored") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 4.0, "test.wav");
        const auto* clip = ClipManager::getInstance().getClip(clipId);

        double originalFactor = clip->audioSources[0].stretchFactor;

        // Try to set stretch factor on invalid index
        ClipManager::getInstance().setAudioSourceStretchFactor(clipId, 1, 2.0);
        ClipManager::getInstance().setAudioSourceStretchFactor(clipId, -1, 2.0);

        // Original should be unchanged
        REQUIRE(clip->audioSources[0].stretchFactor == originalFactor);
    }
}

TEST_CASE("Audio Clip - Left edge resize trims file offset", "[audio][clip][trim]") {
    using namespace magda;

    ClipManager::getInstance().shutdown();

    SECTION("Trim from left advances file offset (audio at clip start)") {
        // Create audio clip: starts at 0, length 4.0, audio at position 0
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 4.0, "test.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);
        REQUIRE(clip != nullptr);
        REQUIRE(clip->audioSources.size() == 1);

        auto& source = clip->audioSources[0];
        source.offset = 0.0;
        source.position = 0.0;
        source.length = 4.0;
        source.stretchFactor = 1.0;

        // Trim from left by 1.0 seconds
        ClipManager::getInstance().resizeClip(clipId, 3.0, true);

        // Clip moved right by 1.0 second
        REQUIRE(clip->startTime == 1.0);
        REQUIRE(clip->length == 3.0);

        // Audio offset advanced by 1.0 second (absolute mode)
        REQUIRE(source.offset == 1.0);
        REQUIRE(source.length == 3.0);
        REQUIRE(source.position == 0.0);

        // Audio that was at timeline position 1.0 is now at clip start
    }

    SECTION("Trim with stretch factor converts to file time") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 8.0, "test.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        auto& source = clip->audioSources[0];
        source.offset = 0.0;
        source.position = 0.0;
        source.length = 8.0;
        source.stretchFactor = 2.0;  // 2x slower, file window = 4.0

        // Trim from left by 2.0 timeline seconds
        ClipManager::getInstance().resizeClip(clipId, 6.0, true);

        REQUIRE(clip->startTime == 2.0);
        REQUIRE(clip->length == 6.0);

        // File trim amount = 2.0 / 2.0 = 1.0 file seconds
        REQUIRE(source.offset == Catch::Approx(1.0));
        REQUIRE(source.length == 6.0);
        REQUIRE(source.position == 0.0);
    }

    SECTION("Trim only empty space before audio") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 8.0, "test.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        auto& source = clip->audioSources[0];
        source.offset = 0.0;
        source.position = 2.0;  // Audio starts 2 seconds into clip
        source.length = 4.0;
        source.stretchFactor = 1.0;

        // Trim from left by 1.0 second (only empty space)
        ClipManager::getInstance().resizeClip(clipId, 7.0, true);

        REQUIRE(clip->startTime == 1.0);
        REQUIRE(clip->length == 7.0);

        // Audio moved left but offset/length unchanged
        REQUIRE(source.offset == 0.0);
        REQUIRE(source.length == 4.0);
        REQUIRE(source.position == 1.0);  // 2.0 - 1.0

        // Audio that was at timeline position 2.0 is still at 2.0 (1.0 + 1.0)
    }

    SECTION("Trim cuts partially into audio block") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 8.0, "test.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        auto& source = clip->audioSources[0];
        source.offset = 0.0;
        source.position = 1.0;  // Audio starts 1 second into clip
        source.length = 5.0;
        source.stretchFactor = 1.0;

        // Trim from left by 2.0 seconds (cuts 1.0 second into audio)
        ClipManager::getInstance().resizeClip(clipId, 6.0, true);

        REQUIRE(clip->startTime == 2.0);
        REQUIRE(clip->length == 6.0);

        // Audio trimmed by 1.0 second (2.0 - 1.0 position)
        REQUIRE(source.offset == 1.0);
        REQUIRE(source.length == 4.0);  // 5.0 - 1.0
        REQUIRE(source.position == 0.0);
    }
}

TEST_CASE("Audio Clip - Right edge resize doesn't change offset", "[audio][clip][resize]") {
    using namespace magda;

    ClipManager::getInstance().shutdown();

    SECTION("Right edge resize only changes length") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 4.0, "test.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        auto& source = clip->audioSources[0];
        source.offset = 1.0;
        source.position = 0.5;
        source.length = 4.0;

        // Resize from right edge
        ClipManager::getInstance().resizeClip(clipId, 6.0, false);

        REQUIRE(clip->startTime == 0.0);
        REQUIRE(clip->length == 6.0);

        // Audio offset and position unchanged
        REQUIRE(source.offset == 1.0);
        REQUIRE(source.position == 0.5);
        REQUIRE(source.length == 4.0);
    }
}

TEST_CASE("Audio Clip - Stretch maintains file window", "[audio][clip][stretch]") {
    using namespace magda;

    ClipManager::getInstance().shutdown();

    SECTION("Stretching by 2x doubles length but file window stays same") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 4.0, "test.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        auto& source = clip->audioSources[0];
        source.offset = 0.0;
        source.position = 0.0;
        source.length = 4.0;
        source.stretchFactor = 1.0;

        double originalFileWindow = source.length / source.stretchFactor;
        REQUIRE(originalFileWindow == 4.0);

        // Stretch 2x: length becomes 8, stretch factor becomes 2
        ClipManager::getInstance().setAudioSourceLength(clipId, 0, 8.0);
        ClipManager::getInstance().setAudioSourceStretchFactor(clipId, 0, 2.0);

        double newFileWindow = source.length / source.stretchFactor;
        REQUIRE(newFileWindow == Catch::Approx(originalFileWindow));
    }

    SECTION("Compressing by 0.5x halves length but file window stays same") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 4.0, "test.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        auto& source = clip->audioSources[0];
        source.offset = 1.0;
        source.position = 0.0;
        source.length = 4.0;
        source.stretchFactor = 1.0;

        double originalFileWindow = source.length / source.stretchFactor;
        REQUIRE(originalFileWindow == 4.0);

        // Compress 2x: length becomes 2, stretch factor becomes 0.5
        ClipManager::getInstance().setAudioSourceLength(clipId, 0, 2.0);
        ClipManager::getInstance().setAudioSourceStretchFactor(clipId, 0, 0.5);

        double newFileWindow = source.length / source.stretchFactor;
        REQUIRE(newFileWindow == Catch::Approx(originalFileWindow));

        // File offset unchanged
        REQUIRE(source.offset == 1.0);
    }
}

TEST_CASE("Audio Clip - Real-world scenario: Amen break trim", "[audio][clip][integration]") {
    using namespace magda;

    ClipManager::getInstance().shutdown();

    SECTION("Trim amen break from left preserves timeline positions") {
        // Amen break: ~4.5 bars at 120 BPM = 9 seconds
        // Beat structure: K K S K | K K S K | K K S K | K K S K | K (4.5 bars)
        // Snare hits at beats 2, 6, 10, 14 (bars 1.3, 2.3, 3.3, 4.3)
        // At 120 BPM, each beat = 0.5s, so snares at 1.0s, 3.0s, 5.0s, 7.0s

        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 9.0, "amen.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        auto& source = clip->audioSources[0];
        source.offset = 0.0;
        source.position = 0.0;
        source.length = 9.0;
        source.stretchFactor = 1.0;

        // Initial state: First snare at timeline position 1.0s
        double firstSnarePosition = 1.0;

        // Trim from left by 1.0 second (to bar 1.3, where first snare is)
        ClipManager::getInstance().resizeClip(clipId, 8.0, true);

        // Clip now starts at 1.0s
        REQUIRE(clip->startTime == 1.0);
        REQUIRE(clip->length == 8.0);

        // Audio offset advanced to 1.0s (skipping first bar)
        REQUIRE(source.offset == 1.0);
        REQUIRE(source.length == 8.0);
        REQUIRE(source.position == 0.0);

        // First snare (file position 1.0s) is now at clip start
        // Absolute timeline position: clip.startTime + source.position = 1.0 + 0.0 = 1.0s
        // This matches the original firstSnarePosition ✓

        // Verify second snare is still at 3.0s
        // File position of second snare: 3.0s
        // Relative to new offset: 3.0 - 1.0 = 2.0s into audio block
        // Timeline position: 1.0 (clip start) + 0.0 (source pos) + 2.0 = 3.0s ✓
    }

    SECTION("Trim stretched amen break converts to file time") {
        // Amen break stretched 2x slower: 18 seconds timeline duration
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 18.0, "amen.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        auto& source = clip->audioSources[0];
        source.offset = 0.0;
        source.position = 0.0;
        source.length = 18.0;
        source.stretchFactor = 2.0;  // 2x slower, file window = 9.0s

        // First snare now at timeline position 2.0s (1.0s * 2)
        double firstSnarePosition = 2.0;

        // Trim from left by 2.0 timeline seconds (to first snare)
        ClipManager::getInstance().resizeClip(clipId, 16.0, true);

        REQUIRE(clip->startTime == 2.0);
        REQUIRE(clip->length == 16.0);

        // File trim amount = 2.0 / 2.0 = 1.0 file seconds
        REQUIRE(source.offset == Catch::Approx(1.0));
        REQUIRE(source.length == 16.0);
        REQUIRE(source.position == 0.0);

        // First snare still at timeline position 2.0s ✓
    }
}

TEST_CASE("Audio Clip - Multiple audio sources", "[audio][clip][multi]") {
    using namespace magda;

    ClipManager::getInstance().shutdown();

    SECTION("Trim affects all audio sources in clip") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 8.0, "test1.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        // Add second audio source manually
        AudioSource source2;
        source2.filePath = "test2.wav";
        source2.offset = 0.0;
        source2.position = 2.0;
        source2.length = 4.0;
        source2.stretchFactor = 1.0;
        clip->audioSources.push_back(source2);

        REQUIRE(clip->audioSources.size() == 2);

        auto& source1 = clip->audioSources[0];
        source1.offset = 0.0;
        source1.position = 0.0;
        source1.length = 8.0;

        // Trim from left by 1.0 second
        ClipManager::getInstance().resizeClip(clipId, 7.0, true);

        // Source 1: audio at clip start, gets trimmed
        REQUIRE(source1.offset == 1.0);
        REQUIRE(source1.length == 7.0);
        REQUIRE(source1.position == 0.0);

        // Source 2: audio starts at 2.0, only empty space trimmed
        REQUIRE(clip->audioSources[1].offset == 0.0);
        REQUIRE(clip->audioSources[1].length == 4.0);
        REQUIRE(clip->audioSources[1].position == 1.0);  // 2.0 - 1.0
    }
}

TEST_CASE("Audio Clip - Edge cases", "[audio][clip][edge]") {
    using namespace magda;

    ClipManager::getInstance().shutdown();

    SECTION("Minimum clip length enforced") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 4.0, "test.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        // Try to resize to very small length
        ClipManager::getInstance().resizeClip(clipId, 0.01, false);

        // Clamped to minimum 0.1
        REQUIRE(clip->length == Catch::Approx(0.1));
    }

    SECTION("Minimum audio source length enforced") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 4.0, "test.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        // Try to set very small audio length
        ClipManager::getInstance().setAudioSourceLength(clipId, 0, 0.01);

        // Clamped to minimum 0.1
        REQUIRE(clip->audioSources[0].length == Catch::Approx(0.1));
    }

    SECTION("Negative position clamped to zero") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 0.0, 4.0, "test.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        // Try to set negative position
        ClipManager::getInstance().setAudioSourcePosition(clipId, 0, -1.0);

        // Clamped to zero
        REQUIRE(clip->audioSources[0].position == 0.0);
    }

    SECTION("Trim to zero start time") {
        ClipId clipId = ClipManager::getInstance().createAudioClip(1, 1.0, 4.0, "test.wav");
        auto* clip = ClipManager::getInstance().getClip(clipId);

        // Resize from left past zero
        ClipManager::getInstance().resizeClip(clipId, 6.0, true);

        // Start time clamped to zero
        REQUIRE(clip->startTime == 0.0);
        REQUIRE(clip->length == 6.0);
    }
}
