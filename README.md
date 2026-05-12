# Blinky NUS

Zephyr OS / nRF Connect SDK Bluetooth LE application.

This application controls 4 buttons and 4 LEDs on a board, and notifies or controls LED states over Nordic UART Service (NUS).

## Features

- Advertises as a BLE Peripheral
- Sends and receives data with Nordic UART Service (NUS)
- Handles 4 button inputs with GPIO interrupts and debouncing
- Controls 4 LEDs with GPIO outputs
- Controls LEDs from button presses or NUS RX commands
- Notifies LED states with NUS TX notifications

## Overview

On startup, the application starts Bluetooth LE advertising. The device name is `Blinky_NUS`.

When a BLE Central connects over NUS and enables notifications, LED states are sent over NUS TX. When a hexadecimal string is sent to NUS RX, the value is interpreted as a bitmask for LED control.

## Button And LED Mapping

| Button | LED | Behavior |
| --- | --- | --- |
| `sw0` | `led0` | Starts or stops blinking at 500 ms ON / 500 ms OFF |
| `sw1` | `led1` | Starts or stops blinking at 250 ms ON / 250 ms OFF |
| `sw2` | `led2` | Starts or stops blinking at 125 ms ON / 125 ms OFF |
| `sw3` | `led3` | Toggles ON / OFF |

Only button press events control LEDs. Button release events do not control LEDs.

## NUS RX Commands

If the first character received on NUS RX is a hexadecimal digit, the received string is interpreted as a hexadecimal bitmask. LEDs whose bits are set to 1 are controlled.

| Example | Target |
| --- | --- |
| `1` | `led0` |
| `2` | `led1` |
| `4` | `led2` |
| `8` | `led3` |
| `F` | All LEDs from `led0` to `led3` |

For example, sending `5` controls `led0` and `led2`.

## NUS TX Notifications

LED states are notified in the following format.

```text
B<led_id> L=<mask>
N<count> L=<mask>
```

- `B<led_id>`: Notification caused by a button operation or LED event
- `N<count>`: State notification, such as on connection or when notifications are enabled
- `L=<mask>`: Hexadecimal bitmask representing the LEDs that are currently ON

Examples:

```text
B0 L=1
N2 L=8
```

## Requirements

This project uses Nordic NUS, so it assumes nRF Connect SDK (NCS), not standalone upstream Zephyr.

See [documents/install.md](documents/install.md) for the verified development environment.

Main requirements:

- nRF Connect SDK `v3.3.0`
- `west`
- CMake / Ninja
- nRF series development board
- devicetree aliases `led0` to `led3` and `sw0` to `sw3` are available

## Build

Build from the repository root with an active NCS west workspace.

Example for nRF52 DK / nRF52832:

```bash
west build -b nrf52dk/nrf52832 .
```

For a pristine build:

```bash
west build -b nrf52dk/nrf52832 . --pristine
```

## Flash

Connect the board and run:

```bash
west flash -r nrfjprog
```

If the runner can be selected automatically in your environment, this may also work:

```bash
west flash
```

## Project Structure

```text
.
├── CMakeLists.txt
├── prj.conf
├── boards/
├── documents/
└── src/
    ├── main.c
    ├── app.c
    ├── button.c
    ├── led.c
    └── nus.c
```

| File | Role |
| --- | --- |
| `src/main.c` | Initialization, callback registration, and event loop |
| `src/app.c` | Application event queue and event handling |
| `src/button.c` | Button GPIO, interrupts, and debouncing |
| `src/led.c` | LED GPIO, ON, OFF, toggle, and blinking |
| `src/nus.c` | BLE initialization, NUS communication, and LED state notifications |
| `prj.conf` | Zephyr settings such as GPIO, BLE Peripheral, and NUS |
| `documents/` | Specification and development environment setup guide |

## Related Documents

- [Specification](documents/specification.md)
- [Development Environment Setup Guide](documents/install.md)

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).

---

# Blinky NUS

Zephyr OS / nRF Connect SDK 向けの Bluetooth LE アプリケーションです。

ボード上の 4 個のボタンで 4 個の LED を操作し、LED の状態を Nordic UART Service (NUS) 経由で通知・制御できます。

