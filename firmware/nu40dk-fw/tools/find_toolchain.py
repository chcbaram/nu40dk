#!/usr/bin/env python3
"""
설치된 nRF Connect SDK 툴체인에서 디버깅에 필요한 경로를 찾는다.

  python3 tools/find_toolchain.py                  경로 조회만
  python3 tools/find_toolchain.py --apply          launch.json 의 현재 OS 블록을 갱신
  python3 tools/find_toolchain.py --apply v2.9.0   다른 SDK 버전으로 갱신

툴체인 위치는 아래 순서로 찾는다.
  1. NCS_INSTALL_DIR 환경변수
  2. OS 별 기본 설치 경로
"""

import json
import os
import platform
import re
import sys


DEFAULT_VERSION = "v3.3.0"
DEFAULT_CONFIG  = "Debug with pyOCD (NU-DAP)"

CANDIDATES = {
    "Darwin":  ["/opt/nordic/ncs", os.path.expanduser("~/ncs")],
    "Linux":   [os.path.expanduser("~/ncs"), "/opt/nordic/ncs"],
    "Windows": [r"C:\ncs", os.path.expandvars(r"%LOCALAPPDATA%\Programs\ncs")],
}

OS_KEY = {"Darwin": "osx", "Linux": "linux", "Windows": "windows"}


def find_ncs_root():
    env = os.environ.get("NCS_INSTALL_DIR")
    if env and os.path.isdir(env):
        return env

    for path in CANDIDATES.get(platform.system(), []):
        if os.path.isdir(os.path.join(path, "toolchains")):
            return path
    return None


def find_bundle(ncs_root, version):
    with open(os.path.join(ncs_root, "toolchains", "toolchains.json")) as f:
        data = json.load(f)

    for entry in data[0]["toolchains"]:
        if version in entry["ncs_versions"]:
            return entry["identifier"]["bundle_id"]
    return None


def get_paths(version):
    ncs_root = find_ncs_root()
    if ncs_root is None:
        sys.exit("NCS 설치 경로를 찾지 못했습니다. NCS_INSTALL_DIR 를 지정하세요.")

    bundle = find_bundle(ncs_root, version)
    if bundle is None:
        sys.exit(f"{version} 용 툴체인이 설치되어 있지 않습니다.")

    exe       = ".exe" if platform.system() == "Windows" else ""
    toolchain = os.path.join(ncs_root, "toolchains", bundle)
    sdk       = os.path.join(ncs_root, version)

    svd = os.path.join(sdk, "modules", "hal", "nordic", "nrfx", "bsp", "stable",
                       "mdk", "nrf52840.svd")
    if not os.path.exists(svd):
        # NCS 3.2 이전 경로
        svd = os.path.join(sdk, "modules", "hal", "nordic", "nrfx", "mdk", "nrf52840.svd")

    return {
        "sdk":        sdk,
        "toolchain":  toolchain,
        "bundle":     bundle,
        "serverpath": os.path.join(toolchain, "bin", "pyocd" + exe).replace("\\", "/"),
        "gdbPath":    os.path.join(toolchain, "opt", "zephyr-sdk", "arm-zephyr-eabi",
                                   "bin", "arm-zephyr-eabi-gdb" + exe).replace("\\", "/"),
        "svd":        svd.replace("\\", "/"),
    }


def find_config_span(text, name):
    """launch.json 에서 지정한 이름을 가진 설정 객체의 { } 범위를 찾는다."""
    marker = text.find(f'"{name}"')
    if marker < 0:
        return None

    depth, start = 0, None
    for i in range(marker, -1, -1):
        if text[i] == "}":
            depth += 1
        elif text[i] == "{":
            if depth == 0:
                start = i
                break
            depth -= 1
    if start is None:
        return None

    depth, end = 0, None
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                end = i + 1
                break
    return (start, end) if end else None


def patch_launch_json(path, config_name, paths):
    with open(path, encoding="utf-8") as f:
        text = original = f.read()

    span = find_config_span(text, config_name)
    if span is None:
        sys.exit(f'launch.json 에서 "{config_name}" 설정을 찾지 못했습니다.')

    start, end = span
    block = text[start:end]
    os_key = OS_KEY[platform.system()]

    # 현재 OS 블록: 중첩 중괄호가 없으므로 단순 매칭으로 충분하다.
    m = re.search(r'"%s"\s*:\s*\{[^{}]*\}' % os_key, block)
    if m:
        section = m.group(0)
        for key in ("serverpath", "gdbPath"):
            section, n = re.subn(r'("%s"\s*:\s*")[^"]*(")' % key,
                                 lambda mm: mm.group(1) + paths[key] + mm.group(2),
                                 section)
            if n == 0:   # 키가 없으면 블록 끝에 추가
                section = re.sub(r'\}\s*$',
                                 f',\n\t\t\t\t"{key}": "{paths[key]}"\n\t\t\t}}',
                                 section)
        block = block[:m.start()] + section + block[m.end():]
    else:
        # OS 블록이 아예 없으면 설정 끝에 새로 만든다.
        new = (f'\t\t\t"{os_key}": {{\n'
               f'\t\t\t\t"serverpath": "{paths["serverpath"]}",\n'
               f'\t\t\t\t"gdbPath": "{paths["gdbPath"]}"\n'
               f'\t\t\t}},\n')
        block = re.sub(r'\n\s*\}$', "\n" + new + "\t\t}", block)

    text = text[:start] + block + text[end:]
    if text == original:
        print("변경 사항 없음 (이미 최신 경로)")
        return False

    with open(path, "w", encoding="utf-8") as f:
        f.write(text)
    print(f"갱신함: {path}  [{os_key}]")
    print(f'  serverpath = {paths["serverpath"]}')
    print(f'  gdbPath    = {paths["gdbPath"]}')
    return True


def main():
    args    = [a for a in sys.argv[1:] if not a.startswith("-")]
    apply   = "--apply" in sys.argv
    version = args[0] if args else DEFAULT_VERSION

    paths = get_paths(version)

    print(f"NCS      : {paths['sdk']}")
    print(f"툴체인   : {paths['toolchain']}  (bundle {paths['bundle']})")
    for name, key in (("pyocd", "serverpath"), ("gdb", "gdbPath"), ("svd", "svd")):
        mark = "" if os.path.exists(paths[key]) else "  <-- 없음"
        print(f"  {name:6s} {paths[key]}{mark}")
    print()

    launch = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                          "..", ".vscode", "launch.json")
    launch = os.path.normpath(launch)

    if apply:
        patch_launch_json(launch, DEFAULT_CONFIG, paths)
    else:
        print("launch.json 에 반영하려면: python3 tools/find_toolchain.py --apply")


if __name__ == "__main__":
    main()
