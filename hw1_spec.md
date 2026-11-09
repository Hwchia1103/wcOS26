# Assignment I 完整規格書：編譯自訂 Linux 核心與實作新系統呼叫
**Compiling a Custom Linux Kernel & Implementing New System Calls**
> **國立陽明交通大學資訊工程學系 [CSIC30015] 作業系統**
> **授課教授**：吳春峰 教授 (Prof. Chun-Feng Wu)
> **評分截止時間**：2026 年 11 月 4 日 (星期三) 23:59 (嚴格執行，逾期 0 分)
> **學術誠信原則**：禁止任何抄襲行為，需獨立完成，違者 0 分。

---

## 1. 作業目標 (Goals)

1. **核心編譯體驗**：從原始碼跨平台編譯適用於 **RISC-V 64 位元架構 (`riscv64`)** 的 Linux v6.1 LTS 核心。
2. **客製化核心標籤**：配置並驗證核心 Local Version，產出符合格式 `6.1.0-os-<student ID>` 的自訂核心映像。
3. **系統呼叫開發**：
   - 實作 `sys_revstr` (Syscall No. 451)：使用者字串原地反轉，包含核心記憶體隔離與 Kernel Ring Buffer 紀錄。
   - 實作 `sys_tempbuf` (Syscall No. 452)：動態鏈結串列字串緩衝區，支援 `ADD`、`REMOVE` 與 `PRINT` 操作，並具備互斥鎖 (`mutex`) 併行保護與健全的錯誤處理機制。
4. **模擬器與 Initramfs 整合**：客製化 BusyBox initramfs，將靜態編譯的 RISC-V 測試程式注入，並於 **QEMU 虛擬機器** 內順利開機測試。
5. **核心修補檔產出**：使用 `git format-patch` 產生標準 Linux Email-format 核心補丁。
6. **繳交檔案封裝**：嚴格按照助教自動化評分規範封裝 ZIP 檔案。

---

## 📋 作業執行進度清單 (Assignment Progress Checklist)

> [!NOTE]
> **當前進度**：恭喜！Section 3.3 核心初次編譯已成功完成（`Kernel: arch/riscv/boot/Image is ready` ✅），基礎核心環境與 Local Version 已就緒！接下來請推進至 **Section 4 系統呼叫實作**。

### 階段一：環境與核心編譯 (Requirement I)
- [x] **1.1 Docker 環境建置**：啟動 Ubuntu 24.04 容器並驗證工具鏈 (`gcc-riscv64-linux-gnu`, `qemu-system-riscv64`)
- [x] **1.2 核心源碼取得**：下載 Linux v6.1 LTS 原始碼樹 (`git clone --branch v6.1 --depth 1`)
- [x] **1.3 核心設定 (.config)**：執行 `make defconfig` 生成 RISC-V 基礎設定
- [x] **1.4 學號 LocalVersion 設定**：配置 `CONFIG_LOCALVERSION="-os-A151064"` 並關閉 `CONFIG_LOCALVERSION_AUTO`
- [x] **1.5 核心版本驗證**：執行 `make kernelrelease` 確認輸出精確為 `6.1.0-os-A151064`
- [x] **1.6 核心映像編譯 (Section 3.3)**：執行 `make -j$(nproc) Image` 成功產生 `arch/riscv/boot/Image` ✅

### 階段二：新增系統呼叫 (Requirement II)
- [x] **2.1 註冊 Syscall 編號**：修改 `arch/riscv/include/uapi/asm/unistd.h` 定義 451 與 452，並更新 `__NR_syscalls` 至 453 ✅
- [x] **2.2 宣告函式原型**：修改 `include/linux/syscalls.h` 加入 `sys_revstr` 與 `sys_tempbuf` ✅
- [x] **2.3 註冊編譯物件**：修改 `kernel/Makefile` 加入 `obj-y += sys_revstr.o sys_tempbuf.o` ✅
- [x] **2.4 實作 Syscall 核心本體**：建立並編寫 `kernel/sys_revstr.c` 與 `kernel/sys_tempbuf.c` ✅
  - [x] 實作 `sys_revstr` (451)：使用者記憶體隔離 (`copy_from_user` / `copy_to_user`)、原地反轉與 `pr_info` ✅
  - [x] 實作 `sys_tempbuf` (452)：`struct list_head` 鏈結串列、`DEFINE_MUTEX` 併行鎖、`ADD`/`REMOVE`/`PRINT` 模式與錯誤碼 ✅
