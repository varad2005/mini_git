# 🔀 Mini Git & Mini GitHub

> **A fully functional, miniature version control system built from scratch using C++, Python, and JavaScript — demonstrating how Git & GitHub actually work under the hood.**

---

## 📌 Problem Statement

Version control systems like **Git** are among the most critical tools in modern software development. Yet most developers use Git as a "black box" without understanding the core **data structures and algorithms** that power it.

**Mini Git** solves this by implementing a complete version control system from first principles — including **content-addressable storage**, **DAG-based commit history**, **LCS-based diffing**, and **branching** — all in under 700 lines of C++ code. **Mini GitHub** provides a sleek web-based visualization layer on top.

---

## ✨ Key Features

| Feature | Description | Algorithm / DSA Used |
|:--------|:------------|:---------------------|
| **Initialize** | Create a new repository to start tracking files | File system tree creation |
| **Stage Files** | Prepare files for the next snapshot (`add` / `add .`) | Recursive directory traversal, Hashing |
| **Commit** | Save a permanent, immutable snapshot of staged files | **Content-Addressable Storage** (Hash-based object store) |
| **Log** | View the complete timeline of all snapshots | **DAG Traversal** (Directed Acyclic Graph) |
| **Status** | See modified, staged, or new files | Hash comparison + **.mygitignore** support |
| **Branching** | Create parallel development paths | Pointer-based branching (branch → commit ID) |
| **Checkout** | Restore the workspace to any version | Tree reconstruction + safe file cleanup |
| **Diff** | Compare any two versions line-by-line | **LCS Algorithm** (Longest Common Subsequence) |
| **Graph** | Visual commit DAG for web interface | **BFS** (Breadth-First Search) over history |
| **Merge** | Combine changes from two branches | **Three-Way Merge** (LCA + Content Merge) |
| **Tags** | Name a commit for easy reference | Persistent Hash Map (name → ID) |
| **Stash** | Temporarily save workspace changes | **Stack (LIFO)** state management |
| **Reset** | Move branch pointer to an old state | Pointer manipulation (reset --hard) |
| **Ignoring** | Ignore specific files/folders | Pattern matching (.mygitignore) |
| **UI** | 100% Visual Control | Integrated Web Dashboard (Mini GitHub) |

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     USER INTERACTION                        │
│                                                             │
│    ┌──────────────┐              ┌──────────────────┐       │
│    │  Terminal     │              │  Web Browser     │       │
│    │  ./mygit ...  │              │  localhost:8000   │       │
│    └──────┬───────┘              └────────┬─────────┘       │
│           │                               │                  │
│           │ direct                        │ HTTP             │
│           │                               │ (JSON)           │
│           ▼                               ▼                  │
│    ┌─────────────────────────────────────────────────┐       │
│    │            BRIDGE SERVER (Python)               │       │
│    │     server.py — HTTP Server + API Router        │       │
│    │     Translates REST calls → CLI commands         │       │
│    └──────────────────────┬──────────────────────────┘       │
│                           │                                  │
│                           │ subprocess                       │
│                           ▼                                  │
│    ┌─────────────────────────────────────────────────┐       │
│    │          BACKEND ENGINE (C++17)                  │       │
│    │                                                  │       │
│    │   Repository.cpp      Diff.cpp                   │       │
│    │   ┌──────────────┐    ┌────────────────┐        │       │
│    │   │ init()       │    │ compare()      │        │       │
│    │   │ add()        │    │ (LCS Algorithm)│        │       │
│    │   │ commit()     │    └────────────────┘        │       │
│    │   │ checkout()   │                               │       │
│    │   │ status()     │    .mygit/                     │       │
│    │   │ log()        │    ├── HEAD                    │       │
│    │   │ branch()     │    ├── index                   │       │
│    │   │ diff()       │    ├── branches/               │       │
│    │   │ getGraph()   │    ├── commits/{id}/           │       │
│    │   └──────────────┘    │   ├── metadata.txt        │       │
│    │                       │   └── tree.txt             │       │
│    │                       └── objects/{hash}           │       │
│    └─────────────────────────────────────────────────┘       │
└─────────────────────────────────────────────────────────────┘
```

---

## 🧠 Data Structures & Algorithms Used

### 1. Content-Addressable Storage (Hash-Based Object Store)
Every file's content is hashed using the **DJB2 algorithm** and stored in `.mygit/objects/`. If two files have the same content, they share the same hash — **zero duplication**.

```
File Content: "hello world"  →  Hash: 310f23888a  →  Stored at: .mygit/objects/310f23888a
```

### 2. Directed Acyclic Graph (DAG) — Commit History
Each commit stores a pointer to its **parent commit**, forming a chain. Branches create forks in this chain. This is the exact same structure Git uses internally.

```
     [commit1] ← [commit2] ← [commit3]   (master)
                      ↖
                       [commit4]          (feature-branch)
