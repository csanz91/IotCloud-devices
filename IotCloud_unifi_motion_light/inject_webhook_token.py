import os
import re

Import("env")

token = os.environ.get("UNIFI_WEBHOOK_TOKEN", "")

if not 32 <= len(token) <= 128:
    print("ERROR: UNIFI_WEBHOOK_TOKEN must contain between 32 and 128 characters.")
    raise SystemExit(2)

if not re.fullmatch(r"[A-Za-z0-9_-]+", token):
    print("ERROR: UNIFI_WEBHOOK_TOKEN may only contain letters, digits, '_' and '-'.")
    raise SystemExit(2)

# The token is injected at build time and is never stored in the repository.
env.Append(CPPDEFINES=[("UNIFI_WEBHOOK_TOKEN", f'\\"{token}\\"')])

