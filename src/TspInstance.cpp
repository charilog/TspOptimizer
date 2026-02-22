#include "TspInstance.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

static inline std::string trim(const std::string& s)
{
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e-1]))) --e;
    return s.substr(b, e - b);
}

TspInstance TspInstance::loadFromTspFile(const std::string& path)
{
    std::ifstream in(path);
    if (!in.is_open())
        throw std::runtime_error("Failed to open file: " + path);

    TspInstance inst;
    inst.m_filePath = path;

    std::string line;
    bool inCoords = false;
    std::vector<TspPoint> pts;

    while (std::getline(in, line))
    {
        line = trim(line);
        if (line.empty())
            continue;

        if (!inCoords)
        {
            if (line.rfind("NAME", 0) == 0)
            {
                auto pos = line.find(':');
                if (pos != std::string::npos) inst.m_name = trim(line.substr(pos + 1));
            }
            if (line == "NODE_COORD_SECTION")
            {
                inCoords = true;
            }
            continue;
        }

        if (line == "EOF")
            break;

        // TSPLIB coordinates: "id x y"
        std::istringstream iss(line);
        int id = 0;
        double x = 0.0, y = 0.0;
        if (!(iss >> id >> x >> y))
            continue;

        TspPoint p;
        p.x = static_cast<int32_t>(x * 10000.0);
        p.y = static_cast<int32_t>(y * 10000.0);
        pts.push_back(p);
    }

    if (pts.empty())
        throw std::runtime_error("No coordinates were parsed from: " + path);

    inst.m_points = std::move(pts);

    inst.m_minX = inst.m_maxX = inst.m_points[0].x;
    inst.m_minY = inst.m_maxY = inst.m_points[0].y;
    for (const auto& p : inst.m_points)
    {
        inst.m_minX = std::min(inst.m_minX, p.x);
        inst.m_maxX = std::max(inst.m_maxX, p.x);
        inst.m_minY = std::min(inst.m_minY, p.y);
        inst.m_maxY = std::max(inst.m_maxY, p.y);
    }

    return inst;
}
