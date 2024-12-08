import sys
from argparse import ArgumentParser
from contextlib import contextmanager

from ._parser import parse_line, MessageDef

def _parse_file(file: str) -> list[MessageDef]:
    ...


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
    parser.add_argument("--format", type=str, default="python", choices=("python", "raw"))
    parser.add_argument("--out", type=str, default=None)
    parser.add_argument("--append", action="store_true", default=False)

    tl_defs = {}

    args = parser.parse_args()
    if args.line:
        for line in args.line:
            tl_def = parse_line(line)
            tl_defs[tl_def.id] = tl_def
    elif args.file:
        ... # TODO: parse files (remove duplicates and append layer number as suffix based)

    with _open(args.out, "a" if args.append else "w", encoding="utf8") as f:
        if not args.append:
            f.write("import pyutl\n\n")

        for tl_def in tl_defs.values():
            if args.format == "raw":
                name = tl_def.full_name("_")
                f.write(f"{name} = pyutl.parse_tl(\"{tl_def.to_tl()}\", {tl_def.layer}, pyutl.TLSection.TYPES)  # TODO: write actual section\n")
            else:
                f.write(tl_def.to_python_class())


if __name__ == "__main__":
    main()