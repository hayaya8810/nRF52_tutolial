# Linux Mint + nRF52-DK + nRF Connect SDK 開発環境 構築手順

[TOC]

## 目的

Linux Mint 上で Nordic nRF52-DK を使い、このリポジトリの Zephyr/NCS アプリケーションを開発・ビルド・書き込みできる環境を作る。

この手順書では、以下を対象にする。

- OS: Linux Mint
- SDK: nRF Connect SDK
- NCS version: `v3.3.0`
- ボード: nRF52-DK
- SoC: nRF52832
- Zephyr board target: `nrf52dk/nrf52832`
- ビルドツール: `west`
- アプリケーション: `blinky`

このプロジェクトは Zephyr を直接使うだけでなく、Nordic の `nrf` モジュールに含まれる Nordic UART Service, NUS を使う。  
そのため、開発環境は upstream Zephyr 単体ではなく、Zephyr を含んだ Nordic 公式 SDK である **nRF Connect SDK, NCS** を前提にする。

---

## 0. このPCで確認した環境

この手順書は、次の環境で確認した。

| 項目 | バージョン / パス |
| --- | --- |
| NCS | `3.3.0` |
| NCS workspace | `/home/my_home/ncs/v3.3.0` |
| Zephyr | `/home/my_home/ncs/v3.3.0/zephyr` |
| west | `1.5.0` |
| Python | `3.12.3` |
| CMake | `3.28.1` |
| Ninja | `1.11.1` |
| Devicetree compiler | `DTC 1.7.0` |
| Zephyr SDK | `0.17.0` |
| ARM GCC | `arm-zephyr-eabi-gcc 12.2.0` |
| nrfjprog | `10.24.2 external` |
| J-Link | `JLinkARM.dll 9.40` |

確認コマンド:

```bash
west topdir
west --version
python3 --version
cmake --version
ninja --version
dtc --version
nrfjprog --version
```

付記:

- NCS に付随して入る Zephyr SDK は、この環境では `0.17.0` で動作確認済み。
- NCS に付随して入る ARM GCC は、この環境では `12.2.0` で動作確認済み。
- NCS に付随して入る Python は、この環境では `3.12.3` で動作確認済み。
- `west` は PATH 上では `/home/my_home/.local/bin/west` が使われている。
- `nrfutil` は NCS toolchain 内にあるが、通常ターミナルの PATH には出ていない場合がある。その場合は `west flash -r nrfjprog` を使う。

---

## 1. NCS と Zephyr の関係

NCS は nRF Connect SDK の略で、Nordic Semiconductor が提供する nRF シリーズ向け SDK。

NCS の中には Zephyr が含まれている。

```text
/home/my_home/ncs/v3.3.0/
├── zephyr/    # Zephyr 本体
├── nrf/       # Nordic 追加モジュール
├── modules/   # HAL や外部モジュール
└── ...
```

このプロジェクトでは [prj.conf](../prj.conf) で以下を有効にしている。

```conf
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_NUS=y
```

`CONFIG_BT_NUS` は Nordic UART Service を使う設定なので、NCS 前提にする。

---

## 2. OS パッケージを入れる

Linux 側に最低限必要なパッケージを入れる。

```bash
sudo apt update
sudo apt upgrade

sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget python3-dev python3-venv python3-tk \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1 \
  minicom
```

確認:

```bash
cmake --version
python3 --version
ninja --version
dtc --version
```

このPCでは以下で確認済み。

```text
CMake 3.28.1
Python 3.12.3
Ninja 1.11.1
DTC 1.7.0
```

---

## 3. NCS をインストールする

このPCでは NCS を以下に置いている。

```text
/home/my_home/ncs/v3.3.0
```

新しく環境を作る場合は、Nordic の nRF Connect SDK `v3.3.0` と、その toolchain をインストールする。  
インストール後、次のような構成になっていればよい。

```text
/home/<ユーザー名>/ncs/v3.3.0/
├── zephyr/
├── nrf/
├── modules/
└── ...

/home/<ユーザー名>/ncs/toolchains/<toolchain-id>/
├── opt/zephyr-sdk/
├── nrfutil/
└── ...
```

このPCの toolchain は以下にある。

