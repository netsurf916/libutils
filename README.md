# libutils

`libutils` is a small, header-focused C++ utility library plus a set of example
programs that demonstrate networking, parsing, threading, file IO, and ncurses
UI helpers. The core library lives under `include/utils/` and is built into
`libutils.a` by the provided Makefile.

## Library overview

The library is organized as a collection of independent, focused utilities that
can be pulled in via the umbrella header `include/utils/Utils.hpp`, or by
including just the headers you need. Most utilities are designed to be simple
wrappers around common C/POSIX primitives, with lightweight classes that keep
usage obvious and easy to inline in small projects.

### Design goals

- **Small and composable**: headers provide single-purpose helpers that you can
  mix and match without a large dependency tree.
- **Simple ownership rules**: objects generally manage their own resources and
  expose predictable RAII-style lifetimes.
- **Examples as documentation**: each example program is intended to show a
  realistic way to wire the utilities together.

### Core building blocks

- **Threading and synchronization**
  - `Lockable` provides a recursive mutex base for derived classes and is paired
    with the RAII `Lock` guard.
  - `Thread<T>` is a lightweight `pthread` wrapper that owns a shared context
    object for thread entry functions.
- **Data containers and helpers**
  - `BitMask` handles 32-bit flag operations with basic locking support.
  - `Buffer` is a fixed-capacity byte buffer implementing `Readable`/`Writable`
    semantics for simple serialization and IO pipelines.
  - `Staque<T>` is a hybrid stack/queue container backed by a linked list.
  - `KeyValuePair<K, V>` is a linked key/value structure with JSON export
    convenience for metadata-like lists.
- **IO interfaces**
  - `Readable` and `Writable` define the byte-stream interfaces used across the
    library (file, socket, buffer, etc.).
- **File and logging utilities**
  - `File` wraps buffered file IO and implements `Readable`/`Writable`.
  - `LogFile` appends timestamped entries to a log file path.
- **Parsing and serialization**
  - `Tokens` contains tokenization and character helpers (whitespace, numbers,
    word casing, etc.).
  - `IniFile` parses INI sections and key/value entries and can write updates.
  - `Serializable` defines a serialization interface and endian helpers.
- **Networking and HTTP**
  - `Socket` is a TCP/UDP wrapper implementing `Readable`/`Writable` with
    client/server initialization helpers.
  - `HttpRequest` parses HTTP requests and can generate basic file responses.
  - `HttpHelpers` provides small HTTP helpers like URI encoding and file checks.
  - `NetInfo` enumerates local interfaces and exposes their addresses.
- **UI helpers**
  - `Window` is an ncurses window wrapper for drawing text and random placement.

## Building

The Makefile builds the static library and example programs using C++23. It
links against pthread and ncurses for threading and terminal UI features.

Common targets:

```sh
make libutils.a   # build the static library
make httpd        # build the HTTP server example
make vic          # build the VIC cipher example
make wordsearch   # build the ncurses wordsearch demo
make all          # build the default examples (vic, httpd)
```

### Build outputs

- `libutils.a` is the static library produced from the headers in
  `include/utils/` and the implementation files in `code/utils/`.
- The example executables are produced in the repository root by default.

## Examples

### `httpd` (simple HTTP server)

The `httpd` example is a lightweight web server intended for small file serving
and request logging. It reads its configuration from `httpd.ini`, binds a TCP
listener, and processes incoming connections using a pool of threads. Internally
it exercises `Socket`, `Thread`, `HttpRequest`, `IniFile`, `LogFile`, and file
helpers to map requests to files and respond with MIME types.

Key behaviors:

- Uses an INI config for the bind address, port, and document root.
- Spawns a fixed-size thread pool (`NUMTHREADS`) to process clients.
- Parses HTTP requests, maps request paths to the document root, and writes
  status codes based on file existence.
- Logs request metadata (method, resource, status) to the configured log file.

### `vic` (VIC cipher demo)

`vic` is a standalone implementation of a VIC cipher encoder/decoder. The
program demonstrates a specialized, table-based encoding scheme using a
straddling checkerboard configuration baked into the logic. It is a compact
example of string handling, numeric transforms, and deterministic table setup.

### `wordsearch` (ncurses demo)

`wordsearch` is a simple ncurses animation that reads words from a file (one per
line) and paints them at random positions, colors, and directions. It uses the
library’s `Window`, `Thread`, `Staque`, `File`, and `Tokens` helpers to manage
rendering, input, and word selection.

Controls:

- `p` to pause/resume
- `q` to quit

## Layout

```
include/utils/   # library headers
code/utils/      # library implementation files
code/            # example programs (httpd, vic, wordsearch)
```

## License

MIT. See `LICENSE` for details.
