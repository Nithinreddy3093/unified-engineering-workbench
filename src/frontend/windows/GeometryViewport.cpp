#ifdef _WIN32

#include "GeometryViewport.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <limits>
#include <iterator>
#include <string>
#include <utility>
#include <windowsx.h>

namespace uew::ui {
namespace {
constexpr wchar_t kViewportClass[] = L"UEW.GeometryViewport";
constexpr int kLegendWidth = 22;
}

GeometryViewport::~GeometryViewport() {
    if (window_ && IsWindow(window_)) DestroyWindow(window_);
}

bool GeometryViewport::registerWindowClass(HINSTANCE instance) {
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = &GeometryViewport::windowProcedure;
    windowClass.style = CS_DBLCLKS;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kViewportClass;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_CROSS);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (RegisterClassW(&windowClass) != 0) return true;
    return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

HWND GeometryViewport::create(HWND parent, const RECT& bounds) {
    window_ = CreateWindowExW(
        WS_EX_CLIENTEDGE, kViewportClass, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        bounds.left, bounds.top, bounds.right - bounds.left,
        bounds.bottom - bounds.top, parent, nullptr, GetModuleHandleW(nullptr), this);
    return window_;
}

void GeometryViewport::setGeometry(const Geometry& geometry) {
    geometry_ = geometry;
    selectedPoint_ = -1;
    showTemperature_ = false;
    updateModelCenter();
    fitView();
}

void GeometryViewport::setTemperaturePoints(std::vector<TemperaturePoint> points) {
    temperaturePoints_ = std::move(points);
    selectedPoint_ = -1;
    updateModelCenter();
    fitView();
}

void GeometryViewport::updateModelCenter() {
    Vec3 minimum{ std::numeric_limits<double>::infinity(),
                  std::numeric_limits<double>::infinity(),
                  std::numeric_limits<double>::infinity() };
    Vec3 maximum{ -minimum.x, -minimum.y, -minimum.z };
    auto include = [&](const Vec3& point) {
        minimum.x = std::min(minimum.x, point.x);
        minimum.y = std::min(minimum.y, point.y);
        minimum.z = std::min(minimum.z, point.z);
        maximum.x = std::max(maximum.x, point.x);
        maximum.y = std::max(maximum.y, point.y);
        maximum.z = std::max(maximum.z, point.z);
    };
    if (showTemperature_) {
        for (const auto& point : temperaturePoints_) include(point.position);
    } else {
        for (const auto& triangle : geometry_.triangles) {
            for (const auto& vertex : triangle.vertices) include(vertex);
        }
    }
    if (std::isfinite(minimum.x)) {
        modelCenter_ = { minimum.x * 0.5 + maximum.x * 0.5,
                         minimum.y * 0.5 + maximum.y * 0.5,
                         minimum.z * 0.5 + maximum.z * 0.5 };
    } else {
        modelCenter_ = {};
    }
}

void GeometryViewport::showTemperature(bool enabled) {
    showTemperature_ = enabled;
    selectedPoint_ = -1;
    updateModelCenter();
    invalidate();
}

void GeometryViewport::setInspectionHandler(InspectionHandler handler) {
    inspectionHandler_ = std::move(handler);
}

void GeometryViewport::fitView() {
    yaw_ = 0.55;
    pitch_ = 0.35;
    zoom_ = 1.0;
    panX_ = 0.0;
    panY_ = 0.0;
    invalidate();
}

void GeometryViewport::invalidate() {
    if (window_) InvalidateRect(window_, nullptr, TRUE);
}

LRESULT CALLBACK GeometryViewport::windowProcedure(HWND window, UINT message,
                                                    WPARAM wParam, LPARAM lParam) {
    auto* viewport = reinterpret_cast<GeometryViewport*>(
        GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        viewport = static_cast<GeometryViewport*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(viewport));
        viewport->window_ = window;
    }
    if (!viewport) return DefWindowProcW(window, message, wParam, lParam);
    const LRESULT result = viewport->handleMessage(message, wParam, lParam);
    if (message == WM_NCDESTROY) {
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        viewport->window_ = nullptr;
    }
    return result;
}

