# bhashmap

A general-purpose hash table library implementation written in C11. Initially developed with the purpose
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

### **`bhm_create`**

```c
BHashMap *
bhm_create(const size_t capacity, const BHashMapConfig *user_config);
```

Create a new BHashMap with the specified initial capacity and configuration. If `capacity` is 0, a default value is used instead. 

The `BHashMapConfig` type is defined as follows, and allows the caller to supply additional configuration options to be used in the new hash map instance:

```c
typedef struct BHashMapConfig {
    double max_load_factor;
    size_t resize_growth_factor;
    bhm_hash_function hashfunc;
} BHashMapConfig;
```

If any of the fields of the configuration struct is `0`, a default value is used in its place. 
If `user_config` is NULL, default values are used for all of the configuration options.

The **`bhm_hash_function`** type is defined as follows, and allows the caller to use a custom hash function throughout the lifetime of hash map instance:

```c
typedef uint32_t (*bhm_hash_function)(const void *data, size_t len);
```

If the hash function one wants to use does not conform to this prototype, one may then define a wrapper that *does*, and pass that
wrapper to `bhm_create`.

Returns a `BHashMap *` on success, and `NULL` on failure.

### **`bhm_set`**

```c
bool
bhm_set(BHashMap *map, const void *key, const size_t keylen, const void *data); 
```

Insert a new key-value pair into the map, or update the associated value of an existing key.
When a new key-value pair is inerted, a copy of the key is made and stored internally.

Returns `true` on success, and `false` on failure.

### **`bhm_get`**

```c
void *
bhm_get(const BHashMap *map, const void *key, const size_t keylen); 
```

Retrieve the associated value of a key. 

Returns a pointer to the value on success, and `NULL` on failure.

### **`bhm_remove`**

```c
bool
bhm_remove(BHashMap *map, const void *key, const size_t keylen); 
```

Remove a key from the map.

Returns `true` if the key was found and removed successfully, and `false` if the wasn't found in the map.

### **`bhm_iterate`**

```c
void
bhm_iterate(const BHashMap *map, bhm_iterator_callback callback_function); 
```

For each key-value pair in the map, call the passed in callback function, passing it a pointer to the key, and a pointer to its associated value.

The `bhm_iterator_callback` type is defined as follows: 

```c
typedef void (*bhm_iterator_callback)(const void *key, const size_t keylen, void *value);
```

Note the **`const`** next to the `key` parameter - the key is internal to the data structure, and should not be modified once set.

### **`bhm_count`**

```c
size_t
bhm_count(const BHashMap *map); 
```

Return the count of key-value pairs currently in the map.

### **`bhm_destroy`**

```c
void
bhm_destroy(BHashMap *map);
```

Free all resources occupied by the `BHashMap` data structure. Attempting to access the map afterwards is considered and error.

### **`bhm_print_debug_stats`**

```c
void
bhm_print_debug_stats(const BHashMap *map, FILE *stream);
```

Log various statistics concerning the internals of the hash map to the specified `FILE *` stream.


# Internals & design decisions

* The implementation handles collisions via the [separate chaining](https://en.wikipedia.org/wiki/Hash_table#Separate_chaining) technique.

* When storing key-value pairs in the hash map, the implementation **creates and stores copies of the keys**. This is a deliberate design decision that imposes additional memory and runtime overhead<sup>1</sup>, but allows for more freedom for the API consumer - they are free to mess with the memory of the key once it has been inserted.

* The maximum load factor after whose limit the hash map is resized, as well as the factor by which the table is resized are both static.

* The default hash function for computing the hash of the keys used by the library is [MurmurHash3](https://en.wikipedia.org/wiki/MurmurHash#MurmurHash3).

<sup>1</sup> Allocating memory for, copying, as well as freeing the memory of copies of the keys all take additional time and memory.

# Examples

See [**EXAMPLES.md**](EXAMPLES.md)
