# UniFi Motion Light

NodeMCU ESP8266 firmware that accepts an authenticated UniFi Protect motion
webhook and controls a light through an external mains-rated relay. It also
keeps an existing wall switch through a mains-rated optocoupler whose output is
safe for 3.3 V GPIO.

Motion owns the light for 30 seconds by default. Every later motion webhook
restarts the countdown. A wall-switch edge or IotCloud `setState` command is a
manual action: manual ON stays on indefinitely, while manual OFF cancels the
active countdown. The next motion event may turn the light on again.

## Hardware

| NodeMCU pin | Purpose |
| --- | --- |
| D1 / GPIO5 | Light relay control; active-HIGH by default |
| D2 / GPIO4 | 3.3 V optocoupler output; either edge toggles the light |
| D3 / GPIO0 | IotCloud reset/provisioning button |
| D4 / GPIO2 | IotCloud status LED |
| D5 / GPIO14 | Reserved for future DFPlayer RX |
| D6 / GPIO12 | Reserved for future DFPlayer TX |

The relay starts OFF after every reboot. The first D2 reading becomes the wall
switch baseline and does not toggle the relay, so the rocker may need one flip
after a restart. D5 and D6 are not configured or driven in firmware v1.

The optocoupler GPIO output must never exceed 3.3 V. The relay contacts, power
supply, fuse/protection, enclosure, conductor sizing, and installation must be
rated for the mains voltage and the light's startup/inrush current. Keep the
mains and SELV sides physically separated and use a qualified electrician for
the mains installation.

## Build and provision

The webhook token is required at build time, must contain 32-128 URL-safe
characters, and is never stored in this repository. Generate and export one:

```sh
export UNIFI_WEBHOOK_TOKEN="$(openssl rand -hex 24)"
pio run -e nodemcuv2
```

For a relay board that switches ON with a LOW GPIO level, build the active-LOW
environment instead:

```sh
pio run -e nodemcuv2_active_low
```

Flash and provision the device using the usual IotCloud flow. Reserve its IP
address in DHCP so the Protect console always has a stable webhook target.

## UniFi Protect configuration

In Protect's Alarm Manager:

1. Create an alarm with **Activity > Motion** as its trigger.
2. Scope it to the intended G6 camera and configure any desired schedule.
3. Add a **Webhook > Custom Webhook** action using HTTP GET.
4. Set the URL to `http://DEVICE_IP/webhooks/unifi/motion`.
5. Add the header `X-Webhook-Token` with the same value used for the build.
6. Disable **Ignore Repeated Actions**, or configure it so it does not suppress
   motion events needed to extend the light timer.

The unauthenticated `GET /healthz` endpoint returns `200 ok` for reachability
checks. A valid motion request returns `204`; a missing or incorrect token
returns `401` without changing the relay.

Example local checks:

```sh
curl -i http://DEVICE_IP/healthz
curl -i -H "X-Webhook-Token: $UNIFI_WEBHOOK_TOKEN" \
  http://DEVICE_IP/webhooks/unifi/motion
```

The webhook uses local HTTP rather than TLS, so keep it on a trusted LAN or a
restricted IoT VLAN. If VLANs are routed, allow only the Protect console to
reach this device on TCP port 80.

## IotCloud behavior

The firmware registers a switch sensor with ID `MotionLight`. Its normal
`setState` endpoint provides manual ON/OFF control. The
`aux/motionTimeoutSeconds` setting accepts values from 10 through 1800 seconds.
A new value applies to the next motion webhook; an already-running countdown
keeps the duration it captured when it started.

The webhook server follows Wi-Fi station connectivity rather than MQTT
connectivity, so local motion control remains available during an MQTT outage.
It is not started while the device is in IotCloud provisioning/AP mode.

## Future audio

`trigger_motion_feedback()` is currently a no-op. D5 and D6 are reserved so a
future implementation can connect a DFPlayer Mini, select an MP3 file from its
microSD card, and drive a small speaker without changing the motion/light state
machine.

## Tests

Run the host-side state-machine tests with:

```sh
pio test -e native
```

