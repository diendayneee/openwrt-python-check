# Mock Project: Check Python 3.9 trên OpenWRT x86-64

> Ứng dụng C kiểm tra phiên bản Python 3.9 — build bằng **OpenWRT SDK**, test trên **OpenWRT rootfs** — toàn bộ container hoá bằng Docker 2-stage.

---

## Mục lục

1. [Mục tiêu](#1-mục-tiêu)
2. [Tại sao cần 2 stage?](#2-tại-sao-cần-2-stage)
3. [Kiến trúc tổng quan](#3-kiến-trúc-tổng-quan)
4. [Cấu trúc thư mục](#4-cấu-trúc-thư-mục)
5. [Lý thuyết nền](#5-lý-thuyết-nền)
6. [Code đầy đủ](#6-code-đầy-đủ)
7. [Hướng dẫn chạy từng bước](#7-hướng-dẫn-chạy-từng-bước)
8. [Output kỳ vọng](#8-output-kỳ-vọng)
9. [Git workflow](#9-git-workflow)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. Mục tiêu

Project này simulate quá trình phát triển một **userspace utility** cho hệ thống embedded Linux (OpenWRT chạy trên Raspberry Pi 4B hoặc x86-64), với 4 mục tiêu học tập chính:

| Mục tiêu | Nội dung |
|---|---|
| **Docker multi-stage** | Tách build env và test env thành 2 image hoàn toàn độc lập |
| **OpenWRT SDK** | Dùng đúng toolchain chính thức + cấu trúc `package/<name>/Makefile` của OpenWRT |
| **Makefile automation** | Tự động hoá toàn bộ flow: build → package → test |
| **opkg** | Cài và verify `.ipk` package trên rootfs OpenWRT thật |

**Không cần phần cứng thật** — môi trường OpenWRT được mock bằng Docker container.

---

## 2. Tại sao cần 2 stage?

### Vấn đề với cách cũ (Ubuntu)

Nếu chỉ dùng `ubuntu:22.04` + `gcc` để compile, app sẽ **link với glibc của Ubuntu**, không chạy được trên OpenWRT vì:

- OpenWRT dùng **musl libc** (không phải glibc)
- Filesystem layout khác (`/usr/lib`, `/lib` khác cấu trúc)
- Package manager là **opkg**, không phải apt/dpkg
- Kernel có thể khác config

### Giải pháp: Cross-compile đúng toolchain

```
Ubuntu (host)
    └── Docker: openwrt/sdk         ← STAGE 1: compile với đúng toolchain OpenWRT
            └── .ipk file
                    └── Docker: openwrt/rootfs   ← STAGE 2: chạy trên "board OpenWRT ảo"
```

| | Ubuntu cũ (sai) | OpenWRT 2-stage (đúng) |
|---|---|---|
| Build env | `ubuntu:22.04` + gcc | `openwrt/sdk` — toolchain chính thức |
| Target OS | Ubuntu | OpenWRT rootfs thật |
| Packaging | Thủ công / hack | `.ipk` chuẩn qua OpenWRT build system |
| Verify | Không test được trên OWT | `opkg install` trên rootfs OpenWRT thật |

---

## 3. Kiến trúc tổng quan

```
HOST MACHINE
┌──────────────────────────────────────────────────────────────────────┐
│                                                                      │
│  ┌─────────────────────────────┐    ┌──────────────────────────────┐ │
│  │  STAGE 1: mock_build        │    │  STAGE 2: mock_test          │ │
│  │                             │    │                              │ │
│  │  Image: openwrt/sdk         │    │  Image: openwrt/rootfs       │ │
│  │         x86-64-23.05.3      │    │         x86-64-23.05.3       │ │
│  │                             │    │                              │ │
│  │  • GCC cross-compiler       │    │  • /bin /sbin /etc chuẩn OWT │ │
│  │  • OpenWRT build system     │    │  • opkg package manager      │ │
│  │  • feeds / rules.mk         │    │  • busybox shell             │ │
│  │                             │    │                              │ │
│  │  Nhiệm vụ:                  │    │  Nhiệm vụ:                   │ │
│  │  Compile check_python.c     │    │  opkg install check-python   │ │
│  │  → sinh .ipk chuẩn OWT      │    │  → chạy check_python         │ │
│  │                             │    │  → kiểm tra output           │ │
│  └──────────────┬──────────────┘    └──────────────▲───────────────┘ │
│                 │                                   │                 │
│         artifacts/                           ipk/                    │
│   check-python_1.0-1_x86_64.ipk  ──────►  (copy thủ công)          │
└──────────────────────────────────────────────────────────────────────┘
```

Hai image **hoàn toàn độc lập**, giao tiếp duy nhất qua file `.ipk`.

---

## 4. Cấu trúc thư mục

```
Mock-Docker/
├── README.md
│
├── mock_build/                        ◄── STAGE 1: Compile → sinh .ipk
│   ├── Dockerfile                         FROM openwrt/sdk:x86-64-23.05.3
│   ├── Makefile                           target: build / package / clean
│   ├── openwrt-pkt/
│   │   ├── Makefile                       OpenWRT package Makefile chuẩn
│   │   └── src/
│   │       └── check_python.c             C source code
│   └── artifacts/                         .ipk output (tạo sau `make package`)
│       └── check-python_1.0-1_x86_64.ipk
│
└── mock_test/                         ◄── STAGE 2: Install .ipk + test
    ├── Dockerfile                         FROM openwrt/rootfs:x86-64-23.05.3
    ├── Makefile                           target: build / run / clean
    └── ipk/                               chứa .ipk copy từ mock_build/artifacts/
        └── check-python_1.0-1_x86_64.ipk
```

---

## 5. Lý thuyết nền

### 5.1 OpenWRT SDK (`openwrt/sdk`)

Image Docker chính thức của OpenWRT, đã tích hợp sẵn:

- **GCC cross-compiler** — compile cho target architecture (x86-64, ARM, MIPS...)
- **binutils** — linker, assembler, objdump...
- **feeds system** — quản lý danh sách package có thể build
- **rules.mk / package.mk** — macro system để định nghĩa package

> **Quirk quan trọng:** Image SDK đã set sẵn `USER buildbot` và `WORKDIR` trỏ vào SDK root. Không cần khai báo lại trong Dockerfile của project.
>
> Verify: `docker run --rm openwrt/sdk:x86-64-23.05.3 bash -c 'whoami; pwd'`

### 5.2 OpenWRT Package Makefile

OpenWRT dùng hệ thống Makefile **riêng biệt** để định nghĩa package — khác hoàn toàn với Makefile thông thường. Cấu trúc bắt buộc:

```makefile
include $(TOPDIR)/rules.mk
# ... khai báo PKG_NAME, PKG_VERSION, PKG_RELEASE
include $(INCLUDE_DIR)/package.mk
# ... define Package, Build/Prepare, Build/Compile, Package/install
$(eval $(call BuildPackage,<tên-package>))
```

Compile **bắt buộc** dùng `$(TARGET_CC)` — cross-compiler của SDK, không được dùng `gcc` thông thường.

### 5.3 Định dạng `.ipk`

`.ipk` là `ar` archive gồm 3 thành phần:

```
check-python_1.0-1_x86_64.ipk  (ar archive)
├── debian-binary       → "2.0\n"  (version marker)
├── control.tar.gz      → Metadata: Package, Version, Architecture, Description
└── data.tar.gz         → Payload: usr/bin/check_python  (giải nén vào / của target)
```

Inspect thủ công:
```bash
ar t artifacts/check-python_1.0-1_x86_64.ipk
ar x artifacts/check-python_1.0-1_x86_64.ipk
tar -tzf data.tar.gz
```

### 5.4 opkg — Package Manager của OpenWRT

`opkg` tương tự `apt` trên Debian/Ubuntu, nhưng dành cho embedded Linux:

```bash
opkg install /tmp/package.ipk      # cài từ file local
opkg list-installed                # xem package đã cài
opkg files <package>               # xem files của package
opkg update                        # cập nhật feed list (cần internet)
opkg install python3-light         # cài từ feed online
```

### 5.5 Logic ứng dụng `check_python.c`

```
START
  │
  ▼
which python3.9 ?
  │
  ├── CÓ ──► popen("python3.9 --version")
  │            ├── output có "Python 3.9" ──► printf("Detected Python Version: ...")
  │            │                              log /tmp/python_ver.log
  │            │                              return 0  ✅
  │            └── không match ──► (fallthrough)
  │
  ▼
which python3 ?
  │
  ├── CÓ ──► popen("python3 --version")
  │            └── printf("Detected Python but not 3.9: ...")
  │                log version vào /tmp/python_ver.log
  │                return 2  ⚠️
  │
  ▼
printf("Error: Python 3 not found")
log "Error: Python 3 not found" vào /tmp/python_ver.log
return 1  ❌
```

| Tình huống | stdout | Log file | Exit code |
|---|---|---|---|
| Có `python3.9` | `Detected Python Version: Python 3.9.x` | version string | `0` |
| Có `python3` nhưng ≠ 3.9 | `Detected Python but not 3.9: Python 3.x.y` | version string | `2` |
| Không có python3 | `Error: Python 3 not found` | Error message | `1` |

---

## 6. Code đầy đủ

### 6.1 `mock_build/openwrt-pkt/src/check_python.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char version[256] = {0};
    FILE *fp;

    /* ── CASE 1: Kiểm tra python3.9 ── */
    if (system("which python3.9 > /dev/null 2>&1") == 0) {
        fp = popen("python3.9 --version 2>&1", "r");
        if (!fp) { perror("popen"); return 1; }
        fgets(version, sizeof(version), fp);
        pclose(fp);
        version[strcspn(version, "\n")] = 0;  /* trim newline */

        if (strstr(version, "Python 3.9")) {
            printf("Detected Python Version: %s\n", version);
            FILE *log = fopen("/tmp/python_ver.log", "w");
            if (log) { fprintf(log, "%s\n", version); fclose(log); }
            return 0;
        }
    }

    /* ── CASE 2: Kiểm tra python3 (version khác 3.9) ── */
    if (system("which python3 > /dev/null 2>&1") == 0) {
        fp = popen("python3 --version 2>&1", "r");
        if (!fp) { perror("popen"); return 1; }
        fgets(version, sizeof(version), fp);
        pclose(fp);
        version[strcspn(version, "\n")] = 0;

        printf("Detected Python but not 3.9: %s\n", version);
        FILE *log = fopen("/tmp/python_ver.log", "w");
        if (log) { fprintf(log, "%s\n", version); fclose(log); }
        return 2;
    }

    /* ── CASE 3: Không có python ── */
    printf("Error: Python 3 not found\n");
    FILE *log = fopen("/tmp/python_ver.log", "w");
    if (log) { fprintf(log, "Error: Python 3 not found\n"); fclose(log); }
    return 1;
}
```

**API hệ thống được dùng:**

| API | Mục đích |
|---|---|
| `system()` | Spawn subprocess kiểm tra binary tồn tại (`which`) |
| `popen()` | Spawn subprocess + đọc stdout |
| `fgets()` | Đọc dòng output từ pipe |
| `strstr()` | Verify chuỗi version có chứa "Python 3.9" |
| `fopen/fprintf/fclose` | Ghi log ra `/tmp/python_ver.log` |
| `pclose()` | Đóng pipe |

---

### 6.2 `mock_build/openwrt-pkt/Makefile`

> ⚠️ Đây **không phải** Makefile thông thường — đây là OpenWRT Package Definition.

```makefile
include $(TOPDIR)/rules.mk

PKG_NAME    := check-python
PKG_VERSION := 1.0
PKG_RELEASE := 1

include $(INCLUDE_DIR)/package.mk

define Package/check-python
  SECTION   := utils
  CATEGORY  := Utilities
  TITLE     := Check Python 3.9 version utility
endef

define Package/check-python/description
  A C utility that checks for Python 3.9 installation
  and logs the version to /tmp/python_ver.log
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) \
	  -o $(PKG_BUILD_DIR)/check_python \
	     $(PKG_BUILD_DIR)/check_python.c
endef

define Package/check-python/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/check_python $(1)/usr/bin/
endef

$(eval $(call BuildPackage,check-python))
```

---

### 6.3 `mock_build/Dockerfile`

```dockerfile
FROM openwrt/sdk:x86-64-23.05.3

# KHÔNG cần USER hoặc WORKDIR — SDK image đã set sẵn:
#   USER buildbot
#   WORKDIR <sdk_root>

# Setup feeds (cần internet lần đầu, sau đó Docker cache lại)
RUN ./scripts/feeds update -a && \
    ./scripts/feeds install -a && \
    make defconfig

# Copy package definition vào đúng cây nguồn SDK
COPY openwrt-pkt/Makefile    package/check-python/Makefile
COPY openwrt-pkt/src         package/check-python/src

# Compile package → sinh .ipk vào bin/packages/x86_64/base/
RUN make package/check-python/compile V=s

# Khi container chạy: copy .ipk ra /out/ (mount từ host)
CMD cp bin/packages/x86_64/base/check-python_*.ipk /out/
```

---

### 6.4 `mock_build/Makefile`

> ⚠️ Indent trong recipe **phải là TAB**, không phải spaces.

```makefile
IMG          = check-python-builder
ARTIFACT_DIR = artifacts

.PHONY: all build package clean

all: build

# Build Docker image (compile C + sinh .ipk bên trong image)
build:
	docker build -t $(IMG) .

# Chạy container, mount artifacts/ vào /out/, CMD copy .ipk ra host
package: build
	mkdir -p $(ARTIFACT_DIR)
	docker run --rm -v $(PWD)/$(ARTIFACT_DIR):/out $(IMG)
	@echo '------------------------------------'
	@echo 'Artifact:'
	@ls -lh $(ARTIFACT_DIR)/

# Dọn dẹp
clean:
	rm -rf $(ARTIFACT_DIR)/*
	docker rmi -f $(IMG) 2>/dev/null || true
	@echo 'Cleaned.'
```

---

### 6.5 `mock_test/Dockerfile`

```dockerfile
FROM openwrt/rootfs:x86-64-23.05.3

# rootfs image rất minimal — tạo các thư mục opkg cần
RUN mkdir -p /var/lock /var/lib/opkg /tmp

# Copy .ipk vào image
COPY ipk/check-python_*.ipk /tmp/

# Cài package bằng opkg (local file, không cần internet)
RUN opkg install /tmp/check-python_*.ipk && \
    rm /tmp/check-python_*.ipk

# Mở shell tương tác khi container start
CMD ["/bin/sh"]
```

---

### 6.6 `mock_test/Makefile`

```makefile
IMG = check-python-runtime

.PHONY: all build run clean check-ipk

all: build

# Guard: kiểm tra có .ipk chưa trước khi build
check-ipk:
	@ls ipk/check-python_*.ipk 2>/dev/null || \
	  (echo 'ERROR: Khong tim thay .ipk trong ipk/' && \
	   echo 'Chay: cp mock_build/artifacts/check-python_*.ipk mock_test/ipk/' && \
	   exit 1)

# Build test image (cài .ipk vào rootfs OpenWRT)
build: check-ipk
	docker build -t $(IMG) .

# Chạy shell tương tác trong container OpenWRT
run:
	docker run --rm -it $(IMG)

# Dọn dẹp
clean:
	docker rmi -f $(IMG) 2>/dev/null || true
	@echo 'Cleaned.'
```

---

## 7. Hướng dẫn chạy từng bước

### Yêu cầu môi trường

| Công cụ | Version | Kiểm tra |
|---|---|---|
| Docker | ≥ 20.10 | `docker --version` |
| Git | ≥ 2.30 | `git --version` |
| make | GNU Make | `make --version` |
| Internet | Lần đầu ~500MB–1.5GB | — |

> **Windows:** Dùng WSL2 hoặc Git Bash — PowerShell không có `make` mặc định.

---

### Setup Git

```bash
mkdir Mock-Docker && cd Mock-Docker
git init
git config user.name  "Tên của bạn"
git config user.email "email@example.com"

# Tạo branch theo yêu cầu đề
git checkout -b feature/python-version-check
```

### Tạo cấu trúc thư mục

```bash
mkdir -p mock_build/openwrt-pkt/src
mkdir -p mock_build/artifacts
mkdir -p mock_test/ipk
```

Sau đó tạo đủ 6 file theo code ở Section 6.

---

### STAGE 1 — Build & Package

```bash
cd mock_build

# Build image (lần đầu ~10-30 phút do feeds update)
make build

# Export .ipk ra host
make package

# Verify
ls -lh artifacts/
# check-python_1.0-1_x86_64.ipk
```

---

### Copy .ipk sang Stage 2

```bash
cd ..
cp mock_build/artifacts/check-python_1.0-1_x86_64.ipk mock_test/ipk/
```

---

### STAGE 2 — Install & Test

```bash
cd mock_test

# Build runtime image (opkg install .ipk vào rootfs)
make build

# Mở shell trong container OpenWRT
make run
```

---

## 8. Output kỳ vọng

Bên trong container shell (`make run`):

```sh
# Rootfs base không có Python → Case 3
/ # check_python
Error: Python 3 not found

/ # echo $?
1

/ # cat /tmp/python_ver.log
Error: Python 3 not found

# Verify package đã cài đúng
/ # opkg list-installed | grep check-python
check-python - 1.0-1

/ # opkg files check-python
Package check-python (1.0-1) is installed on root and has the following files:
/usr/bin/check_python

/ # which check_python
/usr/bin/check_python
```

> **`Error: Python 3 not found` là kết quả ĐÚNG** — rootfs OpenWRT base không cài sẵn Python.

### Test thêm Case 2 (exit 2)

```sh
# Bên trong container shell:
opkg update
opkg install python3-light    # OpenWRT 23.05 ship Python 3.11

check_python
# Detected Python but not 3.9: Python 3.11.x
echo $?
# 2
```

> **Case 1 (exit 0):** OpenWRT 23.05 không ship python3.9 nên cần cài thủ công từ source nếu muốn test đầy đủ.

---

## 9. Git workflow

```bash
# .gitignore — không commit artifact
echo 'mock_build/artifacts/' >> .gitignore
echo 'mock_test/ipk/'        >> .gitignore

# Commit code
git add .
git commit -m "feat: add C source and OpenWRT package definition"

git add .
git commit -m "feat: add Dockerfile and Makefile for 2-stage Docker build"

# Tag release
git tag v1.0-python-check

# Verify
git log --oneline
git tag
# v1.0-python-check

# Push (nếu có remote)
git push origin feature/python-version-check --tags
```

---

## 10. Troubleshooting

| Lỗi | Nguyên nhân | Fix |
|---|---|---|
| `make build` (stage 2) báo "Khong tim thay .ipk" | `mock_test/ipk/` trống | `cp mock_build/artifacts/check-python_*.ipk mock_test/ipk/` |
| `opkg install` báo `Incompatible architecture` | `.ipk` build cho arch khác | Đảm bảo cả 2 stage cùng dùng `x86-64-23.05.3` |
| `feeds update -a` lỗi network | Không có internet hoặc qua proxy | Kiểm tra kết nối; thêm `--build-arg http_proxy=...` nếu cần |
| Build chậm 30+ phút | Feeds update download metadata lớn | Bình thường lần đầu — không `Ctrl+C` |
| `make: missing separator` | Makefile dùng spaces thay TAB | Đổi indent thành TAB; kiểm tra bằng `cat -A Makefile` |
| `check_python: not found` | PATH không bao gồm `/usr/bin` | Gõ `/usr/bin/check_python` hoặc `export PATH=/usr/bin:$PATH` |
| `docker: permission denied` | User chưa trong group docker | `sudo usermod -aG docker $USER` rồi logout/login lại |
| `opkg: cannot create lock` | Thiếu `/var/lock` trong rootfs | Đã xử lý trong Dockerfile: `mkdir -p /var/lock` |

---

## Tham khảo

- [OpenWRT SDK documentation](https://openwrt.org/docs/guide-developer/toolchain/using_the_sdk)
- [OpenWRT Package Makefile](https://openwrt.org/docs/guide-developer/packages)
- [OpenWRT Docker images](https://hub.docker.com/u/openwrt)
- [opkg manual](https://openwrt.org/docs/guide-user/additional-software/opkg)
