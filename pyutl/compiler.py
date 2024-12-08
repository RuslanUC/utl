import sys
from argparse import ArgumentParser
from contextlib import contextmanager

from ._parser import parse_line, MessageDef

def _parse_file(file: str, assume_layer: int = 0) -> list[MessageDef]:
    from pyutl import TLSection

    result = []
    with open(file, "r", encoding="utf8") as f:
        lines = f.read().split("\n")

    layer = assume_layer
    if not layer:
        # Reversed because usually layer is placed at the bottom
        for line in reversed(lines):
            if line.strip().startswith("//") and "LAYER" in line:
                layer = int(line.strip().split(" ")[-1])
                break

    section = TLSection.TYPES
    for line in lines:
        if line.startswith("//"):
            continue

        if line.strip() == "---types---":
            section = TLSection.TYPES
            continue
        elif line.strip() == "---functions---":
            section = TLSection.FUNCTIONS
            continue

        if message_def := parse_line(line, layer, section):
            result.append(message_def)

    return result


@contextmanager
def _open(filename: str | None = None, mode: str = "w", **kwargs):
    if filename is not None:
        fh = open(filename, mode, **kwargs)
    else:
        fh = sys.stdout

    try:
        yield fh
    finally:
        if fh is not sys.stdout:
            fh.close()


def main() -> None:
    parser = ArgumentParser()
    line_or_file_group = parser.add_mutually_exclusive_group(required=True)
    line_or_file_group.add_argument("--line", type=str, nargs="+")
    line_or_file_group.add_argument("--file", type=str, nargs="+")
    parser.add_argument("--base_layer", type=int, default=0)
    parser.add_argument("--format", type=str, default="python", choices=("python", "raw"))
    parser.add_argument("--out", type=str, default=None)
    parser.add_argument("--append", action="store_true", default=False)

    tl_defs = {}
    tl_names = {}

    args = parser.parse_args()
    if args.line:
        for line in args.line:
            tl_def = parse_line(line)
            tl_defs[tl_def.id] = tl_def
    elif args.file:
        base_layer_objects = []
        while args.file and not (base_layer_objects := _parse_file(args.file.pop(0), args.base_layer)):
            pass
        if not base_layer_objects:
            return

        for m_def in base_layer_objects:
            tl_defs[m_def.id] = m_def
            tl_names[m_def.full_name()] = m_def

        for file in args.file:
            for m_def in _parse_file(file):
                if m_def.id in tl_defs:
                    continue

                if m_def.full_name() in tl_names:
                    m_def.name += f"_{m_def.layer}"

                tl_defs[m_def.id] = m_def
                tl_names[m_def.full_name()] = m_def

    with _open(args.out, "a" if args.append else "w", encoding="utf8") as f:
        if not args.append:
            f.write("import pyutl\n\n")

        for tl_def in tl_defs.values():
            if args.format == "raw":
                name = tl_def.full_name("_")
                f.write(f"{name} = pyutl.parse_tl(\"{tl_def.to_tl()}\", {tl_def.layer}, pyutl.TLSection.TYPES)\n")
            else:
                f.write(tl_def.to_python_class())


if __name__ == "__main__":
    main()