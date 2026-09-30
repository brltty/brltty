# Sphinx configuration for BRLTTY manuals
#
# Each manual is built with:
#   sphinx-build -b singlehtml -D master_doc=<NAME> <srcdir> <outdir>

project = 'BRLTTY'
copyright = '1995-2026, The BRLTTY Developers'
author = 'The BRLTTY Developers'

extensions = []
# Keep Sphinx from treating the build output directory as source.
exclude_patterns = ['_build']
source_encoding = 'utf-8'
master_doc = 'BRLTTY'

# Technical manuals describe command-line syntax; smart-quote substitution
# would turn '--option' into en-dashes and ISO dates into em-dashes.
smartquotes = False

# The singlehtml builder generates neither a general index nor a search
# page, yet the theme still links to genindex.html and search.html from
# the page header (text browsers show these as links at the top).
# Suppress both links.
html_use_index = False

# The theme's sidebar ("Navigation", "Documentation overview") has nothing
# to offer a single-page manual, and the footer only repeats the copyright
# notice already at the top of each manual and credits the tooling. Both
# are noise, especially when the page is read with a screen reader.
html_sidebars = {'**': []}
html_show_copyright = False
html_show_sphinx = False

# Don't append a "¶" permalink to every heading, table and code block:
# text browsers and screen readers present each one as an extra link.
html_permalinks = False

def _no_search_page(app):
    app.builder.search = False

# Only the .html file of each manual is installed, so anything it loads
# from _static/ would be missing. Make the page self-contained, as the
# rst2html-generated README pages are: embed the stylesheets (resolving
# @import), and drop the scripts, which only serve the search feature.

import os
import re

def _read_static_css(static_dir, name):
    with open(os.path.join(static_dir, name), encoding='utf-8') as css:
        text = css.read()

    return re.sub(
        r'@import\s+url\(\s*["\']?([^"\')]+)["\']?\s*\)\s*;',
        lambda match: _read_static_css(static_dir, match.group(1)),
        text
    )

def _embed_static_files(app, exception):
    if exception or app.builder.format != 'html':
        return

    out_dir = str(app.outdir)
    static_dir = os.path.join(out_dir, '_static')

    def embed_stylesheet(match):
        name = re.search(r'href="_static/([^"?]+)', match.group(0)).group(1)
        return '<style>\n' + _read_static_css(static_dir, name) + '</style>'

    for name in os.listdir(out_dir):
        if not name.endswith('.html'):
            continue

        path = os.path.join(out_dir, name)
        with open(path, encoding='utf-8') as page:
            html = page.read()

        html = re.sub(
            r'<link\b[^>]*\brel="stylesheet"[^>]*\bhref="_static/[^>]*>',
            embed_stylesheet, html
        )
        html = re.sub(r'[ \t]*<script\b[^>]*\bsrc="_static/[^"]*"[^>]*></script>\n?', '', html)

        with open(path, 'w', encoding='utf-8') as page:
            page.write(html)

def setup(app):
    app.connect('builder-inited', _no_search_page)
    app.connect('build-finished', _embed_static_files)
