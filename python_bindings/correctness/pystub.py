from __future__ import print_function
from __future__ import division

import halide as hl
import complexstub
import simplestub

def _realize_and_check(f, offset = 0):
    buf = hl.Buffer(hl.Float(32), [2, 2, 1])
    f.realize(buf)
    
    assert buf[0, 0, 0] == 3.5 + offset + 123
    assert buf[0, 1, 0] == 4.5 + offset + 123
    assert buf[1, 0, 0] == 4.5 + offset + 123
    assert buf[1, 1, 0] == 5.5 + offset + 123


def test_simplestub():
    x, y, c = hl.Var(), hl.Var(), hl.Var()
    t = hl.get_jit_target_from_environment()

    typed_buffer_input = hl.Buffer(hl.UInt(8), [10, 10, 3])
    typed_buffer_input.fill(123)

    func_input = hl.Func("func_input")
    func_input[x, y, c] = x + y

    # ----------- Inputs by-position
    f = simplestub.generate(t, [typed_buffer_input, func_input, 3.5])
    _realize_and_check(f)

    # ----------- Inputs by-position w/ named arg
    f = simplestub.generate(t, inputs = [typed_buffer_input, func_input, 3.5])
    _realize_and_check(f)

    # ----------- Inputs by-name
    f = simplestub.generate(t, 
        {"float_arg": 3.5, "typed_buffer_input": typed_buffer_input, "func_input": func_input})
    _realize_and_check(f)

    # ----------- Inputs by-name w/ named arg
    f = simplestub.generate(t, 
        inputs = {"float_arg": 3.5, "func_input": func_input, "typed_buffer_input": typed_buffer_input})
    _realize_and_check(f)

    # ----------- Test with GeneratorParams
    offset = 42
    f = simplestub.generate(t, [typed_buffer_input, func_input, 3.5], {"offset": offset})
    _realize_and_check(f, offset)

    # ----------- Test with GeneratorParams w/ named args
    offset = 42
    f = simplestub.generate(t, generator_params = {"offset": offset}, inputs = [typed_buffer_input, func_input, 3.5])
    _realize_and_check(f, offset)

    # ----------- Test various failure modes
    try:
        # too many args
        f = simplestub.generate(t, [typed_buffer_input, func_input, 3.5, 4])
    except RuntimeError as e:
        assert 'Expected exactly 3 inputs but got 4' in str(e)
    else:
        assert False, 'Did not see expected exception!'

    try:
        # args that can't be converted to what the receiver needs
        f = simplestub.generate(t, [3.141592, "happy"])
    except RuntimeError as e:
        assert 'Unable to cast Python instance' in str(e)
    else:
        assert False, 'Did not see expected exception!'

    try:
        # dicts that are missing required keys
        f = simplestub.generate(t, {})
    except RuntimeError as e:
        assert "The input 'typed_buffer_input' must be specified." in str(e)
    else:
        assert False, 'Did not see expected exception!'

    try:
        # dicts that have incorrect keys
        f = simplestub.generate(t, 
            {"float_arg": 3.5, "funk_input": "wat", "typed_buffer_input": 0})
    except RuntimeError as e:
        assert "The input 'func_input' must be specified." in str(e)
    else:
        assert False, 'Did not see expected exception!'

def test_looplevel():
    x, y, c = hl.Var('x'), hl.Var('y'), hl.Var('c')
    t = hl.get_jit_target_from_environment()

    typed_buffer_input = hl.Buffer(hl.UInt(8), [10, 10, 3])
    typed_buffer_input.fill(123)

    func_input = hl.Func("func_input")
    func_input[x, y, c] = x + y

    simple_compute_at = hl.LoopLevel()
    simple = simplestub.generate(t, [typed_buffer_input, func_input, 3.5],
        {"compute_level": simple_compute_at})

    computed_output = hl.Func('computed_output')
    computed_output[x, y, c] = simple[x, y, c] + 3

    simple_compute_at.set(hl.LoopLevel(computed_output, x))

    _realize_and_check(computed_output, 3)


