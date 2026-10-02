let lastOutput = '';

function esc(s) { return s; }
function setText(id, value) { document.getElementById(id).textContent = value; }
function mark(id, state) { const e = document.getElementById(id); e.className = state || ''; }

function render(s) {
  const badge = document.getElementById('statusBadge');
  badge.className = 'badge ' + s.status;
  badge.textContent = s.status === 'passed' ? 'PASS' : s.status === 'failed' ? 'FAILED' : s.status === 'running' ? 'RUNNING' : 'READY';
  document.getElementById('runBtn').disabled = s.running;
  setText('phase', s.phase);
  setText('subphase', s.running ? 'Validation is executing against the repository test scripts.' : (s.status === 'passed' ? 'Validation completed successfully.' : 'The dashboard executes the repository’s real validation flow.'));
  setText('elapsed', (s.elapsed_s || 0).toFixed(1) + 's');

  const m = s.tests || {};
  setText('ctest', m.ctest_total != null ? `${m.ctest_passed}/${m.ctest_total}` : '—');
  setText('pytest', m.pytest_passed != null ? `${m.pytest_passed} passed` : '—');
  setText('bridge', m.bridge_pass ? 'PASS' : s.running ? 'RUNNING' : '—');
  setText('qemu', m.qemu_build ? 'PASS' : (s.running ? 'RUNNING' : '—'));

  const o = s.output || '';
  const phases = [
    /cmake.*-B build/i.test(o),
    /cmake --build|Built target/i.test(o),
    /tests passed|Test #[0-9]+/i.test(o),
    /pytest|[0-9]+ passed/i.test(o),
    /complete host regression \+ peripheral socket bridge/i.test(o),
    /Cross-compiled QEMU demo successfully\./i.test(o),
  ];
  phases.forEach((done, i) => mark('f' + (i+1), done ? 'done' : ''));
  if (s.running) {
    const p = Math.max(0, phases.findIndex(x => !x));
    mark('f' + (p + 1), 'active');
  }
  const log = document.getElementById('log');
  if (o !== lastOutput) { log.textContent = o || 'Waiting for a validation run…'; log.scrollTop = log.scrollHeight; lastOutput = o; }
}

async function refresh() {
  try { const r = await fetch('/api/status', {cache:'no-store'}); render(await r.json()); } catch(e) { setText('phase', 'Dashboard server unavailable'); }
}
async function runValidation() {
  const r = await fetch('/api/run', {method:'POST'});
  if (!r.ok) { alert('A validation run is already active.'); return; }
  lastOutput = '';
  await refresh();
}
function clearLog() { document.getElementById('log').textContent = 'Log view cleared. The next refresh will restore the live pipeline output.'; lastOutput = ''; }
refresh();
setInterval(refresh, 700);
