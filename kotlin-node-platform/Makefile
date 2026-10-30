.PHONY: build test c2-capture continuous-capture metrics schemas
NUM_THREADS ?= 4
GRADLE = ./gradlew --max-workers=$(NUM_THREADS)
FLATC ?= flatc

build: schemas
	$(GRADLE) build

test: schemas
	$(GRADLE) test

c2-capture continuous-capture metrics: schemas
	$(GRADLE) --console=plain run --args=$@

schemas:
	$(FLATC) --java -o src/main/java flatbuffers/*.fbs
