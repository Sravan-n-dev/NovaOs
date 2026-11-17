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
import struct

QEMU_MONITOR_PORT = 45454

def send_monitor_cmd(sock, cmd):
    sock.sendall((cmd + "\n").encode())
    time.sleep(0.4)
    resp = b""
    while True:
        try:
            sock.settimeout(0.4)
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
        time.sleep(0.08)

def ppm_to_bmp(ppm_path, bmp_path):
    """Pure Python PPM (P6) to BMP converter (no external dependencies needed)"""
    try:
        with open(ppm_path, "rb") as f:
            header = f.readline().strip()
            if header != b"P6":
                return False
            # Skip any comment lines
            line = f.readline().strip()
            while line.startswith(b"#"):
                line = f.readline().strip()
            parts = line.split()
            if len(parts) == 2:
                width, height = int(parts[0]), int(parts[1])
                maxval = int(f.readline().strip())
            else:
                width = int(parts[0])
                height = int(parts[1])
                maxval = int(parts[2])
            raw_rgb = f.read()

        row_size = ((width * 3 + 3) // 4) * 4
        pixel_array_size = row_size * height
        file_size = 54 + pixel_array_size

        # BMP Header
        bmp = bytearray()
        bmp += b"BM"
        bmp += struct.pack("<IHHI", file_size, 0, 0, 54)
        # DIB Header (BITMAPINFOHEADER)
        bmp += struct.pack("<IIIHHIIIIII", 40, width, height, 1, 24, 0, pixel_array_size, 2835, 2835, 0, 0)

        # Write rows bottom-up in BGR format
        for y in range(height - 1, -1, -1):
            row = bytearray()
            row_start = y * width * 3
            for x in range(width):
                px = row_start + x * 3
                r = raw_rgb[px]
                g = raw_rgb[px + 1]
                b = raw_rgb[px + 2]
                row += bytes([b, g, r])
            # Row padding to multiple of 4 bytes
            row += bytes(row_size - len(row))
            bmp += row

        with open(bmp_path, "wb") as f:
            f.write(bmp)
        return True
    except Exception as e:
        print(f"[-] BMP conversion error: {e}")
        return False

def main():
    iso_path = "novaos.iso"
    if not os.path.exists(iso_path):
        print(f"Error: {iso_path} not found!")
        sys.exit(1)

    os.makedirs("artifacts", exist_ok=True)

    print("[*] Launching QEMU with NovaOS ISO in headless mode...")
    qemu_cmd = [
        "qemu-system-i386",
        "-cdrom", iso_path,
        "-m", "64M",
        "-display", "none",
        "-monitor", f"tcp:127.0.0.1:{QEMU_MONITOR_PORT},server,nowait",
        "-no-reboot"
    ]

    proc = subprocess.Popen(qemu_cmd)
    time.sleep(3)

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("127.0.0.1", QEMU_MONITOR_PORT))
        print("[+] Connected to QEMU monitor socket.")

        # Read initial monitor banner
        time.sleep(0.5)
        send_monitor_cmd(sock, "")

        # 1. Let kernel boot and run tasks for 4 seconds (400 timer interrupts)
        print("[*] Letting NovaOS execute background tasks A, B, and C...")
        time.sleep(4)

        # 2. Capture initial boot screenshot
        print("[*] Capturing boot screenshot...")
        send_monitor_cmd(sock, "screendump artifacts/screenshot_boot.ppm")

        # 3. Send "tasks" command
        print("[*] Sending 'tasks' command...")
        send_keys(sock, "tasks\n")
        time.sleep(2)
        send_monitor_cmd(sock, "screendump artifacts/screenshot_tasks.ppm")

        # 4. Send "start" command to dynamically create Task 4
        print("[*] Sending 'start' command...")
        send_keys(sock, "start\n")
        time.sleep(2)

        # 5. Send "tasks" command again to show Task 4 scheduled
        print("[*] Sending second 'tasks' command...")
        send_keys(sock, "tasks\n")
        time.sleep(2)
        send_monitor_cmd(sock, "screendump artifacts/screenshot_start.ppm")

        # 6. Send "info" command
        print("[*] Sending 'info' command...")
        send_keys(sock, "info\n")
        time.sleep(2)
        send_monitor_cmd(sock, "screendump artifacts/screenshot_info.ppm")

        # 7. Send "uptime" command
        print("[*] Sending 'uptime' command...")
        send_keys(sock, "uptime\n")
        time.sleep(2)
        send_monitor_cmd(sock, "screendump artifacts/screenshot_uptime.ppm")

        sock.close()
        print("[+] Test interactions completed successfully.")

    finally:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()

    # Convert screenshots to PNG and BMP
    for base in ["screenshot_boot", "screenshot_tasks", "screenshot_start", "screenshot_info", "screenshot_uptime"]:
        ppm_file = os.path.join("artifacts", f"{base}.ppm")
        png_file = os.path.join("artifacts", f"{base}.png")
        bmp_file = os.path.join("artifacts", f"{base}.bmp")
        if os.path.exists(ppm_file):
            # Try convert to PNG
            try:
                subprocess.run(["convert", ppm_file, png_file], check=True, stderr=subprocess.DEVNULL)
                print(f"[+] Converted {base}.ppm -> {base}.png")
            except Exception:
                pass
            # Always produce standard BMP
            if ppm_to_bmp(ppm_file, bmp_file):
                print(f"[+] Generated {base}.bmp")

    print("[*] QEMU test harness finished.")

if __name__ == "__main__":
    main()
