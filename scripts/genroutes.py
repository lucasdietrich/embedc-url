#
# Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

from __future__ import annotations

from typing import Dict, Tuple, Iterable, List, Union, Optional
from dataclasses import dataclass, field
from pprint import pprint
from enum import IntEnum, IntFlag
from abc import ABC, abstractmethod
import re
import argparse

import logging
l = logging.getLogger("genroutes")

ROUTES_DESCR_BEGIN_BOUNDARY = "/* ROUTES BEGIN"
ROUTES_DESCR_END_BOUNDARY = "ROUTES END */"

ROUTES_DEF_BEGIN_BOUNDARY = "/* ROUTES DEF BEGIN */"
ROUTES_DEF_END_BOUNDARY = "/* ROUTES DEF END */"


class Method(IntFlag):
    GET = 1 << 0
    POST = 1 << 1
    PUT = 1 << 2
    DELETE = 1 << 3


# dynamically generate the variable
METHOD_ALL = sum(m for m in Method)
class Flag(IntFlag):
    GET = 1 << 0      # GET
    POST = 1 << 1     # POST
    PUT = 1 << 2      # PUT
    DELETE = 1 << 3   # DELETE

    ARG_UINT = 1 << 4     # is unsigned int argument
    ARG_STR = 1 << 5     # is str argument
    ARG_HEX = 1 << 6     # is hex argument

    LEAF = 1 << 7  # is leaf

    def __str__(self) -> str:
        hidden_flags = [Flag.LEAF]

        string = " | ".join([
            f"{f.name}" for f in Flag if f in self and f not in hidden_flags
        ])

        if string == "":
            string = "0u"

        return string


def part_name_to_arg_flags(part: str) -> Flag:
    arg_descr_table = {
        "x": Flag.ARG_HEX,
        "s": Flag.ARG_STR,
        "u": Flag.ARG_UINT,
    }
    if is_arg_descr(part):
        return arg_descr_table.get(part[1:], Flag(0))
    else:
        return Flag(0)


def is_arg_descr(part: str) -> bool:
    return re.match(r"\:[a-zA-Z0-9_]+", part) is not None


@dataclass
class RouteRepr:
    method: Method
    path: str

    req_handler: str
    resp_handler: str

    conditions: set[str] = field(default_factory=set)
    user_data: str = "0u"

    def unconditional(self) -> bool:
        return len(self.conditions) == 0

    @staticmethod
    def parse_descr(line: str) -> RouteRepr:
        """
        Parse a route description line, as follows:
            GET /demo/json -> rest_demo_json
            POST /dfu -> http_dfu_image_upload, http_dfu_image_upload_response (CONFIG_DFU) | 0x70u
            GET /dfu -> http_dfu_status (CONFIG_DFU) | REST
        """
        rec = re.compile(
            r"^(?P<method>[a-zA-Z]+)\s"
            r"/(?P<path>[a-zA-Z0-9_/:.]*)\s->\s"
            r"(?P<req_handler>[a-zA-Z0-9_]+)\s?"
            r"(,\s(?P<resp_handler>[a-zA-Z0-9_]+)\s?)?"
            r"(\s\((?P<conditions>([A-Z_]+)(\s[A-Z_]+)*)\))?"
            r"(\s*\|\s*(?P<user_data>([0-9]+u)|([A-Za-z0-9_]+)|(0x[0-9A-Fa-f]+u)?))?$"
        )

        if line != "":
            m = rec.match(line)

            if m:
                path = m.group("path")
                path = path.strip("/")

                conditions = set()
                if m.group("conditions"):
                    for c in m.group("conditions").split(" "):
                        if c != "":
                            conditions.add(c)

                user_data = m.group("user_data")
                if not user_data:
                    user_data = "0u"

                return RouteRepr(
                    method=Method[m.group("method").upper()],
                    path=path,
                    req_handler=m.group("req_handler"),
                    resp_handler=m.group("resp_handler") or "",
                    conditions=conditions,
                    user_data=user_data,
                )
            else:
                l.warning(f"Failed to parse route description: {line}")

    def __repr__(self):
        ret = f"{self.method.name} {self.path} -> {self.req_handler}"
        if self.resp_handler:
            ret += f", {self.resp_handler}"
        if self.conditions:
            ret += f" ({' '.join(self.conditions)})"
        return ret


