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
