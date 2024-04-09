import requests
from datetime import datetime, timedelta
from math import pow
import json
import pytz

OURSKY_API_TOKEN = ""
TARGET_IDS = ['example1', 'example2']
DAYS = 1

def parse_datetime(datetime_str):
    for fmt in ('%Y-%m-%dT%H:%M:%S.%f', '%Y-%m-%dT%H:%M:%S'):
        try:
            return datetime.strptime(datetime_str, fmt)
        except ValueError:
            pass
    raise ValueError('no valid date format found')

def get_satellite_info(target_id):
    url = "https://api.prod.oursky.ai/v1/satellite-target"
    headers = {"Authorization": f"Bearer {OURSKY_API_TOKEN}"}
    querystring = {"id": target_id}
    response = requests.get(url, headers=headers, params=querystring)
    if response.status_code == 200:
        return response.json()
    else:
        print(f"failed to get info for target_id {target_id}")
        return None

def determine_meters_off(nodeLocation, target_info, obsTime_str):
    obsTime = parse_datetime(obsTime_str)
    time_diff = (obsTime - parse_datetime(target_info['tleEpoch'].rstrip('Z'))).total_seconds()
    staleness_days = max(0, min(5.0, time_diff / 86400))
    return pow(staleness_days, 1.5276) * 1000 # estimate from https://destevez.net/2017/11/a-brief-study-of-tle-variation/

def get_potentials(target_id, until_date):
    url = "https://api.prod.oursky.ai/v1/satellite-target-potentials"
    querystring = {"satelliteTargetId": target_id, "until": until_date}
    headers = {"Authorization": OURSKY_API_TOKEN}

    response = requests.get(url, headers=headers, params=querystring)
    if response.status_code == 200:
        try:
            response_data = response.json()
            return [item for item in response_data if item['lastObservableTime'][-9:] != 'T00:00:00Z']
        except json.JSONDecodeError:
            print("error:", response.text)
    else:
        print(f"failed to get potentials for target_id {target_id}: {response.status_code}")
        print(response.text)
    return []

def schedule_observations(target_ids, observation_time, until_time):
    orbit_type_offsets = {
        'LEO': [(1, 1/4), (3, 1/2), (5, 1), (7, 2), (12, 5), (float('inf'), 10)],
        'MEO': [(1, 1/4), (3, 1/3), (5, 1/2), (7, 1), (12, 2), (float('inf'), 5)],
        'GEOSYNCHRONOUS': [(1, 1/4), (3, 1/3), (5, 1/2), (7, 3/4), (12, 1), (float('inf'), 2)],
        'GEOSTATIONARY': [(1, 1/5), (3, 1/4), (5, 1/3), (7, 1/2), (12, 3/4), (float('inf'), 1)]
    }

    for target_id in target_ids:
        target_info = get_satellite_info(target_id)

        if target_info:
            orbit_type = target_info.get('orbitType')
            tle_age_days = (parse_datetime(observation_time) - parse_datetime(target_info['tleEpoch'].rstrip('Z'))).total_seconds() / 86400
            meters_off = determine_meters_off(None, target_info, observation_time)
            offset_fraction = next(fraction for age, fraction in orbit_type_offsets.get(orbit_type, orbit_type_offsets['LEO']) if tle_age_days <= age)
            adjusted_along_cross_offset = meters_off * offset_fraction

            print(f"\nTLE age for target {target_id}: {tle_age_days:.2f} days, orbit type: {orbit_type}")
            print(f"Along-track & Cross-track Offset = {adjusted_along_cross_offset:.2f} meters, Radial Offset = {meters_off:.2f} meters")

            if input("Confirm creating search instructions (y/n):").lower() == 'y':
                tracking_type_choice = input("Sidereal or rate tracked (s/r):").lower()
                search_type = input("Enter search instruction type (flyingv, raster, spiral, concentric, stayontarget, onestep):").lower()
                tracking_type = "SIDEREAL" if tracking_type_choice == 's' else "TARGET_RATE"
                potential_windows = get_potentials(target_id, until_time)

                print("Potential windows:", potential_windows)

                for window in potential_windows:
                    first_obs_time = window['firstObservableTime']
                    last_obs_time = window['lastObservableTime']
                    steps = search_instruction_type(search_type, adjusted_along_cross_offset, first_obs_time, last_obs_time)

                    print(f"Scheduling observation for window: Start at {first_obs_time}, end at {last_obs_time}")
                    schedule_observation(target_id, tracking_type, steps)
            else:
                print("Skipping this target")

