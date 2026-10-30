.PHONY: help test status c2-capture continuous-capture metrics schemas
.DEFAULT_GOAL := help
PYTHON ?= python3
FLATC ?= flatc

help:
	@echo "Choose: make status, c2-capture, continuous-capture, metrics, or test"

status c2-capture continuous-capture metrics:
	$(PYTHON) main.py $@

test:
	$(PYTHON) -m pytest -q

schemas:
	mkdir -p generated
	$(FLATC) --python -o . flatbuffers/*.fbs
	$(PYTHON) -m grpc_tools.protoc -I proto --python_out=generated proto/metrics.proto
	touch generated/__init__.py
