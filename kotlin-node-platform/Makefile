.PHONY: build test c2-capture continuous-capture metrics
NUM_THREADS ?= 4
GRADLE = ./gradlew --max-workers=$(NUM_THREADS)

build:
	$(GRADLE) build

test:
	$(GRADLE) test

c2-capture continuous-capture metrics:
	$(GRADLE) --console=plain run --args=$@
