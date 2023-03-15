#!/usr/bin/env python3

import pcpp
from io import StringIO, IOBase
import argparse
import sys
from typing import NewType, Optional, TypeVar
from pathlib import Path

CMacros = NewType('CMacros', list[str])
PathLike = TypeVar("PathLike", Path, str)

parser = argparse.ArgumentParser()

pp = pcpp.Preprocessor()

trans = "".maketrans({"\r":"", "\n": "", "\"":"", "\'":""})



def extract_c_defines(pp_text : str, defines : Optional[CMacros] = None) -> dict:
    sio = StringIO()

    pp.parse(pp_text, "dummy.c")
    pp.write(sio)

    if defines is None:
        defines = pp.macros.keys()

    return {k: pp.macros[k].value[0].value.translate(trans) for k in defines}

def get_c_defines(defines : CMacros, input : PathLike | IOBase) -> dict:
    name = None
    if isinstance(input, str):
        input = Path(input)

    if isinstance(input, Path):
        name = input.absolute().as_uri()
        input = input.open("r")
    else:
        raise IOError()

    if isinstance(input, IOBase):
        if name is None:
            if hasattr(input, "name"):
                name = input.name
            else:
                name = str(id(name))
        stream = input
    else:
        raise IOError()
    
    txt = stream.read()
    stream.close()
    return {name: extract_c_defines(txt, defines)}

def process(headers: list) -> dict:
    defines = {}
    for header in headers:
        try:
            res = get_c_defines(args.defines, header)
        except IOError:
            print(f"I don't know how to handle {header}({type(header)})")
            if not Path(header).exists():
                print(f"but the path {header} does not exist")
        else:
            defines.update(res)
    return defines

def get_index(defines: dict) -> dict:
    index = {}
    for file, data in defines.items():
        for k in data.keys():
            if k not in index:
                index[k] = set((file,))
            else:
                index[k].add(file)
    return index

def flatten(defines: dict) -> dict:
    index = {}
    for file, data in defines.items():
        for define, value in data.items():
            index[define] = value
    return index

if __name__ == "__main__":
    parser.add_argument("-d", dest="defines", type=str, action="append", nargs="?", const=str,
                        help = \
"""One or more -d <define> to filter on the defines you are\n
searching for. An absense of -d returns all defines""")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("-i", action="store_true", dest="get_index", default=False,
                        help = \
"""Normal return dict like:\n
     dict({file<f>: {define<d>: value<v>,...},...})\n
  This switch returns a define/file index like:\n
     dict({define : [file1,file2,...]})
     """)
    group.add_argument("-f", action="store_true", dest="flat", default=False,
                       help = \
"""This switch returns a flat index,\n
     Note: multiple definitions will be lost\n
    {define<d>: value<v>,....}
""")
    parser.add_argument("header", nargs="*")
    args = parser.parse_args()

    if len(args.header) == 0:
        headers = sys.stdin
    elif isinstance(args.header, str):
        headers = [args.header]
    else:
        headers = args.header

    defines= process(headers)
    if not args.get_index and not args.flat:
        print(defines)
    if args.get_index:
        print(get_index(defines))
    if args.flat:
        index = flatten(defines)
        for define, value in index.items():
            if ("\t" in value or " " in value) and "{" not in value:
                quote="\""
            else:
                quote=""
            print(f"{define}\t{quote}{value}{quote}")