```text
/home/my_home/ncs/toolchains/911f4c5c26
```

付記:

- この環境では Zephyr SDK `0.17.0` が NCS toolchain に含まれている。
- 手動で別途 `~/zephyr-sdk-*` を入れる構成ではない。
- upstream Zephyr 用の `~/zephyrproject` は、このプロジェクトでは前提にしない。

---

## 4. west workspace を確認する

このプロジェクトのビルドでは、west workspace が NCS を指している必要がある。

```bash
cd /home/my_home/workspace/blinky
west topdir
```

期待値:

```text
/home/my_home/ncs/v3.3.0
```

west の設定も確認する。

```bash
west config --list
```

このPCでは以下の設定。

```text
manifest.path=nrf
manifest.file=west.yml
zephyr.base=zephyr
```

---

## 5. このプロジェクトを取得する

このPCではアプリケーションリポジトリを以下に置いている。

```text
/home/my_home/workspace/blinky
```

新しく clone する場合の例:

```bash
mkdir -p ~/workspace
cd ~/workspace
git clone <このリポジトリのURL> blinky
cd blinky
```

GitHub への clone で次のようなエラーが出る場合がある。

```console
remote: Invalid username or token. Password authentication is not supported for Git operations.
fatal: Authentication failed
```

GitHub は HTTPS のパスワード認証をサポートしていないため、以下のどちらかを使う。

- HTTPS + Personal Access Token
- SSH key

---

## 6. プロジェクト構成

このリポジトリは Zephyr の application としてビルドする。

```text
blinky/
├── CMakeLists.txt
├── prj.conf
├── sample.yaml
├── boards/
└── src/
    ├── main.c
    ├── button.c
    ├── button.h
    ├── led.c
    ├── led.h
    ├── nus.c
    └── nus.h
```

主な役割:

| ファイル | 役割 |
|---|---|
| `src/main.c` | 初期化、ボタンコールバック登録、メインループ |
| `src/button.c/h` | nRF52-DK の 4 ボタン入力、GPIO 割り込み、デバウンス |
| `src/led.c/h` | nRF52-DK の 4 LED 制御、ON/OFF/Toggle/Blink |
| `src/nus.c/h` | BLE Peripheral、Nordic UART Service、LED 状態通知 |
| `prj.conf` | GPIO、Bluetooth、NUS の Kconfig 設定 |
| `CMakeLists.txt` | `main.c`, `button.c`, `led.c`, `nus.c` をビルド対象にする |

現在の `prj.conf`:

```conf
CONFIG_GPIO=y

CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_DEVICE_NAME="Blinky_NUS"
CONFIG_BT_MAX_CONN=1
CONFIG_BT_NUS=y
```

---

## 7. アプリケーションの動作

このアプリケーションは nRF52-DK 上で次の動作をする。

- Button 1: LED 1 の 1000ms 周期点滅開始/停止
- Button 2: LED 2 の 500ms 周期点滅開始/停止
- Button 3: LED 3 の 250ms 周期点滅開始/停止
- Button 4: LED 4 の ON/OFF トグル
- BLE Peripheral として `Blinky_NUS` という名前で Advertising する
- NUS notification が有効な Central に LED 状態を送る

LED 点滅は `k_work_delayable` を使って `src/led.c` 内に閉じ込めている。  
`main.c` は `led_blink_toggle()` や `led_toggle()` を呼ぶだけにしている。

LED 状態は bit mask として扱う。

| bit | 対応 |
|---|---|
| bit0 | LED 1 |
| bit1 | LED 2 |
| bit2 | LED 3 |
| bit3 | LED 4 |

例:

```text
0x00 = 全部OFF
0x01 = LED1 ON
0x02 = LED2 ON
0x04 = LED3 ON
0x08 = LED4 ON
0x0F = LED1〜LED4 全部ON
```

現在の NUS payload は 1 byte 生データではなく、確認しやすいテキスト形式で送っている。

例:

```text
B0 L=1
N3 L=5
```

将来的に Central 側との仕様を固定する段階で、payload を 1 byte の bit mask に変更してもよい。

---

## 8. ビルドする

このプロジェクトのディレクトリで実行する。

```bash
cd /home/my_home/workspace/blinky
west build -b nrf52dk/nrf52832 . -p always
```

