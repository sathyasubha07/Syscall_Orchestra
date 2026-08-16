# Syscall Orchestra Architecture

This document describes the design and implementation details of **Syscall Orchestra**, a real-time Linux tracer that translates running program actions into electronic music.

## Core Architecture Overview

The system runs as a multi-threaded C application. Tracing and audio generation are separated into two distinct pipelines connected by a thread-safe lock-free bounded queue:

```
+----------------------------------------------------------------------------------+
| Main Thread                                                                      |
|  1. fork() -> child process                                                      |
|  2. Child: ptrace(PTRACE_TRACEME) + execve()                                      |
|  3. Parent: waitpid() loop + ptrace(PTRACE_SYSCALL)                              |
|  4. Registers read (PTRACE_GETREGS) -> Syscall Number, Arguments, Return Value   |
|  5. Option: Instruction pointer inspection via ptrace(PTRACE_PEEKTEXT)           |
|  6. Decode & format syscall properties                                           |
|  7. Enqueue event to Sound Queue (drops if queue full to prevent tracing hang)   |
|  8. Update terminal UI and status dashboard                                      |
+----------------------------------------------------------------------------------+
                                         |
                                         v (Thread-safe Queue)
+----------------------------------------------------------------------------------+
| Audio Synthesis Thread                                                           |
|  1. pop() event from queue (blocking wait when empty)                            |
|  2. Apply rate-limiting / throttling (minimum inter-note interval)               |
|  3. Synthesize pure 16-bit PCM waveform in memory (sine, chords, sweeps, blips)  |
|  4. Write to ALSA soundcard or pipe to aplay for audio playback                  |
|  5. Fallback silently if audio hardware or utilities are absent                  |
+----------------------------------------------------------------------------------+
```

---

## 1. Process Launching and Tracing State Machine

1. **Child Launching**: The tracer calls `fork()`. The child immediately invokes `ptrace(PTRACE_TRACEME, 0, NULL, NULL)` and halts itself by executing `execve()`. This yields control to the parent before the first instruction runs.
2. **Tracers Options**: The parent sets `PTRACE_SETOPTIONS` with `PTRACE_O_TRACESYSGOOD`. This causes stops to return `(SIGTRAP | 0x80)` instead of a standard `SIGTRAP` signal, allowing clear separation between syscall stops and normal program exceptions.
3. **Loop Continuation**: The tracer continues the child using `ptrace(PTRACE_SYSCALL, child_pid, 0, 0)`. The child runs until the next syscall entry or exit point and halts.
4. **State Machine**: The tracer tracks entry vs exit state using a simple boolean flag.
   - **Entry Stop**: Registers are queried to extract `orig_rax` (the syscall number) and the 6 input registers (`rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9`). The syscall name is looked up, parameters are formatted, and the event is queued for sound generation.
   - **Exit Stop**: Registers are queried to extract `rax` (the syscall return value). The formatted exit summary is printed if verbose mode is enabled.

---

## 2. Register Reading on x86-64 Linux

Syscall parameters are extracted using `ptrace(PTRACE_GETREGS, target_pid, 0, &regs)` using the structure `struct user_regs_struct`:

- **Syscall Number**: `regs.orig_rax`
- **Argument 1**: `regs.rdi`
- **Argument 2**: `regs.rsi`
- **Argument 3**: `regs.rdx`
- **Argument 4**: `regs.r10`
- **Argument 5**: `regs.r8`
- **Argument 6**: `regs.r9`
- **Return Value**: `regs.rax`
- **Instruction Pointer**: `regs.rip`

---

## 3. PTRACE_PEEKTEXT Debugging

When run with the `--debug` flag, the tracer executes `ptrace(PTRACE_PEEKTEXT, target_pid, (void*)regs.rip, NULL)` at every syscall stop to read the 64-bit instruction word currently pointed to by the target's instruction pointer (`rip`).
- Failure of this call is handled gracefully, ensuring the tracer continues trace execution normally without interrupting the child or exiting.

---

## 4. Thread-Safe Sound Queue & Throttling

To prevent tracer execution hangs, the tracing loop and audio engine are decoupled:
- **Event Queue**: Uses a bounded ring buffer queue of capacity 256. Access is protected by a mutex and condition variables.
- **Throttling**: If a target program generates thousands of syscalls in milliseconds, the queue fills. Rather than blocking the tracer, `event_queue_push` drops the sound event, increments the `dropped` statistic, and returns. This caps the memory and ensures tracing is always fast and non-blocking.
- **Playback Rate Limiter**: The audio worker thread enforces a minimum playback interval (e.g. 25ms) between notes, preventing audio distortion or sound card buffer overflows.

---

## 5. Audio Synthesis Engine (Planned — Review 2)

The design target is 16-bit Mono PCM synthesis at 44.1 kHz, with procedurally generated
sine/square/triangle waveforms shaped by attack-decay envelopes, output via ALSA with an
`aplay` pipe fallback and a silent mock mode if no audio device is available.

In the current build, `sound_engine.c` implements only the consumer-thread scaffolding
(it drains the event queue so tracing is never blocked) — the actual waveform synthesis
and audio device output are not implemented yet.