- [x] **2.5 重新編譯核心映像**：再次執行 `make -j$(nproc) ... Image` 成功產生新核心映像 ✅

### 階段三：測試與 QEMU 驗證 (Section 5)
- [x] **3.1 靜態交叉編譯測試程式**：使用 `riscv64-linux-gnu-gcc -static` 編譯 `test_revstr`, `test_tempbuf`, `verify_syscalls` ✅
- [x] **3.2 解包與重封裝 Initramfs**：解開 `initramfs.cpio.gz`，注入測試執行檔並重新打包 ✅
- [x] **3.3 QEMU 開機驗證**：使用 `qemu-system-riscv64` 開機進入 BusyBox Shell ✅
- [x] **3.4 執行測試與採集截圖 (Screenshot Checklist)**：
  - [x] **SS-01**: `uname -a` (驗證 `-os-A151064`) ✅
  - [x] **SS-02**: `cat /proc/version` (驗證編譯環境) ✅
  - [x] **SS-03**: `./test_revstr` 使用者輸出 ✅
  - [x] **SS-04**: `dmesg | tail -n 10` for `sys_revstr` ✅
  - [x] **SS-05**: `./test_tempbuf` 使用者輸出 ✅
  - [x] **SS-06**: `dmesg | tail -n 15` for `sys_tempbuf` ✅
  - [x] **SS-07**: `./verify_syscalls` 通過隱藏邊界測資 ✅

### 階段四：補丁產出、結案報告與封裝繳交 (Section 7)
- [x] **4.1 建立 Git Patch**：核心目錄簽出分支並執行 `git format-patch -1` 生成 `0001-xxxx.patch` ✅
- [x] **4.2 匯出套件清單**：執行 `dpkg --get-selections > packages_list_A151064.txt` ✅
- [x] **4.3 備份核心設定**：複製 `/home/ubuntu/linux/.config` ✅
- [x] **4.4 完成結案報告**：將截圖放入 `report.md` 並匯出為 `A151064_report.pdf` ✅
- [x] **4.5 壓縮繳交檔案**：打包為 `HW1_A151064.zip` 並嚴格確認第一層目錄為 `HW1_A151064/` ✅

---

## 2. 環境前置準備 (Prerequisites)

### 2.1 硬體與主機需求
- **作業系統**：Linux (Ubuntu 22.04 / 24.04 LTS 原生環境) 或 Windows 11 **WSL2** (Ubuntu)。
- **磁碟空間**：建議保留 **至少 30 GB ~ 50 GB** 可用空間（Linux 核心源碼解壓縮與編譯中間產物約需 15GB~25GB，加上 Docker 映像檔與 cpio 空間）。
- **記憶體與 SWAP**：建議實體記憶體至少 8 GB，若實體記憶體較小，請設定至少 8 GB SWAP，避免多核心編譯 (`-j$(nproc)`) 時觸發 OOM Killer。

### 2.2 核心編譯必備套件清單
Docker 容器內部或原生主機需安裝以下建置工具鏈與函式庫：
```bash
sudo apt update && sudo apt install -y \
    build-essential \
    gcc-riscv64-linux-gnu \
    qemu-system-misc \
    bison \
    flex \
    bc \
    libncurses5-dev \
    libncurses-dev \
    libssl-dev \
    libelf-dev \
    dwarves \
    cpio \
    rsync \
    git \
    python3 \
    python3-pip \
    ninja-build \
    cmake \
    libglib2.0-dev \
    libslirp-dev \
    zip \
    unzip
```

### 2.3 Docker Workspace 配置架構
助教環境提供標準化 Docker 環境（Ubuntu 24.04），以確保工具鏈一致性：
- 容器掛載點：`./docker/home/ubuntu` $\rightarrow$ 容器內 `/home/ubuntu`
- 專案工作區掛載：`.:/workspace:ro`（建議設定為唯讀掛載，保護主機原始程式）
- SSH 存取：預設轉發至本機連接埠 `22025`，SSH 金鑰存放於 `docker/home/ubuntu/.ssh/id_rsa`。