成功すると、最後にメモリ使用量が表示される。

このPCではビルド成功を確認済み。

```text
Memory region         Used Size  Region Size  %age Used
           FLASH:      108228 B       512 KB     20.64%
             RAM:       20544 B        64 KB     31.35%
```

付記:

- この環境では NCS `v3.3.0` の sysbuild が使われる。
- ビルド時に Partition Manager deprecated の警告が出るが、このアプリのビルド自体は成功する。

---

## 9. nRF52-DK を接続する

nRF52-DK を USB で PC に接続する。

シリアルポートを確認する。

```bash
ls /dev/ttyACM*
```

例:

```text
/dev/ttyACM0
```

シリアル出力を見る場合:

```bash
minicom -D /dev/ttyACM0 -b 115200
```

終了は `Ctrl-A` のあと `X`。

---

## 10. 書き込む

まず通常の flash を試す。

```bash
cd /home/my_home/workspace/blinky
west flash
```

このPCの build context では default runner は `nrfutil`。

ただし、通常ターミナルで `nrfutil` が PATH にない場合は、`nrfjprog` runner を明示する。

```bash
west flash -r nrfjprog
```

このPCでは以下が入っている。

```text
nrfjprog 10.24.2 external
JLinkARM.dll 9.40
```

複数の J-Link / nRF52-DK を接続している場合は、デバイスIDを指定する。

```bash
west flash -r nrfjprog --dev-id <J-Link serial number>
```

---

## 11. 書き込みトラブルシュート

### `west flash` で `nrfutil` が見つからない

NCS toolchain 内には `nrfutil` があるが、通常ターミナルの PATH に入っていない場合がある。

対処:

```bash
west flash -r nrfjprog
```

または、NCS toolchain の環境変数を有効にしてから実行する。

このPCでは `nrfutil` は以下にある。

```text
/home/my_home/ncs/toolchains/911f4c5c26/nrfutil/bin/nrfutil
```

### J-Link 権限エラーになる

udev ルールが不足している可能性がある。  
SEGGER J-Link Software または Nordic Command Line Tools のインストール時に udev ルールも入れる。

反映後、udev を再読み込みして USB を抜き差しする。

```bash
sudo udevadm control --reload
sudo udevadm trigger
```

### ボードが複数あって書き込み先が選べない

J-Link serial number を指定する。

```bash
west flash -r nrfjprog --dev-id <J-Link serial number>
```

---

## 12. 動作確認

書き込み後、シリアルログを確認する。

```bash
minicom -D /dev/ttyACM0 -b 115200
```

期待するログ例:

```text
BLE advertising as Blinky_NUS
```

ボタン操作:

- Button 1 を押すと LED 1 が 1000ms 周期で点滅開始/停止する
- Button 2 を押すと LED 2 が 500ms 周期で点滅開始/停止する
- Button 3 を押すと LED 3 が 250ms 周期で点滅開始/停止する
- Button 4 を押すと LED 4 が ON/OFF する

BLE Central から `Blinky_NUS` に接続し、NUS TX characteristic の notification を有効にすると、LED 状態が通知される。

---

## 13. よく使うコマンド

```bash
# NCS workspace の確認
west topdir

# west のバージョン
west --version

# ビルド
cd /home/my_home/workspace/blinky
west build -b nrf52dk/nrf52832 . -p always

# 書き込み
west flash

# nrfjprog runner を使って書き込み
west flash -r nrfjprog

# シリアルログ
minicom -D /dev/ttyACM0 -b 115200

# ボード一覧で nRF52-DK を確認
west boards | rg '^nrf52dk'
```

---

## 14. 開発メモ

このプロジェクトでは、機能ごとに責務を分ける。

- GPIO や割り込みの細部は `button.c` に閉じ込める
- LED の状態管理と点滅処理は `led.c` に閉じ込める
- BLE/NUS の接続管理と通知処理は `nus.c` に閉じ込める
- `main.c` は初期化とアプリの流れに集中させる

今後の変更候補:

