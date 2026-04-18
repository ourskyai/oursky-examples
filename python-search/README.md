# python-search

Create searches for a satellite target using the OurSky v2 search endpoints.
Two scripts are included:

- `plane_scan.py` - 1D search along the target's orbital plane. Best for
  the common LEO case where along-track uncertainty dominates.
- `ric_volumetric.py` - 3D box in the target's Radial / In-track /
  Cross-track frame. Best when radial or cross-track uncertainty is also
  material, such as a freshly cataloged target or a post-maneuver reacquisition.

## Requirements

- Python 3.10+
- An OurSky account and API token from
  [https://console.prod.oursky.ai/](https://console.prod.oursky.ai/) ->
  Account -> General
- The UUID of a satellite target in your organization. Find it in the
  console, or from `GET /v1/satellite-targets`.

## Setup

```
pip install ourskyai-sda-api
export OURSKY_API_TOKEN=<your token>
```

## Run

Plane scan, 2 degrees on either side of the nominal anomaly:

```
python plane_scan.py --target-id <uuid> --plus-minus-anomaly-degrees 2
```

RIC volumetric with conservative LEO defaults (R=500 m, I=10 km, C=1 km
at 3-sigma - edit the constants at the top of the script to change them):

```
python ric_volumetric.py --target-id <uuid>
```

## Sizing the search

For a LEO target with a day-old TLE, 1-3 degrees on either side of the
predicted anomaly is typically sufficient for a plane scan. If the TLE is a
week old, widen to 5-10 degrees.

Suggested 3-sigma starting values for a LEO target in a RIC volumetric search:

- Radial 3-sigma: 500m
- In-track 3-sigma: 10km (grows fastest)
- Cross-track 3-sigma: 1km