LRESULT GeometryViewport::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        paint();
        return 0;
    case WM_LBUTTONDOWN:
        dragging_ = true;
        panning_ = false;
        pointerMoved_ = false;
        lastX_ = GET_X_LPARAM(lParam);
        lastY_ = GET_Y_LPARAM(lParam);
        SetCapture(window_);
        return 0;
    case WM_RBUTTONDOWN:
        dragging_ = true;
        panning_ = true;
        pointerMoved_ = false;
        lastX_ = GET_X_LPARAM(lParam);
        lastY_ = GET_Y_LPARAM(lParam);
        SetCapture(window_);
        return 0;
    case WM_MOUSEMOVE:
        if (dragging_ && GetCapture() == window_) {
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);
            const int dx = x - lastX_;
            const int dy = y - lastY_;
            if (std::abs(dx) + std::abs(dy) > 1) pointerMoved_ = true;
            if (panning_) {
                panX_ += dx;
                panY_ += dy;
            } else {
                yaw_ += static_cast<double>(dx) * 0.01;
                pitch_ = std::clamp(pitch_ + static_cast<double>(dy) * 0.01,
                                    -1.5, 1.5);
            }
            lastX_ = x;
            lastY_ = y;
            invalidate();
        }
        return 0;
    case WM_LBUTTONUP:
        if (dragging_ && !panning_ && !pointerMoved_ && showTemperature_) {
            inspectAt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        dragging_ = false;
        panning_ = false;
        if (GetCapture() == window_) ReleaseCapture();
        return 0;
    case WM_RBUTTONUP:
        dragging_ = false;
        panning_ = false;
        if (GetCapture() == window_) ReleaseCapture();
        return 0;
    case WM_CAPTURECHANGED:
        dragging_ = false;
        panning_ = false;
        return 0;
    case WM_MOUSEWHEEL: {
        const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        zoom_ = std::clamp(zoom_ * std::pow(1.1, delta / WHEEL_DELTA), 0.1, 30.0);
        invalidate();
        return 0;
    }
    case WM_LBUTTONDBLCLK:
        fitView();
        return 0;
    }
    return DefWindowProcW(window_, message, wParam, lParam);
}

GeometryViewport::ProjectedPoint GeometryViewport::project(
    const Vec3& point, double scale, double centerX, double centerY) const {
    double x = point.x;
    double y = point.y;
    double z = point.z;
    x -= modelCenter_.x;
    y -= modelCenter_.y;
    z -= modelCenter_.z;
    const double cy = std::cos(yaw_);
    const double sy = std::sin(yaw_);
    const double cp = std::cos(pitch_);
    const double sp = std::sin(pitch_);
    const double rotatedX = cy * x + sy * z;
    const double rotatedZ = -sy * x + cy * z;
    const double rotatedY = cp * y - sp * rotatedZ;
    const double depth = sp * y + cp * rotatedZ;
    return { centerX + rotatedX * scale + panX_,
             centerY - rotatedY * scale + panY_, depth };
}

