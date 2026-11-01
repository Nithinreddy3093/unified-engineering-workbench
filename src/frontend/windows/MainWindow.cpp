#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <algorithm>
#include <exception>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "../../core/common/SystemInfo.h"
#include "../../core/export/CsvExport.h"
#include "../../core/geometry/StlParser.h"
#include "../../core/geometry/TemperatureCsvParser.h"
#include "../../core/hardware/MockDevices.h"
#include "../../core/ml/Backprop.h"
#include "../../core/performance/Benchmark.h"
#include "../../core/thermal/ThermalSolver.h"
#include "GeometryViewport.h"

namespace {
using uew::ui::GeometryViewport;
constexpr UINT kSetOutputMessage = WM_APP + 1;
constexpr UINT kRunDoneMessage = WM_APP + 2;
enum ControlId { GeometryPage = 101, ThermalPage, PerformancePage, HardwarePage,
                 ImportButton, RunButton, FitButton, TrainingPage };

HWND mainWindow{};
HWND outputEdit{};
HWND runButton{};
DWORD uiThreadId{};
bool operationRunning{};
std::unique_ptr<GeometryViewport> viewport;

int currentPage(HWND window) {
    return static_cast<int>(reinterpret_cast<INT_PTR>(GetPropW(window, L"UEW.Page")));
}

void setOutput(const std::wstring& text) {
    if (GetCurrentThreadId() == uiThreadId) {
        if (outputEdit) SetWindowTextW(outputEdit, text.c_str());
        return;
    }
    auto* messageText = new std::wstring(text);
    if (!PostMessageW(mainWindow, kSetOutputMessage, 0,
                      reinterpret_cast<LPARAM>(messageText))) {
        delete messageText;
    }
}

std::wstring toWide(const std::string& text) {
    if (text.empty()) return {};
    const int required = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (required <= 1) return {};
    std::wstring result(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), required);
    result.resize(static_cast<std::size_t>(required - 1));
    return result;
}

std::string toUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int required = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1,
                                             nullptr, 0, nullptr, nullptr);
    if (required <= 1) return {};
    std::string result(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), required,
                        nullptr, nullptr);
    result.resize(static_cast<std::size_t>(required - 1));
    return result;
}

std::string chooseFile(HWND owner, const wchar_t* filter) {
    wchar_t fileName[MAX_PATH]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = fileName;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&dialog)) return {};
    return toUtf8(fileName);
}

HWND createButton(HWND parent, const wchar_t* text, int id, int x, int width = 148) {
    return CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         x, 12, width, 32, parent,
                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                         nullptr, nullptr);
}

void layoutChildren(HWND window, int width, int height) {
    const int contentTop = 58;
    const int contentHeight = std::max(80, height - contentTop - 12);
    const int page = currentPage(window);
    const bool hasViewport = page == GeometryPage || page == ThermalPage;
    if (viewport && viewport->handle()) {
        ShowWindow(viewport->handle(), hasViewport ? SW_SHOW : SW_HIDE);
        if (hasViewport) {
            const int viewportWidth = std::max(300, width * 52 / 100);
            MoveWindow(viewport->handle(), 12, contentTop, viewportWidth, contentHeight, TRUE);
            MoveWindow(outputEdit, viewportWidth + 24, contentTop,
                       std::max(260, width - viewportWidth - 36), contentHeight, TRUE);
        } else {
            MoveWindow(outputEdit, 12, contentTop, std::max(300, width - 24),
                       contentHeight, TRUE);
        }
    } else if (outputEdit) {
        MoveWindow(outputEdit, 12, contentTop, std::max(300, width - 24),
                   contentHeight, TRUE);
    }
}