class Tree:
    @dataclass
    class Part(ABC):
        name: str
        flags: Flag
        user_data: str

        parent: Tree.Section
        conditions: set[str]

        def make_unconditional(self):
            self.conditions = set()
            
        def unconditional(self) -> bool:
            return len(self.conditions) == 0

        def get_conds_ifdef_clause(self, force: bool = False):
            if not force and self.parent is not None and self.parent.conditions == self.conditions:
                return ""

            if not self.unconditional():
                return "#if " + " && ".join(
                    [f"defined({cond})" for cond in self.conditions]
                ) + "\n"
            else:
                return ""
        
        def get_conds_endif_clause(self, force: bool = False):
            if not force and self.parent is not None and self.parent.conditions == self.conditions:
                return ""
            
            if not self.unconditional():
                return "\n#endif"
            else:
                return ""

        @abstractmethod
        def toc(self) -> str:
            pass

        def _get_parent_addr(self):
            if self.parent.is_root:
                return "NULL"
            else:
                return f"&{self.parent.parent._to_c_array_name()}[{self.parent.index}]"

    @dataclass
    class Leaf(Part):
        resph: str
        reqh: str

        def unconditional(self) -> bool:
            return len(self.conditions) == 0

        def toc(self) -> str:
            c = ""
            
            c += self.get_conds_ifdef_clause()

            c += f"\tLEAF(\"{self.name}\", {self.flags}, "

            resph = self.resph if self.resph else "NULL"

            c += f"{self.reqh}, {resph}, {self.user_data}),"

            c += self.get_conds_endif_clause()

            return c

        def __repr__(self) -> str:
            return f"[L] {self.flags} {self.name} -> {self.reqh}, {self.resph}" + " ! " + ", ".join(self.conditions)

    @dataclass
    class Section(Part):
        children: List[Tree.Part] = field(default_factory=list)

        index: int = -1
        is_root: bool = False

        def add_condition(self, cond: str):
            if not self.unconditional():
                self.conditions.add(cond)

        def add_conditions(self, conds: set[str]):
            if not self.unconditional():
                self.conditions.update(conds)

        def __repr__(self) -> str:
            return f"{self.flags} {self.name} :" + " ! " + ", ".join(self.conditions)

        def add_part(self, part: Tree.Part) -> Tree.Part:
            self.children.append(part)

        def find_leaf(self, name: str, method: Method) -> Optional[Tree.Leaf]:
            for child in self.children:
                if isinstance(child, Tree.Leaf) and child.name == name and child.flags & method:
                    return child

        def find_leafs(self, name: str, method: Method) -> Iterable[Tree.Leaf]:
            for child in self.children:
                if isinstance(child, Tree.Leaf) and child.name == name and child.flags & method:
                    yield child

        def find_section(self, name: str) -> Optional[Tree.Section]:
            for child in self.children:
                if isinstance(child, Tree.Section) and child.name == name:
                    return child

        def _to_c_array_name(self) -> str:
            if self.parent:
                name = self.parent._to_c_array_name() + "_"
            else:
                name = "root"

            name += self.name.replace(":", "z")

            return name

        def toc(self) -> str:
            c = ""

            c += self.get_conds_ifdef_clause()
                
            c += f"\tSECTION(\"{self.name}\", {self.flags}, " \
                f"{self._to_c_array_name()}, \n\t\t"\
                f"ARRAY_SIZE({self._to_c_array_name()}), {self.user_data}),"

            c += self.get_conds_endif_clause()

            return c

        def toc_array(self) -> str:
            c = ""
            c += self.get_conds_ifdef_clause(True)
            c += f"static const struct route_descr {self._to_c_array_name()}[] = {{\n"

            c += f"\n".join([child.toc() for child in self.children])
            c += "\n};"
            c += self.get_conds_endif_clause(True)

            c += "\n"

            return c

    def __init__(self) -> None:
        self.root = Tree.Section("", Flag(0), "", None, set())
        self.root.is_root = True

        self.handlers = list()

    def add_route(self, route: RouteRepr):
        parts = route.path.split("/")
        section = self.root
        n = len(parts)

        for i, part_name in enumerate(parts):
            LEAF = i == n - 1

            if LEAF:
                leaf = section.find_leaf(part_name, route.method)
                if leaf:
                    l.warning(f"Duplicate route: {route}")
                    return
                else:
                    if route.resp_handler not in self.handlers:
                        self.handlers.append(route.resp_handler)

                    if route.req_handler != "" and route.req_handler not in self.handlers:
                        self.handlers.append(route.req_handler)

                    # If we have an already created section "/devices/" containing
                    # "caniot" and "xiaomi". And we want to add a new leaf "/devices" to
                    # the tree. In this case we need to add the leaf to the already
                    # existing section "/devices/" with part name "" instead of
                    # creating a new leaf with part name "devices" at the root.
                    group_section = section.find_section(part_name)
                    if group_section is not None:
                        part_name = ""
                        section = group_section

                    section.add_part(
                        Tree.Leaf(
                            name=part_name,
                            flags=route.method | Flag.LEAF | part_name_to_arg_flags(
                                part_name),
                            user_data=route.user_data,
                            parent=section,
                            conditions=set(route.conditions),
                            resph=route.resp_handler,
                            reqh=route.req_handler,
                        )
                    )
            else:
                existing_section = section.find_section(part_name)

                if not existing_section:
                    new_section = Tree.Section(
                        name=part_name,
                        flags=part_name_to_arg_flags(part_name),
                        user_data=route.user_data,
                        parent=section,
                        conditions=set(route.conditions)
                    )

                    # If leafs with same part name exists, we need to move them
                    # into this new created section.
                    leafs_with_same_name = list(
                        section.find_leafs(part_name, METHOD_ALL))
                    for leaf in leafs_with_same_name:
                        leaf.name = ""
                        leaf.parent.children.remove(leaf)
                        new_section.add_part(leaf)

                    section.add_part(new_section)
                    section = new_section
                else:
                    if route.unconditional():
                        existing_section.make_unconditional()
                    else:
                        existing_section.add_conditions(route.conditions)
                    section = existing_section

    def show(self):
        def _show(section: Tree.Section, indent: int):
            for child in section.children:
                print(" -- " * indent, child)
                if isinstance(child, Tree.Section):
                    _show(child, indent + 1)

        _show(self.root, 0)

    def generate_c(self) -> str:
        def _generate_c(part: Tree.Section, arrays: List, sections: List[Tree.Section]):
            array = part.toc_array()
            arrays.append(array)

            for index, child in enumerate(part.children):
                if isinstance(child, Tree.Section):
                    # set index
                    child.index = index
                    sections.append(child)

                    _generate_c(child, arrays, sections)

        arrays = []
        sections = []
        _generate_c(self.root, arrays, sections)

        c = "\n" 
        
        # c += self.generate_c_sections(sections) + "\n\n"

        c += "\n".join(reversed(arrays)) + "\n"

        return c

    def generate_c_handlers(self, extern: bool = True):
        if extern:
            c = "\n".join([f"extern void {h}(void);" for h in self.handlers])
        else:
            c = "\n".join([f"void {h}(void)" + " { }" for h in self.handlers])

        return c

    def generate_c_sections(self, sections: Iterable[Tree.Section]):
        return "\n".join([f"static const struct route_descr {s._to_c_array_name()}[];" for s in sections])