```

### 3. Longest Common Subsequence (LCS) — Diff Engine
The `diff` command uses a classic **dynamic programming** approach to find the longest common subsequence between two file versions, then highlights additions (`+`) and deletions (`-`).

```
Time Complexity:  O(m × n)
Space Complexity: O(m × n)
Where m, n = number of lines in each file version
```

### 4. Breadth-First Search (BFS) — Graph Export
The `graph` command uses BFS starting from all branch tips and HEAD to traverse the entire commit DAG and export it as JSON for the visual graph renderer.

### 5. Triple-Way Merge (LCA Algorithm)
The `merge` command finds the **Lowest Common Ancestor (LCA)** of two branches and performs a content-level merge, intelligently combining changes or flagging conflicts.

---

## 🚀 How to Run

### Prerequisites
| Tool | Version | Purpose |
|:-----|:--------|:--------|
| `g++` | C++17 support | Compile the backend engine |
| `Python 3` | 3.6+ | Run the bridge server |
| `Web Browser` | Any modern | View the Mini GitHub interface |

### Step 1: Build the Backend
```bash
cd backend
make clean && make
```

### Step 2: Launch the Web Interface
```bash
cd ..
python3 bridge/server.py
```
Then open **http://localhost:8000** in your browser.

### Step 3: Use via Terminal (Optional)
```bash
cd backend
./mygit           # Show help
./mygit init      # Initialize repository
./mygit add .     # Stage all files
./mygit commit "first version"
./mygit log       # View history
```

---

## 📖 Complete Command Reference

### `./mygit init`
> **Purpose:** Start tracking a folder for version control.

Creates a hidden `.mygit/` directory containing:
- `HEAD` — points to the current branch
- `index` — the staging area
- `branches/` — branch pointers
- `commits/` — stored snapshots
- `objects/` — content-addressable file store

```bash
$ ./mygit init
Initialized empty MyGit repository in /home/user/project/.mygit/
```

---

### `./mygit status`
> **Purpose:** Check what has changed since the last snapshot.

Compares three areas:
1. **Staged (Index vs HEAD)** — files ready to be committed
2. **Unstaged (Workspace vs Index)** — changes you haven't staged yet
3. **Untracked** — new files not being tracked

```bash
$ ./mygit status
On branch master

Changes to be committed:
  (new file)  main.cpp
  (modified)  utils.h

Changes not staged for commit:
  (modified)  README.md

Untracked files:
  notes.txt
```

---

### `./mygit add <file>` or `./mygit add .`
> **Purpose:** Prepare files for the next snapshot.

- `add <file>` — stages a single file
- `add .` — stages **all** non-ignored files recursively

The file is hashed and stored in the object store. The `index` file is updated.

```bash
$ ./mygit add main.cpp
Added main.cpp to staging area.

$ ./mygit add .
Added 12 files to staging area.
```

---

### `./mygit commit "<message>"`
> **Purpose:** Save a permanent snapshot of all staged files.

Creates:
- A unique **commit ID** (hash of message + parent + timestamp)
- `metadata.txt` — stores message, parent ID, timestamp
- `tree.txt` — maps filenames to their content hashes

```bash
$ ./mygit commit "Add login feature"
[master a3f8b2c1] Add login feature
```

---

### `./mygit log`
> **Purpose:** View the full timeline of saved snapshots.

Traverses the DAG from HEAD backwards through parent pointers.

```bash
$ ./mygit log
commit a3f8b2c1e4567890
Date: 2026-04-18 20:06:31

    Add login feature

commit 559e0007401acdd7
Date: 2026-04-12 19:49:25

    Initial workspace setup
```

---

### `./mygit branch <name>`
> **Purpose:** Create a new development path.

A branch is simply a file in `.mygit/branches/` that contains a commit ID. Creating a branch is O(1).

```bash
$ ./mygit branch feature-login
Created branch 'feature-login' at commit a3f8b2c1
```

---

### `./mygit checkout <branch|commit-id>`
> **Purpose:** Jump to a different version or branch.

This performs a **safe checkout**:
1. Reads the target commit's file tree
2. Restores all files from the object store
3. **Deletes** any files that exist in the current version but NOT in the target
4. Updates the index to match

```bash
$ ./mygit checkout feature-login
Switched to branch 'feature-login'

