# ESP-Miner Unit Test Coverage Report

## Overview

ESP-Miner includes unit tests that run on both physical ESP32S3 hardware and in QEMU emulation for CI/CD pipelines. This document tracks test coverage and identifies areas for improvement.

## Test Infrastructure

| Item | Location | Description |
|------|----------|-------------|
| Test Framework | Unity (ESP-IDF) | Standard ESP32 unit testing framework |
| CI Workflow | `.github/workflows/unittest.yml` | Runs on every push/PR |
| QEMU Runner | `bitaxeorg/esp32-qemu-test-action` | ESP32S3 emulation for CI |
| CI Test App | `test-ci/` | CI-optimized test binary (watchdogs disabled) |
| Hardware Tests | `test/` | Full test suite for physical device |
| Documentation | `doc/unit_testing.md` | How to build, flash, and add tests |

### Running Tests

**On QEMU (CI):**
```bash
cd test-ci
idf.py build
# Automatically runs via GitHub Actions
```

**On Hardware:**
```bash
cd test
idf.py build
python -m esptool -p /dev/ttyACM0 --chip esp32s3 -b 460800 write_flash ...
idf.py -p /dev/ttyACM0 monitor
```

---

## Component: `stratum`

### `utils.c` - 6/11 functions tested (55%)

| Function | Tested | QEMU | Test File | Notes |
|----------|:------:|:----:|-----------|-------|
| `bin2hex()` | ✅ | ✅ | `test_utils.c` | |
| `hex2bin()` | ✅ | ✅ | `test_utils.c` | |
| `double_sha256_bin()` | ✅ | ✅ | `test_utils.c` | |
| `reverse_32bit_words()` | ✅ | ✅ | `test_utils.c` | |
| `reverse_endianness_per_word()` | ✅ | ✅ | `test_utils.c` | |
| `networkDifficulty()` | ✅ | ✅ | `test_utils.c` | |
| `midstate_sha256_bin()` | ❌ | ✅ | - | **Can add** |
| `le256todouble()` | ❌ | ✅ | - | **Can add** |
| `suffixString()` | ❌ | ✅ | - | **Can add** |
| `hashCounterToGhs()` | ❌ | ✅ | - | **Can add** |
| `print_hex()` / `prettyHex()` | ❌ | - | - | Logging only |

### `mining.c` - 6/7 functions tested (86%)

| Function | Tested | QEMU | Test File | Notes |
|----------|:------:|:----:|-----------|-------|
| `calculate_coinbase_tx_hash()` | ✅ | ✅ | `test_mining.c` | |
| `calculate_merkle_root_hash()` | ✅ | ✅ | `test_mining.c` | 2 test cases |
| `construct_bm_job()` | ✅ | ✅ | `test_mining.c` | |
| `increment_bitmask()` | ✅ | ✅ | `test_mining.c` | |
| `extranonce_2_generate()` | ✅ | ✅ | `test_mining.c` | |
| `test_nonce_value()` | ✅ | ❌ | `test_mining.c` | Uses hardware SHA, tagged `[not-on-qemu]` |
| `free_bm_job()` | ❌ | ✅ | - | Memory cleanup |

### `stratum_api.c` - 1/12 functions tested (8%)

| Function | Tested | QEMU | Notes |
|----------|:------:|:----:|-------|
| `STRATUM_V1_parse()` | ✅ | ✅ | 11 test cases in `test_stratum_json.c` |
| `STRATUM_V1_transport_init()` | ❌ | ❌ | Network required |
| `STRATUM_V1_initialize_buffer()` | ❌ | ✅ | **Can add** |
| `STRATUM_V1_receive_jsonrpc_line()` | ❌ | ❌ | Network required |
| `STRATUM_V1_subscribe()` | ❌ | ❌ | Network required |
| `STRATUM_V1_stamp_tx()` | ❌ | ✅ | **Can add** |
| `STRATUM_V1_free_mining_notify()` | ❌ | ✅ | **Can add** |
| `STRATUM_V1_authorize()` | ❌ | ❌ | Network required |
| `STRATUM_V1_configure_version_rolling()` | ❌ | ❌ | Network required |
| `STRATUM_V1_suggest_difficulty()` | ❌ | ❌ | Network required |
| `STRATUM_V1_submit_share()` | ❌ | ❌ | Network required |
| `STRATUM_V1_get_response_time_ms()` | ❌ | ✅ | **Can add** |

---

## Component: `asic`

### `pll.c` - 1/1 functions tested (100%)

| Function | Tested | QEMU | Test File |
|----------|:------:|:----:|-----------|
| `pll_get_parameters()` | ✅ | ✅ | `test_pll.c` |

### `crc.c` - 0/3 functions tested (0%)

| Function | Tested | QEMU | Notes |
|----------|:------:|:----:|-------|
| `crc5()` | ❌ | ✅ | **Can add** - pure algorithm |
| `crc16()` | ❌ | ✅ | **Can add** - pure algorithm |
| `crc16_false()` | ❌ | ✅ | **Can add** - pure algorithm |

### `common.c` - 3/5 functions tested (60%)

