# CSE120 PA4 Tests

17 Tests for PA4 ([Preview](../master/tester/ref_outputs.txt))

## Installation

1. Put `pa4tests.c` and `tester.py` under your `pa4` folder.

2. Then do this in `Makefile`:
```bash
# Makefile
...
# 1. Add `tests` to `PA4`.
PA4 =	pa4a pa4b pa4c tests

...

# 2. Add new test target
tests:	pa4tests.c aux.h umix.h mycode4.h mycode4.o
	$(CC) $(FLAGS) $(OPTION) -o tests pa4tests.c mycode4.o

...
```

Now run `make clean tests` and it will build `./tests` executable, which you can run a target test via `N` (from 1 to 17) with:
```bash
N=1 ./tests
```

For convenience just do:
```bash
make clean tests && N=1 ./tests
```

## Running REF (Prof.) version*
You can run Prof.'s version of the thread package i.e. `InitThreads`, `CreateThread` to compare the results. 

Do this by passing environment variable `OPTION=-DREF` to `make` (which in turn will pass `-DREF` to `cc` to compile):
```bash
make clean tests OPTION=-DREF && N=1 ./tests
```
**REF version should pass all tests!**

*Note: We `clean` every time just to make sure it uses the right version when we switch between `-DREF` and without it.*

## Script to run all tests

Run all tests:
```bash
python tester.py runtests
```
Output will be written to `tester/test_outputs.txt` (look like [this](../master/tester/ref_outputs.txt)).

Run all tests with REF version:
```bash
python tester.py runtests -r
```
Output will be written to `tester/ref_outputs.txt` (look like [this](../master/tester/ref_outputs.txt)).
