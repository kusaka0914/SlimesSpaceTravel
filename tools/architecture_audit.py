#!/usr/bin/env python3
"""Inventory first-party files and report structural review candidates without declaring them defects."""

import argparse
import csv
import hashlib
import json
import re
import subprocess
from collections import defaultdict
from pathlib import Path

CPP_SUFFIXES = {".h", ".hpp", ".cpp", ".mm"}
TEXT_SUFFIXES = CPP_SUFFIXES | {".py", ".ps1", ".bat", ".txt", ".md", ".yaml", ".yml", ".json", ".vert", ".frag", ".geom"}
INCLUDE = re.compile(r'^\s*#include\s+"([^"]+)"', re.MULTILINE)
# Keep line positions while ignoring braces and identifiers inside comments and literals.
NON_CODE = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|R"(?P<delimiter>\w*)\([\s\S]*?\)(?P=delimiter)"|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
FUNCTION = re.compile(r'(?P<name>[\w:~]+)\s*\([^;{}]*\)\s*(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?\{')


def mask_literals(source):
    return NON_CODE.sub(lambda match: "".join("\n" if char == "\n" else " " for char in match.group()), source)


def game_accesses(source):
    code = mask_literals(source)
    # Include reference-based injection as well as pointers; Game-returning accessors are still heuristic.
    names = {"mGame", "game"} | set(re.findall(r'\bGame\s*[*&]\s*(\w+)', code))
    named_receiver = r'\b(?:' + '|'.join(re.escape(name) for name in sorted(names)) + r')\b'
    accessor_receiver = r'\b(?:\w+\s*(?:->|\.)\s*)?GetGame\s*\(\s*\)'
    calls = re.compile(r'(?P<receiver>' + named_receiver + '|' + accessor_receiver +
                       r')\s*(?:->|\.)\s*(?P<call>\w+)\s*\(')
    for match in calls.finditer(code):
        yield {"line": code.count("\n", 0, match.start()) + 1,
               "receiver": match["receiver"], "api": match["call"]}


def inheritance_declarations(source):
    code = mask_literals(source)
    declaration = re.compile(r'\b(?:class|struct)\s+(?P<name>\w+(?:::\w+)*)\s*(?:final\s*)?'
                             r':\s*(?!:)(?P<bases>[^\{;]+)\{')
    for match in declaration.finditer(code):
        yield {"line": code.count("\n", 0, match.start()) + 1,
               "class": match["name"], "bases": match["bases"].strip()}


def functions(source):
    code = mask_literals(source)
    for match in FUNCTION.finditer(code):
        if match["name"] in {"if", "for", "while", "switch", "catch"}:
            continue
        depth, maximum, end = 1, 1, match.end()
        while end < len(code) and depth:
            if code[end] == "{":
                depth += 1
                maximum = max(maximum, depth)
            elif code[end] == "}":
                depth -= 1
            end += 1
        if depth:
            continue
        first = source.count("\n", 0, match.start()) + 1
        last = source.count("\n", 0, end) + 1
        body = source[match.end():end - 1]
        # Exact token-equivalent bodies retain string literals; similar-looking algorithms are not merged.
        normalized = re.sub(r"\s+", " ", re.sub(r"//[^\n]*|/\*[\s\S]*?\*/", "", body)).strip()
        yield {"name": match["name"], "line": first, "lines": last - first + 1,
               "brace_depth": maximum, "body": normalized}


def header_cycles(edges):
    graph = defaultdict(set)
    for edge in edges:
        if Path(edge["from"]).suffix in {".h", ".hpp"}:
            graph[edge["from"]].add(edge["to"])
    visited, active, stack, cycles = set(), set(), [], set()

    def visit(node):
        if node in active:
            cycle = stack[stack.index(node):]
            first = min(range(len(cycle)), key=cycle.__getitem__)
            cycles.add(tuple(cycle[first:] + cycle[:first]))
            return
        if node in visited:
            return
        visited.add(node)
        active.add(node)
        stack.append(node)
        for target in sorted(graph.get(node, ())):
            visit(target)
        stack.pop()
        active.remove(node)

    for node in sorted(graph):
        visit(node)
    return sorted(cycles)


def write_csv(path, rows, columns):
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=columns)
        writer.writeheader()
        writer.writerows(rows)


def audit(root, output):
    output.mkdir(parents=True, exist_ok=True)
    tracked = subprocess.check_output(["git", "ls-files", "-z", "--cached", "--others", "--exclude-standard"], cwd=root).decode().split("\0")
    paths = sorted({path for path in tracked if path and not path.startswith(("src/thirdParty/", "review/")) and (root / path).is_file()})
    inventory, calls, inheritance, hotspots = [], [], [], []
    clones = defaultdict(list)
    edges = []
    for name in paths:
        path = root / name
        is_cpp = name.startswith(("src/", "tests/", "tools/")) and path.suffix in CPP_SUFFIXES
        is_text = path.suffix in TEXT_SUFFIXES or path.name == "CMakeLists.txt"
        data = path.read_bytes()
        source = data.decode("utf-8-sig", errors="replace") if is_text else ""
        inventory.append({"path": name, "kind": "cpp" if is_cpp else "text" if is_text else "asset",
                          "bytes": len(data), "lines": source.count("\n") if is_text else "",
                          "sha256": hashlib.sha256(data).hexdigest(),
                          "audit": "static_scan_complete" if is_cpp else "inventoried"})
        if not is_cpp:
            continue
        code = mask_literals(source)
        calls.extend({"path": name, **call} for call in game_accesses(source))
        inheritance.extend({"path": name, **declaration} for declaration in inheritance_declarations(source))
        for include in INCLUDE.findall(source):
            target = root / "src" / include
            if target.is_file():
                edges.append({"from": name, "to": target.relative_to(root).as_posix()})
        for function in functions(source):
            location = {"path": name, **{key: function[key] for key in ("name", "line", "lines", "brace_depth")}}
            if function["lines"] >= 80 or function["brace_depth"] >= 6:
                hotspots.append(location)
            if len(function["body"]) >= 100:
                clones[function["body"]].append(location)
    write_csv(output / "inventory.csv", inventory, ["path", "kind", "bytes", "lines", "sha256", "audit"])
    write_csv(output / "game-access.csv", calls, ["path", "line", "receiver", "api"])
    write_csv(output / "inheritance.csv", inheritance, ["path", "line", "class", "bases"])
    report = {"head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root).decode().strip(),
              "first_party_cpp_files": sum(row["kind"] == "cpp" for row in inventory),
              "hotspots": sorted(hotspots, key=lambda row: row["lines"], reverse=True),
              "exact_body_clone_candidates": [group for group in clones.values() if len(group) > 1],
              "include_edges": edges,
              "header_include_cycles": header_cycles(edges),
              "limitations": ["Static candidates require semantic review; no defect is inferred from size alone.",
                               "Game accesses use named/typed receivers and GetGame accessors; inferred aliases, templates and callbacks are not resolved.",
                               "Pointer lifetime and behavioral equivalence require call-site review and runtime tests.",
                               "File inventory includes assets; static scan status does not mean manual review of every line."]}
    (output / "structure.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"files": len(inventory), "cpp_files": report["first_party_cpp_files"], "game_calls": len(calls),
                      "hotspots": len(hotspots), "clone_candidates": len(report["exact_body_clone_candidates"])}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("review/current"))
    arguments = parser.parse_args()
    audit(Path(__file__).resolve().parents[1], arguments.output)
