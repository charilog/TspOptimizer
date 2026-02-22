#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct TspPoint
{
    int32_t x = 0; // scaled by 10000 (same as the Java app)
    int32_t y = 0; // scaled by 10000
};

class TspInstance
{
public:
    static TspInstance loadFromTspFile(const std::string& path); // throws std::runtime_error on error

    const std::vector<TspPoint>& points() const { return m_points; }
    int size() const { return static_cast<int>(m_points.size()); }

    int32_t minX() const { return m_minX; }
    int32_t minY() const { return m_minY; }
    int32_t maxX() const { return m_maxX; }
    int32_t maxY() const { return m_maxY; }

    const std::string& filePath() const { return m_filePath; }
    const std::string& name() const { return m_name; }

private:
    std::string m_filePath;
    std::string m_name;
    std::vector<TspPoint> m_points;

    int32_t m_minX = 0;
    int32_t m_minY = 0;
    int32_t m_maxX = 0;
    int32_t m_maxY = 0;
};
