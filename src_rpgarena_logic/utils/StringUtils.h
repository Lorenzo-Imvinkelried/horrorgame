#pragma once
#include <string>
#include <sstream>
#include <iomanip>

namespace Utils {

inline std::string makePaddedPath(const std::string& folder,
                                  const std::string& base,
                                  unsigned idx1based)
{
    std::ostringstream os;
    os << folder << "/" << base << std::setw(3) << std::setfill('0') << idx1based;
    // Asumimos que la extensión siempre es .png por simplicidad
    os << ".png"; 
    return os.str();
}

inline std::string makeZeroBasedPath(const std::string& folder,
                                     const std::string& base,
                                     unsigned idx0based)
{
    std::ostringstream os;
    os << folder << "/" << base << idx0based << ".png";
    return os.str();
}

} // namespace Utils