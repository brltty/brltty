all: debug
debug: app-debug
release: app-release

core-debug:
	./gradlew -q :core:assembleDebug

core-release:
	./gradlew -q :core:assembleRelease

app-debug:
	./gradlew -q :app:assembleDebug

app-release:
	./gradlew -q :app:assembleRelease

bundle:
	./gradlew -q :app:bundleRelease

lint:
	./gradlew -q lintDebug

clean:
	./gradlew -q clean

purge: clean
	-rm -f -r .gradle

