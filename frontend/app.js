const API_BASE = 'http://localhost:8000';

function updateStatus(msg) {
    document.getElementById('statusBar').textContent = msg;
}

async function apiCall(endpoint, method = 'GET', data = null) {
    try {
        const options = {
            method,
            headers: { 'Content-Type': 'application/json' }
        };
        if (data) options.body = JSON.stringify(data);
        
        const response = await fetch(`${API_BASE}${endpoint}`, options);
        return await response.json();
    } catch (err) {
        console.error(err);
        updateStatus('Error: Backend not reachable');
        return null;
    }
}

async function runInit() {
    const res = await apiCall('/init');
    if (res) {
        updateStatus(res.output || res.stderr);
        loadHistory();
    }
}

async function runAdd() {
    const filename = document.getElementById('filename').value;
    if (!filename) return updateStatus('Enter a filename');
    const res = await apiCall('/add', 'POST', { filename });
    if (res) updateStatus(res.output || res.stderr);
}

async function runCommit() {
    const message = document.getElementById('commitMsg').value;
    if (!message) return updateStatus('Enter a message');
    const res = await apiCall('/commit', 'POST', { message });
    if (res) {
        updateStatus(res.output || res.stderr);
        loadHistory();
    }
}

async function runBranch() {
    const name = document.getElementById('branchName').value;
    if (!name) return updateStatus('Enter branch name');
    const res = await apiCall('/branch', 'POST', { name });
    if (res) updateStatus(res.output || res.stderr);
}

async function runCheckout() {
    const name = document.getElementById('checkoutTarget').value;
    if (!name) return updateStatus('Enter target');
    const res = await apiCall('/checkout', 'POST', { name });
    if (res) {
        updateStatus(res.output || res.stderr);
        loadHistory();
    }
}

async function runDiff() {
    const c1 = document.getElementById('commit1').value;
    const c2 = document.getElementById('commit2').value;
    if (!c1 || !c2) return updateStatus('Enter two commit IDs');
    const res = await apiCall(`/diff/${c1}/${c2}`);
    if (res) {
        renderDiff(res.output);
    }
}

function renderDiff(text) {
    const output = document.getElementById('diffOutput');
    output.innerHTML = '';
    const lines = text.split('\n');
    lines.forEach(line => {
        const div = document.createElement('div');
        if (line.startsWith('+')) div.className = 'diff-add';
        else if (line.startsWith('-')) div.className = 'diff-remove';
        div.textContent = line;
        output.appendChild(div);
    });
}

async function loadHistory() {
    const res = await apiCall('/graph');
    if (!res || !res.output) return;

    const list = document.getElementById('commitList');
    list.innerHTML = '';

    try {
        const commits = JSON.parse(res.output);
        if (commits.length === 0) return;

        commits.forEach(commit => {
            const date = new Date(parseInt(commit.timestamp) * 1000).toLocaleString();
            
            const card = document.createElement('div');
            card.className = 'commit-card';
            
            let branchBadges = (commit.branches || []).map(b => `<span class="branch-badge">${b}</span>`).join(' ');
            
            card.innerHTML = `
                <div class="commit-header">
                    <span class="commit-id">${commit.id.substring(0, 8)}</span>
                    ${branchBadges}
                    <span class="commit-date">${date}</span>
                </div>
                <div class="commit-msg">${commit.message}</div>
                ${commit.parent ? `<div class="commit-parent">Parent: ${commit.parent.substring(0, 8)}</div>` : ''}
            `;
            
            card.onclick = () => {
                if (!document.getElementById('commit1').value) {
                    document.getElementById('commit1').value = commit.id;
                } else {
                    document.getElementById('commit2').value = commit.id;
                }
            };
            list.appendChild(card);
        });
    } catch (e) {
        console.error("Failed to parse graph JSON", e);
    }
}

function switchTab(tabId) {
    document.querySelectorAll('.view').forEach(v => v.style.display = 'none');
    document.getElementById(`${tabId}-view`).style.display = 'block';
    
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    document.querySelector(`[onclick="switchTab('${tabId}')"]`).classList.add('active');
}

// Initial load
loadHistory();
