#include <pybind11/pybind11.h>

#include <iostream>
#include <string>
#include <vector>

#include "Halide.h"

namespace py = pybind11;

#ifndef HALIDE_PYSTUB_NAME
    #error "HALIDE_PYSTUB_NAME must be defined"
#endif

#define HALIDE_MAKE_NAME(a, b) PYBIND11_CONCAT(a, b)

// Forward-declare the factory function for creating an instance of our Generator
namespace halide_register_generator {
namespace HALIDE_MAKE_NAME(HALIDE_PYSTUB_NAME, _ns) {
extern std::unique_ptr<Halide::Internal::GeneratorBase> factory(const Halide::GeneratorContext& context);
}  // namespace
}  // namespace

// Forward-declare the generic PyStub implementation shared by everyone
namespace Halide {
namespace PythonBindings {
extern py::object generate_impl(const Internal::GeneratorFactory &factory, const Target &target,
                         py::object py_inputs, py::dict py_generator_params);
}  // namespace PythonBindings
}  // namespace Halide

PYBIND11_MODULE(HALIDE_PYSTUB_NAME, m) {
    m.def("generate", [](const Halide::Target &target, py::object inputs, py::dict generator_params) -> py::object {
        const Halide::Internal::GeneratorFactory factory =
            halide_register_generator:: HALIDE_MAKE_NAME(HALIDE_PYSTUB_NAME, _ns) ::factory;
        return Halide::PythonBindings::generate_impl(factory, target, inputs, generator_params);
    }, py::arg("target"), py::arg("inputs"), py::arg("generator_params") = py::dict());
}
