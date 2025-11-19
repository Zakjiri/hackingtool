const audioPlayer = document.getElementById('audioPlayer');
const photoPreview = document.getElementById('photoPreview');
const videoView = document.getElementById('videoStream');

const statusEls = {
  recState: document.getElementById('recState'),
  recTime: document.getElementById('recTime'),
  audioStream: document.getElementById('audioStreamState'),
  videoStream: document.getElementById('videoStreamState'),
  battery: document.getElementById('battery'),
  wifiIp: document.getElementById('wifiIp'),
  apIp: document.getElementById('apIp'),
  sd: document.getElementById('sdState'),
  device: document.getElementById('deviceName')
};

let settingsInitialized = false;

async function loadStatus() {
  const res = await fetch('/api/status');
  if (!res.ok) return;
  const data = await res.json();
  statusEls.recState.textContent = data.recording ? 'YES' : 'NO';
  statusEls.recTime.textContent = data.record_seconds;
  statusEls.audioStream.textContent = data.audio_streaming ? 'ON' : 'OFF';
  statusEls.videoStream.textContent = data.video_streaming ? 'ON' : 'OFF';
  statusEls.battery.textContent = data.battery >= 0 ? data.battery + '%' : 'N/A';
  statusEls.wifiIp.textContent = data.wifi_ip;
  statusEls.apIp.textContent = data.ap_ip;
  statusEls.sd.textContent = data.sd ? 'OK' : 'Missing';
  statusEls.device.textContent = data.device;
  if (data.config && !settingsInitialized) {
    Object.entries(data.config).forEach(([key, value]) => {
      const input = form.querySelector(`[name=${key}]`);
      if (input) input.value = value;
    });
    settingsInitialized = true;
  }
}

async function control(action) {
  let url = '';
  switch (action) {
    case 'start-audio':
      url = '/api/record/start';
      break;
    case 'stop-audio':
      url = '/api/record/stop';
      break;
    case 'photo':
      url = '/api/photo';
      break;
    case 'start-audio-stream':
      await fetch('/api/stream/audio/start', { method: 'POST' });
      audioPlayer.src = `/audio_stream?ts=${Date.now()}`;
      audioPlayer.play();
      loadStatus();
      return;
    case 'stop-audio-stream':
      await fetch('/api/stream/audio/stop', { method: 'POST' });
      audioPlayer.pause();
      audioPlayer.removeAttribute('src');
      audioPlayer.load();
      loadStatus();
      return;
    case 'start-video-stream':
      await fetch('/api/stream/video/start', { method: 'POST' });
      videoView.src = `/video_stream?ts=${Date.now()}`;
      loadStatus();
      return;
    case 'stop-video-stream':
      await fetch('/api/stream/video/stop', { method: 'POST' });
      videoView.removeAttribute('src');
      loadStatus();
      return;
  }
  if (!url) return;
  const res = await fetch(url, { method: 'POST' });
  if (res.ok && action === 'photo') {
    const text = await res.text();
    photoPreview.src = '/download?path=' + encodeURIComponent(text);
  }
  loadStatus();
  listFiles(currentList);
}

document.querySelectorAll('button[data-action]').forEach(btn => {
  btn.addEventListener('click', () => control(btn.dataset.action));
});

let currentList = 'audio';

async function listFiles(type) {
  currentList = type;
  const res = await fetch(`/api/files?type=${type}`);
  if (!res.ok) return;
  const data = await res.json();
  const list = document.getElementById('fileList');
  list.innerHTML = '';
  data.files.forEach(file => {
    const li = document.createElement('li');
    const date = file.modified ? new Date(file.modified * 1000).toLocaleString() : 'n/a';
    li.textContent = `${file.name} (${file.size} bytes, ${date})`;
    const dl = document.createElement('a');
    dl.textContent = 'Download';
    dl.href = `/download?path=${encodeURIComponent(file.name)}`;
    dl.target = '_blank';
    const del = document.createElement('button');
    del.textContent = 'Delete';
    del.addEventListener('click', async () => {
      await fetch(`/api/file?path=${encodeURIComponent(file.name)}`, { method: 'DELETE' });
      listFiles(type);
    });
    li.appendChild(dl);
    li.appendChild(del);
    list.appendChild(li);
  });
}

document.querySelectorAll('.tabs button').forEach(btn => {
  btn.addEventListener('click', () => listFiles(btn.dataset.list));
});

const form = document.getElementById('settingsForm');
form.addEventListener('submit', async e => {
  e.preventDefault();
  const formData = new FormData(form);
  const payload = {};
  formData.forEach((value, key) => payload[key] = value);
  await fetch('/api/settings', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
  alert('Saved. Reboot to apply Wi-Fi changes.');
});

setInterval(loadStatus, 3000);
loadStatus();
listFiles('audio');