---

## 3. 核心取得與編譯全流程規格 (Kernel Compilation)

> **背景說明 (CIP SLTS)**：
> 本作業要求使用 **Linux v6.1 LTS (或 v6.1.0)**。該版本為長期支援核心，維護期至 2027 年 12 月；此外，民生基礎設施專案 (Civil Infrastructure Platform, CIP) 已將 v6.1 選為 Super-LTS (SLTS)，計劃提供長達 10 年（至 2033 年 8 月）之維護期。

### 3.1 取得原始碼
```bash
cd /home/ubuntu
git clone https://github.com/torvalds/linux --branch v6.1 --depth 1 linux
cd /home/ubuntu/linux
```
> 使用 `--depth 1` 可略過數十萬筆歷史 commit，大幅減少下載容量（由 ~5GB 降至 ~300MB）。

### 3.2 核心配置 (`.config`) 與學號設定
必須遵守以下核心配置規定：
1. 目標架構：`ARCH=riscv`
2. 交叉編譯器前綴：`CROSS_COMPILE=riscv64-linux-gnu-`
3. 基準設定：使用 RISC-V 預設配置 `defconfig`：
   ```bash
   make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- defconfig
   ```
4. **Local Version 設定 (重要！佔 20% 分數)**：
   - 設定 `CONFIG_LOCALVERSION="-os-<student ID>"`（例如 `-os-A151064`）。
   - 禁用 `CONFIG_LOCALVERSION_AUTO`（設為 `n`，避免版本尾端附加 git commit hash 如 `-g830b3c68c` 導致比對失敗）。
   - 執行腳本配置：
     ```bash
     scripts/config --file .config --set-str CONFIG_LOCALVERSION "-os-<student ID>"
     scripts/config --file .config --disable CONFIG_LOCALVERSION_AUTO
     make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- olddefconfig
     make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- prepare
     ```
5. **版本字串驗證**：
   ```bash
   make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- kernelrelease
   ```
   **標準輸出必須精確為**：
   ```text
   6.1.0-os-<student ID>
   ```

### 3.3 編譯目標與產物
```bash
make -j$(nproc) ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- Image
```
- **編譯成功標誌**：
  ```text
    LD      vmlinux
    NM      System.map
    SORTTAB vmlinux
    OBJCOPY arch/riscv/boot/Image
    Kernel: arch/riscv/boot/Image is ready
  ```
- 生出的 RISC-V 未壓縮二進位核心映像為：`arch/riscv/boot/Image`（大小約 20MB ~ 25MB）。
- **當前狀態**：✅ **已成功編譯完成** (`Kernel: arch/riscv/boot/Image is ready`)！基礎核心與學號版本設定已經建置就緒。

---

## 4. 系統呼叫 (System Calls) 實作詳細規格

### 4.1 System Call 1: `sys_revstr` (Syscall No. 451)

#### 介面定義
```c
#include <unistd.h>
#define __NR_revstr 451

int syscall(__NR_revstr, char *str, size_t n);
```

#### 行為規格
1. 傳入參數：使用者空間字串指標 `str` 以及其長度 `n`。
2. 透過 `copy_from_user()` 將字串安全拷貝至核心空間動態配置的緩衝區。
3. 輸出原字串至 Kernel Ring Buffer (`pr_info` 或 `printk`)：
   - 格式：`The origin string: <str>`
4. 於核心空間進行**原地字串反轉 (In-place Reversal)**。
5. 輸出反轉後字串至 Kernel Ring Buffer：
   - 格式：`The reversed string: <rev_str>`
6. 透過 `copy_to_user()` 將反轉後的字串覆寫回使用者空間指標 `str`。
7. **回傳值**：
   - 成功：回傳 `0`。
   - 記憶體存取異常：回傳 `-EFAULT`。
   - 核心記憶體配置失敗：回傳 `-ENOMEM`。

---

### 4.2 System Call 2: `sys_tempbuf` (Syscall No. 452)

