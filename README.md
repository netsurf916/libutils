# libutils

`libutils` is a small, header-focused C++ utility library plus a set of example
programs that demonstrate networking, parsing, threading, file IO, and ncurses
UI helpers. The core library lives under `include/utils/` and is built into
`libutils.a` by the provided Makefile.【F:include/utils/Utils.hpp†L1-L31】【F:Makefile†L1-L34】

## Library overview

The library is organized as a collection of independent, focused utilities that
can be pulled in via the umbrella header `include/utils/Utils.hpp`, or by
including just the headers you need.【F:include/utils/Utils.hpp†L1-L31】

### Core building blocks

- **Threading and synchronization**
  - `Lockable` provides a recursive mutex base for derived classes and is paired
    with the RAII `Lock` guard.【F:include/utils/Lockable.hpp†L1-L53】【F:include/utils/Lock.hpp†L1-L52】
  - `Thread<T>` is a lightweight `pthread` wrapper that owns a shared context
    object for thread entry functions.【F:include/utils/Thread.hpp†L1-L123】
- **Data containers and helpers**
  - `BitMask` handles 32-bit flag operations with basic locking support.【F:include/utils/BitMask.hpp†L1-L101】
  - `Buffer` is a fixed-capacity byte buffer implementing `Readable`/`Writable`
    semantics for simple serialization and IO pipelines.【F:include/utils/Buffer.hpp†L1-L150】
  - `Staque<T>` is a hybrid stack/queue container backed by a linked list.
    【F:include/utils/Staque.hpp†L1-L199】
  - `KeyValuePair<K, V>` is a linked key/value structure with JSON export
    convenience for metadata-like lists.【F:include/utils/KeyValuePair.hpp†L1-L113】
- **IO interfaces**
  - `Readable` and `Writable` define the byte-stream interfaces used across the
    library (file, socket, buffer, etc.).【F:include/utils/Readable.hpp†L1-L77】
- **File and logging utilities**
  - `File` wraps buffered file IO and implements `Readable`/`Writable`.
    【F:include/utils/File.hpp†L1-L93】
  - `LogFile` appends timestamped entries to a log file path.
    【F:include/utils/LogFile.hpp†L1-L76】
- **Parsing and serialization**
  - `Tokens` contains tokenization and character helpers (whitespace, numbers,
    word casing, etc.).【F:include/utils/Tokens.hpp†L1-L167】
  - `IniFile` parses INI sections and key/value entries and can write updates.
    【F:include/utils/IniFile.hpp†L1-L114】
  - `Serializable` defines a serialization interface and endian helpers.
    【F:include/utils/Serializable.hpp†L1-L127】
- **Networking and HTTP**
  - `Socket` is a TCP/UDP wrapper implementing `Readable`/`Writable` with
    client/server initialization helpers.【F:include/utils/Socket.hpp†L1-L182】
  - `HttpRequest` parses HTTP requests and can generate basic file responses.
    【F:include/utils/HttpRequest.hpp†L1-L120】
  - `HttpHelpers` provides small HTTP helpers like URI encoding and file checks.
    【F:include/utils/HttpHelpers.hpp†L1-L103】
  - `NetInfo` enumerates local interfaces and exposes their addresses.
    【F:include/utils/NetInfo.hpp†L1-L122】
- **UI helpers**
  - `Window` is an ncurses window wrapper for drawing text and random placement.
    【F:include/utils/Window.hpp†L1-L116】

## Building

The Makefile builds the static library and example programs using C++23. It
links against pthread and ncurses for threading and terminal UI features.
【F:Makefile†L1-L34】

Common targets:

```sh
make libutils.a   # build the static library
make httpd        # build the HTTP server example
make vic          # build the VIC cipher example
make wordsearch   # build the ncurses wordsearch demo
make all          # build the default examples (vic, httpd)
```

## Examples

### `httpd` (simple HTTP server)

The `httpd` example is a lightweight web server intended for small file serving
and request logging. It reads its configuration from `httpd.ini`, binds a TCP
listener, and processes incoming connections using a pool of threads. Internally
it exercises `Socket`, `Thread`, `HttpRequest`, `IniFile`, `LogFile`, and file
helpers to map requests to files and respond with MIME types.
【F:code/httpd.cpp†L1-L120】【F:code/httpd.cpp†L124-L200】【F:httpd.ini†L1-L42】

Key behaviors:

- Uses an INI config for the bind address, port, and document root.
  【F:code/httpd.cpp†L34-L63】【F:httpd.ini†L1-L13】
- Spawns a fixed-size thread pool (`NUMTHREADS`) to process clients.
  【F:code/httpd.cpp†L18-L118】
- Parses HTTP requests and logs request metadata for inspection.
  【F:code/httpd.cpp†L140-L200】

### `vic` (VIC cipher demo)

`vic` is a standalone implementation of a VIC cipher encoder/decoder. The
program demonstrates a specialized, table-based encoding scheme using a
straddling checkerboard configuration baked into the logic.
【F:code/vic.cpp†L1-L120】【F:code/vic.cpp†L121-L200】

### `wordsearch` (ncurses demo)

`wordsearch` is a simple ncurses animation that reads words from a file (one per
line) and paints them at random positions, colors, and directions. It uses the
library’s `Window`, `Thread`, `Staque`, `File`, and `Tokens` helpers to manage
rendering, input, and word selection.
【F:code/wordsearch.cpp†L1-L120】

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
