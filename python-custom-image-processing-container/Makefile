# Makefile for Custom Image Processing Consumer

# Variables
PYTHON := python3
VENV := venv
VENV_BIN := $(VENV)/bin
PIP := $(VENV_BIN)/pip
PYTHON_VENV := $(VENV_BIN)/python
OPENAPI_SPEC := consumer/openapi.yaml
GENERATED_DIR := consumer/generated

# Phony targets (not actual files)
.PHONY: help install generate run clean venv

# Default target
help:
	@echo "Available targets:"
	@echo "  install    - Create virtual environment and install dependencies"
	@echo "  generate   - Generate FastAPI code from OpenAPI spec"
	@echo "  run        - Build and run Docker containers"
	@echo "  clean      - Remove virtual environment and generated code"
	@echo "  venv       - Create virtual environment only"

# Create virtual environment
venv:
	$(PYTHON) -m venv $(VENV)
	$(PIP) install --upgrade pip

# Install dependencies
install: venv
	$(PIP) install -r requirements.txt

# Generate FastAPI code from OpenAPI spec
generate: install
	$(VENV_BIN)/openapi-generator-cli generate \
		-i $(OPENAPI_SPEC) \
		-g python-fastapi \
		-o $(GENERATED_DIR)

build:
	docker buildx create --name multiarch --use || true
	docker buildx build --platform linux/arm64 -t custom-image-processing:latest --load .


# Run Docker containers
run: build
	cd $(GENERATED_DIR) && docker compose up

# Clean up generated files and virtual environment
clean:
	rm -rf $(VENV)
	rm -rf $(GENERATED_DIR)
