#!/usr/bin/env python3
"""Generate per-subsystem autoconf.h headers from a Kconfig tree.

Every CONFIG_* symbol is written into the autoconf.h of the directory
containing the Kconfig file that declared it, so each subsystem only
sees its own configuration instead of one flat global header.
"""
import argparse
import os

from kconfiglib import BOOL, HEX, INT, STRING, TRISTATE, Kconfig

HEADER_TEMPLATE = """\
/* Auto-generated from Kconfig by {script} - do not edit. */
#ifndef {guard}
#define {guard}

{defines}

#endif /* {guard} */
"""


def header_name_for(rel_dir):
    """Turn 'src/kernel/fs' into 'kernel_fs_autoconf.h'.

    Every subsystem's header gets a unique, subsystem-qualified name (rather
    than a bare 'autoconf.h') so that a target linking several subsystems
    never has two same-named-but-different headers shadowing each other on
    its include path.
    """
    parts = [p for p in rel_dir.split("/") if p not in ("", ".", "src")]
    return "_".join(parts + ["autoconf.h"]) if parts else "autoconf.h"


def guard_for(rel_dir):
    if rel_dir in ("", "."):
        return "AUTOCONF_H"
    safe = rel_dir.upper().replace("/", "_").replace("-", "_")
    return f"AUTOCONF_{safe}_H"


def define_for(sym):
    name = f"CONFIG_{sym.name}"
    if sym.type in (BOOL, TRISTATE):
        return f"#define {name} 1"
    if sym.type == STRING:
        return f'#define {name} "{sym.str_value}"'
    if sym.type in (INT, HEX):
        return f"#define {name} {sym.str_value}"
    return f"#define {name} {sym.str_value}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kconfig", required=True, help="Path to the root Kconfig file")
    parser.add_argument("--dotconfig", required=True, help="Path to .config (may not exist)")
    parser.add_argument("--defconfig", required=True,
                         help="Path to the committed default config, used when .config is absent")
    parser.add_argument("--src-root", required=True, help="Repo root used to compute subsystem dirs")
    parser.add_argument("--out-dir", required=True, help="Directory to write generated headers under")
    args = parser.parse_args()

    # kconfiglib resolves relative `source` paths against $srctree.
    os.environ["srctree"] = os.path.abspath(args.src_root)

    kconf = Kconfig(args.kconfig)
    if os.path.exists(args.dotconfig):
        kconf.load_config(args.dotconfig)
    elif os.path.exists(args.defconfig):
        kconf.load_config(args.defconfig)

    src_root = os.path.abspath(args.src_root)
    by_dir = {}

    for sym in kconf.unique_defined_syms:
        if not sym.nodes or sym.str_value in ("n", ""):
            continue

        decl_file = os.path.abspath(sym.nodes[0].filename)
        rel_dir = os.path.relpath(os.path.dirname(decl_file), src_root)
        by_dir.setdefault(rel_dir, []).append(sym)

    os.makedirs(args.out_dir, exist_ok=True)
    for rel_dir, syms in by_dir.items():
        defines = "\n".join(
            define_for(sym) for sym in sorted(syms, key=lambda s: s.name)
        )
        header_path = os.path.join(args.out_dir, header_name_for(rel_dir))
        with open(header_path, "w") as f:
            f.write(HEADER_TEMPLATE.format(
                script=os.path.basename(__file__),
                guard=guard_for(rel_dir),
                defines=defines,
            ))


if __name__ == "__main__":
    main()