def search_instruction_type(search_type, adjusted_along_cross_offset, start_time, end_time):
    steps = []
    step_duration = timedelta(seconds=10)
    interval_duration = timedelta(seconds=10)
    current_time = datetime.strptime(start_time, '%Y-%m-%dT%H:%M:%S.%fZ')

    max_duration = timedelta(minutes=4, seconds=59)
    observation_end_time = current_time + max_duration
    final_end_time = datetime.strptime(end_time, '%Y-%m-%dT%H:%M:%S.%fZ')

    if observation_end_time > final_end_time:
        observation_end_time = final_end_time

    def add_step(along_track, cross_track, radial=0):
        nonlocal current_time
        step_end_time = current_time + step_duration
        if step_end_time > observation_end_time:
            return False

        steps.append({
            "alongTrackOffsetMeters": along_track,
            "crossTrackOffsetMeters": cross_track,
            "radialOffsetMeters": radial,
            "startTime": current_time.strftime('%Y-%m-%dT%H:%M:%S.%fZ')[:-4] + 'Z',
            "endTime": step_end_time.strftime('%Y-%m-%dT%H:%M:%S.%fZ')[:-4] + 'Z'
        })
        current_time = step_end_time + interval_duration
        return True

    if search_type == "flyingv":
        for offsets in [(0, 0), (-1, 1), (2, 0), (-2, -1), (1, 1)]:
            if not add_step(offsets[0] * adjusted_along_cross_offset, offsets[1] * adjusted_along_cross_offset):
                break

    elif search_type == "raster":
        for i in range(10):
            if not add_step(adjusted_along_cross_offset * i, adjusted_along_cross_offset) or not add_step(adjusted_along_cross_offset * i, -adjusted_along_cross_offset):
                break

    elif search_type == "spiral":
        for i in range(1, 6):
            if not add_step(adjusted_along_cross_offset * i, adjusted_along_cross_offset * i):
                break

    elif search_type == "concentric":
        directions = [(1, 0), (0, 1), (-1, 0), (0, -1)]
        for i, direction in enumerate(directions):
            if not add_step(adjusted_along_cross_offset * direction[0] * (i+1), adjusted_along_cross_offset * direction[1] * (i+1)):
                break

    elif search_type == "movealongtrack":
        j = 1
        while(abs(j) < 30):
            if not add_step(adjusted_along_cross_offset * j, 0):
                break
            j += 1
            j *= -1
    elif search_type == "stayontarget":
        total_steps = (max_duration // (step_duration + interval_duration))

        for _ in range(int(total_steps)):
            if not add_step(0, 0):
                break
    elif search_type == "onestep":
        add_step(0, 0)

    else:
        raise ValueError("Invalid search type")

    return steps

def schedule_observation(target_id, tracking_type, steps):
    url = "https://api.prod.oursky.ai/v1/search-instruction"

    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {OURSKY_API_TOKEN}"
    }

    payload = {
        "steps": steps,
        "targetId": target_id,
        "trackingType": tracking_type
    }

    response = requests.post(url, json=payload, headers=headers)
    if response.status_code == 200:
        print(f"Successfully scheduled observation for target {target_id}")
    else:
        print(f"Failed to schedule observation for target {target_id}: {response.text}")

if __name__ == "__main__":
    observation_time = datetime.now().isoformat(timespec='microseconds')
    until_utc = datetime.now(pytz.utc) + timedelta(hours= 24 * DAYS)
    until_time = until_utc.strftime('%Y-%m-%dT%H:%M:%S.%fZ')[:-4] + 'Z'
    schedule_observations(TARGET_IDS, observation_time, until_time)