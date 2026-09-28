"""This saves every optical tube test report shared with your organization to optical_tube_test_reports.txt.

Requires Python 3.9 or newer and httpx (pip install httpx).
Set OURSKY_API_TOKEN to your API token and run: python optical_tube_test_reports.py
"""

import asyncio
import os
import httpx

API_URL = "https://api.prod.oursky.ai"
PAGE_SIZE = 100
OUTPUT_PATH = "optical_tube_test_reports.txt"


async def fetch_report_summaries(client):
    summaries = []
    while True:
        response = await client.get(
            "/v1/optical-tube-test-reports",
            params={"limit": PAGE_SIZE, "offset": len(summaries)},
        )
        response.raise_for_status()
        page = response.json()
        summaries.extend(page["reports"])
        if not page["reports"] or len(summaries) >= page["total"]:
            return summaries


async def fetch_report(client, report_id):
    response = await client.get(
        "/v1/optical-tube-test-report",
        params={"opticalTubeTestReportId": report_id},
    )
    response.raise_for_status()
    return response.json()


def format_report(report):
    lines = [f"Optical tube {report['serialNumber']}, tested {report['createdAt']}"]
    for measurement in report["focusMeasurements"]:
        ee50 = measurement["halfFluxDiameterStats"]
        lines.append(f"\t{measurement['fieldPosition']}\tEE50 mean {ee50['mean']:.2f} px, median {ee50['median']:.2f} px, min {ee50['min']:.2f} px")
        for exposure in measurement["exposures"]:
            lines.append(f"\t\texposure {exposure['exposureIndex']}\tEE50 {exposure['halfFluxDiameterPixels']:.2f} px\t{exposure['fitsUrl']}")
    return lines


async def main():
    headers = {"Authorization": f"Bearer {os.environ['OURSKY_API_TOKEN']}"}
    async with httpx.AsyncClient(base_url=API_URL, headers=headers, timeout=30.0) as client:
        summaries = await fetch_report_summaries(client)
        lines = [f"{len(summaries)} optical tubes with a test report", ""]
        for summary in summaries:
            report = await fetch_report(client, summary["reportId"])
            lines += format_report(report) + [""]
    with open(OUTPUT_PATH, "w") as output_file:
        output_file.write("\n".join(lines))


if __name__ == "__main__":
    asyncio.run(main())