void GeometryViewport::paint() {
    PAINTSTRUCT paintStruct{};
    HDC dc = BeginPaint(window_, &paintStruct);
    RECT bounds{};
    GetClientRect(window_, &bounds);
    FillRect(dc, &bounds, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    SetBkMode(dc, TRANSPARENT);
    const int width = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;
    const double centerX = width * 0.5;
    const double centerY = height * 0.5;
    const int legendSpace = showTemperature_ ? 58 : 0;

    Vec3 minimum{ std::numeric_limits<double>::infinity(),
                  std::numeric_limits<double>::infinity(),
                  std::numeric_limits<double>::infinity() };
    Vec3 maximum{ -minimum.x, -minimum.y, -minimum.z };
    auto include = [&](const Vec3& point) {
        minimum.x = std::min(minimum.x, point.x);
        minimum.y = std::min(minimum.y, point.y);
        minimum.z = std::min(minimum.z, point.z);
        maximum.x = std::max(maximum.x, point.x);
        maximum.y = std::max(maximum.y, point.y);
        maximum.z = std::max(maximum.z, point.z);
    };

    if (showTemperature_) {
        for (const auto& point : temperaturePoints_) include(point.position);
    } else {
        for (const auto& triangle : geometry_.triangles) {
            for (const auto& vertex : triangle.vertices) include(vertex);
        }
    }

    const bool hasData = showTemperature_ ? !temperaturePoints_.empty()
                                          : !geometry_.triangles.empty();
    if (!hasData) {
        const wchar_t* message = showTemperature_
            ? L"Import a temperature CSV to view nodes."
            : L"Import an ASCII STL to view triangles.";
        DrawTextW(dc, message, -1, &bounds,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        EndPaint(window_, &paintStruct);
        return;
    }

    const double span = std::max({ maximum.x - minimum.x,
                                   maximum.y - minimum.y,
                                   maximum.z - minimum.z, 1e-9 });
    if (!std::isfinite(span)) {
        DrawTextW(dc, L"Coordinates exceed the viewport numeric range.", -1,
                  &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        EndPaint(window_, &paintStruct);
        return;
    }
    const double scale = std::max(1.0, std::min(width - legendSpace, height) * 0.42)
                       / span * zoom_;
    const double originX = (width - legendSpace) * 0.5;

    if (showTemperature_) {
        double minTemperature = std::numeric_limits<double>::infinity();
        double maxTemperature = -std::numeric_limits<double>::infinity();
        for (const auto& point : temperaturePoints_) {
            minTemperature = std::min(minTemperature, point.temperatureC);
            maxTemperature = std::max(maxTemperature, point.temperatureC);
        }
        const double temperatureScale = std::max({ 1.0, std::abs(minTemperature),
                                                   std::abs(maxTemperature) });
        const double lowTemperature = minTemperature / temperatureScale;
        const double temperatureRange = std::max(
            1e-12, maxTemperature / temperatureScale - lowTemperature);
        for (std::size_t i = 0; i < temperaturePoints_.size(); ++i) {
            const auto& point = temperaturePoints_[i];
            const auto projected = project(point.position, scale, originX, centerY);
            const double ratio = std::clamp(
                (point.temperatureC / temperatureScale - lowTemperature) / temperatureRange,
                0.0, 1.0);
            const int red = static_cast<int>(ratio * 255.0);
            const int blue = 255 - red;
            const COLORREF color = RGB(red, 55, blue);
            HBRUSH brush = CreateSolidBrush(color);
            HPEN pen = CreatePen(PS_SOLID, i == static_cast<std::size_t>(selectedPoint_) ? 2 : 1,
                                 i == static_cast<std::size_t>(selectedPoint_) ? RGB(0, 0, 0) : color);
            HGDIOBJ oldBrush = SelectObject(dc, brush);
            HGDIOBJ oldPen = SelectObject(dc, pen);
            const int x = static_cast<int>(projected.x);
            const int y = static_cast<int>(projected.y);
            Ellipse(dc, x - 5, y - 5, x + 5, y + 5);
            SelectObject(dc, oldBrush);
            SelectObject(dc, oldPen);
            DeleteObject(brush);
            DeleteObject(pen);
        }

        const int legendX = width - 38;
        const int legendTop = 70;
        const int legendHeight = std::max(40, height - 140);
        for (int i = 0; i < legendHeight; ++i) {
            const double ratio = 1.0 - static_cast<double>(i) / (legendHeight - 1);
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(static_cast<int>(ratio * 255), 55,
                                                  255 - static_cast<int>(ratio * 255)));
            HGDIOBJ oldPen = SelectObject(dc, pen);
            MoveToEx(dc, legendX, legendTop + i, nullptr);
            LineTo(dc, legendX + kLegendWidth, legendTop + i);
            SelectObject(dc, oldPen);
            DeleteObject(pen);
        }
        wchar_t label[64]{};
        std::swprintf(label, std::size(label), L"%.2f \u00B0C", maxTemperature);
        TextOutW(dc, legendX - 8, legendTop - 22, label, lstrlenW(label));
        std::swprintf(label, std::size(label), L"%.2f \u00B0C", minTemperature);
        TextOutW(dc, legendX - 8, legendTop + legendHeight + 5, label, lstrlenW(label));
        constexpr wchar_t instructions[] = L"Left drag: rotate   Right drag: pan   Wheel: zoom   Double-click: fit   Click: inspect node";
        TextOutW(dc, 12, 12, instructions, static_cast<int>(std::size(instructions) - 1));
    } else {
        struct Face {
            POINT points[3];
            double depth{};
        };
        std::vector<Face> faces;
        faces.reserve(geometry_.triangles.size());
        for (const auto& triangle : geometry_.triangles) {
            Face face{};
            for (int i = 0; i < 3; ++i) {
                const auto point = project(triangle.vertices[static_cast<std::size_t>(i)],
                                           scale, originX, centerY);
                face.points[i] = { static_cast<LONG>(point.x), static_cast<LONG>(point.y) };
                face.depth += point.depth / 3.0;
            }
            faces.push_back(face);
        }
        std::sort(faces.begin(), faces.end(), [](const Face& a, const Face& b) {
            return a.depth < b.depth;
        });
        HBRUSH brush = CreateSolidBrush(RGB(188, 205, 222));
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(47, 75, 102));
        HGDIOBJ oldBrush = SelectObject(dc, brush);
        HGDIOBJ oldPen = SelectObject(dc, pen);
        for (const auto& face : faces) Polygon(dc, face.points, 3);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
        constexpr wchar_t instructions[] = L"Left drag: rotate   Right drag: pan   Wheel: zoom   Double-click: fit";
        TextOutW(dc, 12, 12, instructions, static_cast<int>(std::size(instructions) - 1));
    }
    EndPaint(window_, &paintStruct);
}

void GeometryViewport::inspectAt(int x, int y) {
    if (temperaturePoints_.empty() || !inspectionHandler_) return;
    RECT bounds{};
    GetClientRect(window_, &bounds);
    const double minX = std::min_element(temperaturePoints_.begin(), temperaturePoints_.end(),
        [](const auto& a, const auto& b) { return a.position.x < b.position.x; })->position.x;
    const double maxX = std::max_element(temperaturePoints_.begin(), temperaturePoints_.end(),
        [](const auto& a, const auto& b) { return a.position.x < b.position.x; })->position.x;
    const double minY = std::min_element(temperaturePoints_.begin(), temperaturePoints_.end(),
        [](const auto& a, const auto& b) { return a.position.y < b.position.y; })->position.y;
    const double maxY = std::max_element(temperaturePoints_.begin(), temperaturePoints_.end(),
        [](const auto& a, const auto& b) { return a.position.y < b.position.y; })->position.y;
    const double minZ = std::min_element(temperaturePoints_.begin(), temperaturePoints_.end(),
        [](const auto& a, const auto& b) { return a.position.z < b.position.z; })->position.z;
    const double maxZ = std::max_element(temperaturePoints_.begin(), temperaturePoints_.end(),
        [](const auto& a, const auto& b) { return a.position.z < b.position.z; })->position.z;
    const double span = std::max({ maxX - minX, maxY - minY, maxZ - minZ, 1e-9 });
    if (!std::isfinite(span)) return;
    const double scale = std::max(1.0, std::min(bounds.right - 58, bounds.bottom) * 0.42)
                       / span * zoom_;
    const double centerX = (bounds.right - 58) * 0.5;
    const double centerY = (bounds.bottom - bounds.top) * 0.5;
    double nearest = 12.0 * 12.0;
    int nearestIndex = -1;
    for (std::size_t i = 0; i < temperaturePoints_.size(); ++i) {
        const auto point = project(temperaturePoints_[i].position, scale, centerX, centerY);
        const double dx = point.x - x;
        const double dy = point.y - y;
        const double distance = dx * dx + dy * dy;
        if (distance < nearest) {
            nearest = distance;
            nearestIndex = static_cast<int>(i);
        }
    }
    if (nearestIndex >= 0) {
        selectedPoint_ = nearestIndex;
        inspectionHandler_(temperaturePoints_[static_cast<std::size_t>(nearestIndex)]);
        invalidate();
    }
}

} // namespace uew::ui

#endif