void showPage(HWND window, int page) {
    SetPropW(window, L"UEW.Page", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(page)));
    EnableWindow(GetDlgItem(window, ImportButton), page == GeometryPage || page == ThermalPage);
    EnableWindow(runButton, page != GeometryPage && !operationRunning);
    if (viewport) {
        viewport->showTemperature(page == ThermalPage);
        ShowWindow(viewport->handle(),
                   page == GeometryPage || page == ThermalPage ? SW_SHOW : SW_HIDE);
    }
    switch (page) {
    case GeometryPage:
        setOutput(L"GEOMETRY\r\nImport an ASCII STL. Left-drag rotates, right-drag pans, the wheel zooms, and double-click fits the mesh.");
        break;
    case ThermalPage:
        setOutput(L"THERMAL ANALYSIS\r\nSteady 1D rod defaults: L=1 m, A=1e-4 m2, k=10 W/(m K), q=10000 W/m3. Run solves N=10,20,40,80 and exports FE/exact CSV and SVG evidence. Import a temperature CSV to inspect its 3D point cloud.");
        break;
    case PerformancePage:
        setOutput(L"PERFORMANCE\r\nRuns identical reproducible thermal cases serially and in parallel, compares numerical outputs, and exports five-repeat benchmark CSV/SVG summaries.");
        break;
    case HardwarePage:
        setOutput(L"HARDWARE — SIMULATED / MOCK\r\nMCU, FPGA, and JTAG devices are software adapters only. No physical hardware is discovered or controlled.");
        break;
    case TrainingPage:
        setOutput(L"BACKPROPAGATION\r\nHandwritten tanh network, analytical gradients, central finite-difference checks, and 100 simultaneous-update training steps.");
        break;
    }
    RECT client{};
    GetClientRect(window, &client);
    layoutChildren(window, client.right, client.bottom);
}

void importGeometry(HWND window, const std::string& path) {
    const auto result = uew::parseAsciiStl(path);
    if (const auto* error = std::get_if<uew::ParseError>(&result)) {
        setOutput(toWide(error->message));
        return;
    }
    const auto& geometry = std::get<uew::Geometry>(result);
    viewport->setGeometry(geometry);
    showPage(window, GeometryPage);
    setOutput(L"ASCII STL imported and validated.\r\nName: " + toWide(geometry.name)
              + L"\r\nTriangles: " + std::to_wstring(geometry.triangles.size()));
}

void importTemperature(HWND window, const std::string& path) {
    const auto result = uew::parseTemperatureCsv(path);
    if (const auto* error = std::get_if<uew::ParseError>(&result)) {
        setOutput(toWide(error->message));
        return;
    }
    const auto& points = std::get<std::vector<uew::TemperaturePoint>>(result);
    viewport->setTemperaturePoints(points);
    showPage(window, ThermalPage);
    setOutput(L"Temperature CSV imported.\r\nNodes: " + std::to_wstring(points.size())
              + L"\r\nClick a point in the viewport to inspect its node data."
              + L"\r\nTemperature values are shown with the CSV's declared Celsius units.");
}

void importFile(HWND window) {
    const int page = currentPage(window);
    const wchar_t* filter = page == GeometryPage
        ? L"ASCII STL (*.stl)\0*.stl\0All files\0*.*\0\0"
        : L"Temperature CSV (*.csv)\0*.csv\0All files\0*.*\0\0";
    const auto path = chooseFile(window, filter);
    if (path.empty()) return;
    try {
        if (page == GeometryPage) importGeometry(window, path);
        else importTemperature(window, path);
    } catch (const std::exception& error) {
        setOutput(toWide(error.what()));
    }
}

