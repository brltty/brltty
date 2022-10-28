SYSTEMD_UNIT = brltty@canute.path

status:
	systemctl $@ $(SYSTEMD_UNIT)

enable disable start stop:
	sudo systemctl $@ $(SYSTEMD_UNIT)

restart:
	sudo systemctl stop $(SYSTEMD_UNIT)
	sudo systemctl start $(SYSTEMD_UNIT)

