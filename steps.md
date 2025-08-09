## 1. Build minimal bootable image
Using aarch64-none-elf toolchain
## 2. Get UART TX working, print hello world
**Problem:** Prints garbage and in the wrong order, looks like a race condition.

**Cause:** GDB shows 4 threads, 1 for each CPU. All of them are running my code at the same time.

**Solution:** Read mpidr_el1 and print hello world only when it is equal to zero.

## 3. Write a simple UART driver for later use
- TODO: how to get chars

## 4. Decide what to do with multiple CPUs
- For now, put them in _Infifnite_loop in start.s
