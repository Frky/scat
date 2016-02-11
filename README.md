
# scat 

> «  Tell me! Everyone is picking up on that feline beat / 'Cause everything else is obsolete. 

> \- Strictly high-buttoned shoes. »

[What is `scat`?](#what-is-scat)

[How does it work?](#how-does-it-work)

[Some results obtained with `scat`](#some-results-obtained-with-scat)

[How to use it?](#how-to-use-it)

[Current limitations](#current-limitations-of-the-implementation)

## What is `scat`?

`scat` is a tool to recover high-level information about functions embedded in an executable
using **dynamic analysis**. In particular, `scat` aims to recover:

* **arity** of functions
* **type** of arguments
* behavioral **coupling** between functions

### Arity

By **arity**, we mean two things: first the number of parameters that a function takes, 
and second if a function returns a value or not.

### Type

`scat` infers a simplified notion of type. We indeed consider only three possible ones: `INT`, `ADDR` and `FLOAT`. 
We consider that they represent three different classes of variables that make sense semantically. 
For instance, the size of an integer (`char`, `short`, `int`, `long int`) and weither it is signed or not
does not make a significant difference semantically.  

### Coupling

We also introduce a notion of *coupling* between functions. Intuitively, two functions are coupled if they
*interact* during the execution. `scat` is also able tor recover couples of functions from one execution.

#### What is coupling?

Let's take an example, with four functions (among others) embedded in a binary: `alloc`, `realloc`, `free` and `strlen`. 
From an execution, we follow the values returned by those four functions, and see if they are given as a parameter to another
function.

![alt data flow](doc/img/data_flow.png "Data flow between functions")
**Figure 1 -** *Data flow between functions*

In **Figure 1**, an arrow from `A` to `B` means that one value returned by `A` was given as a parameter to `B`.
From this observation, what we want is to *invert* the information. It means that instead of knowing *where the
output goes*, we want to know *from where parameters come from*. So for each function, we compute the proportion 
of times a parameter is a value returned by a particular function.

![alt data flow](doc/img/couple.png "Coupling")
**Figure 2 -** *Coupling*

In **Figure 2**, we see that 70% of the parameters of `free` come directly from an output of `alloc`. Therefore, in this example 
(meaning regarding this execution), `alloc` and `free` are coupled with a coupling coefficient of 0.7. In the same way, `realloc` and `alloc`
are coupled with a coefficient of 1.

#### What for?

Finding functions coupled with a high coefficient can have different interests. 
For example, two functions that are always coupled with a high coefficient are `malloc` and `free`, or 
more generally the allocating and the liberating functions of an allocator. Therefore, the notion
of coupling can be the first step to retrieve custom allocators embedded in binaries (for security purposes, 
such as use-after-free detection). 

#### Example with `scat`

```
scat > couple ./pgm/bin/mupdf-x11 ./testfile.pdf
[*] Launching couple inference on ./pgm/bin/mupdf-x11
[*] /usr/bin/pin/pin -t ./bin/obj-intel64/couple.so -o ./log/mupdf-x11_couple_1452528857.log -i ./log/mupdf-x11_type_1452528848.log -- ./pgm/bin/mupdf-x11 ./testfile.pdf
[*] Inference results logged in ./log/mupdf-x11_couple_1452528857.log

scat > display mupdf-x11 couple
...
(jsV_toobject, newproperty[2]) -- 0.986
(jsV_toobject, insert[2]) -- 0.976
(jsV_toobject, jsV_newobject[3]) -- 0.924
(jsV_toobject, jsV_setproperty[2]) -- 0.986
(jsV_toobject, js_pushobject[2]) -- 0.926
(jsV_toobject, jsR_defproperty[2]) -- 0.986
(jsU_bsearch, fz_new_pixmap_with_data[2]) -- 0.998
(jsU_bsearch, fz_new_pixmap[2]) -- 0.998
(jsU_bsearch, jsP_newnode[4]) -- 0.82

Information about inference
| Last inference:           1970-01-01 01:00:11
| Total number of couples:  899
| Unique left/right-side:   83/244
```

## How does `scat` work?

### General Idea
`scat` uses `pin` to instrument dynamically an execution of the program. During the execution, 
we use heuristics on the use of registers and memory access to find arguments and retrieve types.
More about heuristics can be found in our [paper](paper/lightweight_heuristics_to_retrieve_parameter_associations.pdf).

### One execution (per recovery)
The goal of `scat` is not to recover information about every function embedded on the binary, but 
to demonstrate the relevance of our heuristics in a lightweight way. for this reason, `scat` only
requires on execution for each of the three steps (**arity**, **type** and **couple**).

**Pros.** The inference is very lightweight. 

**Cons.** Only functions that are executed at least one can be infered. 

## Some results obtained with `scat`

Here are presented some results obtained with `scat` on several open-source libraries. 
First, note that each result is a consequence of one single execution with standard inputs. 
Second, the accuracy was obtained by comparison between results given by `scat` and the
source code of the binary under inference. 

### Inputs used for test

* For emacs, we open a `C` source file of about 500 lines.
* For midori, we do not give any input (the browser automatically opens http://google.com/ when it starts).
* For MuPDF, we open a pdf of 67 slides generated with Keynote.
* For grep, we look for the expression "void" in a folder containing about 15000 files for a total of about 30 millions of lines.

### Arity inference

* #function is the number of functions detected during the one execution we ran.
* accuracy is the percentage of functions (that we detect) for which the arity we infered is consistant with the source code.

|              |  midori  |  grep  |  mupdf  |  emacs  |
|--------------|----------|--------|---------|---------|
| #function    | 4094     | 51     | 526     | 591     |
| accuracy (%) | 95.8     | 95.6   | 98.7    | 92.4    |


### Type inference

* #function is the number of functions detected during the one execution we ran.
* accuracy is the percentage of functions (that we detect) for which the type of each parameter we infered is consistant with the source code.

|              |  midori  |  grep  |  mupdf  |  emacs  |
|--------------|----------|--------|---------|---------|
| #function    | 4094     | 51     | 526     | 591     |
| accuracy (%) | 96.2     | 100    | 92.5    | 90.4    |

### Overhead

* #function is the number of functions detected during the one execution we ran.
* T0 is the time of execution with no instrumentation.
* T1 is the time of execution with arity inference.
* T2 is the time of execution with type inference. 

|              | grep | tar  | a2ps |
| ------------ | ---- | ---  | ---- |
| size (KB)    | 188  | 346  | 360  |
| #function    | 46   | 101  | 127  |
| T0 (s)   | 0.80 | 0.99 | 0.80 |
| T1 (s)   | 1.70 | 2.64 | 31.6 |
| T2 (s)   | 1.06 | 1.79 | 13.2 |


## How to use it?

### Requirements

* `scat` requires **python 2.7** and is not compatible with **python 3**.
* You need to have `pin` installed on your computer.
* You need `gcc`, version **lower** than (excluding) 5. Some issues are reported using a recent version of `gcc`.
* If you want to test the results of inference (see [relative section](#accuracy-of-inference)), you also need to have `libclang1-3.4` installed.

### Installation

`$LOCAL_DIR` represents the path to where you want to download `scat`.

* Clone this repository: `git clone https://github.com/Frky/scat.git $LOCAL_DIR` 
* (optional) Create a virtualenv for `scat`: `virtualenv ~/.venv/scat && source ~/.venv/scat/bin/activate`
* Install required python libraries: `pip install -r requirements.txt`

### Configuration

The configuration of `scat` is set in a yaml file, namely `./config/config.yaml`. You
can edit this file in order to fit with your own configuration. Main points are:

* `pin -> bin`: set the path to the `pin` executable. Typical value for this is `/usr/bin/pin/pin` or `/usr/bin/pin/intel64/bin/pinbin`. **Required for `scat` to work correctly.**
* `pin -> path`: set the path to the `pin` main directory. May be different from the path to the executable. Typical value for this is `/usr/bin/pin/`. **Required for `scat` to work correctly.**
* `log -> path`: set the path to the log directory.

**Important note:** You need to ensure that you have acces permissions to the path to pin. Indeed, pintools will be compiled from this directory.

If you use an esoteric linux distribution (e.g. ArchLinux), it may not be supported by `Pin` explicitly. You may encounter an error like this:

```
E: 4.3 is not a supported linux release
```

If so, you can add the command line argument `-ifeellucky` to `pin` by setting the entry `pin -> cli-options` in the configuration file.

#### Example of a configuration file

```
pin:
    path:   /usr/bin/pin
    bin: /usr/bin/pin/pin
    cli-options: -ifeellucky
    pintool-obj:
        arity: ./bin/obj-intel64/arity.so
        type: ./bin/obj-intel64/type.so
        couple: ./bin/obj-intel64/couple.so
        alloc: ./bin/obj-intel64/alloc.so
    pintool-src:
        arity: ./src/pintool/arity.cpp
        type: ./src/pintool/type.cpp
        couple: ./src/pintool/couple.cpp
        alloc: ./src/pintool/alloc.cpp

res:
    path: ./res

log:
    path: ./log

clang:
    lib-path: /usr/lib/x86_64-linux-gnu/libclang.so.1
    data-path: ./data/
```

### Basic usage

Run `scat` (from your virtualenv): `./scat.py`. You are now in th `scat` shell, where you can launch inference 
on different binaries and display results. 

### Commands

### Make pintools

Before being able to launch any inference, you must compile pintools:

```
scat > make
[*] Compiling ./src/pintool/arity.cpp ...
[*] Compiling ./src/pintool/type.cpp ...
[*] Compiling ./src/pintool/couple.cpp ...
```

### Inference

* `arity $PGM`: launch arity inference on `$PGM`, where `$PGM` is an executable and its arguments if any.
* `type $PGM`: launch type inference on `$PGM`. **Note** that it requires that arity inference was previously run on the same program.
* `couple $PGM`: launch couple inference on `$PGM`. **Note** that it requires that type inference was previously run on the same program.

```
scat > arity grep -r "def" ./src
[*] Launching arity inference on grep
[*] /usr/bin/pin/pin -t ./bin/obj-intel64/arity.so -o ./log/grep_arity_1451915233.log -- grep -r "def" ./
[*] Inference results logged in ./log/grep_arity_1451915233.log

scat > type grep -R "def" ./
[*] Launching type inference on grep
[*] /usr/bin/pin/pin -t ./bin/obj-intel64/type.so -o ./log/grep_type_1451915649.log -i ./log/grep_arity_1451915233.log -- grep -R "def" ./
[*] Inference results logged in ./log/grep_type_1451915649.log

```

### Results of inference

* `display $PGM $INF`: show the results of the last inference `$INF` on the program `$PGM`. `$INF` can be either `arity`, `type` or `couple`.

```
scat > display grep arity
0x7fffe4031c80 ('strnlen', 2, 1)
0x7fffe403d180 ('argz_stringify', 3, 0)
0x7fffe4035bf0 ('', 2, 1)
0x402c30 ('', 3, 1)
...

| Last inference:           2016-01-04 14:47:13
| Total functions infered:  62

scat > display grep type
addr fts_read(addr);
int strnlen(addr, int);
addr memchr(addr, int, int, float, float, float, float, float);
void _dl_mcount_wrapper_check();
addr fgets_unlocked(addr, int, addr);
void argz_stringify(addr, int, int);
int tcgetattr(int, addr);
int strlen(addr);

| Last inference:           2016-01-04 14:54:09
| Total functions infered:  9
```

### Accuracy of inference

#### Requirements

#### Parse data from source code

* `parsedata $PGM $SRCPATH`

#### Check inference results

* `accuracy $PGM $INF`

## Current limitations of the implementation

### Unstripped binaries

### Non object-oriented binaries

### Calling convention