void runOperation(int page) {
    try {
        if (page == ThermalPage) {
            const uew::ThermalParameters parameters;
            const uew::ThermalSolver solver;
            std::wstring result = L"Nodal FE solve and element-midpoint verification\r\n"
                                  L"Elements | maximum midpoint absolute error (C)\r\n";
            for (int elements : {10, 20, 40, 80}) {
                const auto solution = solver.solve(parameters, elements);
                result += std::to_wstring(elements) + L" | "
                       + std::to_wstring(solution.maxMidpointError) + L"\r\n";
                uew::exportThermal(solution, parameters,
                    "results/thermal/N_" + std::to_string(elements));
            }
            uew::exportConvergence("results/thermal");
            result += L"\r\nCSVs and labelled SVG plots exported under results/thermal.";
            setOutput(result);
        } else if (page == PerformancePage) {
            const auto cases = uew::generateCases(80, 40, 20261031);
            const auto serial = uew::runBatch(cases, 1);
            const unsigned logicalCpus = std::max(1u, std::thread::hardware_concurrency());
            const unsigned parallelWorkers = std::min(2u, logicalCpus);
            const auto parallel = uew::runBatch(cases, parallelWorkers);
            const double difference = uew::maxBatchDifference(serial, parallel);
            const auto rows = uew::benchmark(cases, logicalCpus, 5);
            uew::exportAgreement(difference, "results/performance");
            uew::exportBenchmark(rows, "results/performance");
            const std::string environment = uew::systemInformation()
                + "Workload cases: 80\nMesh elements per case: 40\nRandom seed: 20261031\n"
                + "Warm-up: one run per worker count\nMeasured repetitions: 5\n";
            uew::exportEnvironment(environment, "results/performance");
            std::wstring result = L"Serial/parallel maximum temperature difference: "
                + std::to_wstring(difference) + L" C\r\nAgreement: "
                + (difference <= 1e-8 ? L"PASS" : L"FAIL")
                + L"\r\nWorkers | median ms | speedup | efficiency\r\n";
            for (const auto& row : rows) {
                result += std::to_wstring(row.workers) + L" | "
                       + std::to_wstring(row.medianMs) + L" | "
                       + std::to_wstring(row.speedup) + L" | "
                       + std::to_wstring(row.efficiency) + L"\r\n";
            }
            setOutput(result);
        } else if (page == HardwarePage) {
            const auto devices = uew::discoverMockDevices(true);
            std::wstring result = L"Discovered software devices (SIMULATED / MOCK)\r\n";
            for (const auto& device : devices) {
                const auto capabilities = device->capabilities();
                result += toWide(device->deviceType()) + L"\r\n  Interfaces: "
                       + toWide(capabilities.interfaces) + L"\r\n  "
                       + toWide(capabilities.description) + L"\r\n";
                auto connection = device->connect();
                result += L"  Connect: " + toWide(connection.message) + L"\r\n";
                if (!connection.ok) {
                    connection = device->connect();
                    result += L"  Retry: " + toWide(connection.message) + L"\r\n";
                }
                if (connection.ok) {
                    const auto telemetry = device->readTelemetry();
                    result += L"  Telemetry (simulated): " + toWide(telemetry.name)
                           + L" = " + std::to_wstring(telemetry.value) + L" "
                           + toWide(telemetry.unit) + L"\r\n";
                    device->disconnect();
                }
            }
            setOutput(result);
        } else if (page == TrainingPage) {
            uew::Parameters parameters;
            const auto initial = uew::forward(parameters);
            const auto gradients = uew::gradients(parameters);
            const auto checks = uew::checkGradients(parameters, 1.0, 2.0, 1e-6, 1e-5);
            const auto losses = uew::train(parameters, 100, 0.1);
            uew::exportGradientCheck(checks, "results/ml");
            uew::exportTraining(losses, "results/ml");
            std::wstring result = L"Initial parameters: w=0.5, b=0, v=1, c=0\r\n"
                L"Forward pass: z=" + std::to_wstring(initial.z)
                + L", h=" + std::to_wstring(initial.h)
                + L", y_hat=" + std::to_wstring(initial.prediction)
                + L", error=" + std::to_wstring(initial.error)
                + L", J=" + std::to_wstring(initial.loss)
                + L"\r\nGradients: dw=" + std::to_wstring(gradients.dw)
                + L", db=" + std::to_wstring(gradients.db)
                + L", dv=" + std::to_wstring(gradients.dv)
                + L", dc=" + std::to_wstring(gradients.dc)
                + L"\r\nCentral finite-difference gradient check:\r\n";
            for (const auto& check : checks) {
                result += toWide(check.parameter) + L": analytic="
                    + std::to_wstring(check.analytical) + L", numeric="
                    + std::to_wstring(check.numerical) + L", abs error="
                    + std::to_wstring(check.absoluteError) + L" "
                    + (check.pass ? L"PASS" : L"FAIL") + L"\r\n";
            }
            result += L"Loss after 100 steps: " + std::to_wstring(losses.back().loss)
                    + L"\r\nCSV and loss SVG exported under results/ml.";
            setOutput(result);
        }
    } catch (const std::exception& error) {
        setOutput(toWide(error.what()));
    }
}

