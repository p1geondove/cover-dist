# cover-dist
**Scan decimal expansion of irrational numbers until all n-digit strings have been seen; a(n) is number of digits that must be scanned.**

this replicates OEIS entries like [A080597](https://oeis.org/A080597) and [A032510](https://oeis.org/A032510) to different numbers.

all the actual code lives in `src/cover_dist/cover_dist.h` while `src/cover_dist/_core.c` is just the python binder.
so if you choose to run this program using c *(which is encouraged - python binder is a lot of overhead and abstraction)*, you can just copy paste the header wherever you need it.

> [!WARNING]
> - **PYTHON BINDER IS VIBE CODED** - more at the bottom of the readme
> - currently only decimal is supported, prs welcome!
> - theres a lot of ai used for the python binder, the core / `cover_dist.h` will stay hand coded

python wheels available only on [github](https://github.com/p1geondove/cover-dist/releases)

if youre seeing this on github, theres also a [codeberg repo](https://codeberg.org/p1geondove/cover-dist)

## build & run
its mostly a C header, but ive included a python binder, because im a sucker for python.
there are prebuilt python wheels, but these come at a cost. compiling and running this in C turns out to be about 25% faster.
ill try to adjust compile flags, but afaik natively compiling c should always be faster than prebuilt binaries.
this is all written, built and tested with [uv](https://docs.astral.sh/uv/getting-started/installation/) in mind, so you might want to install that if you want to use python.

### downloading
```bash
git clone https://github.com/p1geondove/cover-dist.git
```

### basic
```bash
uv sync
uv run demos/simple.py
```

### "manual" python install
```bash
uv pip install -e .
```

## testing
i didnt make a test.c, testing is done in python, which should actually cover the c code as well as the binder anyway

```bash
uv pip install -e .
uv run pytest
```

## ai usage
i used ai to program the python binder as you saw at the top, it also says so in the file itself.
its important for me to declare such things, since im usually not pro ai, but i guess im starting to give in.
the binder is also not fully vibe coded, its more like ai-assisted. i understand every line on a surface level, but only on a surface level.
also no actual copy pasting going on, every line is hand written (important for me to actually learn), so whenever i notice something seems unknown/fluffy to me i do some research.
im not too good with cpython-api, but i really wanted to have this, and since i use a lot of python aswell, i though this would be a good project to learn the api.
the actual math and scanning done in **`cover_dist.h` is and will always stay 100% hand coded**, even if that could mean its not as performant as it could be. im choosing correctness over speed here.

