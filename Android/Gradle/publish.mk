GRADLE_PUBLISH_COMMAND = $(GRADLE_WRAPPER_COMMAND) :$(GRADLE_APP_NAME):publishReleaseBundle
GRADLE_PROMOTE_COMMAND = $(GRADLE_WRAPPER_COMMAND) :$(GRADLE_APP_NAME):promoteArtifact

GRADLE_LISTING_DIRECTORY = app/src/main/play

publish-alpha:
	$(GRADLE_PUBLISH_COMMAND) --track alpha --release-status inProgress

publish-beta:
	$(GRADLE_PUBLISH_COMMAND) --track beta --release-status inProgress

publish-production:
	$(GRADLE_PUBLISH_COMMAND) --track production --release-status completed

promote-to-beta:
	$(GRADLE_PROMOTE_COMMAND) --from-track alpha --promote-track beta --release-status inProgress

promote-to-production:
	$(GRADLE_PROMOTE_COMMAND) --from-track beta --promote-track production --release-status completed

download-listing:
	$(GRADLE_WRAPPER_COMMAND) bootstrap

publish-listing:
	$(GRADLE_WRAPPER_COMMAND) publishListing

clean-listing:
	-rm -f -r $(GRADLE_LISTING_DIRECTORY)