$ ./mygit checkout 559e0007
Switched to detached HEAD at 559e0007401acdd7
Removed main.cpp (not in target commit)
```

---

### `./mygit diff <commit1> <commit2>`
> **Purpose:** See exactly what changed between two versions.

Uses LCS (Longest Common Subsequence) to produce a colored diff:
- 🟢 `+ line` = added in commit2
- 🔴 `- line` = removed from commit1

```bash
$ ./mygit diff 559e0007 a3f8b2c1
@@ hunk start near line 5 @@
- old_function()
+ new_function()
+ added_line()
```

---

### `./mygit merge <target>`
> **Purpose:** Combine changes from another branch into your current branch.

Performs a **Three-Way Merge**:
1. Finds the Lowest Common Ancestor (LCA).
2. Merges changes from the target branch.
3. If both changed the same file, it inserts **Conflict Markers**.

```bash
$ ./mygit merge feature-login
Auto-merging...
CONFLICT (content): Merge conflict in utils.h
Automatic merge failed; fix conflicts and then commit.
```

---

### `./mygit tag <name>`
> **Purpose:** Give a commit a permanent name (v1.0, release-ready).

```bash
$ ./mygit tag v1.0
Created tag 'v1.0' at commit a3f8b2c1
```

---

### `./mygit stash push/pop`
> **Purpose:** "Pause" your work and save it for later without committing.

Uses a **Stack** to store your workspace state.

```bash
$ ./mygit stash push
Saved working directory and index state WIP on stash@{0}
```

---

### `./mygit reset <commit-id> [--hard]`
> **Purpose:** Undo changes by moving your branch pointer back in time.

```bash
$ ./mygit reset a3f8b2c1 --hard
HEAD is now at a3f8b2c1 (Add login feature)
```

---

### `./mygit graph`
> **Purpose:** Export the commit DAG as JSON for the web interface.

Uses BFS to traverse all reachable commits and outputs structured JSON with commit IDs, messages, timestamps, parents, and branch associations.

---

## 🎨 Web Interface (Mini GitHub)

The web interface at `http://localhost:8000` provides three views:

| Tab | What It Shows |
|:----|:--------------|
| **Commit History** | Visual timeline of all snapshots with branch labels, tags, and parent links. |
| **Graph Visualization**| **Dynamic DAG Rendering** with multi-parent merge lines and real-time layout. |
| **Workspace Status** | Colored live view of staged, unstaged, and untracked files. |
| **Diff Viewer** | Line-by-line comparison highlighting additions and deletions. |

### UI Features
- 🌙 **Dark theme** with GitHub-inspired design
- 📖 **Built-in Guide** — click "How to Use" for a step-by-step walkthrough
- 💡 **Hint text** under every button explaining what it does
- 🎯 **One-click commit selection** — click any commit card to auto-fill diff inputs

---

## 📁 Project Structure

```
mini-git/
├── backend/                    # C++ Engine (Core Logic)
│   ├── include/
│   │   ├── Repository.h        # Repository class declaration
│   │   └── Diff.h              # Diff engine declaration
│   ├── src/
│   │   ├── main.cpp            # CLI entry point
│   │   ├── Repository.cpp      # All version control logic
│   │   └── Diff.cpp            # LCS-based diff algorithm
│   └── Makefile                # Build configuration
│
├── bridge/                     # Python Bridge (API Layer)
│   └── server.py               # HTTP server + REST API router
│
├── frontend/                   # Web Interface (Mini GitHub)
│   ├── index.html              # Page structure + Guide modal
│   ├── style.css               # Dark theme + responsive design
│   └── app.js                  # API calls + dynamic rendering
│
├── simple-git-snapshot/        # Earlier prototype (reference)
│   └── main.cpp                # Standalone snapshot system
│
└── README.md                   # This file
```

---

## 🔬 How It Works Internally

### The Three Areas (Just Like Real Git)

```
  ┌──────────────┐     add      ┌──────────────┐    commit    ┌──────────────┐
  │  WORKSPACE   │  ────────►   │    INDEX      │  ────────►   │   COMMIT     │
  │  (Your Files)│              │ (Staging Area)│              │  (Snapshot)  │
  └──────────────┘              └──────────────┘              └──────────────┘

  What you see           What's prepared           What's saved
  in your folder         for the next save         permanently
```

### Object Store — Content Deduplication
```
workspace/notes.md  ──hash──►  8c9381232b  ──store──►  .mygit/objects/8c9381232b
workspace/setup.txt ──hash──►  e741165d49  ──store──►  .mygit/objects/e741165d49

If two files have identical content → same hash → stored only ONCE
```

### Commit Chain — DAG Structure
```
metadata.txt contains:
    message: "Initial setup"
    parent:  (none)              ← Root commit
    timestamp: 1712923765

tree.txt contains:
    notes.md 8c9381232b          ← filename → content hash mapping
    setup.txt e741165d49
```

---

## 🏆 Competition Highlights

| Criteria | Our Implementation |
|:---------|:-------------------|
| **Correctness** | Full snapshot-based VCS with safe checkout (deletes ghost files) |
| **DSA Applied** | LCS (diff), DAG (commits), BFS (graph), Hashing (objects) |
| **Architecture** | Clean 3-tier: Engine → API → UI separation |
| **UI/UX** | Dark-themed web dashboard with built-in guide and hints |
| **Code Quality** | Modular C++17, clear separation of concerns, ~700 LOC backend |
| **Real-World Relevance** | Directly mirrors how Git works internally |

---

## 👥 Team

| Name | Role |
|:-----|:-----|
| Varad | Lead Developer — Backend Engine, Architecture |
| | |
| | |

---

## 📜 License

This project is built for educational and competition purposes. Feel free to study and learn from the source code.

---

<p align="center">
  <b>Built with ❤️ using C++17, Python 3, and Vanilla JavaScript</b>
</p>
