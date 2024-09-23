# iot-mqtt-cam-surveillance
Implementación de C para chip ESP32-CAM con funcionalidad de enviar imágenes a una cola MQTT.

# Configurar ambiente de Desarrollo.
- [Configurar Espressif IDF (IoT Development Framework)](.docs/INSTALLIDF.MD)

Add a dependency on espressif/esp32-camera component:
```bash
idf.py add-dependency "espressif/esp32-camera"

# enable PSRAM
# Change the partition table size and mode.
```