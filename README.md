# bhashmap

A general hash table library implementation written in C11. Initially developed with the purpose
of helping better understand the ins-and-outs of the hash table data structure, it has since evolved
into a very solid and usable library.

The main goals are:

* Allow for keys and values of arbitrary lengths and types
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

| **Function**           | **Arguments**                                                           | **Description**                                                                                                                                         | **Return value**                                                                            |
|------------------------|-------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------|
| `BHashMap* bhm_create` | `const size_t capacity`                                                 | Create a new BHashMap with the specified initial capacity.                                                                                              | A `BHashMap *` if the hash map was successfully created, `NULL` otherwise.                  |
| `bool bhm_set`         | `BHashMap *map, const void *key, const size_t keylen, const void *data` | Insert a new key-value pair into the map, or update the value of an existing key.                                                                       | `true` if the pair was successfully inserted, `false` otherwise.                            |
| `void *bhm_get`        | `const BHashMap *map, const void *key, const size_t keylen `            | Get the value of an existing key in the map.                                                                                                            | A `void *` pointer of the value of the given key if it exists in the map, `NULL` otherwise. |
| `size_t bhm_count`     | `const BHashMap *map`                                                   | Get the number of key-value pairs currently in the map.                                                                                                           | A `size_t` integer.                                                                         |
| `void bhm_iterate`     | `const BHashMap *map, bhm_iterator_callback callback_function`          | For each key-value pair in the map, call the passed in `callback_function`, passing it the pointer to the key, and the pointer of its associated value. | `void`                                                                                      |
| `void bhm_destroy`     | `BHashMap *`                                                            | Free all resources occupied by the BHashMap structure. Attempting to access the map afterwards is considered an error.                                  | `void`                                                                                      |* ### `bhm_create()` - create a new HashMap

### **`bhm_iterate`**

The `bhm_iterator_callback` type is defined as follows: 

```c
typedef void (*bhm_iterator_callback)(const void *key, const size_t keylen, void *value);
```

Note the **`const`** next to the `key` parameter - the key is internal to the data structure, and should not be modified once set.


# Internals & design decisions

* The implementation handles collisions via the [separate chaining](https://en.wikipedia.org/wiki/Hash_table#Separate_chaining) technique.

* When storing key-value pairs in the hash map, the implementation **creates and stores copies of the keys**. This is a deliberate design decision that imposes additional memory and runtime overhead<sup>1</sup>, but allows for more freedom for the API consumer - they are free to mess with the memory of the key once it has been inserted.

* The maximum load factor after whose limit the hash map is resized, as well as the factor by which the table is resized are both static.

<sup>1</sup> Allocating memory for, copying, as well as freeing the memory of copies of the keys all take additional time and memory.

# Examples

See [**EXAMPLES.md**](EXAMPLES.md)