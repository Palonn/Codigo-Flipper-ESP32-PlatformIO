-- Bluetooth BLE scan
os.title("Escaneando BLE...")
local devs = os.bt_scan()
os.title("Dispositivos BLE")
print(devs)
os.wait()
