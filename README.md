# cover-dist
**Scan decimal expansion of irrational numbers until all n-digit strings have been seen; a(n) is number of digits that must be scanned.**

this replicates OEIS entries like [A080597](https://oeis.org/A080597) and [A032510](https://oeis.org/A032510) to different numbers.

scanning is done in c while multithreading and coordination is done in python.

the `example.py` example utilizes a folder called `nums` where [y-cruncher](https://www.numberworld.org/y-cruncher/) digits lie in.

> [!WARNING]
> currently only decimal is supported, prs welcome!

## build & run
```bash
uv sync
uv run example.py
```

python wheels available only on [github](https://github.com/p1geondove/cover-dist/releases)

if youre seeing this on github, theres also a [codeberg repo](https://codeberg.org/p1geondove/cover-dist)
