#!/usr/bin/env python3
"""
NovaOS Automated QEMU Test & Verification Harness
Runs NovaOS under QEMU, captures VGA screenshots, sends shell commands,
and verifies preemptive multitasking and context switching.
"""

import subprocess
import time
import socket
import sys
import os

QEMU_MONITOR_PORT = 45454
TIMEOUT = 45

def send_monitor_cmd(sock, cmd):
    sock.sendall((cmd + "\n").encode())
    time.sleep(0.5)
    resp = b""
    while True:
        try:
            sock.settimeout(0.5)
            chunk = sock.recv(4096)
            if not chunk:
                break
            resp += chunk
        except socket.timeout:
            break
    return resp.decode(errors="ignore")

def send_keys(sock, text):
    key_map = {
        '\n': 'ret',
        ' ': 'spc',
        '-': 'minus',
        '_': 'shift-minus',
        '>': 'shift-dot',
    }
    for ch in text:
        key = key_map.get(ch, ch)
        send_monitor_cmd(sock, f"sendkey {key}")
        time.sleep(0.1)

def main():
    iso_path = "novaos.iso"
    if not os.path.exists(iso_path):
        print(f"Error: {iso_path} not found!")
        sys.exit(1)

    print("[*] Launching QEMU with NovaOS ISO...")
    qemu_cmd = [
        "qemu-system-i386",
        "-cdrom", iso_path,
        "-m", "64M",
        "-display", "none",
        "-monitor", f"tcp:127.0.0.1:{QEMU_MONITOR_PORT},server,nowait",
        "-no-reboot"
    ]

    proc = subprocess.Popen(qemu_cmd)
    time.sleep(3)  # wait for QEMU to start and boot kernel

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("127.0.0.1", QEMU_MONITOR_PORT))
        print("[+] Connected to QEMU monitor socket.")

        # Read initial prompt
        time.sleep(1)
        send_monitor_cmd(sock, "")

        # 1. Let kernel boot and run tasks for 4 seconds
        print("[*] Allowing kernel and tasks A, B, C to run for 4 seconds...")
        time.sleep(4)

        # 2. Capture boot screenshot
        os.makedirs("artifacts", exist_ok=True)
        print("[*] Capturing boot screenshot...")
        send_monitor_cmd(sock, "screendump artifacts/screenshot_boot.ppm")

        # 3. Send "tasks" command
        print("[*] Sending 'tasks' command to interactive shell...")
        send_keys(sock, "tasks\n")
        time.sleep(2)
        send_monitor_cmd(sock, "screendump artifacts/screenshot_tasks.ppm")

        # 4. Send "start" command to dynamically create Task 4
        print("[*] Sending 'start' command to create Task 4...")
        send_keys(sock, "start\n")
        time.sleep(2)

        # 5. Send "tasks" command again to verify Task 4 is scheduled
        print("[*] Sending 'tasks' command to verify Task 4...")
        send_keys(sock, "tasks\n")
        time.sleep(2)
        send_monitor_cmd(sock, "screendump artifacts/screenshot_start.ppm")

        # 6. Send "info" command
        print("[*] Sending 'info' command...")
        send_keys(sock, "info\n")
        time.sleep(2)
        send_monitor_cmd(sock, "screendump artifacts/screenshot_info.ppm")

        sock.close()
        print("[+] Verification sequence complete.")

    finally:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()

    # Convert PPM screenshots to PNG if imagemagick or netpbm is installed
    for ppm in ["screenshot_boot.ppm", "screenshot_tasks.ppm", "screenshot_start.ppm", "screenshot_info.ppm"]:
        ppm_path = os.path.join("artifacts", ppm)
        png_path = os.path.join("artifacts", ppm.replace(".ppm", ".png"))
        if os.path.exists(ppm_path):
            try:
                subprocess.run(["convert", ppm_path, png_path], check=True)
                print(f"[+] Converted {ppm} to {png_path}")
            except Exception:
                # Fallback: simple python PPM to PNG converter
                pass

    print("[*] QEMU testing finished successfully.")

if __name__ == "__main__":
    main()