## 主な機能

- BLE Peripheral として advertising
- Nordic UART Service (NUS) による送受信
- 4 個のボタン入力を GPIO 割り込みとデバウンスで処理
- 4 個の LED を GPIO 出力で制御
- ボタン押下または NUS RX コマンドで LED を操作
- LED 状態を NUS TX notification で通知

## 動作概要

起動すると Bluetooth LE advertising を開始します。デバイス名は `Blinky_NUS` です。

BLE Central から NUS に接続し、notification を有効にすると、LED 状態が NUS TX で通知されます。NUS RX に 16 進数文字列を送ると、その値を LED 操作用のビットマスクとして扱います。

## ボタンと LED の対応

| ボタン | LED | 動作 |
| --- | --- | --- |
| `sw0` | `led0` | 500 ms ON / 500 ms OFF の点滅を開始または停止 |
| `sw1` | `led1` | 250 ms ON / 250 ms OFF の点滅を開始または停止 |
| `sw2` | `led2` | 125 ms ON / 125 ms OFF の点滅を開始または停止 |
| `sw3` | `led3` | 点灯 / 消灯をトグル |

各ボタンは押下時だけ処理されます。離したときのイベントでは LED 操作は行いません。

## NUS RX コマンド

NUS RX で受信した先頭文字が 16 進数の場合、その文字列を 16 進数のビットマスクとして解釈します。ビットが 1 の LED が操作対象です。

| 送信例 | 操作対象 |
| --- | --- |
| `1` | `led0` |
| `2` | `led1` |
| `4` | `led2` |
| `8` | `led3` |
| `F` | `led0` から `led3` すべて |

たとえば `5` を送信すると、`led0` と `led2` が操作されます。

## NUS TX 通知

LED 状態は次の形式で通知されます。

```text
B<led_id> L=<mask>
N<count> L=<mask>
```

- `B<led_id>`: ボタン操作または LED イベントによる通知
- `N<count>`: 接続時や notification 有効化時などの状態通知
- `L=<mask>`: 現在 ON の LED を表す 16 進数ビットマスク

例:

```text
B0 L=1
N2 L=8
```

## 前提環境

このプロジェクトは Nordic の NUS を使うため、upstream Zephyr 単体ではなく nRF Connect SDK (NCS) を前提にしています。

確認済みの開発環境は [documents/install.md](documents/install.md) を参照してください。

主な前提は次のとおりです。

- nRF Connect SDK `v3.3.0`
- `west`
- CMake / Ninja
- nRF シリーズ開発ボード
- devicetree alias `led0` から `led3`、`sw0` から `sw3` が利用できること

## ビルド

NCS の west workspace が有効な状態で、このリポジトリのルートからビルドします。

例: nRF52 DK / nRF52832 の場合

```bash
west build -b nrf52dk/nrf52832 .
```

クリーンビルドする場合:

```bash
west build -b nrf52dk/nrf52832 . --pristine
```

## 書き込み

ボードを接続して、次を実行します。

```bash
west flash -r nrfjprog
```

環境によって runner が自動選択できる場合は、次でも書き込めます。

```bash
west flash
```

## プロジェクト構成

```text
.
├── CMakeLists.txt
├── prj.conf
├── boards/
├── documents/
└── src/
    ├── main.c
    ├── app.c
    ├── button.c
    ├── led.c
    └── nus.c
```

| ファイル | 役割 |
| --- | --- |
| `src/main.c` | 初期化、コールバック登録、イベントループ |
| `src/app.c` | アプリケーションイベントキューとイベント処理 |
| `src/button.c` | ボタン GPIO、割り込み、デバウンス |
| `src/led.c` | LED GPIO、点灯、消灯、トグル、点滅 |
| `src/nus.c` | BLE 初期化、NUS 通信、LED 状態通知 |
| `prj.conf` | GPIO、BLE Peripheral、NUS などの Zephyr 設定 |
| `documents/` | 仕様書と開発環境構築手順 |

## 関連ドキュメント

- [仕様書](documents/specification.md)
- [開発環境構築手順](documents/install.md)

## ライセンス

このプロジェクトは MIT License で公開しています。詳細は [LICENSE](LICENSE) を参照してください。
