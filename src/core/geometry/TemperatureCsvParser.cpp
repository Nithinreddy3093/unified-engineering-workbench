#include "TemperatureCsvParser.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
namespace uew {
TemperatureCsvResult parseTemperatureCsv(const std::string& path, std::size_t maxBytes) {
    std::ifstream in(std::filesystem::u8path(path), std::ios::binary); if (!in) return ParseError{"Temperature CSV import failed: could not open file.",0};
    in.seekg(0,std::ios::end); auto size=in.tellg(); in.seekg(0); if(size<=0) return ParseError{"Temperature CSV is empty.",0};
    if(static_cast<unsigned long long>(size)>maxBytes) return ParseError{"Temperature CSV import failed: file exceeds configured limit of "+std::to_string(maxBytes)+" bytes.",0};
    std::string line; if(!std::getline(in,line)) return ParseError{"Temperature CSV is empty.",0};
    if(!line.empty()&&line.back()=='\r') line.pop_back();
    if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF
        && static_cast<unsigned char>(line[1]) == 0xBB
        && static_cast<unsigned char>(line[2]) == 0xBF) line.erase(0, 3);
    if(line!="node_id,x_m,y_m,z_m,temperature_C") {
        std::istringstream header(line);
        std::vector<std::string> columns;
        std::string column;
        while (std::getline(header, column, ',')) columns.push_back(column);
        const char* required[]={"node_id","x_m","y_m","z_m","temperature_C"};
        for (const char* name : required) {
            if (std::find(columns.begin(), columns.end(), name) == columns.end()) {
                return ParseError{"Temperature CSV import failed: required column '"+std::string(name)+"' is missing.",1};
            }
        }
        return ParseError{"Temperature CSV import failed: expected header node_id,x_m,y_m,z_m,temperature_C in the required order.",1};
    }
    std::vector<TemperaturePoint> points; std::size_t row=1;
    while(std::getline(in,line)) { ++row; if(!line.empty()&&line.back()=='\r')line.pop_back(); if(line.empty()) return ParseError{"Temperature CSV import failed at row "+std::to_string(row)+": blank rows are not allowed.",row};
        std::istringstream s(line); std::string fields[5]; for(int i=0;i<5;++i) { if(!std::getline(s,fields[i],',')) return ParseError{"Temperature CSV import failed at row "+std::to_string(row)+": expected 5 columns.",row}; }
        if(std::getline(s,line,',')) return ParseError{"Temperature CSV import failed at row "+std::to_string(row)+": expected 5 columns.",row};
        TemperaturePoint p; std::size_t n=0; try { p.nodeId=std::stoll(fields[0],&n); if(n!=fields[0].size()) throw 1; } catch(...) { return ParseError{"Temperature CSV import failed at row "+std::to_string(row)+": node_id is not a valid integer.",row}; }
            double* vals[]={&p.position.x,&p.position.y,&p.position.z,&p.temperatureC};
            const char* names[]={"x_m","y_m","z_m","temperature_C"};
            for(int i=0;i<4;++i){ try { n=0; *vals[i]=std::stod(fields[i+1],&n); } catch(...) { return ParseError{"Temperature CSV import failed at row "+std::to_string(row)+": "+names[i]+" is not a valid finite number.",row}; } if(n!=fields[i+1].size()||!std::isfinite(*vals[i])) return ParseError{"Temperature CSV import failed at row "+std::to_string(row)+": "+names[i]+" is not a valid finite number.",row}; }
            points.push_back(p);
    }
    if(points.empty()) return ParseError{"Temperature CSV contains a header but no data rows.",1}; return points;
}
}
