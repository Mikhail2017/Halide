#ifndef HALIDE_PYTHON_BINDINGS_PYSTUBIMPL_H
#define HALIDE_PYTHON_BINDINGS_PYSTUBIMPL_H

#include "PyHalide.h"

namespace Halide {
namespace PythonBindings {

py::object generate_impl(const Internal::GeneratorFactory &factory, const GeneratorContext &context,
                         py::args args, py::kwargs kwargs);
py::object generate_impl(const Internal::GeneratorFactory &factory, const Target &target,
                         py::args args, py::kwargs kwargs);

}  // namespace PythonBindings
}  // namespace Halide

#endif  // HALIDE_PYTHON_BINDINGS_PYSTUBIMPL_H
