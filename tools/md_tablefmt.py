#!/usr/bin/env -S uv --quiet run --script
# /// script
# requires-python = ">=3.13"
# dependencies = []
# ///
#
"""Markdown pipe-table formatter.

Reformats GitHub-flavoured markdown pipe tables into aligned columns so they
read well in raw/text editors, while staying valid markdown. Also usable as a
library.

Alignment
---------
By default every column is left-aligned. Pass per-column alignment via the
header row's existing delimiter syntax (``:---``, ``:--:``, ``---:``), or
explicitly with :func:`align_table`.

Examples
--------
Library::

    from tools.md_tablefmt import align_table
    print(align_table(["a", "b"], [["1", "2"], ["33", "4"]]))

CLI - format tables in a markdown file in place::

    python tools/md_tablefmt.py README.md

CLI - read from stdin, write to stdout::

    cat README.md | python tools/md_tablefmt.py -

CLI - emit a TSV code block instead (tab-separated, editor-friendly)::

    python tools/md_tablefmt.py README.md --tsv
"""
from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass

__all__ = [
    "ALIGN",
    "parse_delimiter",
    "align_table",
    "format_markdown_tables",
    "convert_table_to_tsv",
]


# Column alignment hint carried by the delimiter row.
@dataclass(frozen=True)
class Align:
    char: str  # one of "", ":", ":" for left, center, right


# Sentinel objects are clearer than magic strings.
class _A:
    LEFT = "left"
    CENTER = "center"
    RIGHT = "right"


ALIGN = _A


_DELIM_RE = re.compile(r"^\|?\s*:?-+:?\s*(\|\s*:?-+:?\s*)*\|?\s*$")


def parse_delimiter(cell: str) -> str:
    """Return one of ALIGN.LEFT/CENTER/RIGHT from a delimiter-row cell.

    ``cell`` is a single column of the ``|---|---:|`` row, already stripped of
    surrounding pipes and whitespace (e.g. ``":--:"``, ``"---"``).
    """
    c = cell.strip()
    left = c.startswith(":")
    right = c.endswith(":")
    if left and right:
        return ALIGN.CENTER
    if right:
        return ALIGN.RIGHT
    return ALIGN.LEFT


def _split_row(line: str) -> list[str]:
    """Split a markdown table row into stripped cells (pipes optional)."""
    s = line.strip()
    if s.startswith("|"):
        s = s[1:]
    if s.endswith("|") and not s.endswith(r"\|"):
        s = s[:-1]
    # split on unescaped pipes
    return [c.strip() for c in re.split(r"(?<!\\)\|", s)]


def _is_table_block(lines: list[str], i: int) -> bool:
    """True if lines[i] is a table header and lines[i+1] a delimiter row."""
    if i + 1 >= len(lines):
        return False
    if "|" not in lines[i]:
        return False
    return bool(_DELIM_RE.match(lines[i + 1].strip()))


def _column_widths(rows: list[list[str]]) -> list[int]:
    """Max content length per column (excluding padding)."""
    return [max(len(c) for c in col) for col in zip(*rows)]


