#pragma once

#include "../frame_state.hpp"
#include "../simulation_engine.hpp"

#include <cstddef>

struct SceneConfig {
    int width = 800;
    int height = 600;
    double durationSeconds = 10.0;
    double xWorldHalfExtent = 2.0;
};

class OpenGLRenderer {
public:
    explicit OpenGLRenderer(SceneConfig config);

    bool init();
    void drawFrame(const FrameState& frame);
    void present();
    void shutdown();

private:
    SceneConfig m_config;
    bool m_initialized = false;

    std::size_t mapPhysicalXToColumn(double x) const;
};

void run(const SceneConfig& sceneConfig, SimulationEngine& simulationEngine);
