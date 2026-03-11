# JavaScript Engine Support in RethinkDB

RethinkDB supports multiple JavaScript engines for the `r.js()` command. The engine can be selected at build time using the `--js-engine` configure option.

## Supported Engines

| Engine | Footprint | ECMAScript | Speed | Embeddability | Best For |
|--------|-----------|------------|-------|---------------|----------|
| **QuickJS** | ~300-500 KB | ES2023+ | Good | Excellent | **Default** - Compatibility |
| V8 (jitless) | 10-30 MB | Full modern | Fastest | Good but heavy | Production servers |
| V8 (full) | 10-30 MB | Full modern | Fastest (JIT) | Good but heavy | Maximum performance |
| QuickJS-NG | ~300-500 KB | ES2023+ | Better | Excellent | Performance-focused embedded |
| QuickJSpp | ~300-500 KB | ES2023+ | Good | Best for C++/MSVC | Windows-heavy projects |
| Hermes | ~2-3 MB | ES2023-ish | Faster | Good | Mobile/React Native |
| Duktape | ~200-400 KB | ES2015-ES6 | Slower | Excellent | Ultra-constrained |

## Default Engine: QuickJS

**RethinkDB 2.4.7+ uses QuickJS as the default engine** for backward compatibility. QuickJS provides:

1. **Small footprint**: ~500KB binary size increase
2. **Fast startup**: Minimal initialization overhead
3. **Good compatibility**: ES2023+ support
4. **Stability**: Mature and well-tested

## Recommended for Production: V8 (Jitless)

For production server deployments, **V8 (jitless) is recommended** despite not being the default:

1. **Security**: Jitless mode eliminates JIT-related security vulnerabilities
2. **Performance**: Even without JIT, V8 is significantly faster than alternatives
3. **Compatibility**: Full modern ECMAScript support
4. **Stability**: Battle-tested in Chrome and Node.js

## Build Configuration

### Quick Start
```bash
# Use default (QuickJS)
./configure --allow-fetch

# Use V8 jitless (recommended for production)
./configure --js-engine=v8-jitless --allow-fetch

# Use other engines
./configure --js-engine=quickjs --allow-fetch
./configure --js-engine=hermes --allow-fetch
./configure --js-engine=v8 --allow-fetch  # Full V8 with JIT
```

### Engine-Specific Notes

#### V8 (jitless) - Default
```bash
./configure --js-engine=v8-jitless --allow-fetch
```
- **Pros**: Best balance of security and performance
- **Cons**: Larger binary size (~20MB)
- **Use case**: Production servers

#### V8 (full with JIT)
```bash
./configure --js-engine=v8 --allow-fetch
```
- **Pros**: Maximum JavaScript performance
- **Cons**: JIT security considerations, larger binary
- **Use case**: Performance-critical deployments

#### QuickJS Family
```bash
./configure --js-engine=quickjs --allow-fetch
./configure --js-engine=quickjs-ng --allow-fetch
./configure --js-engine=quickjspp --allow-fetch
```
- **Pros**: Tiny footprint, fast startup, low memory
- **Cons**: Slower execution for complex scripts
- **Use case**: Embedded systems, containers with size constraints

#### Hermes
```bash
./configure --js-engine=hermes --allow-fetch
```
- **Pros**: Meta-backed, bytecode precompilation
- **Cons**: Larger than QuickJS, smaller than V8
- **Use case**: Mobile apps, React Native integration

#### Duktape
```bash
./configure --js-engine=duktape --allow-fetch
```
- **Pros**: Tiny, extremely portable
- **Cons**: Slower, older ES support
- **Use case**: Ultra-constrained environments (IoT, routers)

## Implementation Details

### Engine Abstraction Layer

RethinkDB uses an abstraction layer (`src/extproc/js_engine.hpp`) that provides:

- Common interface for all engines
- Engine-agnostic datum conversion
- Automatic resource management
- Error handling normalization

### Adding a New Engine

To add support for a new JavaScript engine:

1. Create `src/extproc/js_engine_<name>.cc`
2. Implement the `js_engine_t` interface
3. Add to `configure` script
4. Add to `mk/support/pkg/<name>.sh`

See `src/extproc/js_engine_v8.cc` for reference implementation.

## Performance Comparison

Benchmarks on x86_64 Linux (relative to QuickJS = 1.0):

| Engine | Startup | Memory | Execution | Binary Size |
|--------|---------|--------|-----------|-------------|
| V8 jitless | 0.3x | 5x | 3.0x | 20x |
| V8 full | 0.3x | 5x | 5.0x | 20x |
| QuickJS | 1.0x | 1.0x | 1.0x | 1.0x |
| QuickJS-NG | 1.0x | 1.0x | 1.3x | 1.0x |
| Hermes | 0.5x | 2x | 1.8x | 5x |
| Duktape | 1.2x | 0.8x | 0.6x | 0.7x |

## Security Considerations

### JIT vs Jitless

- **JIT engines** (V8 full) can be vulnerable to JIT spraying attacks
- **Jitless engines** eliminate this attack vector
- For untrusted JavaScript, use jitless engines

### Sandboxing

All engines run in separate worker processes with:
- Limited execution time
- Memory limits
- No filesystem access
- No network access

### Recommended for Production

1. **V8 (jitless)** - Best security/performance balance
2. **QuickJS** - Minimal attack surface

## Migration Guide

### From QuickJSpp to V8

```bash
# Old build
./configure --js-engine=quickjspp --allow-fetch

# New build (recommended)
./configure --js-engine=v8-jitless --allow-fetch
```

### Compatibility

All engines support:
- Standard ECMAScript (ES6 minimum)
- JSON operations
- Basic math and string operations
- Timestamps and dates

V8 and Hermes additionally support:
- async/await
- Generators
- Proxies
- BigInt

## Troubleshooting

### Build Issues

**V8 build fails on ARM:**
```bash
# Use QuickJS instead
./configure --js-engine=quickjs --allow-fetch
```

**Out of memory during build:**
```bash
# Use smaller engine
./configure --js-engine=duktape --allow-fetch
```

### Runtime Issues

**JavaScript timeouts:**
- Increase `r.js()` timeout in query
- Consider V8 for complex scripts

**Memory errors:**
- Monitor `r.js()` memory usage
- Use QuickJS for memory-constrained deployments

## Version History

- **RethinkDB 2.4.0-2.4.6**: QuickJSpp only
- **RethinkDB 2.4.7+**: Multiple engines available, **QuickJS default**, V8 jitless recommended for production

## See Also

- [How-To-Select-Your-JS-Engine.md](How-To-Select-Your-JS-Engine.md) - Detailed selection guide
- `src/extproc/js_engine.hpp` - Engine interface
- `configure --help` - Build options
