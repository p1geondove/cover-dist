# cover-dist
**Scan decimal expansion of irrational numbers until all n-digit strings have been seen; a(n) is number of digits that must be scanned.**

this replicates OEIS entries like [A080597](https://oeis.org/A080597) and [A032510](https://oeis.org/A032510) to different numbers.

all the actual code lives in `cover_dist.h` while `cover_dist.c` is just the python binder.

the `example.py` example utilizes a folder called `nums` where [y-cruncher](https://www.numberworld.org/y-cruncher/) digits lie in.

> [!WARNING]
> - currently only decimal is supported, prs welcome!
> - theres a lot of ai used for the python binder, the core / `cover_dist.h` will stay hand coded

## build & run
there are prebuilt python wheels, but these come at a cost. compiling and running this in C turns out to be about 25% faster.
ill try to adjust compile flags, but afaik natively compiling c should always be faster than prebuilt binaries.

its mostly a C header, but ive included a python binder, because im a sucker for python.
this is all written, built and tested with [uv](https://docs.astral.sh/uv/getting-started/installation/) in mind, so you might want to install that if you want to use python.

**the examples are not complete functioning code, add number files to `nums/`**

### downloading
```bash
git clone https://github.com/p1geondove/cover-dist.git
```

### basic python install
```bash
uv sync
uv run example.py
```

### "manual" python install
```bash
uv pip install -e .
```

### c compile
```bash
cc -O3 example.c -o example
./example
```

## testing
i didnt make a test.c, testing is done in python, which should actually cover the c code as well as the binder anyway

```bash
uv pip install -e .
uv run pytest
```

python wheels available only on [github](https://github.com/p1geondove/cover-dist/releases)

if youre seeing this on github, theres also a [codeberg repo](https://codeberg.org/p1geondove/cover-dist)
