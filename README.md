# nu40dk

nRF52840 기반 커스텀 보드(NU40DK)와 Zephyr/NCS 펌웨어 프로젝트.

## 저장소 구조

```
nu40dk/
├── hardware/
│   ├── NU40-DK-V2-Basic.pdf          회로도 V2 (최신, 6페이지)
│   └── NU40DK_Schematic.pdf          회로도 V1 (초기 버전)
└── firmware/
    └── nu40dk-fw/                    Zephyr 애플리케이션 (freestanding)
        ├── CMakeLists.txt
        ├── prj.conf
        ├── boards/nucode/nu40dk/     아웃오브트리 보드 정의
        └── src/
            ├── main.c                hwInit() → apInit() → apMain()
            ├── ap/                   애플리케이션 로직
            ├── bsp/                  Zephyr API 래퍼 (delay, millis 등)
            ├── hw/                   하드웨어 계층 + driver/
            └── common/               공용 코드 (core, hw/include)
```

애플리케이션은 NCS 워크스페이스 밖에 있는 **freestanding 애플리케이션**이다. west 워크스페이스(`west.yml`)를 별도로 두지 않고 설치된 NCS를 그대로 참조한다.

## 하드웨어

MCU는 nRF52840 (QIAA, Cortex-M4F, 1 MB Flash / 256 KB RAM).

| 회로도 | 파일 | 비고 |
|---|---|---|
| **V2 (최신)** | `hardware/NU40-DK-V2-Basic.pdf` | 6페이지 |
| V1 | `hardware/NU40DK_Schematic.pdf` | 초기 버전 |

보드 정의(`nu40dk.dts`)의 핀 배치는 V1 기준으로 작성되어 있다.

## 개발 환경

| 항목 | 버전 / 경로 |
|---|---|
| nRF Connect SDK | **v3.3.0** (`/opt/nordic/ncs/v3.3.0`) |
| Zephyr | 4.3.99 (ncs-v3.3.0) |
| 툴체인 번들 | v3.3.0 용 bundle id **`0c0f19d91c`** (`/opt/nordic/ncs/toolchains/0c0f19d91c`) |
| 툴체인 번들 빌드 | nrfutil-toolchain-bundler 0.21.0 / 2026-02-12 / `aarch64-apple-darwin` |
| Zephyr SDK | 0.17.0 (arm-zephyr-eabi, GCC / binutils 2.38) |
| west | 1.5.0 |
| IDE | VS Code + nRF Connect for VS Code 확장 |
| 검증 호스트 | macOS 12.7.6 (Darwin 21.6, Apple Silicon) |

보드 타겟은 `nu40dk/nrf52840`, `BOARD_ROOT`는 애플리케이션 디렉터리(`firmware/nu40dk-fw`)다.

NCS 버전과 툴체인 번들 id의 대응은 `/opt/nordic/ncs/toolchains/toolchains.json`, 번들 상세는 `<번들경로>/manifest.json` 에서 확인한다.

### 설치

1. VS Code에 **nRF Connect for VS Code Extension Pack** 설치
2. 확장의 Toolchain Manager에서 **nRF Connect SDK v3.3.0** 설치 (툴체인이 함께 설치됨)
3. `firmware/nu40dk-fw` 를 애플리케이션으로 추가하고 빌드 설정 생성
   - Board target: `nu40dk/nrf52840`
   - Board roots: `firmware/nu40dk-fw`

## 빌드

### VS Code

nRF Connect 확장의 Application → Build 사용. 빌드 설정은 `firmware/nu40dk-fw/build/` 에 생성된다.

### 커맨드라인

freestanding 애플리케이션이므로 `west build` 는 NCS 워크스페이스 루트에서 실행해야 한다.

```bash
export NCS=/opt/nordic/ncs/v3.3.0
export TC=/opt/nordic/ncs/toolchains/0c0f19d91c
export APP=~/hdd/git/nu40dk/firmware/nu40dk-fw

export PATH="$TC/bin:$TC/usr/bin:$TC/usr/local/bin:$TC/opt/bin:$TC/opt/zephyr-sdk/arm-zephyr-eabi/bin:$PATH"
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=$TC/opt/zephyr-sdk
export NRFUTIL_HOME=$TC/nrfutil/home

cd $NCS
west build -d $APP/build $APP --board nu40dk/nrf52840 -- -DBOARD_ROOT=$APP
```

전체 재빌드는 `--pristine` 추가.

산출물:

- `build/nu40dk-fw/zephyr/zephyr.elf` / `zephyr.hex`
- `build/merged.hex`

## 플래시 / 디버그

`boards/nucode/nu40dk/board.cmake` 에 등록된 러너: `nrfjprog`, `nrfutil`, `jlink`, `pyocd`, `openocd`.

```bash
west flash -d $APP/build -r pyocd     # 또는 jlink / openocd / nrfjprog
```

VS Code 디버깅은 `firmware/nu40dk-fw/.vscode/launch.json` 의 **Debug with OpenOCD** 설정을 사용한다 (cortex-debug + OpenOCD, 인터페이스 `stlink.cfg`, 타겟 `nrf52.cfg`).

## 보드 정의 (`boards/nucode/nu40dk/`)

| 파일 | 역할 |
|---|---|
| `board.yml` | 보드 메타데이터 (`name`, `full_name`, vendor, SoC) |
| `board.cmake` | 플래시/디버그 러너 설정 |
| `Kconfig.nu40dk` | `BOARD_NU40DK` → `SOC_NRF52840_QIAA` |
| `Kconfig.defconfig` | 보드 기본 Kconfig 값 |
| `nu40dk_defconfig` | `CONFIG_ARM_MPU`, `CONFIG_HW_STACK_PROTECTION` |
| `nu40dk.dts` | 디바이스트리 (LED, 파티션, gpio/gpiote) |
| `nu40dk-pinctrl.dtsi` | pinctrl (현재 비어 있음) |
| `nu40dk.yml` | twister 보드 식별 정보 |

### 플래시 파티션 (`nu40dk.dts`)

| 파티션 | 주소 | 크기 |
|---|---|---|
| `mcuboot` | 0x00000000 | 48 KB |
| `image-0` (slot0, 코드 파티션) | 0x0000C000 | 472 KB |
| `image-1` (slot1) | 0x00082000 | 472 KB |
| `storage` | 0x000F8000 | 32 KB |

## 라이선스

MIT License. `LICENSE` 참조.
