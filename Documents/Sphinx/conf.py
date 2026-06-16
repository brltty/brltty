# Sphinx configuration for BRLTTY manuals
#
# Each manual is built with:
#   sphinx-build -b singlehtml -D master_doc=<NAME> <srcdir> <outdir>

project = 'BRLTTY'
copyright = '1995-2026, The BRLTTY Developers'
author = 'The BRLTTY Developers'

extensions = []
# French chapter files included via .. include:: in the master document.
# If Sphinx discovers them independently it creates duplicate labels.
# These names don't exist in the English or BrlAPI directories so the
# exclude is harmless for those builds.
exclude_patterns = [
    '_build',
    'Introduction.rst',
    'Compilation.rst',
    'Utilisation.rst',
    'Features.rst',
    'Translation.rst',
    'Advanced.rst',
    'Displays.rst',
    'Synthesizers.rst',
    'Drivers.rst',
    'Screen.rst',
    'Syntax.rst',
    'Dots.rst',
    'nabcc.rst',
    'fr-2007.rst',
    'Midi.rst',
]
source_encoding = 'utf-8'
master_doc = 'BRLTTY'

# Technical manuals describe command-line syntax; smart-quote substitution
# would turn '--option' into en-dashes and ISO dates into em-dashes.
smartquotes = False