def _make_constant_image():
    constant_image = hl.Buffer(hl.UInt(8), [32, 32, 3], 'constant_image')
    for x in range(32):
        for y in range(32):
            for c in range(3):
                constant_image[x, y, c] = x + y + c
    return constant_image

def test_complexstub():

    constant_image = _make_constant_image()
    input = hl.ImageParam(hl.UInt(8), 3, 'input')
    input.set(constant_image)

    x, y, c = hl.Var(), hl.Var(), hl.Var()
    t = hl.get_jit_target_from_environment()

    float_arg = 1.25
    int_arg = 33
    inputs = {
        "typed_buffer_input": constant_image,
        "untyped_buffer_input": constant_image,
        "simple_input": input,
        "array_input": [ input, input ],
        "float_arg": float_arg,
        "int_arg": [ int_arg, int_arg ],
    }
    generator_params = {
        "untyped_buffer_output_type": "uint8",
        "vectorize": True,
    }

    (simple_output, tuple_output, array_output, 
    typed_buffer_output, untyped_buffer_output, 
    static_compiled_buffer_output) = complexstub.generate(t, generator_params = generator_params, inputs = inputs)

    b = simple_output.realize(32, 32, 3, t)
    assert b.type() == hl.Float(32)
    for x in range(32):
        for y in range(32):
            for c in range(3):
                expected = constant_image[x, y, c]
                actual = b[x, y, c]
                assert expected == actual, "Expected %s Actual %s" % (expected, actual)

    b = tuple_output.realize(32, 32, 3, t)
    assert b[0].type() == hl.Float(32)
    assert b[1].type() == hl.Float(32)
    assert len(b) == 2
    for x in range(32):
        for y in range(32):
            for c in range(3):
                expected1 = constant_image[x, y, c] * float_arg
                expected2 = expected1 + int_arg
                actual1, actual2 = b[0][x, y, c], b[1][x, y, c]
                assert expected1 == actual1, "Expected1 %s Actual1 %s" % (expected1, actual1)
                assert expected2 == actual2, "Expected2 %s Actual1 %s" % (expected2, actual2)

    assert len(array_output) == 2
    for a in array_output:
        b = a.realize(32, 32, t)
        assert b.type() == hl.Int(16)
        for x in range(32):
            for y in range(32):
                expected = constant_image[x, y, 0] + int_arg
                actual = b[x, y]
                assert expected == actual, "Expected %s Actual %s" % (expected, actual)

    # TODO: Output<Buffer<>> has additional behaviors useful when a Stub
    # is used within another Generator; this isn't yet implemented since there
    # isn't yet Python bindings for Generator authoring. This section
    # of the test may need revision at that point.
    b = typed_buffer_output.realize(32, 32, 3)
    assert b.type() == hl.Float(32)
    for x in range(32):
        for y in range(32):
            for c in range(3):
                expected = constant_image[x, y, c]
                actual = b[x, y, c]
                assert expected == actual, "Expected %s Actual %s" % (expected, actual)

    b = untyped_buffer_output.realize(32, 32, 3)
    assert b.type() == hl.UInt(8)
    for x in range(32):
        for y in range(32):
            for c in range(3):
                expected = constant_image[x, y, c]
                actual = b[x, y, c]
                assert expected == actual, "Expected %s Actual %s" % (expected, actual)

    b = static_compiled_buffer_output.realize(4, 4, 1)
    assert b.type() == hl.UInt(8)
    for x in range(4):
        for y in range(4):
            for c in range(1):
                expected = constant_image[x, y, c] + 42
                actual = b[x, y, c]
                assert expected == actual, "Expected %s Actual %s" % (expected, actual)


if __name__ == "__main__":
    test_simplestub()
    test_looplevel()
    test_complexstub()
