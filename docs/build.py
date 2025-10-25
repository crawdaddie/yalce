#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.11"
# dependencies = [
#     "markdown",
#     "pygments",
# ]
# ///
"""Build documentation from markdown files."""

import os
import glob
import re
from pathlib import Path

import markdown
from markdown.extensions.codehilite import CodeHiliteExtension
from pygments.formatters import HtmlFormatter


def convert_md_links_to_html(md_content):
    """Convert markdown links to .md files into .html links."""
    # Pattern: [text](file.md) or [text](path/to/file.md)
    pattern = r'\[([^\]]+)\]\(([^\)]+\.md)\)'

    def replacer(match):
        text = match.group(1)
        link = match.group(2)
        # Replace .md with .html
        html_link = link.replace('.md', '.html')
        return f'[{text}]({html_link})'

    return re.sub(pattern, replacer, md_content)


def build_docs():
    """Convert all .md files in docs/ to individual HTML files."""
    docs_dir = Path(__file__).parent
    md_files = sorted(docs_dir.glob("*.md"))

    if not md_files:
        print("No markdown files found in docs/")
        return

    # Use a light-background friendly style with explicit background color
    formatter = HtmlFormatter(style='default', noclasses=False)
    pygments_css = formatter.get_style_defs('.codehilite')

    template_path = docs_dir / "template.html"
    if not template_path.exists():
        print(f"Error: Template file not found at {template_path}")
        exit(1)

    with open(template_path, 'r') as f:
        template = f.read()

    # Convert each markdown file to HTML
    output_files = []
    for md_file in md_files:
        with open(md_file, 'r') as f:
            md_content = f.read()

            # Convert markdown links to HTML links
            md_content = convert_md_links_to_html(md_content)

            # Convert markdown to HTML with syntax highlighting
            html_content = markdown.markdown(
                md_content,
                extensions=[
                    'fenced_code',
                    'tables',
                    CodeHiliteExtension(linenums=False, guess_lang=False)
                ]
            )

        # Replace template variables
        html = template.replace("{{ CONTENT }}", html_content)
        html = html.replace("{{ PYGMENTS_CSS }}", pygments_css)

        # Write to corresponding .html file
        html_filename = md_file.stem + ".html"
        html_path = docs_dir / "web" / html_filename
        with open(html_path, 'w') as f:
            f.write(html)

        output_files.append(html_filename)

    print(f"✓ Built documentation from {len(md_files)} markdown file(s)")
    for output in output_files:
        print(f"  → {output}")


if __name__ == "__main__":
    build_docs()
