#ifndef HALIDE_PYTHON_BINDINGS_PYSTUBIMPL_H
#define HALIDE_PYTHON_BINDINGS_PYSTUBIMPL_H

#include "PyHalide.h"

namespace Halide {
namespace PythonBindings {

py::object generate_impl(const Internal::GeneratorFactory &factory, const GeneratorContext &context,
                         py::object py_inputs, py::dict py_generator_params);
py::object generate_impl(const Internal::GeneratorFactory &factory, const Target &target,
                         py::object py_inputs, py::dict py_generator_params);

}  // namespace PythonBindings
}  // namespace Halide

#endif  // HALIDE_PYTHON_BINDINGS_PYSTUBIMPL_H
