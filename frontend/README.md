# Frontend Del Rack

Interfaz web estatica para operar la API REST del proyecto.

## Ejecucion

Con la API REST corriendo en `http://127.0.0.1:8000`, servir esta carpeta en el puerto `5173`:

```bash
cd frontend
python -m http.server 5173
```

Tambien se puede usar el helper incluido:

```bash
python serve.py
```

Abrir:

```text
http://127.0.0.1:5173
```

## Funciones

- Ver estado general del rack.
- Cambiar modo `standard`, `energy_saving` o `performance`.
- Ver y controlar ventilador.
- Bloquear y desbloquear puerta.
- Abrir Grafana o la documentacion `/docs` de la API.

## Configuracion

La URL de la API se puede cambiar desde el campo superior de la interfaz. El valor se guarda en `localStorage`.
