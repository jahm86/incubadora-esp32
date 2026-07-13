document.addEventListener('DOMContentLoaded', function() {
    loadConfig();

    document.getElementById('use-static-ip').addEventListener('change', function() {
        document.getElementById('static-ip-fields').classList.toggle('hidden', !this.checked);
    });

    document.getElementById('mqtt-tls').addEventListener('change', function() {
        document.getElementById('tls-fields').classList.toggle('hidden', !this.checked);
    });

    document.getElementById('save-btn').addEventListener('click', saveAll);
});

async function loadConfig() {
    try {
        const res = await fetch('/api/config');
        const data = await res.json();

        setValue('ssid', data.ssid);
        setValue('mqtt-server', data.mqtt_server);
        setValue('mqtt-port', data.mqtt_port);
        setValue('mqtt-user', data.mqtt_user);
        setValue('mqtt-password', data.mqtt_password);
        setChecked('mqtt-tls', data.mqtt_tls);
        if (data.mqtt_tls) document.getElementById('tls-fields').classList.remove('hidden');
        setValue('mqtt-cert', data.mqtt_cert);
        setValue('temp-offset', data.temp_offset);
        setValue('hum-offset', data.hum_offset);
        setValue('setpoint', data.setpoint);
        setValue('temp-alarm-high', data.temp_alarm_high);
        setValue('temp-alarm-low', data.temp_alarm_low);
        setValue('hum-on', data.hum_on);
        setValue('hum-off', data.hum_off);
        setValue('hum-alarm-high', data.hum_alarm_high);
        setValue('hum-alarm-low', data.hum_alarm_low);
        setValue('turn-interval', data.turn_interval);
        setValue('turn-duration', data.turn_duration);
        setValue('controller-type', data.controller_type);
    } catch (e) {
        showStatus('Error cargando configuración', 'error');
    }
}

async function saveAll() {
    const params = new URLSearchParams();
    params.append('ssid', getValue('ssid'));
    params.append('password', getValue('password'));

    try {
        const res = await fetch('/api/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: params.toString()
        });
        const data = await res.json();

        if (data.status === 'ok') {
            showStatus('Configuración guardada. Reiniciando...', 'success');
            setTimeout(() => {
                fetch('/api/restart', { method: 'POST' });
            }, 1000);
        } else {
            showStatus('Error guardando configuración', 'error');
        }
    } catch (e) {
        showStatus('Error de conexión', 'error');
    }
}

function getValue(id) {
    const el = document.getElementById(id);
    return el ? el.value : '';
}

function setValue(id, val) {
    const el = document.getElementById(id);
    if (el && val !== undefined) el.value = val;
}

function setChecked(id, val) {
    const el = document.getElementById(id);
    if (el) el.checked = !!val;
}

function showStatus(msg, type) {
    const el = document.getElementById('status');
    el.textContent = msg;
    el.className = type;
    el.classList.remove('hidden');
}
