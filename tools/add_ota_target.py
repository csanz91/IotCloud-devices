import os
import re
import gzip
import importlib.util
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


def _ota_compression_enabled() -> bool:
    value = _get_first_project_option("custom_ota_compress")
    return value is not None and value.lower() in ("1", "true", "yes", "on")


def _firmware_for_upload(firmware_path: str) -> str:
    if not _ota_compression_enabled():
        return firmware_path

    compressed_path = firmware_path + ".gz"
    with open(firmware_path, "rb") as source:
        compressed = gzip.compress(source.read(), compresslevel=9, mtime=0)
    with open(compressed_path, "wb") as target:
        target.write(compressed)
    print(f"Compressed OTA image: {len(compressed)} bytes ({compressed_path})")
    return compressed_path


def _load_size_check_module():
    # SCons runs this file without __file__, so find tools/ from the project.
    tools_dir = os.path.abspath(os.path.join(env.subst("$PROJECT_DIR"), "..", "tools"))
    spec = importlib.util.spec_from_file_location(
        "ota_size_check", os.path.join(tools_dir, "ota_size_check.py")
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _find_esp8266_ldscript():
    name = env.subst("$LDSCRIPT_PATH")
    if not name:
        return None
    if os.path.isabs(name) and os.path.isfile(name):
        return name
    search_dirs = [env.subst(str(d)) for d in env.get("LIBPATH", [])]
    try:
        framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif8266")
        search_dirs.append(os.path.join(framework_dir, "tools", "sdk", "ld"))
    except Exception:
        pass
    for directory in search_dirs:
        path = os.path.join(directory, os.path.basename(name))
        if os.path.isfile(path):
            return path
    return None


def _check_ota_size(image_path: str, upload_size: int):
    """Returns (SizeCheck, None), or (None, reason) when the size can't be checked."""
    size_check = _load_size_check_module()
    image_size = os.path.getsize(image_path)
    platform = env.subst("$PIOPLATFORM")
    try:
        if platform == "espressif32":
            csv_path = env.subst("$PARTITIONS_TABLE_CSV")
            if not csv_path or not os.path.isfile(csv_path):
                return None, "partition table not found"
            return size_check.check_esp32(image_size, _read_text_file(csv_path)), None
        if platform == "espressif8266":
            ld_path = _find_esp8266_ldscript()
            if not ld_path:
                return None, "linker script not found"
            return size_check.check_esp8266(image_size, upload_size, _read_text_file(ld_path)), None
    except ValueError as error:
        return None, str(error)
    return None, f"no OTA size rule for platform {platform!r}"


def _gzip_size(path: str) -> int:
    with open(path, "rb") as source:
        return len(gzip.compress(source.read(), compresslevel=9, mtime=0))


def _report_ota_size(target, source, env):
    # After every build: show the real OTA margin. Never fails the build.
    image_path = str(target[0])
    upload_size = _gzip_size(image_path) if _ota_compression_enabled() else os.path.getsize(image_path)
    result, reason = _check_ota_size(image_path, upload_size)
    print(result.message() if result else f"OTA size: not checked ({reason})")


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
    image_path = str(env.subst("$BUILD_DIR/firmware.bin"))
    firmware_path = _firmware_for_upload(image_path)

    result, reason = _check_ota_size(image_path, os.path.getsize(firmware_path))
    if result is None:
        print(f"WARNING: OTA size not checked ({reason}).")
    else:
        print(result.message())
        if not result.fits:
            # The ESP8266 estimate can be wrong when the device runs a smaller
            # build; `custom_ota_size_check = warn` lets such an upload through.
            if (_get_first_project_option("custom_ota_size_check") or "").lower() == "warn":
                print("WARNING: image may not fit; uploading anyway (custom_ota_size_check = warn).")
            else:
                print("ERROR: the device can't accept this image. Not uploading.")
                print("If the device runs a smaller build, set custom_ota_size_check = warn.")
                raise SystemExit(2)

    uploader_script = os.path.abspath(os.path.join(repo_dir, "tools", "upload_ota.sh"))

    model_num, version = _discover_model_and_version(project_dir, src_dir)
    if not model_num or not version:
        print("ERROR: Missing OTA metadata.")
        print("- Provide in platformio.ini: model_num = ... and version = ... (or ota_model_num / ota_version)")
        print("- OR define in code: #define IOTCLOUD_MODEL_NUM \"...\" and #define IOTCLOUD_FW_VERSION \"vX.Y.Z\"")
        raise SystemExit(2)

    cmd = f'bash "{uploader_script}" "{firmware_path}" "{model_num}" "{version}"'

    # OTA_TARGET=canary|early|<id>[,<id>...] makes it a staged rollout
    # (see tools/deploy_ota.sh). Checked here because it goes into a shell command.
    target = os.environ.get("OTA_TARGET", "").strip()
    if target:
        if not re.fullmatch(r"canary|early|[A-Za-z0-9_-]{1,50}(,[A-Za-z0-9_-]{1,50})*", target):
            print("ERROR: OTA_TARGET must be 'canary', 'early' or comma-separated device IDs.")
            raise SystemExit(2)
        cmd += f' "{target}"'
    else:
        print("Deploying to every device of the model (set OTA_TARGET for a staged rollout).")

    env.Execute(cmd)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _report_ota_size)

# Register the custom target
env.AddCustomTarget(
    "upload_remote",
    "$BUILD_DIR/firmware.bin",
    run_remote_upload,
    title="Upload Remote",
    description="Upload firmware and deploy it (OTA_TARGET=canary|early|<ids> for a staged rollout)",
)
