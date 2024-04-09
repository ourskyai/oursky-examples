# Example: python-schedule-search-instructions

Example for scheduling satellite search instructions by fetching observation potentials and estimating a search step size based on orbit type and TLE data age. 

* Some custom search patterns to be assigned for each target, which are then scheduled for the potential observation windows.

* Sidereal or rate tracking are both possible. Sidereal steps will require much larger overheads.

## Requirements:

* You should have signed up to [OurSky](https://console.prod.oursky.ai/) and have a valid token. Insert it into `OURSKY_API_TOKEN`. 

## Getting Started:

* Enter a list of `TARGET_IDS` from the OurSky platform. 

* Enter how many days ahead to query observation potentials for in `DAYS`.