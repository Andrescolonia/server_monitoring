# Frontend Web

La carpeta `frontend/` contiene una interfaz web estatica para operar el rack desde la API REST.

## Flujo

```text
Frontend -> API REST -> HiveMQ -> ESP32
Frontend -> API REST -> InfluxDB
Frontend -> Grafana
```

## Vistas

| Vista | Contenido |
|---|---|
| Resumen | Estado general, motivo, red, temperatura, humedad y posicion |
| Modo | Modo activo y cambio entre `standard`, `energy_saving`, `performance` |
| Desbloquear puerta | Estado fisico, bloqueo, seguridad, formulario de desbloqueo y cierre |
| Ventilador | Modo automatico/manual y potencia |
| Historial | Enlace a Grafana y documentacion de API |

## Ejecucion

1. Ejecutar el bridge Python.
2. Ejecutar la API REST en `http://127.0.0.1:8000`.
3. Servir el frontend:

```bash
cd frontend
python -m http.server 5173
```

4. Abrir:

```text
http://127.0.0.1:5173
```

## Validacion

- `Actualizar` debe traer datos desde `/api/rack/status`.
- Cambiar modo debe modificar `control/mode` en MQTT.
- Ventilador manual debe publicar `commands/fan/manual`.
- Desbloqueo correcto debe publicar `commands/door/unlock`.
- Si la puerta se abre despues del desbloqueo correcto, el motivo debe ser `door_open_authorized` y el estado general debe quedar en `warning`.
- Si la puerta se abre sin desbloqueo previo, el motivo debe ser `door_open_forced` y el estado general debe quedar en `critical`.
- Desbloqueo incorrecto debe mostrar error de API.
