#pragma once
#include <array>
#include <string>
#include <vector>
namespace uew {
struct Parameters { double w{0.5},b{0.0},v{1.0},c{0.0}; };
struct ForwardPass { double z{},h{},prediction{},error{},loss{}; };
struct Gradients { double dw{},db{},dv{},dc{}; };
struct GradientCheck { std::string parameter; double analytical{},numerical{},absoluteError{}; bool pass{}; };
struct TrainingPoint { int step{}; double loss{}; };
ForwardPass forward(const Parameters&,double x=1.0,double target=2.0);
Gradients gradients(const Parameters&,double x=1.0,double target=2.0);
std::vector<GradientCheck> checkGradients(const Parameters&,double x=1.0,double target=2.0,double epsilon=1e-6,double tolerance=1e-5);
std::vector<TrainingPoint> train(Parameters&,int steps=100,double learningRate=0.1);
}