| Function | Tested | QEMU | Test File | Notes |
|----------|:------:|:----:|-----------|-------|
| `_reverse_bits()` | ✅ | ✅ | `test_common.c` | 9 test cases |
| `_largest_power_of_two()` | ✅ | ✅ | `test_common.c` | 13 test cases |
| `get_difficulty_mask()` | ✅ | ✅ | `test_common.c` | 6 test cases |
| `count_asic_chips()` | ❌ | ❌ | - | Hardware UART/SERIAL required |
| `receive_work()` | ❌ | ❌ | - | Hardware UART/SERIAL required |

### ASIC Drivers (BM1366/BM1368/BM1370/BM1397)

These require physical ASIC hardware and cannot run on QEMU. The `test_job_command.c` file contains a commented-out hardware test for BM1397.

---

## Components Without Tests

| Component | Location | Reason | QEMU Testable? |
|-----------|----------|--------|:--------------:|
| WiFi/Network | `components/connect/` | Hardware WiFi stack | ❌ |
| DNS Server | `components/dns_server/` | Network listener | ❌ |
| ADC | `main/adc.c` | ADC hardware | ❌ |
| NVS Config | `main/nvs_config.c` | NVS storage | ⚠️ Partial |
| Work Queue | `main/work_queue.c` | FreeRTOS queue | ✅ |
| Thermal | `main/thermal/` | I2C sensors (EMC2101/2103/2302, TMP1075) | ❌ |
| Power | `main/power/` | I2C PMBus (DS4432U, vcore) | ❌ |
| BAP Protocol | `main/bap/` | Protocol layer | ⚠️ Partial (parsing) |
| HTTP Server | `main/http_server/` | HTTP/WebSocket | ❌ |
| Display | `main/display.c` | LVGL hardware | ❌ |
| Device Config | `main/device_config.c` | NVS dependent | ⚠️ Partial |

---

## Test Summary

```
┌─────────────────────────────────────────────────────┐
│ CURRENT COVERAGE                                    │
├─────────────────────────────────────────────────────┤
│ Functions with tests:  17 / ~50 testable (34%)      │
│ Active test cases:     ~53                          │
│ QEMU compatible:       ~50 tests                    │
│ Hardware only:         ~3 tests (tagged [not-on-qemu])│
└─────────────────────────────────────────────────────┘
```

---

## Recommended New Tests

### High Priority - Pure algorithms, easy to add

1. **`crc.c`** tests for `crc5()`, `crc16()`, `crc16_false()`
   - Pure CRC algorithms with known test vectors
   
2. ~~**`common.c`** tests for `_reverse_bits()`, `_largest_power_of_two()`, `get_difficulty_mask()`~~ ✅ **DONE**
   - Added in `test_common.c` with 28 test cases
   
3. **`utils.c`** tests for `midstate_sha256_bin()`, `le256todouble()`, `suffixString()`, `hashCounterToGhs()`
   - Math and string formatting functions

### Medium Priority - May need mocking

4. **`work_queue.c`** - Queue operations (FreeRTOS available on QEMU)
5. **`stratum_api.c`** - `STRATUM_V1_stamp_tx()`, `STRATUM_V1_get_response_time_ms()`

### Low Priority - Complex setup

6. **Integration tests** - Full mining job flow (construct → hash → verify)
7. **BAP protocol parsing** - Message encoding/decoding

---

## How to Add a New Test

### 1. Create test file in component's test directory

```c
// components/asic/test/test_crc.c
#include "unity.h"
#include "crc.h"

TEST_CASE("CRC5 calculation", "[crc]")
{
    uint8_t data[] = {0x01, 0x02, 0x03};
    uint8_t result = crc5(data, 3);
    TEST_ASSERT_EQUAL_UINT8(expected_crc, result);
}
```

### 2. Add CMakeLists.txt if needed

```cmake
# components/asic/test/CMakeLists.txt
idf_component_register(SRC_DIRS "."
                    INCLUDE_DIRS "."
                    REQUIRES unity asic)
```

### 3. Tag hardware-dependent tests

```c
// This test will be skipped on QEMU
TEST_CASE("Test with hardware SHA", "[mining][not-on-qemu]")
{
    // Uses hardware acceleration
}
```

### 4. Rebuild and run

```bash
cd test-ci
idf.py build
# Push to GitHub to run on QEMU
```

---

## Python Verifiers

Located in `components/stratum/test/verifiers/`:

| File | Purpose |
|------|---------|
| `bm1397.py` | BM1397 job verification - generates expected midstate values |
| `merklecalc.py` | Merkle root calculation - validates merkle tree implementation |

These Python scripts help generate test vectors for the C unit tests.

---

## CI/CD Integration

### Workflow: `.github/workflows/unittest.yml`

1. **Build**: ESP-IDF v5.5.1 builds `test-ci/` with `esp32s3` target
2. **Run**: QEMU executes tests, skipping `[not-on-qemu]` tagged tests
3. **Report**: Results uploaded as `report.xml` artifact
4. **Publish**: `unittest-results.yml` posts results to PR

### Test Configuration

- Watchdogs disabled via `test-ci/sdkconfig.defaults`:
  ```
  CONFIG_ESP_INT_WDT=n
  CONFIG_ESP_TASK_WDT=n
  ```
- Test runner in `test-ci/main/unit_test_all.c` excludes hardware tests:
  ```c
  unity_run_tests_by_tag("[not-on-qemu]", true);  // true = invert/skip
  ```
