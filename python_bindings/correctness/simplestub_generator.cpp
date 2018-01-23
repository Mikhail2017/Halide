#include "Halide.h"

namespace {

class SimpleStub : public Halide::Generator<SimpleStub> {
public:
    GeneratorParam<int> offset{"offset", 0};
    GeneratorParam<LoopLevel> compute_level{ "compute_level", LoopLevel::root() };

    Input<Buffer<uint8_t>> typed_buffer_input{ "typed_buffer_input", 3 };
    Input<Func> func_input{ "func_input", 3 };  // require a 3-dimensional Func but leave Type unspecified
    Input<float> float_arg{ "float_arg", 1.0f, 0.0f, 100.0f };

    Output<Func> simple_output{ "simple_output", Float(32), 3};

    void generate() {
        simple_output(x, y, c) = cast<float>(func_input(x, y, c)
            + offset + typed_buffer_input(x, y, c)) + float_arg;
    }

    void schedule() {
        simple_output.compute_at(compute_level);
    }

private:
    Var x{"x"}, y{"y"}, c{"c"};
};

}  // namespace

HALIDE_REGISTER_GENERATOR(SimpleStub, simplestub)
