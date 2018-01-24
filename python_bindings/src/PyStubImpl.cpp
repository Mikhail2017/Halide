#include "PyStubImpl.h"

namespace Halide {
namespace PythonBindings {

using GeneratorParamsMap = Internal::GeneratorParamsMap;
using Stub = Internal::GeneratorStub;
using StubInput = Internal::StubInput;
using StubInputBuffer = Internal::StubInputBuffer<void>;

#if defined(_MSC_VER)
#ifdef Halide_EXPORTS
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif
#else
#define EXPORT __attribute__((visibility("default")))
#endif

// Experiment: return multivalued output as tuple (rather than dict)
#ifndef HALIDE_STUB_RETURN_TUPLE
    #define HALIDE_STUB_RETURN_TUPLE 1
#endif

namespace {

// Anything that defines __getitem__ looks sequencelike to pybind,
// so also check for __len_ to avoid things like Buffer and Func here.
bool is_real_sequence(py::object o) {
    return py::isinstance<py::sequence>(o) && py::hasattr(o, "__len__");
}

StubInput to_stub_input(py::object o) {
    // Don't use isinstance: we want to get things that
    // can be implicitly converted as well (eg ImageParam -> Func)
    try {
        return StubInput(StubInputBuffer(o.cast<Buffer<>>()));
    } catch (...) {
        // Not convertible to Buffer. Fall thru and try next.
    }

    try {
        return StubInput(o.cast<Func>());
    } catch (...) {
        // Not convertible to Func. Fall thru and try next.
    }

    return StubInput(o.cast<Expr>());
}

void append_input(py::object value, std::vector<StubInput> &v) {
    if (is_real_sequence(value)) {
        for (auto o : py::reinterpret_borrow<py::sequence>(value)) {
            v.push_back(to_stub_input(o));
        }
    } else {
        v.push_back(to_stub_input(value));
    }
}

}  // namespace

EXPORT py::object generate_impl(const Internal::GeneratorFactory &factory,
                                const GeneratorContext &context,
                                py::object py_inputs, py::dict py_generator_params) {
    Stub stub(context, factory);
    auto names = stub.get_names();

    std::vector<std::vector<StubInput>> inputs;
    if (is_real_sequence(py_inputs)) {
        // If inputs is listlike, unpack by position
        py::sequence s = py::reinterpret_borrow<py::sequence>(py_inputs);
        inputs.resize(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            append_input(s[i], inputs[i]);
        }
    } else if (py::isinstance<py::dict>(py_inputs)) {
        // If inputs is dictlike, look up by name and assemble in the correct order
        py::dict d = py::reinterpret_borrow<py::dict>(py_inputs);
        inputs.resize(names.inputs.size());
        for (size_t i = 0; i < names.inputs.size(); ++i) {
            py::str name = py::str(names.inputs[i]);
            _halide_user_assert(d.contains(name))
                << "The input '" << names.inputs[i] << "' must be specified.";
            append_input(d[name], inputs[i]);
        }
        _halide_user_assert(names.inputs.size() == d.size())
            << "Expected exactly " << names.inputs.size() << " inputs but got " << d.size();
    }

    GeneratorParamsMap generator_params;
    for (auto o : py_generator_params) {
        std::string key = o.first.cast<std::string>();
        if (py::isinstance<LoopLevel>(o.second)) {
            generator_params[key] = o.second.cast<LoopLevel>();
        } else {
            generator_params[key] = py::str(o.second).cast<std::string>();
        }
    }

    stub.generate(generator_params, inputs);

#if HALIDE_STUB_RETURN_TUPLE
    const auto outputs = stub.get_output_vector();
    py::tuple py_outputs(outputs.size());
    for (size_t i = 0; i < outputs.size(); i++) {
        py::object o;
        if (outputs[i].size() == 1) {
            // convert list-of-1 into single element
            o = py::cast(outputs[i][0]);
        } else {
            o = py::cast(outputs[i]);
        }
        if (outputs.size() == 1) {
            // bail early, return the single object rather than a dict
            return o;
        }
        py_outputs[i] = o;
    }
    return py_outputs;
#else
    const auto outputs = stub.get_output_map();
    py::dict py_outputs;
    py::object py_single_output;
    for (const auto &it : outputs) {
        const auto &name = it.first;
        const auto &v = it.second;
        py::object o = (v.size() == 1) ? py::cast(v[0]) : py::cast(v);
        if (outputs.size() == 1) {
            // bail early, return the single object rather than a dict
            return o;
        }
        py_outputs[py::str(name)] = o;
    }
    return py_outputs;
#endif
}

// We must ensure this is marked as 'exported' as Stubs will use it.
EXPORT py::object generate_impl(const Internal::GeneratorFactory &factory, const Target &target,
                                py::object py_inputs, py::dict py_generator_params) {
    return generate_impl(factory, GeneratorContext(target), py_inputs, py_generator_params);
}

}  // namespace PythonBindings
}  // namespace Halide

