# RaspberryPi 수신 연습 코드

import asyncio
from bleak import BleakClient, BleakScanner

SERVICE_UUID = "12345678-1234-1234-1234-1234567890ab"
CHAR_UUID = "abcdefab-1234-1234-1234-abcdefabcdef"

def handle_notification(sender, data):
    print(data)

async def main():
    print("Scanning...")
    devices = await BleakScanner.discover()
    target = None
    for d in devices:
        if "ESP32" in d.name:
            target = d
            break

    if not target:
        print("not found")
        return

    async with BleakClient(target.address) as client:
        print("Connected")
        await client.start_notify(CHAR_UUID, handle_notification)
        print("Listening for button notifications...")
        while True:
            await asyncio.sleep(1)

if __name__ == "__main__":
    asyncio.run(main())
