# Tilck (Tiny Linux-Compatible Kernel) 

## Building

### Building With CMake and Zig 

```bash
cmake -B build -G Ninja -DCC="zig cc" -DCXX="zig c++" -DUSE_SYSCC=1 -DCMAKE_BUILD_TYPE=Release -DARCH_GTESTS=true
```

