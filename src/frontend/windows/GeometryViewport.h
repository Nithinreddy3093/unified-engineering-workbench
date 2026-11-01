#pragma once

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "../../core/geometry/GeometryTypes.h"
#include <functional>
#include <vector>

namespace uew::ui {

class GeometryViewport {
public:
    using InspectionHandler = std::function<void(const TemperaturePoint&)>;

    static bool registerWindowClass(HINSTANCE instance);
    ~GeometryViewport();
    HWND create(HWND parent, const RECT& bounds);
    void setGeometry(const Geometry& geometry);
    void setTemperaturePoints(std::vector<TemperaturePoint> points);
    void showTemperature(bool enabled);
    void setInspectionHandler(InspectionHandler handler);
    void fitView();
    HWND handle() const noexcept { return window_; }

private:
    struct ProjectedPoint { double x{}, y{}, depth{}; };
    static LRESULT CALLBACK windowProcedure(HWND, UINT, WPARAM, LPARAM);
    LRESULT handleMessage(UINT, WPARAM, LPARAM);
    void paint();
    void updateModelCenter();
    ProjectedPoint project(const Vec3&, double scale, double centerX, double centerY) const;
    void inspectAt(int x, int y);
    void invalidate();

    HWND window_{};
    Geometry geometry_;
    std::vector<TemperaturePoint> temperaturePoints_;
    Vec3 modelCenter_{};
    InspectionHandler inspectionHandler_;
    bool showTemperature_{false};
    bool dragging_{false};
    bool panning_{false};
    bool pointerMoved_{false};
    int lastX_{};
    int lastY_{};
    double yaw_{0.55};
    double pitch_{0.35};
    double zoom_{1.0};
    double panX_{};
    double panY_{};
    int selectedPoint_{-1};
};

} // namespace uew::ui

#endif
