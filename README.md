# bhashmap

A general hash table library implementation written in C11. Initially developed with the purpose
of helping better understand the ins-and-outs of the hash table data structure, it has since evolved
into a very solid and usable library.

The main goals are:

* Allow for keys and values of arbitrary length and types
* Provide a sane, easy to use API

# Setup

Development is done using [`meson`](https://mesonbuild.com/) as the build system. The only other dependecy is a modern C compiler.

1. Clone the repository 

```
$ git clone https://github.com/bool3max/bhashmap
$ cd bhashmap
```

2. Setup a build directory

```
$ meson setup <build_directory> --buildtype release
```

3. (*OPTIONAL*) Setup benchmark files

If you wish to also compile a suite of benchmark executables:
    
```
$ meson configure <build_directory> -D build_benchmarks=true
```

```
$ wget 'https://raw.githubusercontent.com/dwyl/english-words/master/words.txt'
```

4. Compile

```
$ meson compile -C <build_directory>
```

5. Install

```
# meson install -C <build_directory>
```

### Config

A few `meson` build options are available. They can be set by running `meson`'s `configure` command as follows:

```
$ meson configure <build_directory> -D <option>=<value>
```

|   **Option name**  |  **Values** |        **Description**        |
|:------------------:|:-----------:|:-----------------------------:|
| `debug_functions`  | true, false | Enable debug message logging. |
| `debug_benchmark`  | true, false | Enable benchmark logging.     |
| `build_benchmarks` | true, false | Build the benchmark suite.    |

If you built the benchmarks, they will be present as executables in the build directory with names beginning with `bench_` and can be ran directly.

# API

# Internals & design decisions

* The implementation handles collisions via the [separate chaining](https://en.wikipedia.org/wiki/Hash_table#Separate_chaining) technique.

* When storing key-value pairs in the hash map, the implementation **creates and stores copies of the keys**. This is a deliberate design decision that imposes additional memory and runtime overhead<sup>1</sup>, but allows for more freedom for the API consumer - they are free to mess with the memory of the key once it has been inserted.

* The maximum load factor after whose limit the hash map is resized, as well as the factor by which the table is resized are both static.

<sup>1</sup> Allocating memory for, copying, as well as freeing the memory of copies of the keys all take additional time and memory.