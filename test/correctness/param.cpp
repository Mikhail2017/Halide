#include <stdio.h>
#include <Halide.h>

using namespace Halide;

int main(int argc, char **argv) {

    Var x("x");
    Func f("f");

    Param<float> u;
    Param<float> u_name("u_name");

    if (u.is_explicit_name()) {
       printf("Expected autogenerated name.\n");
       return -1;
    }

    if (!u_name.is_explicit_name()) {
       printf("Expected explicit name.\n");
       return -1;
    }

    f(x) = u;

    Target target = get_jit_target_from_environment();
    if (target.has_gpu_feature()) {
        f.gpu_tile(x, 256, Device_Default_GPU);
    }

    u.set(17.0f);
    Image<float> out_17 = f.realize(1024, target);

    u.set(123.0f);
    Image<float> out_123 = f.realize(1024, target);

    for (int i = 0; i < 1024; i++) {
        if (out_17(i) != 17.0f || out_123(i) != 123.0f) {
            printf("Failed!\n");
            for (int i = 0; i < 1024; i++) {
                printf("%f %f\n", out_17(i), out_123(i));
            }
            return -1;
        }
    }

    printf("Success!\n");
    return 0;
}
