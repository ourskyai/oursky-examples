# Python Node Platform - System Status and Mount Model

Uses the OurSky Node Platform Python SDK to fetch and display the system status and mount model of a node.

## Setup

1. Install dependencies:

```bash
pip install -r requirements.txt
```

2. Copy `.env.example` to `.env` and set your API token (required), Lineage ID (required), and optionally the API base url.

```bash
cp .env.example .env
```

3. Run the example:

```bash
python main.py
```

## Mount model examples

The default run calls `v1GetSystemStatus` and `v1GetMountModel`. `main.py` also includes commented examples for APIs that modify the current mount model:

- `v1AddMountModelPointHere`
- `v1AddMountModelPointUsingImage`
- `v1RemoveMountModelPoint`
- `v1ClearMountModel`

Uncomment only the calls you want to run.