def align_table(
    header: list[str],
    rows: list[list[str]],
    alignments: list[str] | None = None,
    pad: int = 2,
) -> str:
    """Render a markdown pipe table with aligned columns.

    Parameters
    ----------
    header, rows
        Cell contents (strings). ``rows`` may be empty.
    alignments
        Per-column alignment; defaults to all :data:`ALIGN.LEFT`. Values are
        ``ALIGN.LEFT`` / ``ALIGN.CENTER`` / ``ALIGN.RIGHT``.
    pad
        Spaces of padding inside each cell (one space added either side of the
        content beyond this).
    """
    ncol = len(header)
    for r in rows:
        if len(r) != ncol:
            raise ValueError(
                f"row has {len(r)} cells, expected {ncol}: {r!r}"
            )
    if alignments is None:
        alignments = [ALIGN.LEFT] * ncol
    if len(alignments) != ncol:
        raise ValueError(
            f"got {len(alignments)} alignments for {ncol} columns"
        )

    all_rows = [header, *rows]
    widths = _column_widths(all_rows)

    def apply(text: str, align: str, w: int) -> str:
        # justify content to width w, then pad each side with `pad` spaces so
        # every rendered cell is exactly (w + 2*pad) chars wide.
        if align == ALIGN.RIGHT:
            core = text.rjust(w)
        elif align == ALIGN.CENTER:
            core = text.center(w)
        else:
            core = text.ljust(w)
        return " " * pad + core + " " * pad

    def row_line(cells: list[str]) -> str:
        return "| " + " | ".join(
            apply(cells[i], alignments[i], widths[i]) for i in range(ncol)
        ).rstrip() + " |"

    def delim_line() -> str:
        parts = []
        for i in range(ncol):
            a = alignments[i]
            # dashes span the full padded cell width (content width + 2*pad),
            # minus one char on each side for any alignment colons.
            span = max(widths[i], 3) + 2 * pad
            if a == ALIGN.CENTER:
                parts.append(":" + "-" * (span - 2) + ":")
            elif a == ALIGN.RIGHT:
                parts.append("-" + "-" * (span - 2) + ":")
            else:
                parts.append("-" + "-" * (span - 1))
        return "| " + " | ".join(parts) + " |"

    out = [row_line(header), delim_line()]
    out.extend(row_line(r) for r in rows)
    return "\n".join(out)


def convert_table_to_tsv(
    header: list[str], rows: list[list[str]]
) -> str:
    """Render a table as a fenced ```tsv code block (tab-separated)."""
    body = "\t".join(header)
    for r in rows:
        body += "\n" + "\t".join(r)
    return "```tsv\n" + body + "\n```"


def format_markdown_tables(
    text: str, *, tsv: bool = False
) -> str:
    """Reformat every pipe table in a markdown string.

    Tables are detected by a header row followed by a delimiter row. Adjacent
    lines until a non-table line form the table body. When ``tsv`` is true,
    each table is rewritten as a ``\\`\\`\\`tsv`` fenced block instead of an
    aligned pipe table.
    """
    lines = text.splitlines(keepends=False)
    out: list[str] = []
    i = 0
    n = len(lines)
    while i < n:
        if _is_table_block(lines, i):
            header = _split_row(lines[i])
            delim = _split_row(lines[i + 1])
            # parse alignment from the delimiter row
            try:
                alignments = [parse_delimiter(c) for c in delim]
            except Exception:
                alignments = [ALIGN.LEFT] * len(header)
            # ensure alignment list matches header length
            if len(alignments) != len(header):
                alignments = [ALIGN.LEFT] * len(header)

            j = i + 2
            rows: list[list[str]] = []
            while j < n and lines[j].strip().startswith("|"):
                rows.append(_split_row(lines[j]))
                j += 1

            if tsv:
                out.append(convert_table_to_tsv(header, rows))
            else:
                out.append(align_table(header, rows, alignments))
            i = j
        else:
            out.append(lines[i])
            i += 1
    return "\n".join(out) + ("\n" if text.endswith("\n") else "")


def _cli(argv: list[str]) -> int:
    p = argparse.ArgumentParser(
        prog="md_tablefmt",
        description="Reformat markdown pipe tables into aligned columns.",
    )
    p.add_argument(
        "path",
        nargs="?",
        default="-",
        help="markdown file to format in place, or '-' for stdin (default).",
    )
    p.add_argument(
        "--tsv",
        action="store_true",
        help="emit each table as a fenced ```tsv block instead.",
    )
    p.add_argument(
        "-o",
        "--output",
        default=None,
        help="output file (defaults to overwriting PATH, or stdout for '-').",
    )
    args = p.parse_args(argv)

    if args.path == "-":
        src = sys.stdin.read()
        result = format_markdown_tables(src, tsv=args.tsv)
        out_path = args.output
        if out_path and out_path != "-":
            with open(out_path, "w") as f:
                f.write(result)
        else:
            sys.stdout.write(result)
        return 0

    with open(args.path) as f:
        src = f.read()
    result = format_markdown_tables(src, tsv=args.tsv)
    out_path = args.output or args.path
    with open(out_path, "w") as f:
        f.write(result)
    return 0


if __name__ == "__main__":
    raise SystemExit(_cli(sys.argv[1:]))
