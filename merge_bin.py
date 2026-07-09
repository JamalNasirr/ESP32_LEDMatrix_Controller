import os
import subprocess
Import("env")

def merge_bin_action(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    
    bootloader = os.path.join(build_dir, "bootloader.bin")
    partitions = os.path.join(build_dir, "partitions.bin")
    firmware = os.path.join(build_dir, "firmware.bin")
    merged = os.path.join(build_dir, "firmware_merged.bin")
    littlefs = os.path.join(build_dir, "littlefs.bin")
    
    python_exe = env.subst("$PYTHONEXE")
    user_home = os.path.expanduser("~")
    esptool_path = os.path.join(user_home, ".platformio", "packages", "tool-esptoolpy", "esptool.py")
    
    if not os.path.exists(esptool_path):
        esptool_path = "esptool.py"
        
    cmd = [
        python_exe, esptool_path, "--chip", "esp32", "merge_bin",
        "-o", merged,
        "--flash_mode", "dio",
        "--flash_size", "16MB",
        "0x1000", bootloader,
        "0x8000", partitions,
        "0x10000", firmware
    ]
    
    if os.path.exists(littlefs):
        print(f"\n>>> Found filesystem image {littlefs}, merging it at offset 0x400000...")
        cmd.extend(["0x400000", littlefs])
    else:
        print(f"\n>>> WARNING: Filesystem image {littlefs} not found! Run 'pio run -t buildfs' to generate it. Simulation will start without a filesystem.")
    
    print("\n>>> Generating merged binary for Wokwi simulation...")
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print(result.stdout)
    if result.returncode != 0:
        print(">>> Error merging binaries:", result.stderr)
        raise RuntimeError("Failed to generate merged.bin")
    else:
        print(">>> Successfully generated firmware_merged.bin!\n")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin_action)