LRESULT CALLBACK mainWindowProcedure(HWND window, UINT message,
                                     WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        mainWindow = window;
        createButton(window, L"Geometry", GeometryPage, 8);
        createButton(window, L"Thermal Analysis", ThermalPage, 162);
        createButton(window, L"Performance", PerformancePage, 316);
        createButton(window, L"Hardware", HardwarePage, 470);
        createButton(window, L"Backpropagation", TrainingPage, 624);
        createButton(window, L"Import", ImportButton, 778);
        runButton = createButton(window, L"Run and Export", RunButton, 932, 158);
        createButton(window, L"Fit View", FitButton, 1096, 120);
        outputEdit = CreateWindowW(
            L"EDIT", L"Unified Engineering Workbench",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            690, 58, 640, 600, window, nullptr, nullptr, nullptr);
        viewport = std::make_unique<GeometryViewport>();
        RECT viewBounds{12, 58, 670, 658};
        if (!viewport->create(window, viewBounds)) return -1;
        viewport->setInspectionHandler([](const uew::TemperaturePoint& point) {
            setOutput(L"Selected temperature node\r\nnode_id: "
                + std::to_wstring(point.nodeId)
                + L"\r\nx_m: " + std::to_wstring(point.position.x)
                + L"\r\ny_m: " + std::to_wstring(point.position.y)
                + L"\r\nz_m: " + std::to_wstring(point.position.z)
                + L"\r\ntemperature_C: " + std::to_wstring(point.temperatureC));
        });
        SetPropW(window, L"UEW.Page",
                 reinterpret_cast<HANDLE>(static_cast<INT_PTR>(ThermalPage)));
        showPage(window, ThermalPage);
        return 0;
    }
    case kSetOutputMessage: {
        std::unique_ptr<std::wstring> text(reinterpret_cast<std::wstring*>(lParam));
        if (text && outputEdit) SetWindowTextW(outputEdit, text->c_str());
        return 0;
    }
    case kRunDoneMessage:
        operationRunning = false;
        EnableWindow(runButton, currentPage(window) != GeometryPage);
        return 0;
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        if (id >= GeometryPage && id <= HardwarePage) {
            showPage(window, id);
        } else if (id == TrainingPage) {
            showPage(window, TrainingPage);
        } else if (id == ImportButton) {
            importFile(window);
        } else if (id == FitButton) {
            if (viewport) viewport->fitView();
        } else if (id == RunButton && !operationRunning) {
            const int operationPage = currentPage(window);
            operationRunning = true;
            EnableWindow(runButton, FALSE);
            setOutput(L"Operation running. Results will appear when it finishes.");
            try {
                std::thread([window, operationPage] {
                    runOperation(operationPage);
                    PostMessageW(window, kRunDoneMessage, 0, 0);
                }).detach();
            } catch (const std::exception& error) {
                operationRunning = false;
                EnableWindow(runButton, operationPage != GeometryPage);
                setOutput(toWide(std::string("Could not start analysis worker: ") + error.what()));
            }
        }
        return 0;
    }
    case WM_SIZE:
        layoutChildren(window, LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_DESTROY:
        viewport.reset();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    uiThreadId = GetCurrentThreadId();
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    if (!GeometryViewport::registerWindowClass(instance)) {
        MessageBoxW(nullptr, L"Could not register the 3D viewport window class.",
                    L"Unified Engineering Workbench", MB_ICONERROR | MB_OK);
        return 1;
    }
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = mainWindowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = L"UEW.MainWindow";
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassW(&windowClass)) return 1;
    HWND window = CreateWindowW(windowClass.lpszClassName,
        L"Unified Engineering Workbench", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1360, 760,
        nullptr, nullptr, instance, nullptr);
    if (!window) return 1;
    ShowWindow(window, show);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
#endif
