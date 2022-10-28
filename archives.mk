INSTALL_CURRENT = $(INSTALL_LOCATION)/current
INSTALL_TREE = $(shell readlink -n -e -- $(INSTALL_CURRENT))

TAR_ARCHIVE = canute-brltty.tar
tar-archive: $(TAR_ARCHIVE)
$(TAR_ARCHIVE):
	tar --file $(TAR_ARCHIVE) --create --absolute-names -- $(INSTALL_TREE) $(INSTALL_CURRENT)

GZIP_ARCHIVE = $(TAR_ARCHIVE).gz
gzip-archive: $(GZIP_ARCHIVE)
$(GZIP_ARCHIVE): $(TAR_ARCHIVE)
	gzip -9 -c $(TAR_ARCHIVE) >$@

BZIP2_ARCHIVE = $(TAR_ARCHIVE).bz2
bzip2-archive: $(BZIP2_ARCHIVE)
$(BZIP2_ARCHIVE): $(TAR_ARCHIVE)
	bzip2 -9 -c $(TAR_ARCHIVE) >$@

XZ_ARCHIVE = $(TAR_ARCHIVE).xz
xz-archive: $(XZ_ARCHIVE)
$(XZ_ARCHIVE): $(TAR_ARCHIVE)
	xz -9 -c $(TAR_ARCHIVE) >$@

clean-archives:
	-rm -f -- *.tar *.tar
	-rm -f -- *.tar *.tar.*

