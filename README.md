# Serial, LIFO and work stealing task schedulers

## Build

Dependencies: cc, GNU make

```
# default compiler (cc)
make test
# windows cross-compilation
env CC=x86_64-w64-mingw32-cc EXT=.exe make test
```

## Test programs

Dependency: bash

Recommended: tee

```
cd test
ls *.{serial,lifo,wsteal}*
./quicksort.wsteal -x 100 -t 8
bash stats.sh
```

## Generate plot using existing stats

Dependencies: python3, Matplotlib

```
cd test
python3 plot.py
```
