# OurSky Examples

This reposistory is a list of OurSky examples using the published SDKs. 

Each folder is an "app" or example with minimal setup and independent readme's to describe the example or setup as needed.

## Examples:

### Python

* [python-poetry-sda-basic](./python-poetry-sda-basic) - Uses the OurSky SDA Python SDK to show how to import and use the SDK to fetch an array of TDMs


## Create Example

### Python + Poetry
```sh
poetry new ./<folder> --name oursky_sda
cd <folder>
poetry add oursky_sda_api
```
