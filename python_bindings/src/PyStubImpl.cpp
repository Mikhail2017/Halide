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
                                py::args args, py::kwargs kwargs) {
    Stub stub(context, factory);
    auto names = stub.get_names();
    std::map<std::string, size_t> input_name_to_pos;
    for (size_t i = 0; i < names.inputs.size(); ++i) {
        input_name_to_pos[names.inputs[i]] = i;
    }

    // Inputs can be specified by either positional or named args,
    // and must all be specified.
    //
    // GeneratorParams can only be specified by name, and are always optional.
    //
    std::vector<std::vector<StubInput>> inputs;
    inputs.resize(names.inputs.size());

    GeneratorParamsMap generator_params;

    // Process the kwargs first.
    for (auto kw : kwargs) {
        // If the kwarg is the name of a known input, stick it in the input
        // vector. If not, stick it in the GeneratorParamsMap (if it's invalid,
        // an error will be reported further downstream).
        std::string key = kw.first.cast<std::string>();
        py::handle value = kw.second;
        auto it = input_name_to_pos.find(key);
        if (it != input_name_to_pos.end()) {
            append_input(py::cast<py::object>(value), inputs[it->second]);
        } else {
            if (py::isinstance<LoopLevel>(value)) {
                generator_params[key] = value.cast<LoopLevel>();
            } else {
                generator_params[key] = py::str(value).cast<std::string>();
            }
        }
    }

    // Now, the positional args.
    _halide_user_assert(args.size() <= names.inputs.size())
        << "Expected at most " << names.inputs.size() << " positional args.";
    for (size_t i = 0; i < args.size(); ++i) {
        _halide_user_assert(inputs[i].size() == 0)
            << "Generator Input named '" << names.inputs[i] << "' was specified by both position and keyword.";
        append_input(args[i], inputs[i]);
    }

    for (size_t i = 0; i < inputs.size(); ++i) {
        _halide_user_assert(inputs[i].size() != 0)
            << "Generator Input named '" << names.inputs[i] << "' was not specified.";
    }

    stub.generate(generator_params, inputs);

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
}

// We must ensure this is marked as 'exported' as Stubs will use it.
EXPORT py::object generate_impl(const Internal::GeneratorFactory &factory, const Target &target,
                                py::object py_inputs, py::dict py_generator_params) {
    return generate_impl(factory, GeneratorContext(target), py_inputs, py_generator_params);
}

}  // namespace PythonBindings
}  // namespace Halide

