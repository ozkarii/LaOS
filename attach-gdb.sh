GDB="gdb"

if command -v gdb-multiarch > /dev/null 2>&1
then
	GDB="gdb-multiarch"
fi

$GDB --ex "set arch aarch64" --ex "symbol build/kernel/kernel.elf" --ex "target remote localhost:1234" --ex "tui layout split" --ex "tui focus cmd"