def parse_routes_repr_file(file: str,
                           rbegin: str = ROUTES_DESCR_BEGIN_BOUNDARY,
                           rend: str = ROUTES_DESCR_END_BOUNDARY) -> Iterable[RouteRepr]:
    state = 0
    with open(file, "r") as f:
        for line in f.readlines():
            line = line.strip()

            if state == 0:
                if line == rbegin:
                    state = 1
            elif state == 1:
                if line == rend:
                    break
                else:
                    route = RouteRepr.parse_descr(line)
                    if route:
                        yield route
            else:
                raise RuntimeError("Invalid state")


def generate_routes_def_file(file: str,
                             c_str: str,
                             rbegin: str = ROUTES_DEF_BEGIN_BOUNDARY,
                             rend: str = ROUTES_DEF_END_BOUNDARY) -> None:
    with open(file, "r+") as f:
        content = f.read()

    begin = content.find(rbegin)
    end = content.find(rend)

    if begin == -1 or end == -1:
        raise RuntimeError("Invalid file format")

    # replace code between boundaries with content of c_str
    content = content[:begin + len(rbegin)] + c_str + content[end:]

    with open(file, "w") as f:
        f.write(content)


def build_routes_tree(routes: Iterable[RouteRepr]) -> Tree:
    tree = Tree()

    for route in routes:
        tree.add_route(route)

    return tree

if __name__ == "__main__":
    # PARSE ARGUMENTS
    p = argparse.ArgumentParser(description='List the content of a folder')

    p.add_argument('input',
                   metavar='input',
                   type=str,
                   help='input file where routes are described')
    p.add_argument('--output',
                   metavar='output',
                   type=str,
                   required=False,
                   help='output file where routes definition will be generated')

    p.add_argument('--descr-begin',
                   metavar='descr_begin',
                   type=str,
                   required=False,
                   default=ROUTES_DESCR_BEGIN_BOUNDARY,
                   help='output file where routes definition will be generated')
    p.add_argument('--descr-end',
                   metavar='descr_end',
                   type=str,
                   required=False,
                   default=ROUTES_DESCR_END_BOUNDARY,
                   help='output file where routes definition will be generated')
    p.add_argument('--def-begin',
                   metavar='def_begin',
                   type=str,
                   required=False,
                   default=ROUTES_DEF_BEGIN_BOUNDARY,
                   help='output file where routes definition will be generated')
    p.add_argument('--def-end',
                   metavar='def_end',
                   type=str,
                   required=False,
                   default=ROUTES_DEF_END_BOUNDARY,
                   help='output file where routes definition will be generated')

    p.add_argument('-v', '--verbose',
                   action='store_true')
    p.add_argument('-gh', '--handlers',
                   action='store_true')

    # Execute the parse_args() method
    args = p.parse_args()

    if args.output is None:
        args.output = args.input

    # PROCESS
    routes = parse_routes_repr_file(args.input, args.descr_begin, args.descr_end)
    tree = build_routes_tree(routes)
    c_str = tree.generate_c()
    generate_routes_def_file(args.output, c_str, args.def_begin, args.def_end)

    if args.verbose:
        pprint(tree.show())

    if args.handlers:
        print(tree.generate_c_handlers(False))

    # print(args)