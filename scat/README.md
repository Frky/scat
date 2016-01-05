
# scat 

> «  Tell me! Everyone is picking up on that feline beat / 'Cause everything else is obsolete. 

> \- Strictly high-buttoned shoes. »

[What is `scat`?](#what-is-scat)

[How does it work?](#how-does-it-work)

[Some results obtained with `scat`](#some-results-obtained-with-scat)

[How to use it?](#how-to-use-it)

[Coupling](#coupling)

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

#### What is coupling?

#### What for?

#### Example with `scat`

## How does it work?

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

You need to have `pin` installed on your computer.

### Installation

`$LOCAL_DIR` represents the path to where you want to download `scat`.

* Clone this repository: `git clone https://github.com/Frky/scat.git $LOCAL_DIR` 
* (optional) Create a virtualenv for `scat`: `virtualenv ~/.venv/scat && source ~/.venv/scat/bin/activate`
* Install required python libraries: `pip install -r requirements.txt`

### Configuration

The configuration of `scat` is set in a yaml file, namely `./config.yaml`. You
can edit this file in order to fit with your own configuration. Main points are:

* `pin -> path`: set the path to the `pin` executable. Required for `scat` to work correctly.
* `log -> path`: set the path to the log directory.

### Basic usage

Run `scat`: `python ./scat.py`. You are now in th `scat` shell, where you can launch inference 
on different binaries and display results. 

#### Example 

In this example, we successively infer **arity** and **type** on the binary `grep`, and display the results at each
step.

```
scat > arity grep -R "def" ./
[*] Launching arity inference on grep
[*] /usr/bin/pin/pin -t ./bin/obj-intel64/arity.so -o ./log/grep_arity_1451915233.log -- grep -R "def" ./
[*] Inference results logged in ./log/grep_arity_1451915233.log

scat > display grep arity
0x7fffe4031c80 ('strnlen', 2, 1)
0x7fffe403d180 ('argz_stringify', 3, 0)
0x7fffe4035bf0 ('', 2, 1)
0x402c30 ('', 3, 1)
...
| Last inference:           2016-01-04 14:47:13
| Total functions infered:  62

scat > type grep -R "def" ./
[*] Launching type inference on grep
[*] /usr/bin/pin/pin -t ./bin/obj-intel64/type.so -o ./log/grep_type_1451915649.log -i ./log/grep_arity_1451915233.log -- grep -R "def" ./
[*] Inference results logged in ./log/grep_type_1451915649.log

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

#### Commands

Inference:

* `arity $PGM`: launch arity inference on `$PGM`, where `$PGM` is an executable and its arguments if any.
* `type $PGM`: launch type inference on `$PGM`. **Note** that it requires that arity inference was previously run on the same program.
* `couple $PGM`: launch couple inference on `$PGM`. **Note** that it requires that type inference was previously run on the same program.

Show results:

* `display $PGM $INF`: show the results of the last inference `$INF` on the program `$PGM`. `$INF` can be either `arity`, `type` or `couple`.

