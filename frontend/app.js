const API_BASE = 'http://localhost:8000';

function updateStatus(msg) {
    const bar = document.getElementById('statusBar');
    const btn = document.getElementById('clearStatusBtn');
    bar.textContent = msg;
    if (msg && msg !== 'Ready') {
        btn.style.display = 'block';
        bar.title = msg; // Show full text on hover
    } else {
        btn.style.display = 'none';
        bar.title = '';
    }
}

function clearStatus() {
    updateStatus('Ready');
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

async function runTag() {
    const name = document.getElementById('tagName').value;
    const commitId = document.getElementById('checkoutTarget').value; // Reuse checkout input for tag target
    if (!name) return updateStatus('Enter tag name');
    const res = await apiCall('/tag', 'POST', { name, commitId });
    if (res) {
        updateStatus(res.output || res.stderr);
        loadHistory();
    }
}

async function runStash(action) {
    const res = await apiCall('/stash', 'POST', { action });
    if (res) {
        updateStatus(res.output || res.stderr);
        runStatus();
    }
}

async function runReset() {
    const commitId = document.getElementById('resetTarget').value;
    if (!commitId) return updateStatus('Enter commit ID to reset to');
    const res = await apiCall('/reset', 'POST', { commitId, hard: true });
    if (res) {
        updateStatus(res.output || res.stderr);
        loadHistory();
        runStatus();
    }
}

async function runStatus() {
    const res = await apiCall('/status');
    if (res) {
        document.getElementById('statusOutput').textContent = res.output || res.stderr;
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
        const data = JSON.parse(res.output);
        renderCommitList(data);
        if (document.getElementById('graph-view').style.display === 'block') {
            renderVisualGraph(data);
        }
    } catch (e) {
        console.error("Failed to parse graph JSON", e);
    }
}

function renderCommitList(commits) {
    const list = document.getElementById('commitList');
    list.innerHTML = '';

    try {
        if (commits.length === 0) return;

        commits.forEach(commit => {
            const date = new Date(parseInt(commit.timestamp) * 1000).toLocaleString();
            
            const card = document.createElement('div');
            card.className = 'commit-card';
            
            let branchBadges = (commit.branches || []).map(b => `<span class="branch-badge">${b}</span>`).join(' ');
            let tagBadges = (commit.tags || []).map(t => `<span class="tag-badge">${t}</span>`).join(' ');
            
            card.innerHTML = `
                <div class="commit-header">
                    <span class="commit-id">${commit.id.substring(0, 8)}</span>
                    ${branchBadges}
                    ${tagBadges}
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

let visualNetwork = null;

function renderVisualGraph(data) {
    const container = document.getElementById('visualGraph');
    const nodes = [];
    const edges = [];

    data.forEach(commit => {
        let color = '#21262d'; 
        let label = commit.id.substr(0, 7);
        
        if (commit.branches && commit.branches.length > 0) {
            color = '#58a6ff'; 
            label += `\n(${commit.branches.join(', ')})`;
        }
        
        if (commit.tags && commit.tags.length > 0) {
            color = '#3fb950'; 
            label += `\n[${commit.tags.join(', ')}]`;
        }

        nodes.push({
            id: commit.id,
            label: label,
            title: `Message: ${commit.message}\nDate: ${commit.timestamp}`,
            color: { background: color, border: '#30363d' },
            font: { color: '#c9d1d9', size: 11 }
        });

        if (commit.parent && commit.parent !== 'null' && commit.parent !== '') {
            edges.push({
                from: commit.id,
                to: commit.parent,
                arrows: 'to',
                color: '#30363d'
            });
        }
    });

    const graphData = { nodes: new vis.DataSet(nodes), edges: new vis.DataSet(edges) };
    const options = {
        layout: { hierarchical: { direction: 'LR', sortMethod: 'directed', levelSeparation: 150 } },
        physics: false,
        interaction: { hover: true, tooltipDelay: 200 }
    };

    if (visualNetwork) visualNetwork.destroy();
    visualNetwork = new vis.Network(container, graphData, options);
}

function switchTab(tabId) {
    document.querySelectorAll('.view').forEach(v => v.style.display = 'none');
    document.getElementById(`${tabId}-view`).style.display = 'block';
    
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    // Find the button that calls switchTab with this tabId
    const activeTab = document.querySelector(`button[onclick="switchTab('${tabId}')"]`);
    if (activeTab) activeTab.classList.add('active');

    if (tabId === 'history' || tabId === 'graph') loadHistory();
    if (tabId === 'status') runStatus();
}

// Initial load
loadHistory();
runStatus();

function toggleGuide() {
    const modal = document.getElementById('guideModal');
    if (modal.style.display === 'block') {
        modal.style.display = 'none';
    } else {
        modal.style.display = 'block';
    }
}

// Close modal when clicking outside
window.onclick = function(event) {
    const modal = document.getElementById('guideModal');
    if (event.target == modal) {
        modal.style.display = 'none';
    }
}
