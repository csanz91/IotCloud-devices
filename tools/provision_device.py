#!/usr/bin/env python3
"""Provision the reset device through its local setup access point.

Connect the computer to the device AP before running this script.  The only
value requested interactively is the fresh MQTT token from the latest app
registration attempt; the token contains the matching backend device ID.

The Wi-Fi credentials come from WIFI_SSID and WIFI_PASSWORD, set in the
environment or in tools/.env (gitignored).
"""

from __future__ import annotations

import base64
import binascii
import getpass
import json
import os
import urllib.error
import urllib.request
from collections.abc import Mapping
from pathlib import Path
from typing import Any


# Fixed recovery configuration for this device.
DEVICE_IP = "192.168.4.1"
LOCATION_ID = "5ca4784d931b1502f377c92d"
ENV_FILE = Path(__file__).resolve().parent / ".env"


class ProvisioningError(RuntimeError):
    pass


def read_env_file(path: Path) -> dict[str, str]:
    """Read KEY=VALUE lines, skipping blanks and comments."""
    values: dict[str, str] = {}
    if not path.is_file():
        return values
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip().strip("'\"")
    return values


def wifi_credentials() -> tuple[str, str]:
    """Wi-Fi SSID and password; the environment wins over tools/.env."""
    file_values = read_env_file(ENV_FILE)
    ssid = os.environ.get("WIFI_SSID") or file_values.get("WIFI_SSID", "")
    password = os.environ.get("WIFI_PASSWORD") or file_values.get("WIFI_PASSWORD", "")
    if not ssid or not password:
        raise ProvisioningError(f"Set WIFI_SSID and WIFI_PASSWORD in {ENV_FILE} or the environment")
    return ssid, password


def json_request(
    url: str,
    *,
    method: str = "GET",
    payload: Mapping[str, Any] | None = None,
) -> tuple[int, Any]:
    headers = {"Accept": "application/json"}
    body = None
    if payload is not None:
        headers["Content-Type"] = "application/json"
        body = json.dumps(payload).encode("utf-8")

    request = urllib.request.Request(url, data=body, headers=headers, method=method)
    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            status = response.status
            raw = response.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as error:
        status = error.code
        raw = error.read().decode("utf-8", errors="replace")
    except urllib.error.URLError as error:
        raise ProvisioningError(f"Cannot reach {url}: {error.reason}") from error

    try:
        return status, json.loads(raw)
    except json.JSONDecodeError as error:
        raise ProvisioningError(
            f"{method} {url} returned malformed JSON: {raw[:200]!r}"
        ) from error


def mapping(value: Any, context: str) -> Mapping[str, Any]:
    if not isinstance(value, Mapping):
        raise ProvisioningError(f"{context} did not return a JSON object")
    return value


def device_id_from_token(token: str) -> str:
    """Read the device ID claim without attempting to verify the JWT."""
    parts = token.split(".")
    if len(parts) != 3:
        raise ProvisioningError("The MQTT token is not a JWT")
    try:
        padding = "=" * (-len(parts[1]) % 4)
        claims = json.loads(base64.urlsafe_b64decode(parts[1] + padding))
    except (binascii.Error, ValueError, json.JSONDecodeError) as error:
        raise ProvisioningError("The MQTT token has an invalid payload") from error

    if claims.get("locationId") not in (None, LOCATION_ID):
        raise ProvisioningError("The MQTT token belongs to a different location")
    device_id = claims.get("deviceId")
    if not isinstance(device_id, str) or not device_id:
        raise ProvisioningError("The MQTT token contains no deviceId")
    return device_id


def main() -> int:
    try:
        wifi_ssid, wifi_password = wifi_credentials()
        status, response = json_request(f"http://{DEVICE_IP}/getDeviceInfo")
        if status != 200:
            raise ProvisioningError(f"/getDeviceInfo returned HTTP {status}: {response!r}")

        envelope = mapping(response, "/getDeviceInfo")
        if envelope.get("result") is not True:
            raise ProvisioningError(f"Device rejected /getDeviceInfo: {envelope.get('data')!r}")
        info = mapping(envelope.get("data"), "/getDeviceInfo.data")
        print(
            f"Device {info.get('deviceInternalId')} | "
            f"firmware {info.get('deviceVersion')} | "
            f"sensors {len(info.get('sensors', []))}"
        )

        mqtt_token = getpass.getpass("Paste the fresh MQTT token: ")
        device_id = device_id_from_token(mqtt_token)
        print(f"Using device ID {device_id}")

        payload = {
            "token": mqtt_token,
            "deviceId": device_id,
            "wifiSSID": wifi_ssid,
            "wifiPassword": wifi_password,
            "locationId": LOCATION_ID,
        }
        status, response = json_request(
            f"http://{DEVICE_IP}/deviceSetup",
            method="POST",
            payload=payload,
        )
        if status != 200:
            raise ProvisioningError(f"/deviceSetup returned HTTP {status}: {response!r}")

        envelope = mapping(response, "/deviceSetup")
        if envelope.get("result") is not True:
            raise ProvisioningError(f"Device rejected /deviceSetup: {envelope.get('data')!r}")
        print("Credentials accepted. The device should restart and join Wi-Fi.")
        return 0
    except KeyboardInterrupt:
        print("Cancelled.")
        return 1
    except ProvisioningError as error:
        print(f"Provisioning failed: {error}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
