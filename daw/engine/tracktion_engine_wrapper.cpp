#include "tracktion_engine_wrapper.hpp"
#include <iostream>

namespace magica {

TracktionEngineWrapper::TracktionEngineWrapper() = default;

TracktionEngineWrapper::~TracktionEngineWrapper() {
    shutdown();
}

bool TracktionEngineWrapper::initialize() {
    try {
        // Initialize Tracktion Engine
        engine_ = std::make_unique<tracktion::Engine>("MagicaDAW");
        
        // Create a new edit (project/session)
        auto editFile = tracktion::File::createTempFile("MagicaProject");
        currentEdit_ = std::make_unique<tracktion::Edit>(*engine_, 
                                                        tracktion::ValueTree::fromXml(R"(
                                                            <EDIT>
                                                                <MASTERVOLUME>
                                                                    <PLUGIN type="volume" id="1001"/>
                                                                </MASTERVOLUME>
                                                                <TEMPOSEQUENCE>
                                                                    <TEMPO startBeat="0.0" bpm="120.0"/>
                                                                </TEMPOSEQUENCE>
                                                            </EDIT>
                                                        )"),
                                                        tracktion::Edit::forEditing,
                                                        nullptr,
                                                        0);
        
        std::cout << "Tracktion Engine initialized successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to initialize Tracktion Engine: " << e.what() << std::endl;
        return false;
    }
}

void TracktionEngineWrapper::shutdown() {
    if (currentEdit_) {
        currentEdit_.reset();
    }
    if (engine_) {
        engine_.reset();
    }
    std::cout << "Tracktion Engine shutdown complete" << std::endl;
}

CommandResponse TracktionEngineWrapper::processCommand(const Command& command) {
    const auto& type = command.getType();
    
    try {
        if (type == "play") {
            play();
            return CommandResponse(CommandResponse::Status::Success, "Playback started");
        }
        else if (type == "stop") {
            stop();
            return CommandResponse(CommandResponse::Status::Success, "Playback stopped");
        }
        else if (type == "createTrack") {
            auto name = command.getParameter<std::string>("name");
            auto trackId = createMidiTrack(name);
            
            nlohmann::json data;
            data["trackId"] = trackId;
            
            auto response = CommandResponse(CommandResponse::Status::Success, "Track created");
            response.setData(data);
            return response;
        }
        else {
            return CommandResponse(CommandResponse::Status::Error, 
                                 "Unknown command: " + type);
        }
    }
    catch (const std::exception& e) {
        return CommandResponse(CommandResponse::Status::Error, 
                             "Command execution failed: " + std::string(e.what()));
    }
}

// TransportInterface implementation
void TracktionEngineWrapper::play() {
    if (currentEdit_) {
        currentEdit_->getTransport().play(false);
        std::cout << "Playback started" << std::endl;
    }
}

void TracktionEngineWrapper::stop() {
    if (currentEdit_) {
        currentEdit_->getTransport().stop(false, false);
        std::cout << "Playback stopped" << std::endl;
    }
}

void TracktionEngineWrapper::pause() {
    stop(); // Tracktion doesn't distinguish between stop and pause
}

void TracktionEngineWrapper::record() {
    if (currentEdit_) {
        currentEdit_->getTransport().record(false);
        std::cout << "Recording started" << std::endl;
    }
}

void TracktionEngineWrapper::locate(double position_seconds) {
    if (currentEdit_) {
        currentEdit_->getTransport().setPosition(position_seconds);
    }
}

void TracktionEngineWrapper::locateMusical(int bar, int beat, int tick) {
    // Convert musical position to time
    if (currentEdit_) {
        auto& tempoSequence = currentEdit_->tempoSequence;
        auto timePosition = tempoSequence.beatsToTime(bar * 4.0 + beat - 1.0 + tick / 1000.0);
        locate(timePosition);
    }
}

double TracktionEngineWrapper::getCurrentPosition() const {
    if (currentEdit_) {
        return currentEdit_->getTransport().getCurrentPosition();
    }
    return 0.0;
}

void TracktionEngineWrapper::getCurrentMusicalPosition(int& bar, int& beat, int& tick) const {
    if (currentEdit_) {
        auto position = getCurrentPosition();
        auto& tempoSequence = currentEdit_->tempoSequence;
        auto beats = tempoSequence.timeToBeats(position);
        
        bar = static_cast<int>(beats / 4.0) + 1;
        beat = static_cast<int>(beats) % 4 + 1;
        tick = static_cast<int>((beats - static_cast<int>(beats)) * 1000);
    }
}

bool TracktionEngineWrapper::isPlaying() const {
    if (currentEdit_) {
        return currentEdit_->getTransport().isPlaying();
    }
    return false;
}

bool TracktionEngineWrapper::isRecording() const {
    if (currentEdit_) {
        return currentEdit_->getTransport().isRecording();
    }
    return false;
}

void TracktionEngineWrapper::setTempo(double bpm) {
    if (currentEdit_) {
        currentEdit_->tempoSequence.insertTempo(0.0, bpm);
    }
}

double TracktionEngineWrapper::getTempo() const {
    if (currentEdit_) {
        return currentEdit_->tempoSequence.getTempoAt(0.0).getBpm();
    }
    return 120.0;
}

void TracktionEngineWrapper::setTimeSignature(int numerator, int denominator) {
    if (currentEdit_) {
        currentEdit_->tempoSequence.insertTimeSignature(0.0, numerator, denominator);
    }
}

void TracktionEngineWrapper::getTimeSignature(int& numerator, int& denominator) const {
    if (currentEdit_) {
        auto timeSig = currentEdit_->tempoSequence.getTimeSignatureAt(0.0);
        numerator = timeSig.numerator;
        denominator = timeSig.denominator;
    } else {
        numerator = 4;
        denominator = 4;
    }
}

void TracktionEngineWrapper::setLooping(bool enabled) {
    if (currentEdit_) {
        currentEdit_->getTransport().looping = enabled;
    }
}

void TracktionEngineWrapper::setLoopRegion(double start_seconds, double end_seconds) {
    if (currentEdit_) {
        currentEdit_->getTransport().setLoopRange({start_seconds, end_seconds});
    }
}

bool TracktionEngineWrapper::isLooping() const {
    if (currentEdit_) {
        return currentEdit_->getTransport().looping;
    }
    return false;
}

// TrackInterface implementation
std::string TracktionEngineWrapper::createMidiTrack(const std::string& name) {
    if (!currentEdit_) {
        throw std::runtime_error("No edit loaded");
    }
    
    auto trackId = generateTrackId();
    auto track = currentEdit_->insertNewTrack(tracktion::TrackInsertPoint(currentEdit_.get(), {}), 
                                             nullptr, 
                                             false);
    
    if (track) {
        track->setName(name);
        trackMap_[trackId] = track;
        std::cout << "Created MIDI track: " << name << " (ID: " << trackId << ")" << std::endl;
        return trackId;
    }
    
    throw std::runtime_error("Failed to create MIDI track");
}

std::string TracktionEngineWrapper::createAudioTrack(const std::string& name) {
    // For now, same as MIDI track - Tracktion handles both
    return createMidiTrack(name);
}

void TracktionEngineWrapper::deleteTrack(const std::string& track_id) {
    auto it = trackMap_.find(track_id);
    if (it != trackMap_.end() && currentEdit_) {
        currentEdit_->deleteTrack(it->second.get());
        trackMap_.erase(it);
        std::cout << "Deleted track: " << track_id << std::endl;
    }
}

void TracktionEngineWrapper::setTrackName(const std::string& track_id, const std::string& name) {
    auto track = findTrackById(track_id);
    if (track) {
        track->setName(name);
    }
}

std::string TracktionEngineWrapper::getTrackName(const std::string& track_id) const {
    auto track = findTrackById(track_id);
    return track ? track->getName().toStdString() : "";
}

// Helper methods
tracktion::Track* TracktionEngineWrapper::findTrackById(const std::string& track_id) const {
    auto it = trackMap_.find(track_id);
    return (it != trackMap_.end()) ? it->second.get() : nullptr;
}

std::string TracktionEngineWrapper::generateTrackId() {
    return "track_" + std::to_string(nextTrackId_++);
}

std::string TracktionEngineWrapper::generateClipId() {
    return "clip_" + std::to_string(nextClipId_++);
}

// Placeholder implementations for remaining interface methods
void TracktionEngineWrapper::setTrackMuted(const std::string& track_id, bool muted) { /* TODO */ }
bool TracktionEngineWrapper::isTrackMuted(const std::string& track_id) const { return false; }
void TracktionEngineWrapper::setTrackSolo(const std::string& track_id, bool solo) { /* TODO */ }
bool TracktionEngineWrapper::isTrackSolo(const std::string& track_id) const { return false; }
void TracktionEngineWrapper::setTrackArmed(const std::string& track_id, bool armed) { /* TODO */ }
bool TracktionEngineWrapper::isTrackArmed(const std::string& track_id) const { return false; }
void TracktionEngineWrapper::setTrackColor(const std::string& track_id, int r, int g, int b) { /* TODO */ }
std::vector<std::string> TracktionEngineWrapper::getAllTrackIds() const { 
    std::vector<std::string> ids;
    for (const auto& pair : trackMap_) {
        ids.push_back(pair.first);
    }
    return ids;
}
bool TracktionEngineWrapper::trackExists(const std::string& track_id) const {
    return trackMap_.find(track_id) != trackMap_.end();
}

// ClipInterface placeholder implementations
std::string TracktionEngineWrapper::createAudioClip(const std::string& track_id, const std::string& file_path, double start_time, double length) { return ""; }
std::string TracktionEngineWrapper::createMidiClip(const std::string& track_id, double start_time, double length) { return ""; }
void TracktionEngineWrapper::deleteClip(const std::string& clip_id) { /* TODO */ }
void TracktionEngineWrapper::setClipPosition(const std::string& clip_id, double start_time) { /* TODO */ }
double TracktionEngineWrapper::getClipPosition(const std::string& clip_id) const { return 0.0; }
void TracktionEngineWrapper::setClipLength(const std::string& clip_id, double length) { /* TODO */ }
double TracktionEngineWrapper::getClipLength(const std::string& clip_id) const { return 0.0; }
std::vector<std::string> TracktionEngineWrapper::getClipsInTrack(const std::string& track_id) const { return {}; }

// MixerInterface placeholder implementations  
void TracktionEngineWrapper::setTrackVolume(const std::string& track_id, float volume) { /* TODO */ }
float TracktionEngineWrapper::getTrackVolume(const std::string& track_id) const { return 1.0f; }
void TracktionEngineWrapper::setTrackPan(const std::string& track_id, float pan) { /* TODO */ }
float TracktionEngineWrapper::getTrackPan(const std::string& track_id) const { return 0.0f; }
void TracktionEngineWrapper::setMasterVolume(float volume) { /* TODO */ }
float TracktionEngineWrapper::getMasterVolume() const { return 1.0f; }

} // namespace magica 