# Syscall Orchestra — Turning a Running Program into Music

A real-time Linux operating system project that traces target execution and decodes its system calls via `ptrace`. The long-term goal is to transform that trace into musical notes in real time; this is a work in progress.

---

## Current Status (Review 1)

**Working now:**
- Process launch via `fork()` + `ptrace(PTRACE_TRACEME)` + `execve()`
- Full `ptrace(PTRACE_SYSCALL)` entry/exit tracing loop with `waitpid()` state machine
- Register decoding via `PTRACE_GETREGS` (syscall number, 6 arguments, return value)
- Syscall-to-name/category lookup table and argument formatting (`decoder.c`)
- Thread-safe bounded event queue with drop-on-full throttling (`event_queue.c`)
- Live terminal display + final statistics summary (`display.c`)
- Graceful shutdown on `SIGINT`/`SIGTERM` (`signal_handler.c`)
- Two working demo targets: `file_activity`, `process_activity`
- `--no-audio`, `--verbose`, `--debug` (PTRACE_PEEKTEXT) flags

**Not implemented yet (planned for Review 2):**
- Real PCM waveform synthesis and ALSA/`aplay` audio output. `sound_engine.c` currently
  ships as a stub that drains the event queue on a worker thread but does not produce
  sound — see the comment block at the top of that file.
- `network_activity` / `mixed_activity` demo targets (drafted separately, not wired
  into the default build yet — see `targets_phase2/`)
- `--quiet` / `--stats`-only output modes

---

## How It Works

1. **Process Launching**: The main binary forks a child process. The child sets up `ptrace(PTRACE_TRACEME)` and executes the target program via `execve()`.
2. **Ptrace State Machine**: The parent catches the child at every system call entry and exit using `ptrace(PTRACE_SYSCALL)`.
3. **Register Retrieval**: Using `ptrace(PTRACE_GETREGS)`, the x86-64 CPU registers are read to obtain the syscall number (`orig_rax`), arguments, and return values (`rax`).
4. **Syscall Decoding**: The syscall number is mapped against an x86-64 lookup table. Call arguments are parsed and converted into readable signatures.
5. **Real-time Audio Engine**: Events are dispatched to a thread-safe lock-free queue. A background audio worker thread pops events, limits sound rates (to prevent trace slowdown), and plays procedural synthesized 16-bit PCM waveforms through the soundcard.

---

## Architectural Layout

```
Target Program -> fork() & execve()
                      ↓
         ptrace(PTRACE_SYSCALL) loop
                      ↓
             waitpid() states
                      ↓
        ptrace(PTRACE_GETREGS) registry
                      ↓
               Syscall Decoder
                      ↓
             Event Queue (Thread-Safe)
                      ↓
             Sound Engine (Synth)
                      ↓
         Audio Device (ALSA) / fallback aplay
```

---

## Requirements

- **Operating System**: Linux (Ubuntu 20.04/22.04+ or compatible distribution).
- **Architecture**: x86-64 (register names and decoding are optimized for x86_64).
- **Libraries**: `libasound2-dev` (ALSA developer files) and `alsa-utils` (for fallback audio utility `aplay`).

---

## Installation & Setup

Install compiler, build tools, and ALSA audio libraries:

### On Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential libasound2-dev alsa-utils pkg-config
```

Alternatively, run our automated environment setup script:
```bash
sudo ./scripts/setup.sh
```

---

## Build Instructions

Compile the main executable and all target binaries:

```bash
# Build everything
make

# Build only main tracer binary
make syscall_orchestra

# Build only target programs
make targets

# Clean all build objects
make clean
```

---

## Run & Demo

### 1. File Activity Demo
Runs a target performing file open, read, write, seek, and close operations:
```bash
./syscall_orchestra ./targets/file_activity
```

### 2. Process Activity Demo
Runs a target creating child processes and running `/bin/true`:
```bash
./syscall_orchestra ./targets/process_activity
```

### 3. Advanced Flags
- `--no-audio`: Skips the (currently stub) sound engine entirely.
- `--verbose`: Shows entry and exit details for every system call.
- `--debug`: Peeks at instructions using `PTRACE_PEEKTEXT` at each stop.

> `mixed_activity` and `network_activity` targets exist as source under
> `targets_phase2/` but aren't part of the current build — planned for the next milestone.

---

## Troubleshooting

### Ptrace permission issues
If you receive permission denied or tracing errors, Yama security settings on your Linux kernel might be restricting ptrace:
```bash
# Temporarily allow tracing of non-child processes
sudo sysctl -w kernel.yama.ptrace_scope=1
```
Or run the program with `sudo`:
```bash
sudo ./syscall_orchestra ./targets/file_activity
```

### WSL (Windows Subsystem for Linux) considerations
- Standard WSL1 does not support complete `ptrace` system call registers or the state machine. Use **WSL2** (which runs a full Linux kernel).
- Audio devices under WSL2 may require installing pulse/system audio configurations (`wslg` or `pulseaudio`). If audio output fails to initialize, the app automatically falls back to silent tracing, preventing crashes.

### Audio is stuttering or delayed
Syscall Orchestra has built-in queue throttling and rate limiting. If the target runs thousands of calls rapidly, some audio notes are dropped (`Dropped` in final statistics) to preserve real-time visual output and prevent stuttering. Use `--no-audio` to test performance at pure speed.
