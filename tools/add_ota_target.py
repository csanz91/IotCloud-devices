import os
import re
from glob import glob

# --- FIX 1: Make VS Code Happy ---
# This trick creates a fake "env" so your editor stops complaining.
# When PlatformIO runs this, 'Import' exists and overwrites it.
try:
    Import("env")
except NameError:
    env = {}  # Dummy for linting

    def Import(*args):
        pass

# --- FIX 2: Safely Read Variables ---
# We use GetProjectOption to read the variable from platformio.ini
try:
    # This reads 'device_name = ...' from your .ini file
    device_name = env.GetProjectOption("device_name")
except Exception:
    device_name = "Unknown_Device"


def _maybe_inject_build_defines():
    # Make model/version available to the firmware at compile time.
    # This lets C++ use IOTCLOUD_MODEL_NUM / IOTCLOUD_FW_VERSION without duplicating
    # values in build_flags across projects.
    try:
        model_num = env.GetProjectOption("model_num")
        version = env.GetProjectOption("version")
    except Exception:
        return

    model_num = str(model_num).strip() if model_num is not None else ""
    version = str(version).strip() if version is not None else ""
    if not model_num or not version:
        return

    # GCC needs quotes for string macros: -DNAME=\"value\"
    model_define = f'\\"{model_num}\\"'
    version_define = f'\\"{version}\\"'

    try:
        env.Append(CPPDEFINES=[("IOTCLOUD_MODEL_NUM", model_define), ("IOTCLOUD_FW_VERSION", version_define)])
    except Exception:
        # If we're not running under PlatformIO/SCons, do nothing.
        pass


_maybe_inject_build_defines()


def _get_first_project_option(*keys):
    for key in keys:
        try:
            value = env.GetProjectOption(key)
        except Exception:
            value = None
        if value is not None:
            value = str(value).strip()
            if value:
                return value
    return None


def _slugify(value: str) -> str:
    value = value.strip()
    value = value.replace(" ", "_")
    value = re.sub(r"[^A-Za-z0-9_\-]", "", value)
    return value


def _read_text_file(path: str) -> str:
    try:
        with open(path, "r", encoding="utf-8", errors="ignore") as f:
            return f.read()
    except Exception:
        return ""


def _extract_c_string_macro(text: str, name: str):
    # Matches either:
    #   #define NAME "value"
    #   const char* NAME = "value";
    #   constexpr const char* NAME = "value";
    patterns = [
        rf"^\s*#\s*define\s+{re.escape(name)}\s+\"([^\"]+)\"\s*$",
        rf"^\s*(?:constexpr\s+)?(?:const\s+)?char\s*\*\s+{re.escape(name)}\s*=\s*\"([^\"]+)\"\s*;\s*$",
    ]
    for pat in patterns:
        m = re.search(pat, text, flags=re.MULTILINE)
        if m:
            return m.group(1).strip()
    return None


def _extract_device_ctor(text: str):
    # Matches: Device my_device = Device("v1.0.2", "Despacho");
    m = re.search(
        r"Device\s+\w+\s*=\s*Device\(\s*\"([^\"]+)\"\s*,\s*\"([^\"]+)\"\s*\)\s*;",
        text,
    )
    if not m:
        return None
    return {"version": m.group(1).strip(), "name": m.group(2).strip()}


def _discover_model_and_version(project_dir: str, src_dir: str):
    # 1) Prefer explicit platformio.ini options
    model_num = _get_first_project_option("ota_model_num", "model_num")
    version = _get_first_project_option("ota_version", "fw_version", "version")

    # 2) Fallback: parse source/header defines
    candidates = []
    candidates.append(os.path.join(project_dir, "include", "device_meta.h"))
    candidates.extend(sorted(glob(os.path.join(src_dir, "*.cpp"))))
    candidates.extend(sorted(glob(os.path.join(src_dir, "*.ino"))))
    candidates.extend(sorted(glob(os.path.join(project_dir, "include", "*.h"))))

    parsed_device_name = None
    parsed_version = None
    parsed_model = None

    for path in candidates:
        text = _read_text_file(path)
        if not text:
            continue

        if parsed_model is None:
            parsed_model = _extract_c_string_macro(text, "IOTCLOUD_MODEL_NUM")
        if parsed_version is None:
            parsed_version = _extract_c_string_macro(text, "IOTCLOUD_FW_VERSION")

        ctor = _extract_device_ctor(text)
        if ctor:
            parsed_version = parsed_version or ctor.get("version")
            parsed_device_name = parsed_device_name or ctor.get("name")

    model_num = model_num or parsed_model
    version = version or parsed_version

    # Last resort for model: derive from configured device_name / Device("..", "Name")
    if not model_num:
        fallback_name = parsed_device_name or device_name
        model_num = _slugify(fallback_name) if fallback_name else None

    if model_num:
        model_num = _slugify(model_num)

    return model_num, version


def run_remote_upload(*args, **kwargs):
    print(f"Executing Remote Upload for: {device_name}")

    project_dir = env.subst("$PROJECT_DIR")
    src_dir = env.subst("$PROJECT_SRC_DIR")
    repo_dir = os.path.abspath(os.path.join(project_dir, ".."))

    # ESP8266 builds reliably produce firmware.bin (PROGNAME can vary and cause
    # the pre-target dependency check to fail if we use ${PROGNAME}.bin).
    firmware_path = str(env.subst("$BUILD_DIR/firmware.bin"))
    uploader_script = os.path.abspath(os.path.join(repo_dir, "tools", "upload_ota.sh"))

    model_num, version = _discover_model_and_version(project_dir, src_dir)
    if not model_num or not version:
        print("ERROR: Missing OTA metadata.")
        print("- Provide in platformio.ini: model_num = ... and version = ... (or ota_model_num / ota_version)")
        print("- OR define in code: #define IOTCLOUD_MODEL_NUM \"...\" and #define IOTCLOUD_FW_VERSION \"vX.Y.Z\"")
        raise SystemExit(2)

    cmd = f'bash "{uploader_script}" "{firmware_path}" "{model_num}" "{version}"'
    env.Execute(cmd)


# Register the custom target
env.AddCustomTarget(
    "upload_remote",
    "$BUILD_DIR/firmware.bin",
    run_remote_upload,
    title="Upload Remote",
    description="Upload firmware via OTA Custom Protocol",
)
