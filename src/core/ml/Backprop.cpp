#include "Backprop.h"
#include <array>
#include <cmath>
#include <stdexcept>

namespace uew {
namespace {
void validateInputs(const Parameters& p, double x, double target) {
    const std::array<double, 6> values{ p.w, p.b, p.v, p.c, x, target };
    for (double value : values) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("Backpropagation inputs and parameters must be finite.");
        }
    }
}
} // namespace

ForwardPass forward(const Parameters& p, double x, double target) {
    validateInputs(p, x, target);
    ForwardPass result;
    result.z = p.w * x + p.b;
    result.h = std::tanh(result.z);
    result.prediction = p.v * result.h + p.c;
    result.error = result.prediction - target;
    result.loss = 0.5 * result.error * result.error;
    if (!std::isfinite(result.loss)) {
        throw std::overflow_error("Backpropagation forward pass produced non-finite loss.");
    }
    return result;
}
Gradients gradients(const Parameters& p, double x, double target) {
    const auto result = forward(p, x, target);
    const double dLoss_dPrediction = result.error;
    const double dLoss_dHidden = dLoss_dPrediction * p.v;
    const double dLoss_dPreActivation = dLoss_dHidden * (1.0 - result.h * result.h);
    return { dLoss_dPreActivation * x, dLoss_dPreActivation,
             dLoss_dPrediction * result.h, dLoss_dPrediction };
}
std::vector<GradientCheck> checkGradients(const Parameters& p, double x, double target,
                                          double epsilon, double tolerance) {
    if (!std::isfinite(epsilon) || epsilon <= 0.0) {
        throw std::invalid_argument("Gradient check epsilon must be finite and positive.");
    }
    if (!std::isfinite(tolerance) || tolerance < 0.0) {
        throw std::invalid_argument("Gradient check tolerance must be finite and nonnegative.");
    }
    const auto analytical = gradients(p, x, target);
    const std::array<const char*, 4> names{ "w", "b", "v", "c" };
    const std::array<double, 4> analyticalValues{
        analytical.dw, analytical.db, analytical.dv, analytical.dc };
    std::vector<GradientCheck> results;
    results.reserve(names.size());
    for (std::size_t i = 0; i < names.size(); ++i) {
        Parameters plus = p;
        Parameters minus = p;
        std::array<double*, 4> plusValues{ &plus.w, &plus.b, &plus.v, &plus.c };
        std::array<double*, 4> minusValues{ &minus.w, &minus.b, &minus.v, &minus.c };
        *plusValues[i] += epsilon;
        *minusValues[i] -= epsilon;
        const double numeric = (forward(plus, x, target).loss
                              - forward(minus, x, target).loss) / (2.0 * epsilon);
        const double error = std::abs(analyticalValues[i] - numeric);
        results.push_back({ names[i], analyticalValues[i], numeric, error, error <= tolerance });
    }
    return results;
}
std::vector<TrainingPoint> train(Parameters& p, int steps, double learningRate) {
    if (steps < 0 || !std::isfinite(learningRate) || learningRate <= 0.0) {
        throw std::invalid_argument("Training requires nonnegative steps and a finite positive learning rate.");
    }
    std::vector<TrainingPoint> history;
    history.reserve(static_cast<std::size_t>(steps));
    for (int step = 1; step <= steps; ++step) {
        const Gradients gradient = gradients(p);
        const Parameters old = p; // Every update uses the same pre-update parameter values.
        p.w = old.w - learningRate * gradient.dw;
        p.b = old.b - learningRate * gradient.db;
        p.v = old.v - learningRate * gradient.dv;
        p.c = old.c - learningRate * gradient.dc;
        history.push_back({ step, forward(p).loss });
    }
    return history;
}
} // namespace uew
