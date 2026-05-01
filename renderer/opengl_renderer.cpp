#include "opengl_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

OpenGLRenderer::OpenGLRenderer(SceneConfig config) : m_config(config) {}

bool OpenGLRenderer::init() {
    m_initialized = true;
    std::cout << "[renderer] init window/context (stub backend) " << m_config.width << "x" << m_config.height << "\n";
    return true;
}

void OpenGLRenderer::drawFrame(const FrameState& frame) {
    if (!m_initialized) {
        return;
    }

    constexpr std::size_t kCols = 80;
    constexpr std::size_t kRows = 20;

    std::vector<std::string> canvas(kRows, std::string(kCols, ' '));

    // clear
    for (auto& row : canvas) {
        std::fill(row.begin(), row.end(), ' ');
    }

    const auto it = frame.quantities.find("x");
    const double x = (it != frame.quantities.end()) ? it->second : 0.0;
    const std::size_t col = mapPhysicalXToColumn(x);
    const std::size_t row = kRows / 2;

    // draw a minimal quad/point representation of mass position
    if (row < kRows && col < kCols) {
        canvas[row][col] = '@';
    }

    std::cout << "\033[H\033[J";
    std::cout << "Noether Renderer | t=" << frame.time << "s | x=" << x << "\n";
    for (const auto& r : canvas) {
        std::cout << r << "\n";
    }
}

void OpenGLRenderer::present() {
    if (!m_initialized) {
        return;
    }
    // present is a no-op for this minimal backend.
}

void OpenGLRenderer::shutdown() {
    if (!m_initialized) {
        return;
    }
    m_initialized = false;
    std::cout << "[renderer] shutdown\n";
}

std::size_t OpenGLRenderer::mapPhysicalXToColumn(double x) const {
    constexpr std::size_t kCols = 80;
    const double halfExtent = std::max(0.0001, m_config.xWorldHalfExtent);
    const double clamped = std::max(-halfExtent, std::min(halfExtent, x));
    const double ndc = clamped / halfExtent; // [-1, 1]
    const double normalized = (ndc + 1.0) * 0.5; // [0, 1]
    return static_cast<std::size_t>(std::round(normalized * static_cast<double>(kCols - 1)));
}

void run(const SceneConfig& sceneConfig, SimulationEngine& simulationEngine) {
    OpenGLRenderer renderer(sceneConfig);
    if (!renderer.init()) {
        return;
    }

    while (true) {
        const FrameState frame = simulationEngine.currentFrame();
        renderer.drawFrame(frame);
        renderer.present();

        if (frame.time >= sceneConfig.durationSeconds) {
            break;
        }

        simulationEngine.step();
    }

    renderer.shutdown();
}
