# Example: python-poetry-sda-basic

Uses the OurSky SDA Python SDK to show how to import and use the SDK to fetch an array of OSRs.

## Requirements:

* [poetry](https://python-poetry.org/) python package manager

## Getting Started

* `cd python-poetry-sda-basic`
* Install packages with `poetry install`
* Configure your API token with an evironment variable `OURSKY_API_TOKEN` (you can put this in `.env` file OR globally in your shell)
* Run the example: `poetry run python oursky_sda/get_osrs.py`

You should see either an empty array or some osr's
```sh
❯ poetry run python oursky_sda/get_osrs.py
[]
```