#### 介面定義
```c
#include <unistd.h>
#define __NR_tempbuf 452

enum mode {
    PRINT,  // 0: 串接所有紀錄並輸出至核心日誌與使用者緩衝區
    ADD,    // 1: 新增紀錄至串列尾端
    REMOVE  // 2: 移除第一筆匹配紀錄
};

int syscall(__NR_tempbuf, enum mode, void *data, size_t size);
```

#### 行為規格與模式說明
維護一個存放在 Linux Kernel 空間的動態字串鏈結串列（建議使用核心標準雙向鏈結串列 `struct list_head`），並透過互斥鎖 (`struct mutex`) 確保並行安全性：

1. **`ADD` (模式 1)**：
   - 將長度為 `size` 的使用者字串 `data` 拷貝至核心，動態配置節點並加入串列尾端 (FIFO)。
   - 輸出至核心日誌：`[tempbuf] Added: <data>`
   - 成功回傳 `0`。
2. **`REMOVE` (模式 2)**：
   - 搜尋串列中**第一筆**內容與長度完全匹配 `data` 的紀錄。
   - 若匹配成功：自串列中脫鉤 (`list_del`) 並釋放記憶體 (`kfree`)。
   - 輸出至核心日誌：`[tempbuf] Removed: <data>`
   - 成功回傳 `0`。
   - **若串列中找不到匹配紀錄：回傳 `-ENOENT`**（使用者端 `errno` 會被設為 `ENOENT`，此為隱藏測資重點）。
3. **`PRINT` (模式 0)**：
   - 將串列中所有紀錄依加入順序以**單一空白字元 (`" "`)** 連接。
   - 輸出至核心日誌：`[tempbuf] <concatenated_string>`（若串列為空，則印出 `[tempbuf] `）。
   - 將連接後字串拷貝回使用者指標 `data`，最大不超過 `size` 位元組，並確保結尾有 `'\0'`。
   - **回傳值 (關鍵細節！)**：回傳**實際拷貝至使用者空間的字元總數，嚴格不包含結尾的 null terminator (`'\0'`)**。若串列為空，回傳 `0`。
4. **錯誤代碼規範**：
   - `data == NULL` 或 `size == 0` 或記憶體存取失敗：回傳 `-EFAULT`。
   - 模式不合法（非 0, 1, 2）：回傳 `-EINVAL`。
   - 找不到刪除項目：回傳 `-ENOENT`。
   - 核心記憶體配置失敗：回傳 `-ENOMEM`。

---

### 4.3 核心修改檔案清單與路徑

在 Linux v6.1 核心樹中，新增系統呼叫共需修改/新增以下 4 個檔案：

| 修改檔案路徑 | 修改性質 | 目的與內容說明 |
| :--- | :---: | :--- |
| `arch/riscv/include/uapi/asm/unistd.h` | 修改 | 定義 `__NR_revstr 451`、`__NR_tempbuf 452`，並將 `__NR_syscalls` 更新為 453 |
| `include/linux/syscalls.h` | 修改 | 宣告函式原型 `sys_revstr` 與 `sys_tempbuf` |
| `kernel/Makefile` | 修改 | 將實作目的檔 `obj-y += hw1_syscalls.o` 編入核心 |
| `kernel/hw1_syscalls.c` | **新增** | 實作兩個系統呼叫之核心本體程式碼 |

---

## 5. 使用者空間測試與 QEMU 驗證

### 5.1 靜態交叉編譯測試程式
由於 BusyBox initramfs 缺乏動態連結載入器 (ld.so)，測試程式必須加上 `-static` 編譯：
```bash
riscv64-linux-gnu-gcc -static -o test_revstr test_revstr.c
riscv64-linux-gnu-gcc -static -o test_tempbuf test_tempbuf.c
riscv64-linux-gnu-gcc -static -o verify_syscalls verify_syscalls.c
```

### 5.2 Initramfs 解包與重封裝
```bash
mkdir -p /home/ubuntu/initramfs && cd /home/ubuntu/initramfs
gzip -dc /workspace/initramfs.cpio.gz | cpio -idmu
cp /home/ubuntu/test_revstr .
cp /home/ubuntu/test_tempbuf .
cp /home/ubuntu/verify_syscalls .
find . -print0 | cpio --null -o -H newc | gzip -c > /home/ubuntu/initramfs.cpio.gz
```

