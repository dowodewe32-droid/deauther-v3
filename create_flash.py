import os
BOOTLOADER_ADDR = 0x1000
PARTITIONS_ADDR = 0x8000
APP_ADDR = 0x10000

with open('.pio/build/esp32dev/bootloader.bin', 'rb') as f:
    bootloader = f.read()
with open('.pio/build/esp32dev/partitions.bin', 'rb') as f:
    partitions = f.read()
with open('.pio/build/esp32dev/firmware.bin', 'rb') as f:
    firmware = f.read()

with open('GMpro87dev_firmware.bin', 'wb') as f:
    f.write(b'\xff' * BOOTLOADER_ADDR)
    f.write(bootloader)
    f.write(b'\xff' * (PARTITIONS_ADDR - (BOOTLOADER_ADDR + len(bootloader))))
    f.write(partitions)
    f.write(b'\xff' * (APP_ADDR - (PARTITIONS_ADDR + len(partitions))))
    f.write(firmware)

print(f"Flash created: {os.path.getsize('flash.bin')} bytes")
