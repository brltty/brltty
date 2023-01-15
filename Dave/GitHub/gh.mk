gh-install: gh-download
	sudo dpkg --install gh.deb

gh-download: gh-clean
	./gh-download

gh-clean:
	-rm -f -- gh.deb

