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

    STREAM = 1 << 4   # stream

    CONTENT_TYPE_TEXT_PLAIN = 1 << 6
    CONTENT_TYPE_TEXT_HTML = 1 << 7
    CONTENT_TYPE_APPLICATION_JSON = 1 << 8
    CONTENT_TYPE_MULTIPART_FORM_DATA = 1 << 9
    CONTENT_TYPE_APPLICATION_OCTET_STREAM = 1 << 10

    ARG_INT = 1 << 11     # is int argument
    ARG_STR = 1 << 12     # is str argument
    ARG_HEX = 1 << 13     # is hex argument
    ARG_UINT = 1 << 14     # is unsigned int argument

    LEAF = 1 << 31  # is leaf

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
        "d": Flag.ARG_INT,
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

    condition: List[str] = field(default_factory=list)

    @staticmethod
    def parse_descr(line: str) -> RouteRepr:
        """
        Parse a route description line, as follows:
            GET /demo/json -> rest_demo_json
            POST /dfu -> http_dfu_image_upload, http_dfu_image_upload_response (CONFIG_DFU)
            GET /dfu -> http_dfu_status (CONFIG_DFU)
        """
        rec = re.compile(r"^(?P<method>[a-zA-Z]+)\s"
                         r"/(?P<path>[a-zA-Z0-9_/:.]*)\s->\s"
                         r"(?P<req_handler>[a-zA-Z0-9_]+)\s?"
                         r"(,\s(?P<resp_handler>[a-zA-Z0-9_]+)\s?)?"
                         r"(\s\((?P<condition>.*)\))?$")

        if line != "":
            m = rec.match(line)

            if m:
                path = m.group("path")
                path = path.strip("/")

                return RouteRepr(
                    method=Method[m.group("method").upper()],
                    path=path,
                    req_handler=m.group("req_handler"),
                    resp_handler=m.group("resp_handler") or "",
                    condition=m.group("condition").split(
                    ) if m.group("condition") else []
                )
            else:
                l.warning(f"Failed to parse route description: {line}")

    def __repr__(self):
        ret = f"{self.method.name} {self.path} -> {self.req_handler}"
        if self.resp_handler:
            ret += f", {self.resp_handler}"
        if self.condition:
            ret += f" ({' '.join(self.condition)})"
        return ret


class Tree:
    @dataclass
    class Part(ABC):
        name: str
        flags: Flag

        parent: Tree.Section

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

        def toc(self) -> str:
            c = f"LEAF(\"{self.name}\", {self.flags}, "

            resph = self.resph if self.resph else "NULL"

            c += f"{self.reqh}, {resph})"

            return c

        def __repr__(self) -> str:
            return f"[L] {self.flags} {self.name} -> {self.reqh}, {self.resph}"

    @dataclass
    class Section(Part):
        children: List[Tree.Part] = field(default_factory=list)

        index: int = -1
        is_root: bool = False

        def __repr__(self) -> str:
            return f"{self.flags} {self.name} :"

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
            return f"SECTION(\"{self.name}\", {self.flags}, " \
                f"{self._to_c_array_name()}, \n\t\t"\
                f"ARRAY_SIZE({self._to_c_array_name()}))"

        def toc_array(self) -> str:
            c = f"static const struct route_descr {self._to_c_array_name()}[] = {{\n\t"

            c += f",\n\t".join([child.toc() for child in self.children])

            c += "\n};\n"

            return c

    def __init__(self) -> None:
        self.root = Tree.Section("", Flag(0), None)
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
                            parent=section,
                            resph=route.resp_handler,
                            reqh=route.req_handler
                        )
                    )
            else:
                existing_section = section.find_section(part_name)
                if not existing_section:

                    new_section = Tree.Section(
                        name=part_name,
                        flags=part_name_to_arg_flags(part_name),
                        parent=section
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

        # c = "\n" + self.generate_c_sections(sections) + "\n\n"

        c = "\n".join(reversed(arrays)) + "\n"

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
    routes = parse_routes_repr_file("samples/parser/route.c")

    tree = build_routes_tree(routes)

    # tree.show()

    c_str = tree.generate_c()

    # print(tree.generate_c_handlers(False))

    generate_routes_def_file("samples/parser/route.c", c_str)
