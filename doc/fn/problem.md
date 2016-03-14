
# Function identification

The function identification problem is one of the main limitations of the current implementation of scat.

## The problem

### Description

To perform the three steps of inference that `scat` targets (*ie* arity, type and coupling), we must execute
the binary under analysis three times (one time per step).
Each step requires information infered during the previous step about functions under analysis. For example,
infering the type of parameters for a given function `f` requires to known the arity of `f` (given by the first
step of inference).  

To be able to give information about functions from an execution to another, we need a way to identify uniquely each
function. **This way must be invariant regarding different executions of the same binary.**

### Current solution
Currently, `scat` uses function names to identify them from an execution to another. This is **not** a reliable solution, 
because it would not be usable over stripped binaries. 

## Using addresses

The idea that naturally comes after using function names is to use function addresses
as identification factor. This solution should work as long as ASLR is disabled, 
but will not eventually be a strong solution for that limitation. 


### Problem
... But it seems that using addresses is not even a short-term solution. Indeed, 
it appears that functions are not located at the exact same address from an
execution to another. This applies to both embedded functions and dynamically loaded libraries.


### Illustration with an example

Let's run `grep` twice, once performing arity inference and once performing type inference.

With arity inference:
```
@0x140737018730992: fgets_unlocked
@0x140737018817216: strlen
@0x140737018817664: strnlen
@0x140737018829840: memchr
@0x140737018864000: argz_stringify
@0x140737019222112: __write
@0x140737019527280: _dl_mcount_wrapper_check
```

With type inference:
```
@0x140737023191536: fgets_unlocked
@0x140737023277760: strlen
@0x140737023278208: strnlen
@0x140737023290384: memchr
@0x140737023324544: argz_stringify
@0x140737023682656: __write
@0x140737023987824: _dl_mcount_wrapper_check
```

As we can see, a given function (*eg* `fgets_unlocked`) is not located at the same address during the first execution (`@0x140737018730992`) and at the second one (`@0x140737023191536`).

However, we have noticed that this addresses do not change from an execution to 
another if the inference performed is the same. For instance, running twice 
the program `grep` performing type inference will lead to the same addresses for a given function.

### Possible causes
First, note that `ASLR` has been disabled (`echo 0 > /proc/sys/kernel/randomize_va_space`).

The last observation we made in the previous section leads to the idea that the 
instrumentation with `Pin` might be the cause of this relocation. This has to be
explored in depth to ensure that the reason is indeed coming from the instrumentation.

## Ideas
If it is not possible to identify functions by name nor by addresses, we have to find
another invariant. Several ideas can be explored, and among them something one
could call "function signature". This would be some sort of hash function
applied to the N first (static) instructions of a given function.
