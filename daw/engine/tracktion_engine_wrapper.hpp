#pragma once

#include <tracktion_engine/tracktion_engine.h>
#include "../command.hpp"
#include "../interfaces/transport_interface.hpp"
#include "../interfaces/track_interface.hpp"
#include "../interfaces/clip_interface.hpp"
#include "../interfaces/mixer_interface.hpp"

namespace magica {

/**
 * @brief Wrapper around Tracktion Engine providing our DAW interfaces
 * 
 * This class bridges our command-based interface with the actual Tracktion Engine,
 * providing real audio functionality to our multi-agent DAW system.
 */
class TracktionEngineWrapper : 
    public TransportInterface,
    public TrackInterface,
    public ClipInterface,
    public MixerInterface {
    
public:
    TracktionEngineWrapper();
    ~TracktionEngineWrapper();
    
    // Initialize the engine
    bool initialize();
    void shutdown();
    
    // Process commands from MCP agents
    CommandResponse processCommand(const Command& command);
    
    // TransportInterface implementation
    void play() override;
    void stop() override;
    void pause() override;
    void record() override;
    void locate(double position_seconds) override;
    void locateMusical(int bar, int beat, int tick = 0) override;
    double getCurrentPosition() const override;
    void getCurrentMusicalPosition(int& bar, int& beat, int& tick) const override;
    bool isPlaying() const override;
    bool isRecording() const override;
    void setTempo(double bpm) override;
    double getTempo() const override;
    void setTimeSignature(int numerator, int denominator) override;
    void getTimeSignature(int& numerator, int& denominator) const override;
    void setLooping(bool enabled) override;
    void setLoopRegion(double start_seconds, double end_seconds) override;
    bool isLooping() const override;
    
    // TrackInterface implementation  
    std::string createAudioTrack(const std::string& name) override;
    std::string createMidiTrack(const std::string& name) override;
    void deleteTrack(const std::string& track_id) override;
    void setTrackName(const std::string& track_id, const std::string& name) override;
    std::string getTrackName(const std::string& track_id) const override;
    void setTrackMuted(const std::string& track_id, bool muted) override;
    bool isTrackMuted(const std::string& track_id) const override;
    void setTrackSolo(const std::string& track_id, bool solo) override;
    bool isTrackSolo(const std::string& track_id) const override;
    void setTrackArmed(const std::string& track_id, bool armed) override;
    bool isTrackArmed(const std::string& track_id) const override;
    void setTrackColor(const std::string& track_id, int r, int g, int b) override;
    std::vector<std::string> getAllTrackIds() const override;
    bool trackExists(const std::string& track_id) const override;
    
    // ClipInterface implementation
    std::string createAudioClip(const std::string& track_id, const std::string& file_path,
                               double start_time, double length) override;
    std::string createMidiClip(const std::string& track_id, double start_time, double length) override;
    void deleteClip(const std::string& clip_id) override;
    void setClipPosition(const std::string& clip_id, double start_time) override;
    double getClipPosition(const std::string& clip_id) const override;
    void setClipLength(const std::string& clip_id, double length) override;
    double getClipLength(const std::string& clip_id) const override;
    std::vector<std::string> getClipsInTrack(const std::string& track_id) const override;
    
    // MixerInterface implementation
    void setTrackVolume(const std::string& track_id, float volume) override;
    float getTrackVolume(const std::string& track_id) const override;
    void setTrackPan(const std::string& track_id, float pan) override;
    float getTrackPan(const std::string& track_id) const override;
    void setMasterVolume(float volume) override;
    float getMasterVolume() const override;
    
private:
    // Tracktion Engine components
    std::unique_ptr<tracktion::Engine> engine_;
    std::unique_ptr<tracktion::Edit> currentEdit_;
    
    // Helper methods
    tracktion::Track* findTrackById(const std::string& track_id) const;
    tracktion::Clip* findClipById(const std::string& clip_id) const;
    std::string generateTrackId();
    std::string generateClipId();
    
    // State tracking
    std::map<std::string, tracktion::Track::Ptr> trackMap_;
    std::map<std::string, tracktion::Clip::Ptr> clipMap_;
    int nextTrackId_ = 1;
    int nextClipId_ = 1;
};

} // namespace magica 