# IKOS Static Analysis Setup for Triton

NASA/JPL's IKOS (Inference Kernel for Open Static analyzers) proves the absence of runtime errors in C/C++ code using abstract interpretation.

## Quick Start

```bash
# 1. Build the IKOS Docker image (one-time, ~10-15 minutes)
docker build -t ikos -f Dockerfile.ikos .

# 2. Test it works
docker run --rm ikos "ikos --version"

# 3. Analyze a file
docker run --rm -v ${PWD}:/src ikos "ikos /src/path/to/file.c"
```

## Step-by-Step Setup

### Prerequisites

1. **Docker Desktop installed and running**
   ```bash
   # Install via Homebrew
   brew install --cask docker
   
   # Launch Docker Desktop from Applications
   # Wait for the whale icon in menu bar to stop animating
   ```

2. **Verify Docker is working**
   ```bash
   docker run hello-world
   ```

### Build IKOS Image

```bash
# Navigate to directory containing Dockerfile.ikos
cd /path/to/ikos_docker

# Build image (takes 10-15 minutes first time)
docker build -t ikos -f Dockerfile.ikos .

# Verify build succeeded
docker run --rm ikos "ikos --version"
```

Expected output:
```
ikos 3.x
```

### Project Setup

Copy these files to your Triton project root:

```
triton/
├── Dockerfile.ikos          # Docker build file
├── analyze_triton.sh        # Analysis script
├── analysis/
│   └── pico_stubs.h         # Pico SDK stubs for analysis
└── src/
    └── ... your code ...
```

Create the analysis directory and copy the stubs:
```bash
mkdir -p analysis
cp pico_stubs.h analysis/
```

Make the analysis script executable:
```bash
chmod +x analyze_triton.sh
```

## Usage

### Analyze a Single File

```bash
# Basic analysis
docker run --rm -v ${PWD}:/src ikos "ikos /src/src/main.c"

# With Pico SDK stubs (for embedded code)
docker run --rm -v ${PWD}:/src ikos \
    "ikos -I/src/analysis -DIKOS_ANALYSIS /src/src/safety/safety_monitor.c"
```

### Analyze Entire Project

```bash
./analyze_triton.sh
```

### Specific Checks Only

```bash
# Buffer overflow analysis only
docker run --rm -v ${PWD}:/src ikos \
    "ikos --analyses=boa /src/src/main.c"

# Division by zero only
docker run --rm -v ${PWD}:/src ikos \
    "ikos --analyses=dbz /src/src/main.c"

# Null pointer only
docker run --rm -v ${PWD}:/src ikos \
    "ikos --analyses=nullity /src/src/main.c"
```

### All Available Checks

| Analysis | Flag | Description |
|----------|------|-------------|
| boa | Buffer Overflow | Array bounds checking |
| dbz | Division By Zero | Divide by zero detection |
| nullity | Null Pointer | Null dereference detection |
| prover | Assertion Prover | Proves assertions hold |
| uva | Uninitialized Variable | Use before initialization |
| pcmp | Pointer Comparison | Invalid pointer comparison |
| shc | Shift Count | Invalid shift amounts |
| poa | Pointer Overflow | Pointer arithmetic overflow |
| sound | Sound Mode | Most thorough (slower) |

Run all checks:
```bash
docker run --rm -v ${PWD}:/src ikos \
    "ikos --analyses=boa,dbz,nullity,prover,uva,pcmp,shc,poa /src/src/main.c"
```

## Understanding Results

### Result Status

- **safe** - Proven no error possible
- **error** - Proven error will occur
- **warning** - Possible error (couldn't prove safe)
- **unreachable** - Code path never executes

### Example Output

```
# Summary:
Total number of checks            : 42
Total number of unreachable checks: 2
Total number of safe checks       : 38
Total number of definite unsafe   : 1
Total number of warnings          : 1

The program is definitely UNSAFE

# Results
src/display.c:89:5: error: buffer overflow, array index out of bounds
    s_frame_buffer[byte_idx] |= (1 << bit_idx);
    ^
```

### View Detailed Results

IKOS generates a SQLite database with full analysis results:

```bash
# Generate report from database
docker run --rm -v ${PWD}:/src ikos \
    "ikos-report /src/ikos_reports/main.db"

# Interactive web viewer (opens on localhost:8080)
docker run --rm -v ${PWD}:/src -p 8080:8080 ikos \
    "ikos-view /src/ikos_reports/main.db"
```

## Integrating with Triton Build

### Modify Source Files

Add to the top of each source file you want to analyze:

```c
#ifdef IKOS_ANALYSIS
#include "pico_stubs.h"
#endif
```

Or create a wrapper header `triton_pico.h`:

```c
#ifndef TRITON_PICO_H
#define TRITON_PICO_H

#ifdef IKOS_ANALYSIS
#include "pico_stubs.h"
#else
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/watchdog.h"
// ... etc
#endif

#endif
```

### CI/CD Integration

Add to your GitHub Actions workflow:

```yaml
jobs:
  static-analysis:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Build IKOS image
        run: docker build -t ikos -f Dockerfile.ikos .
        
      - name: Run IKOS analysis
        run: |
          docker run --rm -v ${PWD}:/src ikos \
            "ikos -I/src/analysis -DIKOS_ANALYSIS \
                  --analyses=boa,dbz,nullity \
                  /src/src/safety/*.c"
```

## Troubleshooting

### Build Fails

```bash
# Clean Docker cache and rebuild
docker builder prune -f
docker build --no-cache -t ikos -f Dockerfile.ikos .
```

### "Cannot find header file"

Include paths must be absolute inside container:
```bash
# Wrong
ikos -I./include /src/main.c

# Correct
ikos -I/src/include /src/main.c
```

### Analysis Takes Too Long

Reduce scope:
```bash
# Limit analysis depth
docker run --rm -v ${PWD}:/src ikos \
    "ikos --proc=intra /src/src/main.c"

# Skip some checks
docker run --rm -v ${PWD}:/src ikos \
    "ikos --analyses=boa,dbz /src/src/main.c"
```

### Memory Issues

Increase Docker memory limit in Docker Desktop settings, or:
```bash
docker run --rm -m 4g -v ${PWD}:/src ikos "ikos /src/main.c"
```

## References

- [IKOS GitHub](https://github.com/NASA-SW-VnV/ikos)
- [IKOS Analyzer Documentation](https://github.com/NASA-SW-VnV/ikos/blob/master/analyzer/README.md)
- [NASA Tutorial PDF](https://ntrs.nasa.gov/api/citations/20220007431/downloads/SWS_TC3_IKOS_Tutorial.pdf)
- [Abstract Interpretation Theory](https://en.wikipedia.org/wiki/Abstract_interpretation)
