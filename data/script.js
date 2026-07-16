document.addEventListener('DOMContentLoaded', function() {
    loadConfig();

    document.getElementById('use-static-ip').addEventListener('change', function() {
        document.getElementById('static-ip-fields').classList.toggle('hidden', !this.checked);
    });

    document.getElementById('mqtt-tls').addEventListener('change', function() {
        document.getElementById('tls-fields').classList.toggle('hidden', !this.checked);
    });

    document.getElementById('controller-type').addEventListener('change', function() {
        showControllerParams(this.value);
    });

    document.getElementById('save-btn').addEventListener('click', saveAll);
});

async function loadConfig() {
    try {
        const res = await fetch('/api/config');
        const data = await res.json();

        setValue('ssid', data.ssid);
        setValue('static-ip', data.static_ip);
        setValue('static-gateway', data.static_gateway);
        setValue('static-netmask', data.static_netmask);
        setChecked('use-static-ip', data.use_static_ip);
        if (data.use_static_ip) document.getElementById('static-ip-fields').classList.remove('hidden');
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
        setValue('kp', data.kp);
        setValue('ki', data.ki);
        setValue('kd', data.kd);
        setValue('hysteresis', data.hysteresis);
        setValue('b0', data.b0);
        setValue('wc', data.wc);
        setValue('wo', data.wo);
        showControllerParams(data.controller_type);
    } catch (e) {
        showStatus('Error cargando configuración', 'error');
    }
}

const PARAM_MAP = {
    'static-ip': 'static_ip', 'static-gateway': 'static_gateway',
    'static-netmask': 'static_netmask', 'use-static-ip': 'use_static_ip',
    'mqtt-server': 'mqtt_server', 'mqtt-port': 'mqtt_port',
    'mqtt-user': 'mqtt_user', 'mqtt-password': 'mqtt_password',
    'mqtt-tls': 'mqtt_tls', 'mqtt-cert': 'mqtt_cert',
    'temp-offset': 'temp_offset', 'hum-offset': 'hum_offset',
    'temp-alarm-high': 'temp_alarm_high', 'temp-alarm-low': 'temp_alarm_low',
    'hum-on': 'hum_on', 'hum-off': 'hum_off',
    'hum-alarm-high': 'hum_alarm_high', 'hum-alarm-low': 'hum_alarm_low',
    'turn-interval': 'turn_interval', 'turn-duration': 'turn_duration',
    'controller-type': 'controller_type'
};

async function saveAll() {
    const fields = [
        'ssid', 'password',
        'static-ip', 'static-gateway', 'static-netmask', 'use-static-ip',
        'mqtt-server', 'mqtt-port', 'mqtt-user', 'mqtt-password',
        'mqtt-tls', 'mqtt-cert',
        'temp-offset', 'hum-offset', 'setpoint',
        'temp-alarm-high', 'temp-alarm-low',
        'hum-on', 'hum-off', 'hum-alarm-high', 'hum-alarm-low',
        'turn-interval', 'turn-duration', 'controller-type',
        'kp', 'ki', 'kd', 'hysteresis', 'b0', 'wc', 'wo'
    ];

    const params = new URLSearchParams();
    for (const id of fields) {
        const el = document.getElementById(id);
        if (!el) continue;
        const paramName = PARAM_MAP[id] || id;
        const val = el.type === 'checkbox' ? (el.checked ? 'true' : 'false') : el.value;
        params.append(paramName, val);
    }

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

function showControllerParams(type) {
    document.getElementById('ctrl-hysteresis').classList.toggle('hidden', type !== '0');
    document.getElementById('ctrl-pid').classList.toggle('hidden', type !== '1');
    document.getElementById('ctrl-ladrc').classList.toggle('hidden', type !== '2');
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
