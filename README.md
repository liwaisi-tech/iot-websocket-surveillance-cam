# ESP32-CAM Image Format Documentation

## Overview
This repository contains documentation and implementation details for various image formats supported by the ESP32-CAM module. These formats are essential for image processing, storage, and transmission in embedded vision applications.

## Supported Pixel Formats

### High Color Depth
- **RGB888** (24 BPP)
  - Full color reproduction
  - 8 bits per color channel (R,G,B)
  - Ideal for high-quality image processing
  - Memory usage: High (3 bytes per pixel)

### Medium Color Depth
- **RGB565** (16 BPP)
  - Balanced color reproduction
  - 5 bits (Red), 6 bits (Green), 5 bits (Blue)
  - Recommended for most applications
  - Memory usage: Medium (2 bytes per pixel)

### Optimized Formats
- **RGB555** (15 BPP)
- **RGB444** (12 BPP)
- **YUV422** (16 BPP)
- **YUV420** (12 BPP)
- **GRAYSCALE** (8 BPP)
- **JPEG** (Variable compression)
- **RAW** (Sensor dependent)

## Performance Considerations
- RGB888: 3 bytes/pixel (24-bit color)
- RGB565: 2 bytes/pixel (16-bit color)
- JPEG: Variable compression ratio
- GRAYSCALE: 1 byte/pixel (8-bit depth)

## Advanced Developer Guidelines

### Memory Management
- Calculate required buffer size: `width * height * bytes_per_pixel`
- Consider PSRAM usage for high-resolution images
- Use DMA transfers when available

### Best Practices
1. **Image Capture**
   ```c
   camera_fb_t * fb = esp_camera_fb_get();
   if(!fb) {
       ESP_LOGE(TAG, "Camera capture failed");
       return;
   }
   ```

2. **Memory Release**
   ```c
   esp_camera_fb_return(fb); // Always return the frame buffer
   ```

3. **Format Conversion**
   - Convert to lower formats when transmitting
   - Use hardware acceleration when available

### Performance Tips
- Use JPEG for storage/transmission
- RGB565 for real-time processing
- GRAYSCALE for basic computer vision

---

# Documentación Avanzada para Desarrolladores

## Gestión de Memoria
- Cálculo de buffer: `ancho * alto * bytes_por_pixel`
- Considerar uso de PSRAM para imágenes de alta resolución
- Utilizar transferencias DMA cuando estén disponibles

## Mejores Prácticas
1. **Captura de Imagen**
   ```c
   camera_fb_t * fb = esp_camera_fb_get();
   if(!fb) {
       ESP_LOGE(TAG, "Fallo en captura de cámara");
       return;
   }
   ```

2. **Liberación de Memoria**
   ```c
   esp_camera_fb_return(fb); // Siempre devolver el buffer
   ```

3. **Conversión de Formatos**
   - Convertir a formatos más ligeros para transmisión
   - Usar aceleración por hardware cuando esté disponible

### Consejos de Rendimiento
- Usar JPEG para almacenamiento/transmisión
- RGB565 para procesamiento en tiempo real
- GRAYSCALE para visión por computadora básica
