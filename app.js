const $ = (selector) => document.querySelector(selector);
const list = $('#alarm-list');
const dialog = $('#alarm-dialog');
const form = $('#alarm-form');
const STORE_KEY = 'tempo-alarms-v1';
let alarms = loadAlarms();
let toastTimer;
let activeRing = null;
const pendingRings = [];

function loadAlarms() {
  try {
    const value = JSON.parse(localStorage.getItem(STORE_KEY) || '[]');
    return Array.isArray(value) ? value.filter((alarm) => alarm && typeof alarm.id === 'string' && /^\d{2}:\d{2}$/.test(alarm.time)) : [];
  } catch { return []; }
}
function save() { localStorage.setItem(STORE_KEY, JSON.stringify(alarms)); }
function updateClock() {
  const now = new Date();
  const time = new Intl.DateTimeFormat(undefined, { hour: '2-digit', minute: '2-digit', hour12: false }).format(now);
  const seconds = new Intl.DateTimeFormat(undefined, { second: '2-digit' }).format(now);
  $('#digital-clock').innerHTML = `${time}<span>${seconds}</span>`;
  $('#today-date').textContent = new Intl.DateTimeFormat(undefined, { weekday: 'long', month: 'long', day: 'numeric' }).format(now);
  $('#timezone').textContent = Intl.DateTimeFormat().resolvedOptions().timeZone.replaceAll('_', ' ').toUpperCase();
}
function render() {
  alarms.sort((a, b) => a.time.localeCompare(b.time));
  list.replaceChildren();
  $('#empty-state').hidden = alarms.length > 0;
  $('#nav-count').textContent = alarms.filter((a) => a.enabled).length;
  $('#heading-count').textContent = alarms.length;
  for (const alarm of alarms) {
    const [hour, minute] = alarm.time.split(':').map(Number);
    const time = new Date(); time.setHours(hour, minute, 0, 0);
    const row = document.createElement('article'); row.className = 'alarm-row'; row.dataset.id = alarm.id;
    if (activeRing?.alarm.id === alarm.id) row.classList.add('ringing');
    const bell = document.createElement('div'); bell.className = 'alarm-bell'; bell.textContent = '♩';
    const details = document.createElement('div');
    const displayTime = document.createElement('div'); displayTime.className = 'alarm-time';
    displayTime.textContent = new Intl.DateTimeFormat(undefined, { hour: '2-digit', minute: '2-digit' }).format(time);
    const meta = document.createElement('div'); meta.className = 'alarm-meta';
    meta.textContent = !alarm.enabled ? 'Paused' : alarm.snoozedUntil ? `Snoozed · ${Math.max(1, Math.ceil((alarm.snoozedUntil - Date.now()) / 60000))} min` : relativeTime(alarm.time);
    details.append(displayTime, meta);
    const title = document.createElement('div'); title.className = 'alarm-name'; title.textContent = alarm.label || 'Alarm';
    const actions = document.createElement('div'); actions.className = 'row-actions';
    const toggle = document.createElement('input'); toggle.type = 'checkbox'; toggle.className = 'toggle'; toggle.checked = alarm.enabled; toggle.setAttribute('aria-label', `Toggle ${alarm.label || 'alarm'}`);
    toggle.addEventListener('change', () => {
      alarm.enabled = toggle.checked;
      if (!alarm.enabled && activeRing?.alarm.id === alarm.id) stopRinging();
      save(); render();
    });
    const remove = document.createElement('button'); remove.className = 'remove'; remove.type = 'button'; remove.textContent = '×'; remove.setAttribute('aria-label', `Delete ${alarm.label || 'alarm'}`);
    remove.addEventListener('click', () => {
      if (activeRing?.alarm.id === alarm.id) stopRinging();
      alarms = alarms.filter((item) => item.id !== alarm.id); save(); render(); showToast('Alarm removed');
    });
    actions.append(toggle, remove); row.append(bell, details, title, actions); list.append(row);
  }
}
function relativeTime(value) {
  const [hour, minute] = value.split(':').map(Number);
  const now = new Date(), next = new Date(); next.setHours(hour, minute, 0, 0);
  if (next <= now) next.setDate(next.getDate() + 1);
  const minutes = Math.max(1, Math.round((next - now) / 60000));
  return `In ${minutes < 60 ? `${minutes} min` : `${Math.floor(minutes / 60)} hr${Math.floor(minutes / 60) === 1 ? '' : 's'}${minutes % 60 ? ` ${minutes % 60} min` : ''}`}`;
}
function showToast(message) {
  const el = $('#toast'); el.textContent = message; el.classList.add('show');
  clearTimeout(toastTimer); toastTimer = setTimeout(() => el.classList.remove('show'), 3200);
}
function openDialog() {
  form.reset();
  const now = new Date(); now.setMinutes(now.getMinutes() + 1);
  $('#alarm-time').value = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`;
  dialog.showModal(); $('#alarm-label').focus();
}
function stopRinging() {
  if (!activeRing) return;
  if (activeRing.timer) clearInterval(activeRing.timer);
  if (activeRing.context) activeRing.context.close().catch(() => {});
  activeRing = null;
  $('#ring-dialog').close();
  document.querySelectorAll('.alarm-row.ringing').forEach((row) => row.classList.remove('ringing'));
}
function ring(alarm, snoozed = false) {
  if (!alarm.enabled) return;
  if (activeRing) { pendingRings.push({ alarm, snoozed }); return; }
  activeRing = { alarm, context: null, timer: null };
  const row = list.querySelector(`[data-id="${CSS.escape(alarm.id)}"]`);
  if (row) row.classList.add('ringing');
  $('#ring-title').textContent = alarm.label || 'Alarm';
  $('#ring-message').textContent = snoozed ? 'Your five-minute snooze is over.' : 'Your alarm is ringing.';
  if (!$('#ring-dialog').open) $('#ring-dialog').showModal();
  showToast(`${alarm.label || 'Alarm'} — it’s time`);
  if ('Notification' in window && Notification.permission === 'granted') new Notification(alarm.label || 'Tempo alarm', { body: 'Your alarm is ringing. Tap to return to Tempo.' });
  try {
    const context = new (window.AudioContext || window.webkitAudioContext)();
    activeRing.context = context;
    const beep = (frequency) => {
      const oscillator = context.createOscillator(), gain = context.createGain();
      oscillator.type = 'sine'; oscillator.frequency.value = frequency;
      gain.gain.setValueAtTime(0.0001, context.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.15, context.currentTime + 0.025);
      gain.gain.exponentialRampToValueAtTime(0.0001, context.currentTime + 0.24);
      oscillator.connect(gain); gain.connect(context.destination);
      oscillator.start(); oscillator.stop(context.currentTime + 0.25);
    };
    const timer = setInterval(() => { beep(880); setTimeout(() => beep(660), 300); }, 900);
    activeRing.timer = timer;
  } catch { /* The visual alert still works when sound is unavailable. */ }
}
function startNextQueuedRing() {
  while (!activeRing && pendingRings.length) {
    const next = pendingRings.shift();
    if (alarms.includes(next.alarm) && next.alarm.enabled) { ring(next.alarm, next.snoozed); break; }
  }
}
function checkAlarms() {
  const now = new Date(), minuteKey = `${now.getFullYear()}-${now.getMonth()}-${now.getDate()}-${now.getHours()}:${now.getMinutes()}`;
  if (activeRing) return;
  alarms.forEach((alarm) => {
    if (!alarm.enabled) return;
    if (alarm.snoozedUntil) {
      if (Date.now() >= alarm.snoozedUntil) {
        alarm.snoozedUntil = null; save(); ring(alarm, true);
      }
      return;
    }
    if (alarm.time === `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}` && alarm.lastRang !== minuteKey) {
      alarm.lastRang = minuteKey; save(); ring(alarm);
    }
  });
}

$('#open-modal').addEventListener('click', openDialog);
$('#empty-add').addEventListener('click', openDialog);
$('#close-modal').addEventListener('click', () => dialog.close());
$('#cancel-modal').addEventListener('click', () => dialog.close());
dialog.addEventListener('click', (event) => { if (event.target === dialog) dialog.close(); });
$('#stop-alarm').addEventListener('click', () => {
  if (activeRing) { activeRing.alarm.snoozedUntil = null; save(); }
  stopRinging(); startNextQueuedRing(); render(); showToast('Alarm dismissed');
});
$('#snooze-alarm').addEventListener('click', () => {
  if (!activeRing) return;
  const alarm = activeRing.alarm;
  alarm.snoozedUntil = Date.now() + 5 * 60 * 1000;
  stopRinging(); startNextQueuedRing(); save(); render(); showToast('Snoozed for 5 minutes');
});
form.addEventListener('submit', (event) => {
  event.preventDefault();
  const time = $('#alarm-time').value;
  if (!time) return;
  const label = $('#alarm-label').value.trim();
  alarms.push({ id: crypto.randomUUID(), time, label: label || 'Alarm', enabled: true, snoozedUntil: null });
  save(); render(); dialog.close(); showToast('Alarm saved on this device');
  if ('Notification' in window && Notification.permission === 'default') Notification.requestPermission().catch(() => {});
});

updateClock(); render(); checkAlarms();
setInterval(checkAlarms, 1000);
setInterval(() => { updateClock(); render(); }, 15000);