### 5.3 QEMU 啟動指令與熱鍵
```bash
qemu-system-riscv64 -nographic -machine virt \
  -kernel /home/ubuntu/linux/arch/riscv/boot/Image \
  -initrd /home/ubuntu/initramfs.cpio.gz \
  -append "console=ttyS0 loglevel=3"
```
> **退出 QEMU**：按下鍵盤 `Ctrl + A`，放開後立即按下 `X`。

---

## 6. 【重點】截圖需求對照表 (Screenshot Checklist)

在產出最終 PDF 報告時，必須包含以下關鍵截圖佐證，任一缺漏皆會導致扣分：

| 項目編號 | 階段 / 測試功能 | 必須執行的指令 | 截圖需包含的關鍵畫面 / 資訊 | 於 Report 中的呈現章節 | 配分 |
| :---: | :--- | :--- | :--- | :--- | :---: |
| **SS-01** | 核心編譯驗證 (系統識別) | `uname -a` | 清楚顯示核心發行版後綴為 `-os-<student ID>` 且架構為 `riscv64` | Section 2.2 核心驗證 | 2.5% |
| **SS-02** | 核心編譯驗證 (編譯版本) | `cat /proc/version` | 顯示 gcc 版本、編譯主機與自訂版本資訊 | Section 2.2 核心驗證 | 2.5% |
| **SS-03** | `sys_revstr` 使用者輸出 | `./test_revstr` | 終端印出 `hello` 轉為 `olleh`，`Operating System` 轉為 `metsyS gnitarepO` | Section 3.5 測試截圖 | 1% |
| **SS-04** | `sys_revstr` 核心日誌 | `dmesg \| tail -n 10` | 顯示 `The origin string: ...` 與 `The reversed string: ...` | Section 3.5 測試截圖 | 1% |
| **SS-05** | `sys_tempbuf` 使用者輸出 | `./test_tempbuf` | 終端印出包含空白的合併字串，以及刪除後剩下的字串 | Section 3.5 測試截圖 | 1% |
| **SS-06** | `sys_tempbuf` 核心日誌 | `dmesg \| tail -n 15` | 顯示 `[tempbuf] Added: ...`、`[tempbuf] Removed: ...` 與 `[tempbuf] ...` | Section 3.5 測試截圖 | 1% |
| **SS-07** | 延伸邊界防護測試 *(強烈建議)* | `./verify_syscalls` | 顯示 `Extended syscall checks passed.` 證明通過隱藏測資 | Section 4 驗證章節 | (輔助佐證) |

---

## 7. 評分標準與繳交規範 (Grading & Submission)

### 7.1 配分比重 (總分 100%)
1. **結案報告 Report (20%)**：
   - 核心編譯證明截圖與編譯步驟列表 (10%)
   - 系統呼叫 4 張測試截圖與技術原理剖析 (10%)
2. **實作評分 Implementation (80%)**：
   - 核心 Local Version 設定符合 `-os-<student ID>` (20%)
   - `sys_revstr` 隱藏測試案例通過 (30%)
   - `sys_tempbuf` 隱藏測試案例通過 (30%)

### 7.2 繳交檔案清單與 ZIP 架構（扣 20 分禁忌警語）
壓縮檔案名稱：**`HW1_<student ID>.zip`**

**內部結構必須完全一致**：
```text
HW1_<student ID>.zip
    |- HW1_<student ID>/
        |- <student ID>_report.pdf
        |- .config
        |- packages_list_<student ID>.txt
        |- 0001-Add-sys_revstr-and-sys_tempbuf-system-calls-for-RISC.patch
```

> [!CAUTION]
> 1. 解壓縮後的根目錄必須剛好為 `HW1_<student ID>/`，不可直接將檔案散落在壓縮檔根目錄，亦不可命名為 `<student ID>/`，**違者一律扣 20 分**。
> 2. 嚴禁包含任何額外目錄或 macOS 系統暫存檔（例如 `__MACOSX`、`.git`、`.vscode` 等）。
> 3. 補丁檔案必須為 `git format-patch` 格式，若助教以 `git apply` 或 `git am` 無法套用至乾淨 v6.1 核心，該實作部分計 0 分。

