#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.11"
# dependencies = [
#     "markdown",
#     "pygments",
# ]
# ///
"""Build documentation from markdown files."""

# import os
# import glob
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


def convert_relative_links_to_github(md_content):
    """Convert relative ../ links to GitHub repository URLs."""
    github_base = "https://github.com/crawdaddie/yalce/tree/main"
    # Pattern: [text](../path)
    pattern = r'\[([^\]]+)\]\(\.\./([^\)]+)\)'

    def replacer(match):
        text = match.group(1)
        path = match.group(2)
        # Convert to GitHub URL
        github_url = f'{github_base}/{path}'
        return f'[{text}]({github_url})'

    return re.sub(pattern, replacer, md_content)


def build_docs():
    """Convert all .md files in docs/ to individual HTML files."""
    # Register the YLC lexer

    docs_dir = Path(__file__).parent
    md_files = sorted(docs_dir.glob("*.md"))

    if not md_files:
        print("No markdown files found in docs/")
        return

    # formatter = HtmlFormatter(style='solarized-light', noclasses=False)
    formatter = HtmlFormatter(style='friendly', noclasses=False)
    pygments_css = formatter.get_style_defs('.codehilite')

    css_template_path = docs_dir / "style.css"
    if css_template_path.exists():
        with open(css_template_path, 'r') as f:
            css_content = f.read()
        css_content = css_content.replace("{{ PYGMENTS_CSS }}", pygments_css)
        css_output_path = docs_dir / "web" / "style.css"
        with open(css_output_path, 'w') as f:
            f.write(css_content)

    template_path = docs_dir / "template.html"
    if not template_path.exists():
        print(f"Error: Template file not found at {template_path}")
        exit(1)

    with open(template_path, 'r') as f:
        template = f.read()

    output_files = []
    for md_file in md_files:
        with open(md_file, 'r') as f:
            md_content = f.read()

            md_content = convert_md_links_to_html(md_content)
            md_content = convert_relative_links_to_github(md_content)

            html_content = markdown.markdown(
                md_content,
                extensions=[
                    'fenced_code',
                    'tables',
                    'toc',
                    CodeHiliteExtension(linenums=False, guess_lang=False)
                ]
            )

        html = template.replace("{{ CONTENT }}", html_content)

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