1. NUS payload をテキスト形式から 1 byte bit mask に変更する
2. BLE 接続時に現在の LED 状態を即時通知する仕様を明確化する
3. LED 点滅中の状態を「物理出力の現在値」として扱うか、「点滅有効状態」として扱うかを決める
4. Central 側アプリの受信仕様を固定する

---

## 15. Tips

### Kconfig を確認する

Kconfig の設定を画面で確認・一時変更したい場合は `menuconfig` を使う。

```bash
cd /home/my_home/workspace/blinky
west build -t menuconfig
```

GUI で見たい場合は `guiconfig` も使える。

```bash
west build -t guiconfig
```

`guiconfig` には `python3-tk` が必要になる場合がある。  
ここで変更した内容は build ディレクトリ側に反映されるだけなので、恒久的に使う設定は `prj.conf` に書き戻す。

### 最終的な Kconfig を確認する

ビルド後の最終的な Kconfig は `.config` に出力される。

```bash
rg CONFIG_BT_NUS build/blinky/zephyr/.config
rg CONFIG_BT_DEVICE_NAME build/blinky/zephyr/.config
```

`prj.conf` に書いた設定が有効になっているか、別の依存関係で無効になっていないかを確認できる。

### 最終的な Devicetree を確認する

ビルド後の最終的な Devicetree は `zephyr.dts` に出力される。

```bash
rg 'led0|led1|led2|led3' build/blinky/zephyr/zephyr.dts
rg 'sw0|sw1|sw2|sw3' build/blinky/zephyr/zephyr.dts
```

LED や Button の alias が、実際にどの GPIO pin に解決されているかを確認できる。  
GPIO が期待通りに動かない場合は、まずここを見るとよい。

### build ディレクトリを後から掃除する

すでに build 済みのディレクトリを pristine 状態に戻したい場合:

```bash
west build -t pristine
```

普段は次のように `-p always` を付けてクリーンビルドしてもよい。

```bash
west build -b nrf52dk/nrf52832 . -p always
```

### flash runner を確認する

`west flash` がどの runner を使うか、どの runner が使えるかを確認する。

```bash
west flash --context
```

このPCでは default runner は `nrfutil`。  
`nrfutil` が PATH にない場合やうまく動かない場合は、runner を明示する。

```bash
west flash -r nrfjprog
west flash -r jlink
```

### BLE の確認には nRF Connect for Mobile を使う

BLE Central 側の確認には、スマートフォンアプリの nRF Connect for Mobile が便利。

確認手順:

1. nRF52-DK に書き込む
2. nRF Connect for Mobile で scan する
3. `Blinky_NUS` を探して接続する
4. Nordic UART Service の TX characteristic notification を有効にする
5. nRF52-DK のボタンを押して通知を確認する

PC 側の Central アプリを作る前に、Peripheral と NUS notification が動いているかを切り分けられる。

### シリアルログが見えない場合

シリアルポートは必ず `/dev/ttyACM0` になるとは限らない。

```bash
ls /dev/ttyACM*
```

nRF52-DK を抜き差しして、増減した port を確認する。  
複数の USB シリアル機器を接続している場合は、`/dev/ttyACM1` などになることがある。

接続例:

```bash
minicom -D /dev/ttyACM0 -b 115200
```

ログが出ない場合は、baudrate が `115200` になっているか、別の `/dev/ttyACM*` を開いていないかを確認する。

### printf / printk の出力が見えない場合

このプロジェクトでは `printf()` でログを出している。  
シリアルに何も出ない場合は、UART console が有効か、正しい port を開いているかを確認する。

必要になったら `prj.conf` に console や logging の設定を追加する。

例:

```conf
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=y
CONFIG_PRINTK=y
```

### NUS payload の形式を決める

現在の `nus.c` は確認しやすいようにテキスト形式で送っている。

例:

```text
B0 L=1
N3 L=5
```

Central 側との仕様を固定する段階では、以下のどちらにするかを決める。

- デバッグしやすいテキスト形式を続ける
- 通信量が少ない 1 byte bit mask に変更する

1 byte bit mask にする場合の例:

```text
bit0 = LED1
bit1 = LED2
bit2 = LED3
bit3 = LED4
```

---

## 参考情報

- nRF Connect SDK documentation
- nRF52-DK board documentation
- Zephyr documentation
- Nordic Command Line Tools documentation
