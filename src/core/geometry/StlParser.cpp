#include "StlParser.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace uew {
namespace {
bool keyword(std::istringstream& s, const char* expected) { std::string v; return (s >> v) && v == expected; }
bool vec(std::istringstream& s, Vec3& v) { return bool(s >> v.x >> v.y >> v.z) && std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
bool noTrailingTokens(std::istringstream& s) { std::string extra; return !(s >> extra); }
ParseError err(std::size_t line, const std::string& why) { return {"STL import failed" + (line ? " near line " + std::to_string(line) : std::string{}) + ": " + why, line}; }
}
GeometryResult parseAsciiStl(const std::string& path, std::size_t maxBytes) {
    std::ifstream in(std::filesystem::u8path(path), std::ios::binary);
    if (!in) return err(0, "could not open file.");
    in.seekg(0, std::ios::end); const auto size = in.tellg(); in.seekg(0);
    if (size <= 0) return err(0, "file is empty.");
    if (static_cast<unsigned long long>(size) > maxBytes) return err(0, "file exceeds configured limit of " + std::to_string(maxBytes) + " bytes.");
    Geometry g; std::string lineText; std::size_t line = 0; bool began = false, ended = false;
    while (std::getline(in, lineText)) {
        ++line; std::istringstream s(lineText); std::string first; if (!(s >> first)) continue;
        if (!began) { if (first != "solid") return err(line, "expected 'solid' header."); began = true; std::getline(s, g.name); if (!g.name.empty() && g.name.front() == ' ') g.name.erase(0,1); continue; }
        if (first == "endsolid") { ended = true; break; }
        if (first != "facet") return err(line, "expected 'facet normal' or 'endsolid'.");
        Triangle t; if (!keyword(s,"normal") || !vec(s,t.normal) || !noTrailingTokens(s)) return err(line, "expected finite 'facet normal x y z'.");
        if (!std::getline(in,lineText)) return err(line,"expected 'outer loop'."); ++line; { std::istringstream q(lineText); if (!keyword(q,"outer") || !keyword(q,"loop") || !noTrailingTokens(q)) return err(line,"expected 'outer loop'."); }
        for (auto& v : t.vertices) { if (!std::getline(in,lineText)) return err(line,"expected 'vertex' definition."); ++line; std::istringstream q(lineText); if (!keyword(q,"vertex") || !vec(q,v) || !noTrailingTokens(q)) return err(line,"expected vertex with three finite coordinates."); }
        if (!std::getline(in,lineText)) return err(line,"expected 'endloop'."); ++line; { std::istringstream q(lineText); if (!keyword(q,"endloop") || !noTrailingTokens(q)) return err(line,"expected 'endloop'."); }
        if (!std::getline(in,lineText)) return err(line,"expected 'endfacet'."); ++line; { std::istringstream q(lineText); if (!keyword(q,"endfacet") || !noTrailingTokens(q)) return err(line,"expected 'endfacet'."); }
        g.triangles.push_back(t);
    }
    if (!began) return err(0,"file is empty or has no solid header.");
    if (!ended) return err(line,"missing 'endsolid'.");
    while (std::getline(in, lineText)) {
        ++line;
        if (lineText.find_first_not_of(" \t\r\n") != std::string::npos) {
            return err(line, "unexpected data after 'endsolid'.");
        }
    }
    if (g.triangles.empty()) return err(0,"STL contains no facets.");
    return g;
}
}
