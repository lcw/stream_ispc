import loopy as lp
import numpy as np
import sys


def transform(knl, vars, stream_dtype):
    vars = [v.strip() for v in vars.split(",")]
    knl = lp.assume(knl, "n>0")
    knl = lp.split_iname(
        knl, "i", 2**18, outer_tag="g.0", slabs=(0, 1))
    knl = lp.split_iname(knl, "i_inner", 8, inner_tag="l.0")
    knl = lp.tag_instructions(knl, "!streaming_store")

    knl = lp.add_and_infer_dtypes(knl, {
        var: stream_dtype
        for var in vars
        })

    knl = lp.set_argument_order(knl, vars + ["n"])

    return knl


def gen_code(knl):
    knl = lp.preprocess_kernel(knl)
    return lp.generate_code_v2(knl).all_code()


def main():
    if "-DSTREAM_TYPE=float" in sys.argv:
        stream_dtype = np.float32
    elif "-DSTREAM_TYPE=double" in sys.argv:
        stream_dtype = np.float64
    else:
        raise ValueError("STREAM_TYPE unrecognized or not found")

    index_dtype = np.int32

    def make_knl(name, insn, vars):
        knl = lp.make_kernel(
                "{[i]: 0<=i<n}",
                insn,
                target=lp.ISPCTarget(), index_dtype=index_dtype,
                name="stream_"+name+"_tasks", lang_version=(2018, 2))

        knl = transform(knl, vars, stream_dtype)
        return knl

    knls = [
        make_knl("init", """
                a[i] = 1
                b[i] = 2
                c[i] = 0
                """, "a,b,c"),
        make_knl("selfscale", """
                a[i] = scalar *a[i]
                """, "a, scalar"),
        make_knl("copy", """
                a[i] = b[i]
                """, "a,b"),
        make_knl("scale", """
                a[i] = scalar * b[i]
                """, "a,b,scalar"),
        make_knl("add", """
                a[i] = b[i] + c[i]
                """, "a,b,c"),
        make_knl("triad", """
                a[i] = b[i] + scalar * c[i]
                """, "a,b,c,scalar")
        ]

    for knl in knls:
        print(gen_code(knl))
        print()


if __name__ == "__main__":
    main()
